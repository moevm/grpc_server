# Инструкция по запуску тестовго стенда из N хостов

## Установка и подготовка окружения

### Установка DPDK
```sh
sudo pacman -S meson ninja python-pyelftools
git clone https://github.com/DPDK/dpdk.git
cd dpdk
meson setup -Denable_drivers=net/af_xdp build
ninja -C build
sudo ninja -C build install
```

### Компиляция worker
```sh
cd worker
make -f Makefile.main_x86 virt
```

## Запуск тестового стенда
```sh
cd test/virtual_load_network 
make # сборка docker образа для генератора трафика
make run [N] # запуск N хостов, если не указывать то по умолчанию 3 хоста
```

## Ожидаемый вывод инициализации тестового стенда и работы программы фильтрации пактов DPDK
```sh
sudo scripts/teardown.sh 3
teardown: 3 hosts
gen1
[-] gen1 removed
gen2
[-] gen2 removed
gen3
[-] gen3 removed
[-] bridge br0 removed
[-] veth0/veth1 removed
sudo scripts/setup.sh 3
setup: 3 hosts
vm.drop_caches = 3
[+] hugepages: 1024
[+] bridge br0 (10.0.0.254)
[+] tc mirror: bridge -> veth1 -> veth0 (DPDK port_in)
net.ipv4.ip_forward = 1
[+] NAT
f87b5e86d4192a756b809488474441f143863a7e8a5da8457a9a202ec2bd4287
[+] gen1: 10.0.0.1/24
65b9f9c56686dcc4e5c1d5701459b65a5e8398886346e427f69afebe610f2f42
[+] gen2: 10.0.0.2/24
d8d7edcdfd18af3d52ead5839906da62a004c9d01d0f27fa1417b9c2941c61de
[+] gen3: 10.0.0.3/24
sudo sh -c 'LD_LIBRARY_PATH=/usr/local/lib ../../worker/main-x86-virt --no-pci --' | grep "port = 53;"
EAL: Detected CPU lcores: 8
EAL: Detected NUMA nodes: 1
EAL: Detected shared linkage of DPDK
EAL: Multi-process socket /var/run/dpdk/rte/mp_socket
EAL: Selected IOVA mode 'PA'
libbpf: elf: skipping unrecognized data section(7) .xdp_run_config
libbpf: elf: skipping unrecognized data section(8) xdp_metadata
libbpf: elf: skipping unrecognized data section(5) xdp_metadata
libbpf: elf: skipping unrecognized data section(5) xdp_metadata
libbpf: elf: skipping unrecognized data section(5) xdp_metadata
libbpf: elf: skipping unrecognized data section(5) xdp_metadata
libbpf: elf: skipping unrecognized data section(5) xdp_metadata
libbpf: elf: skipping unrecognized data section(7) .xdp_run_config
libbpf: elf: skipping unrecognized data section(8) xdp_metadata
libbpf: elf: skipping unrecognized data section(5) xdp_metadata
libbpf: elf: skipping unrecognized data section(5) xdp_metadata
libbpf: elf: skipping unrecognized data section(5) xdp_metadata
[PKT] port = 53; domain = www.google.com
[PKT] port = 53; domain = www.google.com
[PKT] port = 53; domain = www.google.com
[PKT] port = 53; domain = www.google.com
[PKT] port = 53; domain = www.google.com
[PKT] port = 53; domain = www.google.com
```

## Очистка тестового стенда
Удаляет созданные N хостов:
```sh
make clean [N]
```
