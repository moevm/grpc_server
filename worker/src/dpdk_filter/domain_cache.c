#include "domain_cache.h"
#include <pthread.h>
#include <rte_spinlock.h>
#include <stdlib.h>

static sqlite3 *cache_table;
static rte_spinlock_t cache_spinlock_domain = RTE_SPINLOCK_INITIALIZER;

static sqlite3 *cache_table;

static struct rte_hash *dns_hash;
static struct rte_hash_parameters hash_params = {
    .name = "dns_cache_hash",
    .entries = CACHE_SIZE,
    .key_len = DOMAIN_MAX_LEN,
    .hash_func = rte_jhash,
    .extra_flag = RTE_HASH_EXTRA_FLAGS_EXT_TABLE,
};
static struct rte_timer cache_save_timer;
static uint64_t save_interval_cycles;

static int insert_loaded_node_domain(const char *domain,
                                     struct node_cache_domain *node) {
  char *key_copy = rte_malloc("dns_key(domain)", DOMAIN_MAX_LEN, 0);
  if (!key_copy) {
    LOG_ERROR("Failed to allocate key for loaded node");
    return -ENOMEM;
  }

  strncpy(key_copy, domain, DOMAIN_MAX_LEN);
  key_copy[DOMAIN_MAX_LEN - 1] = '\0';
  node->key_domain = key_copy;

  rte_spinlock_lock(&cache_spinlock_domain);
  int ret = rte_hash_add_key_data(dns_hash, key_copy, node);
  rte_spinlock_unlock(&cache_spinlock_domain);

  if (ret < 0) {
    LOG_ERROR("Failed to insert loaded node into hash: %s", strerror(-ret));
    rte_free(key_copy);
    return ret;
  }
  return 0;
}

static int load_domain_categories(const char *domain,
                                  struct node_cache_domain *node) {
  const char *sql =
      "SELECT certain_category FROM categories_table WHERE domain = ?;";
  sqlite3_stmt *stmt = NULL;
  int rc = sqlite3_prepare_v2(cache_table, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    LOG_ERROR("Failed to prepare categories SELECT: %s",
              sqlite3_errmsg(cache_table));
    return -1;
  }

  sqlite3_bind_text(stmt, 1, domain, -1, SQLITE_STATIC);

  int cat_idx = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW && cat_idx < MAX_CATEGORIES) {
    const unsigned char *cat = sqlite3_column_text(stmt, 0);
    if (cat) {
      strncpy(node->categories[cat_idx], (const char *)cat,
              CATEGORY_MAX_LEN - 1);
      node->categories[cat_idx][CATEGORY_MAX_LEN - 1] = '\0';
    }
    cat_idx++;
  }

  sqlite3_finalize(stmt);
  return 0;
}

static enum load_result create_domain_node_from_db_row(sqlite3_stmt *stmt,
                                                       uint64_t now_cycles,
                                                       uint64_t hz) {

  const char *domain = (const char *)sqlite3_column_text(stmt, 0);
  int solution_is_send = sqlite3_column_int(stmt, 1);
  int trust_lvl = sqlite3_column_int(stmt, 2);
  uint64_t timestamp = (uint64_t)sqlite3_column_int64(stmt, 3);
  uint32_t ttl_seconds = (uint32_t)sqlite3_column_int(stmt, 4);

  uint64_t age_seconds = (now_cycles - timestamp) / hz;
  if (age_seconds >= ttl_seconds) {
    return LOAD_EXPIRED;
  }

  struct node_cache_domain *node =
      rte_malloc("loaded_node_cache", sizeof(struct node_cache_domain), 0);
  if (!node) {
    LOG_ERROR("Failed to allocate node for domain: %s", domain);
    return LOAD_ERROR;
  }

  node->solution_is_send = solution_is_send;
  node->trust_lvl = trust_lvl;
  node->timestamp = timestamp;
  node->ttl_seconds = ttl_seconds;

  if (load_domain_categories(domain, node) != 0) {
    rte_free(node);
    return LOAD_ERROR;
  }

  if (insert_loaded_node_domain(domain, node) != 0) {
    rte_free(node);
    return LOAD_ERROR;
  }

  return LOAD_OK;
}

