#include "../../include/dpdk_filter/proc_packets.h"
#include "../../include/dpdk_filter/dns_cache.h"

const uint16_t LIST_EXCEPTION_PORTS[LEN_LIST_EXCEPTION_PORTS] = {22};

void package_sending_decision(bool solution_is_send, struct rte_mbuf *pkt,
                              struct net_port *port_out,
                              uint16_t queue_number) {
  if (solution_is_send) {
    struct rte_mbuf *tx_pkt[1] = {pkt};
    uint16_t ret = rte_eth_tx_burst(port_out->port_id, queue_number, tx_pkt, 1);

    if (ret < 1) {
      printf("[ERROR] Failed to send packet\n");
      // PLUG (to be added later) - need to add processing for this case
      rte_pktmbuf_free(pkt);
    }
    return;
  }
  rte_pktmbuf_free(pkt);
}

bool check_is_exception(uint16_t number_port) {
  for (int i = 0; i < LEN_LIST_EXCEPTION_PORTS; i++) {
    if (number_port == LIST_EXCEPTION_PORTS[i]) {
      return true;
    }
  }
  return false;
}

void pakage_processing(struct net_port *port_in, struct net_port *port_out,
                       struct net_port *port_exception, uint16_t queue_number,
                       uint16_t nb_pkts, struct rte_mbuf **pkts,
                       struct BASE_POLICY *policy) {

  uint16_t nb_rx =
      rte_eth_rx_burst(port_in->port_id, queue_number, pkts, nb_pkts);

  for (int i = 0; i < nb_rx; i++) {

    struct info_of_pakage info_pac;
    memset(&info_pac, 0, sizeof(info_pac));

    parsing_pakage(pkts[i], &info_pac);
    printf("[PKT] port = %hu; domain = %s\n", ntohs(info_pac.number_port),
           info_pac.domain);
    if (info_pac.domain[0] == '\0') {
      printf("[INFO] Packet without dns request\n");
      package_sending_decision(true, pkts[i], port_out, queue_number);
      continue;
    }

    if (check_is_exception(info_pac.number_port) == true) {
      package_sending_decision(true, pkts[i], port_exception, queue_number);
      continue;
    }

    struct node_cache *cached_node = NULL;
    int ret = lookup_dns_cache(info_pac.domain, &cached_node);

    if (ret >= 0 && cached_node) {

      package_sending_decision(cached_node->solution_is_send, pkts[i], port_out,
                               queue_number);
    } else if (ret == -ENOENT) {

      struct requested_classification req_clas;

      bool solution_is_send = main_filtring(&req_clas, policy, info_pac.domain);

      package_sending_decision(solution_is_send, pkts[i], port_out,
                               queue_number);

      struct node_cache *new_node =
          rte_calloc("struct_node_cache", 1, sizeof(struct node_cache),
                     RTE_CACHE_LINE_SIZE);
      if (!new_node) {
        printf("[ERROR] Failed to allocate memory for struct node_cache\n");
        continue;
      }
      new_node->solution_is_send = solution_is_send;
      // NEED TO FILL THE STRUCTURE WITH CATEGORIES
      add_to_dns_cache(info_pac.domain, new_node);
    } else {
      printf(
          "[ERROR] Failed to search a key-value pair in the hash table: %s\n",
          strerror(-ret));
    }
  }
}
