/*
 * Copyright (C) 2007 Delta Networks Inc.
 * It should be the last match in the netfilter rules to reduce
 * the number of rules.
 */

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/ip.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>
#include <net/route.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_fastlog.h>

#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

/* Use lock to serialize, so printks don't overlap */
static DEFINE_SPINLOCK(fastlog_lock);

/* The following function is derived from dump_packet() in ipt_LOG.c */
static void dump_packet(const struct sk_buff *skb,
			unsigned int iphoff)
{
	struct iphdr _iph, *ih;

	ih = skb_header_pointer(skb, iphoff, sizeof(_iph), &_iph);
	if (ih == NULL) {
		printk("TRUNCATED");
		return;
	}

	/* Important fields:
	 * TOS, len, DF/MF, fragment offset, TTL, src, dst, options. */
	/* Max length: 40 "SRC=255.255.255.255 DST=255.255.255.255 " */
	printk("SRC=%u.%u.%u.%u DST=%u.%u.%u.%u ",
	       NIPQUAD(ih->saddr), NIPQUAD(ih->daddr));

	/* Max length: 46 "LEN=65535 TOS=0xFF PREC=0xFF TTL=255 ID=65535 " */
	printk("LEN=%u TOS=0x%02X PREC=0x%02X TTL=%u ID=%u ",
	       ntohs(ih->tot_len), ih->tos & IPTOS_TOS_MASK,
	       ih->tos & IPTOS_PREC_MASK, ih->ttl, ntohs(ih->id));

	/* Max length: 6 "CE DF MF " */
	if (ntohs(ih->frag_off) & IP_CE)
		printk("CE ");
	if (ntohs(ih->frag_off) & IP_DF)
		printk("DF ");
	if (ntohs(ih->frag_off) & IP_MF)
		printk("MF ");

	/* Max length: 11 "FRAG:65535 " */
	if (ntohs(ih->frag_off) & IP_OFFSET)
		printk("FRAG:%u ", ntohs(ih->frag_off) & IP_OFFSET);

	switch (ih->protocol) {
	case IPPROTO_TCP: {
		struct tcphdr _tcph, *th;

		/* Max length: 10 "PROTO=TCP " */
		printk("PROTO=TCP ");

		if (ntohs(ih->frag_off) & IP_OFFSET)
			break;

		/* Max length: 25 "INCOMPLETE [65535 bytes] " */
		th = skb_header_pointer(skb, iphoff + ih->ihl * 4,
					sizeof(_tcph), &_tcph);
		if (th == NULL) {
			printk("INCOMPLETE [%u bytes] ",
			       skb->len - iphoff - ih->ihl*4);
			break;
		}

		/* Max length: 20 "SPT=65535 DPT=65535 " */
		printk("SPT=%u DPT=%u ",
		       ntohs(th->source), ntohs(th->dest));
		/* Max length: 13 "WINDOW=65535 " */
		printk("WINDOW=%u ", ntohs(th->window));
		/* Max length: 9 "RES=0x3F " */
		printk("RES=0x%02x ", (u8)(ntohl(tcp_flag_word(th) & TCP_RESERVED_BITS) >> 22));
		/* Max length: 32 "CWR ECE URG ACK PSH RST SYN FIN " */
		if (th->cwr)
			printk("CWR ");
		if (th->ece)
			printk("ECE ");
		if (th->urg)
			printk("URG ");
		if (th->ack)
			printk("ACK ");
		if (th->psh)
			printk("PSH ");
		if (th->rst)
			printk("RST ");
		if (th->syn)
			printk("SYN ");
		if (th->fin)
			printk("FIN ");
		/* Max length: 11 "URGP=65535 " */
		printk("URGP=%u ", ntohs(th->urg_ptr));

		break;
	}
	case IPPROTO_UDP: {
		struct udphdr _udph, *uh;

		/* Max length: 10 "PROTO=UDP " */
		printk("PROTO=UDP ");

		if (ntohs(ih->frag_off) & IP_OFFSET)
			break;

		/* Max length: 25 "INCOMPLETE [65535 bytes] " */
		uh = skb_header_pointer(skb, iphoff+ih->ihl*4,
					sizeof(_udph), &_udph);
		if (uh == NULL) {
			printk("INCOMPLETE [%u bytes] ",
			       skb->len - iphoff - ih->ihl*4);
			break;
		}

		/* Max length: 20 "SPT=65535 DPT=65535 " */
		printk("SPT=%u DPT=%u LEN=%u ",
		       ntohs(uh->source), ntohs(uh->dest),
		       ntohs(uh->len));
		break;
	}
	/* Max length: 10 "PROTO 255 " */
	default:
		printk("PROTO=%u ", ih->protocol);
	}


	/* Proto    Max log string length */
	/* IP:      40+46+6+11+127 = 230 */
	/* TCP:     10+max(25,20+30+13+9+32+11+127) = 252 */
	/* UDP:     10+max(25,20) = 35 */
	/* ICMP:    11+max(25, 18+25+max(19,14,24+3+n+10,3+n+10)) = 91+n */
	/* ESP:     10+max(25)+15 = 50 */
	/* AH:      9+max(25)+15 = 49 */
	/* unknown: 10 */

	/* (ICMP allows recursion one level deep) */
	/* maxlen =  IP + ICMP +  IP + max(TCP,UDP,ICMP,unknown) */
	/* maxlen = 230+   91  + 230 + 252 = 803 */
}

static int match(const struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		const void *matchinfo,
		int offset,
		int *hotdrop)
{
	if (net_ratelimit() == 0)
		return 1;

	struct iphdr *iph = skb->nh.iph;
	struct tcphdr *tcph = (void *)iph + iph->ihl*4;	/* Might be TCP, UDP */
	const struct ipt_fastlog_info *loginfo = matchinfo;
	int has_port = (iph->protocol == IPPROTO_TCP || iph->protocol == IPPROTO_UDP);
	
	if (has_port) {
		spin_lock_bh(&fastlog_lock);
		printk("%s ", loginfo->prefix);
		dump_packet(skb, 0);
		printk("\n");
		spin_unlock_bh(&fastlog_lock);
	}
	
	return 1;
}

static int checkentry(const char *tablename,
		const struct ipt_ip *ip,
		void *matchinfo,
		unsigned int matchsize,
		unsigned int hook_mask)
{
	struct ipt_fastlog_info *loginfo = (struct ipt_fastlog_info *)matchinfo;

	if (matchsize != IPT_ALIGN(sizeof(struct ipt_fastlog_info))) {
		DEBUGP("log: matchsize %u != %u\n", matchsize, IPT_ALIGN(sizeof(struct ipt_fastlog_info)));
		return 0;
	}

	if (loginfo->prefix[sizeof(loginfo->prefix)-1] != '\0') {
		DEBUGP("log: prefix term %i\n", loginfo->prefix[sizeof(loginfo->prefix)-1]);
		return 0;
	}

	return 1;
}

static struct ipt_match fastlog_match = {
	.name		= "fastlog",
	.match		= match,
	.checkentry	= checkentry,
	.me		= THIS_MODULE,
};

static int __init init(void)
{
	if (ipt_register_match(&fastlog_match))
		return -EINVAL;

	return 0;
}

static void __exit fini(void)
{
	ipt_unregister_match(&fastlog_match);
}

module_init(init);
module_exit(fini);
