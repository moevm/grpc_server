#include "metrics-collector.hpp"

#include <thread>
#include <chrono>
#include <unistd.h>
#include <fstream>
#include <iostream>

namespace {
    double GetMemoryUsed()
    {
        std::ifstream file("/proc/self/statm");
        if (!file.is_open()) {
            return 0;
        }

        long mem_pages = 0;
        file >> mem_pages;
        file.close();
        return mem_pages * (double)getpagesize();
    }
}

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

        file >> cpu_name >> cpu.time.user >> cpu.time.user_low
             >> cpu.time.sys >> cpu.time.idle
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
    thread = std::thread(&MetricsCollector::MainLoop, this);
    is_task_running = false;
}

void MetricsCollector::MainLoop()
{
    while (is_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        memory_used_gauge->Set(::GetMemoryUsed());
        GetCPUUsage();
        
        if (is_task_running) {
            auto cur_time = std::chrono::high_resolution_clock::now();
            task_processing_time_gauge->Set(std::chrono::duration<double>(cur_time - task_start).count());
        }
        else {
            task_processing_time_gauge->Set(0);
        }

        int status = gateway.PushAdd();
        if (status != 200) {
            std::cerr << "[ERROR] Failed to push metrics. Status " << status << std::endl;
        }
    }
}

MetricsCollector::~MetricsCollector()
{
    is_running = false;
    thread.join();
}

void MetricsCollector::GetCPUUsage()
{
    std::ifstream file("/proc/stat");
    CPUInfo::Time cur_time;
    double percent;
    
    std::string cpu_name;
    int ign;
    
    while (true) {
        file >> cpu_name >> cur_time.user >> cur_time.user_low >> cur_time.sys >> cur_time.idle
             >> ign >> ign >> ign >> ign >> ign >> ign;
        
        if (cpu_name.find("cpu") != 0)
            break;
        
        CPUInfo &cpu = cpu_usage[cpu_name];
        if (cur_time.user < cpu.time.user || cur_time.user_low < cpu.time.user_low ||
            cur_time.sys < cpu.time.sys || cur_time.idle < cpu.time.idle) {
            // overflow detection
            percent = -1.0;
        }
        else {
            uint64_t total = (cur_time.user - cpu.time.user) + (cur_time.user_low - cpu.time.user_low) +
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

void MetricsCollector::StartTask()
{
    is_task_running = true;
    task_start = std::chrono::high_resolution_clock::now();
}

void MetricsCollector::StopTask()
{
    is_task_running = false;
    task_processing_time_gauge->Set(0);
    gateway.PushAdd();
}
