#!/bin/bash
sudo rmmod nvidia nvidia_uvm nvidia_modeset nvidia_drm
sudo rmmod nvidia nvidia_uvm nvidia_modeset nvidia_drm
sudo rmmod nvidia nvidia_uvm nvidia_modeset nvidia_drm
sudo rmmod nvidia nvidia_uvm nvidia_modeset nvidia_drm
set -e
make modules -j$(nproc)
sudo make modules_install -j$(nproc)
sudo depmod
nvidia-smi
