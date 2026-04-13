# Драйвера dpdk
DPDK должен быть собран с  драйверами net/af_xdp net/tap


# Кросс-компиляция 

## Окружение
Скрипт `scripts/setup-riscv-env.sh` автоматически скачивает (при необходимости) и собирает DPDK 23.11 для архитектуры RISC-V.

```bash
./scripts/setup-riscv-env.sh
```

## SQLite
Если целевая архитектура — RISC-V, SQLite необходимо собрать кросс-компилятором.

```bash
wget https://www.sqlite.org/2024/sqlite-autoconf-3460100.tar.gz
tar -xzf sqlite-autoconf-3460100.tar.gz
cd sqlite-autoconf-3460100

./configure --host=riscv64-linux-gnu --prefix=/path/to/sqlite3-riscv-install
make -j$(nproc)
make install
```

После установки в указанном prefix появятся подкаталоги include/ и lib/ с необходимыми файлами.



# Создание пары veth и TAP-устройства

```bash
sudo ./scripts/set_virt_dev_for_test_xdp.sh
```
Скрипт создаёт пару veth0 - veth1


```bash
sudo ./scripts/set_tap_dev.sh
```
Скрипт создаёт TAP-устройство tap0



# Сборка проекта
Для реальных портов (eth0/eth1):
```bash
make -f Makefile.main_riscv all
```

Для виртуальных портов (veth0/veth1 + tap0):
```bash
make -f Makefile.main_riscv virt
```
Определение макроса -DVIRT_PORTS переключает программу на использование виртуальных интерфейсов.


Перед запуском рекомендуется выполнить скрипт настройки виртуальных устройств:
```bash
sudo ./scripts/set_virt_dev_for_test_xdp.sh
```


# Очистка
```bash
make -f Makefile.main_riscv clean
```

# Запуск
Программа требует прав суперпользователя (для работы с DPDK и XDP):
```bash
sudo ./main-riscv-virt
```


# Примечания
Кэш DNS автоматически сохраняется в cache.db (SQLite) и восстанавливается при перезапуске.

Периодическое сохранение кэша происходит каждый час с помощью таймеров DPDK.
