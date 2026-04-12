# Запуск теста

### 1. Сборка компонентов

```bash
cd controller
bazel build //cmd/grpc_server:grpc_server

cd ../worker
bazel build //:worker
```

### 2. Запуск интеграционного теста

```bash
cd ../controller
./test/run.sh
```