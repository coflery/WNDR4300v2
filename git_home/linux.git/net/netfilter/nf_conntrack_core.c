/* Connection state tracking for netfilter.  This is separated from,
   but required by, the NAT layer; it can also be used by an iptables
   extension. */

/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 * (C) 2003,2004 USAGI/WIDE Project <http://www.linux-ipv6.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/netfilter.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/stddef.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/jhash.h>
#include <linux/err.h>
#include <linux/percpu.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/socket.h>
#include <linux/mm.h>
#include <linux/rculist_nulls.h>
#include <linux/ip.h>

#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_l3proto.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_extend.h>
#include <net/netfilter/nf_conntrack_acct.h>
#include <net/netfilter/nf_conntrack_ecache.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_core.h>

#define NF_CONNTRACK_VERSION	"0.5.0"

#define print_tuple(x) \
printk("%s:%d reply src:%d reply dst:%d org src:%d org dst:%d\n", __FILE__, __LINE__, \
 	   x->src.u.all, x->dst.u.all);

int (*nfnetlink_parse_nat_setup_hook)(struct nf_conn *ct,
				      enum nf_nat_manip_type manip,
				      struct nlattr *attr) __read_mostly;
EXPORT_SYMBOL_GPL(nfnetlink_parse_nat_setup_hook);

DEFINE_SPINLOCK(nf_conntrack_lock);
EXPORT_SYMBOL_GPL(nf_conntrack_lock);

unsigned int nf_conntrack_htable_size __read_mostly;
EXPORT_SYMBOL_GPL(nf_conntrack_htable_size);

unsigned int nf_conntrack_max __read_mostly;
EXPORT_SYMBOL_GPL(nf_conntrack_max);

struct nf_conn nf_conntrack_untracked __read_mostly;
EXPORT_SYMBOL_GPL(nf_conntrack_untracked);

#if defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT) || defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT_MODULE)

static unsigned long *tcp_high_prio_port_table;
static unsigned long *udp_high_prio_port_table;

#define PORT_TCP	(1 << 0)
#define PORT_UDP	(1 << 1)
#define PORT_BOTH	(PORT_TCP | PORT_UDP)

#if 0
#define NAT_LAN_DEBUGP printk
#else
#define NAT_LAN_DEBUGP(format, args...)
#endif

struct high_port_item
{
	char *name;

	__u16 start, end;

	unsigned int proto;
};

struct high_port_item ports_table[] = {
	/* The session is initiated from the inside and its destination TCP/UDP
	 * port number is below 1024 (i.e. 1 ~ 1023). */
	{ "Well Known Port", 1, 1023, PORT_BOTH },

	{ "PPTP", 1723, 1723, PORT_TCP },
	{ "L2TP", 1701, 1701, PORT_UDP },
	{ "H.323",1720, 1720, PORT_TCP },
	{ "SIP",  5060, 5060, PORT_BOTH },
	{ "AIM",  5190, 5190, PORT_BOTH },

	{ "NetMeeting",	389, 389, PORT_TCP },
	{ "NetMeeting",	522, 522, PORT_TCP },
	{ "NetMeeting",	1503, 1503, PORT_TCP },
	{ "NetMeeting",	1720, 1720, PORT_TCP },
	{ "NetMeeting",	1731, 1731, PORT_TCP },

	{ "Yahoo Messenger instant messenging",	5050, 5050, PORT_TCP },	
	{ "Yahoo Messenger voice chat",	5000, 5010, PORT_BOTH },
	{ "Yahoo Messenger video(Webcams)",	5100, 5100, PORT_BOTH },

	{ "MSN instant messenging", 1863, 1863, PORT_TCP },
	{ "MSN Whiteboard and Application Sharing", 1503, 1503, PORT_BOTH },
	{ "MSN voice (computer to phone)", 2001, 2120, PORT_UDP },
	{ "MSN voice (computer to phone)", 6801, 6801, PORT_UDP },
	{ "MSN voice (computer to phone)", 6901, 6901, PORT_UDP },
	{ "MSN file transfers", 6891, 6900, PORT_TCP },
	{ "MSN voice communications (computer to computer)", 6901, 6901, PORT_BOTH },

	{ "Skype", 80, 80, PORT_TCP },
	{ "Skype", 443, 443, PORT_TCP },	

	{ "Starcraft", 6112, 6112, PORT_BOTH },
	{ "Kazaa", 1214, 1214, PORT_TCP },
	{ "Gnutella", 6346, 6347, PORT_BOTH },

	{ "BitTorrent",	6881, 6889, PORT_TCP },
	{ "BitTorrent", 6969, 6969, PORT_TCP },

	{ "QQ Server", 4000, 4000, PORT_TCP },
	{ "QQ Client", 8000, 8000, PORT_TCP },


	{ NULL, 0, 0, 0 } /* The End! */
};


static void init_high_prio_tables(void)
{
	__u16 port;
	struct high_port_item *item;

	for (item = &ports_table[0]; item->name != NULL; item++) {
		NAT_LAN_DEBUGP("Adding high priority port %d~%d %s %s %s\n",
				item->start,
				item->end,
				item->name,
				item->proto & PORT_TCP ? "TCP" : "",
				item->proto & PORT_UDP ? "UDP" : "");
		for (port = item->start; port <= item->end; port++) {
			if (item->proto & PORT_TCP)
				__set_bit(port, tcp_high_prio_port_table);
			if (item->proto & PORT_UDP)
				__set_bit(port, udp_high_prio_port_table);
		}
	}
}

#if defined(CONFIG_NF_CONNTRACK_LOCAL_MANAGEMENT)
extern unsigned int nf_conntrack_local_max;
extern atomic_t nf_conntrack_local_count;
extern unsigned int nf_conntrack_icmp_reserve_max;
extern atomic_t nf_conntrack_icmp_reserve_count;
extern unsigned int nf_conntrack_tcp_reserve_max;
extern atomic_t nf_conntrack_tcp_reserve_count;
#endif

#endif

#if defined(CONFIG_NF_CONNTRACK_CONENAT_MANAGEMENT)
struct hlist_nulls_head *conenat_ct_hash;
EXPORT_SYMBOL(conenat_ct_hash);
int conenat_ct_hash_vmalloc = 0;
EXPORT_SYMBOL(conenat_ct_hash_vmalloc);
#endif

static struct kmem_cache *nf_conntrack_cachep __read_mostly;

static int nf_conntrack_hash_rnd_initted;
static unsigned int nf_conntrack_hash_rnd;

extern int sysctl_nat_filtering_behavior;

#ifdef CONFIG_ATHRS_HW_NAT
athr_nf_nat_ops_t *athr_nat_sw_ops;
EXPORT_SYMBOL_GPL(athr_nat_sw_ops);
#endif
#if defined(CONFIG_ATHRS_HW_NAT)
uint32_t hash_conntrack(const struct nf_conntrack_tuple *tuple);
EXPORT_SYMBOL(hash_conntrack);
#endif

#if defined(CONFIG_NF_CONNTRACK_NAT_UDP_MANAGEMENT)
struct hlist_nulls_head *dni_ct_hash;
EXPORT_SYMBOL(dni_ct_hash);
int dni_ct_hash_vmalloc = 0;
EXPORT_SYMBOL(dni_ct_hash_vmalloc);
#endif

static u_int32_t __hash_conntrack(const struct nf_conntrack_tuple *tuple,
				  unsigned int size, unsigned int rnd)
{
	unsigned int n;
	u_int32_t h;

	/* The direction must be ignored, so we hash everything up to the
	 * destination ports (which is a multiple of 4) and treat the last
	 * three bytes manually.
	 */
	n = (sizeof(tuple->src) + sizeof(tuple->dst.u3)) / sizeof(u32);
	h = jhash2((u32 *)tuple, n,
		   rnd ^ (((__force __u16)tuple->dst.u.all << 16) |
			  tuple->dst.protonum));

	return ((u64)h * size) >> 32;
}

#if defined(CONFIG_ATHRS_HW_NAT)
uint32_t hash_conntrack(const struct nf_conntrack_tuple *tuple)
#else
static inline u_int32_t hash_conntrack(const struct nf_conntrack_tuple *tuple)
#endif
{
	return __hash_conntrack(tuple, nf_conntrack_htable_size,
				nf_conntrack_hash_rnd);
}

bool
nf_ct_get_tuple(const struct sk_buff *skb,
		unsigned int nhoff,
		unsigned int dataoff,
		u_int16_t l3num,
		u_int8_t protonum,
		struct nf_conntrack_tuple *tuple,
		const struct nf_conntrack_l3proto *l3proto,
		const struct nf_conntrack_l4proto *l4proto)
{
	memset(tuple, 0, sizeof(*tuple));

	tuple->src.l3num = l3num;
	if (l3proto->pkt_to_tuple(skb, nhoff, tuple) == 0)
		return false;

	tuple->dst.protonum = protonum;
	tuple->dst.dir = IP_CT_DIR_ORIGINAL;

	return l4proto->pkt_to_tuple(skb, dataoff, tuple);
}
EXPORT_SYMBOL_GPL(nf_ct_get_tuple);

bool nf_ct_get_tuplepr(const struct sk_buff *skb, unsigned int nhoff,
		       u_int16_t l3num, struct nf_conntrack_tuple *tuple)
{
	struct nf_conntrack_l3proto *l3proto;
	struct nf_conntrack_l4proto *l4proto;
	unsigned int protoff;
	u_int8_t protonum;
	int ret;

	rcu_read_lock();

	l3proto = __nf_ct_l3proto_find(l3num);
	ret = l3proto->get_l4proto(skb, nhoff, &protoff, &protonum);
	if (ret != NF_ACCEPT) {
		rcu_read_unlock();
		return false;
	}

	l4proto = __nf_ct_l4proto_find(l3num, protonum);

	ret = nf_ct_get_tuple(skb, nhoff, protoff, l3num, protonum, tuple,
			      l3proto, l4proto);

	rcu_read_unlock();
	return ret;
}
EXPORT_SYMBOL_GPL(nf_ct_get_tuplepr);

bool
nf_ct_invert_tuple(struct nf_conntrack_tuple *inverse,
		   const struct nf_conntrack_tuple *orig,
		   const struct nf_conntrack_l3proto *l3proto,
		   const struct nf_conntrack_l4proto *l4proto)
{
	memset(inverse, 0, sizeof(*inverse));

	inverse->src.l3num = orig->src.l3num;
	if (l3proto->invert_tuple(inverse, orig) == 0)
		return false;

	inverse->dst.dir = !orig->dst.dir;

	inverse->dst.protonum = orig->dst.protonum;
	return l4proto->invert_tuple(inverse, orig);
}
EXPORT_SYMBOL_GPL(nf_ct_invert_tuple);

