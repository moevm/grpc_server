#ifndef PARS_PAK_H
#define PARS_PAK_H

#include <rte_mbuf.h>
#include <stdint.h>

struct info_of_pakage {
  uint16_t ethernet_type_host;
  uint16_t ethernet_type_protocol;
  uint16_t number_port;
  char domain[260];
};

void parsing_pakage(struct rte_mbuf *paket, struct info_of_pakage *info_pac);

#endif
