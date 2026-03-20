#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../../include/dpdk_filter/dns_parser.h"
#include "../../include/dpdk_filter/dns_cache.h"

static const char* blocked_domains[] = { // пока как заглушка вместо политики воркера
    "facebook.com",
    "youtube.com",
    "instagram.com",
};


static int is_domain_blocked(const char* domain) {
    for (int i = 0; blocked_domains[i] != NULL; i++) {
        if (strstr(domain, blocked_domains[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

void extract_dns_domain(struct rte_mbuf* pkt, char* domain, int max_len) {
    printf("\nThe package has arrived");
    struct rte_ether_hdr* eth_hdr;
    struct rte_ipv4_hdr* ip_hdr;
    struct rte_udp_hdr* udp_hdr;
    int dns_hdr = 12;
    uint8_t* dns_data;
    
    eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
    

    uint16_t ether_type_host = rte_be_to_cpu_16(eth_hdr->ether_type);

    switch(ether_type_host) {
        case RTE_ETHER_TYPE_IPV4:
            printf("(IPv4)\n");
            break;
        case RTE_ETHER_TYPE_IPV6:
            printf("(IPv6)\n");
            domain[0] = '\0';
            return;
        case RTE_ETHER_TYPE_ARP:
            printf("(ARP)\n");
            domain[0] = '\0';
            return;
        case 0x8100:
            printf("(VLAN)\n");
            domain[0] = '\0';
            return;
        default:
            printf("(Unknown): %u (0x%04x)\n", ether_type_host, ether_type_host);
            domain[0] = '\0';
            return;
    }

    ip_hdr = (struct rte_ipv4_hdr* )(eth_hdr + 1);
    
    if (ip_hdr->next_proto_id != IPPROTO_UDP) {
        domain[0] = '\0';
        printf("\n      not udp\n");

        return;
    }
    
    udp_hdr = (struct rte_udp_hdr* )((uint8_t* )ip_hdr + ((ip_hdr->version_ihl) & 0x0f) * 4);
    
    if (rte_be_to_cpu_16(udp_hdr->dst_port) != 53) {
        domain[0] = '\0';
        printf("\n      not 53 port\n");

        return;
    }
    
    dns_data = (uint8_t* )(udp_hdr + 1);

    uint16_t flags = (dns_data[2] << 8) | dns_data[3];
    int is_query = ((flags & 0x8000) == 0);
    if (!is_query) {
        domain[0] = '\0';
        printf("\n      it is not request\n");

        return;
    }

    uint8_t* qname = dns_data + dns_hdr;
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
    

    int cached_result;
    if (lookup_dns_cache(domain, &cached_result)) {
        if (cached_result) {
            printf("\n[CACHE HIT] block: %s\n", domain);
        } else {
            printf("\n[CACHE HIT] allow: %s\n", domain);
        }
        return;
    }
    
    int is_blocked = is_domain_blocked(domain);
    if (is_blocked) {
        printf("\n block: %s\n", domain);
    } else {
        printf("\n allow: %s\n", domain);
    }
    
    add_to_dns_cache(domain, is_blocked);

}