static void
clean_from_lists(struct nf_conn *ct)
{
	pr_debug("clean_from_lists(%p)\n", ct);
	hlist_nulls_del_rcu(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode);
	hlist_nulls_del_rcu(&ct->tuplehash[IP_CT_DIR_REPLY].hnnode);

	/* Destroy all pending expectations */
	nf_ct_remove_expectations(ct);
}

static void
destroy_conntrack(struct nf_conntrack *nfct)
{
	struct nf_conn *ct = (struct nf_conn *)nfct;
	struct net *net = nf_ct_net(ct);
	struct nf_conntrack_l4proto *l4proto;

	pr_debug("destroy_conntrack(%p)\n", ct);
	NF_CT_ASSERT(atomic_read(&nfct->use) == 0);
	NF_CT_ASSERT(!timer_pending(&ct->timeout));

	/* To make sure we don't get any weird locking issues here:
	 * destroy_conntrack() MUST NOT be called with a write lock
	 * to nf_conntrack_lock!!! -HW */
	rcu_read_lock();
	l4proto = __nf_ct_l4proto_find(nf_ct_l3num(ct), nf_ct_protonum(ct));
	if (l4proto && l4proto->destroy)
		l4proto->destroy(ct);

	rcu_read_unlock();

	spin_lock_bh(&nf_conntrack_lock);
	/* Expectations will have been removed in clean_from_lists,
	 * except TFTP can create an expectation on the first packet,
	 * before connection is in the list, so we need to clean here,
	 * too. */
	nf_ct_remove_expectations(ct);

	#if defined(CONFIG_NETFILTER_XT_MATCH_LAYER7) || defined(CONFIG_NETFILTER_XT_MATCH_LAYER7_MODULE)
	if(ct->layer7.app_proto)
		kfree(ct->layer7.app_proto);
	if(ct->layer7.app_data)
	kfree(ct->layer7.app_data);
	#endif

#if defined(CONFIG_NF_CONNTRACK_NAT_UDP_MANAGEMENT)
	if (ct->hashnode.pprev)
		hlist_nulls_del_rcu(&ct->hashnode);
#endif

#if defined(CONFIG_NF_CONNTRACK_CONENAT_MANAGEMENT)
	if (ct->conenat_hashnode.pprev)
		hlist_nulls_del_rcu(&ct->conenat_hashnode);
#endif

	/* We overload first tuple to link into unconfirmed list. */
	if (!nf_ct_is_confirmed(ct)) {
		BUG_ON(hlist_nulls_unhashed(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode));
		hlist_nulls_del_rcu(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode);
	}

	NF_CT_STAT_INC(net, delete);
	spin_unlock_bh(&nf_conntrack_lock);

	if (ct->master)
		nf_ct_put(ct->master);

	pr_debug("destroy_conntrack: returning ct=%p to slab\n", ct);
	nf_conntrack_free(ct);
}

void nf_ct_delete_from_lists(struct nf_conn *ct)
{
	struct net *net = nf_ct_net(ct);

	nf_ct_helper_destroy(ct);
	spin_lock_bh(&nf_conntrack_lock);
	/* Inside lock so preempt is disabled on module removal path.
	 * Otherwise we can get spurious warnings. */
	NF_CT_STAT_INC(net, delete_list);
	clean_from_lists(ct);
	spin_unlock_bh(&nf_conntrack_lock);
}
EXPORT_SYMBOL_GPL(nf_ct_delete_from_lists);

static void death_by_event(unsigned long ul_conntrack)
{
	struct nf_conn *ct = (void *)ul_conntrack;
	struct net *net = nf_ct_net(ct);


	if (nf_conntrack_event(IPCT_DESTROY, ct) < 0) {
		/* bad luck, let's retry again */
		ct->timeout.expires = jiffies +
			(random32() % net->ct.sysctl_events_retry_timeout);
		add_timer(&ct->timeout);
		return;
	}
	/* we've got the event delivered, now it's dying */
	set_bit(IPS_DYING_BIT, &ct->status);
	spin_lock(&nf_conntrack_lock);
	hlist_nulls_del(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode);
	spin_unlock(&nf_conntrack_lock);
	nf_ct_put(ct);
}

void nf_ct_insert_dying_list(struct nf_conn *ct)
{
	struct net *net = nf_ct_net(ct);

	/* add this conntrack to the dying list */
	spin_lock_bh(&nf_conntrack_lock);
	hlist_nulls_add_head(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode,
			     &net->ct.dying);
	spin_unlock_bh(&nf_conntrack_lock);
	/* set a new timer to retry event delivery */
	setup_timer(&ct->timeout, death_by_event, (unsigned long)ct);
	ct->timeout.expires = jiffies +
		(random32() % net->ct.sysctl_events_retry_timeout);
	add_timer(&ct->timeout);
}
EXPORT_SYMBOL_GPL(nf_ct_insert_dying_list);

static void death_by_timeout(unsigned long ul_conntrack)
{
	struct nf_conn *ct = (void *)ul_conntrack;

#ifdef CONFIG_ATHRS_HW_NAT
        void (*athr_process_hwnat)(struct sk_buff *, struct nf_conn *,
                                   enum ip_conntrack_info, u_int8_t);

        if (athr_nat_sw_ops) {
                athr_process_hwnat = rcu_dereference(athr_nat_sw_ops->nf_process_nat);
                if (athr_process_hwnat)
                        athr_process_hwnat(NULL, ct, 0, 0);
        }
#endif

	if (!test_bit(IPS_DYING_BIT, &ct->status) &&
	    unlikely(nf_conntrack_event(IPCT_DESTROY, ct) < 0)) {
		/* destroy event was not delivered */
		nf_ct_delete_from_lists(ct);
		nf_ct_insert_dying_list(ct);
		return;
	}
	set_bit(IPS_DYING_BIT, &ct->status);
	nf_ct_delete_from_lists(ct);
	nf_ct_put(ct);
}

/*
 * Warning :
 * - Caller must take a reference on returned object
 *   and recheck nf_ct_tuple_equal(tuple, &h->tuple)
 * OR
 * - Caller must lock nf_conntrack_lock before calling this function
 */
struct nf_conntrack_tuple_hash *
__nf_conntrack_find(struct net *net, const struct nf_conntrack_tuple *tuple)
{
	struct nf_conntrack_tuple_hash *h;
	struct hlist_nulls_node *n;
	unsigned int hash = hash_conntrack(tuple);

	/* Disable BHs the entire time since we normally need to disable them
	 * at least once for the stats anyway.
	 */
	local_bh_disable();
begin:
	hlist_nulls_for_each_entry_rcu(h, n, &net->ct.hash[hash], hnnode) {
		if (nf_ct_tuple_equal(tuple, &h->tuple)) {
			NF_CT_STAT_INC(net, found);
			local_bh_enable();
			return h;
		}
		NF_CT_STAT_INC(net, searched);
	}
	/*
	 * if the nulls value we got at the end of this lookup is
	 * not the expected one, we must restart lookup.
	 * We probably met an item that was moved to another chain.
	 */
	if (get_nulls_value(n) != hash)
		goto begin;
	local_bh_enable();

	return NULL;
}
EXPORT_SYMBOL_GPL(__nf_conntrack_find);

static inline bool udp_same_src_cmp
(const struct nf_conntrack_tuple_hash *master, const struct nf_conntrack_tuple_hash *other)
{
	struct nf_conn *conntrack = nf_ct_tuplehash_to_ctrack(other);

	if (master == other)
		return 0;

	if (other->tuple.dst.protonum != IPPROTO_UDP)
		return 0;

	/* The connection SHOULD be initiated from LAN to Internet: SNAT-ed. */
	if (!(conntrack->status & IPS_SRC_NAT))
		return 0;

#ifndef CONFIG_NF_CONNTRACK_CHECK_TCP_UDP_TIMEOUT_IN_EARLY_DROP
	if (!(conntrack->status & IPS_ASSURED))
		return 0;
#endif

	if(sysctl_nat_filtering_behavior){
		/* open mode */
		return __nf_ct_tuple_src_equal(&master->tuple, &other->tuple);
	} else {
		/* secured mode */
		return (__nf_ct_tuple_src_equal(&master->tuple, &other->tuple) &&
				__nf_ct_tuple_dst_addr_equal(&master->tuple, &other->tuple));
	}
}

/* Find a connection corresponding to a tuple. */
struct nf_conntrack_tuple_hash *
nf_conntrack_find_get(struct net *net, const struct nf_conntrack_tuple *tuple)
{
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct;

	rcu_read_lock();
begin:
	h = __nf_conntrack_find(net, tuple);
	if (h) {
		ct = nf_ct_tuplehash_to_ctrack(h);
		if (unlikely(nf_ct_is_dying(ct) ||
			     !atomic_inc_not_zero(&ct->ct_general.use)))
			h = NULL;
		else {
			if (unlikely(!nf_ct_tuple_equal(tuple, &h->tuple))) {
				nf_ct_put(ct);
				goto begin;
			}
		}
	}
	rcu_read_unlock();

	return h;
}
EXPORT_SYMBOL_GPL(nf_conntrack_find_get);

static void __nf_conntrack_hash_insert(struct nf_conn *ct,
				       unsigned int hash,
				       unsigned int repl_hash)
{
	struct net *net = nf_ct_net(ct);

	hlist_nulls_add_head_rcu(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode,
			   &net->ct.hash[hash]);
	hlist_nulls_add_head_rcu(&ct->tuplehash[IP_CT_DIR_REPLY].hnnode,
			   &net->ct.hash[repl_hash]);
}

