#include "worker.hpp"
#include "md_calculator.hpp"
#include <iostream>
#include <vector>

class HashWorker : public Worker {
protected:
    void DoTask(int client_fd) {
        uint64_t task_size;
        ReadExact(client_fd, (void*)&task_size, sizeof(task_size));
        
        MDCalculator md_calculator("md5");

        while (true) {
            uint64_t frame_len;
            ReadExact(client_fd, (void*)&frame_len, sizeof(frame_len));
            
            if (frame_len == 0) break;

            std::vector<uint8_t> frame(frame_len);
            ReadExact(client_fd, frame.data(), frame_len);
            md_calculator.update(frame.data(), frame_len);
        }
        
        std::string hash = md_calculator.finalize();

        uint64_t hash_size = hash.size();
        WriteExact(client_fd, (void*)&hash_size, sizeof(uint64_t));
        WriteExact(client_fd, (void*)hash.data(), hash_size);
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
}
