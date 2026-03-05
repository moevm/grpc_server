# Admin Client

## Установка зависимостей
```bash
pip install -r requirements.txt
```

## Генерация proto-файлов
```bash
python -m grpc_tools.protoc \
    --python_out=. \
    --grpc_python_out=. \
    -I . \
    admin_service.proto
```

## Запуск контроллера
```bash
cd ../controller
bazel run //cmd/grpc_server:grpc_server
```

## Запуск клиента
```bash
python admin.py --file config.toml
```