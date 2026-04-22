#include "dns_cache.h"
#include "net_port.h"
#include "proc_packets.h"
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static volatile int running = 1;

static void signal_handler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    printf("\n Signal %d received, shutting down.\n", signum);
    running = 0;
  }
}

void forward_to_out(struct net_port *incoming_port,
                    struct net_port *outgoing_port, uint16_t queue_number) {
  struct rte_mbuf *tap_pkts[32];
  uint16_t nb_tap =
      rte_eth_rx_burst(incoming_port->port_id, queue_number, tap_pkts, 32);
  for (int i = 0; i < nb_tap; i++) {
    int ret =
        rte_eth_tx_burst(outgoing_port->port_id, queue_number, &tap_pkts[i], 1);
    if (ret < 1) {
      printf("[ERROR] Failed to send packet\n");
      // PLUG (to be added later) - need to add processing for this case
      rte_pktmbuf_free(tap_pkts[i]);
    }
  }
}

int main(int argc, char **argv) {
  // since BASE_POLICY is filled when initializing worker, let’s initialize here
  struct BASE_POLICY policy;
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    printf("[ERROR] Failed to set SIGINT handler\n");
    return 1;
  }
  if (signal(SIGTERM, signal_handler) == SIG_ERR) {
    printf("[ERROR] Failed to set SIGTERM handler\n");
    return 1;
  }

  struct net_port *port_in = NULL;
  struct net_port *port_out = NULL;
  struct net_port *port_exception = NULL;
  struct rte_mempool *mbuf_pool;
  unsigned mbuf_quantity_in_pool = 8192;
  unsigned cache_size_per_kernel = 250;
  uint16_t queue_number = 0;
  uint16_t nb_pkts = 32;
  uint16_t priv_size = 0;
  struct rte_mbuf *pkts[32];

  int ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    printf("[ERROR] EAL init failed: %s\n", rte_strerror(rte_errno));
    return 1;
  }

  mbuf_pool = rte_pktmbuf_pool_create(
      "POOL", mbuf_quantity_in_pool, cache_size_per_kernel, priv_size,
      RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
  if (!mbuf_pool) {
    printf("[ERROR] Failed to create mbuf pool: %s\n", rte_strerror(rte_errno));
    return -1;
  }
  init_dns_cache();

#ifdef VIRT_PORTS
  printf("Using virtual ports: veth0/veth1\n");
  port_in = init_struct_af_xdp_port("veth0", mbuf_pool);
  port_out = init_struct_af_xdp_port("veth1", mbuf_pool);
#else
  printf("Using real ports: eth0/eth1\n");
  port_in = init_struct_af_xdp_port("eth0", mbuf_pool);
  port_out = init_struct_af_xdp_port("eth1", mbuf_pool);
#endif

  port_exception = init_struct_tap_port("tap0", mbuf_pool);

  if (!port_in || !port_out || !port_exception) {
    return 1;
  }

  if (net_port_init(port_in) || net_port_init(port_out) ||
      net_port_init(port_exception)) {
    return 1;
  }

  if (net_port_start(port_in->port_id) || net_port_start(port_out->port_id) ||
      net_port_start(port_exception->port_id)) {
    return 1;
  }

  printf("An endless cycle has been started. Packets pass from port with id=%u "
         "to port with id=%u\n",
         port_in->port_id, port_out->port_id);

  uint64_t timer_check_counter = 0;
  const uint64_t timer_check_interval = 10000;

  while (running) {
    forward_to_out(port_exception, port_in, queue_number);
    pakage_processing(port_in, port_out, port_exception, queue_number, nb_pkts,
                      pkts, &policy);
    forward_to_out(port_out, port_in, queue_number);

    if (++timer_check_counter >= timer_check_interval) {
      rte_timer_manage();
      timer_check_counter = 0;
    }
  }

  save_all_cache_to_sqlite();
  free_dns_cache();

  net_port_close(port_in);
  net_port_close(port_out);
  net_port_close(port_exception);

  net_port_destroy(port_in);
  net_port_destroy(port_out);
  net_port_destroy(port_exception);
  return 0;
}