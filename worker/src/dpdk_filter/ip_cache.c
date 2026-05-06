#include "ip_cache.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <rte_spinlock.h>
#include <stdlib.h>

static sqlite3 *ip_cache_table;
static rte_spinlock_t cache_spinlock_ip = RTE_SPINLOCK_INITIALIZER;

static struct rte_hash *ip_hash;
static struct rte_hash_parameters ip_hash_params = {
    .name = "ip_cache_hash",
    .entries = CACHE_SIZE,
    .key_len = IP_MAX_LEN,
    .hash_func = rte_jhash,
    .extra_flag = RTE_HASH_EXTRA_FLAGS_EXT_TABLE,
};

static struct rte_timer cache_save_timer;
static uint64_t save_interval_cycles;

static int insert_loaded_node_ip(const struct ip_key *ip,
                                 struct node_cache_ip *node) {
  struct ip_key *key_copy = rte_malloc("ip_key(ip)", IP_MAX_LEN, 0);
  if (!key_copy) {
    LOG_ERROR("Failed to allocate key for loaded node");
    return -ENOMEM;
  }

  memcpy(key_copy, ip, IP_MAX_LEN);
  node->key = key_copy;

  rte_spinlock_lock(&cache_spinlock_ip);
  int ret = rte_hash_add_key_data(ip_hash, key_copy, node);
  rte_spinlock_unlock(&cache_spinlock_ip);

  if (ret < 0) {
    LOG_ERROR("Failed to insert loaded node into hash: %s", strerror(-ret));
    rte_free(key_copy);
    return ret;
  }
  return 0;
}

static int ip_str_to_key(const char *ip_str, struct ip_key *key) {
  if (inet_pton(AF_INET, ip_str, &key->addr.ip4) == 1) {
    key->version = 4;
    return 0;
  }
  if (inet_pton(AF_INET6, ip_str, &key->addr.ip6) == 1) {
    key->version = 6;
    return 0;
  }
  LOG_ERROR("Failed to parse IP: %s", ip_str);
  return -1;
}

static int load_ip_categories(const char *ip_str,
                              struct node_cache_ip *node_ip) {
  const char *sql_cat =
      "SELECT certain_category FROM categories_table WHERE ip_str = ?;";
  sqlite3_stmt *stmt_cat = NULL;
  int rc_cat = sqlite3_prepare_v2(ip_cache_table, sql_cat, -1, &stmt_cat, NULL);
  if (rc_cat != SQLITE_OK) {
    LOG_ERROR("Failed to prepare categories SELECT: %s",
              sqlite3_errmsg(ip_cache_table));
    return -1;
  }

  sqlite3_bind_text(stmt_cat, 1, ip_str, -1, SQLITE_STATIC);

  int cat_idx = 0;
  while (sqlite3_step(stmt_cat) == SQLITE_ROW && cat_idx < MAX_CATEGORIES) {
    const unsigned char *cat_text = sqlite3_column_text(stmt_cat, 0);
    if (cat_text) {
      strncpy(node_ip->categories[cat_idx], (const char *)cat_text,
              CATEGORY_MAX_LEN - 1);
      node_ip->categories[cat_idx][CATEGORY_MAX_LEN - 1] = '\0';
    } else {
      node_ip->categories[cat_idx][0] = '\0';
    }
    cat_idx++;
  }

  sqlite3_finalize(stmt_cat);
  return 0;
}

static enum load_result
create_node_from_db_row(sqlite3_stmt *stmt, uint64_t now_cycles, uint64_t hz) {

  const char *ip_str = (const char *)sqlite3_column_text(stmt, 0);
  int solution_is_send = sqlite3_column_int(stmt, 1);
  int trust_lvl = sqlite3_column_int(stmt, 2);
  uint64_t timestamp = (uint64_t)sqlite3_column_int64(stmt, 3);
  uint32_t ttl_seconds = (uint32_t)sqlite3_column_int(stmt, 4);

  uint64_t age_seconds = (now_cycles - timestamp) / hz;
  if (age_seconds >= ttl_seconds) {
    return LOAD_EXPIRED;
  }

  struct node_cache_ip *node =
      rte_malloc("loaded_node_cache", sizeof(struct node_cache_ip), 0);
  if (!node) {
    LOG_ERROR("Failed to allocate node for IP: %s", ip_str);
    return LOAD_ERROR;
  }

  node->solution_is_send = solution_is_send;
  node->trust_lvl = trust_lvl;
  node->timestamp = timestamp;
  node->ttl_seconds = ttl_seconds;

  if (load_ip_categories(ip_str, node)) {
    rte_free(node);
    return LOAD_ERROR;
  }

  struct ip_key key;
  if (ip_str_to_key(ip_str, &key)) {
    rte_free(node);
    return LOAD_ERROR;
  }

  if (insert_loaded_node_ip(&key, node)) {
    rte_free(node);
    return LOAD_ERROR;
  }

  return LOAD_OK;
}

