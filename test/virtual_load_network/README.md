# Тестовый стенд виртуальной нагрузки

## Установка

### DPDK
```sh
sudo apt-get install -y meson ninja-build python3-pyelftools libbpf-dev
git clone https://github.com/DPDK/dpdk.git
cd dpdk
meson setup -Denable_drivers=net/af_xdp,net/tap build
ninja -C build
sudo ninja -C build install
```

### Компиляция worker
```sh
cd worker
make -f Makefile.main_x86 virt
```

## Конфигурация

Параметры задаются в `.env`:

| Переменная    | По умолчанию              | Описание                         |
|---------------|---------------------------|----------------------------------|
| NUM_HOSTS     | 3                         | Количество генераторов трафика   |
| SUBNET        | 10.0.0                    | Подсеть (первые 3 октета)        |
| GATEWAY       | 10.0.0.254                | Адрес шлюза                      |
| DNS           | 8.8.8.8                   | DNS для контейнеров              |
| FILTER_PATH   | ../../worker/main-x86-virt| Путь до бинаря фильтра           |
| HUGEPAGES     | 1024                      | Количество hugepages             |

## Запуск
```sh
cd test/virtual_load_network
sudo scripts/start.sh
```

Скрипт выполняет:
1. Настройку hugepages
2. Запуск контейнеров-генераторов через `docker compose`
3. Создание veth пары и tc mirroring на bridge compose-сети
4. Настройку NAT
5. Запуск DPDK фильтра

## Остановка
```sh
sudo scripts/stop.sh
```
