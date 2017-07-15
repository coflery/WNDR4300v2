/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 * (C) 2008 Patrick McHardy <kaber@trash.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/random.h>
#include <linux/ip.h>

#include <linux/netfilter.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_core.h>
#include <net/netfilter/nf_nat_rule.h>
#include <net/netfilter/nf_nat_protocol.h>

bool nf_nat_proto_in_range(const struct nf_conntrack_tuple *tuple,
			   enum nf_nat_manip_type maniptype,
			   const union nf_conntrack_man_proto *min,
			   const union nf_conntrack_man_proto *max)
{
	__be16 port;

	if (maniptype == IP_NAT_MANIP_SRC)
		port = tuple->src.u.all;
	else
		port = tuple->dst.u.all;

	return ntohs(port) >= ntohs(min->all) &&
	       ntohs(port) <= ntohs(max->all);
}
EXPORT_SYMBOL_GPL(nf_nat_proto_in_range);

#if defined(CONFIG_IP_NF_TARGET_SNATP2P) || defined(CONFIG_IP_NF_TARGET_SNATP2P_MODULE) || defined(CONFIG_IP_NF_TARGET_HAIRPIN) || defined(CONFIG_IP_NF_TARGET_HAIRPIN_MODULE)
unsigned int udp_tcp_port_flags = 0;
#endif

bool nf_nat_proto_unique_tuple(struct nf_conntrack_tuple *tuple,
			       const struct nf_nat_range *range,
			       enum nf_nat_manip_type maniptype,
			       const struct nf_conn *ct,
			       u_int16_t *rover)
{
	unsigned int range_size, min, i;
	__be16 *portptr;
	u_int16_t off;
#if defined(CONFIG_IP_NF_TARGET_SNATP2P) || defined(CONFIG_IP_NF_TARGET_SNATP2P_MODULE) || defined(CONFIG_IP_NF_TARGET_HAIRPIN) || defined(CONFIG_IP_NF_TARGET_HAIRPIN_MODULE)
	unsigned int ports_flag = 0;
#endif


	if (maniptype == IP_NAT_MANIP_SRC)
		portptr = &tuple->src.u.all;
	else
		portptr = &tuple->dst.u.all;

	/* If no range specified... */
	if (!(range->flags & IP_NAT_RANGE_PROTO_SPECIFIED)) {
		/* If it's dst rewrite, can't change port */
		if (maniptype == IP_NAT_MANIP_DST)
			return false;

		if (ntohs(*portptr) < 1024) {
			/* Loose convention: >> 512 is credential passing */
			if (ntohs(*portptr) < 512) {
				min = 1;
				range_size = 511 - min + 1;
			} else {
				min = 600;
				range_size = 1023 - min + 1;
			}
		} else {
#if defined(CONFIG_IP_NF_TARGET_SNATP2P) || defined(CONFIG_IP_NF_TARGET_SNATP2P_MODULE) || defined(CONFIG_IP_NF_TARGET_HAIRPIN) || defined(CONFIG_IP_NF_TARGET_HAIRPIN_MODULE)
			min = ntohs(*portptr) & 0x1 ? 49153 : 49152;
			range_size = 65535 - min + 1;
			ports_flag = 1;
#else
			min = 1024;
			range_size = 65535 - 1024 + 1;
#endif
		}
	} else {
		min = ntohs(range->min.all);
		range_size = ntohs(range->max.all) - min + 1;
	}

	if (range->flags & IP_NAT_RANGE_PROTO_RANDOM)
		off = secure_ipv4_port_ephemeral(tuple->src.u3.ip, tuple->dst.u3.ip,
						 maniptype == IP_NAT_MANIP_SRC
						 ? tuple->dst.u.all
						 : tuple->src.u.all);
	else
		off = *rover;

#if defined(CONFIG_IP_NF_TARGET_SNATP2P) || defined(CONFIG_IP_NF_TARGET_SNATP2P_MODULE) || defined(CONFIG_IP_NF_TARGET_HAIRPIN) || defined(CONFIG_IP_NF_TARGET_HAIRPIN_MODULE)
	if (ports_flag && (maniptype == IP_NAT_MANIP_SRC) && (ct->status & IPS_SNATP2P_MASK)) {
		if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_UDP) {
			if (udp_tcp_port_flags & (min & 0x1 ? IP_NAT_UDP_ODD_PORT_FULL : IP_NAT_UDP_EVEN_PORT_FULL)) {
				set_bit(IPS_PORT_FULL_BIT, &ct->status);
				return false;
			}
		} else if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_TCP) {
			if (udp_tcp_port_flags & (min & 0x1 ? IP_NAT_TCP_ODD_PORT_FULL : IP_NAT_TCP_EVEN_PORT_FULL)) {
				set_bit(IPS_PORT_FULL_BIT, &ct->status);
				return false;
			}
		}
	}

	for (i = 0; i < range_size; i += 2, off += 2)
	{
		if (off >= range_size)
			off = 0;
		*portptr = htons(min + off);
#else
	for (i = 0; i < range_size; i++, off++)
	{
		*portptr = htons(min + off % range_size);
#endif
		if (nf_nat_used_tuple(tuple, ct))
			continue;
		if (!(range->flags & IP_NAT_RANGE_PROTO_RANDOM))
			*rover = off;
		return true;
	}

#if defined(CONFIG_IP_NF_TARGET_SNATP2P) || defined(CONFIG_IP_NF_TARGET_SNATP2P_MODULE) || defined(CONFIG_IP_NF_TARGET_HAIRPIN) || defined(CONFIG_IP_NF_TARGET_HAIRPIN_MODULE)
	if (ports_flag && (maniptype == IP_NAT_MANIP_SRC) && (ct->status & IPS_SNATP2P_MASK)) {
		if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_UDP) {
			udp_tcp_port_flags |= (min & 0x1 ? IP_NAT_UDP_ODD_PORT_FULL : IP_NAT_UDP_EVEN_PORT_FULL);
			set_bit(IPS_PORT_FULL_BIT, &ct->status);
		} else if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_TCP) {
			udp_tcp_port_flags |= (min & 0x1 ? IP_NAT_TCP_ODD_PORT_FULL : IP_NAT_TCP_EVEN_PORT_FULL);
			set_bit(IPS_PORT_FULL_BIT, &ct->status);
		}
	}
#endif

	return false;
}
EXPORT_SYMBOL_GPL(nf_nat_proto_unique_tuple);

#if defined(CONFIG_NF_CT_NETLINK) || defined(CONFIG_NF_CT_NETLINK_MODULE)
int nf_nat_proto_range_to_nlattr(struct sk_buff *skb,
				 const struct nf_nat_range *range)
{
	NLA_PUT_BE16(skb, CTA_PROTONAT_PORT_MIN, range->min.all);
	NLA_PUT_BE16(skb, CTA_PROTONAT_PORT_MAX, range->max.all);
	return 0;

nla_put_failure:
	return -1;
}
EXPORT_SYMBOL_GPL(nf_nat_proto_nlattr_to_range);

int nf_nat_proto_nlattr_to_range(struct nlattr *tb[],
				 struct nf_nat_range *range)
{
	if (tb[CTA_PROTONAT_PORT_MIN]) {
		range->min.all = nla_get_be16(tb[CTA_PROTONAT_PORT_MIN]);
		range->max.all = range->min.tcp.port;
		range->flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
	}
	if (tb[CTA_PROTONAT_PORT_MAX]) {
		range->max.all = nla_get_be16(tb[CTA_PROTONAT_PORT_MAX]);
		range->flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(nf_nat_proto_range_to_nlattr);
#endif
