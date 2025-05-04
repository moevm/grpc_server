#include <chrono>
#include <prometheus/gateway.h>
#include <prometheus/gauge.h>
#include <prometheus/registry.h>

class MetricsCollector {
  prometheus::Gateway gateway;
  std::shared_ptr<prometheus::Registry> registry;

  struct CPUInfo {
    prometheus::Gauge *gauge;

    struct Time {
      uint64_t user;
      uint64_t user_low;
      uint64_t sys;
      uint64_t idle;
    };

    Time time;
  };

  std::unordered_map<std::string, CPUInfo> cpu_usage;

  prometheus::Gauge *memory_used_gauge;
  prometheus::Gauge *task_processing_time_gauge;

  std::atomic<bool> is_running{true};
  std::thread thread;

  std::atomic<bool> is_task_running;
  std::chrono::time_point<std::chrono::high_resolution_clock> task_start;

  void GetCPUUsage();
  void MainLoop();

public:
  MetricsCollector(const char *gateway_address, const char *gateway_port,
                   const char *worker_name);
  ~MetricsCollector();

  void StartTask();
  void StopTask();
};
