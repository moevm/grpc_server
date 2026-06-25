
sudo ethtool -K eth0 tx off rx off
sudo ethtool -K eth1 tx off rx off
sudo ethtool -K eth0 tx-checksum-ip-generic off
sudo ethtool -K eth1 tx-checksum-ip-generic off


sudo ethtool -k eth0 | grep checksum
sudo ethtool -k eth1 | grep checksum
