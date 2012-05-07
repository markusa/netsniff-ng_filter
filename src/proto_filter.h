/*
 * netsniff-ng - the packet sniffing beast
 * Copyright (C) 2012 Markus Amend <markus@netsniff-ng.org>
 * Subject to the GPL, version 2.
 */

#ifndef PROTO_FILTER_H
#define PROTO_FILTER_H

struct ipv4_filter {
  uint8_t ipv4;
} __packed;

struct filter_all {
  struct ipv4_filter ip4;
} _packed;

#endif /* PROTO_FILTER_H */