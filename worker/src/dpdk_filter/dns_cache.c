#include "../../include/dpdk_filter/dns_cache.h"

static struct rte_hash* dns_hash;
static struct rte_hash_parameters hash_params = {
    .name = "dns_cache_hash",
    .entries = CACHE_SIZE,
    .key_len = DOMAIN_MAX_LEN,
    .hash_func = rte_jhash,
};

void init_dns_cache(void) {
    dns_hash = rte_hash_create(&hash_params);
    if (!dns_hash) {
        printf("Failed to create DNS cache hash table\n");
    }
}

int lookup_dns_cache(const char* domain, int* is_blocked) {
    int ret = rte_hash_lookup_data(dns_hash, domain, (void **)is_blocked);
    return (ret >= 0);
}

void add_to_dns_cache(const char* domain, int is_blocked) {
    int* value = rte_malloc("dns_cache_value", sizeof(int), 0);
    *value = is_blocked;
    rte_hash_add_key_data(dns_hash, domain, value);
}
