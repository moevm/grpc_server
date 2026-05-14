#include "../include/metrics_collector.hpp"

#include <chrono>
#include <fstream>
#include <spdlog/spdlog.h>
#include <thread>
#include <unistd.h>

namespace {
double GetMemoryUsed() {
  std::ifstream file("/proc/self/statm");
  if (!file.is_open()) {
    return 0;
  }

  long mem_pages = 0;
  file >> mem_pages;
  file.close();
  return mem_pages * (double)getpagesize();
}
} // namespace

MetricsCollector::MetricsCollector(const char *gateway_address,
                                   const char *gateway_port,
                                   const char *worker_name)
    : gateway(gateway_address, gateway_port, worker_name),
      registry(std::make_shared<prometheus::Registry>()) {

  auto &cpu_usage_family = prometheus::BuildGauge()
                               .Name("cpu_usage")
                               .Help("CPU Usage in percents")
                               .Register(*registry);

  auto &memory_used_family = prometheus::BuildGauge()
                                 .Name("memory_used")
                                 .Help("Memory used by worker in bytes")
                                 .Register(*registry);
  memory_used_gauge = &memory_used_family.Add({});

  auto &task_processing_time_family =
      prometheus::BuildGauge()
          .Name("task_processing_time")
          .Help("Task processing time (in seconds)")
          .Register(*registry);
  task_processing_time_gauge = &task_processing_time_family.Add({});

  auto &packets_received_family = prometheus::BuildCounter()
                                      .Name("packets_received_total")
                                      .Help("Total number of packets received")
                                      .Register(*registry);
  packets_received_counter = &packets_received_family.Add({});

  auto &packets_passed_family =
      prometheus::BuildCounter()
          .Name("packets_passed_total")
          .Help("Total number of packets passed/forwarded")
          .Register(*registry);
  packets_passed_counter = &packets_passed_family.Add({});

  auto &packets_dropped_family = prometheus::BuildCounter()
                                     .Name("packets_dropped_total")
                                     .Help("Total number of packets dropped")
                                     .Register(*registry);
  packets_dropped_counter = &packets_dropped_family.Add({});

  packets_dropped_by_reason_family =
      &prometheus::BuildCounter()
           .Name("packets_dropped_by_reason_total")
           .Help("Packets dropped by reason")
           .Register(*registry);

  blocked_domains_family = &prometheus::BuildCounter()
                                .Name("blocked_domains_total")
                                .Help("Number of blocked requests by domain/IP")
                                .Register(*registry);

  auto &tasks_completed_family = prometheus::BuildCounter()
                                     .Name("tasks_completed_total")
                                     .Help("Total number of completed tasks")
                                     .Register(*registry);
  tasks_completed_counter = &tasks_completed_family.Add({});

  auto &task_duration_family = prometheus::BuildHistogram()
                                   .Name("task_duration_seconds")
                                   .Help("Task execution duration in seconds")
                                   .Register(*registry);

  prometheus::Histogram::BucketBoundaries task_buckets = {
      0.001, 0.005, 0.01, 0.025, 0.05, 0.1,  0.25,
      0.5,   1.0,   2.5,  5.0,   10.0, 30.0, 60.0};
  task_duration_histogram =
      &task_duration_family.Add({}, std::move(task_buckets));

  auto &push_errors_family = prometheus::BuildCounter()
                                 .Name("push_errors_total")
                                 .Help("Total number of push gateway errors")
                                 .Register(*registry);
  push_errors_total = &push_errors_family.Add({});

  std::ifstream file("/proc/stat");
  int ign;
  std::string cpu_name;

  while (true) {
    CPUInfo cpu;

    file >> cpu_name >> cpu.time.user >> cpu.time.user_low >> cpu.time.sys >>
        cpu.time.idle >> ign >> ign >> ign >> ign >> ign >> ign;

    if (cpu_name.find("cpu") != 0)
      break;

    cpu.gauge = &cpu_usage_family.Add({{"cpu", std::string(cpu_name)}});
    cpu_usage.insert({std::string(cpu_name), cpu});
  }

  file.close();

  gateway.RegisterCollectable(registry);
  thread = std::thread(&MetricsCollector::MainLoop, this);
  is_task_running = false;
}

