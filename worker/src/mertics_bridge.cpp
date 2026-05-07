#include "metrics_bridge.hpp"
#include "worker.hpp"

static Worker* g_worker = nullptr;

extern "C" void metrics_set_worker(Worker* worker) {
    g_worker = worker;
}

extern "C" void metrics_record_packet_received(void) {
    if (g_worker) {
        g_worker->RecordPacketReceived();
    }
}

extern "C" void metrics_record_packet_passed(void) {
    if (g_worker) {
        g_worker->RecordPacketPassed();
    }
}

extern "C" void metrics_record_packet_dropped(const char* reason) {
    if (g_worker && reason) {
        g_worker->RecordPacketDropped(std::string(reason));
    }
}

extern "C" void metrics_record_domain_blocked(const char* domain_or_ip) {
    if (g_worker && domain_or_ip) {
        g_worker->RecordDomainBlocked(std::string(domain_or_ip));
    }
}

extern "C" void metrics_record_task_start(void) {
    if (g_worker) {
        g_worker->RecordTaskStart();
    }
}

extern "C" void metrics_record_task_end(void) {
    if (g_worker) {
        g_worker->RecordTaskEnd();
    }
}