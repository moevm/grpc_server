INTERFACE="eth0"

sudo ip link set $INTERFACE up
sudo ip addr add 10.0.0.2/24 dev $INTERFACE
sudo ip route add default via $GATEWAY dev $INTERFACE

sudo ip route add 10.0.0.0/24 via 10.0.1.1 dev $INTERFACE
