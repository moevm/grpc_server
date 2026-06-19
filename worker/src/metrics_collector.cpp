#include "../include/metrics_collector.hpp"

#include <chrono>
#include <fstream>
#include <spdlog/spdlog.h>
#include <thread>
#include <unistd.h>

namespace {
double GetMemoryUsed() {
  std::ifstream file("/proc/self/statm");
  if (!file.is_open())
    return 0;

  long total_pages = 0;
  long rss_pages = 0;
  file >> total_pages >> rss_pages;

  return rss_pages * (double)getpagesize();
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

  auto &tasks_completed_family = prometheus::BuildCounter()
                                     .Name("tasks_completed_total")
                                     .Help("Total number of completed tasks")
                                     .Register(*registry);
  tasks_completed_counter = &tasks_completed_family.Add({});

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
  if (!file.is_open())
    return;

  std::string line;
  while (std::getline(file, line)) {
    if (line.find("cpu") != 0)
      break;

    std::istringstream iss(line);
    std::string cpu_name;
    long user, nice, sys, idle, iowait, irq, softirq, steal, guest, guest_nice;

    iss >> cpu_name >> user >> nice >> sys >> idle >> iowait >> irq >>
        softirq >> steal >> guest >> guest_nice;

    if (cpu_name.empty())
      continue;

    uint64_t non_idle = user + nice + sys + irq + softirq + steal;
    uint64_t total = non_idle + idle + iowait;

    auto it = cpu_usage.find(cpu_name);
    if (it != cpu_usage.end()) {
      CPUInfo &cpu = it->second;

      if (cpu.last_total > 0) {
        uint64_t total_diff = total - cpu.last_total;
        uint64_t non_idle_diff = non_idle - cpu.last_non_idle;

        double percent = (total_diff == 0)
                             ? 0.0
                             : (double)non_idle_diff / total_diff * 100.0;
        cpu.gauge->Set(percent);
      }

      cpu.last_total = total;
      cpu.last_non_idle = non_idle;
    }
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

void MetricsCollector::IncrementPacketsDropped(int count) {
  if (packets_dropped_counter) {
    packets_dropped_counter->Increment(count);
  }
}