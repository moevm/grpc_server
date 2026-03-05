#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../../include/dpdk_filter/dns_parser.h"

static const char *blocked_domains[] = { // пока как заглушка вместо политики воркера
    "facebook.com",
    "youtube.com",
    "instagram.com",
};


static int is_domain_blocked(const char *domain) {
    for (int i = 0; blocked_domains[i] != NULL; i++) {
        if (strstr(domain, blocked_domains[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

void extract_dns_domain(struct rte_mbuf *pkt, char *domain, int max_len) {
    struct rte_ether_hdr *eth_hdr;
    struct rte_ipv4_hdr *ip_hdr;
    struct rte_udp_hdr *udp_hdr;
    int dns_hdr = 12;
    uint8_t *dns_data;
    
    eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
    
    if (rte_be_to_cpu_16(eth_hdr->ether_type) != RTE_ETHER_TYPE_IPV4) {
        domain[0] = '\0';
        return;
    }
    
    ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
    
    if (ip_hdr->next_proto_id != IPPROTO_UDP) {
        domain[0] = '\0';
        return;
    }
    
    udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ip_hdr + ((ip_hdr->version_ihl) & 0x0f) * 4);
    
    if (rte_be_to_cpu_16(udp_hdr->dst_port) != 53) {
        domain[0] = '\0';
        return;
    }
    
    dns_data = (uint8_t *)(udp_hdr + 1);
    uint8_t *qname = dns_data + dns_hdr;
    int pos = 0;
    
    while (*qname != 0 && pos < max_len - 1) {
        uint8_t label_len = *qname;
        qname++;
        
        for (int i = 0; i < label_len && pos < max_len - 1; i++) {
            domain[pos++] = *qname++;
        }
        
        if (*qname != 0 && pos < max_len - 1) {
            domain[pos++] = '.';
        }
    }
    
    domain[pos] = '\0';
    
    if (is_domain_blocked(domain)) {
        printf("\n block: %s\n", domain);
    } else {
        printf("\n allow: %s\n", domain);
    }
}