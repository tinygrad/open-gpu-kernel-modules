#!/bin/bash
sudo rmmod nvidia_drm nvidia_modeset nvidia_uvm nvidia
set -e
make modules -j$(nproc)
sudo make modules_install -j$(nproc)
sudo depmod
nvidia-smi
