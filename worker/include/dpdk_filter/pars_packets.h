#ifndef PARS_PAK_H
#define PARS_PAK_H

#include <stdint.h>
#include <rte_mbuf.h>

struct info_of_pakage {
    uint16_t ethernet_type_host;
    uint16_t ethernet_type_protocol;
    uint16_t number_port;
    char* domain;
};

void parsing_pakage(struct rte_mbuf* paket, struct info_of_pakage* info_pac);


#endif