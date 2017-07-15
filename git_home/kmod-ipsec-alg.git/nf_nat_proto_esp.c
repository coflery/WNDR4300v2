/*
<:copyright-gpl
 Copyright 2002 Broadcom Corp. All Rights Reserved.

 This program is free software; you can distribute it and/or modify it
 under the terms of the GNU General Public License (Version 2) as
 published by the Free Software Foundation.

 This program is distributed in the hope it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
:>
*/
/******************************************************************************
//
//  Filename:       ip_nat_proto_esp.c
//  Author:         Pavan Kumar
//  Creation Date:  05/27/04
//  Modified by Vincent Yang at DNI. Taiwan 2009/11/27
//
//  Description:
//      Implements the ESP ALG connectiontracking.
//
*****************************************************************************/
//#define DEBUG
#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ip.h>

#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_rule.h>
#include <net/netfilter/nf_nat_protocol.h>
#include "nf_conntrack_esp.h"

static bool
esp_in_range(const struct nf_conntrack_tuple *tuple,
	     enum nf_nat_manip_type maniptype,
	     const union nf_conntrack_man_proto *min,
	     const union nf_conntrack_man_proto *max)
{
	return 1;
}

static bool
esp_unique_tuple(struct nf_conntrack_tuple *tuple,
                 const struct nf_nat_range *range,
                 enum nf_nat_manip_type maniptype,
                 const struct nf_conn *ct)
{
	pr_debug("manitype %d srcip %u.%u.%u.%u dstip %u.%u.%u.%u srcspi %u dstspi %u\n",
			maniptype, NIPQUAD(tuple->src.u3.ip), NIPQUAD(tuple->dst.u3.ip),
			tuple->src.u.esp_spi, tuple->dst.u.esp_spi );
	return 1;
}

static bool
esp_manip_pkt(struct sk_buff *skb, unsigned int iphdroff,	
              const struct nf_conntrack_tuple *tuple,			  
              enum nf_nat_manip_type maniptype)
{
	u_int32_t oldip;
	const struct iphdr *iph = (struct iphdr *)(skb->data + iphdroff);
	unsigned int hdroff = iphdroff + iph->ihl * 4;
	struct esphdr *hdr = (void *)skb->data + hdroff;

	if (maniptype == IP_NAT_MANIP_SRC) {
		/* Get rid of src ip and src pt */
		oldip = iph->saddr;
		pr_debug("MANIP_SRC oldip %u.%u.%u.%u dstip %u.%u.%u.%u manip %u.%u.%u.%u "
			   "spi 0x%x seq 0x%x\n",
			   NIPQUAD(oldip), NIPQUAD(iph->daddr), NIPQUAD(tuple->src.u3.ip),
			   ntohl(hdr->spi), ntohl(hdr->seq) );
	} else {
		/* Get rid of dst ip and dst pt */
		oldip = iph->daddr;
		pr_debug("MANIP_DST oldip %u.%u.%u.%u srcip %u.%u.%u.%u manip %u.%u.%u.%u "
			   "spi 0x%x seq 0x%x\n",
			   NIPQUAD(oldip), NIPQUAD(iph->saddr), NIPQUAD(tuple->dst.u3.ip),
			   ntohl(hdr->spi), ntohl(hdr->seq) );
	}
	return true;
}

static struct nf_nat_protocol nf_nat_protocol_esp = {
	.protonum     = IPPROTO_ESP,
	.me           = THIS_MODULE,
	.manip_pkt    = esp_manip_pkt,
	.in_range     = esp_in_range,
	.unique_tuple = esp_unique_tuple,
};

static int __init init(void)
{
    return nf_nat_protocol_register(&nf_nat_protocol_esp);
}

static void __exit fini(void)
{
	nf_nat_protocol_unregister(&nf_nat_protocol_esp);
}

module_init(init);
module_exit(fini);

MODULE_LICENSE("GPL");
