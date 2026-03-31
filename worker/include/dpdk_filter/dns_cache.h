#ifndef DNS_HASH_H
#define DNS_HASH_H

#include <rte_hash.h>
#include <rte_jhash.h>
#include <rte_malloc.h>
#include <stdint.h>

#define CACHE_SIZE 1024
#define DOMAIN_MAX_LEN 260
#define MAX_CATEGORIES 100
#define CATEGORY_MAX_LEN 64

struct node_cache {
  char categories[MAX_CATEGORIES][CATEGORY_MAX_LEN];
  bool solution_is_send;
};

void init_dns_cache(void);
int lookup_dns_cache(const char *domain, struct node_cache **return_node);
void add_to_dns_cache(const char *domain, struct node_cache *node);
void free_dns_cache(void);

#endif