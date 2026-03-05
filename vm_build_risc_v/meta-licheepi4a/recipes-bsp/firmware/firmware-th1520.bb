SUMMARY = "TH1520 AON firmware binary"
DESCRIPTION = "Firmware blob for the TH1520 Always-On subsystem. \
Required by the kernel for power management and co-processor communication."
HOMEPAGE = "https://github.com/revyos/th1520-boot-firmware"
LICENSE = "CLOSED"
SRC_URI = "git://github.com/revyos/th1520-boot-firmware.git;branch=master;protocol=https"
SRCREV = "725756411ecc20f2c2dbc5ea6b8e5aacc6f83aad"

S = "${WORKDIR}/git"

inherit deploy

do_install() {
    install -d ${D}${nonarch_base_libdir}/firmware
    if [ -f ${S}/addons/boot/light_aon_fpga.bin ]; then
        install -m 0644 ${S}/addons/boot/light_aon_fpga.bin ${D}${nonarch_base_libdir}/firmware/
    fi
}

do_deploy() {
    if [ -f ${S}/addons/boot/light_aon_fpga.bin ]; then
        install -m 0644 ${S}/addons/boot/light_aon_fpga.bin ${DEPLOYDIR}/
    fi
}

addtask deploy after do_install

FILES:${PN} = "${nonarch_base_libdir}/firmware/light_aon_fpga.bin"

COMPATIBLE_MACHINE = "(licheepi4a)"