void load_cache_from_sqlite(void) {
  if (!dns_hash || !cache_table) {
    LOG_ERROR("Cache or DB not initialized");
    return;
  }

  uint64_t now_cycles = rte_get_timer_cycles();
  uint64_t hz = rte_get_timer_hz();

  const char *sql = "SELECT domain, solution_is_send, trust_lvl, timestamp, "
                    "ttl_seconds FROM main_table;";

  sqlite3_stmt *stmt = NULL;
  int ret = sqlite3_prepare_v2(cache_table, sql, -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to prepare SELECT: %s", sqlite3_errmsg(cache_table));
    return;
  }

  int loaded = 0;
  int expired = 0;
  int errors = 0;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    enum load_result res = create_domain_node_from_db_row(stmt, now_cycles, hz);
    switch (res) {
    case LOAD_OK:
      loaded++;
      break;
    case LOAD_EXPIRED:
      expired++;
      break;
    case LOAD_ERROR:
      errors++;
      break;
    }
  }

  sqlite3_finalize(stmt);
  LOG_INFO("Loaded %d domains, expired %d, errors %d", loaded, expired, errors);
}

static void cache_save_timer_cb(struct rte_timer *tim, void *arg) {
  (void)tim;
  (void)arg;

  LOG_INFO("Periodic cache saving to SQLite.");

  pthread_t tid;
  if (pthread_create(&tid, NULL, save_all_cache_to_sqlite, NULL) != 0) {
    LOG_ERROR("Failed to create save thread");
    return;
  }

  pthread_detach(tid);
}

void close_sqlite_cache(void) {
  if (cache_table) {
    int ret = sqlite3_close(cache_table);
    if (ret != SQLITE_OK) {
      LOG_ERROR("Failed close SQLite connection: %s",
                sqlite3_errmsg(cache_table));
    }
  }
  cache_table = NULL;
}

static int insert_domain_main_record(const char *domain,
                                     struct node_cache_domain *node) {
  const char *sql =
      "INSERT OR REPLACE INTO main_table "
      "(domain, solution_is_send, trust_lvl, timestamp, ttl_seconds) "
      "VALUES (?, ?, ?, ?, ?)";
  sqlite3_stmt *stmt = NULL;
  int ret = sqlite3_prepare_v2(cache_table, sql, -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to prepare main insert: %s", sqlite3_errmsg(cache_table));
    return ret;
  }

  sqlite3_bind_text(stmt, 1, domain, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, node->solution_is_send ? 1 : 0);
  sqlite3_bind_int(stmt, 3, node->trust_lvl);
  sqlite3_bind_int64(stmt, 4, (sqlite3_int64)node->timestamp);
  sqlite3_bind_int(stmt, 5, (int)node->ttl_seconds);

  ret = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (ret != SQLITE_DONE) {
    LOG_ERROR("Failed to execute insert: %s", sqlite3_errmsg(cache_table));
    return ret;
  }
  return SQLITE_OK;
}

static int delete_domain_categories(const char *domain) {
  const char *sql = "DELETE FROM categories_table WHERE domain = ?";
  sqlite3_stmt *stmt = NULL;
  int ret = sqlite3_prepare_v2(cache_table, sql, -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to prepare delete: %s", sqlite3_errmsg(cache_table));
    return ret;
  }

  sqlite3_bind_text(stmt, 1, domain, -1, SQLITE_STATIC);
  ret = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return (ret == SQLITE_DONE) ? SQLITE_OK : ret;
}

