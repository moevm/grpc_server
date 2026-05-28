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

## Пример toml файла с политикой
```toml
[global.rules]                                   # Глобальные правила для всех воркеров
block_categories = ["Gambling", "Weapons"]       # Категории для блокировки
block_domains = ["youtube.com", "tiktok.com"]    # Домены для блокировки
allow_domains = ["github.com", "stackoverflow.com"] # Разрешённые домены (приоритет над блокировкой)
block_ips = ["192.168.0.1", "2001:0db8:85a3:0000:0000:8a2e:0370:7334"]      # IP для блокировки (IPv4 и IPv6)
allow_ips = ["8.8.8.8"]                           # Разрешённые IP 
ttl_ip = 604800                                   # TTL кэша IP 
ttl_domain = 604800                               # TTL кэша доменов 
min_trust_level = 5                               # Мин. уровень доверия 

[global.rules.block_by_trust]                     # Блокировка по уровню доверия
ENTERTAINMENT = 6                                 
NEWS = 4                                          

[filters.filter_1]                                # Правила для воркера #1
block_categories = ["Weapons", "Malware"]         # Доп. категории к глобальным
block_domains = ["instagram.com"]                 # Доп. домены к глобальным
allow_domains = ["vk.com"]                        # Доп. разрешённые домены
min_trust_level = 0                               # Переопределяет глобальный 

[filters.filter_1.block_by_trust]                 # Уровни доверия для воркера #1
SOCIAL = 8                                       
ENTERTAINMENT = 7                                

[filters.filter_2]                                # Правила для воркера #2
block_categories = ["Malware"]                    # Доп. категории к глобальным
allow_domains = ["github.com", "gitlab.com"]      # Доп. разрешённые домены

```

### Возможные категории
- Adult Content (Pornography and adult entertainment)
- Gambling (Online casinos, betting, lotteries)
- Drugs (Illegal substances)
- Violence (Violent content)
- Weapons (Firearms, explosives)
- Malware (Viruses and malicious software)
- Social media 
- Hate Speech (Discrimination, extremism)
- Anonymizers (VPN, proxies to bypass blocks)
- Online shop


## Запуск клиента
### Отправить новую политику на контроллер

```bash
python admin.py load --file <your_policy>.toml
```

### Получить текущую политику с контроллера

```bash
python admin.py get --save <your_policy>.toml
```

### Включить/отключить фильтрацию на воркере

```bash
python admin.py toggle --id 1 --on  #id - worker id
python admin.py toggle --id 1 --off #id - worker id
```


## Описание применения политики из конфига
Политика для каждого фильтра формируется путём объединения глобальных правил и индивидуальных настроек конкретного фильтра.

### Глобальный уровень 
Сначала загружается общий TOML-файл конфигурации. В нём есть раздел [global.rules], который содержит правила, применяемые ко всем фильтрам без исключения:
- Какие категории сайтов блокировать всегда
- Какие домены и ip в чёрном списке
- Какие домены и ip в белом списке
- Пороги доверия для разных категорий
- Минимальный уровень доверия по умолчанию
- Время жизни кэша для ip и домена

Эти правила образуют глобальную политику, одинаковую для всех фильтров.

### Локальный уровень
В том же файле есть раздел [filters], где для каждого фильтра можно задать свои правила - например, [filters.filter_1], [filters.filter_2] и т.д.

Локальные правила не заменяют глобальные, а дополняют и уточняют их:
- Если в локальных правилах указаны дополнительные категории для блокировки, они добавляются к глобальным
- Если указаны дополнительные домены или ip, они добавляются в соответствующие списки
- Пороги доверия для категорий могут переопределяться (если для той же категории указано новое значение)
- Минимальный уровень доверия может быть изменён для конкретного фильтра

### Финальная политика
Когда фильтр запрашивает политику, контроллер делает следующее:
1. Берёт глобальные правила как основу
2. Накладывает поверх них локальные правила конкретного фильтра
3. Объединяет списки (добавляет новые элементы, не удаляя старые)
4. Перезаписывает отдельные параметры, если они заданы локально

Таким образом, каждый фильтр получает индивидуальную политику, которая наследует общие правила, но может быть более строгой или более мягкой в зависимости от настроек его сегмента сети.
