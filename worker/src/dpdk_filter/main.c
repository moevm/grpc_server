#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <stdio.h>
#include "../../include/dpdk_filter/af_xdp_port.h"
#include "../../include/dpdk_filter/dns_cache.h"
#include "../../include/dpdk_filter/dns_parser.h"
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
    init_dns_cache();
    
    mbuf_pool = rte_pktmbuf_pool_create("POOL", mbuf_quantity_in_pool, cache_size_per_kernel, priv_size, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    
    af_xdp_port_init("veth0", &port_in, mbuf_pool);
    af_xdp_port_init("veth1", &port_out, mbuf_pool);
    
    af_xdp_port_start(port_in);
    af_xdp_port_start(port_out);
    
    printf("An endless cycle has been started. Packets pass from port with id=%u to port with id=%u\n", port_in, port_out);
    int i = 0;
    while (i < 10) {
        
        uint16_t nb_rx = rte_eth_rx_burst(port_in, queue_number, pkts, nb_pkts);
        for (int i = 0; i < nb_rx; i++) {
            char domain[256];
            extract_dns_domain(pkts[i], domain, 256);
            struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkts[i], struct rte_ether_hdr *);
            uint16_t ether_type = rte_be_to_cpu_16(eth_hdr->ether_type);

            if (ether_type == 0x0806) {
                printf("ARP packet received\n");
            }
            rte_eth_tx_burst(port_out, queue_number, &pkts[i], 1);
        }
        sleep(1);
        i++;
    }

    af_xdp_port_close("veth0", port_in);
    af_xdp_port_close("veth1", port_out);
    return 0;
}