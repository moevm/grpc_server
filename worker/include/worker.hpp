#ifndef WORKER_HPP
#define WORKER_HPP

#include "../include/communication.pb.hh"

#include <cstdint>
#include <string>

#define SOCKET_DIR "/run/controller/"
#define MAIN_SOCKET_NAME "main.sock"
#define EXPECTED_PULSE_TIME 60
#define MIN_PULSE_TIME 30
#define MAX_PULSE_TIME 45

enum class WorkerState {
  BOOTING,       // Инициализация
  FREE,          // Ожидает задачи
  BUSY,          // Выполняет задачу
  SHUTTING_DOWN, // Завершение работы
  ERROR          // Ошибка
};

class WorkerException : public std::exception {
  std::string msg;

public:
  const char *what() const noexcept { return msg.data(); }
  WorkerException(const char *msg) : msg(msg) {}
  WorkerException(const std::string &msg) : msg(msg) {}
  ~WorkerException() = default;
};

class Worker {
  std::string socket_path;
  int listener_fd = -1;
  uint64_t worker_id = 0;
  uint64_t current_task_id = 0;
  std::chrono::time_point<std::chrono::steady_clock> last_pulse_time;
  uint64_t pulse_interval = MIN_PULSE_TIME;

  std::string fetch_data;
  std::string extra_data;

  enum class InitResponse : uint64_t { OK = 1 };
  WorkerState state;
  void LogStateChange(WorkerState new_state);
  void SetState(WorkerState new_state);
  void SendPulse(PulseType type);
  int GetPulseTimeout();
  void HandleControlMessage(int client_fd);

  void HandleRestartControlMessage(WorkerResponse &resp);
  void HandleFetchControlMessage(WorkerResponse &resp);
  void HandleSetTaskControlMessage(const ControlMsg &msg, WorkerResponse &resp,
                                   const std::vector<char> &extra);
  void HandleGetStatusControlMessage(WorkerResponse &resp);

  static void ProcessTask_Static(Worker *worker, const std::vector<char> &data);

protected:
  static void ReadMessage(int fd, std::string &msg);
  static void WriteMessage(int fd, const std::string &msg);

  static void ReadExact(int fd, void *buf, size_t n);
  static void WriteExact(int fd, const void *buf, size_t n);

  template <class T> static inline void ReadExact(int fd, T &data) {
    ReadExact(fd, &data, sizeof(data));
  }

  template <class T> static inline void WriteExact(int fd, const T &data) {
    WriteExact(fd, &data, sizeof(data));
  }

  template <class T> static inline void ReadProtoMessage(int fd, T &msg) {
    std::string buf;
    ReadMessage(fd, buf);
    if (!msg.ParseFromArray(buf.data(), buf.size())) {
      throw WorkerException("failed to parse message");
    }
  }

  template <class T> static inline void WriteProtoMessage(int fd, T &msg) {
    std::string buf;
    if (!msg.SerializeToString(&buf)) {
      throw WorkerException("serialization failed");
    }
    WriteMessage(fd, buf);
  }

  inline void SetFetchData(const std::string &data) {
    fetch_data = data;
    SendPulse(PULSE_FETCH_ME);
  }

public:
  Worker();
  ~Worker();

  WorkerState GetState() const { return state; }
  virtual void ProcessTask(const std::vector<char> &data) = 0;
  void MainLoop();
};

#endif
