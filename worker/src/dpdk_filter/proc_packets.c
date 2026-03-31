#include "../../include/dpdk_filter/proc_packets.h"

void pakage_processing(struct af_xdp_port *port_in,
                       struct af_xdp_port *port_out, uint16_t queue_number,
                       uint16_t nb_pkts, struct rte_mbuf **pkts) {

  uint16_t nb_rx =
      rte_eth_rx_burst(port_in->port_id, queue_number, pkts, nb_pkts);

  for (int i = 0; i < nb_rx; i++) {

    struct info_of_pakage info_pac;
    memset(&info_pac, 0, sizeof(info_pac));

    parsing_pakage(pkts[i], &info_pac);

    bool skip_packet = main_filtring(&info_pac);

    if (!skip_packet) {

      uint16_t ret =
          rte_eth_tx_burst(port_out->port_id, queue_number, &pkts[i], 1);

      if (ret < 1) {
        printf("[ERROR] Failed to send packet\n");
        // PLUG (to be added later) - need to add processing for this case
        rte_pktmbuf_free(pkts[i]);
      }
    } else {
      rte_pktmbuf_free(pkts[i]);
    }
  }
}
