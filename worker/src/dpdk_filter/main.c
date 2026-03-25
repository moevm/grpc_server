#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <stdio.h>
#include "../../include/dpdk_filter/af_xdp_port.h"
#include "../../include/dpdk_filter/proc_pak.h"
#include <unistd.h>
#include <rte_ip.h>


int main(int argc, char** argv) {
    struct af_xdp_port* port_in = NULL;
    struct af_xdp_port* port_out = NULL;
    struct rte_mempool *mbuf_pool;
    unsigned mbuf_quantity_in_pool = 8192;
    unsigned cache_size_per_kernel = 250;
    uint16_t queue_number = 0;
    uint16_t nb_pkts = 32;
    uint16_t priv_size = 0;
    struct rte_mbuf* pkts[32];
    
    rte_eal_init(argc, argv);
    
    mbuf_pool = rte_pktmbuf_pool_create("POOL", mbuf_quantity_in_pool, cache_size_per_kernel, priv_size, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    
    port_in = init_struct_af_xdp_port("veth0", mbuf_pool);
    port_out = init_struct_af_xdp_port("veth1", mbuf_pool);

    if (af_xdp_port_init(port_in) || af_xdp_port_init(port_out)) {
        return 1;
    }
    
    af_xdp_port_start(port_in->port_id);
    af_xdp_port_start(port_out->port_id);
    
    printf("An endless cycle has been started. Packets pass from port with id=%u to port with id=%u\n", port_in->port_id, port_out->port_id);
    
    while (1) {
        
        pakage_processing(port_in, port_out, queue_number, nb_pkts, pkts);

    }

    af_xdp_port_close(port_in);
    af_xdp_port_close(port_out);
    return 0;
}