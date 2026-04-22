#ifndef PARS_PAK_H
#define PARS_PAK_H

#include "constants.h"
#include "types.h"
#include <rte_mbuf.h>
#include <stdint.h>

void parsing_pakage(struct rte_mbuf *paket, struct info_of_pakage *info_pac);

#endif