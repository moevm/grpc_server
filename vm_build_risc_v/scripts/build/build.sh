#!/bin/bash

set -e

YOCTO_BRANCH="scarthgap"
WORKDIR="/home/builder/yocto"

mkdir -p "${WORKDIR}"
sudo chown -R builder:builder "${WORKDIR}"
cd "${WORKDIR}"

if [ ! -d "poky" ]; then
    git clone -b "${YOCTO_BRANCH}" --depth=1 https://git.yoctoproject.org/poky
fi

cd poky

if [ ! -d "meta-openembedded" ]; then
    git clone -b "${YOCTO_BRANCH}" --depth=1 https://github.com/openembedded/meta-openembedded
fi

source oe-init-build-env build

bitbake-layers add-layer ../meta-openembedded/meta-oe 2>/dev/null || true
bitbake-layers add-layer ../meta-openembedded/meta-python 2>/dev/null || true
bitbake-layers add-layer ../meta-openembedded/meta-networking 2>/dev/null || true
bitbake-layers add-layer ../meta-openembedded/meta-filesystems 2>/dev/null || true
bitbake-layers add-layer /home/builder/meta-cluster 2>/dev/null || true
bitbake-layers add-layer /home/builder/meta-licheepi4a 2>/dev/null || true

if ! grep -q "### LICHEEPI4A CONFIG ###" conf/local.conf 2>/dev/null; then
    cat >> conf/local.conf << 'CONFEOF'
### LICHEEPI4A CONFIG ###
# Machine: Lichee Pi 4A with TH1520 SoC
MACHINE = "licheepi4a"

# Use RevyOS kernel 6.6 for full hardware support
PREFERRED_PROVIDER_virtual/kernel = "linux-revyos-th1520"

# Extra disk space in rootfs
IMAGE_ROOTFS_EXTRA_SPACE = "1048576"

# Accept all licenses (needed for some firmware blobs)
LICENSE_FLAGS_ACCEPTED = "commercial"

# Package management on target
EXTRA_IMAGE_FEATURES += "package-management"

# Disable network connectivity check (runs inside Docker)
CONNECTIVITY_CHECK_URIS = ""
### END LICHEEPI4A CONFIG ###
CONFEOF
fi

bitbake licheepi4a-dpdk-image

DEPLOY="tmp/deploy/images/licheepi4a"
BOOT_DIR=$(mktemp -d)
BOOT_IMG="${DEPLOY}/boot-licheepi4a.ext4"
BOOT_SIZE_MB=200

cp -v "${DEPLOY}/Image"                    "${BOOT_DIR}/"
cp -v "${DEPLOY}/fw_dynamic.bin"           "${BOOT_DIR}/"
cp -v "${DEPLOY}/light_aon_fpga.bin"       "${BOOT_DIR}/"

mkdir -p "${BOOT_DIR}/dtbs"
cp -v "${DEPLOY}/th1520-lichee-pi-4a.dtb"  "${BOOT_DIR}/dtbs/"

mkdir -p "${BOOT_DIR}/extlinux"
cat > "${BOOT_DIR}/extlinux/extlinux.conf" << 'EXTEOF'
default yocto
menu title Lichee Pi 4A Boot Menu

label yocto
    menu label Yocto DPDK Image
    linux /Image
    fdt /dtbs/th1520-lichee-pi-4a.dtb
    append root=PARTUUID=80a5a8e9-c744-491a-93c1-4f4194fd690a console=ttyS0,115200 rootwait rw earlycon clk_ignore_unused loglevel=7 rootfstype=ext4
EXTEOF

cat "${BOOT_DIR}/extlinux/extlinux.conf"

dd if=/dev/zero of="${BOOT_IMG}" bs=1M count=${BOOT_SIZE_MB}
mkfs.ext4 -d "${BOOT_DIR}" -L boot "${BOOT_IMG}"

rm -rf "${BOOT_DIR}"
echo "Created: ${BOOT_IMG}"
echo "Output artifacts in: ${DEPLOY}/"
