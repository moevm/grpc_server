#include "../include/worker.hpp"
#include "../include/md_calculator.hpp"
#include <iostream>
#include <vector>

class HashWorker : public Worker {
protected:
    void DoTask(int client_fd) {
        uint64_t task_size;
        UnixRead(client_fd, &task_size, sizeof(task_size));
        MDCalculator md_calculator("md5");

        std::vector<uint8_t> task(task_size);
        UnixRead(client_fd, task.data(), task_size);

        md_calculator.update(task.data(), task_size);        
        std::string hash = md_calculator.finalize();

        uint64_t hash_size = hash.size();
        UnixWrite(client_fd, &hash_size, sizeof(hash_size));
        UnixWrite(client_fd, hash.data(), hash_size);
    }
};

int main()
{
    try {
        HashWorker().MainLoop();
    }
    catch(WorkerException &e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
    catch(std::exception &e) {
        std::cerr << "[ERROR] Unhandled exception " << typeid(e).name() << ": " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