void load_cache_ip_from_sqlite(void) {
  if (!ip_hash) {
    LOG_ERROR("Hash table not initialized for loading");
    return;
  }
  if (!ip_cache_table) {
    LOG_ERROR("SQLite connection not open for loading");
    return;
  }

  uint64_t now_cycles = rte_get_timer_cycles();
  uint64_t hz = rte_get_timer_hz();

  const char *sql = "SELECT ip_str, solution_is_send, trust_lvl, timestamp, "
                    "ttl_seconds FROM ip_main_table;";

  sqlite3_stmt *stmt = NULL;
  int ret = sqlite3_prepare_v2(ip_cache_table, sql, -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to prepare SELECT from ip_main_table: %s",
              sqlite3_errmsg(ip_cache_table));
    return;
  }

  int loaded = 0;
  int errors = 0;
  int expired = 0;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    ret = create_node_from_db_row(stmt, now_cycles, hz);
    switch (ret) {
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

  ret = sqlite3_finalize(stmt);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to delete prepared statement: %s",
              sqlite3_errmsg(ip_cache_table));
    return;
  }
  LOG_INFO("Loaded %d records from SQLite, %d records expired, %d errors",
           loaded, expired, errors);
}

static void cache_save_timer_cb(struct rte_timer *tim, void *arg) {
  (void)tim;
  (void)arg;

  LOG_INFO("Periodic cache saving to SQLite.");

  pthread_t tid;
  if (pthread_create(&tid, NULL, save_all_cache_ip_to_sqlite, NULL) != 0) {
    LOG_ERROR("Failed to create save thread");
    return;
  }

  pthread_detach(tid);
}

void close_sqlite_cache_ip(void) {
  if (ip_cache_table) {
    int ret = sqlite3_close(ip_cache_table);
    if (ret != SQLITE_OK) {
      LOG_ERROR("Failed close SQLite connection: %s",
                sqlite3_errmsg(ip_cache_table));
    }
  }
  ip_cache_table = NULL;
}

