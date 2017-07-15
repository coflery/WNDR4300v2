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
//  Filename:       ip_conntrack_proto_esp.c
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
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/seq_file.h>
#include <linux/in.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_core.h>
#include "nf_conntrack_esp.h"

static struct _esp_table esp_table[MAX_PORTS];

static void
esp_free_entry(int index)
{
    if (esp_table[index].inuse) {
        if (esp_table[index].timer_active) {
            pr_debug("free esp_entry (index) : %d\n", index);
            del_timer(&esp_table[index].refresh_timer);
        }
        memset(&esp_table[index], 0, sizeof(struct _esp_table));
    }
}

static void
esp_refresh_ct(unsigned long data)
{
    struct _esp_table *esp_entry = NULL;

    if (data > MAX_PORTS) {
        return;
    }

    esp_entry = &esp_table[data];
    if ( esp_entry == NULL ) {
        return;
    }
    pr_debug("ntimeouts %d pkt_rcvd %d entry %p data %lu ct %p\n",
           esp_entry->ntimeouts, esp_entry->pkt_rcvd, esp_entry, data, esp_entry->ct);
    if (esp_entry->pkt_rcvd) {
        esp_entry->pkt_rcvd  = 0;
        esp_entry->ntimeouts = 0;
    } else {
        esp_entry->ntimeouts++;
        if (esp_entry->ntimeouts >= ESP_TMOUT_COUNT) {
            esp_free_entry(data);
            return;
        }
    }
    esp_entry->refresh_timer.expires = jiffies + ESP_REF_TMOUT;
    esp_entry->refresh_timer.function = esp_refresh_ct;
    esp_entry->refresh_timer.data = data;
    add_timer(&esp_entry->refresh_timer);
    esp_entry->timer_active = 1;
    pr_debug("Refreshed timer pkt_rcvd %d timeouts %d\n",
           esp_entry->pkt_rcvd, esp_entry->ntimeouts);
}

/*
 * Allocate a free IPSEC table entry.
 */
struct _esp_table *alloc_esp_entry ( void )
{
	int idx = 0;
	struct _esp_table *esp_entry = esp_table;

	for ( ; idx < MAX_PORTS; idx++ ) {
		if ( esp_entry->inuse == IPSEC_FREE ) {
			esp_entry->tspi  = TEMP_SPI_START + idx;
			esp_entry->inuse = IPSEC_INUSE;
			pr_debug( "New esp_entry at idx %d entry %p tspi %u\n",
					idx, esp_entry, esp_entry->tspi );
			init_timer(&esp_entry->refresh_timer);
			esp_entry->refresh_timer.data     = idx;
			esp_entry->pkt_rcvd               = 0;
			esp_entry->refresh_timer.expires  = jiffies + ESP_REF_TMOUT;
			esp_entry->refresh_timer.function = esp_refresh_ct;
			add_timer(&esp_entry->refresh_timer);
			esp_entry->timer_active         = 1;
			return esp_entry;
		}
		esp_entry++;
	}
	return NULL;
}

/*
 * Search an ESP table entry by the Security Parameter Identifier (SPI).
 */
struct _esp_table *search_esp_entry_by_spi ( const struct esphdr *esph,
					     u_int32_t saddr, u_int32_t daddr )
{
	int idx = 0;
	struct _esp_table *esp_entry = esp_table;

	pr_debug( "(0x%x) %u.%u.%u.%u %u.%u.%u.%u\n", 
			ntohl(esph->spi), NIPQUAD(saddr), NIPQUAD(daddr) );

	for ( ; idx < MAX_PORTS; idx++, esp_entry++ ) {
		if ( esp_entry->inuse == IPSEC_FREE ) {
			continue;
		}
		/* If we have seen traffic both ways */
		if ( esp_entry->r_spi != 0 ) {
			if ( esp_entry->l_spi == ntohl(esph->spi) ||
				 esp_entry->r_spi == ntohl(esph->spi) ) {
				pr_debug("Both Ways Traffic Entry %p\n", esp_entry);
				return esp_entry;
			}
			continue;
		}

		/* If we have seen traffic only one way */
		if ( esp_entry->r_ip == ntohl(saddr) || esp_entry->l_ip == ntohl(daddr) ) {
			/* This must be the first packet from remote */
			esp_entry->r_spi = ntohl(esph->spi);
			pr_debug("First Packet from Remote Entry %p\n", esp_entry);
			return esp_entry;
		}
		if ( ntohl(esph->spi) == esp_entry->l_spi ) {
			pr_debug("One Way Traffic From Local Entry %p\n", esp_entry);
			return esp_entry;
		}
	}
	pr_debug("No Entry\n");
	return NULL;
}

static bool esp_pkt_to_tuple(const struct sk_buff *skb, unsigned int dataoff,
			    struct nf_conntrack_tuple *tuple)
{
	const struct esphdr *esph;
	struct esphdr _esph;
	struct _esp_table *esp_entry;

