#ifndef METRICS_COLLECTOR_HPP
#define METRICS_COLLECTOR_HPP

#include <atomic>
#include <chrono>
#include <prometheus/counter.h>
#include <prometheus/gateway.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

class MetricsCollector {
public:
  MetricsCollector(const char *gateway_address, const char *gateway_port,
                   const char *worker_name);
  ~MetricsCollector();

  void IncrementPacketsReceived(uint64_t count = 1);
  void IncrementPacketsPassed(uint64_t count = 1);
  void IncrementPacketsDropped(uint64_t count = 1);

private:
  struct CPUInfo {
    prometheus::Gauge *gauge;

    struct Time {
      uint64_t user;
      uint64_t user_low;
      uint64_t sys;
      uint64_t idle;
    };

    Time time;

    uint64_t last_total{0};
    uint64_t last_non_idle{0};
  };

  void GetCPUUsage();
  void MainLoop();
  void PushMetrics();

  prometheus::Gateway gateway;
  std::shared_ptr<prometheus::Registry> registry;

  std::unordered_map<std::string, CPUInfo> cpu_usage;
  prometheus::Gauge *memory_used_gauge;

  prometheus::Counter *packets_received_counter;
  prometheus::Counter *packets_passed_counter;
  prometheus::Counter *packets_dropped_counter;

  prometheus::Counter *push_errors_total;

  std::atomic<bool> is_running{true};
  std::thread thread;
};

#endif