void nf_conntrack_hash_insert(struct nf_conn *ct)
{
	unsigned int hash, repl_hash;

	hash = hash_conntrack(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
	repl_hash = hash_conntrack(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);

	__nf_conntrack_hash_insert(ct, hash, repl_hash);
}
EXPORT_SYMBOL_GPL(nf_conntrack_hash_insert);

#if defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT) || defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT_MODULE)

/* Outbound refresh idle time is less than or equal to 10 minutes */
#define HIGH_PRIO_IDLE_TIME	(10 * 60 * HZ)

#define NAT_SESSION_DIFF_NUM	(50)

#define NAT_LAN_HASH_SIZE	(16)
#define NAT_LAN_HASH_MASK	(NAT_LAN_HASH_SIZE - 1)

static struct nf_conn_lan *nat_lan_max;
static struct list_head nat_lans[NAT_LAN_HASH_SIZE];
static struct kmem_cache *nf_ct_natlan_cachep;

static noinline int early_drop(struct net *net, unsigned int hash);

static inline unsigned int nat_lan_hash(__u32 addr)
{
	return ntohl(addr) & NAT_LAN_HASH_MASK;
}

static int ip_ct_nat_lan_insert(struct nf_conn *ct)
{
	__u32 addr;
	unsigned int hash;
	struct nf_conn_lan *pos, *lan = NULL;

	addr = (ct->status & IPS_SRC_NAT)
		? ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip
		: ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip;
	hash = nat_lan_hash(addr);

	list_for_each_entry(pos, &nat_lans[hash], list) {
		if (pos->addr == addr) {
			lan = pos;
			break;
		}
	}

	if (lan == NULL) {
		lan = kmem_cache_alloc(nf_ct_natlan_cachep, GFP_ATOMIC);
		if (lan == NULL)
			return 0;

		lan->addr = addr;
		lan->count = 0;
		INIT_LIST_HEAD(&lan->low_prio_lru);
		INIT_LIST_HEAD(&lan->hig_prio);
		list_add(&lan->list, &nat_lans[hash]);
	}

	lan->count++;
	ct->lan_nat = lan;
	list_add(&ct->lan_hash, (ct->status & IPS_NAT_STATIC_HIGH_PRIORITY)
		 ? &lan->hig_prio : &lan->low_prio_lru);

	if ((nat_lan_max == NULL) || 
	    (nat_lan_max != lan && nat_lan_max->count < lan->count)) {
		NAT_LAN_DEBUGP("%u.%u.%u.%u has the maximum %d NAT sessions!\n",
				NIPQUAD(lan->addr), lan->count);
		nat_lan_max = lan;
	}

	return 1;
}

static int nf_ct_nat_lan_destroy(struct nf_conn *ct)
{
	struct nf_conn_lan *lan;

	if ((lan = ct->lan_nat) == NULL)
		return 0;

	list_del(&ct->lan_hash);

	if (--lan->count <= 0) {
		NAT_LAN_DEBUGP("%u.%u.%u.%u has no LAN sessions.\n",
				NIPQUAD(lan->addr));

		if (nat_lan_max == lan) {
			NAT_LAN_DEBUGP("%u.%u.%u.%u is the last NAT LAN host!\n",
					NIPQUAD(lan->addr));
			nat_lan_max = NULL;
		}

		list_del(&lan->list);
		kmem_cache_free(nf_ct_natlan_cachep, lan);
	} else if (nat_lan_max == lan) {
		unsigned int i;
		struct nf_conn_lan *pos;

		for (i = 0; i < NAT_LAN_HASH_SIZE; i++) {
			list_for_each_entry(pos, &nat_lans[i], list) {
				if (pos->count > lan->count) {
					NAT_LAN_DEBUGP("Hello, %u.%u.%u.%u has the maximum "
						       "NAT sessions now!\n", NIPQUAD(pos->addr));
					nat_lan_max = pos;

					return 1; /* Two for Loops, just return! */
				}
			}
		}
	}

	return 1;
}

/*
 * The rules below are listed from high priority to low priority
 * (i.e. rule 1 has the highest priority), and an implementation
 * has to check the rules one by one, from high priority to low
 * priority, until an applicable one is found.
 *
 * 1. If NumNATSessMAX - NumNATSessNEW >= 50, identify the LRU (least
 * recently used) low-priority session that belongs to LanIPMAX and
 * replace it with the new session.
 *
 * If rule #1 above cannot be applied, it means either
 *	NumNATSessMAX - NumNATSessNEW < 50
 * or
 *	There is no low-priority session belongs to LanIPMAX (i.e. all
 * of the LanIPMAX's sessions in the NAT mapping table are with
 * high-priority)
 *
 * Then
 *
 * 2. If the new session is a high-priority session, identify the LRU
 * low-priority session that belongs to LanIPNEW and replace it with
 * the new session. If there is no low-priority session belonging to
 * LanIPNEW, then the new session cannot be established.
 *
 * 3. If the new session is a low-priority session, the new session
 * cannot be established.
 */
static int remove_lru_low_prio_nat(struct nf_conn_lan *lan)
{
	struct nf_conn *pos, *ct;
	extern unsigned int tcp_timeouts[];

	ct = NULL;
	list_for_each_entry(pos, &lan->hig_prio, lan_hash) {
		/*  Only TCP ESTABLISHED timeout(1800) larger than HIGH_PRIO_IDLE_TIME(600)
		 *  So only check IPPROTO_TCP now.*/
		if (pos->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_TCP) {
			if ((pos->proto.tcp.state == TCP_CONNTRACK_ESTABLISHED) &&
			    ((pos->timeout.expires - jiffies) < (tcp_timeouts[TCP_CONNTRACK_ESTABLISHED] - HIGH_PRIO_IDLE_TIME))) {
				ct = pos;
				goto ret;
			}
		}
	}

	if (list_empty(&lan->low_prio_lru)) {
		NAT_LAN_DEBUGP("No low priority session for %u.%u.%u.%u!\n",
				NIPQUAD(lan->addr));
		return 0;
	}

	ct = list_entry(lan->low_prio_lru.prev, struct nf_conn, lan_hash);

ret:
	if (ct == NULL) {
		NAT_LAN_DEBUGP("No low priority session for %u.%u.%u.%u!\n",
				NIPQUAD(lan->addr));
		return 0;
	}

	if (unlikely(nf_ct_is_dying(ct) || !atomic_inc_not_zero(&ct->ct_general.use)))
		return 1;

	if (del_timer(&ct->timeout))
		death_by_timeout((unsigned long)ct);

	nf_ct_put(ct);
	return 1;
}

static int ip_ct_handle_nat_full(struct nf_conn *ct)
{
	int count;
	__u32 addr;
	unsigned int hash;
	struct nf_conn_lan *pos, *lan = NULL;

	addr = (ct->status & IPS_SRC_NAT)
		? ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip
		: ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip;
	hash = nat_lan_hash(addr);

	list_for_each_entry(pos, &nat_lans[hash], list) {
		if (pos->addr == addr) {
			lan = pos;
			break;
		}
	}

	if (nat_lan_max != NULL && nat_lan_max != lan) {
		count = lan != NULL ? lan->count : 0;
		
		if ((nat_lan_max->count - count >= NAT_SESSION_DIFF_NUM) &&
		    remove_lru_low_prio_nat(nat_lan_max))
			return 1;
	}

	if ((lan != NULL) && (ct->status & IPS_NAT_STATIC_HIGH_PRIORITY) &&
	    remove_lru_low_prio_nat(lan))
		return 1;

	return 0;
}

static int ip_ct_nat_lan_session(struct nf_conn *ct)
{
	int high;
	struct nf_conntrack_tuple *otuple;

	otuple = &ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple;
	if (otuple->src.l3num != PF_INET || !(ct->status & IPS_NAT_MASK))
		return 0;

	if (ct->status & IPS_DST_NAT) {
		/***********************************************************
		 * For `REDIRECT` target on `NF_IP_PRE_ROUTING`, It also do
		 * 'IPS_DST_NAT' to change its destination port, but NOT the
		 * destination IP address:
		 *
		 * O: src=10.0.0.1 dst={@10.0.0.2@} sport=3622 dport=3333
		 * R: src={@10.0.0.2@} dst=10.0.0.1 sport=80 dport=3622
		 *
		 * This conntrack is not be treated as an Internet session.
		 ************************************************************/
		if (ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip == otuple->dst.u3.ip)
			return 0;

		/* 1. The session matches a Port Triggering rule. */
		if (ct->status & IPS_TRIGGER)
			ct->status |= IPS_NAT_STATIC_HIGH_PRIORITY;

		return 1;
	}

	/****************************************************************
	 * 2. The session is initiated from the inside and its layer 4
	 * protocol (i.e. the protocol above the IP) is not TCP or UDP.
	 *
	 * 3. The session is initiated from the inside and its destination
	 * TCP/UDP port number is below 1024 (i.e. 1 ~ 1023).
	 *
	 * 4. The session is initiated from the inside and its destination
	 * TCP/UDP port number is one of the applications in the table.
	 ****************************************************************/
	if (otuple->dst.protonum == IPPROTO_TCP)
		high = test_bit(ntohs(otuple->dst.u.all), tcp_high_prio_port_table);
	else if (otuple->dst.protonum == IPPROTO_UDP)
		high = test_bit(ntohs(otuple->dst.u.all), udp_high_prio_port_table);
	else
		high = 1;

	if (high)
		ct->status |= IPS_NAT_STATIC_HIGH_PRIORITY;

	return 1;
}

static void ip_ct_lru_low_prio_update(struct nf_conn *ct)
{
	struct nf_conn_lan *lan;

	if ((ct->status & IPS_NAT_STATIC_HIGH_PRIORITY) ||
	    (lan = ct->lan_nat) == NULL)
		return;
	
	/* Move to head of low_prio_lru, and it will not be replaced by new session soon.
	 * As in function remove_lru_low_prio_nat(), delete form tail of low_prio_lru (low_prio_lru.prev) */
	list_move(&ct->lan_hash, &lan->low_prio_lru);
}
#endif

#if defined(CONFIG_NF_CONNTRACK_NAT_UDP_MANAGEMENT)
u_int32_t dni_hash_conntrack(const struct nf_conntrack_tuple *tuple)
{
	unsigned int size = nf_conntrack_htable_size;
	unsigned int rnd = nf_conntrack_hash_rnd;
	unsigned int n;
	u_int32_t h;

	if (sysctl_nat_filtering_behavior) {
		/* open mode */
		n = sizeof(tuple->src) / sizeof(u32);
	} else {
		/* secured mode */
		n = (sizeof(tuple->src) + sizeof(tuple->dst.u3)) / sizeof(u32);
	}

	h = jhash2((u32 *)tuple, n, rnd ^ tuple->dst.protonum);

	return ((u64)h * size) >> 32;
}
EXPORT_SYMBOL(dni_hash_conntrack);

static inline bool dni_hash_tuple_cmp(const struct nf_conntrack_tuple *t1,
				      const struct nf_conntrack_tuple *t2)
{
	if (sysctl_nat_filtering_behavior) {
		/* open mode */
		return __nf_ct_tuple_src_equal(t1, t2);
	} else {
		/* secured mode */
		return (__nf_ct_tuple_src_equal(t1, t2) &&
			__nf_ct_tuple_dst_addr_equal(t1, t2));
	}
}

void nf_conntrack_dni_hash_find_remove(const struct nf_conntrack_tuple *tuple,
				       const unsigned int hash,
				       const unsigned int nat_type)
{
	struct nf_conntrack_tuple *h;
	struct hlist_nulls_node *n;
	struct nf_conn *ct = NULL, *tmp;

	rcu_read_lock();
	hlist_nulls_for_each_entry_rcu(tmp, n, &dni_ct_hash[hash], hashnode) {
		if ((tmp->status & IPS_SRC_NAT) && (nat_type == 1))
			h = &tmp->tuplehash[IP_CT_DIR_ORIGINAL].tuple;
		else if ((tmp->status & IPS_DST_NAT) && (nat_type == 0))
			h = &tmp->tuplehash[IP_CT_DIR_REPLY].tuple;
		else 
			continue;

		if (dni_hash_tuple_cmp(tuple, h))
			ct = tmp;

		if (ct && unlikely(nf_ct_is_dying(ct) ||
				   !atomic_inc_not_zero(&ct->ct_general.use)))
			ct = NULL;

		if (ct)
			break;
	}
	rcu_read_unlock();

	if (ct) {
		if (del_timer(&ct->timeout))
			death_by_timeout((unsigned long)ct);
		nf_ct_put(ct);
	}
}
#endif

#if defined(CONFIG_NF_CONNTRACK_CONENAT_MANAGEMENT)
u_int32_t conenat_hash_conntrack(const struct nf_conntrack_tuple *tuple)
{
	unsigned int size = nf_conntrack_htable_size;
	u_int32_t h;

	if (sysctl_nat_filtering_behavior) {
		/* open mode */
		h = (__force u32)tuple->dst.u.all;

		return (h % size);
	} else {
		/* secured mode */
		h = jhash_3words((__force u32)tuple->src.u3.ip,
				 (__force u32)tuple->dst.u.all,
				 (__force u32)tuple->dst.protonum,
				 0);

		return ((u64)h * size) >> 32;
	}
}

EXPORT_SYMBOL(conenat_hash_conntrack);
#endif

/* Confirm a connection given skb; places it in hash table */
int
__nf_conntrack_confirm(struct sk_buff *skb)
{
	unsigned int hash, repl_hash;
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct;
	struct nf_conn_help *help;
	struct hlist_nulls_node *n;
	enum ip_conntrack_info ctinfo;
	struct net *net;
#if defined(CONFIG_NF_CONNTRACK_NAT_UDP_MANAGEMENT)
	unsigned int dni_hash;
#endif
#if defined(CONFIG_NF_CONNTRACK_CONENAT_MANAGEMENT)
	unsigned int conenat_hash;
#endif

	ct = nf_ct_get(skb, &ctinfo);
	net = nf_ct_net(ct);

	/* ipt_REJECT uses nf_conntrack_attach to attach related
	   ICMP/TCP RST packets in other direction.  Actual packet
	   which created connection will be IP_CT_NEW or for an
	   expected connection, IP_CT_RELATED. */
	if (CTINFO2DIR(ctinfo) != IP_CT_DIR_ORIGINAL)
		return NF_ACCEPT;

	hash = hash_conntrack(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
	repl_hash = hash_conntrack(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);

	/* We're not in hash table, and we refuse to set up related
	   connections for unconfirmed conns.  But packet copies and
	   REJECT will give spurious warnings here. */
	/* NF_CT_ASSERT(atomic_read(&ct->ct_general.use) == 1); */

	/* No external references means noone else could have
	   confirmed us. */
	NF_CT_ASSERT(!nf_ct_is_confirmed(ct));
	pr_debug("Confirming conntrack %p\n", ct);

	spin_lock_bh(&nf_conntrack_lock);

#if defined(CONFIG_IP_NF_TARGET_SNATP2P) || defined(CONFIG_IP_NF_TARGET_SNATP2P_MODULE) || defined(CONFIG_IP_NF_TARGET_HAIRPIN) || defined(CONFIG_IP_NF_TARGET_HAIRPIN_MODULE)
	if (ct->status & IPS_PORT_FULL) {
		pr_debug("Port conflict, and no port can be chose!\n");
		goto out;
	}
#endif

	/* See if there's one in the list already, including reverse:
	   NAT could have grabbed it without realizing, since we're
	   not in the hash.  If there is, we lost race. */
	hlist_nulls_for_each_entry(h, n, &net->ct.hash[hash], hnnode)
		if (nf_ct_tuple_equal(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple,
				      &h->tuple)) {
                        printk("\nDropping packet:%s:%d %d %d %d::%d %d %d %d %d\n",__func__,__LINE__, \
                                ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.all, \
                                ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.l3num, \
                                ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.all, \
                                ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum, \
                                h->tuple.src.u.all, \
                                h->tuple.src.l3num, \
                                h->tuple.dst.u.all, \
                                h->tuple.dst.protonum
                                );
			goto out;
                }
	hlist_nulls_for_each_entry(h, n, &net->ct.hash[repl_hash], hnnode)
		if (nf_ct_tuple_equal(&ct->tuplehash[IP_CT_DIR_REPLY].tuple,
				      &h->tuple)) {

                        printk("\nDropping packet:%s:%d %d %d %d :: %d %d %d %d %d\n",__func__,__LINE__, \
                                ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.all, \
                                ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.l3num, \
                                ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.all, \
                                ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.protonum, \
                                h->tuple.src.u.all, \
                                h->tuple.src.l3num, \
                                h->tuple.dst.u.all, \
                                h->tuple.dst.protonum
                                );
			goto out;
               }

#if defined(CONFIG_NF_CONNTRACK_NAT_UDP_MANAGEMENT)
	/*  Check single UDP conntrack for all LAN-WAN sessions before conntrack sessions full checking. */
	if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.l3num == PF_INET &&
	    ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_UDP) {
		if (ct->status & IPS_SRC_NAT) {
			if (!(ct->status & IPS_DST_NAT)) {
				dni_hash = dni_hash_conntrack(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
				nf_conntrack_dni_hash_find_remove(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple, dni_hash, 1);
				hlist_nulls_add_head_rcu(&ct->hashnode, &dni_ct_hash[dni_hash]);
			}
		} else if ((ct->status & IPS_DST_NAT) && (ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u3.ip !=
							  ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u3.ip)) {
			dni_hash = dni_hash_conntrack(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);
			nf_conntrack_dni_hash_find_remove(&ct->tuplehash[IP_CT_DIR_REPLY].tuple, dni_hash, 0);
			hlist_nulls_add_head_rcu(&ct->hashnode, &dni_ct_hash[dni_hash]);
		}
	}
#endif

#if defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT) || defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT_MODULE)
	if (ip_ct_nat_lan_session(ct)) {
		if (nf_conntrack_max) {
			if (unlikely(atomic_read(&net->ct.count) >= nf_conntrack_max) && 
			    early_drop(net, hash)) {
				if (net_ratelimit())
					printk(KERN_WARNING "nf_conntrack: freeing un-assure connection.\n");
			}

			/* As the dropped connection by early_drop() maybe not the LAN-WAN session,
			 * so it maybe not be counted, and need to read the ct count again. */
			if (unlikely(atomic_read(&net->ct.count) >= nf_conntrack_max) && 
			    (ip_ct_handle_nat_full(ct) == 0)) {
				if (net_ratelimit())
					printk(KERN_WARNING "Internet sessions full, dropping packet.\n");
				goto out;
			}
		}

		if (ip_ct_nat_lan_insert(ct)) /* No memory if failed! */
			atomic_inc(&net->ct.count);
		else
			goto out;
	}
#if defined(CONFIG_NF_CONNTRACK_LOCAL_MANAGEMENT)
	else if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.l3num == PF_INET) {
		if (nf_conntrack_local_max &&
		    atomic_read(&nf_conntrack_local_count) >= nf_conntrack_local_max) {
			if ((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_TCP) && 
			    nf_conntrack_tcp_reserve_max &&
			    atomic_read(&nf_conntrack_tcp_reserve_count) < nf_conntrack_tcp_reserve_max) {
				ct->status |= IPS_CT_TCP_RESERVE;
				atomic_inc(&nf_conntrack_tcp_reserve_count);
			} else if ((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_ICMP) && 
			    nf_conntrack_icmp_reserve_max &&
			    atomic_read(&nf_conntrack_icmp_reserve_count) < nf_conntrack_icmp_reserve_max) {
				ct->status |= IPS_CT_ICMP_RESERVE;
				atomic_inc(&nf_conntrack_icmp_reserve_count);
			} else {
				if (net_ratelimit())
					printk(KERN_WARNING "Local sessions full, dropping packet.\n");
				goto out;
			}
		} else
			atomic_inc(&nf_conntrack_local_count);
	}
#endif
#endif

	/* Remove from unconfirmed list */
	hlist_nulls_del_rcu(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode);

	/* Timer relative to confirmation time, not original
	   setting time, otherwise we'd get timer wrap in
	   weird delay cases. */
	ct->timeout.expires += jiffies;
	add_timer(&ct->timeout);
	atomic_inc(&ct->ct_general.use);
	set_bit(IPS_CONFIRMED_BIT, &ct->status);

	/* Since the lookup is lockless, hash insertion must be done after
     * starting the timer and setting the CONFIRMED bit. The RCU barriers
     * guarantee that no other CPU can find the conntrack before the above
     * stores are visible.
     */
    __nf_conntrack_hash_insert(ct, hash, repl_hash);
	NF_CT_STAT_INC(net, insert);

#if defined(CONFIG_NF_CONNTRACK_CONENAT_MANAGEMENT)
	if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.l3num == PF_INET &&
	    ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_UDP) {
		if (ct->status & IPS_SRC_NAT) {
			conenat_hash = conenat_hash_conntrack(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);
			hlist_nulls_add_head_rcu(&ct->conenat_hashnode, &conenat_ct_hash[conenat_hash]);
		}
	}
#endif

	spin_unlock_bh(&nf_conntrack_lock);

	help = nfct_help(ct);
	if (help && help->helper)
		nf_conntrack_event_cache(IPCT_HELPER, ct);

	nf_conntrack_event_cache(master_ct(ct) ?
				 IPCT_RELATED : IPCT_NEW, ct);
	return NF_ACCEPT;

out:
	NF_CT_STAT_INC(net, insert_failed);
	spin_unlock_bh(&nf_conntrack_lock);
	return NF_DROP;
}
EXPORT_SYMBOL_GPL(__nf_conntrack_confirm);