	if ((esph = skb_header_pointer(skb, dataoff, sizeof(_esph), &_esph)) == NULL){
		/* try to behave like "nf_conntrack_proto_generic" */
		tuple->src.u.all = 0;
		tuple->dst.u.all = 0;
		return true;
	}

	pr_debug("spi = 0x%x\n", ntohl(esph->spi));
	nf_ct_dump_tuple(tuple);

	esp_entry = search_esp_entry_by_spi(esph, tuple->src.u3.ip, tuple->dst.u3.ip);
	if (esp_entry == NULL) {
		esp_entry = alloc_esp_entry();
		if (esp_entry == NULL) {
			pr_debug("fail to alloc esp_entry.\n");
			return false;
		}
		esp_entry->l_spi = ntohl(esph->spi);
		esp_entry->l_ip  = ntohl(tuple->src.u3.ip);
		esp_entry->r_ip  = ntohl(tuple->dst.u3.ip);
		esp_entry->r_spi = 0;
	}
	pr_debug("tspi %u spi 0x%x seq 0x%x srcip %u.%u.%u.%u dstip %u.%u.%u.%u\n",
		   esp_entry->tspi, ntohl(esph->spi), ntohl(esph->seq),
		   NIPQUAD(tuple->src.u3.ip), NIPQUAD(tuple->dst.u3.ip));
	tuple->dst.u.esp_spi = esp_entry->tspi;
	tuple->src.u.esp_spi = esp_entry->tspi;
	esp_entry->pkt_rcvd++;
	return true;
}

static bool esp_invert_tuple(struct nf_conntrack_tuple *tuple,
			    const struct nf_conntrack_tuple *orig)
{
	tuple->src.u.esp_spi = orig->dst.u.esp_spi;
	tuple->dst.u.esp_spi = orig->src.u.esp_spi;
	return true;
}

/* Print out the per-protocol part of the tuple. */
static int esp_print_tuple(struct seq_file *s,
				    const struct nf_conntrack_tuple *tuple)
{
	return seq_printf(s, "srcspi=%u dstspi=%u ",
		       ntohs(tuple->src.u.esp_spi), ntohs(tuple->dst.u.esp_spi));
}

/* Print out the private part of the conntrack. */
static int esp_print_conntrack(struct seq_file *s, struct nf_conn *ct)
{
	return 0;
}

/* Returns verdict for packet, and may modify conntrack */
static int esp_packet(struct nf_conn *ct,
		      const struct sk_buff *skb,
		      unsigned int dataoff,
		      enum ip_conntrack_info ctinfo,
		      u_int8_t pf,
		      unsigned int hooknum)
{
	const struct esphdr *esph;
	struct esphdr _esph;

	if ((esph = skb_header_pointer(skb, dataoff, sizeof(_esph), &_esph)) == NULL)
		return NF_ACCEPT;

	pr_debug("spi = 0x%x, ct->status = %lu\n", ntohl(esph->spi), ct->status);

	if (ct->status & IPS_SEEN_REPLY) {
		nf_ct_refresh(ct, skb, ESP_CONN_TMOUT);
		set_bit(IPS_ASSURED_BIT, &ct->status);
	} else {
		nf_ct_refresh(ct, skb, ESP_REF_TMOUT);
   	}

	return NF_ACCEPT;
}

/* Called when a new connection for this protocol found. */
static bool esp_new(struct nf_conn *ct, const struct sk_buff *skb,
		   unsigned int dataoff)
{
	const struct esphdr *esph;
	struct esphdr _esph;

	if ((esph = skb_header_pointer(skb, dataoff, sizeof(_esph), &_esph)) == NULL)
		return true;

	pr_debug("spi = 0x%x\n", ntohl(esph->spi));
	nf_ct_dump_tuple(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);

	return true;
}

static struct nf_conntrack_l4proto nf_conntrack_l4proto_esp4 __read_mostly = {
	.l3proto         = AF_INET,
	.l4proto           = IPPROTO_ESP,
	.name            = "esp",
	.pkt_to_tuple    = esp_pkt_to_tuple,
	.invert_tuple    = esp_invert_tuple,
	.print_tuple     = esp_print_tuple,
	.print_conntrack = esp_print_conntrack,
	.packet          = esp_packet,
	.new             = esp_new,
	.me              = THIS_MODULE,
};

static int __init init(void)
{
    int rv;

    rv = nf_conntrack_l4proto_register(&nf_conntrack_l4proto_esp4);
    if (rv < 0)
        printk("nf_conntrack_ipv4: can't register esp.\n");
    else
        printk("nf_conntrack_proto_esp loaded\n");

    return rv;
}

static void __exit fini(void)
{
    nf_conntrack_l4proto_unregister(&nf_conntrack_l4proto_esp4);
}

module_init(init);
module_exit(fini);

MODULE_LICENSE("GPL");
