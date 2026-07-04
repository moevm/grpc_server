#include "proc_packets.h"
#include "domain_cache.h"
#include "ip_cache.h"
#include <rte_icmp.h>
#include <stdatomic.h>

extern bool worker_classify(const char *type, const char *target,
                            struct requested_classification *out_req);

const uint16_t LIST_EXCEPTION_PORTS[LEN_LIST_EXCEPTION_PORTS] = {22};

void learn_neighbor_mac(struct net_port *port, struct rte_mbuf *pkt) {
  struct rte_ether_hdr *eth = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
  if (!port->neighbor_learned) {
    rte_ether_addr_copy(&eth->src_addr, &port->neighbor_mac);
    port->neighbor_learned = true;
    LOG_INFO("Learned neighbor MAC on %s: %02x:%02x:%02x:%02x:%02x:%02x",
             port->iface_name, port->neighbor_mac.addr_bytes[0],
             port->neighbor_mac.addr_bytes[1],
             port->neighbor_mac.addr_bytes[2],
             port->neighbor_mac.addr_bytes[3],
             port->neighbor_mac.addr_bytes[4],
             port->neighbor_mac.addr_bytes[5]);
  }
}

void rewrite_l2_and_forward(struct rte_mbuf *pkt, struct net_port *in_port,
                            struct net_port *out_port,
                            uint16_t queue_number) {
  struct rte_ether_hdr *eth = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);

  learn_neighbor_mac(in_port, pkt);

  rte_ether_addr_copy(&out_port->mac_addr, &eth->src_addr);
  if (out_port->neighbor_learned) {
    rte_ether_addr_copy(&out_port->neighbor_mac, &eth->dst_addr);
  }

  if (eth->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
    struct rte_ipv4_hdr *ip =
        (struct rte_ipv4_hdr *)((uint8_t *)eth + sizeof(struct rte_ether_hdr));
    if (ip->time_to_live > 1) {
      ip->time_to_live--;
      uint32_t cksum = rte_be_to_cpu_16(ip->hdr_checksum) + 0x0100;
      cksum = (cksum & 0xFFFF) + (cksum >> 16);
      ip->hdr_checksum = rte_cpu_to_be_16((uint16_t)cksum);
    } else {
      rte_pktmbuf_free(pkt);
      return;
    }
  }

  package_sending_decision(true, pkt, out_port, queue_number);
}

void dump_checksum_before_tx(struct rte_mbuf *pkt) {
  struct rte_ether_hdr *eth = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
  if (eth->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4))
    return;

  struct rte_ipv4_hdr *ip =
      (struct rte_ipv4_hdr *)((uint8_t *)eth + sizeof(struct rte_ether_hdr));
  void *l4 = (uint8_t *)ip + rte_ipv4_hdr_len(ip);
  uint16_t cksum = 0;
  const char *proto_name = "OTHER";

  if (ip->next_proto_id == IPPROTO_ICMP) {
    struct rte_icmp_hdr *icmp = (struct rte_icmp_hdr *)l4;
    cksum = icmp->icmp_cksum;
    proto_name = "ICMP";
  } else if (ip->next_proto_id == IPPROTO_TCP) {
    struct rte_tcp_hdr *tcp = (struct rte_tcp_hdr *)l4;
    cksum = tcp->cksum;
    proto_name = "TCP";
  } else if (ip->next_proto_id == IPPROTO_UDP) {
    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)l4;
    cksum = udp->dgram_cksum;
    proto_name = "UDP";
  }

  LOG_INFO("[DIAG] %s before tx_burst: cksum=0x%04x ip_cksum=0x%04x "
           "ol_flags=0x%lx",
           proto_name, rte_be_to_cpu_16(cksum),
           rte_be_to_cpu_16(ip->hdr_checksum),
           (unsigned long)pkt->ol_flags);
}

