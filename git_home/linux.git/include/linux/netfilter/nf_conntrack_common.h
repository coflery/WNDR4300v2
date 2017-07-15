#ifndef _NF_CONNTRACK_COMMON_H
#define _NF_CONNTRACK_COMMON_H
/* Connection state tracking for netfilter.  This is separated from,
   but required by, the NAT layer; it can also be used by an iptables
   extension. */
enum ip_conntrack_info
{
	/* Part of an established connection (either direction). */
	IP_CT_ESTABLISHED,

	/* Like NEW, but related to an existing connection, or ICMP error
	   (in either direction). */
	IP_CT_RELATED,

	/* Started a new connection to track (only
           IP_CT_DIR_ORIGINAL); may be a retransmission. */
	IP_CT_NEW,

	/* >= this indicates reply direction */
	IP_CT_IS_REPLY,

	/* Number of distinct IP_CT types (no NEW in reply dirn). */
	IP_CT_NUMBER = IP_CT_IS_REPLY * 2 - 1
};

/* Bitset representing status of connection. */
enum ip_conntrack_status {
	/* It's an expected connection: bit 0 set.  This bit never changed */
	IPS_EXPECTED_BIT = 0,
	IPS_EXPECTED = (1 << IPS_EXPECTED_BIT),

	/* We've seen packets both ways: bit 1 set.  Can be set, not unset. */
	IPS_SEEN_REPLY_BIT = 1,
	IPS_SEEN_REPLY = (1 << IPS_SEEN_REPLY_BIT),

	/* Conntrack should never be early-expired. */
	IPS_ASSURED_BIT = 2,
	IPS_ASSURED = (1 << IPS_ASSURED_BIT),

	/* Connection is confirmed: originating packet has left box */
	IPS_CONFIRMED_BIT = 3,
	IPS_CONFIRMED = (1 << IPS_CONFIRMED_BIT),

	/* Connection needs src nat in orig dir.  This bit never changed. */
	IPS_SRC_NAT_BIT = 4,
	IPS_SRC_NAT = (1 << IPS_SRC_NAT_BIT),

	/* Connection needs dst nat in orig dir.  This bit never changed. */
	IPS_DST_NAT_BIT = 5,
	IPS_DST_NAT = (1 << IPS_DST_NAT_BIT),

	/* Both together. */
	IPS_NAT_MASK = (IPS_DST_NAT | IPS_SRC_NAT),

	/* Connection needs TCP sequence adjusted. */
	IPS_SEQ_ADJUST_BIT = 6,
	IPS_SEQ_ADJUST = (1 << IPS_SEQ_ADJUST_BIT),

	/* NAT initialization bits. */
	IPS_SRC_NAT_DONE_BIT = 7,
	IPS_SRC_NAT_DONE = (1 << IPS_SRC_NAT_DONE_BIT),

	IPS_DST_NAT_DONE_BIT = 8,
	IPS_DST_NAT_DONE = (1 << IPS_DST_NAT_DONE_BIT),

	/* Both together */
	IPS_NAT_DONE_MASK = (IPS_DST_NAT_DONE | IPS_SRC_NAT_DONE),

	/* Connection is dying (removed from lists), can not be unset. */
	IPS_DYING_BIT = 9,
	IPS_DYING = (1 << IPS_DYING_BIT),

	/* Connection has fixed timeout. */
	IPS_FIXED_TIMEOUT_BIT = 10,
	IPS_FIXED_TIMEOUT = (1 << IPS_FIXED_TIMEOUT_BIT),

#ifdef CONFIG_ATHRS_HW_NAT

        /* Marked when a ct/nat help owns this pkt */
        IPS_NAT_ALG_PKT_BIT = 11,
        IPS_NAT_ALG_PKT = (1 << IPS_NAT_ALG_PKT_BIT),

        /* Marked when the tuple is added to the h/w nat */
        IPS_ATHR_HW_NAT_ADDED_BIT = 12,
        IPS_ATHR_HW_NAT_ADDED = (1 << IPS_ATHR_HW_NAT_ADDED_BIT),

        /* Marked when the tuple is added to the h/w nat for a UDP pkt*/
        IPS_ATHR_HW_NAT_IS_UDP_BIT = 13,
        IPS_ATHR_HW_NAT_IS_UDP = (1 << IPS_ATHR_HW_NAT_IS_UDP_BIT),

        /* Marked when the tuple is added to the h/w nat for a UDP pkt*/
        IPS_ATHR_HW_NAT_IS_ONLY_EGRESS_BIT = 14,
        IPS_ATHR_HW_NAT_IS_ONLY_EGRESS = (1 << IPS_ATHR_HW_NAT_IS_ONLY_EGRESS_BIT),

        /* Marked when the tuple is added to the h/w nat for a UDP pkt*/
        IPS_ATHR_SW_NAT_SKIPPED_BIT = 15,
        IPS_ATHR_SW_NAT_SKIPPED = (1 << IPS_ATHR_SW_NAT_SKIPPED_BIT),

        /*
         * Addded for nat frag table fast hash entry lookup
         */

	IPS_ATHR_HW_CT_INGRESS_BIT = 16,
	IPS_ATHR_HW_CT_INGRESS = (1 << IPS_ATHR_HW_CT_INGRESS_BIT),

