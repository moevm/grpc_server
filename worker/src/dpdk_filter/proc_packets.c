#include "proc_packets.h"
#include "domain_cache.h"
#include "ip_cache.h"

extern bool worker_classify_domain(const char *domain,
                                   struct requested_classification *out_req);

const uint16_t LIST_EXCEPTION_PORTS[LEN_LIST_EXCEPTION_PORTS] = {22};

void package_sending_decision(bool solution_is_send, struct rte_mbuf *pkt,
                              struct net_port *port_out,
                              uint16_t queue_number) {
  if (solution_is_send) {
    struct rte_mbuf *tx_pkt[1] = {pkt};
    uint16_t ret = rte_eth_tx_burst(port_out->port_id, queue_number, tx_pkt, 1);

    if (ret < 1) {
      LOG_ERROR("Failed to send packet");
      // PLUG (to be added later) - need to add processing for this case
      rte_pktmbuf_free(pkt);
    }
    return;
  }
  rte_pktmbuf_free(pkt);
}

bool check_is_exception(uint16_t *port) {
  for (int i = 0; i < LEN_LIST_EXCEPTION_PORTS; i++) {
    if (*port == LIST_EXCEPTION_PORTS[i]) {
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
      struct node_cache_ip *cached_node_ip = NULL;

      if (check_is_exception(&info_pac.number_port) == true) {
        package_sending_decision(true, pkts[i], port_exception, queue_number);
        continue;
      }

      int ret;
      if (info_pac.ip_version == IP_4) {
        ret = lookup_dns_cache(info_pac.ip4_dist, &cached_node_ip);
      } else {
        ret = lookup_dns_cache(info_pac.ip6_dist, &cached_node_ip);
      }

      if (ret >= 0 && cached_node_ip) {
        package_sending_decision(cached_node_ip->solution_is_send, pkts[i],
                                 port_out, queue_number);
      } else if (ret == -ENOENT) {

        struct requested_classification req_clas; // query to ip controller

        bool solution_is_send =
            main_filtring_by_ip(&req_clas, policy, info_pac);

        package_sending_decision(solution_is_send, pkts[i], port_out,
                                 queue_number);

        struct node_cache_ip *new_node =
            rte_calloc("struct_node_cache_ip", 1, sizeof(struct node_cache_ip),
                       RTE_CACHE_LINE_SIZE);
        if (!new_node) {
          printf(
              "[ERROR] Failed to allocate memory for struct node_cache_ip\n");
          continue;
        }

        new_node->solution_is_send = solution_is_send;

        add_to_dns_cache(info_pac.domain, new_node);

      } else {
        printf(
            "[ERROR] Failed to search a key-value pair in the hash table: %s\n",
            strerror(-ret));
      }
    } else {
      printf("[INFO] Packet with dns request\n");
      struct node_cache_domain *cached_node_domain = NULL;

      if (check_is_exception(&info_pac.number_port) == true) {
        package_sending_decision(true, pkts[i], port_exception, queue_number);
        continue;
      }

      int ret = lookup_dns_cache(info_pac.domain, &cached_node_domain);

      if (ret >= 0 && cached_node_domain) {
        package_sending_decision(cached_node_domain->solution_is_send, pkts[i],
                                 port_out, queue_number);
      } else if (ret == -ENOENT) {

        struct requested_classification req_clas; // query to domain controller

        bool solution_is_send =
            main_filtring_by_domain(&req_clas, policy, info_pac);

        package_sending_decision(solution_is_send, pkts[i], port_out,
                                 queue_number);

        struct node_cache_domain *new_node =
            rte_calloc("struct_node_cache", 1, sizeof(struct node_cache_domain),
                       RTE_CACHE_LINE_SIZE);
        if (!new_node) {
          printf("[ERROR] Failed to allocate memory for struct node_cache\n");
          continue;
        }

        new_node->solution_is_send = solution_is_send;

        add_to_dns_cache(info_pac.domain, new_node);
      } else {
        printf(
            "[ERROR] Failed to search a key-value pair in the hash table: %s\n",
            strerror(-ret));
      }
    }
  }
}
