SUMMARY = "U-Boot for T-Head TH1520 (Lichee Pi 4A)"
DESCRIPTION = "U-Boot bootloader for TH1520-based boards from RevyOS project. \
Includes SPL, secure boot libraries, LPDDR4 init, and full LPi4A board support."

LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=30503fd321432fc713238f582193b78e"

SRC_URI = " \
    git://github.com/revyos/thead-u-boot.git;protocol=https;branch=th1520 \
"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"
B = "${S}"

DEPENDS += "dtc-native bison-native flex-native swig-native python3-native bc-native openssl-native"

UBOOT_MACHINE = "light_lpi4a_defconfig"

EXTRA_OEMAKE = " \
    CROSS_COMPILE='${TARGET_PREFIX}' \
    ARCH=riscv \
    HOSTCC='${BUILD_CC}' \
    HOSTCXX='${BUILD_CXX}' \
    HOSTCFLAGS='${BUILD_CFLAGS}' \
    HOSTLDFLAGS='${BUILD_LDFLAGS}' \
"

do_configure() {
    oe_runmake ${UBOOT_MACHINE}
}

do_compile() {
    LIBGCC_DIR=$(dirname $(${CC} -print-libgcc-file-name))
    oe_runmake PLATFORM_LIBGCC="-L ${LIBGCC_DIR} -lgcc"
}

inherit deploy

do_install() {
    :
}

do_deploy() {
    if [ -f ${B}/u-boot-with-spl.bin ]; then
        install -m 644 ${B}/u-boot-with-spl.bin ${DEPLOYDIR}/
    fi
    if [ -f ${B}/u-boot.bin ]; then
        install -m 644 ${B}/u-boot.bin ${DEPLOYDIR}/
    fi
    if [ -f ${B}/spl/u-boot-spl.bin ]; then
        install -m 644 ${B}/spl/u-boot-spl.bin ${DEPLOYDIR}/
    fi
}

addtask deploy after do_compile

TOOLCHAIN = "gcc"
COMPATIBLE_MACHINE = "(licheepi4a)"
PACKAGE_ARCH = "${MACHINE_ARCH}"
PARALLEL_MAKE = ""