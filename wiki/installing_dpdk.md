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

## Environment
The `worker/scripts/setup-riscv-env.sh` script automatically downloads (if necessary) and builds DPDK 23.11 for the RISC-V architecture.

```bash
./worker/scripts/setup-riscv-env.sh
```

## SQLite
If the target architecture is RISC-V, SQLite must be built with a cross compiler.

```bash
wget https://www.sqlite.org/2024/sqlite-autoconf-3460100.tar.gz
tar -xzf sqlite-autoconf-3460100.tar.gz
cd sqlite-autoconf-3460100

./configure --host=riscv64-linux-gnu --prefix=/path/to/sqlite3-riscv-install
make -j$(nproc)
make install
```

After installation, the subdirectories include/ and lib/ with the necessary files will appear in the specified prefix.
