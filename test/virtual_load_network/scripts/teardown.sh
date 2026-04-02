#!/bin/bash

N=${1:-3}

echo "teardown: $N hosts"

for i in $(seq 1 $N); do
    docker rm -f gen${i} 2>/dev/null && echo "[-] gen${i} removed"
    ip link delete br-h${i} 2>/dev/null && echo "[-] br-h${i} removed"
done

iptables -t nat -D POSTROUTING -s 10.0.0.0/24 ! -d 10.0.0.0/24 -j MASQUERADE 2>/dev/null
iptables -D FORWARD -i br0 -j ACCEPT 2>/dev/null
iptables -D FORWARD -o br0 -j ACCEPT 2>/dev/null
tc qdisc del dev br0 clsact 2>/dev/null
ip link delete br0 2>/dev/null && echo "[-] bridge br0 removed"
ip link delete veth0 2>/dev/null && echo "[-] veth0/veth1 removed"
