#!/bin/bash

set -e

sudo chown builder /home/builder/qemu

cd /home/builder/qemu/

if [ ! -d "poky" ]; then
  git clone -b scarthgap https://git.yoctoproject.org/poky
fi

cd poky

if [ ! -d "meta-virtualization" ]; then
  git clone -b scarthgap git://git.yoctoproject.org/meta-virtualization
fi

if [ ! -d "meta-openembedded" ]; then
  git clone -b scarthgap https://github.com/openembedded/meta-openembedded
fi

mkdir -p vm_build_risc_v
rm -rf vm_build_risc_v/meta-cluster
cp -r /home/builder/meta-cluster vm_build_risc_v/

# sed -i 's/LAYERSERIES_COMPAT_meta-cluster = .*/LAYERSERIES_COMPAT_meta-cluster = "kirkstone walnascar whinlatter scarthgap"/' vm_build_risc_v/meta-cluster/conf/layer.conf

source oe-init-build-env

bitbake-layers add-layer ../meta-openembedded/meta-oe ../meta-openembedded/meta-python \
  ../meta-openembedded/meta-networking ../meta-openembedded/meta-filesystems ../meta-virtualization \
  ../vm_build_risc_v/meta-cluster

echo "MACHINE ?= \"qemuriscv64\"" >>conf/local.conf
echo "DISTRO_FEATURES:append = \" virtualization\"" >>conf/local.conf
echo "IMAGE_INSTALL:append = \" docker docker-compose git controller-bin worker-bin dpdk dpdk-examples\"" >>conf/local.conf
echo "IMAGE_ROOTFS_EXTRA_SPACE = \" 1048576\"" >>conf/local.conf

# # Настройки для надежной работы с Git
# echo "BB_NUMBER_THREADS = \"8\"" >>conf/local.conf
# echo "PARALLEL_MAKE = \"-j 8\"" >>conf/local.conf
# echo "CONNECTIVITY_CHECK_URIS = \"\"" >>conf/local.conf
# echo "BB_GIT_SHALLOW = \"0\"" >>conf/local.conf
# echo "BB_GENERATE_MIRROR_TARBALLS = \"0\"" >>conf/local.conf
# echo "FETCHCMD_git = \"git -c http.postBuffer=524288000 -c http.maxRequestBuffer=100M -c core.compression=0\"" >>conf/local.conf

bitbake core-image-minimal