int save_single_node_ip_to_sqlite(const struct ip_key *key,
                                  struct node_cache_ip *node) {
  sqlite3_stmt *stmt = NULL;
  int ret;

  const char *sql_main =
      "INSERT OR REPLACE INTO ip_main_table "
      "(ip_str, solution_is_send, trust_lvl, timestamp, ttl_seconds) "
      "VALUES (?, ?, ?, ?, ?)";

  ret = sqlite3_prepare_v2(ip_cache_table, sql_main, -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to prepare ip_main_table insert: %s",
              sqlite3_errmsg(ip_cache_table));
    sqlite3_finalize(stmt);
    return ret;
  }

  char ip_str[INET6_ADDRSTRLEN];
  if (key->version == 4) {
    inet_ntop(AF_INET, &key->addr.ip4, ip_str, sizeof(ip_str));
  } else {
    inet_ntop(AF_INET6, &key->addr.ip6, ip_str, sizeof(ip_str));
  }

  sqlite3_bind_text(stmt, 1, ip_str, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, node->solution_is_send ? 1 : 0);
  sqlite3_bind_int(stmt, 3, node->trust_lvl);
  sqlite3_bind_int64(stmt, 4, (sqlite3_int64)node->timestamp);
  sqlite3_bind_int(stmt, 5, (int)node->ttl_seconds);

  ret = sqlite3_step(stmt);
  if (ret != SQLITE_DONE) {
    LOG_ERROR("Failed to insert into ip_main_table: %s",
              sqlite3_errmsg(ip_cache_table));
    sqlite3_finalize(stmt);
    return ret;
  }

  ret = sqlite3_finalize(stmt);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to delete prepared statement: %s",
              sqlite3_errmsg(ip_cache_table));
    return ret;
  }

  const char *sql_del = "DELETE FROM categories_table WHERE ip_str = ?";
  ret = sqlite3_prepare_v2(ip_cache_table, sql_del, -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to prepare delete: %s", sqlite3_errmsg(ip_cache_table));
    sqlite3_finalize(stmt);
    return ret;
  }

  sqlite3_bind_text(stmt, 1, ip_str, -1, SQLITE_STATIC);
  ret = sqlite3_step(stmt);
  if (ret != SQLITE_DONE) {
    LOG_ERROR("Failed to delete old categories: %s",
              sqlite3_errmsg(ip_cache_table));
    sqlite3_finalize(stmt);
    return ret;
  }

  ret = sqlite3_finalize(stmt);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to delete prepared statement: %s",
              sqlite3_errmsg(ip_cache_table));
    return ret;
  }

  const char *sql_cat =
      "INSERT INTO categories_table (ip_str, certain_category) VALUES (?, ?)";
  ret = sqlite3_prepare_v2(ip_cache_table, sql_cat, -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to prepare categories insert: %s",
              sqlite3_errmsg(ip_cache_table));
    return ret;
  }

  for (int i = 0; i < MAX_CATEGORIES; i++) {
    if (strlen(node->categories[i]) == 0) {
      break;
    }

    sqlite3_bind_text(stmt, 1, ip_str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, node->categories[i], -1, SQLITE_STATIC);

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
      LOG_ERROR("Failed to prepare categories_table insert: %s",
                sqlite3_errmsg(ip_cache_table));
      sqlite3_finalize(stmt);
      return ret;
    }

    ret = sqlite3_reset(stmt);
    if (ret != SQLITE_OK) {
      LOG_ERROR("Failed to reset prepared statement: %s",
                sqlite3_errmsg(ip_cache_table));
      sqlite3_finalize(stmt);
      return ret;
    }
  }

  ret = sqlite3_finalize(stmt);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to delete prepared statement: %s",
              sqlite3_errmsg(ip_cache_table));
    return ret;
  }

  return SQLITE_OK;
}

void copy_data_from_hash_to_snapshot_ip(struct snapshot_ip *snapt) {

  int i = 0;
  uint32_t next = 0;
  const void *key;
  void *data;

  rte_spinlock_lock(&cache_spinlock_ip);

  while (rte_hash_iterate(ip_hash, &key, &data, &next) >= 0) {

    memcpy(&snapt[i].key, (const struct ip_key *)(key), IP_MAX_LEN);
    struct node_cache_ip *orig = (struct node_cache_ip *)data;
    memcpy(&snapt[i].node, orig, sizeof(struct node_cache_ip));

    i++;
  }

  rte_spinlock_unlock(&cache_spinlock_ip);
}

void *save_all_cache_ip_to_sqlite(void *arg) {
  (void)arg;
  if (!ip_hash) {
    LOG_ERROR("Hash table is not initialized");
    return NULL;
  }

  if (!ip_cache_table) {
    LOG_ERROR("SQLite connection is not open");
    return NULL;
  }

  uint32_t count = rte_hash_count(ip_hash);
  struct snapshot_ip *snapt = malloc(count * sizeof(struct snapshot_ip));
  copy_data_from_hash_to_snapshot_ip(snapt);

  int ret;

  ret = sqlite3_exec(ip_cache_table, "BEGIN TRANSACTION;", NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to exec BEGIN TRANSACTION: %s",
              sqlite3_errmsg(ip_cache_table));
    return NULL;
  }

  int records = 0;
  int errors = 0;
  for (int i = 0; i < count; i++) {
    if (save_single_node_ip_to_sqlite(&snapt[i].key, &snapt[i].node) ==
        SQLITE_OK)
      records++;
    else
      errors++;
  }

  ret = sqlite3_exec(ip_cache_table, "COMMIT;", NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to exec COMMIT: %s", sqlite3_errmsg(ip_cache_table));
    return NULL;
  }

  free(snapt);
  LOG_INFO("Saved %d records to SQLite, %d errors", count, errors);
  return NULL;
}

