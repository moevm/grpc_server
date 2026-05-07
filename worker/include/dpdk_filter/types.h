#ifndef TYPES_H
#define TYPES_H

#include "constants.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef DEBUG
#define LOG_INFO(info, ...)                                                    \
  fprintf(stderr, "[INFO] %s: %d: " info "\n", __func__, __LINE__,             \
          ##__VA_ARGS__)

#define LOG_ERROR(error, ...)                                                  \
  fprintf(stderr, "[ERROR] %s: %d: " error "\n", __func__, __LINE__,           \
          ##__VA_ARGS__)

#define LOG_WARNING(warning, ...)                                              \
  fprintf(stderr, "[WARNING] %s: %d: " warning "\n", __func__, __LINE__,       \
          ##__VA_ARGS__)

#else
#define LOG_INFO(info, ...)                                                    \
  do {                                                                         \
  } while (0)

#define LOG_ERROR(error, ...)                                                  \
  fprintf(stderr, "[ERROR] %s: %d: " error "\n", __func__, __LINE__,           \
          ##__VA_ARGS__)

#define LOG_WARNING(warning, ...)                                              \
  fprintf(stderr, "[WARNING] %s: %d: " warning "\n", __func__, __LINE__,       \
          ##__VA_ARGS__)

#endif

enum ip_version { IP_4 = 1, IP_6 = 2 };

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
  uint8_t ip_version;
  uint32_t ip4_src;
  uint32_t ip4_dist;
  uint8_t ip6_src[IP6_LEN];
  uint8_t ip6_dist[IP6_LEN];
};

struct trust_categories_with_lvl {
  char locked_by_trust_category[CATEGORY_MAX_LEN];
  int trust_lvl;
};

struct BASE_POLICY {
  char locked_categories[MAX_CATEGORIES][CATEGORY_MAX_LEN];
  struct trust_categories_with_lvl
      categories_with_lvl[MAX_CATEGORIES_BY_TRUST_LVL];
  char block_domains[MAX_DOMAINS][DOMAIN_MAX_LEN];
  char allow_domains[MAX_DOMAINS][DOMAIN_MAX_LEN];
  uint32_t block_ip4[MAX_IP4];
  uint32_t allow_ip4[MAX_IP4];
  uint8_t block_ip6[MAX_IP6][IP6_LEN];
  uint8_t allow_ip6[MAX_IP6][IP6_LEN];
  int min_trust_level;
};

struct requested_classification {
  char get_categories[MAX_CATEGORIES][CATEGORY_MAX_LEN];
  int get_trust_level;
};

struct node_cache_domain {
  char categories[MAX_CATEGORIES][CATEGORY_MAX_LEN];
  bool solution_is_send;
  int trust_lvl;
  uint64_t timestamp;
  uint32_t ttl_seconds;
  enum ip_version ip;
  uint32_t ip4_src;
  uint8_t ip6_src[IP6_LEN];
  char *key_domain;
};

struct snapshot_domain {
  struct node_cache_domain node;
  char domain[DOMAIN_MAX_LEN];
};

struct node_cache_ip {
  char categories[MAX_CATEGORIES][CATEGORY_MAX_LEN];
  bool solution_is_send;
  int trust_lvl;
  uint64_t timestamp;
  uint32_t ttl_seconds;
  struct ip_key *key;
};

struct ip_key {
  enum ip_version version;
  union {
    uint32_t ip4;
    uint8_t ip6[IP6_LEN];
  } addr;
};

struct snapshot_ip {
  struct node_cache_ip node;
  struct ip_key key;
};

enum load_result { LOAD_OK = 0, LOAD_EXPIRED = 1, LOAD_ERROR = -1 };

#endif
