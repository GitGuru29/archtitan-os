#!/bin/sh
# Titan GPU Switcher — NVIDIA dGPU launcher template
# Copy this to /etc/titan-gpu/launchers/<appname>.sh
# Then add to Hyprland config: exec = /etc/titan-gpu/launchers/<appname>.sh %U

# Force NVIDIA dGPU rendering via PRIME offload
export __NV_PRIME_RENDER_OFFLOAD=1
export __GLX_VENDOR_LIBRARY_NAME=nvidia

# For Vulkan apps, the Vulkan ICD loader auto-selects NVIDIA when available.
# If you need to force it, uncomment:
# export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/nvidia_icd.x86_64.json.json

exec "$@"
