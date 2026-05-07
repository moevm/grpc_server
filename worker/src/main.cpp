#include "metrics_collector.hpp"
#include "worker.hpp"

#include <spdlog/spdlog.h>

class FiltrWorker : public Worker {
  MetricsCollector metrics_collector;

protected:
  void ProcessTask(const std::vector<char> &data) {
    metrics_collector.StartTask();
    metrics_collector.StopTask();
  }

public:
  FiltrWorker(const char *gateway_address, const char *gateway_port,
              uint64_t id)
      : Worker(id),
        metrics_collector(gateway_address, gateway_port,
                          ("worker-" + std::to_string(id)).c_str()) {}
};

int main(int argc, char **argv) {
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

  spdlog::info("Starting worker ID: {}", worker_id);
  spdlog::info("Initialize MetricsCollector with {}:{}", gateway_address,
               gateway_port);

  try {
    Worker worker(worker_id);
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

    if (const char *target = getenv("TEST_CLASSIFY_TARGET")) {
      const char *type = getenv("TEST_CLASSIFY_TYPE");
      if (!type)
        type = "domain";

      test_mode = true;
      spdlog::info("Test mode: classifying {} '{}'", type, target);
      std::this_thread::sleep_for(std::chrono::seconds(1));
      struct requested_classification req_clas;
      memset(&req_clas, 0, sizeof(req_clas));

      bool success = worker.classify(type, target, &req_clas);
      if (success) {
        spdlog::info("Classification successful: trust_level={}",
                     req_clas.get_trust_level);
      } else {
        spdlog::error("Classification failed");
      }
    }

    if (test_mode) {
      spdlog::info("Test mode completed, exiting");
      return 0;
    }
    worker.initDPDK(argc, argv);
    worker.MainLoop();

  } catch (std::exception &e) {
    spdlog::error("unhandled exception {}: {}", typeid(e).name(), e.what());
    return 1;
  }

  return 0;
}
