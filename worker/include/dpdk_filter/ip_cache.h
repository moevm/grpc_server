#ifndef IP_HASH_H
#define IP_HASH_H

#include <rte_cycles.h>
#include <rte_errno.h>
#include <rte_hash.h>
#include <rte_jhash.h>
#include <rte_malloc.h>
#include <rte_timer.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>

#include "constants.h"
#include "types.h"

void init_ip_cache(void);
void free_ip_cache(void);

int lookup_ip_cache(const struct ip_key *key,
                    struct node_cache_ip **return_node);
void add_to_ip_cache(const struct ip_key *key, struct node_cache_ip *node);

void init_tables_sqlite_ip_cache(void);
void load_cache_ip_from_sqlite(void);
void close_sqlite_cache_ip(void);
int save_single_node_ip_to_sqlite(const struct ip_key *key,
                                  struct node_cache_ip *node);
void *save_all_cache_ip_to_sqlite(void *arg);
void copy_data_from_hash_to_snapshot_ip(struct snapshot_ip *snapt);

#endif