#ifndef PROC_PAK_H
#define PROC_PAK_H

#include <rte_mempool.h>
#include <stdint.h>
#include <rte_mbuf.h>
#include "../../include/dpdk_filter/af_xdp_port.h"
#include "../../include/dpdk_filter/pars_pak.h"
#include "../../include/dpdk_filter/filtr_pak.h"
#include <stdlib.h>
#include <rte_ethdev.h>


void pakage_processing(struct af_xdp_port* port_in, struct af_xdp_port* port_out, uint16_t queue_number, uint16_t nb_pkts, struct rte_mbuf** pkts);

#endif