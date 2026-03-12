#ifndef AF_XDP_PORT_H
#define AF_XDP_PORT_H

#include <stdint.h>
#include <rte_mempool.h>

int af_xdp_port_init(const char* iface_name, uint16_t* port_id, struct rte_mempool* mbuf_pool);

int af_xdp_port_start(uint16_t port_id);

void af_xdp_port_close(const char* ifface_name, uint16_t port_id);

#endif