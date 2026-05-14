#ifndef METRICS_COLLECTOR_HPP
#define METRICS_COLLECTOR_HPP

#include <atomic>
#include <chrono>
#include <prometheus/counter.h>
#include <prometheus/gateway.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <string>
#include <thread>
#include <unordered_map>

class MetricsCollector {
public:
  MetricsCollector(const char *gateway_address, const char *gateway_port,
                   const char *worker_name);
  ~MetricsCollector();

  void IncrementPacketsReceived(int count = 1);
  void IncrementPacketsPassed(int count = 1);
  void IncrementPacketsDropped(const std::string &reason, int count = 1);
  void IncrementBlockedDomain(const std::string &domain_or_ip);

  void StartTask();
  void StopTask();
  void ObserveCollectionDuration(double seconds);

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
  };

  void GetCPUUsage();
  void MainLoop();
  void PushMetrics();

  prometheus::Gateway gateway;
  std::shared_ptr<prometheus::Registry> registry;

  std::unordered_map<std::string, CPUInfo> cpu_usage;
  prometheus::Gauge *memory_used_gauge;
  prometheus::Gauge *task_processing_time_gauge;

  prometheus::Counter *packets_received_counter;
  prometheus::Counter *packets_passed_counter;
  prometheus::Counter *packets_dropped_counter;
  prometheus::Family<prometheus::Counter> *packets_dropped_by_reason_family;
  prometheus::Family<prometheus::Counter> *blocked_domains_family;

  prometheus::Counter *tasks_completed_counter;
  prometheus::Histogram *task_duration_histogram;

  prometheus::Histogram *metrics_collection_duration;
  prometheus::Counter *push_errors_total;

  std::atomic<bool> is_running{true};
  std::thread thread;

  std::atomic<bool> is_task_running{false};
  std::chrono::time_point<std::chrono::high_resolution_clock> task_start;
};

#endif