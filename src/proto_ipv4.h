/*
 * netsniff-ng - the packet sniffing beast
 * By Daniel Borkmann <daniel@netsniff-ng.org>.
 * Copyright (C) 2009, 2010 Daniel Borkmann
 * Copyright (C) 2012 Christoph Jaeger <christoph@netsniff-ng.org>
 * Subject to the GPL, version 2.
 */

#ifndef PROTO_IPV4_H
#define PROTO_IPV4_H

#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h>    /* for ntohs() */
#include <arpa/inet.h>     /* for inet_ntop() */
#include <asm/byteorder.h>

#include "csum.h"
#include "proto_struct.h"
#include "dissector_eth.h"
#include "pkt_buff.h"
#include "built_in.h"


struct ipv4hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__extension__ uint8_t h_ihl:4,
			      h_version:4;
#elif defined (__BIG_ENDIAN_BITFIELD)
	__extension__ uint8_t h_version:4,
			      h_ihl:4;
#else
# error "Please fix <asm/byteorder.h>"
#endif
	uint8_t h_tos;
	uint16_t h_tot_len;
	uint16_t h_id;
	uint16_t h_frag_off;
	uint8_t h_ttl;
	uint8_t h_protocol;
	uint16_t h_check;
	uint32_t h_saddr;
	uint32_t h_daddr;
} __packed;

#define FRAG_OFF_RESERVED_FLAG(x)      ((x) & 0x8000)
#define FRAG_OFF_NO_FRAGMENT_FLAG(x)   ((x) & 0x4000)
#define FRAG_OFF_MORE_FRAGMENT_FLAG(x) ((x) & 0x2000)
#define FRAG_OFF_FRAGMENT_OFFSET(x)    ((x) & 0x1fff)

/* IP Option Numbers (http://www.iana.org/assignments/ip-parameters) */
#define IP_OPT_EOOL 0x00
#define IP_OPT_NOP  0x01

#define IP_OPT_COPIED_FLAG(x)  ((x) & 0x80)
#define IP_OPT_CLASS(x)       (((x) & 0x60) >> 5)
#define IP_OPT_NUMBER(x)       ((x) & 0x1F)