void MetricsCollector::MainLoop() {
  while (is_running) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    memory_used_gauge->Set(::GetMemoryUsed());
    GetCPUUsage();

    if (is_task_running) {
      auto cur_time = std::chrono::high_resolution_clock::now();
      task_processing_time_gauge->Set(
          std::chrono::duration<double>(cur_time - task_start).count());
    } else {
      task_processing_time_gauge->Set(0);
    }

    PushMetrics();
  }
}

void MetricsCollector::PushMetrics() {
  int status = gateway.PushAdd();
  if (status != 200) {
    spdlog::warn("Failed to push metrics. Status {}", status);
    if (push_errors_total) {
      push_errors_total->Increment();
    }
  }
}

MetricsCollector::~MetricsCollector() {
  is_running = false;
  if (thread.joinable()) {
    thread.join();
  }
}

void MetricsCollector::GetCPUUsage() {
  std::ifstream file("/proc/stat");
  CPUInfo::Time cur_time;
  double percent;

  std::string cpu_name;
  int ign;

  while (true) {
    file >> cpu_name >> cur_time.user >> cur_time.user_low >> cur_time.sys >>
        cur_time.idle >> ign >> ign >> ign >> ign >> ign >> ign;

    if (cpu_name.find("cpu") != 0)
      break;

    CPUInfo &cpu = cpu_usage[cpu_name];
    if (cur_time.user < cpu.time.user ||
        cur_time.user_low < cpu.time.user_low || cur_time.sys < cpu.time.sys ||
        cur_time.idle < cpu.time.idle) {
      // overflow detection
      percent = -1.0;
    } else {
      uint64_t total = (cur_time.user - cpu.time.user) +
                       (cur_time.user_low - cpu.time.user_low) +
                       (cur_time.sys - cpu.time.sys);

      percent = total;
      total += (cur_time.idle - cpu.time.idle);
      percent = (total == 0) ? -1.0 : (percent / total) * 100.0;
    }

    cpu.time = cur_time;
    cpu.gauge->Set(percent);
  }

  file.close();
}

void MetricsCollector::IncrementPacketsReceived(int count) {
  if (packets_received_counter) {
    packets_received_counter->Increment(count);
  }
}

void MetricsCollector::IncrementPacketsPassed(int count) {
  if (packets_passed_counter) {
    packets_passed_counter->Increment(count);
  }
}

void MetricsCollector::IncrementPacketsDropped(const std::string &reason,
                                               int count) {
  if (packets_dropped_counter) {
    packets_dropped_counter->Increment(count);
  }
  if (packets_dropped_by_reason_family) {
    packets_dropped_by_reason_family->Add({{"reason", reason}})
        .Increment(count);
  }
}

void MetricsCollector::IncrementBlockedDomain(const std::string &domain_or_ip) {
  if (blocked_domains_family) {
    blocked_domains_family->Add({{"domain", domain_or_ip}}).Increment();
  }
}

void MetricsCollector::StartTask() {
  is_task_running = true;
  task_start = std::chrono::high_resolution_clock::now();
  task_processing_time_gauge->Set(0);
  PushMetrics();
}

void MetricsCollector::StopTask() {
  auto duration = std::chrono::duration<double>(
                      std::chrono::high_resolution_clock::now() - task_start)
                      .count();

  if (task_duration_histogram) {
    task_duration_histogram->Observe(duration);
  }

  if (tasks_completed_counter) {
    tasks_completed_counter->Increment();
  }

  is_task_running = false;
  task_processing_time_gauge->Set(0);
  PushMetrics();
}

void MetricsCollector::ObserveCollectionDuration(double seconds) {
  // Метод оставлен для совместимости
  (void)seconds;
}