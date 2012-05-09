/*
 * netsniff-ng - the packet sniffing beast
 * By Daniel Borkmann <daniel@netsniff-ng.org>
 * Copyright 2009, 2010, 2011, 2012 Daniel Borkmann.
 * Subject to the GPL, version 2.
 */

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "built_in.h"
#include "tprintf.h"
#include "pkt_buff.h"
#include "proto_struct.h"
#define __without_ops
#include "proto_none.h"
#include "dissector.h"
#include "dissector_eth.h"


int dissector_set_print_type(void *ptr, int type)
{
	struct protocol *proto;

	for (proto = (struct protocol *) ptr; proto; proto = proto->next) {
		switch (type) {
		case FNTTYPE_PRINT_NORM:
			proto->process = proto->print_full;
			break;
		case FNTTYPE_PRINT_LESS:
			proto->process = proto->print_less;
			break;
		case FNTTYPE_PRINT_HEX:
		case FNTTYPE_PRINT_ASCII:
		case FNTTYPE_PRINT_HEX_ASCII:
		case FNTTYPE_PRINT_NONE:
		default:
			proto->process = NULL;
			break;
		}
	}

	return 0;
}

static void dissector_main(struct pkt_buff *pkt, struct protocol *start,
			   struct protocol *end, char **buffer_pkt,
			   uint8_t *switch_filter, uint8_t *stats)
{
	struct protocol *proto;
	
	pkt->buffer_pkt = buffer_pkt;
	struct filter_all filter;
	memset(&filter,0,sizeof(struct filter_all));
	pkt->filter = &filter;
	pkt->switch_filter = switch_filter;

	for (pkt->proto = start; pkt->proto; ) {
		if (unlikely(!pkt->proto->process))
			break;

		proto = pkt->proto;
		pkt->proto = NULL;
		proto->process(pkt);
	}

	if(*switch_filter){
	    if((*pkt->filter).ip4.ipv4 && (*pkt->filter).ip4.proto==6) {
		    if(*buffer_pkt) {
			  tprintf("%s",*buffer_pkt);
			  xfree(*buffer_pkt);
		    }
	    }
	    else {
		    *stats = 0;
		    return;
	    }
	}


	if (end && likely(end->process) && *switch_filter != 2)
		    end->process(pkt);
}

void dissector_entry_point(uint8_t *packet, struct frame_map *hdr, int linktype,
			      int mode, char **buffer_pkt,
			      uint8_t *switch_filter, uint8_t *stats)
{
	struct protocol *proto_start = NULL;
	struct protocol *proto_end = NULL;
	struct pkt_buff *pkt = NULL;

	if (mode == FNTTYPE_PRINT_NONE)
		return;

	pkt = pkt_alloc(packet, hdr->tp_h.tp_snaplen);

	switch (linktype) {
	case LINKTYPE_EN10MB:
		proto_start = dissector_get_ethernet_entry_point();
		proto_end = dissector_get_ethernet_exit_point();
		break;
	default:
		panic("Linktype not supported!\n");
	};

	dissector_main(pkt, proto_start, proto_end, buffer_pkt, switch_filter,
								    stats);


	switch (mode) {
	case FNTTYPE_PRINT_HEX:
		hex(pkt);
		break;
	case FNTTYPE_PRINT_ASCII:
		ascii(pkt);
		break;
	case FNTTYPE_PRINT_HEX_ASCII:
		hex_ascii(pkt);
		break;
	}
	tprintf_flush();
	pkt_free(pkt);
}

void dissector_init_all(int fnttype)
{
	dissector_init_ethernet(fnttype);
}

void dissector_cleanup_all(void)
{
	dissector_cleanup_ethernet();
}
