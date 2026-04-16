#!/bin/bash

sudo apt update
sudo apt install -y qemu-system-misc opensbi u-boot-qemu qemu-user-static binfmt-support


wget https://cdimage.ubuntu.com/releases/24.04/release/ubuntu-24.04.4-preinstalled-server-riscv64.img.xz

xz -d ubuntu-24.04.4-preinstalled-server-riscv64.img.xz

LOOP=$(sudo losetup -f)

sudo losetup -P $LOOP ubuntu-24.04.4-preinstalled-server-riscv64.img

sudo mkdir -p /mnt/riscv-rootfs
sudo mount ${LOOP}p1 /mnt/riscv-rootfs

sudo cp /usr/bin/qemu-riscv64-static /mnt/riscv-rootfs/usr/bin/

sudo rm -f /mnt/riscv-rootfs/etc/resolv.conf
sudo cp /etc/resolv.conf /mnt/riscv-rootfs/etc/resolv.conf

sudo chroot /mnt/riscv-rootfs /bin/bash -c "
  apt update
  apt install -y dpdk dpdk-dev build-essential git meson ninja-build python3-pyelftools libnuma-dev
"

sudo umount /mnt/riscv-rootfs
sudo losetup -d $LOOP


echo "Image create"



