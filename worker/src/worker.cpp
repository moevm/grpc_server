#include "../include/worker.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <endian.h>
#include <poll.h>
#include <spdlog/spdlog.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

void Worker::LogStateChange(WorkerState new_state) {
  const char *state_names[] = {"BOOTING", "FREE", "BUSY", "SHUTTING_DOWN",
                               "ERROR"};

  spdlog::info("Switch states: {} -> {}", state_names[static_cast<int>(state)],
               state_names[static_cast<int>(new_state)]);
}

void Worker::SetState(WorkerState new_state) {
  if (state != new_state) {
    LogStateChange(new_state);
    state = new_state;
  }
}

void Worker::ReadExact(int fd, void *buf, size_t n) {
  size_t total = 0;
  ssize_t r;

  while (total < n) {
    r = read(fd, (char *)buf + total, n - total);
    if (r < 0) {
      if (errno == EINTR) {
        continue;
      }
      throw WorkerException(std::string("ReadExact failed with error ") +
                            strerror(errno));
    } else if (r == 0) {
      throw WorkerException("ReadExact failed with Premature EOF");
    }
    total += r;
  }
}

void Worker::WriteExact(int fd, const void *buf, size_t n) {
  size_t total = 0;
  ssize_t w;
  while (total < n) {
    w = write(fd, (const char *)buf + total, n - total);
    if (w < 0) {
      if (errno == EINTR) {
        continue;
      }
      throw WorkerException(std::string("WriteExact failed with error ") +
                            strerror(errno));
    }
    total += w;
  }
}

void Worker::ReadMessage(int fd, std::string &msg) {
  uint64_t length_le64 = 0;
  Worker::ReadExact(fd, length_le64);
  uint64_t length = le64toh(length_le64);
  msg.resize(length);
  Worker::ReadExact(fd, (void *)msg.data(), length);
}

void Worker::WriteMessage(int fd, const std::string &msg) {
  uint64_t length_le64 = htole64(msg.size());
  Worker::WriteExact(fd, length_le64);
  Worker::WriteExact(fd, msg.data(), msg.size());
}

void Worker::SendPulse(PulseType type) {
  int main_fd = 0;
  try {
    WorkerPulse pulse;
    pulse.set_type(type);
    pulse.set_worker_id(worker_id);
    pulse.set_task_id(current_task_id);
    pulse.set_next_pulse(EXPECTED_PULSE_TIME);
    
    spdlog::info("Sending pulse... {{{}}}", pulse.ShortDebugString());

    main_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (main_fd < 0)
      throw WorkerException(std::string("socket: ") + strerror(errno));

    sockaddr_un addr{.sun_family = AF_UNIX};
    strncpy(addr.sun_path, SOCKET_DIR MAIN_SOCKET_NAME,
            sizeof(addr.sun_path) - 1);

    if (connect(main_fd, (sockaddr *)&addr, sizeof(addr))) {
      throw WorkerException(std::string("connect: ") + strerror(errno));
    }

    WriteProtoMessage(main_fd, pulse);
    spdlog::info("OK. Waiting for response");

    pulse_interval =
        MIN_PULSE_TIME + (rand() % (MAX_PULSE_TIME - MIN_PULSE_TIME + 1));
    last_pulse_time = std::chrono::steady_clock::now();

    PulseResponse response;
    ReadProtoMessage(main_fd, response);
    
    if (response.error() != CTRL_ERR_OK) {
      throw WorkerException(ControllerError_Name(response.error()));
    }
    close(main_fd);

    if (type == PULSE_REGISTER) {
      worker_id = response.worker_id();
    }

    spdlog::info("Received {{{}}}", response.ShortDebugString());
  } catch (const std::exception &e) {
    close(main_fd);
    SetState(WorkerState::ERROR);
    throw WorkerException(std::string("SendPulse: ") + e.what());
  }
}

Worker::Worker() : listener_fd(-1), state(WorkerState::BOOTING) {
  SendPulse(PULSE_REGISTER);

  socket_path = std::string(SOCKET_DIR) + std::to_string(worker_id) + ".sock";
  unlink(socket_path.c_str());

  spdlog::info("worker_id: {}", worker_id);
  spdlog::info("socket_path: {}", socket_path);
  srand(worker_id); // for random pulse time generation

  listener_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listener_fd < 0) {
    throw WorkerException("socket() failed");
  }

  sockaddr_un addr{.sun_family = AF_UNIX};
  strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

  if (bind(listener_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    close(listener_fd);
    listener_fd = -1;
    throw WorkerException("bind() failed");
  }

  if (listen(listener_fd, 100) < 0) {
    close(listener_fd);
    listener_fd = -1;
    throw WorkerException("listen() failed");
  }

  SendPulse(PULSE_OK);
  SetState(WorkerState::FREE);
}

