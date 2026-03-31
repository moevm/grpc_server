#!/bin/bash

sudo ip link delete veth0 2>/dev/null
sudo ip link delete veth1 2>/dev/null
sudo ip link add veth0 type veth peer name veth1
sudo ip link set veth0 up
sudo ip link set veth1 up

sudo ip addr add 10.0.0.1/24 dev veth0
sudo ip addr add 10.0.0.2/24 dev veth1
