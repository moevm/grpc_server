#ifndef WORKER_HPP
#define WORKER_HPP

#include "communication.grpc.pb.h"
#include "communication.pb.h"
#include "../include/metrics_collector.hpp"
#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <atomic>
extern "C" {
#include "dpdk_filter/dns_cache.h"
#include "dpdk_filter/filtr_packets.h"
#include "dpdk_filter/net_port.h"
#include "dpdk_filter/proc_packets.h"
#include "dpdk_filter/types.h"
}
#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>

#define EXPECTED_POLICY_TIME 60
#define MIN_POLICY_TIME 30
#define MAX_POLICY_TIME 45

#define EXPECTED_STATS_TIME 60
#define MIN_STATS_TIME 30
#define MAX_STATS_TIME 45

enum class WorkerState {
  FREE,          // Ожидает задачи
  SHUTTING_DOWN, // Завершение работы
};

class Worker {
  uint64_t worker_id = 0;

  uint64_t current_config_version = 0;
  std::chrono::time_point<std::chrono::steady_clock> last_policy_time;
  std::chrono::time_point<std::chrono::steady_clock> last_stats_time;
  int64_t policy_interval = MIN_POLICY_TIME;
  int64_t stats_interval = MIN_STATS_TIME;

  struct net_port *port_in = nullptr;
  struct net_port *port_out = nullptr;
  struct net_port *port_exception = nullptr;
  struct rte_mempool *mbuf_pool = nullptr;
  std::mutex policy_mutex;
  struct BASE_POLICY current_policy;
  uint16_t queue_number = 0;

  std::unique_ptr<DataService::Stub> stub_;
  inline static Worker *instance = nullptr;

  WorkerState state;
  void LogStateChange(WorkerState new_state);
  void SetState(WorkerState new_state);

  std::unique_ptr<MetricsCollector> metrics_collector_;

  std::atomic<uint64_t> packets_received_count{0};
  std::atomic<uint64_t> packets_passed_count{0};
  std::atomic<uint64_t> packets_dropped_count{0};

public:
  Worker(uint64_t id);
  ~Worker();

  void initDPDK(int argc, char **argv);
  inline uint64_t GetID() const { return worker_id; }
  void requestPolicyFromController();
  bool classify(const std::string &type, const std::string &target,
                struct requested_classification *out_req);
  void forward_to_out(struct net_port *incoming_port,
                      struct net_port *outgoing_port, uint16_t queue_number);
  void statsReport();
  WorkerState GetState() const { return state; }
  static Worker *getInstance();
  void MainLoop();

  void RecordPacketReceived();
  void RecordPacketPassed();
  void RecordPacketDropped(const std::string& reason);
  void RecordDomainBlocked(const std::string& domain_or_ip);
  void RecordTaskStart();
  void RecordTaskEnd();
};

#endif