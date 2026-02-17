SUMMARY = "Prometheus Client Library for Modern C++"
HOMEPAGE = "https://github.com/jupp0r/prometheus-cpp"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "git://github.com/jupp0r/prometheus-cpp.git;protocol=https;branch=master;destsuffix=git"
SRCREV = "v1.3.0"

S = "${WORKDIR}/git"

DEPENDS = "zlib openssl curl"

inherit cmake

EXTRA_OECMAKE = " \
    -DBUILD_SHARED_LIBS=ON \
    -DENABLE_PUSH=ON \
    -DENABLE_TESTING=OFF \
    -DUSE_THIRDPARTY_LIBRARIES=OFF \
    -DENABLE_COMPRESSION=OFF \
    -DOVERRIDE_CATCH=OFF \
    -DGENERATE_PKGCONFIG=ON \
    -DENABLE_PULL=OFF \
"

OECMAKE_SOURCEPATH = "${S}"

do_configure:prepend() {
    bbnote "=== DEBUG: Content of ${S} ==="
    ls -la ${S}
    bbnote "=== Looking for CMakeLists.txt ==="
    find ${S} -name "CMakeLists.txt" -type f
}