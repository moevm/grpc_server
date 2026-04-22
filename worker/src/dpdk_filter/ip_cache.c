#include "ip_cache.h"

static sqlite3 *cache_table_ip;

static struct rte_hash *ip_hash;
static struct rte_hash_parameters hash_params = {
    .name = "ip_hash",
    .entries = CACHE_SIZE,
    .key_len = DOMAIN_MAX_LEN,
    .hash_func = rte_jhash,
    .extra_flag = RTE_HASH_EXTRA_FLAGS_EXT_TABLE,
};
static struct rte_timer cache_save_timSer;
static uint64_t save_interval_cycles;

static int insert_loaded_node_ip(const void *ip, struct node_cache_ip *node) {
  char *key_copy = rte_malloc("ip_key(ip)", DOMAIN_MAX_LEN, 0);
  if (!key_copy) {
    printf("[ERROR] Failed to allocate key for loaded node\n");
    return -ENOMEM;
  }

  strncpy(key_copy, ip, 16);
  key_copy[DOMAIN_MAX_LEN - 1] = '\0';
  node->key_ip = key_copy;

  int ret = rte_hash_add_key_data(dns_hash, key_copy, node);
  if (ret < 0) {
    printf("[ERROR] Failed to insert loaded node into hash: %s\n",
           strerror(-ret));
    rte_free(key_copy);
    return ret;
  }
  return 0;
}

void load_cache_from_sqlite_ip(void) {
  if (!dns_hash) {
    printf("[ERROR] Hash table not initialized for loading\n");
    return;
  }
  if (!cache_table_ip) {
    printf("[ERROR] SQLite connection not open for loading\n");
    return;
  }

  uint64_t now_cycles = rte_get_timer_cycles();
  uint64_t hz = rte_get_timer_hz();

  const char *sql = "SELECT domain, solution_is_send, trust_lvl, timestamp, "
                    "ttl_seconds FROM main_table;";

  sqlite3_stmt *stmt = NULL;
  int ret = sqlite3_prepare_v2(cache_table_ip, sql, -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to prepare SELECT from main_table: %s\n",
           sqlite3_errmsg(cache_table_ip));
    return;
  }

  int loaded = 0;
  int expired = 0;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *domain = (const char *)sqlite3_column_text(stmt, 0);
    int solution_is_send = sqlite3_column_int(stmt, 1);
    int trust_lvl = sqlite3_column_int(stmt, 2);
    uint64_t timestamp = (uint64_t)sqlite3_column_int64(stmt, 3);
    uint32_t ttl_seconds = (uint32_t)sqlite3_column_int(stmt, 4);

    uint64_t age_seconds = (now_cycles - timestamp) / hz;
    if (age_seconds >= ttl_seconds) {
      expired++;
      continue;
    }

    struct node_cache_ip *node =
        rte_malloc("loaded_node_cache_ip", sizeof(struct node_cache_ip), 0);
    if (!node) {
      printf("[ERROR] Failed to allocate node for domain %s\n", domain);
      continue;
    }

    node->solution_is_send = solution_is_send ? true : false;
    node->trust_lvl = trust_lvl;
    node->timestamp = timestamp;
    node->ttl_seconds = ttl_seconds;

    const char *sql_cat =
        "SELECT certain_category FROM categories_table WHERE domain = ?;";
    sqlite3_stmt *stmt_cat = NULL;
    int rc_cat =
        sqlite3_prepare_v2(cache_table_ip, sql_cat, -1, &stmt_cat, NULL);
    if (rc_cat != SQLITE_OK) {
      printf("[ERROR] Failed to prepare categories SELECT: %s\n",
             sqlite3_errmsg(cache_table_ip));
      rte_free(node);
      continue;
    }

    sqlite3_bind_text(stmt_cat, 1, domain, -1, SQLITE_STATIC);

    int cat_idx = 0;
    while (sqlite3_step(stmt_cat) == SQLITE_ROW && cat_idx < MAX_CATEGORIES) {
      const unsigned char *cat_text = sqlite3_column_text(stmt_cat, 0);
      if (cat_text) {
        strncpy(node->categories[cat_idx], (const char *)cat_text,
                CATEGORY_MAX_LEN - 1);
        node->categories[cat_idx][CATEGORY_MAX_LEN - 1] = '\0';
      } else {
        node->categories[cat_idx][0] = '\0';
      }
      cat_idx++;
    }

    sqlite3_finalize(stmt_cat);

    if (insert_loaded_node(domain, node) == 0) {
      loaded++;
    } else {
      rte_free(node);
    }
  }

  ret = sqlite3_finalize(stmt);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to delete prepared statement: %s\n",
           sqlite3_errmsg(cache_table_ip));
    return;
  }
  printf("[INFO] Loaded %d records from SQLite, %d expired skipped\n", loaded,
         expired);
}

