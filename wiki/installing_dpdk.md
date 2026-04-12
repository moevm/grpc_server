# Installing DPDK 23.11.6

```sh
sudo apt install -y wget
wget https://fast.dpdk.org/rel/dpdk-23.11.6.tar.xz
tar -xf dpdk-23.11.6.tar.xz
cd dpdk-stable-23.11.6

sudo apt-get install -y meson ninja-build python3-pyelftools libbpf-dev

meson setup -Denable_drivers=net/af_xdp,net/tap build
ninja -C build
sudo ninja -C build install
sudo ldconfig
```
```
