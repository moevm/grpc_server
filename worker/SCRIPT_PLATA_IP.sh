ip link set eth0 up
ip link set eth1 up

ip addr add 10.0.0.1/24 dev eth0 #подключен к ПК 1
ip addr add 10.0.0.2/24 dev eth1 #подключен к ПК 2

sysctl -w net.ipv4.ip_forward=1
