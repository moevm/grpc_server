#!/bin/bash

DIR=$1
shift 

HOSTFWD=""
for fwd in "$@"; do
    HOSTFWD="$HOSTFWD,hostfwd=$fwd"
done

qemu-system-riscv64 \
    -machine virt,acpi=off -m 4G -smp cpus=2 \
    -nographic \
    -kernel /usr/lib/u-boot/qemu-riscv64_smode/uboot.elf \
    -netdev user,id=net0$HOSTFWD \
    -device virtio-net-device,netdev=net0 \
    -device virtio-rng-pci \
    -drive file=ubuntu-24.04.4-preinstalled-server-riscv64.img,format=raw,if=virtio \
    -virtfs local,path=$DIR,mount_tag=host_share,security_model=mapped-xattr


