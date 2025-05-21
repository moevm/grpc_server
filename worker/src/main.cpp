#include "../include/md_calculator.hpp"
#include "../include/metrics_collector.hpp"
#include "../include/worker.hpp"
#include <iostream>
#include <vector>

class HashWorker : public Worker {
  MetricsCollector &metrics_collector;

protected:
  void ProcessTask(const std::vector<char> &data) {
    metrics_collector.StartTask();

    MDCalculator md_calculator("md5");
    md_calculator.update((const unsigned char *)data.data(), data.size());
    std::string hash = md_calculator.finalize();

    SetFetchData(hash);
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
