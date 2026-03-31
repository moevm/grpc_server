#include "../../include/dpdk_filter/dns_cache.h"

static struct rte_hash *dns_hash;
static struct rte_hash_parameters hash_params = {
    .name = "dns_cache_hash",
    .entries = CACHE_SIZE,
    .key_len = DOMAIN_MAX_LEN,
    .hash_func = rte_jhash,
};

void init_dns_cache(void) {
  if (dns_hash)
    return;

  dns_hash = rte_hash_create(&hash_params);
  if (!dns_hash) {
    printf("[ERROR] Failed to create DNS cache hash table\n");
  }
}

int lookup_dns_cache(const char *domain, struct node_cache **return_node) {
  int ret = rte_hash_lookup_data(dns_hash, domain, (void **)return_node);
  return ret;
}

void add_to_dns_cache(const char *domain, struct node_cache *node) {
  int ret = rte_hash_add_key_data(dns_hash, domain, node);
  if (ret) {
    printf("[ERROR] Failed to add key data in hash table\n");
  }
}

void free_dns_cache(void) {
  if (!dns_hash)
    return;

  struct node_cache *node;
  uint32_t next = 0;
  void *key;
  void *data;

  while (rte_hash_iterate(dns_hash, &key, &data, &next) >= 0) {
    if (data) {
      free(data);
    }
  }

  rte_hash_free(dns_hash);
  dns_hash = NULL;
}