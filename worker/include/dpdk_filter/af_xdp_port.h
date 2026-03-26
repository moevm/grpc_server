#ifndef AF_XDP_PORT_H
#define AF_XDP_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <rte_mempool.h>
#include <stdint.h>

struct af_xdp_port {
  uint16_t port_id;
  char iface_name[32];
  char dev_name[64];
  char dev_args[256];
  struct rte_mempool *mbuf_pool;
};

struct af_xdp_port *init_struct_af_xdp_port(const char *iface_name,
                                            struct rte_mempool *mbuf_pool);

int af_xdp_port_init(struct af_xdp_port *port);

int af_xdp_port_start(uint16_t port_id);

void af_xdp_port_close(struct af_xdp_port *port);

int find_port_by_dev_name(const char *dev_name, uint16_t *port_id_dev);

#ifdef __cplusplus
}
#endif

#endif
