#ifndef WORKER_HPP
#define WORKER_HPP

#include <string>
#include <asio.hpp>

class WorkerException {
    std::string msg;
public:
    const std::string &what() { return msg; }
    WorkerException(const char *msg) : msg(msg) {}
};

class Worker {
    static constexpr char SOCKET_DIR[] = "/run/controller/";
    static constexpr char INIT_SOCKET_PATH[] = "/run/controller/init.sock";

    std::string socket_path;

    asio::io_context io_context;
    asio::local::stream_protocol::acceptor acceptor;

protected:
    static void ReadExact(int fd, void *buf, size_t n);
    static void WriteExact(int fd, void *buf, size_t n);

public:
    Worker();
    ~Worker();

    virtual void DoTask(asio::local::stream_protocol::socket& io) = 0;
    void MainLoop();
};

#endif
