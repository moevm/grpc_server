SUMMARY = "Cluster Controller"
DESCRIPTION = "A stub package for the cluster controller node"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://controller-init.sh"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/controller-init.sh ${D}${bindir}/controller-init.sh
}

FILES_${PN} = "${bindir}/controller-init.sh"
