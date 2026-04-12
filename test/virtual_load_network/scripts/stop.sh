#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_DIR"

if [ -f .env ]; then
    set -a
    source .env
    set +a
fi

SUBNET="${SUBNET:-10.0.0}"
BRIDGE_IFACE="${BRIDGE_IFACE:-br-testnet}"

tc qdisc del dev "$BRIDGE_IFACE" clsact 2>/dev/null
iptables -D FORWARD -i "$BRIDGE_IFACE" -j ACCEPT 2>/dev/null
iptables -D FORWARD -o "$BRIDGE_IFACE" -j ACCEPT 2>/dev/null

docker compose down 2>/dev/null

iptables -t nat -D POSTROUTING -s ${SUBNET}.0/24 ! -d ${SUBNET}.0/24 -j MASQUERADE 2>/dev/null
ip link delete veth0 2>/dev/null
