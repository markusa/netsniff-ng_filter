/*
 * netsniff-ng - the packet sniffing beast
 * By Daniel Borkmann <daniel@netsniff-ng.org>
 * Copyright 2009, 2010 Daniel Borkmann.
 * Copyright 2010 Emmanuel Roullit.
 * Subject to the GPL, version 2.
 */

#ifndef VLAN_H
#define VLAN_H

#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h>    /* for ntohs() */

#include "proto_struct.h"
#include "dissector_eth.h"
#include "pkt_buff.h"

struct vlanhdr {
	uint16_t h_vlan_TCI;
	uint16_t h_vlan_encapsulated_proto;
} __attribute__((packed));

static inline void vlan(struct pkt_buff *pkt)
{
	uint16_t tci;
	struct vlanhdr *vlan = (struct vlanhdr *) pkt_pull(pkt, sizeof(*vlan));

	if (vlan == NULL)
		return;

	tci = ntohs(vlan->h_vlan_TCI);

	alloc_string(*pkt->buffer_pkt,pkt->switch_filter," [ VLAN ");
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,
		     "Prio (%d), ", (tci & 0xE000) >> 13);
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,
		     "CFI (%d), ", (tci & 0x1000) >> 12);
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,
		     "ID (%d), ", (tci & 0x0FFF));
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,"Proto (0x%.4x)",
		     ntohs(vlan->h_vlan_encapsulated_proto));
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter," ]\n");

	pkt_set_proto(pkt, &eth_lay2, ntohs(vlan->h_vlan_encapsulated_proto));
}

static inline void vlan_less(struct pkt_buff *pkt)
{
	uint16_t tci;
	struct vlanhdr *vlan = (struct vlanhdr *) pkt_pull(pkt, sizeof(*vlan));

	if (vlan == NULL)
		return;

	tci = ntohs(vlan->h_vlan_TCI);

	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,
		     " VLAN%d", (tci & 0x0FFF));

	pkt_set_proto(pkt, &eth_lay2, ntohs(vlan->h_vlan_encapsulated_proto));
}

struct protocol vlan_ops = {
	.key = 0x8100,
	.print_full = vlan,
	.print_less = vlan_less,
};

#endif /* VLAN_H */
