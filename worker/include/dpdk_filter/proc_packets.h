#ifndef PROC_PAK_H
#define PROC_PAK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../include/dpdk_filter/af_xdp_port.h"
#include "../../include/dpdk_filter/filtr_packets.h"
#include "../../include/dpdk_filter/pars_packets.h"
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <stdint.h>
#include <stdlib.h>

void pakage_processing(struct af_xdp_port *port_in,
                       struct af_xdp_port *port_out, uint16_t queue_number,
                       uint16_t nb_pkts, struct rte_mbuf **pkts);

#ifdef __cplusplus
}
#endif

#endif
