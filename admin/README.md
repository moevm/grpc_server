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
    -I ../controller/pkg/proto/admin_service \
    ../controller/pkg/proto/admin_service/admin_service.proto
```

## Запуск контроллера (в отдельном терминале)
```bash
cd ../controller
bazel run //cmd/grpc_server:grpc_server
```

## Запуск клиента
```bash
python admin.py --file config.toml
```