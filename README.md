# NVIDIA Linux Open GPU with P2P support

This is a fork of NVIDIA's driver with P2P support added for 4090's.

`./install.sh` to install if that's all you want.

You may need to uninstall the driver from DKMS. Your system needs large BAR support and IOMMU off.

Not sure all the cache flushes are right, please file issues on here if you find any issues.

NOTE: This is not a hack, this is using PCIe according to the spec. With cleanups, this could potentially be upstreamed.

## How it works

Normally, P2P on NVIDIA cards uses MAILBOXP2P. This is some hardware interface designed to allow GPUs to transfer memory back in the days of small BAR. It is not present or disabled in hardware on the 4090s, and that's why P2P doesn't work. There [was a bug in early versions](https://forums.developer.nvidia.com/t/standard-nvidia-cuda-tests-fail-with-dual-rtx-4090-linux-box/233202) of the driver that reported that it did work, and it was actually sending stuff on the PCIe bus. However, because the mailbox hardware wasn't present, these copies wouldn't go to the right place. You could even crash the system by doing something like `torch.zeros(10000,10000).cuda().to("cuda:1")`

In some 3090s and all 4090s, NVIDIA added large BAR support.

```
tiny@tiny14:~$ lspci -s 01:00.0 -v
01:00.0 VGA compatible controller: NVIDIA Corporation AD102 [GeForce RTX 4090] (rev a1) (prog-if 00 [VGA controller])
        Subsystem: Micro-Star International Co., Ltd. [MSI] Device 510b
        Physical Slot: 49
        Flags: bus master, fast devsel, latency 0, IRQ 377
        Memory at b2000000 (32-bit, non-prefetchable) [size=16M]
        Memory at 28800000000 (64-bit, prefetchable) [size=32G]
        Memory at 28400000000 (64-bit, prefetchable) [size=32M]
        I/O ports at 3000 [size=128]
        Expansion ROM at b3000000 [virtual] [disabled] [size=512K]
        Capabilities: <access denied>
        Kernel driver in use: nvidia
        Kernel modules: nvidiafb, nouveau, nvidia_drm, nvidia
```

Notice how BAR1 is size 32G. In H100, they also added support for a PCIe mode that uses the BAR directly instead of the mailboxes, called BAR1P2P. So, what happens if we try to enable that on a 4090?

We do this by bypassing the HAL and calling a bunch of the GH100 methods directly. Methods like `kbusEnableStaticBar1Mapping_GH100`, which maps the entire VRAM into BAR1. This mostly just works, but we had to disable the use of that region in the `MapAperture` function for some reason. Shouldn't matter.

```
[ 3491.654009] NVRM: kbusEnableStaticBar1Mapping_GH100: Static bar1 mapped offset 0x0 size 0x5e9200000
[ 3491.793389] NVRM: kbusEnableStaticBar1Mapping_GH100: Static bar1 mapped offset 0x0 size 0x5e9200000
```

Perfect, we now have the VRAM mapped. However, it's not that easy to get P2P. When you run `./simpleP2P` from `cuda-samples`, you get this error.

```
[ 3742.840689] NVRM: kbusCreateP2PMappingForBar1P2P_GH100: added PCIe BAR1 P2P mapping between GPU2 and GPU3
[ 3742.840762] NVRM: kbusCreateP2PMappingForBar1P2P_GH100: added PCIe BAR1 P2P mapping between GPU3 and GPU2
[ 3742.841089] NVRM: nvAssertFailed: Assertion failed: (shifted >> pField->shift) == value @ field_desc.h:272
[ 3742.841106] NVRM: nvAssertFailed: Assertion failed: (shifted & pField->maskPos) == shifted @ field_desc.h:273
[ 3742.841281] NVRM: nvAssertFailed: Assertion failed: (shifted >> pField->shift) == value @ field_desc.h:272
[ 3742.841292] NVRM: nvAssertFailed: Assertion failed: (shifted & pField->maskPos) == shifted @ field_desc.h:273
[ 3742.865948] NVRM: GPU at PCI:0000:01:00: GPU-49c7a6c9-e3a8-3b48-f0ba-171520d77dd1
[ 3742.865956] NVRM: Xid (PCI:0000:01:00): 31, pid=21804, name=simpleP2P, Ch 00000013, intr 00000000. MMU Fault: ENGINE CE3 HUBCLIENT_CE1 faulted @ 0x7f97_94000000. Fault is of type FAULT_INFO_TYPE_UNSUPPORTED_KIND ACCESS_TYPE_VIRT_WRITE
```

Failing with an MMU fault. So you dive into this and find that it's using `GMMU_APERTURE_PEER` as the mapping type. That doesn't seem supported in the 4090. So let's see what types are supported, `GMMU_APERTURE_VIDEO`,`GMMU_APERTURE_SYS_NONCOH`, and `GMMU_APERTURE_SYS_COH`. We don't care about being coherent with the CPU's L2 cache, but it does have to go out the PCIe bus, so we rewrite `GMMU_APERTURE_PEER` to `GMMU_APERTURE_SYS_NONCOH`. We also no longer set the peer id that was corrupting the page table.

```
cudaMemcpyPeer / cudaMemcpy between GPU0 and GPU1: 24.21GB/s
Preparing host buffer and memcpy to GPU0...
Run kernel on GPU1, taking source data from GPU0 and writing to GPU1...
Run kernel on GPU0, taking source data from GPU1 and writing to GPU0...
Copy data back to host from GPU0 and verify results...
Verification error @ element 1: val = 0.000000, ref = 4.000000
Verification error @ element 2: val = 0.000000, ref = 8.000000
```

Progress! `./simpleP2P` appears to work, however the copy isn't happening. The address is likely wrong. It turns out they have a separate field for the peer address called `fldAddrPeer`, we change that to `fldAddrSysmem`. We also print out the addresses and note that the physical BAR address isn't being added properly, they provide a field `fabricBaseAddress` for `GMMU_APERTURE_PEER`, we reuse it and put the `BAR1` base address in there.

That's it. Thanks to NVIDIA for writing such a stable driver. And with this, the tinybox green is even better.

~ the tiny corp

## Functional

```
Enabling peer access between GPU0 and GPU1...
Allocating buffers (64MB on GPU0, GPU1 and CPU Host)...
Creating event handles...
cudaMemcpyPeer / cudaMemcpy between GPU0 and GPU1: 24.44GB/s
Preparing host buffer and memcpy to GPU0...
Run kernel on GPU1, taking source data from GPU0 and writing to GPU1...
Run kernel on GPU0, taking source data from GPU1 and writing to GPU0...
Copy data back to host from GPU0 and verify results...
Disabling peer access...
Shutting down...
Test passed
```

## Fast

```
Bidirectional P2P=Enabled Bandwidth Matrix (GB/s)
   D\D     0      1      2      3      4      5
     0 919.39  50.11  50.15  51.22  50.59  51.22
     1  50.19 921.29  50.31  51.21  50.62  51.22
     2  50.23  50.55 921.83  51.22  50.39  51.22
     3  50.33  50.65  51.20 920.20  50.43  51.22
     4  50.18  50.68  50.26  51.22 922.30  51.23
     5  50.12  50.09  50.44  51.22  51.21 921.29
```

## And NCCL (aka torch) compatible!

```
tiny@tiny14:~/build/nccl-tests/build$ ./all_reduce_perf -g 6
# nThread 1 nGpus 6 minBytes 33554432 maxBytes 33554432 step: 1048576(bytes) warmup iters: 5 iters: 20 agg iters: 1 validation: 1 graph: 0
#
# Using devices
#  Rank  0 Group  0 Pid  26230 on     tiny14 device  0 [0x01] NVIDIA GeForce RTX 4090
#  Rank  1 Group  0 Pid  26230 on     tiny14 device  1 [0x42] NVIDIA GeForce RTX 4090
#  Rank  2 Group  0 Pid  26230 on     tiny14 device  2 [0x81] NVIDIA GeForce RTX 4090
#  Rank  3 Group  0 Pid  26230 on     tiny14 device  3 [0x82] NVIDIA GeForce RTX 4090
#  Rank  4 Group  0 Pid  26230 on     tiny14 device  4 [0xc1] NVIDIA GeForce RTX 4090
#  Rank  5 Group  0 Pid  26230 on     tiny14 device  5 [0xc2] NVIDIA GeForce RTX 4090
#
#                                                              out-of-place                       in-place
#       size         count      type   redop    root     time   algbw   busbw #wrong     time   algbw   busbw #wrong
#        (B)    (elements)                               (us)  (GB/s)  (GB/s)            (us)  (GB/s)  (GB/s)
    33554432       8388608     float     sum      -1   2275.1   14.75   24.58      0   2282.5   14.70   24.50      0
# Out of bounds values : 0 OK
# Avg bus bandwidth    : 24.5413
#
```
