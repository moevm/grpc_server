#include "../include/md_calculator.hpp"
#include "../include/metrics_collector.hpp"
#include "../include/worker.hpp"
#include <iostream>
#include <vector>

class HashWorker : public Worker {
  MetricsCollector &metrics_collector;

protected:
  void DoTask(int client_fd) {
    metrics_collector.StartTask();

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

    metrics_collector.StopTask();
  }

public:
  HashWorker(MetricsCollector &metrics_collector)
      : metrics_collector(metrics_collector) {}
};

int main() {
  const char *gateway_address = getenv("METRICS_GATEWAY_ADDRESS");
  const char *gateway_port = getenv("METRICS_GATEWAY_PORT");
  const char *worker_name = getenv("METRICS_WORKER_NAME");

  if (gateway_address == nullptr || gateway_port == nullptr ||
      worker_name == nullptr) {
    std::cerr << "[ERROR] Environment variables are not fully specified\n"
                 "[INFO] Specify METRICS_GATEWAY_ADDRESS METRICS_GATEWAY_PORT "
                 "METRICS_WORKER_NAME\n";
    return 1;
  }

  try {
    MetricsCollector metrics_collector(gateway_address, gateway_port,
                                       worker_name);
    HashWorker(metrics_collector).MainLoop();
  } catch (WorkerException &e) {
    std::cerr << "[ERROR] " << e.what() << std::endl;
    return 1;
  } catch (std::exception &e) {
    std::cerr << "[ERROR] Unhandled exception " << typeid(e).name() << ": "
              << e.what() << std::endl;
    return 1;
  }

  return 0;
}
