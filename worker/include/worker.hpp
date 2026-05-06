#ifndef WORKER_HPP
#define WORKER_HPP

#include "communication.grpc.pb.h"
#include "communication.pb.h"
#include "../include/metrics_collector.hpp"
#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <atomic>

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

  std::unique_ptr<DataService::Stub> stub_;

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

  inline uint64_t GetID() const { return worker_id; }
  void requestPolicyFromController();
  void classifyDomain(const std::string &domain);
  void statsReport();
  WorkerState GetState() const { return state; }
  void MainLoop();

  void RecordPacketReceived();
  void RecordPacketPassed();
  void RecordPacketDropped(const std::string& reason);
  void RecordDomainBlocked(const std::string& domain_or_ip);
  void RecordTaskStart();
  void RecordTaskEnd();
};

#endif