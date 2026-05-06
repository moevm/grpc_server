#include "../include/worker.hpp"

#include "communication.grpc.pb.h"
#include <cstdlib>
#include <ctime>
#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>
#include <thread>

void Worker::LogStateChange(WorkerState new_state) {
  const char *state_names[] = {"FREE", "SHUTTING_DOWN"};

  spdlog::info("Switch states: {} -> {}", state_names[static_cast<int>(state)],
               state_names[static_cast<int>(new_state)]);
}

void Worker::SetState(WorkerState new_state) {
  if (state != new_state) {
    LogStateChange(new_state);
    state = new_state;
  }
}

void Worker::requestPolicyFromController() {
  try {
    spdlog::info("Worker {} requests policy", worker_id);

    RecordTaskStart();

    GetPolicyRequest req;
    req.set_worker_id(worker_id);
    req.set_config_version(current_config_version);

    GetPolicyResponse resp;
    grpc::ClientContext context;

    auto status = stub_->GetPolicy(&context, req, &resp);

    if (!status.ok()) {
      spdlog::error("GetPolicy failed: " + status.error_message());
      RecordPacketDropped("grpc_error");
      RecordTaskEnd();
      return;
    }

    switch (resp.result()) {
    case GetPolicyResponse::POLICY_PROVIDED:
      spdlog::info("Policy received");
      current_config_version = resp.policy().config_version();
      RecordPacketPassed();
      break;
    case GetPolicyResponse::POLICY_UNCHANGED:
      spdlog::info("Policy unchanged");
      RecordPacketPassed();
      break;
    default:
      spdlog::error("Unknown response result");
      RecordPacketDropped("unknown_response");
    }

    RecordTaskEnd();

  } catch (const std::exception &e) {
    spdlog::error("requestPolicyFromController exception: {}", e.what());
    RecordPacketDropped("exception");
    RecordTaskEnd();
  }
}

void Worker::classifyDomain(const std::string &domain) {

    RecordPacketReceived();

  try {
    spdlog::info("Worker {} classifying domain '{}'", worker_id, domain);

    RecordTaskStart();

    ClassifyRequest req;
    req.set_worker_id(worker_id);
    req.set_domain(domain);

    ClassifyResponse resp;
    grpc::ClientContext context;

    auto status = stub_->Classify(&context, req, &resp);
    if (!status.ok()) {
      spdlog::error("Classify failed: " + status.error_message());
      RecordPacketDropped("classification_failed");
      RecordTaskEnd();
      return;
    }

    std::string cat =
        resp.categories_size() > 0 ? resp.categories(0) : "unknown";
    spdlog::info("Domain '{}' classified as category '{}' with trust level {}",
                 domain, cat, resp.trust_level());

    if (resp.trust_level() < 5) {
      RecordDomainBlocked(domain);
      RecordPacketDropped("low_trust_level");
    } else {
      RecordPacketPassed();
    }
    RecordTaskEnd();

  } catch (const std::exception &e) {
    spdlog::error(std::string("classifyDomain: ") + e.what());
    RecordPacketDropped("exception");
    RecordTaskEnd();
  }
}

void Worker::statsReport() {
  try {
    spdlog::info("Worker {} send stats", worker_id);

    RecordTaskStart();

    StatsReport report;
    report.set_worker_id(worker_id);
    report.set_time(time(nullptr));

    report.set_packets_received(packets_received_count.load());
    report.set_packets_passed(packets_passed_count.load());
    report.set_packets_dropped(packets_dropped_count.load());

    grpc::ClientContext context;
    google::protobuf::Empty response;

    auto status = stub_->SendStats(&context, report, &response);
    if (!status.ok()) {
      spdlog::error("SendStats failed: " + status.error_message());
      RecordPacketDropped("stats_send_failed");
      RecordTaskEnd();
      return;
    }

    spdlog::info("Stats sent successfully");

    RecordPacketPassed();
    RecordTaskEnd();

  } catch (const std::exception &e) {
    spdlog::error("statsReport failed: {}", e.what());
    RecordPacketDropped("exception");
    RecordTaskEnd();
  }
}

Worker::Worker(uint64_t id) : worker_id(id), state(WorkerState::FREE) {

  std::string controller_addr = "localhost:50051";
  if (const char *env_addr = getenv("CONTROLLER_GRPC_ADDR")) {
    controller_addr = env_addr;
  }
  auto channel =
      grpc::CreateChannel(controller_addr, grpc::InsecureChannelCredentials());
  stub_ = DataService::NewStub(channel);
  spdlog::info("gRPC channel created to {}", controller_addr);

  srand(time(nullptr));
  SetState(WorkerState::FREE);
  requestPolicyFromController();
}

Worker::~Worker() {
  SetState(WorkerState::SHUTTING_DOWN);
  spdlog::info("Worker {} shutting down", worker_id);
}

void Worker::MainLoop() {
  using namespace std::chrono;

  last_policy_time = steady_clock::now();
  last_stats_time = steady_clock::now();

  while (GetState() != WorkerState::SHUTTING_DOWN) {
    auto now = steady_clock::now();

    int64_t seconds_since_stats = (now - last_stats_time) / 1s;
    if (seconds_since_stats >= stats_interval) {
      std::thread([this]() { statsReport(); }).detach();
      last_stats_time = now;
      stats_interval =
          MIN_STATS_TIME + (rand() % (MAX_STATS_TIME - MIN_STATS_TIME + 1));
    }

    int64_t seconds_since_policy = (now - last_policy_time) / 1s;
    if (seconds_since_policy >= policy_interval) {
      std::thread([this]() { requestPolicyFromController(); }).detach();
      last_policy_time = now;
      policy_interval =
          MIN_POLICY_TIME + (rand() % (MAX_POLICY_TIME - MIN_POLICY_TIME + 1));
    }

    std::this_thread::sleep_for(milliseconds(100));
  }
}


void Worker::RecordPacketReceived() {
  packets_received_count++;
  if (metrics_collector_) {
    metrics_collector_->IncrementPacketsReceived();
  }
}

void Worker::RecordPacketPassed() {
  packets_passed_count++;
  if (metrics_collector_) {
    metrics_collector_->IncrementPacketsPassed();
  }
}

void Worker::RecordPacketDropped(const std::string& reason) {
  packets_dropped_count++;
  if (metrics_collector_) {
    metrics_collector_->IncrementPacketsDropped(reason);
  }
}

void Worker::RecordDomainBlocked(const std::string& domain_or_ip) {
  if (metrics_collector_) {
    metrics_collector_->IncrementBlockedDomain(domain_or_ip);
  }
}

void Worker::RecordTaskStart() {
  if (metrics_collector_) {
    metrics_collector_->StartTask();
  }
}

void Worker::RecordTaskEnd() {
  if (metrics_collector_) {
    metrics_collector_->StopTask();
  }
}