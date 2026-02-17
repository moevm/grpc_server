SUMMARY = "Go protocol buffers compiler plugin"
HOMEPAGE = "https://github.com/golang/protobuf"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=939cce1ec101726fa754e698ac871622"

SRC_URI = "git://github.com/golang/protobuf;branch=master;protocol=https;destsuffix=git"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

DEPENDS = "go-native"

GOARCH = "${@ "amd64" if d.getVar('TARGET_ARCH') == "x86_64" else "riscv64"}"

do_configure[noexec] = "1"
do_compile[network] = "1"

do_compile() {
    cd ${S}
    if [ -d "cmd/protoc-gen-go" ]; then
        cd cmd/protoc-gen-go
    elif [ -d "protoc-gen-go" ]; then
        cd protoc-gen-go
    else
        bbfatal "Cannot find protoc-gen-go directory"
    fi
    GOOS=${TARGET_GOOS} GOARCH=${GOARCH} go build -trimpath -o protoc-gen-go
}

do_install() {
    install -d ${D}${bindir}
    if [ -f ${S}/cmd/protoc-gen-go/protoc-gen-go ]; then
        install -m 0755 ${S}/cmd/protoc-gen-go/protoc-gen-go ${D}${bindir}/protoc-gen-go
    elif [ -f ${S}/protoc-gen-go/protoc-gen-go ]; then
        install -m 0755 ${S}/protoc-gen-go/protoc-gen-go ${D}${bindir}/protoc-gen-go
    else
        bbfatal "protoc-gen-go binary not found"
    fi
}

INSANE_SKIP:${PN} += "ldflags"

FILES:${PN} = "${bindir}"

BBCLASSEXTEND = "native"