static int insert_domain_categories(const char *domain,
                                    struct node_cache_domain *node) {
  const char *sql =
      "INSERT INTO categories_table (domain, certain_category) VALUES (?, ?)";
  sqlite3_stmt *stmt = NULL;
  int ret = sqlite3_prepare_v2(cache_table, sql, -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to prepare categories insert: %s",
              sqlite3_errmsg(cache_table));
    return ret;
  }

  for (int i = 0; i < MAX_CATEGORIES && node->categories[i][0] != '\0'; i++) {
    sqlite3_bind_text(stmt, 1, domain, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, node->categories[i], -1, SQLITE_STATIC);

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
      LOG_ERROR("Failed to insert category: %s", sqlite3_errmsg(cache_table));
      sqlite3_finalize(stmt);
      return ret;
    }
    sqlite3_reset(stmt);
  }

  sqlite3_finalize(stmt);
  return SQLITE_OK;
}

int save_single_node_to_sqlite(const char *domain,
                               struct node_cache_domain *node) {
  int ret;

  ret = insert_domain_main_record(domain, node);
  if (ret != SQLITE_OK)
    return ret;

  ret = delete_domain_categories(domain);
  if (ret != SQLITE_OK)
    return ret;

  ret = insert_domain_categories(domain, node);
  if (ret != SQLITE_OK)
    return ret;

  return SQLITE_OK;
}

void copy_data_from_hash_to_snapshot(struct snapshot_domain *snapt) {

  int i = 0;
  uint32_t next = 0;
  const void *key;
  void *data;

  rte_spinlock_lock(&cache_spinlock_domain);

  while (rte_hash_iterate(dns_hash, &key, &data, &next) >= 0) {

    strncpy(snapt[i].domain, (const char *)key, DOMAIN_MAX_LEN - 1);
    snapt[i].domain[DOMAIN_MAX_LEN - 1] = '\0';

    struct node_cache_domain *orig = (struct node_cache_domain *)data;
    memcpy(&snapt[i].node, orig, sizeof(struct node_cache_domain));

    i++;
  }

  rte_spinlock_unlock(&cache_spinlock_domain);
}

void *save_all_cache_to_sqlite(void *arg) {
  (void)arg;
  if (!dns_hash) {
    LOG_ERROR("Hash table is not initialized");
    return NULL;
  }

  if (!cache_table) {
    LOG_ERROR("SQLite connection is not open");
    return NULL;
  }

  uint32_t count = rte_hash_count(dns_hash);
  struct snapshot_domain *snapt =
      malloc(count * sizeof(struct snapshot_domain));
  copy_data_from_hash_to_snapshot(snapt);

  int ret;

  ret = sqlite3_exec(cache_table, "BEGIN TRANSACTION;", NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to exec BEGIN TRANSACTION: %s",
              sqlite3_errmsg(cache_table));
    return NULL;
  }

  int records = 0;
  int errors = 0;
  for (int i = 0; i < count; i++) {
    if (save_single_node_to_sqlite(snapt[i].domain, &snapt[i].node) ==
        SQLITE_OK)
      records++;
    else
      errors++;
  }

  ret = sqlite3_exec(cache_table, "COMMIT;", NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to exec COMMIT: %s", sqlite3_errmsg(cache_table));
    return NULL;
  }

  free(snapt);
  LOG_INFO("Saved %d records to SQLite, %d errors", count, errors);
  return NULL;
}

void init_tables_sqlite_dns_cache(void) {
  int ret = sqlite3_open("cache.db", &cache_table);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to open cache.db");
    return;
  }

  const char *create_main_table = "CREATE TABLE IF NOT EXISTS main_table("
                                  "domain TEXT PRIMARY KEY, "
                                  "solution_is_send INT NOT NULL, "
                                  "trust_lvl INT NOT NULL, "
                                  "timestamp INT NOT NULL, "
                                  "ttl_seconds INT NOT NULL)";

  ret = sqlite3_exec(cache_table, create_main_table, NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to create table 'main_table'");
    return;
  }

  const char *create_categories_table =
      "CREATE TABLE IF NOT EXISTS categories_table("
      "domain TEXT NOT NULL, "
      "certain_category TEXT NOT NULL, "
      "PRIMARY KEY (domain, certain_category), "
      "FOREIGN KEY (domain) REFERENCES main_table(domain))";

  ret = sqlite3_exec(cache_table, create_categories_table, NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to create table 'categories_table'");
    return;
  }

  ret =
      sqlite3_exec(cache_table, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to include foreign_keys");
  }
}

