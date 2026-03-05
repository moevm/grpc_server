SUMMARY = "Data Plane Development Kit - helloworld example for RISC-V"
HOMEPAGE = "http://dpdk.org"
LICENSE = "BSD-3-Clause & LGPL-2.1-only & GPL-2.0-only"
LIC_FILES_CHKSUM = "file://license/gpl-2.0.txt;md5=b234ee4d69f5fce4486a80fdaf4a4263 \
                    file://license/lgpl-2.1.txt;md5=4b54a1fd55a448865a0b32d41598759d \
                    file://license/bsd-3-clause.txt;md5=0f00d99239d922ffd13cabef83b33444"

SRC_URI = "git://dpdk.org/git/dpdk-stable;protocol=https;branch=23.11 \
           file://0001-config-meson-get-cpu_instruction_set-from-meson-opti.patch \
           "
SRCREV = "62f583c49bf67dd4d6733ece14e55fe6839e66d1"

S = "${WORKDIR}/git"

DEPENDS = "numactl python3-pyelftools-native"

inherit meson pkgconfig

EXTRA_OEMESON = " \
    -Dplatform=generic \
    -Dcpu_instruction_set=rv64gc \
    -Dexamples=helloworld \
    -Denable_docs=false \
    -Dtests=false \
    -Ddisable_drivers=* \
    -Dmax_lcores=128 \
    -Dmax_numa_nodes=1 \
"

EXTRA_OEMESON:append:riscv64 = " \
    --cross-file ${WORKDIR}/riscv-dpdk-cross.ini \
"

do_configure:prepend() {
    cat > ${WORKDIR}/riscv-dpdk-cross.ini << 'EOF'
[properties]
vendor_id = 'generic'
arch_id = 'generic'
numa = false
EOF
}

do_install:append() {
    if [ -f ${B}/examples/dpdk-helloworld ]; then
        install -d ${D}${bindir}
        install -m 0755 ${B}/examples/dpdk-helloworld ${D}${bindir}/dpdk-helloworld
    fi

    rm -rf ${D}${datadir}/dpdk/examples || true
    rm -rf ${D}${prefix}/share/dpdk/examples || true
}

PACKAGES =+ "${PN}-examples"

FILES:${PN} += " \
    ${libdir}/*.so* \
    ${libdir}/dpdk/ \
"
FILES:${PN}-examples = "${bindir}/dpdk-helloworld"
FILES:${PN}-dev = "${includedir} ${libdir}/pkgconfig"

RDEPENDS:${PN} += "numactl"
RDEPENDS:${PN}-examples += "${PN}"

INSANE_SKIP:${PN} += "dev-so"

COMPATIBLE_MACHINE = "(qemuriscv64|licheepi4a)"
