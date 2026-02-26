# Прошивка Lichee Pi 4A

Прошивка образа [RevyOS](https://docs.revyos.dev/) `20251226` (ядро 6.6) в eMMC через fastboot.

## Что понадобится

- Lichee Pi 4A ([спецификации](https://wiki.sipeed.com/hardware/en/lichee/th1520/lpi4a/1_intro.html))
- USB-C кабель
- Блок питания 5V 2A
- Компьютер с Linux и установленным `fastboot`

## Скачивание образа

Зеркало: https://mirror.iscas.ac.cn/revyos/extra/images/lpi4a/20251226/

U-Boot файл зависит от конфигурации платы:

| Конфигурация | Файл U-Boot |
|---|---|
| 8G RAM + 8G eMMC | `u-boot-with-spl-lpi4a_8gemmc.bin` |
| 8G RAM + 32G eMMC | `u-boot-with-spl-lpi4a.bin` |
| 16G RAM | `u-boot-with-spl-lpi4a-16g.bin` |

Пример для платы с 16G RAM:

```bash
wget https://mirror.iscas.ac.cn/revyos/extra/images/lpi4a/20251226/u-boot-with-spl-lpi4a-16g.bin
wget https://mirror.iscas.ac.cn/revyos/extra/images/lpi4a/20251226/boot-lpi4a-20251225_175338.ext4.zst
wget https://mirror.iscas.ac.cn/revyos/extra/images/lpi4a/20251226/root-lpi4a-20251225_175338.ext4.zst
```

Распаковка:

```bash
unzstd boot-lpi4a-20251225_175338.ext4.zst
unzstd root-lpi4a-20251225_175338.ext4.zst
```

## Установка fastboot

```bash
sudo apt install fastboot
```

## Вход в режим прошивки

1. Извлечь SD-карту, если вставлена.
2. Зажать кнопку BOOT на плате.
3. Не отпуская BOOT, подключить USB-C от платы к компьютеру.
4. Отпустить BOOT.

Проверить подключение:

```bash
lsusb | grep "T-HEAD"
```

Должно быть: `ID 2345:7654 T-HEAD USB download gadget`.

## Прошивка

Загрузить U-Boot в RAM и перезагрузиться:

```bash
sudo fastboot flash ram u-boot-with-spl-lpi4a-16g.bin
sudo fastboot reboot
sleep 1
```

Записать uboot, boot и root:

```bash
sudo fastboot flash uboot u-boot-with-spl-lpi4a-16g.bin
sudo fastboot flash boot boot-lpi4a-20251225_175338.ext4
sudo fastboot flash root root-lpi4a-20251225_175338.ext4
```

## Первый запуск

- **Логин:** `debian`
- **Пароль:** `debian`

## Ссылки

- [RevyOS — документация по прошивке LicheePi 4A](https://docs.revyos.dev/docs/Installation/licheepi4a/)
- [Sipeed Wiki — Lichee Pi 4A](https://wiki.sipeed.com/hardware/en/lichee/th1520/lpi4a/1_intro.html)
- [Sipeed Wiki — прошивка](https://wiki.sipeed.com/hardware/en/lichee/th1520/lpi4a/4_burn_image.html)
- [Зеркало образов RevyOS](https://mirror.iscas.ac.cn/revyos/extra/images/lpi4a/)
