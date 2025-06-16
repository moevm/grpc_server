#include "../include/md_calculator.hpp"
#include "../include/metrics_collector.hpp"
#include "../include/worker.hpp"

#include <spdlog/spdlog.h>

class HashWorker : public Worker {
  MetricsCollector metrics_collector;

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
  HashWorker(const char *gateway_address, const char *gateway_port)
      : metrics_collector(
          gateway_address,
          gateway_port,
          ("worker-" + std::to_string(GetID())).c_str()
        )
  {}
};

int main() {
  const char *gateway_address = getenv("METRICS_GATEWAY_ADDRESS");
  const char *gateway_port = getenv("METRICS_GATEWAY_PORT");

  if (gateway_address == nullptr || gateway_port == nullptr) {
    spdlog::error("Environment variables are not fully specified. "
                  "Specify METRICS_GATEWAY_ADDRESS and METRICS_GATEWAY_PORT");
    return 1;
  }

  spdlog::info("Initialize MetricsCollector with {}:{}", gateway_address, gateway_port);

  try {
    HashWorker(gateway_address, gateway_port).MainLoop();
  } catch (WorkerException &e) {
    spdlog::error(e.what());
    return 1;
  } catch (std::exception &e) {
    spdlog::error("unhandled exception {}: {}", typeid(e).name(), e.what());
    return 1;
  }

  return 0;
}
