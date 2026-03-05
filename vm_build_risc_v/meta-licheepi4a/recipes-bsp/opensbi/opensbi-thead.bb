SUMMARY = "OpenSBI for T-Head TH1520 SoC"
DESCRIPTION = "RISC-V Open Source Supervisor Binary Interface implementation \
from RevyOS project, with T-Head TH1520 platform support."
LICENSE = "BSD-2-Clause"
LIC_FILES_CHKSUM = "file://COPYING.BSD;md5=42dd9555eb177f35150cf9aa240b61e5"

inherit deploy

BRANCH = "th1520"
SRC_URI = "git://github.com/revyos/thead-opensbi.git;branch=${BRANCH};protocol=https"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

DEPENDS += "dtc-native"

EXTRA_OEMAKE += "PLATFORM=${RISCV_SBI_PLAT} I=${D} FW_PIC=n CLANG_TARGET="

do_configure[noexec] = "1"

do_compile() {
    oe_runmake
}

do_install() {
    install -d ${D}/share/opensbi
    install -m 0644 ${S}/build/platform/${RISCV_SBI_PLAT}/firmware/fw_dynamic.bin ${D}/share/opensbi/
    install -m 0644 ${S}/build/platform/${RISCV_SBI_PLAT}/firmware/fw_dynamic.elf ${D}/share/opensbi/
}

do_deploy() {
    install -m 0644 ${S}/build/platform/${RISCV_SBI_PLAT}/firmware/fw_dynamic.bin ${DEPLOYDIR}/
    install -m 0644 ${S}/build/platform/${RISCV_SBI_PLAT}/firmware/fw_dynamic.elf ${DEPLOYDIR}/
}

addtask deploy after do_install

FILES:${PN} += "/share/opensbi/*"

COMPATIBLE_MACHINE = "(licheepi4a)"
INHIBIT_PACKAGE_STRIP = "1"
SECURITY_CFLAGS = ""
