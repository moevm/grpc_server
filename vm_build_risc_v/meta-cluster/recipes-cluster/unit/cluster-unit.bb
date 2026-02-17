SUMMARY = "Cluster Unit"
DESCRIPTION = "A stub package for the cluster unit node"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://unit-init.sh"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/unit-init.sh ${D}${bindir}/unit-init.sh
}

FILES_${PN} = "${bindir}/unit-init.sh"
