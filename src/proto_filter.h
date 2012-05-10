/*
 * netsniff-ng - the packet sniffing beast
 * Copyright (C) 2012 Markus Amend <markus@netsniff-ng.org>
 * Subject to the GPL, version 2.
 */

#ifndef PROTO_FILTER_H
#define PROTO_FILTER_H

/* Initialized in dissector.c -> dissector_main() */

struct tcp_filter {
  uint8_t tcp;
  uint16_t dest_port;
  uint16_t src_port;
};

struct ipv4_filter {
  uint8_t ipv4;
  uint8_t proto;
};

struct eth_filter {
  uint8_t eth;
  uint16_t proto;
};

struct filter_all {
  struct ipv4_filter ip4;
  struct eth_filter eth;
  struct tcp_filter tcp;
};

#endif /* PROTO_FILTER_H */
