#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ -f "$PROJECT_DIR/.env" ]; then
    set -a
    source "$PROJECT_DIR/.env"
    set +a
fi

VM_NAME="$1"
SHARED_DIR="$2"
TAP_IFACE="$3"
MEMORY="${4:-${QEMU_MEMORY:-4G}}"
CPUS="${5:-${QEMU_CPUS:-2}}"

YOCTO_DEPLOY_DIR="${YOCTO_DEPLOY_DIR:-/home/lespend/program/yadro/vm_build_risc_v/qemu/poky/build/tmp/deploy/images/qemuriscv64}"
QEMU_ROOTFS="${QEMU_ROOTFS:-${YOCTO_DEPLOY_DIR}/cluster-image-qemuriscv64.rootfs.ext4}"
QEMU_KERNEL="${QEMU_KERNEL:-${YOCTO_DEPLOY_DIR}/Image}"
QEMU_BIOS="${QEMU_BIOS:-${YOCTO_DEPLOY_DIR}/fw_jump.elf}"

if [ -z "$VM_NAME" ] || [ -z "$SHARED_DIR" ] || [ -z "$TAP_IFACE" ]; then
    echo "Usage: $0 <vm_name> <shared_dir> <tap_iface> [memory] [cpus]"
    exit 1
fi

OVERLAY="$PROJECT_DIR/${VM_NAME}.qcow2"
if [ ! -f "$OVERLAY" ]; then
    qemu-img create -f qcow2 -b "$QEMU_ROOTFS" -F raw "$OVERLAY" >/dev/null
fi

ip tuntap add dev "$TAP_IFACE" mode tap 2>/dev/null || true
ip link set "$TAP_IFACE" up

qemu-system-riscv64 \
    -machine virt -m "$MEMORY" -smp "$CPUS" \
    -display none -serial "file:/tmp/qemu-${VM_NAME}.log" \
    -name "$VM_NAME" \
    -pidfile "/tmp/qemu-${VM_NAME}.pid" \
    -bios "$QEMU_BIOS" \
    -kernel "$QEMU_KERNEL" \
    -append "root=/dev/vda rw earlycon=sbi console=ttyS0" \
    -netdev tap,id=net0,ifname="$TAP_IFACE",script=no,downscript=no \
    -device virtio-net-device,netdev=net0 \
    -netdev user,id=net1,hostfwd=tcp::$(shuf -i 10000-20000 -n1)-:22 \
    -device virtio-net-device,netdev=net1 \
    -object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0 \
    -drive "id=disk0,file=${OVERLAY},format=qcow2,if=none" -device virtio-blk-device,drive=disk0 \
    -virtfs "local,path=$SHARED_DIR,mount_tag=host_share,security_model=mapped-xattr" \
    -daemonize
