#ifndef TYPES_H
#define TYPES_H

#include "constants.h"
#include <stdint.h>
#include <stdbool.h>

struct net_port {
  uint16_t port_id;
  char iface_name[32];
  char dev_name[64];
  char dev_args[256];
  struct rte_mempool *mbuf_pool;
};
struct info_of_pakage {
  uint16_t ethernet_type_host;
  uint16_t ethernet_type_protocol;
  uint16_t number_port;
  char domain[DOMAIN_MAX_LEN];
};

struct trust_categories_with_lvl {
    char locked_by_trust_category[CATEGORY_MAX_LEN];
    int trust_lvl;
};

struct BASE_POLICY {
  char locked_categories[MAX_CATEGORIES][CATEGORY_MAX_LEN];
  struct trust_categories_with_lvl categories_with_lvl[MAX_CATEGORIES_BY_TRUST_LVL];
  char block_domains[MAX_DOMAINS][DOMAIN_MAX_LEN];
  char allow_domains[MAX_DOMAINS][DOMAIN_MAX_LEN];
  int min_trust_level;
};

struct requested_classification {
    char get_categories[MAX_CATEGORIES][CATEGORY_MAX_LEN];
    int get_trust_level;
};


struct node_cache {
  char categories[MAX_CATEGORIES][CATEGORY_MAX_LEN];
  bool solution_is_send;
  int trust_lvl;
  uint64_t timestamp;
  uint32_t ttl_seconds;
  char *key_domain;
};


#endif
