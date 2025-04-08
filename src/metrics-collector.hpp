#include <prometheus/gauge.h>
#include <prometheus/registry.h>
#include <prometheus/gateway.h>

class MetricsCollector {
    prometheus::Gateway gateway;
    std::shared_ptr<prometheus::Registry> registry;

    struct CPUInfo {
        prometheus::Gauge *gauge;
        uint64_t last_total_user;
        uint64_t last_total_user_low;
        uint64_t last_total_sys;
        uint64_t last_total_idle;
    };

    prometheus::Gauge *memory_used_gauge;
    std::unordered_map<std::string, CPUInfo> cpu_usage;
    prometheus::Gauge *task_processing_time_gauge;
    bool is_running;
    std::thread thread;

    bool is_task_running;
    struct timespec task_start;

    void getMemoryUsed();
    void getCPUUsage();
    void mainLoop();

public:
    MetricsCollector(const char *gateway_address, const char *gateway_port, const char *worker_name);
    ~MetricsCollector();

    void startTask();
    void stopTask();
};