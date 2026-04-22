#include "dns_cache.h"

static struct rte_hash *dns_hash;
static struct rte_hash_parameters hash_params = {
    .name = "dns_cache_hash",
    .entries = CACHE_SIZE,
    .key_len = DOMAIN_MAX_LEN,
    .hash_func = rte_jhash,
    .extra_flag = RTE_HASH_EXTRA_FLAGS_EXT_TABLE,
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

  if (ret >= 0 && *return_node) {
    uint64_t now = rte_get_timer_cycles();
    uint64_t hz = rte_get_timer_hz();
    uint64_t age_seconds = (now - (*return_node)->timestamp) / hz;

    if (age_seconds >= (*return_node)->ttl_seconds) {

      int ret_del = rte_hash_del_key(dns_hash, domain);
      if (ret_del < 0) {
        printf("[ERROR] Failed to deleting an obsolete hashtable value\n");
        return -ENOENT;
      }
      rte_free((*return_node)->key_domain);
      rte_free(*return_node);
      *return_node = NULL;

      return -ENOENT;
    }
  }
  return ret;
}

void add_to_dns_cache(const char *domain, struct node_cache *node) {
  char *key_copy = rte_malloc("dns_key(domain)", DOMAIN_MAX_LEN, 0);
  if (!key_copy) {
    printf("[ERROR] Failed to allocate memory for key cache\n");
    rte_free(node);
    return;
  }
  strncpy(key_copy, domain, DOMAIN_MAX_LEN);
  key_copy[DOMAIN_MAX_LEN - 1] = '\0';
  node->timestamp = rte_get_timer_cycles();
  node->ttl_seconds = DNS_CACHE_DEFAULT_TTL;
  node->key_domain = key_copy;

  int ret = rte_hash_add_key_data(dns_hash, key_copy, node);
  if (ret) {
    printf("[ERROR] Failed to add key data in hash table\n");
    rte_free(key_copy);
    rte_free(node);
  }
}

void free_dns_cache(void) {
  if (!dns_hash)
    return;

  uint32_t next = 0;
  const void *key;
  void *data;

  while (rte_hash_iterate(dns_hash, &key, &data, &next) >= 0) {

    if (data) {
      struct node_cache *node = (struct node_cache *)data;
      if (node->key_domain) {
        rte_free(node->key_domain);
      }
      rte_free(node);
    }
  }

  rte_hash_free(dns_hash);
  dns_hash = NULL;
}