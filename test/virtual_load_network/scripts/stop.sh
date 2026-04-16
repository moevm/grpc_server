#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_DIR"

if [ -f .env ]; then
    set -a
    source .env
    set +a
fi

INET_SUBNET="${INET_SUBNET:-10.0.3}"
INET_BRIDGE="${INET_BRIDGE:-br-inet}"
MGMT_BRIDGE="${MGMT_BRIDGE:-br-mgmt}"

for VM in filter1 filter2 controller; do
    PID_FILE="/tmp/qemu-${VM}.pid"
    if [ -f "$PID_FILE" ]; then
        kill "$(cat "$PID_FILE")" 2>/dev/null
        rm -f "$PID_FILE"
    fi
done

docker compose down 2>/dev/null

iptables -t nat -D POSTROUTING -s ${INET_SUBNET}.0/24 ! -d ${INET_SUBNET}.0/24 -j MASQUERADE 2>/dev/null
iptables -D FORWARD -i "$INET_BRIDGE" -j ACCEPT 2>/dev/null
iptables -D FORWARD -o "$INET_BRIDGE" -j ACCEPT 2>/dev/null

for TAP in tap-f1-in tap-f1-out tap-f2-in tap-f2-out tap-f1-mgmt tap-f2-mgmt tap-ctrl; do
    ip tuntap del dev "$TAP" mode tap 2>/dev/null
done

ip link delete "$INET_BRIDGE" 2>/dev/null
ip link delete "$MGMT_BRIDGE" 2>/dev/null

rm -rf "$PROJECT_DIR/shared"
rm -f "$PROJECT_DIR"/filter1.qcow2 "$PROJECT_DIR"/filter2.qcow2 "$PROJECT_DIR"/controller.qcow2
rm -f "$PROJECT_DIR"/cloud-init-*.iso