/* Returns true if a connection correspondings to the tuple (required
   for NAT). */
int
nf_conntrack_tuple_taken(const struct nf_conntrack_tuple *tuple,
			 const struct nf_conn *ignored_conntrack)
{
	struct net *net = nf_ct_net(ignored_conntrack);
	struct nf_conntrack_tuple_hash *h;
	struct hlist_nulls_node *n;
	unsigned int hash = hash_conntrack(tuple);
#ifdef CONFIG_ATHRS_HW_NAT
        int (*athr_tuple_taken)(const struct nf_conntrack_tuple *,
                                const struct nf_conn *);
#endif

	/* Disable BHs the entire time since we need to disable them at
	 * least once for the stats anyway.
	 */
	rcu_read_lock_bh();
	hlist_nulls_for_each_entry_rcu(h, n, &net->ct.hash[hash], hnnode) {

		if (nf_ct_tuplehash_to_ctrack(h) != ignored_conntrack &&
		    nf_ct_tuple_equal(tuple, &h->tuple)) {
			NF_CT_STAT_INC(net, found);
			rcu_read_unlock_bh();
			return 1;
		}
		NF_CT_STAT_INC(net, searched);
	}
	rcu_read_unlock_bh();

#ifdef CONFIG_ATHRS_HW_NAT
        if (athr_nat_sw_ops) {
                athr_tuple_taken = rcu_dereference(athr_nat_sw_ops->nf_tuple_taken);
                if (athr_tuple_taken)
                        return athr_tuple_taken(tuple, ignored_conntrack);
        }
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(nf_conntrack_tuple_taken);

#define NF_CT_EVICTION_RANGE	8

/* There's a small race here where we may free a just-assured
   connection.  Too bad: we're in trouble anyway. */
static noinline int early_drop(struct net *net, unsigned int hash)
{
	/* Use oldest entry, which is roughly LRU */
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct = NULL, *tmp;
	struct hlist_nulls_node *n;
	unsigned int i, cnt = 0;
	int dropped = 0;

	rcu_read_lock();
	for (i = 0; i < nf_conntrack_htable_size; i++) {
		hlist_nulls_for_each_entry_rcu(h, n, &net->ct.hash[hash],
					 hnnode) {
			tmp = nf_ct_tuplehash_to_ctrack(h);
			if (!test_bit(IPS_ASSURED_BIT, &tmp->status))
#ifdef CONFIG_NF_CONNTRACK_CHECK_TCP_UDP_TIMEOUT_IN_EARLY_DROP
				if ((nf_ct_protonum(tmp) != IPPROTO_TCP
				     && nf_ct_protonum(tmp) != IPPROTO_UDP)
				    || !timer_pending(&tmp->timeout))
#endif
			{
				ct = tmp;
			}
			cnt++;
		}

		if (ct && unlikely(nf_ct_is_dying(ct) ||
				   !atomic_inc_not_zero(&ct->ct_general.use)))
			ct = NULL;
		if (ct || cnt >= NF_CT_EVICTION_RANGE)
			break;
		hash = (hash + 1) % nf_conntrack_htable_size;
	}
	rcu_read_unlock();

	if (!ct)
		return dropped;

	if (del_timer(&ct->timeout)) {
		death_by_timeout((unsigned long)ct);
		dropped = 1;
		NF_CT_STAT_INC_ATOMIC(net, early_drop);
	}
	nf_ct_put(ct);
	return dropped;
}

struct nf_conn *nf_conntrack_alloc(struct net *net,
				   const struct nf_conntrack_tuple *orig,
				   const struct nf_conntrack_tuple *repl,
				   gfp_t gfp)
{
	struct nf_conn *ct;

	if (unlikely(!nf_conntrack_hash_rnd_initted)) {
		get_random_bytes(&nf_conntrack_hash_rnd,
				sizeof(nf_conntrack_hash_rnd));
		nf_conntrack_hash_rnd_initted = 1;
	}

#if !defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT) && !defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT_MODULE)
	/* We don't want any race condition at early drop stage */
	atomic_inc(&net->ct.count);

	if (nf_conntrack_max &&
	    unlikely(atomic_read(&net->ct.count) > nf_conntrack_max)) {
		unsigned int hash = hash_conntrack(orig);
		if (!early_drop(net, hash)) {
			atomic_dec(&net->ct.count);
			if (net_ratelimit())
				printk(KERN_WARNING
				       "nf_conntrack: table full, dropping"
				       " packet.\n");
			return ERR_PTR(-ENOMEM);
		}
	}
#endif

	/*
	 * Do not use kmem_cache_zalloc(), as this cache uses
	 * SLAB_DESTROY_BY_RCU.
	 */
	ct = kmem_cache_alloc(nf_conntrack_cachep, gfp);
	if (ct == NULL) {
		pr_debug("nf_conntrack_alloc: Can't alloc conntrack.\n");
#if !defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT) && !defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT_MODULE)
		atomic_dec(&net->ct.count);
#endif
		return ERR_PTR(-ENOMEM);
	}
	/*
	 * Let ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode.next
	 * and ct->tuplehash[IP_CT_DIR_REPLY].hnnode.next unchanged.
	 */
	memset(&ct->tuplehash[IP_CT_DIR_MAX], 0,
	       sizeof(*ct) - offsetof(struct nf_conn, tuplehash[IP_CT_DIR_MAX]));

	spin_lock_init(&ct->lock);
	ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple = *orig;
	ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode.pprev = NULL;
	ct->tuplehash[IP_CT_DIR_REPLY].tuple = *repl;
	ct->tuplehash[IP_CT_DIR_REPLY].hnnode.pprev = NULL;
#if defined(CONFIG_NF_CONNTRACK_NAT_UDP_MANAGEMENT)
	ct->hashnode.pprev = NULL;
#endif
#if defined(CONFIG_NF_CONNTRACK_CONENAT_MANAGEMENT)
	ct->conenat_hashnode.pprev = NULL;
#endif
	/* Don't set timer yet: wait for confirmation */
	setup_timer(&ct->timeout, death_by_timeout, (unsigned long)ct);
#ifdef CONFIG_NET_NS
	ct->ct_net = net;
#endif
#ifdef CONFIG_ATHRS17_HNAT
        ct->in_hnat = 0;
#endif

	/*
	 * changes to lookup keys must be done before setting refcnt to 1
	 */
	smp_wmb();
	atomic_set(&ct->ct_general.use, 1);
	return ct;
}
EXPORT_SYMBOL_GPL(nf_conntrack_alloc);

void nf_conntrack_free(struct nf_conn *ct)
{
	struct net *net = nf_ct_net(ct);
	nf_ct_ext_destroy(ct);
#if defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT) || defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT_MODULE)
	if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.l3num == PF_INET && 
	    nf_ct_is_confirmed(ct)) {
		if (nf_ct_nat_lan_destroy(ct))
			atomic_dec(&net->ct.count);
#if defined(CONFIG_NF_CONNTRACK_LOCAL_MANAGEMENT)
		else if (ct->status & IPS_CT_ICMP_RESERVE)
			atomic_dec(&nf_conntrack_icmp_reserve_count);
		else if (ct->status & IPS_CT_TCP_RESERVE)
			atomic_dec(&nf_conntrack_tcp_reserve_count);
		else
			atomic_dec(&nf_conntrack_local_count);
#endif
	}
#else
	atomic_dec(&net->ct.count);
#endif
	nf_ct_ext_free(ct);
	kmem_cache_free(nf_conntrack_cachep, ct);
}
EXPORT_SYMBOL_GPL(nf_conntrack_free);

