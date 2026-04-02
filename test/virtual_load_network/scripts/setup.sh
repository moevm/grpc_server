#!/bin/bash

N=${1:-3}
BRIDGE="br0"
SUBNET="10.0.0"
GW="10.0.0.254"
DNS="8.8.8.8"

echo "setup: $N hosts"

sysctl -w vm.drop_caches=3
echo 1024 >/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
echo "[+] hugepages: $(cat /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages)"

ip link add $BRIDGE type bridge
ip link set $BRIDGE up
ip addr add ${GW}/24 dev $BRIDGE
echo "[+] bridge $BRIDGE ($GW)"

ip link delete veth0 2>/dev/null
ip link add veth0 type veth peer name veth1
ip link set veth0 up
ip link set veth1 up

tc qdisc add dev $BRIDGE clsact
tc filter add dev $BRIDGE egress matchall action mirred egress mirror dev veth1
tc filter add dev $BRIDGE ingress matchall action mirred egress mirror dev veth1
echo "[+] tc mirror: bridge -> veth1 -> veth0 (DPDK port_in)"

sysctl -w net.ipv4.ip_forward=1
iptables -t nat -A POSTROUTING -s 10.0.0.0/24 ! -d 10.0.0.0/24 -j MASQUERADE
iptables -I FORWARD -i $BRIDGE -j ACCEPT
iptables -I FORWARD -o $BRIDGE -j ACCEPT
echo "[+] NAT"

for i in $(seq 1 $N); do
    IP="${SUBNET}.${i}/24"
    VETH="veth-h${i}"
    BR_VETH="br-h${i}"
    NAME="gen${i}"

    docker run -d \
        --network=none \
        --name $NAME \
        --cap-add NET_ADMIN \
        --dns $DNS \
        traffic-gen

    ip link add $VETH type veth peer name $BR_VETH
    ip link set $BR_VETH master $BRIDGE
    ip link set $BR_VETH up

    PID=$(docker inspect $NAME --format '{{.State.Pid}}')
    ip link set $VETH netns $PID

    docker exec $NAME ip link set $VETH up
    docker exec $NAME ip addr add $IP dev $VETH
    docker exec $NAME ip route add default via ${GW}

    echo "[+] $NAME: $IP"
done
