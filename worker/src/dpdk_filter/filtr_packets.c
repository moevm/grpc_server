#include "../../include/dpdk_filter/filtr_packets.h"
#include "../../include/dpdk_filter/pars_packets.h"

bool check_domain(char *domain) {
  // PLUG (to be added later)
  return true;
}

bool main_filtring(
    struct info_of_pakage
        *info_pac) { // CHECK BY CATEGORY, BY TRUST LEVEL, MAYBE SOMETHING ELSE
  if (!check_domain(info_pac->domain)) {
    printf("domain is block");
    return false;
  }
  // OTHER REQUIRED CHECKS
  return true;
}