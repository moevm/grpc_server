#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <openssl/md5.h>
#include <fcntl.h>

const char INIT_SOCKET_PATH[] = "/run/controller/init.sock";
const char SOCKET_DIR[] = "/run/controller/";

int listener_fd = -1;
std::string socket_path;

int read_exact(int fd, void* buf, size_t n)
{
    uint8_t* ptr = static_cast<uint8_t*>(buf);

    while (n > 0) {
        ssize_t cnt = read(fd, ptr, n);
        if (cnt <= 0) {
            std::cerr << "[ERROR] read_exact failed" << std::endl;
            return n;
        }

        ptr += cnt;
        n -= cnt;
    }

    return  n;
}

int write_exact(int fd, const void* buf, size_t n)
{
    const uint8_t* ptr = static_cast<const uint8_t*>(buf);
    
    while (n > 0) {
        ssize_t cnt = write(fd, ptr, n);
        if (cnt <= 0) {
            std::cerr << "[ERROR] write_exact failed" << std::endl;
            return n;
        }
        
        ptr += cnt;
        n -= cnt;
    }

    return n;
}

void DoTask(int client_fd)
{
    std::cout << "[INFO] Worker started performing task..." << std::endl;

    uint64_t task_size;
    read_exact(client_fd, (void*)&task_size, sizeof(task_size));
    
    fprintf(stderr, "[INFO] task_size=%llu\n", task_size);
    std::vector<uint8_t> task;
    task.reserve(task_size);

    while (true) {
        uint64_t frame_len;
        read_exact(client_fd, (void*)&frame_len, sizeof(frame_len));
        
        if (frame_len == 0) break;

        std::vector<uint8_t> frame(frame_len);
        read_exact(client_fd, frame.data(), frame_len);
        task.insert(task.end(), frame.begin(), frame.end());
    }
    
    uint8_t md5sum[MD5_DIGEST_LENGTH];
    MD5(task.data(), task.size(), md5sum);

    uint64_t md5_len = MD5_DIGEST_LENGTH;
    write_exact(client_fd, (void*)&md5_len, sizeof(uint64_t));
    write_exact(client_fd, (void*)md5sum, MD5_DIGEST_LENGTH);

    std::cout << "[INFO] Task completed" << std::endl;
}

void cleanup(int signum)
{
    std::cerr << "\n[INFO] Cleaning up..." << std::endl;
    
    if (listener_fd != -1) {
        close(listener_fd);
        unlink(socket_path.c_str());
    }
    
    exit(EXIT_SUCCESS);
}

int64_t generate_id()
{
    int64_t id;
    int urandom = open("/dev/urandom", O_RDONLY);
    if (urandom == -1 || read(urandom, (void*)&id, sizeof(uint64_t)) != sizeof(uint64_t)) {
        std::cerr << "[ERROR] failed to generate ID" << std::endl;
        exit(EXIT_FAILURE); // TODO: do something about it
    }
    close(urandom);
    return id;
}

void setup_cleanup()
{
    struct sigaction sa;
    sa.sa_handler = cleanup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

int main()
{
    setup_cleanup();
    fprintf(stderr, "[INFO] cleanup set up\n");

    int64_t id = generate_id();
    socket_path = SOCKET_DIR + std::to_string(id) + ".sock";
    unlink(socket_path.c_str());
    fprintf(stderr, "[INFO] id = %lli\n", id);

    listener_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listener_fd == -1) {
        fprintf(stderr, "[ERROR] failed to create a socket\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(listener_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1 || listen(listener_fd, SOMAXCONN) == -1) {
        fprintf(stderr, "[ERROR] failed to set up socket\n");
        close(listener_fd);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "[INFO] socket set up\n");

    int init_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (init_fd == -1) {
        fprintf(stderr, "[ERROR] failed to open init_socket\n");
        close(listener_fd);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "[INFO] init_socket opened\n");

    struct sockaddr_un init_addr = {};
    init_addr.sun_family = AF_UNIX;
    strncpy(init_addr.sun_path, INIT_SOCKET_PATH, sizeof(init_addr.sun_path) - 1);

    if (connect(init_fd, (struct sockaddr*)&init_addr, sizeof(init_addr)) == -1) {
        fprintf(stderr, "[ERROR] connection to init_socket failed\n"); 
        close(listener_fd);
        close(init_fd);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "[INFO] successfully connected to init_socket\n");

    write_exact(init_fd, (void*)&id, sizeof(uint64_t));

    uint64_t resp;
    read_exact(init_fd, (void*)&resp, sizeof(resp));
    close(init_fd);

    if (resp != 1) {
        fprintf(stderr, "[INFO] Worker registration failed\n");
        close(listener_fd);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "[INFO] Worker registered successfully\n");

    while (true) {
        int client_fd = accept(listener_fd, nullptr, nullptr);
        if (client_fd == -1) {
            fprintf(stderr, "[ERROR] Accept failed\n");
            break;
        }
        DoTask(client_fd);
        close(client_fd);
    }

    close(listener_fd);
    return EXIT_SUCCESS;
}