Worker::~Worker() {
  SetState(WorkerState::SHUTTING_DOWN);

  try {
    SendPulse(PULSE_SHUTDOWN);
  } catch (...) {
  } // ignore errors cuz we don't care

  if (listener_fd != -1)
    close(listener_fd);

  unlink(socket_path.c_str());
}

int Worker::GetPulseTimeout() {
  using namespace std::chrono;
  return (pulse_interval -
          duration_cast<seconds>(steady_clock::now() - last_pulse_time)
              .count()) *
         1000;
}

void Worker::HandleRestartControlMessage(WorkerResponse &resp) {
  SetState(WorkerState::SHUTTING_DOWN);
  resp.set_error(WORKER_ERR_OK);
}

void Worker::HandleFetchControlMessage(WorkerResponse &resp) {
  if (fetch_data.size() == 0) {
    resp.set_error(WORKER_ERR_NO_FETCH);
    return;
  }

  extra_data = std::move(fetch_data);
  SetState(WorkerState::FREE);
}

void Worker::ProcessTask_Static(Worker *worker, const std::vector<char> &data) {
  worker->SetState(WorkerState::BUSY);
  worker->ProcessTask(data);
  worker->SetState(WorkerState::FREE);
}

void Worker::HandleSetTaskControlMessage(const ControlMsg &msg,
                                         WorkerResponse &resp,
                                         const std::vector<char> &extra) {
  if (GetState() != WorkerState::FREE) {
    resp.set_error(WORKER_ERR_BUSY);
    return;
  }

  current_task_id = msg.task_id();
  std::thread(ProcessTask_Static, this, extra).detach();
}

void Worker::HandleGetStatusControlMessage(WorkerResponse &resp) {}

void Worker::HandleControlMessage(int client_fd) {
  WorkerResponse response;
  response.set_task_id(current_task_id);
  response.set_error(WORKER_ERR_OK);

  try {
    ControlMsg msg;
    ReadProtoMessage(client_fd, msg);

    spdlog::info("Recieved {{{}}}", msg.ShortDebugString());

    std::vector<char> extra(msg.extra_size());
    ReadExact(client_fd, extra.data(), extra.size());

    switch (msg.type()) {
    case CTRL_RESTART:
      HandleRestartControlMessage(response);
      break;
    case CTRL_FETCH:
      HandleFetchControlMessage(response);
      break;
    case CTRL_SET_TASK:
      HandleSetTaskControlMessage(msg, response, extra);
      break;
    case CTRL_GET_STATUS:
      HandleGetStatusControlMessage(response);
      break;
    default:
      response.set_error(WORKER_ERR_FAILED);
      break;
    }
  } catch (const std::exception &e) {
    response.set_error(WORKER_ERR_FAILED);
  }

  response.set_extra_size(extra_data.size());
  WriteProtoMessage(client_fd, response);

  if (extra_data.size() != 0) {
    WriteExact(client_fd, extra_data.data(), extra_data.size());
    extra_data.clear();
  }
  spdlog::info("Sent {{{}}}", response.ShortDebugString());
}

void Worker::MainLoop() {
  while (GetState() != WorkerState::SHUTTING_DOWN) {
    pollfd fds[1] = {{listener_fd, POLLIN, 0}};
    int timeout = GetPulseTimeout();

    int res = poll(fds, 1, timeout > 0 ? timeout : 0);
    if (res < 0) {
      SetState(WorkerState::ERROR);
      throw WorkerException("poll failed");
    }

    if (res == 0) {
      SendPulse(PULSE_OK);
      continue;
    }

    if (fds[0].revents & POLLIN) {
      int client_fd = accept(listener_fd, nullptr, nullptr);
      if (client_fd < 0) {
        SetState(WorkerState::ERROR);
        throw WorkerException("accept failed");
      }
      HandleControlMessage(client_fd);
      close(client_fd);
    }
  }
}
