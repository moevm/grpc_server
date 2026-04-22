#ifndef DNS_HASH_H
#define DNS_HASH_H

#include <rte_cycles.h>
#include <rte_errno.h>
#include <rte_hash.h>
#include <rte_jhash.h>
#include <rte_malloc.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>

#include "../../include/dpdk_filter/constants.h"
#include "../../include/dpdk_filter/types.h"

void init_dns_cache(void);
int lookup_dns_cache(const char *domain, struct node_cache **return_node);
void add_to_dns_cache(const char *domain, struct node_cache *node);
void free_dns_cache(void);

#endif