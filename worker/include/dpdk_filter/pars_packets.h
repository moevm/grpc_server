#ifndef PARS_PAK_H
#define PARS_PAK_H

#include <rte_mbuf.h>
#include <stdint.h>
#include "../../include/dpdk_filter/constants.h"
#include "../../include/dpdk_filter/types.h"



void parsing_pakage(struct rte_mbuf *paket, struct info_of_pakage *info_pac);

#endif
