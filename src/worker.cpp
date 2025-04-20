#include "worker.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

namespace {
    int64_t GenerateRandomID()
    {
        int64_t id;
        int urandom = open("/dev/urandom", O_RDONLY);
        if (urandom == -1 || read(urandom, (void*)&id, sizeof(uint64_t)) != sizeof(uint64_t)) {
            throw WorkerException("failed to generate ID");
        }
        close(urandom);
        return id;
    }
}

void Worker::ReadExact(int fd, void *buf, size_t n)
{
    uint8_t* ptr = static_cast<uint8_t*>(buf);

    while (n > 0) {
        ssize_t cnt = read(fd, ptr, n);
        if (cnt <= 0) {
            throw WorkerException("failed to read from socket");
        }

        ptr += cnt;
        n -= cnt;
    }
}

void Worker::WriteExact(int fd, void* buf, size_t n)
{
    const uint8_t* ptr = static_cast<const uint8_t*>(buf);
    
    while (n > 0) {
        ssize_t cnt = write(fd, ptr, n);
        if (cnt <= 0) {
            throw WorkerException("failed to write to socket");
        }
        
        ptr += cnt;
        n -= cnt;
    }
}

Worker::Worker()
{
    int64_t id = GenerateRandomID();
    socket_path = SOCKET_DIR + std::to_string(id) + ".sock";
    
    unlink(socket_path.c_str());
    std::cerr << "[INFO] id = " << id << std::endl;

    listener_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listener_fd == -1) {
        throw WorkerException("failed to create socket");
    }

    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(listener_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1 || listen(listener_fd, SOMAXCONN) == -1) {
        close(listener_fd);
        throw WorkerException("failed to set up socket");
    }
    std::cerr << "[INFO] socket set up" << std::endl;

    int init_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (init_fd == -1) {
        close(listener_fd);
        throw WorkerException("failed to open init_socket");
    }
    std::cerr << "[INFO] init_socket opened" << std::endl;

    struct sockaddr_un init_addr = {};
    init_addr.sun_family = AF_UNIX;
    strncpy(init_addr.sun_path, INIT_SOCKET_PATH, sizeof(init_addr.sun_path) - 1);

    if (connect(init_fd, (struct sockaddr*)&init_addr, sizeof(init_addr)) == -1) {
        close(listener_fd);
        close(init_fd);
        throw WorkerException("connection to init_socket failed");
    }
    std::cerr << "successfully connected to init_socket" << std::endl;

    WriteExact(init_fd, (void*)&id, sizeof(uint64_t));

    uint64_t resp;
    ReadExact(init_fd, (void*)&resp, sizeof(resp));
    close(init_fd);

    if (resp != 1) {
        close(listener_fd);
        throw WorkerException("worker registration failed");
    }
    std::cerr << "[INFO] worker registered successfully" << std::endl;
}

Worker::~Worker()
{
    close(listener_fd);
}

void Worker::MainLoop()
{
    while (true) {
        int client_fd = accept(listener_fd, nullptr, nullptr);
        if (client_fd == -1) {
            throw WorkerException("accept failed");
        }

        std::cerr << "[INFO] Worker started performing task..." << std::endl;
        DoTask(client_fd);
        std::cerr << "[INFO] Task completed" << std::endl;

        close(client_fd);
    }
}
