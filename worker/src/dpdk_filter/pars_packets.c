#include "pars_packets.h"
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_net.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <stdlib.h>
#include <string.h>

void parsing_pakage(struct rte_mbuf *packet, struct info_of_pakage *info_pac) {

  struct rte_net_hdr_lens hdr_lens;
  uint32_t pkt_type = rte_net_get_ptype(packet, &hdr_lens, RTE_PTYPE_ALL_MASK);

  if (pkt_type == RTE_PTYPE_UNKNOWN) {
    printf("[ERROR PARS] Problem with get lens of headers");
    return;
  }

  struct rte_ether_hdr *eth_hdr =
      rte_pktmbuf_mtod(packet, struct rte_ether_hdr *);
  info_pac->ethernet_type_host = eth_hdr->ether_type;

  uint32_t l3_offset = hdr_lens.l2_len;

  if (pkt_type & RTE_PTYPE_L3_IPV4) {
    info_pac->ethernet_type_protocol = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

    struct rte_ipv4_hdr *ipv4_hdr =
        rte_pktmbuf_mtod_offset(packet, struct rte_ipv4_hdr *, l3_offset);

    if (pkt_type & RTE_PTYPE_L4_TCP) {
      struct rte_tcp_hdr *tcp_hdr = rte_pktmbuf_mtod_offset(
          packet, struct rte_tcp_hdr *, l3_offset + hdr_lens.l3_len);
      info_pac->number_port = tcp_hdr->dst_port;
    } else if (pkt_type & RTE_PTYPE_L4_UDP) {
      struct rte_udp_hdr *udp_hdr = rte_pktmbuf_mtod_offset(
          packet, struct rte_udp_hdr *, l3_offset + hdr_lens.l3_len);
      info_pac->number_port = udp_hdr->dst_port;
    }
  } else if (pkt_type & RTE_PTYPE_L3_IPV6) {
    info_pac->ethernet_type_protocol = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV6);

    if (pkt_type & RTE_PTYPE_L4_TCP) {
      struct rte_tcp_hdr *tcp_hdr = rte_pktmbuf_mtod_offset(
          packet, struct rte_tcp_hdr *, l3_offset + hdr_lens.l3_len);
      info_pac->number_port = tcp_hdr->dst_port;
    } else if (pkt_type & RTE_PTYPE_L4_UDP) {
      struct rte_udp_hdr *udp_hdr = rte_pktmbuf_mtod_offset(
          packet, struct rte_udp_hdr *, l3_offset + hdr_lens.l3_len);
      info_pac->number_port = udp_hdr->dst_port;
    }
  }

  if ((pkt_type & RTE_PTYPE_L4_UDP) &&
      info_pac->number_port == rte_cpu_to_be_16(53)) {
    uint32_t l4_offset = l3_offset + hdr_lens.l3_len + hdr_lens.l4_len;
    uint8_t *udp_payload =
        rte_pktmbuf_mtod_offset(packet, uint8_t *, l4_offset);

    uint32_t len_dns_header = 12;
    if (rte_pktmbuf_data_len(packet) > l4_offset + len_dns_header) {
      uint8_t *dns_name_start = udp_payload + len_dns_header;
      uint8_t *pos = dns_name_start;
      size_t domain_len = 0;

      while (*pos != 0 && domain_len < 255) {
        domain_len += *pos + 1;
        pos += *pos + 1;
      }

      if (*pos == 0 && domain_len > 0) {

        pos = dns_name_start;
        char *dst = info_pac->domain;
        while (*pos != 0) {
          uint8_t label_len = *pos++;
          if (dst != info_pac->domain)
            *dst++ = '.';
          memcpy(dst, pos, label_len);
          dst += label_len;
          pos += label_len;
        }
        *dst = '\0';
      }
    }
  }
}
