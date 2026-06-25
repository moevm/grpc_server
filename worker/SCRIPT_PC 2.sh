INTERFACE="enp3s0"

sudo ip addr add 10.0.1.2/24 dev $INTERFACE
sudo ip link set dev $INTERFACE up
sudo ip route add 10.0.1.0/24 via 10.0.1.1 dev $INTERFACE
sudo ip addr set dev $INTERFACE up