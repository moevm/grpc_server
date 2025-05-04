#ifndef WORKER_HPP
#define WORKER_HPP

#include <cstdint>
#include <string>

#define SOCKET_DIR "/run/controller/"
#define INIT_SOCKET_NAME "init.sock"

enum class WorkerState {
  BOOTING,       // Инициализация
  CONNECTING,    // Подключение к контроллеру
  READY,         // Ожидает задачи
  PROCESSING,    // Выполняет задачу
  SHUTTING_DOWN, // Завершение работы
  ERROR          // Ошибка
};

class WorkerException {
  std::string msg;

public:
  const std::string &what() { return msg; }
  WorkerException(const char *msg) : msg(msg) {}
};

class Worker {
  std::string socket_path;
  int listener_fd;

  enum class InitResponse : uint64_t { OK = 1 };
  WorkerState state;
  void LogStateChange(WorkerState new_state);
  void SetState(WorkerState new_state);

protected:
  static ssize_t ReadExact(int fd, void *buf, size_t n);
  static ssize_t WriteExact(int fd, const void *buf, size_t n);
  static int UnixRead(int fd, void *buf, size_t n);
  static int UnixWrite(int fd, const void *buf, size_t n);

public:
  Worker();
  ~Worker();

  WorkerState GetState() const { return state; }
  virtual void DoTask(int client_fd) = 0;
  void MainLoop();
};

#endif