/* Allocate a new conntrack: we return -ENOMEM if classification
   failed due to stress.  Otherwise it really is unclassifiable. */
static struct nf_conntrack_tuple_hash *
init_conntrack(struct net *net,
	       struct nf_conntrack_tuple *tuple,
	       struct nf_conntrack_l3proto *l3proto,
	       struct nf_conntrack_l4proto *l4proto,
	       struct sk_buff *skb,
	       unsigned int dataoff
	       )
{
	struct nf_conn *ct;
	struct nf_conn_help *help;
	struct nf_conntrack_tuple repl_tuple;
	struct nf_conntrack_expect *exp;
#ifdef CONFIG_ATHRS_HW_NAT
        void (*athr_get_wan_addr)(uint32_t *,uint8_t);
	uint8_t index=0;
#endif

	if (!nf_ct_invert_tuple(&repl_tuple, tuple, l3proto, l4proto)) {
		pr_debug("Can't invert tuple.\n");
		return NULL;
	}
#ifdef CONFIG_ATHRS_HW_NAT
        if ((skb->ath_hw_nat_fw_flags == 3) && athr_nat_sw_ops) {
                athr_get_wan_addr = rcu_dereference(athr_nat_sw_ops->get_wan_ipaddr);
                if (athr_get_wan_addr)
                        athr_get_wan_addr(&tuple->dst.u3.ip,index);
        }
#endif
	ct = nf_conntrack_alloc(net, tuple, &repl_tuple, GFP_ATOMIC);
	if (IS_ERR(ct)) {
		pr_debug("Can't allocate conntrack.\n");
		return (struct nf_conntrack_tuple_hash *)ct;
	}
	if (!l4proto->new(ct, skb, dataoff)) {
		nf_conntrack_free(ct);
		pr_debug("init conntrack: can't track with proto module\n");
		return NULL;
	}

	nf_ct_acct_ext_add(ct, GFP_ATOMIC);
	nf_ct_ecache_ext_add(ct, GFP_ATOMIC);

	spin_lock_bh(&nf_conntrack_lock);
	exp = nf_ct_find_expectation(net, tuple);
	if (exp) {
		pr_debug("conntrack: expectation arrives ct=%p exp=%p\n",
			 ct, exp);
		/* Welcome, Mr. Bond.  We've been expecting you... */
		__set_bit(IPS_EXPECTED_BIT, &ct->status);
		ct->master = exp->master;
		if (exp->helper) {
			help = nf_ct_helper_ext_add(ct, GFP_ATOMIC);
			if (help)
				rcu_assign_pointer(help->helper, exp->helper);
		}

#ifdef CONFIG_NF_CONNTRACK_MARK
		ct->mark = exp->master->mark;
#endif
#ifdef CONFIG_NF_CONNTRACK_SECMARK
		ct->secmark = exp->master->secmark;
#endif
		nf_conntrack_get(&ct->master->ct_general);
		NF_CT_STAT_INC(net, expect_new);
	} else {
		__nf_ct_try_assign_helper(ct, GFP_ATOMIC);
		NF_CT_STAT_INC(net, new);
	}

	/* Overload tuple linked list to put us in unconfirmed list. */
	hlist_nulls_add_head_rcu(&ct->tuplehash[IP_CT_DIR_ORIGINAL].hnnode,
		       &net->ct.unconfirmed);

#ifdef CONFIG_ATHRS_HW_NAT
	if(skb->ath_hw_nat_fw_flags == 3)
		set_bit(IPS_ATHR_SW_NAT_SKIPPED_BIT, &ct->status);
#endif
	spin_unlock_bh(&nf_conntrack_lock);

	if (exp) {
		if (exp->expectfn)
			exp->expectfn(ct, exp);
		nf_ct_expect_put(exp);
	}

	return &ct->tuplehash[IP_CT_DIR_ORIGINAL];
}

void
nf_conntrack_nat_filtering_find_and_remove(struct net *net,
	          const struct nf_conntrack_tuple_hash *master)
{
	struct nf_conntrack_tuple_hash *h;
	struct hlist_nulls_node *n;
	unsigned int i;
	struct nf_conn *ct = NULL;

	rcu_read_lock();
	for (i = 0; i < nf_conntrack_htable_size; i++) {
		hlist_nulls_for_each_entry_rcu(h, n, &net->ct.hash[i], hnnode) {
			/* we only want to deal DIR_ORIGINAL */
			if (NF_CT_DIRECTION(h) == IP_CT_DIR_ORIGINAL) {
				if (udp_same_src_cmp(master, h)) {
					ct = nf_ct_tuplehash_to_ctrack(h);
				}
			}
		}
		if (ct && unlikely(nf_ct_is_dying(ct) ||
				   !atomic_inc_not_zero(&ct->ct_general.use)))
			ct = NULL;
		if (ct)
			break;
	}
	rcu_read_unlock();
	if (ct) {
		if (del_timer(&ct->timeout))
			death_by_timeout((unsigned long)ct);
		/* ... else the timer will get him soon. */
		nf_ct_put(ct);
	}
}

