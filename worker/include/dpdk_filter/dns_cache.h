#ifndef DNS_HASH_H
#define DNS_HASH_H

#include <rte_hash.h>
#include <stdint.h>
#include <rte_jhash.h>
#include <rte_malloc.h>


#define CACHE_SIZE 1024
#define DOMAIN_MAX_LEN 256

void init_dns_cache(void);
int lookup_dns_cache(const char *domain, int *is_blocked);
void add_to_dns_cache(const char *domain, int is_blocked);
void cleanup_dns_cache(void);

#endif