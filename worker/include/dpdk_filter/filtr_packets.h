#ifndef FILTR_PAK_H
#define FILTR_PAK_H

#include "../../include/dpdk_filter/constants.h"
#include "../../include/dpdk_filter/types.h"
#include "pars_packets.h"
#include <rte_mbuf.h>
#include <stdint.h>

bool check_is_block(char domain[DOMAIN_MAX_LEN],
                    char block_domains[MAX_DOMAINS][DOMAIN_MAX_LEN]);

bool check_is_allow(char domain[DOMAIN_MAX_LEN],
                    char allow_domains[MAX_DOMAINS][DOMAIN_MAX_LEN]);

bool check_trust_level(int get_trust_level, int min_trust_level);

bool check_categories(char get_categories[MAX_CATEGORIES][CATEGORY_MAX_LEN],
                      char locked_categories[MAX_CATEGORIES][CATEGORY_MAX_LEN]);

bool check_categories_with_lvl(
    struct requested_classification *req_clas,
    struct trust_categories_with_lvl
        categories_with_lvl[MAX_CATEGORIES_BY_TRUST_LVL]);

bool main_filtring(struct requested_classification *req_clas,
                   struct BASE_POLICY *policy, char domain[DOMAIN_MAX_LEN]);

#endif