void init_dns_cache(void) {
  if (dns_hash)
    return;

  dns_hash = rte_hash_create(&hash_params);
  if (!dns_hash) {
    LOG_ERROR("Failed to create DNS cache hash table");
    return;
  }

  init_tables_sqlite_dns_cache();

  load_cache_from_sqlite();

  rte_timer_init(&cache_save_timer);
  save_interval_cycles = rte_get_timer_hz() * 3600;

  rte_timer_reset(&cache_save_timer, save_interval_cycles, PERIODICAL,
                  rte_lcore_id(), cache_save_timer_cb, NULL);
}

int lookup_dns_cache(const char *domain,
                     struct node_cache_domain **return_node) {
  rte_spinlock_lock(&cache_spinlock_domain);
  int ret = rte_hash_lookup_data(dns_hash, domain, (void **)return_node);

  if (ret >= 0 && *return_node) {
    uint64_t now = rte_get_timer_cycles();
    uint64_t hz = rte_get_timer_hz();
    uint64_t age_seconds = (now - (*return_node)->timestamp) / hz;

    if (age_seconds >= (*return_node)->ttl_seconds) {

      int ret_del = rte_hash_del_key(dns_hash, domain);
      if (ret_del < 0) {
        LOG_ERROR("Failed to deleting an obsolete hashtable value");
        rte_spinlock_unlock(&cache_spinlock_domain);
        return -ENOENT;
      }
      rte_free((*return_node)->key_domain);
      rte_free(*return_node);
      *return_node = NULL;

      rte_spinlock_unlock(&cache_spinlock_domain);
      return -ENOENT;
    }
  }
  rte_spinlock_unlock(&cache_spinlock_domain);
  return ret;
}

void add_to_dns_cache(const char *domain, struct node_cache_domain *node) {
  char *key_copy = rte_malloc("dns_key(domain)", DOMAIN_MAX_LEN, 0);
  if (!key_copy) {
    LOG_ERROR("Failed to allocate memory for key cache");
    rte_free(node);
    return;
  }
  strncpy(key_copy, domain, DOMAIN_MAX_LEN);
  key_copy[DOMAIN_MAX_LEN - 1] = '\0';
  node->timestamp = rte_get_timer_cycles();
  node->ttl_seconds = DNS_CACHE_DEFAULT_TTL;
  node->key_domain = key_copy;

  rte_spinlock_lock(&cache_spinlock_domain);
  int ret = rte_hash_add_key_data(dns_hash, key_copy, node);
  rte_spinlock_unlock(&cache_spinlock_domain);

  if (ret) {
    LOG_ERROR("Failed to add key data in hash table");
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
  rte_spinlock_lock(&cache_spinlock_domain);
  while (rte_hash_iterate(dns_hash, &key, &data, &next) >= 0) {

    if (data) {
      struct node_cache_domain *node = (struct node_cache_domain *)data;
      if (node->key_domain) {
        rte_free(node->key_domain);
      }
      rte_free(node);
    }
  }

  rte_hash_free(dns_hash);

  close_sqlite_cache();
  dns_hash = NULL;

  rte_spinlock_unlock(&cache_spinlock_domain);
  int ret = rte_timer_stop(&cache_save_timer);
  if (!ret) {
    LOG_ERROR("Failed to stopping timer");
  }
}

void clear_dns_cache(void) {
  if (!dns_hash)
    return;

  rte_spinlock_lock(&cache_spinlock_domain);
  rte_hash_reset(dns_hash);
  rte_spinlock_unlock(&cache_spinlock_domain);
  LOG_INFO("DNS cache clear");
}