static inline void ipv4(struct pkt_buff *pkt)
{
	uint16_t csum, frag_off;
	char src_ip[INET_ADDRSTRLEN];
	char dst_ip[INET_ADDRSTRLEN];
	struct ipv4hdr *ip = (struct ipv4hdr *) pkt_pull(pkt, sizeof(*ip));
	uint8_t *opt;
	ssize_t opts_len, opt_len;

	if (!ip)
		return;

(*pkt->filter).ip4.ipv4 = 1;
(*pkt->filter).ip4.proto = ip->h_protocol;

	frag_off = ntohs(ip->h_frag_off);
	csum = calc_csum(ip, ip->h_ihl * 4, 0);

	inet_ntop(AF_INET, &ip->h_saddr, src_ip, sizeof(src_ip));
	inet_ntop(AF_INET, &ip->h_daddr, dst_ip, sizeof(dst_ip));

	alloc_string(*pkt->buffer_pkt,pkt->switch_filter," [ IPv4 ");
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,"Addr (%s => %s), ",
						      src_ip, dst_ip);
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,"Proto (%u), ",
						      ip->h_protocol);
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,"TTL (%u), ",
						      ip->h_ttl);
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,"TOS (%u), ",
						      ip->h_tos);
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,"Ver (%u), ",
						      ip->h_version);
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,"IHL (%u), ",
						      ip->h_ihl);
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,"Tlen (%u), ",
						      ntohs(ip->h_tot_len));
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,"ID (%u), ",
						      ntohs(ip->h_id));
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,
		     "Res (%u), NoFrag (%u), MoreFrag (%u), FragOff (%u), ",
		FRAG_OFF_RESERVED_FLAG(frag_off) ? 1 : 0,
		FRAG_OFF_NO_FRAGMENT_FLAG(frag_off) ? 1 : 0,
		FRAG_OFF_MORE_FRAGMENT_FLAG(frag_off) ? 1 : 0,
		FRAG_OFF_FRAGMENT_OFFSET(frag_off));
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter,"CSum (0x%.4x) is %s",
		ntohs(ip->h_check),
		csum ? colorize_start_full(black, red) "bogus (!)"
		       colorize_end() : "ok");
	if (csum)
		alloc_string(*pkt->buffer_pkt,pkt->switch_filter,"%s should be \
			0x%.4x%s",colorize_start_full(black, red),
			csum_expected(ip->h_check, csum), colorize_end());
	alloc_string(*pkt->buffer_pkt,pkt->switch_filter," ]\n");

	opts_len = max((uint8_t) ip->h_ihl, sizeof(*ip) / sizeof(uint32_t)) *
		   sizeof(uint32_t) - sizeof(*ip);

	for (opt = pkt_pull(pkt, opts_len); opt && opts_len > 0; opt++) {
		alloc_string(*pkt->buffer_pkt,pkt->switch_filter,"   \
		[ Option Copied (%u), Class (%u), Number (%u)",
			IP_OPT_COPIED_FLAG(*opt) ? 1 : 0, IP_OPT_CLASS(*opt),
			IP_OPT_NUMBER(*opt));

		switch (*opt) {
		case IP_OPT_EOOL:
		case IP_OPT_NOP:
			alloc_string(*pkt->buffer_pkt,pkt->switch_filter,
				     "]\n");
			opts_len--;
			break;
		default:
			/*
			 * Assuming that EOOL and NOP are the only single-byte
			 * options, treat all other options as variable in
			 * length with a minimum of 2.
			 *
			 * TODO: option length might be incorrect in malformed
packets,
			 *       check and handle that
			 */
			opt_len = *(++opt);
			if (opt_len > opts_len) {

alloc_string(*pkt->buffer_pkt,pkt->switch_filter,
					    ", Len (%u, invalid) ]\n",opt_len);
				goto out;
			} else

alloc_string(*pkt->buffer_pkt,pkt->switch_filter,
					    ",Len (%u) ]\n",opt_len);
			opts_len -= opt_len;
			alloc_string(*pkt->buffer_pkt,pkt->switch_filter,
					    "     [Data hex ");
			for (opt_len -= 2; opt_len > 0; opt_len--)

alloc_string(*pkt->buffer_pkt,pkt->switch_filter,
					    "%.2x",*(++opt));
			alloc_string(*pkt->buffer_pkt,pkt->switch_filter,
				     "]\n");
			break;
		}
	}
out:
	/* cut off everything that is not part of IPv4 payload */
	pkt_trim(pkt, pkt_len(pkt) - min(pkt_len(pkt), (ntohs(ip->h_tot_len) -
			ip->h_ihl * sizeof(uint32_t))));
	pkt_set_proto(pkt, &eth_lay3, ip->h_protocol);
}

static inline void ipv4_less(struct pkt_buff *pkt)
{
	char src_ip[INET_ADDRSTRLEN];
	char dst_ip[INET_ADDRSTRLEN];
	struct ipv4hdr *ip = (struct ipv4hdr *) pkt_pull(pkt, sizeof(*ip));

	if (!ip)
		return;

	inet_ntop(AF_INET, &ip->h_saddr, src_ip, sizeof(src_ip));
	inet_ntop(AF_INET, &ip->h_daddr, dst_ip, sizeof(dst_ip));

	alloc_string(*pkt->buffer_pkt,pkt->switch_filter," %s/%s Len %u",
			src_ip, dst_ip,ntohs(ip->h_tot_len));

	/* cut off IP options and everything that is not part of IPv4 payload */
	pkt_pull(pkt, max((uint8_t) ip->h_ihl, sizeof(*ip) / sizeof(uint32_t))
		* sizeof(uint32_t) - sizeof(*ip));
	pkt_trim(pkt, pkt_len(pkt) - min(pkt_len(pkt), (ntohs(ip->h_tot_len) - ip->h_ihl * sizeof(uint32_t))));
	pkt_set_proto(pkt, &eth_lay3, ip->h_protocol);
}

struct protocol ipv4_ops = {
	.key = 0x0800,
	.print_full = ipv4,
	.print_less = ipv4_less,
};

#endif /* PROTO_IPV4_H */