void package_sending_decision(bool solution_is_send, struct rte_mbuf *pkt,
                              struct net_port *port_out,
                              uint16_t queue_number) {
  if (solution_is_send) {
    dump_checksum_before_tx(pkt);
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
  uint16_t host_port = rte_be_to_cpu_16(*port);
  for (int i = 0; i < LEN_LIST_EXCEPTION_PORTS; i++) {
    if (host_port == LIST_EXCEPTION_PORTS[i]) {
      return true;
    }
  }
  return false;
}

void pakage_processing(struct net_port *port_in, struct net_port *port_out,
                       struct net_port *port_exception, uint16_t queue_number,
                       uint16_t nb_pkts, struct rte_mbuf **pkts,
                       struct BASE_POLICY *policy,
                       bool filtring_is_turned_off) {

  uint16_t nb_rx =
      rte_eth_rx_burst(port_in->port_id, queue_number, pkts, nb_pkts);

  if (nb_rx > 0) {
    LOG_INFO("Received %hu packets on queue %hu", nb_rx, queue_number);
  }
  if (atomic_load(&filtring_is_turned_off)) {
    for (int i = 0; i < nb_rx; i++) {
      rewrite_l2_and_forward(pkts[i], port_in, port_out, queue_number);
    }
    return;
  }

  for (int i = 0; i < nb_rx; i++) {

    struct info_of_pakage info_pac;
    memset(&info_pac, 0, sizeof(info_pac));

    parsing_pakage(pkts[i], &info_pac);

    if (info_pac.ip_version != IP_4 && info_pac.ip_version != IP_6) {
      LOG_INFO("Non-IP packet (ARP/other), forwarding without filtering");
      package_sending_decision(true, pkts[i], port_out, queue_number);
      continue;
    }

    LOG_INFO("[PKT] port = %hu", ntohs(info_pac.number_port));
    if (info_pac.domain[0] == '\0') {
      LOG_INFO("Packet without dns request");
      struct node_cache_ip *cached_node_ip = NULL;

      if (check_is_exception(&info_pac.number_port) == true) {
        LOG_INFO("Exception port %hu, forwarding to exception port",
                 ntohs(info_pac.number_port));
        package_sending_decision(true, pkts[i], port_exception, queue_number);
        continue;
      }

      int ret;
      struct ip_key key;
      if (info_pac.ip_version == IP_4) {
        key.version = 4;
        key.addr.ip4 = info_pac.ip4_dist;
        ret = lookup_ip_cache(&key, &cached_node_ip);
      } else {
        key.version = 6;
        memcpy(key.addr.ip6, info_pac.ip6_dist, 16);
        ret = lookup_ip_cache(&key, &cached_node_ip);
      }

      if (ret >= 0 && cached_node_ip) {
        LOG_INFO("IP cache hit, decision: %s",
                 cached_node_ip->solution_is_send ? "send" : "drop");
        if (cached_node_ip->solution_is_send) {
          rewrite_l2_and_forward(pkts[i], port_in, port_out, queue_number);
        } else {
          rte_pktmbuf_free(pkts[i]);
        }
      } else if (ret == -ENOENT) {
        LOG_INFO("IP cache miss, applying filter");

        struct requested_classification req_clas; // query to ip controller

        bool solution_is_send;

        char ip_str[INET6_ADDRSTRLEN] = {0};
        if (info_pac.ip_version == IP_4) {
          struct in_addr addr;
          addr.s_addr = info_pac.ip4_dist;
          inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));
        } else {
          struct in6_addr addr;
          memcpy(&addr, info_pac.ip6_dist, 16);
          inet_ntop(AF_INET6, &addr, ip_str, sizeof(ip_str));
        }

        bool classification_success = worker_classify("ip", ip_str, &req_clas);

        if (classification_success) {
          solution_is_send = main_filtring_by_ip(&req_clas, policy, &info_pac);
        } else {
          solution_is_send = true;
          LOG_WARNING("Classification failed for IP %s", ip_str);
        }

        if (solution_is_send) {
          rewrite_l2_and_forward(pkts[i], port_in, port_out, queue_number);
        } else {
          rte_pktmbuf_free(pkts[i]);
        }

        struct node_cache_ip *new_node =
            rte_calloc("struct_node_cache_ip", 1, sizeof(struct node_cache_ip),
                       RTE_CACHE_LINE_SIZE);
        if (!new_node) {
          LOG_ERROR("Failed to allocate memory for struct node_cache_ip");
          continue;
        }

        new_node->solution_is_send = solution_is_send;

        struct ip_key key;
        if (info_pac.ip_version == IP_4) {
          key.version = 4;
          key.addr.ip4 = info_pac.ip4_dist;
          add_to_ip_cache(&key, new_node, policy->ttl_ip);
        } else {
          key.version = 6;
          memcpy(key.addr.ip6, info_pac.ip6_dist, 16);
          add_to_ip_cache(&key, new_node, policy->ttl_ip);
        }

      } else {
        LOG_ERROR("Failed to search a key-value pair in the hash table: %s",
                  strerror(-ret));
      }
    } else {
      LOG_INFO("[INFO] Packet with dns request");
      struct node_cache_domain *cached_node_domain = NULL;

      if (check_is_exception(&info_pac.number_port) == true) {
        LOG_INFO("Exception port %hu, forwarding to exception port",
                 ntohs(info_pac.number_port));
        package_sending_decision(true, pkts[i], port_exception, queue_number);
        continue;
      }

      int ret = lookup_dns_cache(info_pac.domain, &cached_node_domain);

      if (ret >= 0 && cached_node_domain) {
        LOG_INFO("Domain cache hit for '%s', decision: %s", info_pac.domain,
                 cached_node_domain->solution_is_send ? "send" : "drop");
        if (cached_node_domain->solution_is_send) {
          rewrite_l2_and_forward(pkts[i], port_in, port_out, queue_number);
        } else {
          rte_pktmbuf_free(pkts[i]);
        }
      } else if (ret == -ENOENT) {
        LOG_INFO("Domain cache miss for '%s', applying filter",
                 info_pac.domain);

        struct requested_classification req_clas; // query to domain controller

        bool solution_is_send;
        bool classification_success =
            worker_classify("domain", info_pac.domain, &req_clas);

        if (classification_success) {
          solution_is_send =
              main_filtring_by_domain(&req_clas, policy, &info_pac);
        } else {
          solution_is_send = true;
          LOG_WARNING("Classification failed for %s", info_pac.domain);
        }

        if (solution_is_send) {
          rewrite_l2_and_forward(pkts[i], port_in, port_out, queue_number);
        } else {
          rte_pktmbuf_free(pkts[i]);
        }

        struct node_cache_domain *new_node =
            rte_calloc("struct_node_cache", 1, sizeof(struct node_cache_domain),
                       RTE_CACHE_LINE_SIZE);
        if (!new_node) {
          LOG_ERROR("Failed to allocate memory for struct node_cache");
          continue;
        }

        new_node->solution_is_send = solution_is_send;

        add_to_dns_cache(info_pac.domain, new_node, policy->ttl_domain);
      } else {
        LOG_ERROR("Failed to search a key-value pair in the hash table: %s",
                  strerror(-ret));
      }
    }
  }
}
