/*
 * dni_netfilter_sysctl.c: DNI netfilter sysctl interface to net subsystem.
 *
 * Copyright (C) 2010 Delta Networks Inc.
 *
 */
#ifdef CONFIG_SYSCTL
#include <linux/sysctl.h>
#include <net/netfilter/nf_conntrack.h>

#include <linux/rculist_nulls.h>
#include <net/netfilter/nf_conntrack_core.h>

extern int sysctl_conntrack_refresh_support;
#if defined(CONFIG_NF_CONNTRACK_IPV6) || defined(CONFIG_NF_CONNTRACK_IPV6_MODULE)
int ipv6_ip6frag_not_check_icmp = 0;
EXPORT_SYMBOL(ipv6_ip6frag_not_check_icmp);
#endif
int sysctl_conntrack_refresh_outbound_only = 0;
int sysctl_do_flush_conntrack = 0;
int sysctl_nat_filtering_behavior= 0;


EXPORT_SYMBOL(sysctl_conntrack_refresh_outbound_only);
EXPORT_SYMBOL(sysctl_nat_filtering_behavior);

#if defined(CONFIG_NF_CONNTRACK_LOCAL_MANAGEMENT)
unsigned int nf_conntrack_local_max __read_mostly;
atomic_t nf_conntrack_local_count = ATOMIC_INIT(0);
EXPORT_SYMBOL(nf_conntrack_local_max);
EXPORT_SYMBOL(nf_conntrack_local_count);

unsigned int nf_conntrack_icmp_reserve_max __read_mostly;
atomic_t nf_conntrack_icmp_reserve_count = ATOMIC_INIT(0);
EXPORT_SYMBOL(nf_conntrack_icmp_reserve_max);
EXPORT_SYMBOL(nf_conntrack_icmp_reserve_count);

unsigned int nf_conntrack_tcp_reserve_max __read_mostly;
atomic_t nf_conntrack_tcp_reserve_count = ATOMIC_INIT(0);
EXPORT_SYMBOL(nf_conntrack_tcp_reserve_max);
EXPORT_SYMBOL(nf_conntrack_tcp_reserve_count);
#endif

static int proc_flush_conntrack(ctl_table * ctl,
				int write, struct file *filp,
				void __user * buffer, size_t * lenp,
				loff_t * ppos)
{
	proc_dointvec(ctl, write, filp, buffer, lenp, ppos);
	if (write && (sysctl_do_flush_conntrack & 1))
		do_flush_conntrack_table();
	return 0;
}