void init_tables_sqlite_ip_cache(void) {
  int ret = sqlite3_open("ip_cache.db", &ip_cache_table);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to open ip_cache.db");
    return;
  }

  const char *create_main_table = "CREATE TABLE IF NOT EXISTS ip_main_table("
                                  "ip_str TEXT PRIMARY KEY, "
                                  "solution_is_send INT NOT NULL, "
                                  "trust_lvl INT NOT NULL, "
                                  "timestamp INT NOT NULL, "
                                  "ttl_seconds INT NOT NULL)";

  ret = sqlite3_exec(ip_cache_table, create_main_table, NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to create table 'ip_main_table'");
    return;
  }

  const char *create_categories_table =
      "CREATE TABLE IF NOT EXISTS categories_table("
      "ip_str TEXT NOT NULL, "
      "certain_category TEXT NOT NULL, "
      "PRIMARY KEY (ip_str, certain_category), "
      "FOREIGN KEY (ip_str) REFERENCES ip_main_table(ip_str))";

  ret = sqlite3_exec(ip_cache_table, create_categories_table, NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to create table 'categories_table'");
    return;
  }

  ret = sqlite3_exec(ip_cache_table, "PRAGMA foreign_keys = ON;", NULL, NULL,
                     NULL);
  if (ret != SQLITE_OK) {
    LOG_ERROR("Failed to include foreign_keys");
  }
}

void init_ip_cache(void) {
  if (ip_hash)
    return;

  ip_hash = rte_hash_create(&ip_hash_params);
  if (!ip_hash) {
    LOG_ERROR("Failed to create ip cache hash table");
    return;
  }

  init_tables_sqlite_ip_cache();

  load_cache_ip_from_sqlite();

  rte_timer_init(&cache_save_timer);
  save_interval_cycles = rte_get_timer_hz() * 3600;

  rte_timer_reset(&cache_save_timer, save_interval_cycles, PERIODICAL,
                  rte_lcore_id(), cache_save_timer_cb, NULL);
}

int lookup_ip_cache(const struct ip_key *key,
                    struct node_cache_ip **return_node) {
  rte_spinlock_lock(&cache_spinlock_ip);
  int ret = rte_hash_lookup_data(ip_hash, key, (void **)return_node);

  if (ret >= 0 && *return_node) {
    uint64_t now = rte_get_timer_cycles();
    uint64_t hz = rte_get_timer_hz();
    uint64_t age_seconds = (now - (*return_node)->timestamp) / hz;

    if (age_seconds >= (*return_node)->ttl_seconds) {

      int ret_del = rte_hash_del_key(ip_hash, key);
      if (ret_del < 0) {
        LOG_ERROR("Failed to deleting an obsolete hashtable value");
        rte_spinlock_unlock(&cache_spinlock_ip);
        return -ENOENT;
      }
      rte_free((*return_node)->key);
      rte_free(*return_node);
      *return_node = NULL;

      rte_spinlock_unlock(&cache_spinlock_ip);
      return -ENOENT;
    }
  }
  rte_spinlock_unlock(&cache_spinlock_ip);
  return ret;
}

void add_to_ip_cache(const struct ip_key *key, struct node_cache_ip *node) {

  struct ip_key *key_copy = rte_malloc("ip_key(ip)", IP_MAX_LEN, 0);
  if (!key_copy) {
    LOG_ERROR("Failed to allocate memory for key cache");
    rte_free(node);
    return;
  }

  memcpy(key_copy, key, IP_MAX_LEN);
  node->timestamp = rte_get_timer_cycles();
  node->ttl_seconds = IP_CACHE_DEFAULT_TTL;
  node->key = key_copy;

  rte_spinlock_lock(&cache_spinlock_ip);
  int ret = rte_hash_add_key_data(ip_hash, key_copy, node);
  rte_spinlock_unlock(&cache_spinlock_ip);

  if (ret) {
    LOG_ERROR("Failed to add key data in hash table");
    rte_free(key_copy);
    rte_free(node);
  }
}

void free_ip_cache(void) {
  if (!ip_hash)
    return;

  uint32_t next = 0;
  const void *key;
  void *data;
  rte_spinlock_lock(&cache_spinlock_ip);
  while (rte_hash_iterate(ip_hash, &key, &data, &next) >= 0) {

    if (data) {
      struct node_cache_ip *node = (struct node_cache_ip *)data;
      if (node->key) {
        rte_free(node->key);
      }
      rte_free(node);
    }
  }

  rte_hash_free(ip_hash);

  close_sqlite_cache_ip();
  ip_hash = NULL;

  rte_spinlock_unlock(&cache_spinlock_ip);
  int ret = rte_timer_stop(&cache_save_timer);
  if (!ret) {
    LOG_ERROR("Failed to stopping timer");
  }
}
