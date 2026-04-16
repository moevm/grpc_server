#!/bin/bash
set -e

VM_NAME="$1"
VM_IMAGE="$2"
SHARED_DIR="$3"
TAP_IFACE="$4"
MEMORY="${5:-4G}"
CPUS="${6:-2}"

if [ -z "$VM_NAME" ] || [ -z "$VM_IMAGE" ] || [ -z "$SHARED_DIR" ] || [ -z "$TAP_IFACE" ]; then
    echo "Usage: $0 <vm_name> <image> <shared_dir> <tap_iface> [memory] [cpus]"
    exit 1
fi

if [ ! -f "$VM_IMAGE" ]; then
    echo "Image not found: $VM_IMAGE"
    exit 1
fi

ip tuntap add dev "$TAP_IFACE" mode tap 2>/dev/null || true
ip link set "$TAP_IFACE" up

qemu-system-riscv64 \
    -machine virt,acpi=off -m "$MEMORY" -smp cpus="$CPUS" \
    -display none -serial "file:/tmp/qemu-${VM_NAME}.log" \
    -name "$VM_NAME" \
    -pidfile "/tmp/qemu-${VM_NAME}.pid" \
    -bios default \
    -netdev tap,id=net0,ifname="$TAP_IFACE",script=no,downscript=no \
    -device virtio-net-device,netdev=net0 \
    -netdev user,id=net1,hostfwd=tcp::$(shuf -i 10000-20000 -n1)-:22 \
    -device virtio-net-device,netdev=net1 \
    -device virtio-rng-pci \
    -drive "file=$VM_IMAGE,format=raw,if=virtio" \
    -virtfs "local,path=$SHARED_DIR,mount_tag=host_share,security_model=mapped-xattr" \
    -daemonize
