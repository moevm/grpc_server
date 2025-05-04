#include "../include/worker.hpp"

#include <arpa/inet.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

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

ssize_t Worker::ReadExact(int fd, void *buf, size_t n) {
  size_t total = 0;
  ssize_t r;
  while (total < n) {
    r = read(fd, (char *)buf + total, n - total);
    if (r <= 0) {
      return r;
    }
    total += r;
  }
  return total;
}

ssize_t Worker::WriteExact(int fd, const void *buf, size_t n) {
  size_t total = 0;
  ssize_t w;
  while (total < n) {
    w = write(fd, (const char *)buf + total, n - total);
    if (w <= 0) {
      return w;
    }
    total += w;
  }
  return total;
}

int Worker::UnixRead(int fd, void *buf, size_t buf_size) {
  uint64_t message_len;
  ssize_t r;

  r = ReadExact(fd, &message_len, sizeof(message_len));
  if (r != sizeof(message_len)) {
    return -1;
  }

  if (message_len > buf_size) {
    return -1;
  }

  size_t offset = 0;
  while (true) {
    uint64_t frame_len;
    r = ReadExact(fd, &frame_len, sizeof(frame_len));
    if (r != sizeof(frame_len)) {
      return -1;
    }

    if (frame_len == 0) {
      break;
    } else if (frame_len < 0) {
      return -1;
    }

    if (offset + frame_len > message_len) {
      return -1;
    }

    r = ReadExact(fd, (char *)buf + offset, frame_len);
    if (r != frame_len) {
      return -1;
    }
    offset += frame_len;
  }

  if (offset != message_len) {
    return -1;
  }

  return (int)message_len;
}

int Worker::UnixWrite(int fd, const void *buf, size_t len) {
  const size_t full_frame_len = 1024;
  size_t offset = 0;
  ssize_t w;

  w = WriteExact(fd, &len, sizeof(len));
  if (w != sizeof(len)) {
    return -1;
  }

  while (1) {
    uint64_t frame_len;
    uint64_t remaining = len - offset;

    if (remaining >= full_frame_len) {
      frame_len = full_frame_len;
    } else if (remaining > 0) {
      frame_len = remaining;
    } else {
      frame_len = 0;
    }

    w = WriteExact(fd, &frame_len, sizeof(frame_len));
    if (w != sizeof(frame_len)) {
      return -1;
    }

    if (frame_len > 0) {
      w = WriteExact(fd, (const char *)buf + offset, frame_len);
      if (w != frame_len) {
        return -1;
      }
      offset += frame_len;
    } else {
      break;
    }
  }

  return (int)offset;
}

Worker::Worker() : state(WorkerState::BOOTING) {
  SetState(WorkerState::CONNECTING);
  try {
    int init_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (init_fd < 0) {
      throw WorkerException("Failed to create init socket");
    }

    struct sockaddr_un init_addr = {.sun_family = AF_UNIX,
                                    .sun_path = SOCKET_DIR INIT_SOCKET_NAME};

    if (connect(init_fd, (sockaddr *)&init_addr, sizeof(init_addr)) != 0) {
      close(init_fd);
      throw WorkerException("Failed to connect to init socket");
    }

    uint64_t id = UINT64_MAX;
    if (UnixRead(init_fd, &id, sizeof(id)) == -1) {
      close(init_fd);
      throw WorkerException("Failed to receive worker ID");
    }

    std::cerr << "[INFO] id = " << id << std::endl;
    socket_path = std::string(SOCKET_DIR) + std::to_string(id) + ".sock";
    unlink(socket_path.c_str());

    listener_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listener_fd < 0) {
      close(init_fd);
      throw WorkerException("Failed to create communication socket");
    }

    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
    };
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(listener_fd, (sockaddr *)&addr, sizeof(addr)) != 0) {
      close(init_fd);
      close(listener_fd);
      throw WorkerException("Failed to bind communication socket");
    }

    if (listen(listener_fd, SOMAXCONN) != 0) {
      close(init_fd);
      close(listener_fd);
      throw WorkerException("Failed to listen communication socket");
    }

    InitResponse response = InitResponse::OK;
    if (UnixWrite(init_fd, &response, sizeof(response)) == -1) {
      close(init_fd);
      close(listener_fd);
      throw WorkerException("Failed to write InitResponse");
    }

    std::cerr
        << "[INFO] Worker registered successfully. Ready to receive tasks."
        << std::endl;
    SetState(WorkerState::READY);
  } catch (...) {
    SetState(WorkerState::ERROR);
    throw;
  }
}

Worker::~Worker() {
  SetState(WorkerState::SHUTTING_DOWN);
  unlink(socket_path.c_str());
}

void Worker::MainLoop() {
  while (true) {
    SetState(WorkerState::READY);
    int client_fd = accept(listener_fd, NULL, NULL);
    if (client_fd < 0) {
      SetState(WorkerState::ERROR);
      throw WorkerException(
          "Failed to accept connection via communication socket");
    }

    SetState(WorkerState::PROCESSING);
    std::cerr << "[INFO] Worker started performing task..." << std::endl;

    try {
      DoTask(client_fd);
    } catch (std::exception &e) {
      SetState(WorkerState::ERROR);
      close(client_fd);
      throw e;
    }

    close(client_fd);
    std::cerr << "[INFO] Task completed" << std::endl;
  }
}
