#!/bin/bash

set -e

usage() {
    cat << EOF
Usage: $0 [options]

Options:"
    -h, --help        Show this help message and exit"
    -i, --image-dir <path>   Directory containing the built Yocto images"

This script flashes the built Yocto images to Lichee Pi 4A using fastboot."
EOF
}

for arg in "$@"; do
    case $arg in
        -h|--help)
            usage
            exit 0
            ;;
        -i|--image-dir)
            shift
            IMAGE_DIR="$1"
            if [ ! -d "$IMAGE_DIR" ]; then
                echo "Error: Image directory '$IMAGE_DIR' does not exist."
                exit 1
            fi
            ;;
        *)
            echo "Unknown option: $arg"
            usage
            exit 1
            ;;
    esac
done

if [ -z "$IMAGE_DIR" ]; then
    IMAGE_DIR="../yocto/poky/build/tmp/deploy/images/licheepi4a"
fi

lsusb | grep "T-HEAD" || {
  echo "Error: Lichee Pi 4A not detected. Please connect the device via USB-C and ensure it's in fastboot mode (hold BOOT button while connecting)."
  exit 1
}

sudo fastboot flash ram "${IMAGE_DIR}/u-boot-with-spl.bin"
sudo fastboot reboot
sleep 1

sudo fastboot flash uboot "${IMAGE_DIR}/u-boot-with-spl.bin"
sudo fastboot flash boot "${IMAGE_DIR}/boot-licheepi4a.ext4"
sudo fastboot flash root "${IMAGE_DIR}/licheepi4a-dpdk-image-licheepi4a.rootfs.ext4"