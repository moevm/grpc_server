#include "../include/worker.hpp"

#include <arpa/inet.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <endian.h>

void Worker::LogStateChange(WorkerState new_state) {
  const char *state_names[] = {"BOOTING",    "CONNECTING",    "READY",
                               "PROCESSING", "SHUTTING_DOWN", "ERROR"};
  std::cerr << "[STATE] " << state_names[static_cast<int>(state)] << " -> "
            << state_names[static_cast<int>(new_state)] << std::endl;
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
    r = read(fd, (char*)buf + total, n - total);
    if (r < 0) {
      if (errno == EINTR) {
        continue;
      }
      throw WorkerException(std::string("ReadExact failed with error ") + strerror(errno));
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
    w = write(fd, (const char*)buf + total, n - total);
    if (w < 0) {
      if (errno == EINTR) {
        continue;
      }
      throw WorkerException(std::string("WriteExact failed with error ") + strerror(errno));
    }
    total += w;
  }
}

void Worker::ReadMessage(int fd, std::string &msg) {
  uint64_t length_le64 = 0;
  Worker::ReadExact(fd, length_le64);
  uint64_t length = le64toh(length_le64);
  msg.resize(length);
  Worker::ReadExact(fd, (void*)msg.data(), length);
}

void Worker::WriteMessage(int fd, const std::string &msg) {
  uint64_t length_le64 = htole64(msg.size());
  Worker::WriteExact(fd, length_le64);
  Worker::WriteExact(fd, msg.data(), msg.size());
}

void Worker::SendPulse(PulseType type) {
  int main_fd = 0;
  try {
    main_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (main_fd < 0) throw WorkerException("socket() failed");

    sockaddr_un addr{.sun_family = AF_UNIX};
    strncpy(addr.sun_path, SOCKET_DIR MAIN_SOCKET_NAME, sizeof(addr.sun_path) - 1);

    if (connect(main_fd, (sockaddr*)&addr, sizeof(addr))) {
      throw WorkerException("connect() failed");
    }

    WorkerPulse pulse;
    pulse.set_type(type);
    pulse.set_worker_id(worker_id);
    pulse.set_task_id(current_task_id);
    pulse.set_next_pulse(EXPECTED_PULSE_TIME);
    WriteProtoMessage(main_fd, pulse);

    pulse_interval = MIN_PULSE_TIME + (rand() % (MAX_PULSE_TIME - MIN_PULSE_TIME + 1));
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

    std::cerr << "[INFO]  " << PulseType_Name(type) << ". Got " << ControllerError_Name(response.error()) << ' ' << response.worker_id() << '\n';
  }
  catch (const std::exception& e) {
    close(main_fd);
    SetState(WorkerState::ERROR);
    throw WorkerException(std::string("Pulse failed: ") + e.what());
  }
}

Worker::Worker() : listener_fd(-1), state(WorkerState::BOOTING) {
  SetState(WorkerState::CONNECTING);
  SendPulse(PULSE_REGISTER);

  socket_path = std::string(SOCKET_DIR) + std::to_string(worker_id) + ".sock";
  unlink(socket_path.c_str());

  std::cerr << "[INFO] worker_id = " << worker_id << '\n';
  std::cerr << "[INFO] socket_path = " << socket_path << '\n';
  srand(worker_id); // for random pulse time generation
  
  listener_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listener_fd < 0) {
    throw WorkerException("socket() failed");
  }
  
  sockaddr_un addr{.sun_family = AF_UNIX};
  strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);
  
  if (bind(listener_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    close(listener_fd);
    listener_fd = -1;
    throw WorkerException("bind() failed");
  }
  
  if (listen(listener_fd, 5) < 0) {
    close(listener_fd);
    listener_fd = -1;
    throw WorkerException("listen() failed");
  }
  
  SetState(WorkerState::READY);
}

Worker::~Worker() {
  SetState(WorkerState::SHUTTING_DOWN);
  
  try {
    SendPulse(PULSE_SHUTDOWN);
  }
  catch (...) {} // ignore errors cuz we don't care

  if (listener_fd != -1)
    close(listener_fd);
  
  unlink(socket_path.c_str());
}

int Worker::GetPulseTimeout() {
  using namespace std::chrono;
  return (pulse_interval - duration_cast<seconds>(steady_clock::now() - last_pulse_time).count()) * 1000;
}

void Worker::HandleControlMessage(int client_fd) {
  try {
    ControlMsg msg;
    ReadProtoMessage(client_fd, msg);

    std::vector<char> extra(msg.extra_size());
    ReadExact(client_fd, extra.data(), extra.size());
    
    WorkerResponse response;
    response.set_task_id(current_task_id);

    switch (msg.type()) {
      case CTRL_RESTART:
        SetState(WorkerState::SHUTTING_DOWN);
        response.set_error(WORKER_ERR_OK);
        break;

      case CTRL_FETCH: {
        if (fetch_data.size() == 0) {
          response.set_error(WORKER_ERR_NO_FETCH);
          break;
        }
        
        response.set_error(WORKER_ERR_OK);
        response.set_extra_size(fetch_data.size());
        
        WriteProtoMessage(client_fd, response);
        WriteExact(client_fd, fetch_data.data(), fetch_data.size());
        return;
      }

      case CTRL_SET_TASK: {
        if (GetState() == WorkerState::PROCESSING) {
          response.set_error(WORKER_ERR_BUSY);
          break;
        }
        
        current_task_id = msg.task_id();
        SetState(WorkerState::PROCESSING);
        ProcessTask(extra);
        response.set_error(WORKER_ERR_OK);
        break;
      }

      case CTRL_GET_STATUS:
        response.set_error(WORKER_ERR_OK);
        break;

      default:
        response.set_error(WORKER_ERR_FAILED);
        break;
    }

    response.set_extra_size(0);
    WriteProtoMessage(client_fd, response);
  } catch (const std::exception& e) {
    WorkerResponse resp;
    resp.set_error(WORKER_ERR_FAILED);
    resp.set_extra_size(0);
    WriteProtoMessage(client_fd, resp);
  }
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