/* On success, returns conntrack ptr, sets skb->nfct and ctinfo */
static inline struct nf_conn *
resolve_normal_ct(struct net *net,
		  struct sk_buff *skb,
		  unsigned int dataoff,
		  u_int16_t l3num,
		  u_int8_t protonum,
		  struct nf_conntrack_l3proto *l3proto,
		  struct nf_conntrack_l4proto *l4proto,
		  int *set_reply,
		  enum ip_conntrack_info *ctinfo)
{
	struct nf_conntrack_tuple tuple;
	struct nf_conntrack_tuple_hash *h = NULL;
	struct nf_conn *ct = NULL;
#ifdef CONFIG_ATHRS_HW_NAT
        struct nf_conn *(*athr_find_get)(struct net *, struct nf_conntrack_tuple *,
                                         __u32, struct nf_conntrack_tuple_hash **);
#endif

	if (!nf_ct_get_tuple(skb, skb_network_offset(skb),
			     dataoff, l3num, protonum, &tuple, l3proto,
			     l4proto)) {
		pr_debug("resolve_normal_ct: Can't get tuple\n");
		return NULL;
	}

#ifdef CONFIG_ATHRS_HW_NAT
        /*
         * for ingress, change the dest ip addr to wan router ip addr
         * so as to make conntrack to find the match. Should be called only 
         * for DNAT */
        if (athr_nat_sw_ops) {
                athr_find_get = rcu_dereference(athr_nat_sw_ops->nf_find_get);
                if (athr_find_get) {
                        ct = athr_find_get(net, &tuple, skb->ath_hw_nat_fw_flags, &h);
                        if (ct)
                                goto out;
                }
        }
#endif

	/* look for tuple match */
	h = nf_conntrack_find_get(net, &tuple);
	if (!h) {
		h = init_conntrack(net, &tuple, l3proto, l4proto, skb, dataoff);
		if (!h)
			return NULL;
		if (IS_ERR(h))
			return (void *)h;
#if defined(CONFIG_NF_NAT) || defined(CONFIG_NF_NAT_MODULE)
#ifdef CONFIG_NF_SINGLE_UDP_CONNTRACK_FOR_LAN_WAN_TUPLE
		if (h->tuple.dst.protonum == IPPROTO_UDP)
			nf_conntrack_nat_filtering_find_and_remove(net, h);
#endif
#endif
	}
	ct = nf_ct_tuplehash_to_ctrack(h);
#ifdef CONFIG_ATHRS_HW_NAT
out:
#endif
	/* It exists; we have (non-exclusive) reference. */
	if (NF_CT_DIRECTION(h) == IP_CT_DIR_REPLY) {
		*ctinfo = IP_CT_ESTABLISHED + IP_CT_IS_REPLY;
		/* Please set reply bit if this packet OK */
		*set_reply = 1;
	} else {
		/* Once we've had two way comms, always ESTABLISHED. */
		if (test_bit(IPS_SEEN_REPLY_BIT, &ct->status)) {
			pr_debug("nf_conntrack_in: normal packet for %p\n", ct);
			*ctinfo = IP_CT_ESTABLISHED;
		} else if (test_bit(IPS_EXPECTED_BIT, &ct->status)) {
			pr_debug("nf_conntrack_in: related packet for %p\n",
				 ct);
			*ctinfo = IP_CT_RELATED;
		} else {
			pr_debug("nf_conntrack_in: new packet for %p\n", ct);
			*ctinfo = IP_CT_NEW;
		}
		*set_reply = 0;
	}
	skb->nfct = &ct->ct_general;
	skb->nfctinfo = *ctinfo;
	return ct;
}

#ifdef CONFIG_ATH_HWCS
/* Setup checksum engine to start computation */
inline __wsum
ath_hwcs_skb_checksum(struct sk_buff *skb, int offset, int len)
{
	extern __sum16 ath_hwcs_start(void *buf, int len);

	int start = skb_headlen(skb);
	int copy = start - offset;

	if (copy > 0) {
        	ath_hwcs_start(skb->data + offset, copy);
	}

	return 0;
}
#endif /* CONFIG_ATH_HWCS */

unsigned int
nf_conntrack_in(struct net *net, u_int8_t pf, unsigned int hooknum,
		struct sk_buff *skb)
{
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;
	struct nf_conntrack_l3proto *l3proto;
	struct nf_conntrack_l4proto *l4proto;
	unsigned int dataoff;
	u_int8_t protonum;
	int set_reply = 0;
	int ret;
#ifdef CONFIG_ATHRS_HW_NAT
        void (*athr_process_hwnat)(struct sk_buff *, struct nf_conn *,
                                   enum ip_conntrack_info, u_int8_t);
#endif

	/* Previously seen (loopback or untracked)?  Ignore. */
	if (skb->nfct) {
		NF_CT_STAT_INC_ATOMIC(net, ignore);
		return NF_ACCEPT;
	}

	/* rcu_read_lock()ed by nf_hook_slow */
	l3proto = __nf_ct_l3proto_find(pf);
	ret = l3proto->get_l4proto(skb, skb_network_offset(skb),
				   &dataoff, &protonum);
	if (ret <= 0) {
		pr_debug("not prepared to track yet or error occured\n");
		NF_CT_STAT_INC_ATOMIC(net, error);
		NF_CT_STAT_INC_ATOMIC(net, invalid);
		return -ret;
	}

	l4proto = __nf_ct_l4proto_find(pf, protonum);

	/* It may be an special packet, error, unclean...
	 * inverse of the return code tells to the netfilter
	 * core what to do with the packet. */

#ifdef CONFIG_ATH_HWCS
        if (hooknum == NF_INET_PRE_ROUTING && l4proto->error != NULL) {
		ath_hwcs_skb_checksum(skb, dataoff - 0x14, skb->len);
	}
#else
#ifdef CONFIG_ATHRS_HW_NAT
        if ((protonum == IPPROTO_ICMP) && (l4proto->error != NULL))
#else
        if (l4proto->error != NULL)
#endif
	{
		ret = l4proto->error(net, skb, dataoff, &ctinfo, pf, hooknum);
		if (ret <= 0) {
			NF_CT_STAT_INC_ATOMIC(net, error);
			NF_CT_STAT_INC_ATOMIC(net, invalid);
			return -ret;
		}
	}
#endif /* CONFIG_ATH_HWCS */

	ct = resolve_normal_ct(net, skb, dataoff, pf, protonum,
			       l3proto, l4proto, &set_reply, &ctinfo);
	if (!ct) {
		/* Not valid part of a connection */
		NF_CT_STAT_INC_ATOMIC(net, invalid);
		return NF_ACCEPT;
	}

	if (IS_ERR(ct)) {
		/* Too stressed to deal. */
		NF_CT_STAT_INC_ATOMIC(net, drop);
		return NF_DROP;
	}

	NF_CT_ASSERT(skb->nfct);

	ret = l4proto->packet(ct, skb, dataoff, ctinfo, pf, hooknum);
	if (ret <= 0) {
		/* Invalid: inverse of the return code tells
		 * the netfilter core what to do */
		pr_debug("nf_conntrack_in: Can't track with proto module\n");
		nf_conntrack_put(skb->nfct);
		skb->nfct = NULL;
		NF_CT_STAT_INC_ATOMIC(net, invalid);
		if (ret == -NF_DROP)
			NF_CT_STAT_INC_ATOMIC(net, drop);
		return -ret;
	}

	if (set_reply && !test_and_set_bit(IPS_SEEN_REPLY_BIT, &ct->status))
		nf_conntrack_event_cache(IPCT_STATUS, ct);

#ifdef CONFIG_ATH_HWCS
#ifdef CONFIG_ATHRS_HW_NAT
        if ((protonum == IPPROTO_ICMP) && (l4proto->error != NULL))
#else
        if (l4proto->error != NULL)
#endif
	{
		ret = l4proto->error(net, skb, dataoff, &ctinfo, pf, hooknum);
		if (ret <= 0) {
			NF_CT_STAT_INC_ATOMIC(net, error);
			NF_CT_STAT_INC_ATOMIC(net, invalid);
			return -ret;
		}
	}
#endif

#ifdef CONFIG_ATHRS_HW_NAT
        if (athr_nat_sw_ops) {
                athr_process_hwnat = rcu_dereference(athr_nat_sw_ops->nf_process_nat);
                if (athr_process_hwnat)
                        athr_process_hwnat(skb, ct, ctinfo, protonum);
        }
#endif
	return ret;
}
EXPORT_SYMBOL_GPL(nf_conntrack_in);

bool nf_ct_invert_tuplepr(struct nf_conntrack_tuple *inverse,
			  const struct nf_conntrack_tuple *orig)
{
	bool ret;

	rcu_read_lock();
	ret = nf_ct_invert_tuple(inverse, orig,
				 __nf_ct_l3proto_find(orig->src.l3num),
				 __nf_ct_l4proto_find(orig->src.l3num,
						      orig->dst.protonum));
	rcu_read_unlock();
	return ret;
}
EXPORT_SYMBOL_GPL(nf_ct_invert_tuplepr);

/* Alter reply tuple (maybe alter helper).  This is for NAT, and is
   implicitly racy: see __nf_conntrack_confirm */
void nf_conntrack_alter_reply(struct nf_conn *ct,
			      const struct nf_conntrack_tuple *newreply)
{
	struct nf_conn_help *help = nfct_help(ct);

	/* Should be unconfirmed, so not in hash table yet */
	NF_CT_ASSERT(!nf_ct_is_confirmed(ct));

	pr_debug("Altering reply tuple of %p to ", ct);
	nf_ct_dump_tuple(newreply);

	ct->tuplehash[IP_CT_DIR_REPLY].tuple = *newreply;
	if (ct->master || (help && !hlist_empty(&help->expectations)))
		return;

	rcu_read_lock();
	__nf_ct_try_assign_helper(ct, GFP_ATOMIC);
	rcu_read_unlock();
}
EXPORT_SYMBOL_GPL(nf_conntrack_alter_reply);