#if defined(CONFIG_NF_CONNTRACK_NAT_UDP_MANAGEMENT)
static int reset_dni_conntrack_hash(unsigned int old_size)
{
	int i, bucket, vmalloced, old_vmalloced;
	unsigned int hashsize;
	struct hlist_nulls_head *hash, *old_hash;
	struct nf_conn *ct;

	if (!old_size)
		return -EINVAL;

	hashsize = nf_conntrack_htable_size;
	hash = nf_ct_alloc_hashtable(&hashsize, &vmalloced, 1);
	if (!hash)
		return -ENOMEM;

	spin_lock_bh(&nf_conntrack_lock);
	for (i = 0; i < old_size; i++) {
		while (!hlist_nulls_empty(&dni_ct_hash[i])) {
			ct = hlist_nulls_entry(dni_ct_hash[i].first, struct nf_conn, hashnode);
			hlist_nulls_del_rcu(&ct->hashnode);
			if (ct->status & IPS_SRC_NAT)
				bucket = dni_hash_conntrack(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
			else
				bucket = dni_hash_conntrack(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);

			hlist_nulls_add_head_rcu(&ct->hashnode, &hash[bucket]);
		}
	}

	old_vmalloced = dni_ct_hash_vmalloc;
	old_hash = dni_ct_hash;

	dni_ct_hash_vmalloc = vmalloced;
	dni_ct_hash = hash;
	spin_unlock_bh(&nf_conntrack_lock);

	nf_ct_free_hashtable(old_hash, old_vmalloced, old_size);
	return 0;
}
#endif

#if defined(CONFIG_NF_CONNTRACK_CONENAT_MANAGEMENT)
static int reset_conenat_conntrack_hash(unsigned int old_size)
{
	int i, bucket, vmalloced, old_vmalloced;
	unsigned int hashsize;
	struct hlist_nulls_head *hash, *old_hash;
	struct nf_conn *ct;

	if (!old_size)
		return -EINVAL;

	hashsize = nf_conntrack_htable_size;
	hash = nf_ct_alloc_hashtable(&hashsize, &vmalloced, 1);
	if (!hash)
		return -ENOMEM;

	spin_lock_bh(&nf_conntrack_lock);
	for (i = 0; i < old_size; i++) {
		while (!hlist_nulls_empty(&conenat_ct_hash[i])) {
			ct = hlist_nulls_entry(conenat_ct_hash[i].first, struct nf_conn, conenat_hashnode);
			hlist_nulls_del_rcu(&ct->conenat_hashnode);
			if (ct->status & IPS_SRC_NAT)
				bucket = conenat_hash_conntrack(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);

			hlist_nulls_add_head_rcu(&ct->conenat_hashnode, &hash[bucket]);
		}
	}

	old_vmalloced = conenat_ct_hash_vmalloc;
	old_hash = conenat_ct_hash;

	conenat_ct_hash_vmalloc = vmalloced;
	conenat_ct_hash = hash;
	spin_unlock_bh(&nf_conntrack_lock);
	nf_ct_free_hashtable(old_hash, old_vmalloced, old_size);
	return 0;
}
#endif

static int proc_nat_filtering_behavior(ctl_table * ctl,
				       int write, struct file *filp,
				       void __user * buffer, size_t * lenp,
				       loff_t * ppos)
{
	int old_type;

	old_type = sysctl_nat_filtering_behavior;
	proc_dointvec(ctl, write, filp, buffer, lenp, ppos);

#if defined(CONFIG_NF_CONNTRACK_NAT_UDP_MANAGEMENT)
	if (write && (old_type != sysctl_nat_filtering_behavior))
		reset_dni_conntrack_hash(nf_conntrack_htable_size);
#endif

#if defined(CONFIG_NF_CONNTRACK_CONENAT_MANAGEMENT)
	if (write && (old_type != sysctl_nat_filtering_behavior))
		reset_conenat_conntrack_hash(nf_conntrack_htable_size);
#endif

	return 0;
}

ctl_table dni_table[] = {
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "conntrack_refresh_support",
		.data		= &sysctl_conntrack_refresh_support,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
		.strategy	= sysctl_intvec,
	},
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "conntrack_refresh_outbound_only",
		.data		= &sysctl_conntrack_refresh_outbound_only,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
		.strategy	= sysctl_intvec,
	},
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "flush_conntrack_table",
		.data		= &sysctl_do_flush_conntrack,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_flush_conntrack,
		.strategy	= sysctl_intvec,
	},
#if defined(CONFIG_NF_CONNTRACK_IPV6) || defined(CONFIG_NF_CONNTRACK_IPV6_MODULE)
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "ipv6_ip6frag_not_check_icmp",
		.data		= &ipv6_ip6frag_not_check_icmp,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &proc_dointvec,
        },
#endif
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "nat_filtering_behavior",
		.data		= &sysctl_nat_filtering_behavior,  /* 0:secured mode, 1: open mode */
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_nat_filtering_behavior,
		.strategy	= sysctl_intvec,
	},
#if defined(CONFIG_NF_CONNTRACK_LOCAL_MANAGEMENT)
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "nf_conntrack_local_max",
		.data		= &nf_conntrack_local_max,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "nf_conntrack_local_count",
		.data		= &nf_conntrack_local_count,
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= proc_dointvec,
	},
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "nf_conntrack_icmp_reserve_max",
		.data		= &nf_conntrack_icmp_reserve_max,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "nf_conntrack_icmp_reserve_count",
		.data		= &nf_conntrack_icmp_reserve_count,
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= proc_dointvec,
	},
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "nf_conntrack_tcp_reserve_max",
		.data		= &nf_conntrack_tcp_reserve_max,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "nf_conntrack_tcp_reserve_count",
		.data		= &nf_conntrack_tcp_reserve_count,
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= proc_dointvec,
	},
#endif
	{0}
};

ctl_table dni_netfilter_sysctl_table[2] = {
	{
		.ctl_name	= CTL_UNNUMBERED,
		.procname	= "dni_netfilter",
		.mode		= 0555,
		.child		= dni_table
	},
	{0}
};
EXPORT_SYMBOL(dni_netfilter_sysctl_table);
#endif
