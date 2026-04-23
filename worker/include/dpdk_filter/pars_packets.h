#ifndef PARS_PAK_H
#define PARS_PAK_H

#include "../../include/dpdk_filter/constants.h"
#include "../../include/dpdk_filter/types.h"
#include <rte_mbuf.h>
#include <stdint.h>

void parsing_pakage(struct rte_mbuf *paket, struct info_of_pakage *info_pac);

#endif