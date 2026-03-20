#ifndef DNS_PARSER_H
#define DNS_PARSER_H

#include <rte_mbuf.h>

void extract_dns_domain(struct rte_mbuf *pkt, char *domain, int max_len);

#endif