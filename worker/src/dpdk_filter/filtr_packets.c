#include "filtr_packets.h"
#include "pars_packets.h"

bool check_domain_is_block(char domain[DOMAIN_MAX_LEN],
                           char block_domains[MAX_DOMAINS][DOMAIN_MAX_LEN]) {

  for (int i = 0; i < MAX_DOMAINS; i++) {
    if (block_domains[i][0] == '\0') {
      break;
    }

    if (strcmp(block_domains[i], domain) == 0) {
      return true;
    }
  }

  return false;
}

bool check_domain_is_allow(char domain[DOMAIN_MAX_LEN],
                           char allow_domains[MAX_DOMAINS][DOMAIN_MAX_LEN]) {

  for (int i = 0; i < MAX_DOMAINS; i++) {
    if (allow_domains[i][0] == '\0') {
      break;
    }

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

bool check_ip_is_block(struct info_of_pakage *info_pac,
                       struct BASE_POLICY *policy) {
  if (info_pac->ip_version == IP_4) {
    for (int i = 0; i < MAX_IP4; i++) {
      if (policy->block_ip4[i] == 0) {
        break;
      }

      if (info_pac->ip4_dist == policy->block_ip4[i]) {
        return true;
      }
    }
  } else {
    for (int i = 0; i < MAX_IP6; i++) {
      if (policy->block_ip6[i][0] == '\0') {
        break;
      }

      if (memcmp(info_pac->ip6_dist, policy->block_ip6[i], IP6_LEN) == 0) {
        return true;
      }
    }
  }
  return false;
}

bool check_ip_is_allow(struct info_of_pakage *info_pac,
                       struct BASE_POLICY *policy) {
  if (info_pac->ip_version == IP_4) {
    for (int i = 0; i < MAX_IP4; i++) {
      if (policy->allow_ip4[i] == 0) {
        break;
      }

      if (info_pac->ip4_dist == policy->allow_ip4[i]) {
        return true;
      }
    }
  } else {
    for (int i = 0; i < MAX_IP6; i++) {
      if (policy->allow_ip6[i][0] == '\0') {
        break;
      }

      if (memcmp(info_pac->ip6_dist, policy->allow_ip6[i], IP6_LEN) == 0) {
        return true;
      }
    }
  }
  return false;
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

bool check_categories_and_trust_level(struct requested_classification *req_clas,
                                      struct BASE_POLICY *policy) {

  if (req_clas->get_trust_level == 0 &&
      strcmp(req_clas->get_categories[0], "unknown") == 0) {
    LOG_INFO("This site does not have a category, is sent");
    return true;
  }

  if (check_categories(req_clas->get_categories, policy->locked_categories) ==
      false) {
    LOG_INFO("This site has a locked category");
    return false;
  }

  if (check_trust_level(req_clas->get_trust_level, policy->min_trust_level) ==
      false) {
    LOG_INFO("This site has a too small trust level");
    return false;
  }

  if (check_categories_with_lvl(req_clas, policy->categories_with_lvl) ==
      false) {
    LOG_INFO(
        "This site blocked in accordance with 'trust categories with level'");
    return false;
  }

  return true;
}

bool main_filtring_by_domain(struct requested_classification *req_clas,
                             struct BASE_POLICY *policy,
                             struct info_of_pakage *info_pac) {

  if (check_domain_is_block(info_pac->domain, policy->block_domains) == true) {
    LOG_INFO("Domain '%s' is blocked", info_pac->domain);
    return false;
  }

  if (check_domain_is_allow(info_pac->domain, policy->allow_domains) == true) {
    LOG_INFO("Domain '%s' is allowed", info_pac->domain);
    return true;
  }

  return check_categories_and_trust_level(req_clas, policy);
}

bool main_filtring_by_ip(struct requested_classification *req_clas,
                         struct BASE_POLICY *policy,
                         struct info_of_pakage *info_pac) {

  if (check_ip_is_block(info_pac, policy) == true) {
    LOG_INFO("IPv%d dst is blocked", info_pac->ip_version == IP_4 ? 4 : 6);
    return false;
  }

  if (check_ip_is_allow(info_pac, policy) == true) {
    LOG_INFO("IPv%d dst is allowed", info_pac->ip_version == IP_4 ? 4 : 6);
    return true;
  }

  return check_categories_and_trust_level(req_clas, policy);
}