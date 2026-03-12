#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <stdio.h>
#include "../../include/dpdk_filter/af_xdp_port.h"
#include <unistd.h>
#include <rte_ip.h>


int main(int argc, char** argv) {
    uint16_t port_in, port_out;
    struct rte_mempool *mbuf_pool;
    unsigned mbuf_quantity_in_pool = 8192;
    unsigned cache_size_per_kernel = 250;
    uint16_t queue_number = 0;
    uint16_t nb_pkts = 32;
    uint16_t priv_size = 0;
    struct rte_mbuf *pkts[32];
    
    rte_eal_init(argc, argv);
    
    mbuf_pool = rte_pktmbuf_pool_create("POOL", mbuf_quantity_in_pool, cache_size_per_kernel, priv_size, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    
    af_xdp_port_init("veth0", &port_in, mbuf_pool);
    af_xdp_port_init("veth1", &port_out, mbuf_pool);
    
    af_xdp_port_start(port_in);
    af_xdp_port_start(port_out);
    
    printf("Запущен бесконечный цикл. Пакеты проходят из порта с id=%u в порт с id=%u\n", port_in, port_out);
    
    while (1) {
        
        uint16_t nb_rx = rte_eth_rx_burst(port_in, queue_number, pkts, nb_pkts);
        for (int i = 0; i < nb_rx; i++) {         
            rte_eth_tx_burst(port_out, queue_number, &pkts[i], 1);
        }

    }

    af_xdp_port_close("veth0", port_in);
    af_xdp_port_close("veth1", port_out);
    return 0;
}