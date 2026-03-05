SUMMARY = "Lichee Pi 4A image with DPDK support"
DESCRIPTION = "Custom Yocto image for Lichee Pi 4A based on RevyOS kernel 6.6 \
with DPDK data plane development kit and networking utilities."
LICENSE = "MIT"

inherit core-image

# Image Features
IMAGE_FEATURES += " \
    ssh-server-openssh \
    tools-debug \
    package-management \
"

# DPDK packages
IMAGE_INSTALL:append = " \
    dpdk \
    dpdk-examples \
    numactl \
"

# Networking tools
IMAGE_INSTALL:append = " \
    ethtool \
    iperf3 \
    iproute2 \
    iptables \
    tcpdump \
    net-tools \
    openssh-sftp-server \
    nfs-utils-client \
"

# System utilities
IMAGE_INSTALL:append = " \
    htop \
    vim \
    nano \
    less \
    screen \
    tmux \
    bash \
    procps \
    util-linux \
    pciutils \
    usbutils \
    i2c-tools \
    lmsensors \
"

# Development tools
IMAGE_INSTALL:append = " \
    gcc \
    g++ \
    make \
    cmake \
    pkgconfig \
    strace \
    gdb \
    python3 \
"

# Kernel modules
IMAGE_INSTALL:append = " \
    kernel-modules \
"

# Firmware
IMAGE_INSTALL:append = " \
    firmware-th1520 \
"

# Extra rootfs space
IMAGE_ROOTFS_EXTRA_SPACE = "1048576"
