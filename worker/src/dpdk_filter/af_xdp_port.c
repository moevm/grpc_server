#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <rte_bus_vdev.h>


#include "../../include/dpdk_filter/af_xdp_port.h"

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

int af_xdp_port_init(const char *iface_name, uint16_t *port_id, struct rte_mempool *mbuf_pool)
{
    int ret;
    char vdev_args[256];
    struct rte_eth_conf port_conf = {0};
    
    printf("[AF_XDP] Initializing on %s\n", iface_name);
    
    snprintf(vdev_args, sizeof(vdev_args), "iface=%s,start_queue=0,queue_count=1", iface_name);
    ret = rte_vdev_init("net_af_xdp", vdev_args);
    
    if (ret < 0) {
        printf("[AF_XDP ERROR] Failed to create vdev: %s\n", strerror(-ret));
        return ret;
    }
    
    *port_id = rte_eth_dev_count_avail() - 1;
    
    ret = rte_eth_dev_configure(*port_id, 1, 1, &port_conf);
    if (ret < 0) {
        printf("[AF_XDP ERROR] Failed to configure port: %s\n", strerror(-ret));
        rte_vdev_uninit("net_af_xdp");
        return ret;
    }
    
    ret = rte_eth_rx_queue_setup(*port_id, 0, RX_RING_SIZE, rte_eth_dev_socket_id(*port_id), NULL, mbuf_pool);
    if (ret < 0) {
        printf("[AF_XDP ERROR] Failed to setup RX queue: %s\n", strerror(-ret));
        return ret;
    }
    
    ret = rte_eth_tx_queue_setup(*port_id, 0, TX_RING_SIZE, rte_eth_dev_socket_id(*port_id), NULL);

    if (ret < 0) {
        printf("[AF_XDP ERROR] Failed to setup TX queue: %s\n", strerror(-ret));
        return ret;
    }
    
    printf("[AF_XDP] Port %u initialized\n", *port_id);
    return 0;
}




int af_xdp_port_start(uint16_t port_id)
{
    int ret;
    
    printf("[AF_XDP] Starting port %u\n", port_id);
    
    ret = rte_eth_dev_start(port_id);
    if (ret < 0) {
        printf("[AF_XDP ERROR] Failed to start: %s\n", strerror(-ret));
        return ret;
    }
    
    rte_eth_promiscuous_enable(port_id);
    
    printf("[AF_XDP] Port %u started\n", port_id);
    return 0;
}




void af_xdp_port_close(uint16_t port_id)
{
    printf("[AF_XDP] Closing port %u\n", port_id);
    
    rte_eth_dev_stop(port_id);
    rte_eth_dev_close(port_id);
    rte_vdev_uninit("net_af_xdp");
    
    printf("[AF_XDP] Port %u closed\n", port_id);
}