static void cache_save_timer_ip(struct rte_timer *tim, void *arg) {
  (void)tim;
  (void)arg;

  printf("[INFO] Periodic cache saving to SQLite.\n");
  save_all_cache_to_sqlite();
}

void close_sqlite_cache_ip(void) {
  if (cache_table_ip) {
    int ret = sqlite3_close(cache_table_ip);
    if (ret != SQLITE_OK) {
      printf("[ERROR] Failed close SQLite connection: %s\n",
             sqlite3_errmsg(cache_table_ip));
    }
  }
  cache_table_ip = NULL;
}

int save_single_node_to_sqlite_ip(const char *domain,
                                  struct node_cache_ip *node) {
  sqlite3_stmt *stmt = NULL;
  int ret;

  const char *sql_main =
      "INSERT OR REPLACE INTO main_table "
      "(domain, solution_is_send, trust_lvl, timestamp, ttl_seconds) "
      "VALUES (?, ?, ?, ?, ?)";

  ret = sqlite3_prepare_v2(cache_table_ip, sql_main, -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to prepare main_table insert: %s\n",
           sqlite3_errmsg(cache_table_ip));
    sqlite3_finalize(stmt);
    return ret;
  }

  sqlite3_bind_text(stmt, 1, domain, -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, node->solution_is_send ? 1 : 0);
  sqlite3_bind_int(stmt, 3, node->trust_lvl);
  sqlite3_bind_int64(stmt, 4, (sqlite3_int64)node->timestamp);
  sqlite3_bind_int(stmt, 5, (int)node->ttl_seconds);

  ret = sqlite3_step(stmt);
  if (ret != SQLITE_DONE) {
    printf("[ERROR] Failed to insert into main_table: %s\n",
           sqlite3_errmsg(cache_table_ip));
    sqlite3_finalize(stmt);
    return ret;
  }

  ret = sqlite3_finalize(stmt);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to delete prepared statement: %s\n",
           sqlite3_errmsg(cache_table_ip));
    return ret;
  }

  const char *sql_del = "DELETE FROM categories_table WHERE domain = ?";
  ret = sqlite3_prepare_v2(cache_table_ip, sql_del, -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to prepare delete: %s\n",
           sqlite3_errmsg(cache_table_ip));
    sqlite3_finalize(stmt);
    return ret;
  }

  sqlite3_bind_text(stmt, 1, domain, -1, SQLITE_STATIC);
  ret = sqlite3_step(stmt);
  if (ret != SQLITE_DONE) {
    printf("[ERROR] Failed to delete old categories: %s\n",
           sqlite3_errmsg(cache_table_ip));
    sqlite3_finalize(stmt);
    return ret;
  }

  ret = sqlite3_finalize(stmt);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to delete prepared statement: %s\n",
           sqlite3_errmsg(cache_table_ip));
    return ret;
  }

  const char *sql_cat =
      "INSERT INTO categories_table (domain, certain_category) VALUES (?, ?)";
  ret = sqlite3_prepare_v2(cache_table_ip, sql_cat, -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to prepare categories insert: %s\n",
           sqlite3_errmsg(cache_table_ip));
    return ret;
  }

  for (int i = 0; i < MAX_CATEGORIES; i++) {
    if (strlen(node->categories[i]) == 0) {
      break;
    }

    sqlite3_bind_text(stmt, 1, domain, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, node->categories[i], -1, SQLITE_STATIC);

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
      printf("[ERROR] Failed to prepare categories_table insert: %s\n",
             sqlite3_errmsg(cache_table_ip));
      sqlite3_finalize(stmt);
      return ret;
    }

    ret = sqlite3_reset(stmt);
    if (ret != SQLITE_OK) {
      printf("[ERROR] Failed to reset prepared statement: %s\n",
             sqlite3_errmsg(cache_table_ip));
      sqlite3_finalize(stmt);
      return ret;
    }
  }

  ret = sqlite3_finalize(stmt);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to delete prepared statement: %s\n",
           sqlite3_errmsg(cache_table_ip));
    return ret;
  }

  return SQLITE_OK;
}

