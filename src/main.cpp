#include "metrics-collector.hpp"
#include "file.hpp"

#include <iostream>
#include <cstring>
#include <thread>
#include <signal.h>

void print_usage(const std::string& program_name) {
    std::cerr << "Usage: " << program_name << " hash" 
              << " PATH ALGORITHM" << std::endl
              << "Supported algorithms are: "
              << "md2, md5, sha, " 
              << "sha1, sha224, sha256, sha384, sha512, "
              << "mdc2 and ripemd160" << std::endl;
}

int hash_main(int argc, char* argv[]) {
    if (argc < 4) {
        print_usage(argv[0]);
        return 1;
    }

    try {
        std::string hash = File::calculate_hash(argv[2], argv[3]);
        std::cout << "Digest is: " << hash << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}


bool is_running = true;

void signal_handler(int signo) {
    std::cerr << "[INFO] Received " << strsignal(signo) << " signal. Stopping" << std::endl;
    is_running = false;
}

int metrics_main(int argc, char *argv[]) {
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    if (argc != 5) {
        std::cerr << "usage: worker" << argv[0] << " metrics" 
                  << " ADDRESS PORT WORKER_NAME" << std::endl;
        return 1;
    }
    
    MetricsCollector metrics_collector(argv[2], argv[3], argv[4]);
   
    srand(time(NULL));
    while (is_running) {
        std::this_thread::sleep_for(std::chrono::seconds(rand() % 5 + 1));
        std::cerr << "[INFO] Doing task" << std::endl;
        metrics_collector.StartTask();
    
        // some "work" that requires memory
        void *data = malloc(rand() % (1024 * 1024 * 50)); // 0 - 50 megabytes
        for (volatile int i = 0; i < 1000000000 + rand() % 10000000000; i++) {}
        free(data);
    
        metrics_collector.StopTask();
        std::cerr << "[INFO] Task done" << std::endl;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    static const char usage[] = "Specify hash/metrics";

    if (argc < 2) {
        std::cerr << usage << std::endl;
        return 1; 
    }

    if (strcmp(argv[1], "hash") == 0) {
        return hash_main(argc, argv);
    } else if (strcmp(argv[1], "metrics") == 0) {
        return metrics_main(argc, argv);
    }

    std::cerr << usage << std::endl;
    return 1;
}
