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

NUM_HOSTS="${NUM_HOSTS:-3}"
SUBNET="${SUBNET:-10.0.0}"
GATEWAY="${GATEWAY:-10.0.0.254}"
HUGEPAGES="${HUGEPAGES:-1024}"
FILTER_PATH="${FILTER_PATH:-../../worker/main-x86-virt}"
BRIDGE_IFACE="${BRIDGE_IFACE:-br-testnet}"

"$SCRIPT_DIR/stop.sh" 2>/dev/null || true

sysctl -w vm.drop_caches=3 > /dev/null
echo "$HUGEPAGES" > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

docker compose up -d --build

for i in $(seq 1 10); do
    ip link show "$BRIDGE_IFACE" &>/dev/null && break
    sleep 0.5
done

if ! ip link show "$BRIDGE_IFACE" &>/dev/null; then
    echo "Bridge interface $BRIDGE_IFACE not found"
    exit 1
fi

ip link delete veth0 2>/dev/null || true
ip link add veth0 type veth peer name veth1
ip link set veth0 up
ip link set veth1 up

tc qdisc add dev "$BRIDGE_IFACE" clsact
tc filter add dev "$BRIDGE_IFACE" egress matchall action mirred egress mirror dev veth1
tc filter add dev "$BRIDGE_IFACE" ingress matchall action mirred egress mirror dev veth1

sysctl -w net.ipv4.ip_forward=1 > /dev/null
iptables -t nat -A POSTROUTING -s ${SUBNET}.0/24 ! -d ${SUBNET}.0/24 -j MASQUERADE
iptables -I FORWARD -i "$BRIDGE_IFACE" -j ACCEPT
iptables -I FORWARD -o "$BRIDGE_IFACE" -j ACCEPT

if [ ! -x "$FILTER_PATH" ]; then
    echo "Filter binary not found: $FILTER_PATH"
    echo "Build: cd worker && make -f Makefile.main_x86 virt"
    exit 1
fi

LD_LIBRARY_PATH=/usr/local/lib exec "$FILTER_PATH" --no-pci --
