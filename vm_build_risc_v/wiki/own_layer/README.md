# Сборка собственного слоя Yocto для кластера (meta-cluster)
## 1. Подготовка окружения
### Установка зависимостей (если не установлены)
```sh
sudo apt update && sudo apt install -y gawk wget git diffstat unzip texinfo gcc g++ \
    build-essential chrpath socat cpio python3 python3-pip python3-pexpect xz-utils \
    debianutils iputils-ping python3-git python3-jinja2 libegl1 libsdl1.2-dev pylint \
    xterm python3-subunit mesa-common-dev zstd liblz4-tool
```

### Клонирование Yocto и слоев
```sh
mkdir ~/yocto-riscv
cd ~/yocto-riscv
git clone -b kirkstone git://git.yoctoproject.org/poky.git
git clone -b kirkstone https://github.com/riscv/meta-riscv.git
git clone -b kirkstone https://git.openembedded.org/meta-openembedded
git clone -b kirkstone git://git.yoctoproject.org/meta-virtualization
```

### Инициализация окружения Yocto
```sh
cd poky
source oe-init-build-env
```

---

## 2. Создание собственного слоя `meta-cluster`
```sh
cd ~/yocto-riscv
bitbake-layers create-layer meta-cluster
```
Добавляем слой в `bblayers.conf`:
```sh
nano ~/yocto-riscv/poky/build/conf/bblayers.conf
```
Добавляем строку:
```
BBLAYERS += "${TOPDIR}/../meta-cluster"
```

---

## 3. Создание заглушек для `controller` и `unit`

### `cluster-controller`
Создаем папку и файл рецепта:
```sh
mkdir -p ~/yocto-riscv/meta-cluster/recipes-cluster/controller
nano ~/yocto-riscv/meta-cluster/recipes-cluster/controller/cluster-controller.bb
```
Добавляем следующий код:
```bitbake
DESCRIPTION = "Cluster Controller Stub"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://controller.sh"

do_install() {
    install -d ${D}/usr/bin
    install -m 0755 ${WORKDIR}/controller.sh ${D}/usr/bin/controller
}
FILES:${PN} += "/usr/bin/controller"
```

Создаем заглушку `controller.sh`:
```sh
mkdir -p ~/yocto-riscv/meta-cluster/recipes-cluster/controller/files
nano ~/yocto-riscv/meta-cluster/recipes-cluster/controller/files/controller.sh
```
Добавляем:
```sh
#!/bin/sh
echo "Cluster Controller is running"
```
Делаем файл исполняемым:
```sh
chmod +x ~/yocto-riscv/meta-cluster/recipes-cluster/controller/files/controller.sh
```

---

### `cluster-unit`
Создаем папку и файл рецепта:
```sh
mkdir -p ~/yocto-riscv/meta-cluster/recipes-cluster/unit
nano ~/yocto-riscv/meta-cluster/recipes-cluster/unit/cluster-unit.bb
```
Добавляем код:
```bitbake
DESCRIPTION = "Cluster Unit Stub"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://unit.sh"

do_install() {
    install -d ${D}/usr/bin
    install -m 0755 ${WORKDIR}/unit.sh ${D}/usr/bin/unit
}
FILES:${PN} += "/usr/bin/unit"
```

Создаем заглушку `unit.sh`:
```sh
mkdir -p ~/yocto-riscv/meta-cluster/recipes-cluster/unit/files
nano ~/yocto-riscv/meta-cluster/recipes-cluster/unit/files/unit.sh
```
Добавляем:
```sh
#!/bin/sh
echo "Cluster Unit is running"
```
Делаем файл исполняемым:
```sh
chmod +x ~/yocto-riscv/meta-cluster/recipes-cluster/unit/files/unit.sh
```

---

## 4. Добавление пакетов в сборку
Редактируем `local.conf`:
```sh
nano ~/yocto-riscv/poky/build/conf/local.conf
```
Добавляем:
```
IMAGE_INSTALL:append = " cluster-controller cluster-unit"
```

---

## 5. Сборка образа
```sh
bitbake core-image-minimal
```

---

## 6. Запуск в QEMU и проверка
```sh
runqemu qemuriscv64
```
Проверяем заглушки:
```sh
controller
unit
```
Они должны вывести:
```
Cluster Controller is running
Cluster Unit is running
```