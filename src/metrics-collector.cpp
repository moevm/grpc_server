#include "metrics-collector.hpp"

#include <thread>
#include <chrono>
#include <unistd.h>
#include <fstream>

MetricsCollector::MetricsCollector(const char *gateway_address, const char *gateway_port, const char *worker_name)
    : gateway(gateway_address, gateway_port, worker_name),
      registry(std::make_shared<prometheus::Registry>())
{
    auto &cpu_usage_family = prometheus::BuildGauge()
        .Name("cpu_usage")
        .Help("CPU Usage in percents")
        .Register(*registry);
    
    auto &memory_used_family = prometheus::BuildGauge()
        .Name("memory_used")
        .Help("Memory used by worker in bytes")
        .Register(*registry);
    
    auto &task_processing_time_family = prometheus::BuildGauge()
        .Name("task_processing_time")
        .Help("Task processing time (in seconds)")
        .Register(*registry);

    std::ifstream file("/proc/stat");
    int ign;
    std::string cpu_name;
    
    while (true) {
        CPUInfo cpu;

        file >> cpu_name >> cpu.last_total_user >> cpu.last_total_user_low
             >> cpu.last_total_sys >> cpu.last_total_idle
             >> ign >> ign >> ign >> ign >> ign >> ign;
        
        if (cpu_name.find("cpu") != 0)
            break;

        cpu.gauge = &cpu_usage_family.Add({{ "cpu", std::string(cpu_name) }});
        cpu_usage.insert({ std::string(cpu_name), cpu });
    }

    file.close();

    memory_used_gauge = &memory_used_family.Add({});
    task_processing_time_gauge = &task_processing_time_family.Add({});

    gateway.RegisterCollectable(registry);

    is_running = true;
    thread = std::thread(&MetricsCollector::mainLoop, this);

    is_task_running = false;
}

void MetricsCollector::mainLoop()
{
    while (is_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        getMemoryUsed();
        getCPUUsage();
        
        if (is_task_running) {
            struct timespec time;
            clock_gettime(CLOCK_MONOTONIC, &time);
            task_processing_time_gauge->Set((time.tv_sec - task_start.tv_sec) + (time.tv_nsec - task_start.tv_nsec) * 1e-9);
        }
        else
            task_processing_time_gauge->Set(0);

        gateway.PushAdd();
    }
}

MetricsCollector::~MetricsCollector()
{
    is_running = false;
    thread.join();
}

void MetricsCollector::getMemoryUsed()
{
    std::ifstream file("/proc/self/statm");
    if (!file.is_open()) {
        memory_used_gauge->Set(0);
        return;
    }

    long mem_pages = 0;
    file >> mem_pages;
    file.close();

    memory_used_gauge->Set(mem_pages * (double)getpagesize());
}

void MetricsCollector::getCPUUsage()
{
    std::ifstream file("/proc/stat");
    uint64_t total_user, total_user_low, total_sys, total_idle, total;
    double percent;
    
    std::string cpu_name;
    int ign;
    
    while (true) {
        file >> cpu_name >> total_user >> total_user_low >> total_sys >> total_idle
             >> ign >> ign >> ign >> ign >> ign >> ign;
        
        if (cpu_name.find("cpu") != 0)
            break;
        
        CPUInfo &cpu = cpu_usage[cpu_name];
        if (total_user < cpu.last_total_user || total_user_low < cpu.last_total_user_low ||
            total_sys < cpu.last_total_sys || total_idle < cpu.last_total_idle) {
            // overflow detection
            percent = -1.0;
        }
        else {
            total = (total_user - cpu.last_total_user) + (total_user_low - cpu.last_total_user_low) +
                (total_sys - cpu.last_total_sys);
            
            percent = total;
            total += (total_idle - cpu.last_total_idle);
            percent /= total;
            percent *= 100;
        }
    
        cpu.last_total_user = total_user;
        cpu.last_total_user_low = total_user_low;
        cpu.last_total_sys = total_sys;
        cpu.last_total_idle = total_idle;

        cpu.gauge->Set(percent);
    }

    file.close();
}

void MetricsCollector::startTask()
{
    is_task_running = true;
    clock_gettime(CLOCK_MONOTONIC, &task_start);
}

void MetricsCollector::stopTask()
{
    is_task_running = false;
    task_processing_time_gauge->Set(0);
    gateway.PushAdd();
}