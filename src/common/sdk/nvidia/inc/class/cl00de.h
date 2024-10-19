/*
 * SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _cl00de_h_
#define _cl00de_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "nvmisc.h"
#include "nvfixedtypes.h"

#define RM_USER_SHARED_DATA                      (0x000000de)

#define RUSD_TIMESTAMP_WRITE_IN_PROGRESS (NV_U64_MAX)
#define RUSD_TIMESTAMP_INVALID           0

#define RUSD_SEQ_START (0xFF00000000000000LLU)

#define RUSD_SEQ_DATA_VALID(x) \
    ((((x) < RUSD_SEQ_START)  && ((x) != RUSD_TIMESTAMP_INVALID)) || \
     (((x) >= RUSD_SEQ_START) && (((x) & 0x1LLU) == 0x0LLU)))

//
// Helper macros to check seq before reading RUSD.
// No dowhile wrap as it is using continue/break
//
#define RUSD_SEQ_CHECK1(dataField)                           \
    NvU64 RUSD_SEQ = (dataField)->lastModifiedTimestamp;     \
    portAtomicMemoryFenceLoad();                             \
    if (!RUSD_SEQ_DATA_VALID(RUSD_SEQ))                      \
         continue;

#define RUSD_SEQ_CHECK2(dataField)                           \
    portAtomicMemoryFenceLoad();                             \
    if (RUSD_SEQ == (dataField)->lastModifiedTimestamp)      \
         break;

//
// Read RUSD data field `dataField` from NV00DE_SHARED_DATA struct `pSharedData` into destination pointer `pDst`
// `pDst` should be the data struct type matching `dataField`
// Check (pDst)->lastModifiedTimestamp using RUSD_IS_DATA_STALE to verify data validity.
//
#define RUSD_READ_DATA(pSharedData,dataField,pDst)                                        \
do {                                                                                      \
    portMemSet((pDst), 0, sizeof(*pDst));                                                 \
    for (NvU32 RUSD_READ_DATA_ATTEMPTS = 0; RUSD_READ_DATA_ATTEMPTS < 10; ++RUSD_READ_DATA_ATTEMPTS) \
    {                                                                                     \
        RUSD_SEQ_CHECK1(&((pSharedData)->dataField));                                     \
        portMemCopy((pDst), sizeof(*pDst), &((pSharedData)->dataField), sizeof(*pDst));   \
        RUSD_SEQ_CHECK2(&((pSharedData)->dataField));                                     \
    }                                                                                     \
} while(0);

//
// Check if RUSD data timestamp is stale.
// For polled data, returns true if data is older than `staleThreshold`
// For non-polled data, returns true if data was successfully read
//
#define RUSD_IS_DATA_STALE(timestamp,currentTime,staleThreshold)        \
    ((((timestamp) < (RUSD_SEQ_START)) &&           /* Polled Data */   \
      (((timestamp) == (RUSD_TIMESTAMP_INVALID)) || /* Invalid */       \
       (((currentTime) - (timestamp)) > (staleThreshold)))) ||          \
     (((timestamp) >= (RUSD_SEQ_START)) &&      /* Non-Polled Data */   \
      (((timestamp) & (0x1LLU)) == 1LLU)))

enum {
    RUSD_CLK_PUBLIC_DOMAIN_GRAPHICS = 0,
    RUSD_CLK_PUBLIC_DOMAIN_MEMORY,
    RUSD_CLK_PUBLIC_DOMAIN_VIDEO,

    // Put at the end. See bug 1000230 NVML doesn't report SM frequency on Kepler
    RUSD_CLK_PUBLIC_DOMAIN_SM,
    RUSD_CLK_PUBLIC_DOMAIN_MAX_TYPE,
};

enum {
    RUSD_CLK_THROTTLE_REASON_GPU_IDLE                         = NVBIT(0),
    RUSD_CLK_THROTTLE_REASON_APPLICATION_CLOCK_SETTING        = NVBIT(1), 
    RUSD_CLK_THROTTLE_REASON_SW_POWER_CAP                     = NVBIT(2), 
    RUSD_CLK_THROTTLE_REASON_HW_SLOWDOWN                      = NVBIT(3), 
    RUSD_CLK_THROTTLE_REASON_SYNC_BOOST                       = NVBIT(4), 
    RUSD_CLK_THROTTLE_REASON_SW_THERMAL_SLOWDOWN              = NVBIT(5), 
    RUSD_CLK_THROTTLE_REASON_HW_THERMAL_SLOWDOWN              = NVBIT(6), 
    RUSD_CLK_THROTTLE_REASON_HW_POWER_BRAKES_SLOWDOWN         = NVBIT(7), 
    RUSD_CLK_THROTTLE_REASON_DISPLAY_CLOCK_SETTING            = NVBIT(8), 
};

