#include "../../include/dpdk_filter/proc_pak.h"


void pakage_processing(struct af_xdp_port* port_in, struct af_xdp_port* port_out, uint16_t queue_number, uint16_t nb_pkts, struct rte_mbuf** pkts) {

    uint16_t nb_rx = rte_eth_rx_burst(port_in->port_id, queue_number, pkts, nb_pkts);
    for (int i = 0; i < nb_rx; i++) {
        struct info_of_pakage* info_pac = calloc(1, sizeof(struct info_of_pakage));
        parsing_pakage(pkts[i], info_pac);

        bool skip_packet = main_filtring(info_pac);
        if (!skip_packet) {
            rte_eth_tx_burst(port_in->port_id, queue_number, &pkts[i], 1);
        }
    }
}

