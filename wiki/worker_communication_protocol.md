# Worker-Controller communication protocol

Будущий протокол коммуникации воркера и контроллера.

## Регистрация

Код protobuf:
```proto
enum PulseType {
  PULSE_REGISTER = 0;
  PULSE_OK = 1;
  PULSE_FETCH_ME = 2;
  PULSE_SHUTDOWN = 3;
}

enum ControllerError {
    CTRL_ERR_OK = 0;
    CTRL_ERR_UNKNOWN_TYPE = 1;
    CTRL_ERR_UNKNOWN_ID = 2;
    CTRL_ERR_FAILED = 3;
}

message WorkerPulse {
    PulseType type = 1;
    uint64 worker_id = 2;
    uint64 task_id = 3;
    uint64 next_pulse = 4;
}

message PulseResponse {
    ControllerError error = 1;
    optional uint64 worker_id = 2;
}
```

Контроллер слушает Unix-сокет `/run/controller/main.sock`. При подключении получает от воркера сообщение WorkerPulse, посылает PulseResponse и завершает соединение.

### REGISTER:

Поля *worker_id* и *task_id* игнорируются. Контроллер регистрирует воркера. При ошибке посылает соответствующее сообщение. Иначе посылает сообщение OK и передаёт воркеру его worker_id.

Если ошибок не произошло, то контроллер присваивает этому воркеру статус BOOTING. Воркер в это время создаёт `/run/controller/WORKER_ID.sock` и слушает подключения.
Контроллер переводит воркера из состояния *BOOTING* в состояние *FREE* как только получает от воркера очередной WorkerPulse. Поэтому воркер после успешного создания сокета посылает *OK WorkerPulse*.

### OK:

Оповещает контроллера о том, что воркер жив. Поле *next_pulse* -- время в секундах до следующего WorkerPulse. Если контроллер за это время не получает очередной WorkerPulse от этого воркера, то воркер считается "мёртвым" и удаляется из списка живых воркеров.

При сообщениях *OK*, *FETCH_ME*, *SHUTDOWN*, если воркера с таким *worker_id* не существует (не зарегистрирован или был зарегистрирован и позже удалён), то посылается соответствующее сообщение об ошибке (UNKNOWN_ID). Если в интересах воркера продолжить коммуникацию с воркером, то ему стоит зарегистрироваться заново.

Ошибка UNKNOWN_TYPE посылается в случае некорректного PulseType.

Ошибка FAILED посылается в случае любой другой ошибки.


Поле *task_id* содержит ID выполняемого воркером задания. Нулевой *task_id* обозначает отсутствие задания (нумерация заданий начинается с единицы). Если оно не совпадает с информацией, которой владеет контроллер, то контроллер посылает воркеру сообщение *RESTART* (см. Коммунакация по выделенному сокету). 

В PulseResponse поле *worker_id* не задаётся и игнорируется воркером.

### FETCH_ME:

Аналогично `OK`, но ещё дополнительно уведомляет контроллера о том, что воркер хочет послать контроллеру сообщение (см. Коммуникация по выделенному сокету, *FETCH*)

### SHUTDOWN:

Поля *task_id* и *next_pulse* игнорируются. Воркер объявляет себя "мёртвым", т.е. завершает текущую сессию. Контроллер должен удалить воркера из списка живых воркеров.

## Получение заданий

Код protobuf:
```proto
enum ControlType {
    CTRL_RESTART = 0;
    CTRL_FETCH = 1;
    CTRL_SET_TASK = 2;
    CTRL_GET_STATUS = 3;
}

message ControlMsg {
    ControlType type = 1;
    uint64 extra_size = 2;
    optional uint64 task_id = 3;
}

enum WorkerError {
    WORKER_ERR_OK = 0;
    WORKER_ERR_NO_FETCH = 1;
    WORKER_ERR_BUSY = 2;
    WORKER_ERR_TASK_FAILED = 3;
    WORKER_ERR_FAILED = 4;
}

message WorkerResponse {
    WorkerError error = 1;
    uint64 task_id = 2;
    uint64 extra_size = 3;
}
```

Воркер слушает `/run/controller/WORKER_ID.sock`. При подключении получает от контроллера сообщение ControlMsg. За ControlMsg следует последовательность байт длиной *extra_size*. Она тоже считывается. Далее воркер отправляет в ответ *WorkerResponse* и последовательность байт длиной *extra_size*.

Как и в случае с *ControllerError*, *WorkerError::FAILED* обозначает любую ошибку, не подходяющую по критериям к другим видам.

Если контроллер получает такую ошибку, значит воркера надо удалить из списка живых воркеров. Если воркер выполняет задание, то это задание надо переназначить на другого воркера.

В случае любой ошибки extra данные либо не содержат ничего (*extra_size* = 0), либо содержат более детальное сообщение об ошибке.

### RESTART:

Контроллер требует воркера завершить работу. Extra данные игнорируются воркером. 

Контроллер игнорирует любые ошибки и удаляет воркера из списка живых. // TODO: FIX?

Поле *ControlMsg::task_id* игнорируется.

### FETCH:

Контроллер запрашивает от воркера сообщение. Extra данные игнорируются воркером.
Контроллер в ответ посылает WorkerResponse с текущим *task_id* без extra данных.

Если контроллер получает *NO_FETCH*, то с воркера нечего считывать.

Если контроллер получает *TASK_FAILED*, значит воркеру не удалось выполнить задание. Контроллер снимает задание с этого и передает его другому.

В случае успеха Extra данные содержат решение задания.

Поле *ControlMsg::task_id* игнорируется.

### SET_TASK:

Контроллер устанавливает задание воркеру. Задание передаётся как extra данные. ID задания указан в *task_id*.

Если контроллер получает *BUSY*, то воркер уже занят. В таком случае задание назначается на другого воркера.


### GET_STATUS:

Контроллер запрашивает состояние воркера. Воркер посылает WorkerResponse без extra данных, указывает текущий *task_id*.


## ABI

Формат передачи сообщений:

| Offset | Type       | Name | Meaning                     |
| ------ | ---------- | ---- | --------------------------- |
| 0      | uint64     | size | Message size                | 
| 8      | byte[size] | msg  | Serialized protobuf message |


## TODO

* Add full english translation (google translate does funny things)
* Protobuf messages do not have a fixed size. This is inconvenient. The *size* field could be removed if messages had a fixed size.
