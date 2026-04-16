#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

IMAGE="ubuntu-24.04.4-preinstalled-server-riscv64.img"
IMAGE_XZ="${IMAGE}.xz"
IMAGE_URL="https://cdimage.ubuntu.com/releases/24.04/release/${IMAGE_XZ}"
DPDK_DEST="$SCRIPT_DIR/../../worker/dpdk-riscv-install"
RUNTIME_DEST="$SCRIPT_DIR/shared/lib"

if ! docker buildx version &>/dev/null; then
    echo "Docker buildx required. Install docker-buildx."
    exit 1
fi

echo "Building DPDK for RISC-V in Docker..."
docker buildx build --platform linux/riscv64 -t dpdk-riscv-builder -f Dockerfile.dpdk --load .

echo "Extracting sysroot for cross-compilation..."
rm -rf "$DPDK_DEST"
mkdir -p "$DPDK_DEST"
CONTAINER=$(docker create --platform linux/riscv64 dpdk-riscv-builder)
docker cp "$CONTAINER:/output/include" "$DPDK_DEST/include"
docker cp "$CONTAINER:/output/lib" "$DPDK_DEST/lib"

echo "Extracting runtime libraries..."
rm -rf "$RUNTIME_DEST"
mkdir -p "$RUNTIME_DEST"
docker cp "$CONTAINER:/output/runtime/." "$RUNTIME_DEST/"
docker rm "$CONTAINER"

echo "Sysroot: $DPDK_DEST"
echo "Runtime libs: $RUNTIME_DEST"

if ! cat /proc/sys/fs/binfmt_misc/qemu-riscv64 &>/dev/null; then
    echo ':qemu-riscv64:M::\x7fELF\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\xf3\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:/usr/bin/qemu-riscv64-static:FPC' | sudo tee /proc/sys/fs/binfmt_misc/register >/dev/null
fi

if [ ! -f "$IMAGE" ]; then
    if [ ! -f "$IMAGE_XZ" ]; then
        echo "Downloading Ubuntu RISC-V image..."
        wget "$IMAGE_URL"
    fi
    xz -dk "$IMAGE_XZ"
fi

echo "Preparing VM image (installing libatomic)..."
sudo losetup -D 2>/dev/null || true
LOOP=$(sudo losetup -f)
sudo losetup -P "$LOOP" "$IMAGE"
sudo mkdir -p /mnt/riscv-rootfs
sudo mount "${LOOP}p1" /mnt/riscv-rootfs
sudo cp /usr/bin/qemu-riscv64-static /mnt/riscv-rootfs/usr/bin/
sudo rm -f /mnt/riscv-rootfs/etc/resolv.conf
sudo cp /etc/resolv.conf /mnt/riscv-rootfs/etc/resolv.conf
sudo chroot /mnt/riscv-rootfs /bin/bash -c "apt update && apt install -y libatomic1 libnuma1"
sudo umount /mnt/riscv-rootfs
sudo losetup -d "$LOOP"

echo "Done. Image ready: $IMAGE"
