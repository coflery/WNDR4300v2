#ifndef __LINUX_BRIDGE_NETFILTER_H
#define __LINUX_BRIDGE_NETFILTER_H

/* bridge-specific defines for netfilter. 
 */

#include <linux/netfilter.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/if_pppox.h>

/* Bridge Hooks */
/* After promisc drops, checksum checks. */
#define NF_BR_PRE_ROUTING	0
/* If the packet is destined for this box. */
#define NF_BR_LOCAL_IN		1
/* If the packet is destined for another interface. */
#define NF_BR_FORWARD		2
/* Packets coming from a local process. */
#define NF_BR_LOCAL_OUT		3
/* Packets about to hit the wire. */
#define NF_BR_POST_ROUTING	4
/* Not really a hook, but used for the ebtables broute table */
#define NF_BR_BROUTING		5
#define NF_BR_NUMHOOKS		6

#ifdef __KERNEL__

#ifdef CONFIG_ATH_WRAP
#define PTYPE_MASK					0xffff
#define WRAP_PTYPE					0x1000
#define PTYPE_IS_WRAP(__type)		(__type & WRAP_PTYPE)?1:0		
/*Special mark used to detect bridge flooded pkt*/
#define WRAP_BR_MARK_FLOOD			0x8001
/*WRAP port attributes*/
#define WRAP_PTYPE_ISO				0x10000
#define WRAP_PTYPE_HAS_ISO(__type)	(__type & WRAP_PTYPE_ISO)?1:0		
/*WRAP port types*/
#define WRAP_PTYPE_ETH				0x1000	/*Port type ethernet*/
#define WRAP_PTYPE_VAP				0x1001	/*Port type VAP*/
#define WRAP_PTYPE_PETH				0x1003	/*Port type proxy ethernet*/
#define WRAP_PTYPE_PVAP				0x1004  /*Port type proxy VAP*/
#define WRAP_PTYPE_MPVAP			0x1005  /*Port type main proxy VAP*/
#define WRAP_PTYPE_PETH_ISO			(WRAP_PTYPE_PETH | WRAP_PTYPE_ISO)  /*Port type proxy ethernet with isolation*/
#define WRAP_PTYPE_PVAP_ISO			(WRAP_PTYPE_PVAP | WRAP_PTYPE_ISO)  /*Port type proxy VAP with isolation*/
#endif

enum nf_br_hook_priorities {
	NF_BR_PRI_FIRST = INT_MIN,
	NF_BR_PRI_NAT_DST_BRIDGED = -300,
	NF_BR_PRI_FILTER_BRIDGED = -200,
	NF_BR_PRI_BRNF = 0,
	NF_BR_PRI_NAT_DST_OTHER = 100,
	NF_BR_PRI_FILTER_OTHER = 200,
	NF_BR_PRI_NAT_SRC = 300,
	NF_BR_PRI_LAST = INT_MAX,
};

#ifdef CONFIG_BRIDGE_NETFILTER

#define BRNF_PKT_TYPE			0x01
#define BRNF_BRIDGED_DNAT		0x02
#define BRNF_DONT_TAKE_PARENT		0x04
#define BRNF_BRIDGED			0x08
#define BRNF_NF_BRIDGE_PREROUTING	0x10


/* Only used in br_forward.c */
extern int nf_bridge_copy_header(struct sk_buff *skb);
static inline int nf_bridge_maybe_copy_header(struct sk_buff *skb)
{
	if (skb->nf_bridge &&
	    skb->nf_bridge->mask & (BRNF_BRIDGED | BRNF_BRIDGED_DNAT))
		return nf_bridge_copy_header(skb);
  	return 0;
}

static inline unsigned int nf_bridge_encap_header_len(const struct sk_buff *skb)
{
	switch (skb->protocol) {
	case __cpu_to_be16(ETH_P_8021Q):
		return VLAN_HLEN;
	case __cpu_to_be16(ETH_P_PPP_SES):
		return PPPOE_SES_HLEN;
	default:
		return 0;
	}
}

/* This is called by the IP fragmenting code and it ensures there is
 * enough room for the encapsulating header (if there is one). */
static inline unsigned int nf_bridge_pad(const struct sk_buff *skb)
{
	if (skb->nf_bridge)
		return nf_bridge_encap_header_len(skb);
	return 0;
}

struct bridge_skb_cb {
	union {
		__be32 ipv4;
	} daddr;
};
#ifdef CONFIG_ATH_WRAP
int br_get_port_type(struct net_bridge_port *port);
#endif
#else
#define nf_bridge_maybe_copy_header(skb)	(0)
#define nf_bridge_pad(skb)			(0)
#endif /* CONFIG_BRIDGE_NETFILTER */

#endif /* __KERNEL__ */
#endif
