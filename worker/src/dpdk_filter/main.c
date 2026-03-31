#include "../../include/dpdk_filter/af_xdp_port.h"
#include "../../include/dpdk_filter/proc_packets.h"
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <stdio.h>
#include "../../include/dpdk_filter/af_xdp_port.h"
#include "../../include/dpdk_filter/dns_cache.h"
#include "../../include/dpdk_filter/dns_parser.h"
#include <unistd.h>
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

int main(int argc, char **argv) {
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    printf("[ERROR] Failed to set SIGINT handler\n");
    return 1;
  }
  if (signal(SIGTERM, signal_handler) == SIG_ERR) {
    printf("[ERROR] Failed to set SIGTERM handler\n");
    return 1;
  }
  struct af_xdp_port *port_in = NULL;
  struct af_xdp_port *port_out = NULL;
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

#ifdef VIRT_PORTS
  printf("Using virtual ports: veth0/veth1\n");
  port_in = init_struct_af_xdp_port("veth0", mbuf_pool);
  port_out = init_struct_af_xdp_port("veth1", mbuf_pool);
#else
  printf("Using real ports: eth0/eth1\n");
  port_in = init_struct_af_xdp_port("eth0", mbuf_pool);
  port_out = init_struct_af_xdp_port("eth1", mbuf_pool);
#endif
  if (!port_in || !port_out) {
    return 1;
  }

  if (af_xdp_port_init(port_in) || af_xdp_port_init(port_out)) {
    return 1;
  }

  if (af_xdp_port_start(port_in->port_id) ||
      af_xdp_port_start(port_out->port_id)) {
    return 1;
  }

  printf("An endless cycle has been started. Packets pass from port with id=%u "
         "to port with id=%u\n",
         port_in->port_id, port_out->port_id);

  while (running) {

    pakage_processing(port_in, port_out, queue_number, nb_pkts, pkts);
  }

  af_xdp_port_close(port_in);
  af_xdp_port_close(port_out);

  af_xdp_port_destroy(port_in);
  af_xdp_port_destroy(port_out);
  return 0;
}