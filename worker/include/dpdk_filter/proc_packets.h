#ifndef PROC_PAK_H
#define PROC_PAK_H

#include "constants.h"
#include "filtr_packets.h"
#include "net_port.h"
#include "pars_packets.h"
#include "types.h"
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <stdint.h>
#include <stdlib.h>

void pakage_processing(struct net_port *port_in, struct net_port *port_out,
                       struct net_port *port_exception, uint16_t queue_number,
                       uint16_t nb_pkts, struct rte_mbuf **pkts,
                       struct BASE_POLICY *policy);

#endif