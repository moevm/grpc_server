#ifndef WORKER_HPP
#define WORKER_HPP

#include <string>
#include <cstdint>

#define SOCKET_DIR "/run/controller/"
#define INIT_SOCKET_NAME "init.sock"

class WorkerException {
    std::string msg;
public:
    const std::string &what() { return msg; }
    WorkerException(const char *msg) : msg(msg) {}
};

class Worker {
    std::string socket_path;
    int listener_fd;

    enum class InitResponse : uint64_t {
        OK = 1
    };

protected:
    static ssize_t ReadExact(int fd, void *buf, size_t n);
    static ssize_t WriteExact(int fd, const void *buf, size_t n);
    static int UnixRead(int fd, void *buf, size_t n);
    static int UnixWrite(int fd, const void *buf, size_t n);

public:
    Worker();
    ~Worker();

    virtual void DoTask(int client_fd) = 0;
    void MainLoop();
};

#endif