/* Refresh conntrack for this many jiffies and do accounting if do_acct is 1 */
void __nf_ct_refresh_acct(struct nf_conn *ct,
			  enum ip_conntrack_info ctinfo,
			  const struct sk_buff *skb,
			  unsigned long extra_jiffies,
			  int do_acct)
{
	NF_CT_ASSERT(ct->timeout.data == (unsigned long)ct);
	NF_CT_ASSERT(skb);

	/* Only update if this is not a fixed timeout */
	if (test_bit(IPS_FIXED_TIMEOUT_BIT, &ct->status))
		goto acct;

	/* If not in hash table, timer will not be active yet */
	if (!nf_ct_is_confirmed(ct)) {
		ct->timeout.expires = extra_jiffies;
	} else {
		unsigned long newtime = jiffies + extra_jiffies;

		/* Only update the timeout if the new timeout is at least
		   HZ jiffies from the old timeout. Need del_timer for race
		   avoidance (may already be dying). */
		if (newtime - ct->timeout.expires >= HZ) {
#if defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT) || defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT_MODULE)
			ip_ct_lru_low_prio_update(ct);
#endif
			mod_timer_pending(&ct->timeout, newtime);
		}
	}

acct:
	if (do_acct) {
		struct nf_conn_counter *acct;

		acct = nf_conn_acct_find(ct);
		if (acct) {
			spin_lock_bh(&ct->lock);
			acct[CTINFO2DIR(ctinfo)].packets++;
			acct[CTINFO2DIR(ctinfo)].bytes +=
				skb->len - skb_network_offset(skb);
			spin_unlock_bh(&ct->lock);
		}
	}
}
EXPORT_SYMBOL_GPL(__nf_ct_refresh_acct);

bool __nf_ct_kill_acct(struct nf_conn *ct,
		       enum ip_conntrack_info ctinfo,
		       const struct sk_buff *skb,
		       int do_acct)
{
	if (do_acct) {
		struct nf_conn_counter *acct;

		acct = nf_conn_acct_find(ct);
		if (acct) {
			spin_lock_bh(&ct->lock);
			acct[CTINFO2DIR(ctinfo)].packets++;
			acct[CTINFO2DIR(ctinfo)].bytes +=
				skb->len - skb_network_offset(skb);
			spin_unlock_bh(&ct->lock);
		}
	}

	if (del_timer(&ct->timeout)) {
		ct->timeout.function((unsigned long)ct);
		return true;
	}
	return false;
}
EXPORT_SYMBOL_GPL(__nf_ct_kill_acct);

#if defined(CONFIG_NF_CT_NETLINK) || defined(CONFIG_NF_CT_NETLINK_MODULE)

#include <linux/netfilter/nfnetlink.h>
#include <linux/netfilter/nfnetlink_conntrack.h>
#include <linux/mutex.h>

/* Generic function for tcp/udp/sctp/dccp and alike. This needs to be
 * in ip_conntrack_core, since we don't want the protocols to autoload
 * or depend on ctnetlink */
int nf_ct_port_tuple_to_nlattr(struct sk_buff *skb,
			       const struct nf_conntrack_tuple *tuple)
{
	NLA_PUT_BE16(skb, CTA_PROTO_SRC_PORT, tuple->src.u.tcp.port);
	NLA_PUT_BE16(skb, CTA_PROTO_DST_PORT, tuple->dst.u.tcp.port);
	return 0;

nla_put_failure:
	return -1;
}
EXPORT_SYMBOL_GPL(nf_ct_port_tuple_to_nlattr);

const struct nla_policy nf_ct_port_nla_policy[CTA_PROTO_MAX+1] = {
	[CTA_PROTO_SRC_PORT]  = { .type = NLA_U16 },
	[CTA_PROTO_DST_PORT]  = { .type = NLA_U16 },
};
EXPORT_SYMBOL_GPL(nf_ct_port_nla_policy);

int nf_ct_port_nlattr_to_tuple(struct nlattr *tb[],
			       struct nf_conntrack_tuple *t)
{
	if (!tb[CTA_PROTO_SRC_PORT] || !tb[CTA_PROTO_DST_PORT])
		return -EINVAL;

	t->src.u.tcp.port = nla_get_be16(tb[CTA_PROTO_SRC_PORT]);
	t->dst.u.tcp.port = nla_get_be16(tb[CTA_PROTO_DST_PORT]);

	return 0;
}
EXPORT_SYMBOL_GPL(nf_ct_port_nlattr_to_tuple);

int nf_ct_port_nlattr_tuple_size(void)
{
	return nla_policy_len(nf_ct_port_nla_policy, CTA_PROTO_MAX + 1);
}
EXPORT_SYMBOL_GPL(nf_ct_port_nlattr_tuple_size);
#endif

/* Used by ipt_REJECT and ip6t_REJECT. */
static void nf_conntrack_attach(struct sk_buff *nskb, struct sk_buff *skb)
{
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;

	/* This ICMP is in reverse direction to the packet which caused it */
	ct = nf_ct_get(skb, &ctinfo);
	if (CTINFO2DIR(ctinfo) == IP_CT_DIR_ORIGINAL)
		ctinfo = IP_CT_RELATED + IP_CT_IS_REPLY;
	else
		ctinfo = IP_CT_RELATED;

	/* Attach to new skbuff, and increment count */
	nskb->nfct = &ct->ct_general;
	nskb->nfctinfo = ctinfo;
	nf_conntrack_get(nskb->nfct);
}

/* Bring out ya dead! */
static struct nf_conn *
get_next_corpse(struct net *net, int (*iter)(struct nf_conn *i, void *data),
		void *data, unsigned int *bucket)
{
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct;
	struct hlist_nulls_node *n;

	spin_lock_bh(&nf_conntrack_lock);
	for (; *bucket < nf_conntrack_htable_size; (*bucket)++) {
		hlist_nulls_for_each_entry(h, n, &net->ct.hash[*bucket], hnnode) {
			ct = nf_ct_tuplehash_to_ctrack(h);
			if (iter(ct, data))
				goto found;
		}
	}
	hlist_nulls_for_each_entry(h, n, &net->ct.unconfirmed, hnnode) {
		ct = nf_ct_tuplehash_to_ctrack(h);
		if (iter(ct, data))
			set_bit(IPS_DYING_BIT, &ct->status);
	}
	spin_unlock_bh(&nf_conntrack_lock);
	return NULL;
found:
	atomic_inc(&ct->ct_general.use);
	spin_unlock_bh(&nf_conntrack_lock);
	return ct;
}

void nf_ct_iterate_cleanup(struct net *net,
			   int (*iter)(struct nf_conn *i, void *data),
			   void *data)
{
	struct nf_conn *ct;
	unsigned int bucket = 0;

	while ((ct = get_next_corpse(net, iter, data, &bucket)) != NULL) {
		/* Time to push up daises... */
		if (del_timer(&ct->timeout))
			death_by_timeout((unsigned long)ct);
		/* ... else the timer will get him soon. */

		nf_ct_put(ct);
	}
}
EXPORT_SYMBOL_GPL(nf_ct_iterate_cleanup);

struct __nf_ct_flush_report {
	u32 pid;
	int report;
};

static int kill_report(struct nf_conn *i, void *data)
{
	struct __nf_ct_flush_report *fr = (struct __nf_ct_flush_report *)data;

	/* If we fail to deliver the event, death_by_timeout() will retry */
	if (nf_conntrack_event_report(IPCT_DESTROY, i,
				      fr->pid, fr->report) < 0)
		return 1;

	/* Avoid the delivery of the destroy event in death_by_timeout(). */
	set_bit(IPS_DYING_BIT, &i->status);
	return 1;
}

static int kill_all(struct nf_conn *i, void *data)
{
	return 1;
}

void do_flush_conntrack_table(void)
{
	nf_ct_iterate_cleanup(&init_net, kill_all, NULL);
}

void nf_ct_free_hashtable(void *hash, int vmalloced, unsigned int size)
{
	if (vmalloced)
		vfree(hash);
	else
		free_pages((unsigned long)hash,
			   get_order(sizeof(struct hlist_head) * size));
}
EXPORT_SYMBOL_GPL(nf_ct_free_hashtable);

void nf_conntrack_flush_report(struct net *net, u32 pid, int report)
{
	struct __nf_ct_flush_report fr = {
		.pid 	= pid,
		.report = report,
	};
	nf_ct_iterate_cleanup(net, kill_report, &fr);
}
EXPORT_SYMBOL_GPL(nf_conntrack_flush_report);

static void nf_ct_release_dying_list(void)
{
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct;
	struct hlist_nulls_node *n;

	spin_lock_bh(&nf_conntrack_lock);
	hlist_nulls_for_each_entry(h, n, &init_net.ct.dying, hnnode) {
		ct = nf_ct_tuplehash_to_ctrack(h);
		/* never fails to remove them, no listeners at this point */
		nf_ct_kill(ct);
	}
	spin_unlock_bh(&nf_conntrack_lock);
}

static void nf_conntrack_cleanup_init_net(void)
{
	nf_conntrack_helper_fini();
	nf_conntrack_proto_fini();
	kmem_cache_destroy(nf_conntrack_cachep);
#if defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT) || defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT_MODULE)
	kmem_cache_destroy(nf_ct_natlan_cachep);
	free_pages((unsigned long) tcp_high_prio_port_table, get_order(65536/8));
	free_pages((unsigned long) udp_high_prio_port_table, get_order(65536/8));
#endif
}

static void nf_conntrack_cleanup_net(struct net *net)
{
 i_see_dead_people:
	nf_ct_iterate_cleanup(net, kill_all, NULL);
	nf_ct_release_dying_list();
	if (atomic_read(&net->ct.count) != 0) {
		schedule();
		goto i_see_dead_people;
	}
	/* wait until all references to nf_conntrack_untracked are dropped */
	while (atomic_read(&nf_conntrack_untracked.ct_general.use) > 1)
		schedule();

	nf_ct_free_hashtable(net->ct.hash, net->ct.hash_vmalloc,
			     nf_conntrack_htable_size);
	nf_conntrack_ecache_fini(net);
	nf_conntrack_acct_fini(net);
	nf_conntrack_expect_fini(net);
	free_percpu(net->ct.stat);
}

/* Mishearing the voices in his head, our hero wonders how he's
   supposed to kill the mall. */
void nf_conntrack_cleanup(struct net *net)
{
	if (net_eq(net, &init_net))
		rcu_assign_pointer(ip_ct_attach, NULL);

	/* This makes sure all current packets have passed through
	   netfilter framework.  Roll on, two-stage module
	   delete... */
	synchronize_net();

	nf_conntrack_cleanup_net(net);

	if (net_eq(net, &init_net)) {
		rcu_assign_pointer(nf_ct_destroy, NULL);
		nf_conntrack_cleanup_init_net();
	}
#if defined(CONFIG_NF_CONNTRACK_NAT_UDP_MANAGEMENT)
	nf_ct_free_hashtable(dni_ct_hash, dni_ct_hash_vmalloc,
			     nf_conntrack_htable_size);
#endif
#if defined(CONFIG_NF_CONNTRACK_CONENAT_MANAGEMENT)
	nf_ct_free_hashtable(conenat_ct_hash, conenat_ct_hash_vmalloc,
			     nf_conntrack_htable_size);
#endif
}

