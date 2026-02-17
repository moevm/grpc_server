Инструкция по запуску приложения DPDK на riscv

```sh
make build
make yocto
make run
```

После выполнения комманд, вводим логин `root` и запускаем программу:

```sh
dpdk-helloworld --no-huge
```
