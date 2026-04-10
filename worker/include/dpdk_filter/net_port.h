#ifndef AF_XDP_PORT_H
#define AF_XDP_PORT_H

#include <rte_mempool.h>
#include <stdint.h>
#include "../../include/dpdk_filter/types.h"



struct net_port *init_struct_tap_port(const char *tap_iface_name,
                                            struct rte_mempool *mbuf_pool);


struct net_port *init_struct_af_xdp_port(const char *iface_name,
                                            struct rte_mempool *mbuf_pool);

int net_port_init(struct net_port *port);

int net_port_start(uint16_t port_id);

void net_port_close(struct net_port *port);

void net_port_destroy(struct net_port *port);

int find_port_by_dev_name(const char *dev_name, uint16_t *port_id_dev);

#endif