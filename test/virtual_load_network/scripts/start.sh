#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_DIR"

if [ -f .env ]; then
    set -a
    source .env
    set +a
fi

SUBNET1="${SUBNET1:-10.0.0}"
SUBNET2="${SUBNET2:-10.0.1}"
INET_SUBNET="${INET_SUBNET:-10.0.3}"
BRIDGE1="${BRIDGE1:-br-testnet1}"
BRIDGE2="${BRIDGE2:-br-testnet2}"
INET_BRIDGE="${INET_BRIDGE:-br-inet}"
MGMT_SUBNET="${MGMT_SUBNET:-10.0.2}"
MGMT_BRIDGE="${MGMT_BRIDGE:-br-mgmt}"
HUGEPAGES="${HUGEPAGES:-1024}"
YOCTO_DEPLOY_DIR="${YOCTO_DEPLOY_DIR:-/home/lespend/program/yadro/vm_build_risc_v/qemu/poky/build/tmp/deploy/images/qemuriscv64}"
QEMU_ROOTFS="${QEMU_ROOTFS:-${YOCTO_DEPLOY_DIR}/cluster-image-qemuriscv64.rootfs.ext4}"
QEMU_KERNEL="${QEMU_KERNEL:-${YOCTO_DEPLOY_DIR}/Image}"
QEMU_BIOS="${QEMU_BIOS:-${YOCTO_DEPLOY_DIR}/fw_jump.elf}"
QEMU_MEMORY="${QEMU_MEMORY:-4G}"
QEMU_CPUS="${QEMU_CPUS:-2}"
FILTER_RISCV_BIN="${FILTER_RISCV_BIN:-../../worker/main-riscv}"
CONTROLLER_BIN="${CONTROLLER_BIN:-../../controller/bin/grpc_server}"
FILTER1_MAC="52:54:00:f1:00:01"
FILTER2_MAC="52:54:00:f2:00:01"

"$SCRIPT_DIR/stop.sh" 2>/dev/null || true

sysctl -w vm.drop_caches=3 >/dev/null
echo "$HUGEPAGES" >/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

docker compose up -d --build

for BRIDGE in "$BRIDGE1" "$BRIDGE2"; do
    for i in $(seq 1 10); do
        ip link show "$BRIDGE" &>/dev/null && break
        sleep 0.5
    done
    if ! ip link show "$BRIDGE" &>/dev/null; then
        echo "Bridge $BRIDGE not found"
        exit 1
    fi
done

sysctl -w net.ipv4.ip_forward=1 >/dev/null

ip link add "$INET_BRIDGE" type bridge 2>/dev/null || true
ip link set "$INET_BRIDGE" up
ip addr add "${INET_SUBNET}.254/24" dev "$INET_BRIDGE" 2>/dev/null || true

iptables -t nat -A POSTROUTING -s ${INET_SUBNET}.0/24 ! -d ${INET_SUBNET}.0/24 -j MASQUERADE
iptables -I FORWARD -i "$INET_BRIDGE" -j ACCEPT
iptables -I FORWARD -o "$INET_BRIDGE" -j ACCEPT

ip link add "$MGMT_BRIDGE" type bridge 2>/dev/null || true
ip link set "$MGMT_BRIDGE" up
ip addr add "${MGMT_SUBNET}.254/24" dev "$MGMT_BRIDGE" 2>/dev/null || true

iptables -I FORWARD -i "$MGMT_BRIDGE" -j ACCEPT
iptables -I FORWARD -o "$MGMT_BRIDGE" -j ACCEPT

SHARED_DIR="$PROJECT_DIR/shared"
mkdir -p "$SHARED_DIR"

if [ -f "$FILTER_RISCV_BIN" ]; then
    cp "$FILTER_RISCV_BIN" "$SHARED_DIR/filter"
fi
if [ -f "$CONTROLLER_BIN" ]; then
    cp "$CONTROLLER_BIN" "$SHARED_DIR/controller"
fi

mkdir -p "$SHARED_DIR/internal/service/config"
cp ../../controller/internal/service/config/categories.json "$SHARED_DIR/internal/service/config/"
cp ../../controller/internal/service/config/providers.json "$SHARED_DIR/internal/service/config/"
if [ -d "$PROJECT_DIR/configs" ]; then
    cp -r "$PROJECT_DIR/configs" "$SHARED_DIR/"
fi

start_controller() {
    local CTRL_OVERLAY="$PROJECT_DIR/controller.qcow2"
    if [ ! -f "$CTRL_OVERLAY" ]; then
        qemu-img create -f qcow2 -b "$QEMU_ROOTFS" -F raw "$CTRL_OVERLAY" >/dev/null
    fi

    qemu-system-riscv64 \
        -machine virt -m "$QEMU_MEMORY" -smp "$QEMU_CPUS" \
        -display none -serial "file:/tmp/qemu-controller.log" \
        -name "controller" \
        -pidfile "/tmp/qemu-controller.pid" \
        -bios "$QEMU_BIOS" \
        -kernel "$QEMU_KERNEL" \
        -append "root=/dev/vda rw earlycon=sbi console=ttyS0 ip=${MGMT_SUBNET}.3::${MGMT_SUBNET}.254:255.255.255.0::eth0:off" \
        -netdev tap,id=net0,ifname="tap-ctrl",script=no,downscript=no \
        -device virtio-net-device,netdev=net0 \
        -object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0 \
        -drive "id=disk0,file=${CTRL_OVERLAY},format=qcow2,if=none" -device virtio-blk-device,drive=disk0 \
        -virtfs "local,path=$SHARED_DIR,mount_tag=host_share,security_model=mapped-xattr" \
        -daemonize ||
        {
            echo "Failed to start controller"
            exit 1
        }
}

