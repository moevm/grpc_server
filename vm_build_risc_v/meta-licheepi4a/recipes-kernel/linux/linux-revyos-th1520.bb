SUMMARY = "RevyOS Linux Kernel for T-Head TH1520 (Lichee Pi 4A)"
DESCRIPTION = "Linux kernel 6.6 from RevyOS project, optimized for TH1520 SoC \
with full hardware support including WiFi, USB, GPU, Ethernet."
SECTION = "kernel"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

inherit kernel

DEPENDS += "dtc-native"

BRANCH = "master"
SRC_URI = "git://github.com/revyos/th1520-linux-kernel.git;protocol=https;branch=${BRANCH} \
           file://dpdk.cfg \
           file://extra.cfg \
           "

SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

LINUX_VERSION ?= "6.6"
LINUX_VERSION_EXTENSION = "-th1520-revyos"
PV = "${LINUX_VERSION}+git"

KERNEL_VERSION_SANITY_SKIP = "1"

KBUILD_DEFCONFIG = "defconfig"

do_configure:prepend() {
    if ! grep -q "CONFIG_ARCH_XUANTIE" ${B}/.config 2>/dev/null; then
        echo "CONFIG_ARCH_XUANTIE=y" >> ${WORKDIR}/extra.cfg
    fi
}

COMPATIBLE_MACHINE = "(licheepi4a)"
