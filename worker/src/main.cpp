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
  HashWorker(const char *gateway_address, const char *gateway_port, uint64_t id)
      : Worker(id),
        metrics_collector(gateway_address, gateway_port,
                          ("worker-" + std::to_string(id)).c_str()) {}
};

int main() {
  const char *worker_id_str = getenv("WORKER_ID");
  if (worker_id_str == nullptr) {
    spdlog::error("WORKER_ID environment variable not set");
    return 1;
  }

  uint64_t worker_id = std::stoull(worker_id_str);
  const char *gateway_address = getenv("METRICS_GATEWAY_ADDRESS");
  const char *gateway_port = getenv("METRICS_GATEWAY_PORT");

  if (gateway_address == nullptr || gateway_port == nullptr) {
    spdlog::error("Environment variables are not fully specified. "
                  "Specify METRICS_GATEWAY_ADDRESS and METRICS_GATEWAY_PORT");
    return 1;
  }

  spdlog::info("Initialize MetricsCollector with {}:{}", gateway_address,
               gateway_port);

  try {
    HashWorker worker(gateway_address, gateway_port, worker_id);

    bool test_mode = false;
    if (getenv("TEST_REQUEST_POLICY") != nullptr) {
      test_mode = true;
      spdlog::info("Test mode: requesting policy");
      std::this_thread::sleep_for(std::chrono::seconds(2));
      worker.requestPolicyFromController();
    }

    if (getenv("TEST_STATS") != nullptr) {
      test_mode = true;
      spdlog::info("Test mode: send stats");
      std::this_thread::sleep_for(std::chrono::seconds(2));
      worker.statsReport();
    }

    if (const char *domain = getenv("TEST_CLASSIFY_DOMAIN")) {
      test_mode = true;
      spdlog::info("Test mode: classifying domain '{}'", domain);
      std::this_thread::sleep_for(std::chrono::seconds(1));
      worker.classifyDomain(domain);
    }

    if (test_mode) {
      spdlog::info("Test mode completed, exiting");
      return 0;
    }

    worker.MainLoop();

  } catch (WorkerException &e) {
    spdlog::error(e.what());
    return 1;
  } catch (std::exception &e) {
    spdlog::error("unhandled exception {}: {}", typeid(e).name(), e.what());
    return 1;
  }

  return 0;
}
