#ifndef METRICS_BRIDGE_HPP
#define METRICS_BRIDGE_HPP

#ifdef __cplusplus
extern "C" {
#endif

void metrics_init(void);

void metrics_record_packet_received(void);
void metrics_record_packet_passed(void);
void metrics_record_packet_dropped(const char* reason);
void metrics_record_domain_blocked(const char* domain_or_ip);
void metrics_record_task_start(void);
void metrics_record_task_end(void);

#ifdef __cplusplus
}
#endif

#endif