void *nf_ct_alloc_hashtable(unsigned int *sizep, int *vmalloced, int nulls)
{
	struct hlist_nulls_head *hash;
	unsigned int nr_slots, i;
	size_t sz;

	*vmalloced = 0;

	BUILD_BUG_ON(sizeof(struct hlist_nulls_head) != sizeof(struct hlist_head));
	nr_slots = *sizep = roundup(*sizep, PAGE_SIZE / sizeof(struct hlist_nulls_head));
	sz = nr_slots * sizeof(struct hlist_nulls_head);
	hash = (void *)__get_free_pages(GFP_KERNEL | __GFP_NOWARN | __GFP_ZERO,
					get_order(sz));
	if (!hash) {
		*vmalloced = 1;
		printk(KERN_WARNING "nf_conntrack: falling back to vmalloc.\n");
		hash = __vmalloc(sz, GFP_KERNEL | __GFP_ZERO, PAGE_KERNEL);
	}

	if (hash && nulls)
		for (i = 0; i < nr_slots; i++)
			INIT_HLIST_NULLS_HEAD(&hash[i], i);

	return hash;
}
EXPORT_SYMBOL_GPL(nf_ct_alloc_hashtable);

int nf_conntrack_set_hashsize(const char *val, struct kernel_param *kp)
{
	int i, bucket, vmalloced, old_vmalloced;
	unsigned int hashsize, old_size;
	int rnd;
	struct hlist_nulls_head *hash, *old_hash;
	struct nf_conntrack_tuple_hash *h;

	/* On boot, we can set this without any fancy locking. */
	if (!nf_conntrack_htable_size)
		return param_set_uint(val, kp);

	hashsize = simple_strtoul(val, NULL, 0);
	if (!hashsize)
		return -EINVAL;

	hash = nf_ct_alloc_hashtable(&hashsize, &vmalloced, 1);
	if (!hash)
		return -ENOMEM;

	/* We have to rehahs for the new table anyway, so we also can
	 * use a newrandom seed */
	get_random_bytes(&rnd, sizeof(rnd));

	/* Lookups in the old hash might happen in parallel, which means we
	 * might get false negatives during connection lookup. New connections
	 * created because of a false negative won't make it into the hash
	 * though since that required taking the lock.
	 */
	spin_lock_bh(&nf_conntrack_lock);
	for (i = 0; i < nf_conntrack_htable_size; i++) {
		while (!hlist_nulls_empty(&init_net.ct.hash[i])) {
			h = hlist_nulls_entry(init_net.ct.hash[i].first,
					struct nf_conntrack_tuple_hash, hnnode);
			hlist_nulls_del_rcu(&h->hnnode);
			bucket = __hash_conntrack(&h->tuple, hashsize, rnd);
			hlist_nulls_add_head_rcu(&h->hnnode, &hash[bucket]);
		}
	}
	old_size = nf_conntrack_htable_size;
	old_vmalloced = init_net.ct.hash_vmalloc;
	old_hash = init_net.ct.hash;

	nf_conntrack_htable_size = hashsize;
	init_net.ct.hash_vmalloc = vmalloced;
	init_net.ct.hash = hash;
	nf_conntrack_hash_rnd = rnd;
	spin_unlock_bh(&nf_conntrack_lock);

	nf_ct_free_hashtable(old_hash, old_vmalloced, old_size);
	return 0;
}
EXPORT_SYMBOL_GPL(nf_conntrack_set_hashsize);

module_param_call(hashsize, nf_conntrack_set_hashsize, param_get_uint,
		  &nf_conntrack_htable_size, 0600);

static int nf_conntrack_init_init_net(void)
{
	int max_factor = 8;
	int ret;
#if defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT) || defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT_MODULE)
	int i;
#endif

	/* Idea from tcp.c: use 1/16384 of memory.  On i386: 32MB
	 * machine has 512 buckets. >= 1GB machines have 16384 buckets. */
	if (!nf_conntrack_htable_size) {
		nf_conntrack_htable_size
			= (((num_physpages << PAGE_SHIFT) / 16384)
			   / sizeof(struct hlist_head));
		if (num_physpages > (1024 * 1024 * 1024 / PAGE_SIZE))
			nf_conntrack_htable_size = 16384;
		if (nf_conntrack_htable_size < 32)
			nf_conntrack_htable_size = 32;

		/* Use a max. factor of four by default to get the same max as
		 * with the old struct list_heads. When a table size is given
		 * we use the old value of 8 to avoid reducing the max.
		 * entries. */
		max_factor = 4;
	}
	nf_conntrack_max = max_factor * nf_conntrack_htable_size;

#if defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT) || defined(CONFIG_NF_CONNTRACK_NAT_MANAGEMENT_MODULE)
	ret = -ENOMEM;

	nf_ct_natlan_cachep = kmem_cache_create("nf_ct_natlan",
						sizeof(struct nf_conn_lan),
						0, 0, NULL);
	if (!nf_ct_natlan_cachep)
		goto err_cache;

	tcp_high_prio_port_table = (unsigned long *)
		__get_free_pages(GFP_KERNEL | __GFP_ZERO, get_order(65536/8));
	udp_high_prio_port_table = (unsigned long *)
		__get_free_pages(GFP_KERNEL | __GFP_ZERO, get_order(65536/8));

	if (!tcp_high_prio_port_table || !udp_high_prio_port_table)
		goto err_cache;

	for (i = 0; i < NAT_LAN_HASH_SIZE; i++)
		INIT_LIST_HEAD(&nat_lans[i]);
	init_high_prio_tables();

	printk("NAT Session Management application ports priority table is loaded!\n");

#if defined(CONFIG_NF_CONNTRACK_LOCAL_MANAGEMENT)
	//Windows8.1 HCK test can not passed.The router will drop the package as the port
	//is not enough.change nf_conntrack_loca_max  to 65535 from 4096
	nf_conntrack_local_max = 65535;
	nf_conntrack_icmp_reserve_max = 100;
	nf_conntrack_tcp_reserve_max = 100;
#endif
#endif

	printk("nf_conntrack version %s (%u buckets, %d max)\n",
	       NF_CONNTRACK_VERSION, nf_conntrack_htable_size,
	       nf_conntrack_max);

	nf_conntrack_cachep = kmem_cache_create("nf_conntrack",
						sizeof(struct nf_conn),
						0, SLAB_DESTROY_BY_RCU, NULL);
	if (!nf_conntrack_cachep) {
		printk(KERN_ERR "Unable to create nf_conn slab cache\n");
		ret = -ENOMEM;
		goto err_cache;
	}

	ret = nf_conntrack_proto_init();
	if (ret < 0)
		goto err_proto;

	ret = nf_conntrack_helper_init();
	if (ret < 0)
		goto err_helper;

	return 0;

err_helper:
	nf_conntrack_proto_fini();
err_proto:
	kmem_cache_destroy(nf_conntrack_cachep);
err_cache:
	return ret;
}

/*
 * We need to use special "null" values, not used in hash table
 */
#define UNCONFIRMED_NULLS_VAL	((1<<30)+0)
#define DYING_NULLS_VAL		((1<<30)+1)

static int nf_conntrack_init_net(struct net *net)
{
	int ret;

	atomic_set(&net->ct.count, 0);
	INIT_HLIST_NULLS_HEAD(&net->ct.unconfirmed, UNCONFIRMED_NULLS_VAL);
	INIT_HLIST_NULLS_HEAD(&net->ct.dying, DYING_NULLS_VAL);
	net->ct.stat = alloc_percpu(struct ip_conntrack_stat);
	if (!net->ct.stat) {
		ret = -ENOMEM;
		goto err_stat;
	}
	net->ct.hash = nf_ct_alloc_hashtable(&nf_conntrack_htable_size,
					     &net->ct.hash_vmalloc, 1);
	if (!net->ct.hash) {
		ret = -ENOMEM;
		printk(KERN_ERR "Unable to create nf_conntrack_hash\n");
		goto err_hash;
	}

#if defined(CONFIG_NF_CONNTRACK_NAT_UDP_MANAGEMENT)
	dni_ct_hash = nf_ct_alloc_hashtable(&nf_conntrack_htable_size,
					    &dni_ct_hash_vmalloc, 1);
	if (!dni_ct_hash) {
		ret = -ENOMEM;
		printk(KERN_ERR "Unable to create dni conntrack hash\n");
		goto err_dni_hash;
	}
#endif

#if defined(CONFIG_NF_CONNTRACK_CONENAT_MANAGEMENT)
	conenat_ct_hash = nf_ct_alloc_hashtable(&nf_conntrack_htable_size,
					    &conenat_ct_hash_vmalloc, 1);
	if (!conenat_ct_hash) {
		ret = -ENOMEM;
		printk(KERN_ERR "Unable to create conenat conntrack hash\n");
		goto err_conenat_hash;
	}
#endif

	ret = nf_conntrack_expect_init(net);
	if (ret < 0)
		goto err_expect;
	ret = nf_conntrack_acct_init(net);
	if (ret < 0)
		goto err_acct;
	ret = nf_conntrack_ecache_init(net);
	if (ret < 0)
		goto err_ecache;

	/* Set up fake conntrack:
	    - to never be deleted, not in any hashes */
#ifdef CONFIG_NET_NS
	nf_conntrack_untracked.ct_net = &init_net;
#endif
	atomic_set(&nf_conntrack_untracked.ct_general.use, 1);
	/*  - and look it like as a confirmed connection */
	set_bit(IPS_CONFIRMED_BIT, &nf_conntrack_untracked.status);

	return 0;

err_ecache:
	nf_conntrack_acct_fini(net);
err_acct:
	nf_conntrack_expect_fini(net);
err_expect:
#if defined(CONFIG_NF_CONNTRACK_CONENAT_MANAGEMENT)
	nf_ct_free_hashtable(conenat_ct_hash, conenat_ct_hash_vmalloc,
			     nf_conntrack_htable_size);
err_conenat_hash:
#endif
#if defined(CONFIG_NF_CONNTRACK_NAT_UDP_MANAGEMENT)
	nf_ct_free_hashtable(dni_ct_hash, dni_ct_hash_vmalloc,
			     nf_conntrack_htable_size);
err_dni_hash:
#endif
	nf_ct_free_hashtable(net->ct.hash, net->ct.hash_vmalloc,
			     nf_conntrack_htable_size);
err_hash:
	free_percpu(net->ct.stat);
err_stat:
	return ret;
}

int nf_conntrack_init(struct net *net)
{
	int ret;

	if (net_eq(net, &init_net)) {
		ret = nf_conntrack_init_init_net();
		if (ret < 0)
			goto out_init_net;
	}
	ret = nf_conntrack_init_net(net);
	if (ret < 0)
		goto out_net;

	if (net_eq(net, &init_net)) {
		/* For use by REJECT target */
		rcu_assign_pointer(ip_ct_attach, nf_conntrack_attach);
		rcu_assign_pointer(nf_ct_destroy, destroy_conntrack);
	}
	return 0;

out_net:
	if (net_eq(net, &init_net))
		nf_conntrack_cleanup_init_net();
out_init_net:
	return ret;
}