	IPS_ATHR_HW_CT_EGRESS_BIT = 17,
	IPS_ATHR_HW_CT_EGRESS = (1 << IPS_ATHR_HW_CT_EGRESS_BIT),

	/*added for hw nat, mark ct when packet go through unsupported layer2 interface*/
	IPS_ATHR_HW_SRC_NAT_L2NOSUPPORT_BIT = 18,
	IPS_ATHR_HW_SRC_NAT_L2NOSUPPORT = (1 << IPS_ATHR_HW_SRC_NAT_L2NOSUPPORT_BIT),

	IPS_ATHR_HW_DST_NAT_L2NOSUPPORT_BIT = 19,
	IPS_ATHR_HW_DST_NAT_L2NOSUPPORT = (1 << IPS_ATHR_HW_DST_NAT_L2NOSUPPORT_BIT),
#endif
	/* Create conntrack from wan interface */
	IPS_WAN_IN_BIT = 11,
	IPS_WAN_IN = (1 << IPS_WAN_IN_BIT),

	IPS_CONENAT_BIT = 12,
	IPS_CONENAT = (1<< IPS_CONENAT_BIT),

	IPS_TRIGGER_BIT = 13,
	IPS_TRIGGER	= (1 << IPS_TRIGGER_BIT),

	IPS_SPI_DoS_BIT = 14,
	IPS_SPI_DoS	= (1 << IPS_SPI_DoS_BIT),

	IPS_SNATP2P_SRC_BIT = 15,
	IPS_SNATP2P_SRC = (1 << IPS_SNATP2P_SRC_BIT),

	IPS_SNATP2P_DST_BIT = 16,
	IPS_SNATP2P_DST = (1 << IPS_SNATP2P_DST_BIT),

	/* Both together. */
	IPS_SNATP2P_MASK = (IPS_SNATP2P_DST | IPS_SNATP2P_SRC),

	IPS_SNATP2P_SRC_DONE_BIT = 17,
	IPS_SNATP2P_SRC_DONE = (1 << IPS_SNATP2P_SRC_DONE_BIT),

	IPS_SNATP2P_DST_DONE_BIT = 18,
	IPS_SNATP2P_DST_DONE = (1 << IPS_SNATP2P_DST_DONE_BIT),

	/* Both together. */
	IPS_SNATP2P_DONE_MASK = (IPS_SNATP2P_DST_DONE | IPS_SNATP2P_SRC_DONE),

#if defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT) || defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT_MODULE)
	/* [NETGEAR SPEC 2.0] 1.10 NAT Session Management */
	IPS_NAT_STATIC_HIGH_PRIORITY_BIT = 19,
	IPS_NAT_STATIC_HIGH_PRIORITY = (1 << IPS_NAT_STATIC_HIGH_PRIORITY_BIT),
#endif

	/*
	 * In Netgear's unofficial "Home Wireless Router IPv6 Spec"
	 * (it will be merged into Home Wireless Router Spec V1.10
	 * according to Netgear), IPv6 SPI Firewall does not have NAT on IPv6,
	 * and there are two routing filtering modes:
	 * Secured Mode (default) and Open Mode.
	 */
	IPS_IPV6_ROUTING_FILTERING_BIT = 20,
	IPS_IPV6_ROUTING_FILTERING = (1 << IPS_IPV6_ROUTING_FILTERING_BIT),

	/* Refresh the idle time of ALG data session's master conntrack */
	IPS_ALG_REFRESH_BIT = 21,
	IPS_ALG_REFRESH = (1 << IPS_ALG_REFRESH_BIT),

#if defined(CONFIG_NF_CONNTRACK_LOCAL_MANAGEMENT)
	IPS_CT_ICMP_RESERVE_BIT = 22,
	IPS_CT_ICMP_RESERVE = (1 << IPS_CT_ICMP_RESERVE_BIT),
	IPS_CT_TCP_RESERVE_BIT = 23,
	IPS_CT_TCP_RESERVE = (1 << IPS_CT_TCP_RESERVE_BIT),
#endif
#if defined(CONFIG_IP_NF_TARGET_SNATP2P) || defined(CONFIG_IP_NF_TARGET_SNATP2P_MODULE) || defined(CONFIG_IP_NF_TARGET_HAIRPIN) || defined(CONFIG_IP_NF_TARGET_HAIRPIN_MODULE)
	IPS_PORT_FULL_BIT = 24,
	IPS_PORT_FULL = (1 << IPS_PORT_FULL_BIT),
#endif
};

#ifdef __KERNEL__
struct ip_conntrack_stat
{
	unsigned int searched;
	unsigned int found;
	unsigned int new;
	unsigned int invalid;
	unsigned int ignore;
	unsigned int delete;
	unsigned int delete_list;
	unsigned int insert;
	unsigned int insert_failed;
	unsigned int drop;
	unsigned int early_drop;
	unsigned int error;
	unsigned int expect_new;
	unsigned int expect_create;
	unsigned int expect_delete;
};

/* call to create an explicit dependency on nf_conntrack. */
extern void need_conntrack(void);

#endif /* __KERNEL__ */

#endif /* _NF_CONNTRACK_COMMON_H */
