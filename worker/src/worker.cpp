#include "worker.hpp"
#include "communication.grpc.pb.h"
#include "proc_packets.h"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <grpcpp/grpcpp.h>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <thread>

extern "C" bool worker_classify(const char *type, const char *target,
                                struct requested_classification *out_req) {
  Worker *worker = Worker::getInstance();
  if (!worker) {
    fprintf(stderr, "worker_classify: worker is null\n");
    return false;
  }
  return worker->classify(std::string(type), std::string(target), out_req);
}

static volatile bool stop_flag = false;

static void signal_handler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    spdlog::info("Signal {} received, shutting down.", signum);
    stop_flag = true;
  }
}

Worker *Worker::getInstance() { return instance; }

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
  port_exception = init_struct_tap_port("tap0", mbuf_pool);

  if (net_port_init(port_in) || net_port_init(port_out) ||
      net_port_init(port_exception)) {
    throw std::runtime_error("Init ports");
  }

  if (net_port_start(port_in->port_id) || net_port_start(port_out->port_id) ||
      net_port_start(port_exception->port_id)) {
    throw std::runtime_error("Start ports");
  }

  init_dns_cache();
  init_ip_cache();

  spdlog::info("DPDK initialized: in_port={}, out_port={}", port_in->port_id,
               port_out->port_id);
}

