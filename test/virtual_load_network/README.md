# Виртуальный тестовый стенд

## Что необходимо сделать
- [x] Настроить топологию сети и виртуальные машины
- [x] Добавить кросс-компиляцию controller под RISC-V
- [x] Добaвить кросс-компиляцию dpdk библиотеки
    - [x] На данный момент используется Docker контейнер внутри которого билдиться библиотека. Это медленно (минут 50, но делается один раз)
    - [ ] (\*) По хорошему необходимо создать отдельный sysroot с помощью Yocto Project внутри которого будем билдить
- [x] Запуск фильтров и контроллера
    - [x] Запуск фильтров происходит через ssh подключение и прописывание вручную комманды
    - [ ] (\*) По хорошему необходимо сделать запуск через systemd
- [ ] Взаимодействие controller и worker, пока не сделано:
    - [ ] Кросс компиляция worker вместе с dpdk-filter. На данный момент билдиться только сам dpdk-filter

## Подготовка

### Создание RISC-V образа
```sh
sudo scripts/create_image.sh
```

Пока скрипт из test-image переписан только под arch linux. Надо переписать его под ubuntu и запускать внутри docker контейнера

Подробности: `wiki/using_test_image.md`

## Конфигурация

Параметры в `.env`:

| Переменная       | По умолчанию         | Описание                          |
|------------------|----------------------|-----------------------------------|
| NUM_HOSTS_NET1   | 2                    | Генераторов в сети 1              |
| NUM_HOSTS_NET2   | 2                    | Генераторов в сети 2              |
| SUBNET1          | 10.0.0               | Подсеть 1                         |
| SUBNET2          | 10.0.1               | Подсеть 2                         |
| MGMT_SUBNET      | 10.0.2               | Управляющая подсеть               |
| QEMU_IMAGE       | ubuntu-...riscv64.img| Путь к RISC-V образу              |
| QEMU_MEMORY      | 4G                   | RAM на каждую VM                  |
| QEMU_CPUS        | 2                    | CPU на каждую VM                  |
| FILTER_RISCV_BIN | ../../worker/main-riscv-virt | Путь к бинарю фильтра     |
| CONTROLLER_BIN   | ../../controller/bin/grpc_server | Путь к контроллеру     |
| HUGEPAGES        | 1024                 | Количество hugepages              |

## Запуск

Виртуальные машины и сеть:
```sh
cd test/virtual_load_network
make run
```

Запуск фильтров осуществляется через подлкючение по ssh:
```
# for filter1
ssh-keygen -R 10.0.2.1
ssh ubuntu@10.0.2.1
# password ubuntu
# in VM:
sudo rm -rf /var/run/dpdk /dev/hugepages/rtemap_*
sudo LIBXDP_OBJECT_PATH=/usr/lib/riscv64-linux-gnu/bpf LD_LIBRARY_PATH=/mnt/lib /mnt/filter --no-pci --iova-mode=va -d /mnt/lib/librte_net_af_xdp.so -d /mnt/lib/librte_net_tap.so --

# Also for filter2
ssh-keygen -R 10.0.2.2
ssh ubuntu@10.0.2.2
# password ubuntu
# in VM:
sudo rm -rf /var/run/dpdk /dev/hugepages/rtemap_*
sudo LIBXDP_OBJECT_PATH=/usr/lib/riscv64-linux-gnu/bpf LD_LIBRARY_PATH=/mnt/lib /mnt/filter --no-pci --iova-mode=va -d /mnt/lib/librte_net_af_xdp.so -d /mnt/lib/librte_net_tap.so --
```

## Остановка
```sh
make stop
```
