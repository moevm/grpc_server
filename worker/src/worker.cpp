#include "worker.hpp"

#include <iostream>
#include <stdexcept>
#include <asio.hpp>

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

Worker::Worker()
    : acceptor(io_context)
{
    int64_t id = GenerateRandomID();
    socket_path = SOCKET_DIR + std::to_string(id) + ".sock";

    unlink(socket_path.c_str());
    std::cerr << "[INFO] id = " << id << std::endl;

    asio::local::stream_protocol::endpoint endpoint(socket_path);

    try {
        acceptor.open(endpoint.protocol());
        acceptor.bind(endpoint);
        acceptor.listen();
    } catch (const asio::system_error &e) {
        throw WorkerException("failed to set up socket");
    }
    std::cerr << "[INFO] socket set up" << std::endl;

    asio::local::stream_protocol::socket init_socket(io_context);
    asio::local::stream_protocol::endpoint init_endpoint(INIT_SOCKET_PATH);

    try {
        init_socket.connect(init_endpoint);
    } catch (const asio::system_error &e) {
        throw WorkerException("connection to init socket failed");
    }
    std::cerr << "[INFO] Successfully connected to init socket" << std::endl;

    try {
        asio::write(init_socket, asio::buffer(&id, sizeof(id)));
    } catch (const asio::system_error &e) {
        throw WorkerException("failed to send ID to init socket");
    }

    uint64_t resp;
    try {
        asio::read(init_socket, asio::buffer(&resp, sizeof(resp)));
    } catch (const asio::system_error &e) {
        throw WorkerException("failed to read response from init socket");
    }

    if (resp != 1) {
        throw WorkerException("worker registration failed");
    }
}

Worker::~Worker()
{
    unlink(socket_path.c_str());
}

void Worker::MainLoop()
{
    while (true) {
        asio::local::stream_protocol::socket client_socket(io_context);
        try {
            acceptor.accept(client_socket);
        } catch (const asio::system_error& e) {
            throw WorkerException("accept failed");
        }

        std::cerr << "[INFO] Worker started performing task..." << std::endl;
        DoTask(client_socket);
        std::cerr << "[INFO] Task completed" << std::endl;
    }
}