int save_all_cache_ip_to_sqlite(void) {
  if (!dns_hash) {
    printf("[ERROR] Hash table is not initialized\n");
    return -1;
  }

  if (!cache_table_ip) {
    printf("[ERROR] SQLite connection is not open\n");
    return -1;
  }

  uint32_t next = 0;
  const void *key;
  void *data;
  int count = 0;
  int errors = 0;
  int ret;

  ret = sqlite3_exec(cache_table_ip, "BEGIN TRANSACTION;", NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to exec BEGIN TRANSACTION: %s\n",
           sqlite3_errmsg(cache_table_ip));
    return -1;
  }

  while (rte_hash_iterate(dns_hash, &key, &data, &next) >= 0) {
    const char *domain = (const char *)key;
    struct node_cache_ip *node = (struct node_cache_ip *)data;

    if (!domain || !node) {
      continue;
    }

    ret = save_single_node_to_sqlite(domain, node);
    if (ret == SQLITE_OK) {
      count++;
    } else {
      errors++;
    }
  }

  ret = sqlite3_exec(cache_table_ip, "COMMIT;", NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to exec COMMIT: %s\n",
           sqlite3_errmsg(cache_table_ip));
    return -1;
  }

  printf("[INFO] Saved %d records to SQLite, %d errors\n", count, errors);
  return count;
}

void init_tables_sqlite_ip_cache(void) {
  int ret = sqlite3_open("cache.db", &cache_table_ip);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to open cache.db\n");
    return;
  }

  const char *create_main_table = "CREATE TABLE IF NOT EXISTS main_table("
                                  "domain TEXT PRIMARY KEY, "
                                  "solution_is_send INT NOT NULL, "
                                  "trust_lvl INT NOT NULL, "
                                  "timestamp INT NOT NULL, "
                                  "ttl_seconds INT NOT NULL)";

  ret = sqlite3_exec(cache_table_ip, create_main_table, NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to create table 'main_table'\n");
    return;
  }

  const char *create_categories_table =
      "CREATE TABLE IF NOT EXISTS categories_table("
      "domain TEXT NOT NULL, "
      "certain_category TEXT NOT NULL, "
      "PRIMARY KEY (domain, certain_category), "
      "FOREIGN KEY (domain) REFERENCES main_table(domain))";

  ret = sqlite3_exec(cache_table_ip, create_categories_table, NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to create table 'categories_table'\n");
    return;
  }

  ret = sqlite3_exec(cache_table_ip, "PRAGMA foreign_keys = ON;", NULL, NULL,
                     NULL);
  if (ret != SQLITE_OK) {
    printf("[ERROR] Failed to include foreign_keys\n");
  }
}

void init_ip_cache(void) {
  if (dns_hash)
    return;

  dns_hash = rte_hash_create(&hash_params);
  if (!dns_hash) {
    printf("[ERROR] Failed to create DNS cache hash table\n");
    return;
  }

  init_tables_sqlite_ip_cache();

  load_cache_from_sqlite();

  rte_timer_init(&cache_save_timer);
  save_interval_cycles = rte_get_timer_hz() * 3600;

  rte_timer_reset(&cache_save_timer, save_interval_cycles, PERIODICAL,
                  rte_lcore_id(), cache_save_timer_cb, NULL);
}

int lookup_ip_cache(const char *domain, struct node_cache_ip **return_node) {
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

void add_to_ip_cache(const char *domain, struct node_cache_ip *node) {
  char *key_copy = rte_malloc("dns_key(domain)", DOMAIN_MAX_LEN, 0);
  if (!key_copy) {
    printf("[ERROR] Failed to allocate memory for key cache\n");
    rte_free(node);
    return;
  }
  strncpy(key_copy, domain, DOMAIN_MAX_LEN);
  key_copy[DOMAIN_MAX_LEN - 1] = '\0';
  node->timestamp = rte_get_timer_cycles();
  node->ttl_seconds = ip_cache_DEFAULT_TTL;
  node->key_domain = key_copy;

  int ret = rte_hash_add_key_data(dns_hash, key_copy, node);
  if (ret) {
    printf("[ERROR] Failed to add key data in hash table\n");
    rte_free(key_copy);
    rte_free(node);
  }
}

void free_ip_cache(void) {
  if (!dns_hash)
    return;

  uint32_t next = 0;
  const void *key;
  void *data;

  while (rte_hash_iterate(dns_hash, &key, &data, &next) >= 0) {

    if (data) {
      struct node_cache_ip *node = (struct node_cache_ip *)data;
      if (node->key_domain) {
        rte_free(node->key_domain);
      }
      rte_free(node);
    }
  }

  rte_hash_free(dns_hash);

  close_sqlite_cache();
  dns_hash = NULL;

  int ret = rte_timer_stop(&cache_save_timer);
  if (!ret) {
    printf("[ERROR] Failed to stopping timer\n");
  }
}