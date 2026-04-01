#include "../include/worker.hpp"
#include "../include/dpdk_filter/proc_packets.h"
#include "communication.grpc.pb.h"
#include <cstdlib>
#include <ctime>
#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>
#include <signal.h>
#include <thread>

static volatile bool stop_flag = false;

static void signal_handler(int signum) {  
  if (signum == SIGINT || signum == SIGTERM) {
    spdlog::info("Signal {} received, shutting down.", signum);
    stop_flag = true;
  }
}

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

void Worker::initDPDK(int argc, char **argv) {
  unsigned mbuf_quantity_in_pool = 8192;
  unsigned cache_size_per_kernel = 250;
  uint16_t priv_size = 0;

  int ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    throw std::runtime_error("EAL init failed");
  }

  mbuf_pool = rte_pktmbuf_pool_create(
      "POOL", mbuf_quantity_in_pool, cache_size_per_kernel, priv_size,
      RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
  if (!mbuf_pool) {
    throw std::runtime_error("Failed to create mbuf pool");
  }
  const char *iface_in = getenv("DPDK_PORT_IN");
  const char *iface_out = getenv("DPDK_PORT_OUT");

  if (!iface_in || !iface_out) {
    throw std::runtime_error("DPDK_PORT_IN and DPDK_PORT_OUT must be set");
  }

  port_in = init_struct_af_xdp_port(iface_in, mbuf_pool);
  port_out = init_struct_af_xdp_port(iface_out, mbuf_pool);

  if (af_xdp_port_init(port_in) || af_xdp_port_init(port_out)) {
    throw std::runtime_error("Init ports");
  }

  if (af_xdp_port_start(port_in->port_id) ||
      af_xdp_port_start(port_out->port_id)) {
    throw std::runtime_error("Start ports");
  }

  spdlog::info("DPDK initialized: in_port={}, out_port={}", port_in->port_id,
               port_out->port_id);
}

void Worker::requestPolicyFromController() {
  try {
    spdlog::info("Worker {} requests policy", worker_id);
    GetPolicyRequest req;
    req.set_worker_id(worker_id);
    req.set_config_version(current_config_version);

    GetPolicyResponse resp;
    grpc::ClientContext context;

    auto status = stub_->GetPolicy(&context, req, &resp);

    if (!status.ok()) {
      spdlog::error("GetPolicy failed: " + status.error_message());
      return;
    }

    switch (resp.result()) {
    case GetPolicyResponse::POLICY_PROVIDED:
      spdlog::info("Policy received");
      current_config_version = resp.policy().config_version();
      break;
    case GetPolicyResponse::POLICY_UNCHANGED:
      spdlog::info("Policy unchanged");
      break;
    default:
      spdlog::error("Unknown response result");
    }

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
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  spdlog::info("Signal handlers registered");

  srand(time(nullptr));
  SetState(WorkerState::FREE);
  requestPolicyFromController();
}

Worker::~Worker() {
  spdlog::info("Worker {} shutting down", worker_id);

  if (port_in && port_out) {
    af_xdp_port_close(port_in);
    af_xdp_port_close(port_out);
    af_xdp_port_destroy(port_in);
    af_xdp_port_destroy(port_out);
    spdlog::info("DPDK ports closed");
  }
}

void Worker::MainLoop() {
  using namespace std::chrono;

  last_policy_time = steady_clock::now();
  last_stats_time = steady_clock::now();

  struct rte_mbuf *pkts[32];
  uint16_t nb_pkts = 32;
  uint16_t queue_number = 0;
  while (!stop_flag && GetState() != WorkerState::SHUTTING_DOWN) {
    pakage_processing(port_in, port_out, queue_number, nb_pkts, pkts);

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

  if (stop_flag) {
        SetState(WorkerState::SHUTTING_DOWN);
    }
}
