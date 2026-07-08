#include "proc_packets.h"
#include "domain_cache.h"
#include "ip_cache.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

extern void record_packet_received();
extern void record_packet_passed();
extern void record_packet_dropped();

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

void forward_packet_with_rewrite(struct rte_mbuf *pkt,
                                               struct net_port *in_port,
                                               struct net_port *out_port,
                                               uint16_t queue_number) {
    struct rte_ether_hdr *eth = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);

    // 1. Учим MAC-адрес соседа на входном порте
    learn_neighbor_mac(in_port, pkt);

    // 2. Source MAC = MAC выходного порта
    rte_ether_addr_copy(&out_port->mac_addr, &eth->src_addr);

    // 3. Destination MAC = MAC соседа на выходном порте (если выучен)
    if (out_port->neighbor_learned) {
        rte_ether_addr_copy(&out_port->neighbor_mac, &eth->dst_addr);
    } else {
        // Если не выучен — отправляем широковещательный пакет
        struct rte_ether_addr broadcast = { .addr_bytes = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff} };
        rte_ether_addr_copy(&broadcast, &eth->dst_addr);
        LOG_WARNING("Neighbor MAC not learned yet on %s, using broadcast", out_port->iface_name);
    }

    // 4. Если это IPv4 — обновить TTL и IP-контрольную сумму
    if (eth->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
        struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t *)eth + sizeof(struct rte_ether_hdr));

        if (ip->time_to_live > 1) {
            ip->time_to_live--;  // Уменьшаем TTL
            ip->hdr_checksum = 0;
            ip->hdr_checksum = rte_ipv4_cksum(ip);  // Пересчитываем IP-контрольную сумму
        } else {
            rte_pktmbuf_free(pkt);  // TTL = 0 — дроп
            return;
        }
    }

    // 5. Отправить пакет
    struct rte_mbuf *tx_pkt[1] = {pkt};
    uint16_t ret = rte_eth_tx_burst(out_port->port_id, queue_number, tx_pkt, 1);
    if (ret < 1) {
        LOG_ERROR("Failed to send packet");
        record_packet_dropped();
        rte_pktmbuf_free(pkt);
    }
    record_packet_passed();
}

void package_sending_decision(bool solution_is_send, struct rte_mbuf *pkt,
                              struct net_port *port_in, struct net_port *port_out,
                              uint16_t queue_number) {
  if (solution_is_send) {
    // struct rte_mbuf *tx_pkt[1] = {pkt};
    forward_packet_with_rewrite(pkt, port_in, port_out, queue_number);
    // uint16_t ret = rte_eth_tx_burst(port_out->port_id, queue_number, tx_pkt, 1);
    return;
  }

  record_packet_dropped();
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
      record_packet_received();
      package_sending_decision(true, pkts[i], port_in, port_out, queue_number);
    }
    return;
  }

  for (int i = 0; i < nb_rx; i++) {

    record_packet_received();

    struct info_of_pakage info_pac;
    memset(&info_pac, 0, sizeof(info_pac));

    parsing_pakage(pkts[i], &info_pac);
    LOG_INFO("[PKT] port = %hu", ntohs(info_pac.number_port));

    if (info_pac.domain[0] == '\0') {
      LOG_INFO("Packet without dns request");
      struct node_cache_ip *cached_node_ip = NULL;

      if (check_is_exception(&info_pac.number_port) == true) {
        LOG_INFO("Exception port %hu, forwarding to exception port",
                 ntohs(info_pac.number_port));
        package_sending_decision(true, pkts[i], port_in, port_exception, queue_number);
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
        package_sending_decision(cached_node_ip->solution_is_send, pkts[i], port_in, 
                                 port_out, queue_number);
      } else if (ret == -ENOENT) {
        LOG_INFO("IP cache miss, applying filter");

        struct requested_classification req_clas;

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

        package_sending_decision(solution_is_send, pkts[i], port_in, port_out,
                                 queue_number);

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
        record_packet_dropped();
        rte_pktmbuf_free(pkts[i]);
      }
    } else {
      LOG_INFO("[INFO] Packet with dns request");
      struct node_cache_domain *cached_node_domain = NULL;

      if (check_is_exception(&info_pac.number_port) == true) {
        LOG_INFO("Exception port %hu, forwarding to exception port",
                 ntohs(info_pac.number_port));
        package_sending_decision(true, pkts[i], port_in, port_exception, queue_number);
        continue;
      }

      int ret = lookup_dns_cache(info_pac.domain, &cached_node_domain);

      if (ret >= 0 && cached_node_domain) {
        LOG_INFO("Domain cache hit for '%s', decision: %s", info_pac.domain,
                 cached_node_domain->solution_is_send ? "send" : "drop");
        package_sending_decision(cached_node_domain->solution_is_send, pkts[i], port_in,
                                 port_out, queue_number);
      } else if (ret == -ENOENT) {
        LOG_INFO("Domain cache miss for '%s', applying filter",
                 info_pac.domain);

        struct requested_classification req_clas;

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

        package_sending_decision(solution_is_send, pkts[i], port_in, port_out,
                                 queue_number);

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
        record_packet_dropped();
        rte_pktmbuf_free(pkts[i]);
      }
    }
  }
}