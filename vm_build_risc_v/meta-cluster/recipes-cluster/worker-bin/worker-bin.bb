SUMMARY = "Worker"
DESCRIPTION = "Worker program for cluster"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "git://github.com/moevm/grpc_server;branch=main;protocol=https;destsuffix=git"
SRCREV = "3bf8b7841cde8c3c4c684f0ae18ff80f950e7d8b"

S = "${WORKDIR}/git"

DEPENDS = "openssl curl prometheus-cpp"

do_compile() {
    cd ${S}/worker/src
    ${CXX} ${CXXFLAGS} ${LDFLAGS} -o worker \
        main.cpp md_calculator.cpp file.cpp metrics_collector.cpp worker.cpp \
        -lssl -lcrypto -lprometheus-cpp-push \
        -lprometheus-cpp-core -lcurl
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${S}/worker/src/worker ${D}${bindir}/worker
}

FILES_${PN} = "${bindir}/worker"
 