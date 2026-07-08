#include <errno.h>
#include <rte_bus.h>
#include <rte_bus_vdev.h>
#include <rte_dev.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "net_port.h"

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

int find_port_by_dev_name(const char *dev_name, uint16_t *port_id_dev) {
  uint16_t count_ports = rte_eth_dev_count_avail();
  struct rte_eth_dev_info dev_info;
  char name[64];

  for (uint16_t port_id = 0; port_id < count_ports; port_id++) {
    int ret = rte_eth_dev_info_get(port_id, &dev_info);

    if (ret) {
      LOG_ERROR("Failed to retrieve the contextual information of an "
                "Ethernet device: %s",
                strerror(-ret));
      return ret;
    }

    if (rte_eth_dev_get_name_by_port(port_id, name) == 0 &&
        strcmp(name, dev_name) == 0) {
      *port_id_dev = port_id;
      return 0;
    }
  }
  return -1;
}

struct net_port *init_struct_tap_port(const char *tap_iface_name,
                                      struct rte_mempool *mbuf_pool) {

  struct net_port *port = calloc(1, sizeof(struct net_port));
  if (!port) {
    LOG_ERROR("Failed to allocate memory for struct net_port");
    return NULL;
  }

  snprintf(port->dev_args, sizeof(port->dev_args), "iface=%s, remote=%s",
           tap_iface_name, tap_iface_name);
  snprintf(port->dev_name, sizeof(port->dev_name), "net_tap_%s",
           tap_iface_name);
  strncpy(port->iface_name, tap_iface_name, sizeof(port->iface_name) - 1);
  port->iface_name[sizeof(port->iface_name) - 1] = '\0';
  port->mbuf_pool = mbuf_pool;
  port->port_id = -1;

  return port;
}

struct net_port *init_struct_af_xdp_port(const char *iface_name,
                                         struct rte_mempool *mbuf_pool) {
  struct net_port *port = calloc(1, sizeof(struct net_port));
  if (!port) {
    LOG_ERROR("Failed to allocate memory for struct net_port");
    return NULL;
  }

  snprintf(port->dev_name, sizeof(port->dev_name), "eth_af_packet_%s", iface_name);
  
  snprintf(port->dev_args, sizeof(port->dev_args),
           "iface=%s,qpairs=3,blocksz=16384,framesz=2048,framecnt=4096", iface_name);
  strncpy(port->iface_name, iface_name, sizeof(port->iface_name) - 1);
  port->iface_name[sizeof(port->iface_name) - 1] = '\0';
  port->mbuf_pool = mbuf_pool;
  port->port_id = -1;

  return port;
}

int net_port_init(struct net_port *port) {
  int ret;
  struct rte_eth_conf port_conf = {0};
  const char *dev_name = port->dev_name;
  uint16_t port_id;

  ret = rte_vdev_init(dev_name, port->dev_args);

  if (ret < 0) {
    LOG_ERROR("Failed to create vdev: %s", strerror(-ret));
    return ret;
  }

  ret = find_port_by_dev_name(port->dev_name, &port_id);
  if (ret) {
    LOG_INFO("no port was found that has the same vdev name. vdev = %s",
             port->dev_name);
    rte_vdev_uninit(dev_name);
    return -1;
  }

  port->port_id = port_id;

  if (!rte_eth_dev_is_valid_port(port_id)) {
    LOG_ERROR("Port %u is not valid", port_id);
    rte_vdev_uninit(dev_name);
    return -EINVAL;
  }

  ret = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
  if (ret < 0) {
    LOG_ERROR("Failed to configure port: %s", strerror(-ret));
    rte_vdev_uninit(dev_name);
    return ret;
  }

  ret = rte_eth_rx_queue_setup(port_id, 0, RX_RING_SIZE,
                               rte_eth_dev_socket_id(port_id), NULL,
                               port->mbuf_pool);
  if (ret < 0) {
    LOG_ERROR("Failed to setup RX queue: %s", strerror(-ret));
    rte_vdev_uninit(dev_name);
    return ret;
  }

  ret = rte_eth_tx_queue_setup(port_id, 0, TX_RING_SIZE,
                               rte_eth_dev_socket_id(port_id), NULL);

  if (ret < 0) {
    LOG_ERROR("Failed to setup TX queue: %s", strerror(-ret));
    rte_vdev_uninit(dev_name);
    return ret;
  }

  ret = rte_eth_macaddr_get(port_id, &port->mac_addr);
  if (ret < 0) {
    LOG_ERROR("Failed to macaddr get: %s", strerror(-ret));
    rte_vdev_uninit(dev_name);
    return ret;
  }
  port->neighbor_learned = false;
  LOG_INFO("Port %u initialized, MAC=%02x:%02x:%02x:%02x:%02x:%02x", port_id,
           port->mac_addr.addr_bytes[0], port->mac_addr.addr_bytes[1],
           port->mac_addr.addr_bytes[2], port->mac_addr.addr_bytes[3],
           port->mac_addr.addr_bytes[4], port->mac_addr.addr_bytes[5]);  return 0;
}

int net_port_start(uint16_t port_id) {
  int ret;

  ret = rte_eth_dev_start(port_id);
  if (ret < 0) {
    LOG_ERROR("Failed to start: %s", strerror(-ret));
    return ret;
  }

  ret = rte_eth_promiscuous_enable(port_id);
  if (ret) {
    LOG_ERROR("Failed to enable receipt in promiscuous mode for an "
              "Ethernet device: %s",
              strerror(-ret));
    return ret;
  }

  LOG_INFO("Port %u started", port_id);
  return 0;
}

void net_port_destroy(struct net_port *port) {
  if (!port)
    return;
  free(port);
}

void net_port_close(struct net_port *port) {

  if (!port)
    return;

  int ret;
  uint16_t port_id = port->port_id;

  ret = rte_eth_dev_stop(port_id);
  if (ret) {
    LOG_ERROR("Failed to stop an Ethernet device: %s", strerror(-ret));
    return;
  }

  ret = rte_eth_dev_close(port_id);
  if (ret) {
    LOG_ERROR("Failed to close a stopped Ethernet device: %s", strerror(-ret));
    return;
  }

  ret = rte_vdev_uninit(port->dev_name);
  if (ret) {
    LOG_ERROR("Failed to uninitialize a driver: %s", strerror(-ret));
    return;
  }

  port->port_id = -1;
  LOG_INFO("Port %u closed", port_id);
}