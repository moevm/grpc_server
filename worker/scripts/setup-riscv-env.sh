#!/bin/bash
# Script for preparing the RISC-V crss compilation environment

set -e


if ! command -v riscv64-linux-gnu-gcc &> /dev/null; then
    echo "No cross compiler found. Install:"
    echo "sudo apt install crossbuild-essential-riscv64"
    exit 1
fi

DPDK_DIR="./dpdk-23.11"
if [ ! -d "$DPDK_DIR" ]; then
    echo "$DPDK_DIR folder not found"
    echo "Install DPDK 23.11:"
    echo "    wget https://fast.dpdk.org/rel/dpdk-23.11.tar.xz"
    echo "    tar -xf dpdk-23.11.tar.xz"
    exit 1
fi


cd "$DPDK_DIR"
rm -rf build-riscv
meson setup build-riscv \
    --cross-file config/riscv/riscv64_linux_gcc \
    --prefix=$(pwd)/../dpdk-riscv-install

ninja -C build-riscv
ninja -C build-riscv install

cd ..

echo "DPDK для RISC-V установлен в ./dpdk-riscv-install"

if [ -f "./dpdk-riscv-install/lib/pkgconfig/libdpdk.pc" ]; then
    echo " .pc files created:"
    ls -la ./dpdk-riscv-install/lib/pkgconfig/
else
    echo " Ошибка: .pc files not created!"
    exit 1
fi
