#include "../../include/dpdk_filter/filtr_packets.h"
#include "../../include/dpdk_filter/pars_packets.h"

bool check_is_block(char domain[DOMAIN_MAX_LEN],
                    char block_domains[MAX_DOMAINS][DOMAIN_MAX_LEN]) {

  for (int i = 0; i < MAX_DOMAINS; i++) {
    if (strcmp(block_domains[i], domain) == 0) {
      return true;
    }
  }

  return false;
}

bool check_is_allow(char domain[DOMAIN_MAX_LEN],
                    char allow_domains[MAX_DOMAINS][DOMAIN_MAX_LEN]) {

  for (int i = 0; i < MAX_DOMAINS; i++) {
    if (strcmp(allow_domains[i], domain) == 0) {
      return true;
    }
  }

  return false;
}

bool check_trust_level(int get_trust_level, int min_trust_level) {

  if (get_trust_level < min_trust_level) {
    return false;
  }

  return true;
}

bool check_categories(
    char get_categories[MAX_CATEGORIES][CATEGORY_MAX_LEN],
    char locked_categories[MAX_CATEGORIES][CATEGORY_MAX_LEN]) {

  for (int i = 0; i < MAX_CATEGORIES; i++) {
    for (int j = 0; j < MAX_CATEGORIES; j++) {
      if (strcmp(get_categories[i], locked_categories[j]) == 0) {
        return false;
      }
    }
  }

  return true;
}

bool check_categories_with_lvl(
    struct requested_classification *req_clas,
    struct trust_categories_with_lvl
        categories_with_lvl[MAX_CATEGORIES_BY_TRUST_LVL]) {

  for (int i = 0; i < MAX_CATEGORIES; i++) {
    for (int j = 0; j < MAX_CATEGORIES; j++) {
      if (strcmp(req_clas->get_categories[j],
                 categories_with_lvl[i].locked_by_trust_category) == 0 &&
          req_clas->get_trust_level < categories_with_lvl[i].trust_lvl) {
        return false;
      }
    }
  }

  return true;
}

bool main_filtring(struct requested_classification *req_clas,
                   struct BASE_POLICY *policy, char domain[DOMAIN_MAX_LEN]) {

  if (check_is_block(domain, policy->block_domains) == true) {
    printf("This domain is blocked");
    return false;
  }

  if (check_is_allow(domain, policy->allow_domains) == true) {
    printf("This domain is allowed");
    return true;
  }

  if (check_categories(req_clas->get_categories, policy->locked_categories) ==
      false) {
    printf("This site has a locked category");
    return false;
  }

  if (check_trust_level(req_clas->get_trust_level, policy->min_trust_level) ==
      false) {
    printf("This site has a too small trust level");
    return false;
  }

  if (check_categories_with_lvl(req_clas, policy->categories_with_lvl) ==
      false) {
    printf(
        "This site blocked in accordance with 'trust categories with level'");
    return false;
  }

  return true;
}