start_filter_vm() {
    local VM_NAME="$1"
    local TAP_IN="$2"
    local TAP_OUT="$3"
    local TAP_MGMT="$4"
    local BRIDGE_IN="$5"
    local MGMT_IP="$6"
    local ETH0_MAC="$7"

    local OVERLAY="$PROJECT_DIR/${VM_NAME}.qcow2"
    if [ ! -f "$OVERLAY" ]; then
        qemu-img create -f qcow2 -b "$QEMU_ROOTFS" -F raw "$OVERLAY" >/dev/null
    fi

    ip tuntap add dev "$TAP_IN" mode tap 2>/dev/null || true
    ip link set "$TAP_IN" master "$BRIDGE_IN"
    ip link set "$TAP_IN" up

    ip tuntap add dev "$TAP_OUT" mode tap 2>/dev/null || true
    ip link set "$TAP_OUT" master "$INET_BRIDGE"
    ip link set "$TAP_OUT" up

    ip tuntap add dev "$TAP_MGMT" mode tap 2>/dev/null || true
    ip link set "$TAP_MGMT" master "$MGMT_BRIDGE"
    ip link set "$TAP_MGMT" up

    qemu-system-riscv64 \
        -machine virt -m "$QEMU_MEMORY" -smp "$QEMU_CPUS" \
        -display none -serial "file:/tmp/qemu-${VM_NAME}.log" \
        -name "$VM_NAME" \
        -pidfile "/tmp/qemu-${VM_NAME}.pid" \
        -bios "$QEMU_BIOS" \
        -kernel "$QEMU_KERNEL" \
        -append "root=/dev/vda rw earlycon=sbi console=ttyS0 ip=${MGMT_IP}::${MGMT_SUBNET}.254:255.255.255.0::eth2:off" \
        -netdev tap,id=net0,ifname="$TAP_IN",script=no,downscript=no \
        -device virtio-net-device,netdev=net0,mac="$ETH0_MAC" \
        -netdev tap,id=net1,ifname="$TAP_OUT",script=no,downscript=no \
        -device virtio-net-device,netdev=net1 \
        -netdev tap,id=net2,ifname="$TAP_MGMT",script=no,downscript=no \
        -device virtio-net-device,netdev=net2 \
        -object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0 \
        -drive "id=disk0,file=${OVERLAY},format=qcow2,if=none" -device virtio-blk-device,drive=disk0 \
        -virtfs "local,path=$SHARED_DIR,mount_tag=host_share,security_model=mapped-xattr" \
        -daemonize ||
        {
            echo "Failed to start $VM_NAME"
            exit 1
        }
}

start_filter_vm "filter1" "tap-f1-in" "tap-f1-out" "tap-f1-mgmt" "$BRIDGE1" "${MGMT_SUBNET}.1" "$FILTER1_MAC"
start_filter_vm "filter2" "tap-f2-in" "tap-f2-out" "tap-f2-mgmt" "$BRIDGE2" "${MGMT_SUBNET}.2" "$FILTER2_MAC"

for SVC in $(docker compose ps -q 2>/dev/null); do
    docker exec "$SVC" ip neigh flush all 2>/dev/null || true
done
for SVC in $(docker compose -p "$(basename "$PROJECT_DIR")" ps --format '{{.Name}}' 2>/dev/null | grep "gen-1"); do
    docker exec "$SVC" arp -s "${SUBNET1}.254" "$FILTER1_MAC" 2>/dev/null || true
done
for SVC in $(docker compose -p "$(basename "$PROJECT_DIR")" ps --format '{{.Name}}' 2>/dev/null | grep "gen-2"); do
    docker exec "$SVC" arp -s "${SUBNET2}.254" "$FILTER2_MAC" 2>/dev/null || true
done

ip tuntap add dev "tap-ctrl" mode tap 2>/dev/null || true
ip link set "tap-ctrl" master "$MGMT_BRIDGE"
ip link set "tap-ctrl" up

start_controller

echo "Test stand is running"
echo "  Filter VMs: filter1 (pid $(cat /tmp/qemu-filter1.pid 2>/dev/null)), filter2 (pid $(cat /tmp/qemu-filter2.pid 2>/dev/null))"
echo "  Controller: pid $(cat /tmp/qemu-controller.pid 2>/dev/null)"
echo "  Traffic generators: $(docker compose ps --format '{{.Name}}' | wc -l) containers"