void Worker::forward_to_out(struct net_port *incoming_port,
                            struct net_port *outgoing_port,
                            uint16_t queue_number) {
  struct rte_mbuf *tap_pkts[32];
  uint16_t nb_tap =
      rte_eth_rx_burst(incoming_port->port_id, queue_number, tap_pkts, 32);
  for (int i = 0; i < nb_tap; i++) {
    int ret =
        rte_eth_tx_burst(outgoing_port->port_id, queue_number, &tap_pkts[i], 1);
    if (ret < 1) {
      spdlog::warn("Failed to send packet");
      // PLUG (to be added later) - need to add processing for this case
      rte_pktmbuf_free(tap_pkts[i]);
    }
  }
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

    enable = resp.filtering_enabled();
    spdlog::info("Filtering: {}", enable ? "ON" : "OFF");

    switch (resp.result()) {
    case GetPolicyResponse::POLICY_PROVIDED: {
      spdlog::info("Policy received");
      const auto &pol = resp.policy();
      std::lock_guard<std::mutex> lock(policy_mutex);
      memset(&current_policy, 0, sizeof(current_policy));

      int block_cat_count = pol.block_categories_size();
      for (int i = 0; i < block_cat_count; ++i) {
        strncpy(current_policy.locked_categories[i],
                pol.block_categories(i).c_str(), CATEGORY_MAX_LEN - 1);
        current_policy.locked_categories[i][CATEGORY_MAX_LEN - 1] = '\0';
      }

      int idx = 0;
      for (const auto &[category, min_trust] : pol.block_by_trust()) {
        if (idx >= MAX_CATEGORIES_BY_TRUST_LVL)
          break;

        strncpy(
            current_policy.categories_with_lvl[idx].locked_by_trust_category,
            category.c_str(), CATEGORY_MAX_LEN - 1);
        current_policy.categories_with_lvl[idx]
            .locked_by_trust_category[CATEGORY_MAX_LEN - 1] = '\0';

        current_policy.categories_with_lvl[idx].trust_lvl = min_trust;

        idx++;
      }

      int block_dom_count = pol.block_domains_size();
      for (int i = 0; i < block_dom_count; ++i) {
        strncpy(current_policy.block_domains[i], pol.block_domains(i).c_str(),
                DOMAIN_MAX_LEN - 1);
        current_policy.block_domains[i][DOMAIN_MAX_LEN - 1] = '\0';
      }

      int allow_dom_count = pol.allow_domains_size();
      for (int i = 0; i < allow_dom_count; ++i) {
        strncpy(current_policy.allow_domains[i], pol.allow_domains(i).c_str(),
                DOMAIN_MAX_LEN - 1);
        current_policy.allow_domains[i][DOMAIN_MAX_LEN - 1] = '\0';
      }

      int block_ips_count = pol.block_ips_size();
      int block_ip4_idx = 0;
      int block_ip6_idx = 0;

      for (int i = 0; i < block_ips_count; ++i) {
        const std::string &ip_str = pol.block_ips(i);

        struct in_addr ip4;
        if (inet_pton(AF_INET, ip_str.c_str(), &ip4) == 1) {
          if (block_ip4_idx < MAX_IP4) {
            current_policy.block_ip4[block_ip4_idx++] = ip4.s_addr;
          }
          continue;
        }

        struct in6_addr ip6;
        if (inet_pton(AF_INET6, ip_str.c_str(), &ip6) == 1) {
          if (block_ip6_idx < MAX_IP6) {
            memcpy(current_policy.block_ip6[block_ip6_idx++], ip6.s6_addr,
                   IP6_LEN);
          }
        }
      }

      int allow_ips_count = pol.allow_ips_size();
      int allow_ip4_idx = 0;
      int allow_ip6_idx = 0;

      for (int i = 0; i < allow_ips_count; ++i) {
        const std::string &ip_str = pol.allow_ips(i);

        struct in_addr ip4;
        if (inet_pton(AF_INET, ip_str.c_str(), &ip4) == 1) {
          if (allow_ip4_idx < MAX_IP4) {
            current_policy.allow_ip4[allow_ip4_idx++] = ip4.s_addr;
          }
          continue;
        }

        struct in6_addr ip6;
        if (inet_pton(AF_INET6, ip_str.c_str(), &ip6) == 1) {
          if (allow_ip6_idx < MAX_IP6) {
            memcpy(current_policy.allow_ip6[allow_ip6_idx++], ip6.s6_addr,
                   IP6_LEN);
          }
        }
      }
      current_policy.min_trust_level = pol.min_trust_level();
      current_policy.ttl_ip = pol.ttl_ip();
      current_policy.ttl_domain = pol.ttl_domain();

      current_config_version = pol.config_version();
      spdlog::info("Clearing cache due to policy update");
      clear_ip_cache();
      clear_dns_cache();
      spdlog::info("POLICY LOADED");
      spdlog::info("Config version: {}", current_config_version);
      spdlog::info("Min trust level: {}", current_policy.min_trust_level);

      spdlog::info("Blocked categories ({} total)", block_cat_count);
      for (int i = 0; i < block_cat_count && i < MAX_CATEGORIES; ++i) {
        if (strlen(current_policy.locked_categories[i]) > 0) {
          spdlog::info("blocked_categories: {}",
                       current_policy.locked_categories[i]);
        }
      }

      spdlog::info("Blocked domains ({} total)", block_dom_count);
      for (int i = 0; i < block_dom_count && i < MAX_DOMAINS; ++i) {
        if (strlen(current_policy.block_domains[i]) > 0) {
          spdlog::info("block_domains: {}", current_policy.block_domains[i]);
        }
      }

      spdlog::info("Allowed domains ({} total)", allow_dom_count);
      for (int i = 0; i < allow_dom_count && i < MAX_DOMAINS; ++i) {
        if (strlen(current_policy.allow_domains[i]) > 0) {
          spdlog::info("allow_domains: {}", current_policy.allow_domains[i]);
        }
      }

      spdlog::info("Blocked IPs ({} total)", block_ips_count);
      spdlog::info("Allowed IPs ({} total)", allow_ips_count);
      break;
    }

    case GetPolicyResponse::POLICY_UNCHANGED: {
      spdlog::info("Policy unchanged");
      break;
    }
    default: {
      spdlog::error("Unknown response result");
    }
    }

  } catch (const std::exception &e) {
    spdlog::error("requestPolicyFromController exception: {}", e.what());
  }
}

