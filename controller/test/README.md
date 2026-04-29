# Запуск теста

### 1. Сборка компонентов

```bash
cd ../worker
bazel build //:worker
```

### 2. Запуск интеграционного теста

```bash
export TEST_CONTROLLER_ADDR="<YOUR_ADDR>"  # например localhost:0 
cd ../controller
./test/run.sh
```