typedef struct RUSD_BAR1_MEMORY_INFO {
    volatile NvU64 lastModifiedTimestamp;
    NvU32 bar1Size;
    NvU32 bar1AvailSize;
} RUSD_BAR1_MEMORY_INFO;

typedef struct RUSD_PMA_MEMORY_INFO {
    volatile NvU64 lastModifiedTimestamp;
    NvU64 totalPmaMemory;
    NvU64 freePmaMemory;
} RUSD_PMA_MEMORY_INFO;

typedef struct RUSD_CLK_PUBLIC_DOMAIN_INFO {
    NvU32 targetClkMHz;
} RUSD_CLK_PUBLIC_DOMAIN_INFO;

typedef struct RUSD_CLK_PUBLIC_DOMAIN_INFOS {
    volatile NvU64 lastModifiedTimestamp;
    RUSD_CLK_PUBLIC_DOMAIN_INFO info[RUSD_CLK_PUBLIC_DOMAIN_MAX_TYPE];
} RUSD_CLK_PUBLIC_DOMAIN_INFOS;

typedef struct RUSD_ENG_UTILIZATION {
    NvU32 clkPercentBusy;
    NvU32 samplingPeriodUs;
} RUSD_ENG_UTILIZATION;

#define RUSD_ENG_UTILIZATION_VID_ENG_NVENC 0
#define RUSD_ENG_UTILIZATION_VID_ENG_NVDEC 1
#define RUSD_ENG_UTILIZATION_VID_ENG_NVJPG 2
#define RUSD_ENG_UTILIZATION_VID_ENG_NVOFA 3 
#define RUSD_ENG_UTILIZATION_COUNT 4

typedef struct RUSD_PERF_DEVICE_UTILIZATION_INFO {
    NvU32 gpuPercentBusy;
    NvU32 memoryPercentBusy;
    RUSD_ENG_UTILIZATION engUtil[RUSD_ENG_UTILIZATION_COUNT];
} RUSD_PERF_DEVICE_UTILIZATION_INFO;

typedef struct RUSD_PERF_DEVICE_UTILIZATION {
    volatile NvU64 lastModifiedTimestamp;
    RUSD_PERF_DEVICE_UTILIZATION_INFO info;
} RUSD_PERF_DEVICE_UTILIZATION;

typedef struct RUSD_PERF_CURRENT_PSTATE {
    volatile NvU64 lastModifiedTimestamp;
    NvU32 currentPstate;
} RUSD_PERF_CURRENT_PSTATE;

typedef struct RUSD_CLK_THROTTLE_REASON {
    volatile NvU64 lastModifiedTimestamp;
    NvU32 reasonMask; // Bitmask of RUSD_CLK_THROTTLE_REASON
} RUSD_CLK_THROTTLE_REASON;

typedef struct RUSD_MEM_ERROR_COUNTS {
    NvU64 correctedVolatile;
    NvU64 correctedAggregate;
    NvU64 uncorrectedVolatile;
    NvU64 uncorrectedAggregate;
} RUSD_MEM_ERROR_COUNTS;

#define RUSD_MEMORY_ERROR_TYPE_TOTAL 0
#define RUSD_MEMORY_ERROR_TYPE_DRAM  1
#define RUSD_MEMORY_ERROR_TYPE_SRAM  2
#define RUSD_MEMORY_ERROR_TYPE_COUNT 3

typedef struct RUSD_MEM_ECC {
    volatile NvU64 lastModifiedTimestamp;
    RUSD_MEM_ERROR_COUNTS count[RUSD_MEMORY_ERROR_TYPE_COUNT];
} RUSD_MEM_ECC;

typedef struct RUSD_POWER_LIMIT_INFO {
    NvU32 requestedmW;
    NvU32 enforcedmW;
} RUSD_POWER_LIMIT_INFO;

typedef struct RUSD_POWER_LIMITS {
    volatile NvU64 lastModifiedTimestamp;
    RUSD_POWER_LIMIT_INFO info;
} RUSD_POWER_LIMITS;

typedef struct RUSD_TEMPERATURE_INFO {
    NvTemp gpuTemperature;
    NvTemp hbmTemperature;
} RUSD_TEMPERATURE_INFO;

typedef struct RUSD_TEMPERATURE {
    volatile NvU64 lastModifiedTimestamp;
    RUSD_TEMPERATURE_INFO info;
} RUSD_TEMPERATURE;

typedef struct RUSD_MEM_ROW_REMAP_INFO {
    NvU32 histogramMax;     // No remapped row is used.
    NvU32 histogramHigh;    // One remapped row is used.
    NvU32 histogramPartial; // More than one remapped rows are used.
    NvU32 histogramLow;     // One remapped row is available.
    NvU32 histogramNone;    // All remapped rows are used.

    NvU32 correctableRows;
    NvU32 uncorrectableRows;
    NvBool isPending;
    NvBool hasFailureOccurred;
} RUSD_MEM_ROW_REMAP_INFO;

