#ifndef FILTR_PAK_H
#define FILTR_PAK_H

#include "constants.h"
#include "pars_packets.h"
#include "types.h"
#include <rte_mbuf.h>
#include <stdint.h>

bool check_domain_is_block(char domain[DOMAIN_MAX_LEN],
                           char block_domains[MAX_DOMAINS][DOMAIN_MAX_LEN]);
bool check_domain_is_allow(char domain[DOMAIN_MAX_LEN],
                           char allow_domains[MAX_DOMAINS][DOMAIN_MAX_LEN]);

bool check_ip_is_block(struct info_of_pakage *info_pac,
                       struct BASE_POLICY *policy);
bool check_ip_is_allow(struct info_of_pakage *info_pac,
                       struct BASE_POLICY *policy);

bool check_trust_level(int get_trust_level, int min_trust_level);

bool check_categories(char get_categories[MAX_CATEGORIES][CATEGORY_MAX_LEN],
                      char locked_categories[MAX_CATEGORIES][CATEGORY_MAX_LEN]);
bool check_categories_with_lvl(
    struct requested_classification *req_clas,
    struct trust_categories_with_lvl
        categories_with_lvl[MAX_CATEGORIES_BY_TRUST_LVL]);
bool check_categories_and_trust_level(struct requested_classification *req_clas,
                                      struct BASE_POLICY *policy);

bool main_filtring_by_domain(struct requested_classification *req_clas,
                             struct BASE_POLICY *policy,
                             struct info_of_pakage *info_pac);
bool main_filtring_by_ip(struct requested_classification *req_clas,
                         struct BASE_POLICY *policy,
                         struct info_of_pakage *info_pac);

#endif