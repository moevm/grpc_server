#ifndef DOMAIN_HASH_H
#define DOMAIN_HASH_H

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

void init_dns_cache(void);
void free_dns_cache(void);
void clear_dns_cache(void);

int lookup_dns_cache(const char *domain,
                     struct node_cache_domain **return_node);
void add_to_dns_cache(const char *domain, struct node_cache_domain *node);

void init_tables_sqlite_dns_cache(void);
void load_cache_from_sqlite(void);
void close_sqlite_cache(void);
int save_single_node_to_sqlite(const char *domain,
                               struct node_cache_domain *node);
void *save_all_cache_to_sqlite(void *arg);
void copy_data_from_hash_to_snapshot(struct snapshot_domain *snapt);

#endif