#include <rte_ethdev.h>

#include "../../include/dpdk_filter/af_xdp_port.h"

int main(int argc, char **argv)
{
    struct rte_mbuf *pkts[32];
    uint64_t total_pkts = 0;
    uint16_t port_id;
    struct rte_mempool *mbuf_pool;
    
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        printf("ERROR: EAL init failed\n");
        return -1;
    }
    
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", 8192, 250, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (!mbuf_pool) {
        printf("ERROR: Failed to create mbuf pool\n");
        return -1;
    }
    
    if (af_xdp_port_init("eth0", &port_id, mbuf_pool) < 0) {
        printf("ERROR: Failed to init port\n");
        return -1;
    }
    
    if (af_xdp_port_start(port_id) < 0) {
        af_xdp_port_close(port_id);
        return -1;
    }
    
    printf("AF_XDP port %u is running. Press Ctrl+C to stop.\n", port_id);
    
    while (1) {
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, pkts, 32);
        if (nb_rx > 0) {
            total_pkts += nb_rx;
            printf("\rPackets received: %u", total_pkts);
            fflush(stdout);
            // parse_packet(pkts[0]);
            for (int i = 0; i < nb_rx; i++) {
                rte_pktmbuf_free(pkts[i]);
            }
        }
        usleep(5);
    }
    
    printf("total pkts: %u", total_pkts);
    af_xdp_port_close(port_id);
    return 0;
}