#!/bin/bash

TAP="tap0"

sudo ip tuntap add $TAP mode tap
sudo ip link set $TAP up
sudo ip addr add 10.0.3.1/24 dev $TAP