bool Worker::classify(const std::string &type, const std::string &target,
                      struct requested_classification *out_req) {
  try {
    spdlog::info("Worker {} classifying '{}' as {}", worker_id, target, type);

    ClassifyRequest req;
    req.set_worker_id(worker_id);
    req.set_type(type);
    req.set_target(target);

    ClassifyResponse resp;
    grpc::ClientContext context;

    auto status = stub_->Classify(&context, req, &resp);
    if (!status.ok()) {
      spdlog::error("Classify failed: " + status.error_message());
      return false;
    }

    std::string categories_str;
    for (int i = 0; i < resp.categories_size(); ++i) {
      if (i > 0)
        categories_str += ", ";
      categories_str += resp.categories(i);
    }
    spdlog::info(
        "Target '{}' classified as categories [{}] with trust level {}", target,
        categories_str, resp.trust_level());

    out_req->get_trust_level = resp.trust_level();
    int cat_count = std::min(resp.categories_size(), MAX_CATEGORIES);
    for (int i = 0; i < cat_count; ++i) {
      strncpy(out_req->get_categories[i], resp.categories(i).c_str(),
              CATEGORY_MAX_LEN - 1);
    }
    return true;
  } catch (const std::exception &e) {
    spdlog::error(std::string("classifyDomain: ") + e.what());
    return false;
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
  instance = this;
  std::string controller_addr = "localhost:50051";
  if (const char *env_addr = getenv("CONTROLLER_GRPC_ADDR")) {
    controller_addr = env_addr;
  }
  auto channel =
      grpc::CreateChannel(controller_addr, grpc::InsecureChannelCredentials());
  stub_ = DataService::NewStub(channel);
  spdlog::info("Worker ID: {}", worker_id);
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
    save_all_cache_to_sqlite(NULL);
    save_all_cache_ip_to_sqlite(NULL);
    free_dns_cache();
    free_ip_cache();

    net_port_close(port_in);
    net_port_close(port_out);
    net_port_close(port_exception);

    net_port_destroy(port_in);
    net_port_destroy(port_out);
    net_port_destroy(port_exception);
    spdlog::info("DPDK ports closed");
  }
}

void Worker::MainLoop() {
  struct BASE_POLICY local_policy;
  using namespace std::chrono;

  last_policy_time = steady_clock::now();
  last_stats_time = steady_clock::now();

  struct rte_mbuf *pkts[32];
  uint16_t nb_pkts = 32;
  uint16_t queue_number = 0;
  uint64_t timer_check_counter = 0;
  const uint64_t timer_check_interval = 10000;
  while (!stop_flag && GetState() != WorkerState::SHUTTING_DOWN) {
    {
      std::lock_guard<std::mutex> lock(policy_mutex);
      local_policy = current_policy;
    }
    forward_to_out(port_exception, port_in, queue_number);
    pakage_processing(port_in, port_out, port_exception, queue_number, nb_pkts,
                      pkts, &local_policy, !enable);
    forward_to_out(port_out, port_in, queue_number);
    if (++timer_check_counter >= timer_check_interval) {
      rte_timer_manage();
      timer_check_counter = 0;
    }

    auto now = steady_clock::now();

    int64_t seconds_since_stats = (now - last_stats_time) / 1s;
    if (seconds_since_stats >= stats_interval) {
      std::thread([this]() { statsReport(); }).detach();
      last_stats_time = now;
      stats_interval =
          MIN_STATS_TIME + (rand() % (MAX_STATS_TIME - MIN_STATS_TIME + 1));
      spdlog::info("Next stats report in {}s", stats_interval);
    }

    int64_t seconds_since_policy = (now - last_policy_time) / 1s;
    if (seconds_since_policy >= policy_interval) {
      std::thread([this]() { requestPolicyFromController(); }).detach();
      last_policy_time = now;
      policy_interval =
          MIN_POLICY_TIME + (rand() % (MAX_POLICY_TIME - MIN_POLICY_TIME + 1));
      spdlog::info("Next policy request in {}s", policy_interval);
    }
  }

  if (stop_flag) {
    SetState(WorkerState::SHUTTING_DOWN);
  }
}