typedef struct RUSD_MEM_ROW_REMAP {
    volatile NvU64 lastModifiedTimestamp;
    RUSD_MEM_ROW_REMAP_INFO info;
} RUSD_MEM_ROW_REMAP;

typedef struct RUSD_AVG_POWER_INFO {
    NvU32 averageGpuPower;      // mW
    NvU32 averageModulePower;   // mW
    NvU32 averageMemoryPower;   // mW
} RUSD_AVG_POWER_INFO;

typedef struct RUSD_AVG_POWER_USAGE {
    volatile NvU64 lastModifiedTimestamp;
    RUSD_AVG_POWER_INFO info;
} RUSD_AVG_POWER_USAGE;

typedef struct RUSD_INST_POWER_INFO {
    NvU32 instGpuPower;         // mW
    NvU32 instModulePower;      // mW
} RUSD_INST_POWER_INFO;

typedef struct RUSD_INST_POWER_USAGE {
    volatile NvU64 lastModifiedTimestamp;
    RUSD_INST_POWER_INFO info;
} RUSD_INST_POWER_USAGE;

typedef struct RUSD_SHADOW_ERR_CONT {
    volatile NvU64 lastModifiedTimestamp;
    NvU32 shadowErrContVal;
} RUSD_SHADOW_ERR_CONT;

typedef struct NV00DE_SHARED_DATA {
    // Temporarily duplicated - to be removed by nested structs below
    volatile NvU64 seq;

    NvU32 bar1Size;
    NvU32 bar1AvailSize;

    NV_DECLARE_ALIGNED(RUSD_BAR1_MEMORY_INFO bar1MemoryInfo, 8);

    NV_DECLARE_ALIGNED(RUSD_PMA_MEMORY_INFO pmaMemoryInfo, 8);

    NV_DECLARE_ALIGNED(RUSD_SHADOW_ERR_CONT shadowErrCont, 8);

    // gpuUpdateUserSharedData is sensitive to these two sections being contiguous

    //
    // Polled data section
    // All data structs are a volatile NvU64 timestamp followed by data contents.
    // Access by reading timestamp, then copying the struct contents, then reading the timestamp again.
    // If time0 matches time1, data has not changed during the read, and contents are valid.
    // If timestamp is RUSD_TIMESTAMP_WRITE_IN_PROGRESS, data was edited during the read, retry.
    // If timestamp is RUSD_TIMESTAMP_INVALID, data is not available or not supported on this platform.
    //

    // POLL_CLOCK
    NV_DECLARE_ALIGNED(RUSD_CLK_PUBLIC_DOMAIN_INFOS clkPublicDomainInfos, 8);

    // POLL_PERF
    NV_DECLARE_ALIGNED(RUSD_CLK_THROTTLE_REASON clkThrottleReason, 8);

    // POLL_PERF
    NV_DECLARE_ALIGNED(RUSD_PERF_DEVICE_UTILIZATION perfDevUtil, 8);

    // POLL_MEMORY
    NV_DECLARE_ALIGNED(RUSD_MEM_ECC memEcc, 8);

    // POLL_PERF
    NV_DECLARE_ALIGNED(RUSD_PERF_CURRENT_PSTATE perfCurrentPstate, 8);

    // POLL_POWER
    // Module Limit is not supported on Ampere/Hopper
    NV_DECLARE_ALIGNED(RUSD_POWER_LIMITS powerLimitGpu, 8);

    // POLL_THERMAL
    NV_DECLARE_ALIGNED(RUSD_TEMPERATURE temperature, 8);

    // POLL_MEMORY
    NV_DECLARE_ALIGNED(RUSD_MEM_ROW_REMAP memRowRemap, 8);

    // POLL_POWER
    NV_DECLARE_ALIGNED(RUSD_AVG_POWER_USAGE avgPowerUsage, 8);

    // POLL_POWER
    NV_DECLARE_ALIGNED(RUSD_INST_POWER_USAGE instPowerUsage, 8);
} NV00DE_SHARED_DATA;

//
// Polling mask bits, pass into ALLOC_PARAMETERS or NV00DE_CTRL_REQEUSET_DATA_POLL
// to request above polled data to be provided
//
#define NV00DE_RUSD_POLL_CLOCK     0x1
#define NV00DE_RUSD_POLL_PERF      0x2
#define NV00DE_RUSD_POLL_MEMORY    0x4
#define NV00DE_RUSD_POLL_POWER     0x8
#define NV00DE_RUSD_POLL_THERMAL   0x10

typedef struct NV00DE_ALLOC_PARAMETERS {
    NvU64 polledDataMask; // Bitmask of data to request polling at alloc time, 0 if not needed
} NV00DE_ALLOC_PARAMETERS;

#ifdef __cplusplus
};     /* extern "C" */
#endif

#endif /* _cl00de_h_ */
