#!/bin/bash
#
# Detach users for 
# rmmod: ERROR: Module nvidia_drm nvidia_uvm is in use
sudo systemctl isolate multi-user.target
#
sudo rmmod nvidia_drm nvidia_modeset nvidia_uvm nvidia
set -e
make modules -j$(nproc)
sudo make modules_install -j$(nproc)
sudo depmod
nvidia-smi
