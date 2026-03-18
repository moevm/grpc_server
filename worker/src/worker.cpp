#include "../include/worker.hpp"

#include "communication.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <cstdlib>
#include <ctime> 

void Worker::LogStateChange(WorkerState new_state) {
  const char *state_names[] = {"BOOTING", "FREE", "BUSY", "SHUTTING_DOWN",
                               "ERROR"};

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
    GetPolicyRequest req;
    req.set_worker_id(worker_id);
    req.set_config_version(current_config_version);

    WorkerPolicy policy;
    grpc::ClientContext context;

    auto status = stub_->GetPolicy(&context, req, &policy);

    if (!status.ok()) {
      spdlog::error("GetPolicy failed: " + status.error_message());
      return;
    }

    if (policy.config_version() == 0) {
      spdlog::info("Policy unchanged");
      return;
    }
    else {
      current_config_version = policy.config_version();
    }

    spdlog::info("Policy received");

  } catch (const std::exception &e) {
    spdlog::error("requestPolicyFromController exception: {}", e.what());
  }
}

void Worker::classifyDomain(const std::string &domain) {
  try {
    spdlog::info("Worker {} classifying domain '{}'", worker_id, domain);

    ClassifyRequest req;
    req.set_worker_id(worker_id);
    req.set_domain(domain);

    ClassifyResponse resp;
    grpc::ClientContext context;

    auto status = stub_->Classify(&context, req, &resp);
    if (!status.ok()) {
      spdlog::error("Classify failed: " + status.error_message());
      return;
    }

    std::string cat =
        resp.categories_size() > 0 ? resp.categories(0) : "unknown";
    spdlog::info("Domain '{}' classified as category '{}' with trust level {}",
                 domain, cat, resp.trust_level());

  } catch (const std::exception &e) {
    spdlog::error(std::string("classifyDomain: ") + e.what());
  }
}

void Worker::statsReport() {
  try {
    spdlog::info("Worker {} send stats", worker_id);

    StatsReport report;
    report.set_worker_id(worker_id);
    report.set_time(time(nullptr));

    grpc::ClientContext context;
    google::protobuf::Empty response;

    auto status = stub_->SendStats(&context, report, &response);
    if (!status.ok()) {
      spdlog::error("SendStats failed: " + status.error_message());
      return;
    }

    spdlog::info("Stats sent successfully");

  } catch (const std::exception &e) {
    spdlog::error("statsReport failed: {}", e.what());
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
