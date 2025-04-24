#include "worker.hpp"
#include "md_calculator.hpp"
#include <iostream>
#include <vector>

class HashWorker : public Worker {
protected:
    void DoTask(asio::local::stream_protocol::socket& io) {
        uint64_t task_size;
        asio::read(io, asio::buffer(&task_size, sizeof(task_size)));
        
        MDCalculator md_calculator("md5");

        while (true) {
            uint64_t frame_len;
            asio::read(io, asio::buffer(&frame_len, sizeof(frame_len)));
            
            if (frame_len == 0) {
                break;
            }

            std::vector<uint8_t> frame(frame_len);
            asio::read(io, asio::buffer(frame.data(), frame_len));
            md_calculator.update(frame.data(), frame_len);
        }
        
        std::string hash = md_calculator.finalize();

        uint64_t hash_size = hash.size();
        asio::write(io, asio::buffer(&hash_size, sizeof(hash_size)));
        asio::write(io, asio::buffer(hash.data(), hash_size));
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
