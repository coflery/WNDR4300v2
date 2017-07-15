/* 
 * Copyright (c) 2014 Qualcomm Atheros, Inc.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 */

/*
 * Hybrid bridge definitions and data types
 *
 * netlink message format:
 *  ------------------------------------------
 * | NLMSG_HEADER | HYMSG_HEADER | HYMSG_DATA |
 *  ------------------------------------------
 * HYMSG_DATA format for the different message types:
 *
 * HYCTL_GET_HA_TABLE:          multiple of struct __hatbl_entry
 * HYCTL_GET_HD_TABLE:          multiple of struct __hdtbl_entry
 * HYCTL_UPDATE_HATBL_ENTRIES:  multiple of struct __hatbl_entry
 * HYCTL_UPDATE_HDTBL_ENTRIES:  multiple of struct __hdtbl_entry
 * HYCTL_ADD_HDTBL_ENTRIES:     multiple of struct __hdtbl_entry
 * HYCTL_DEL_HDTBL_ENTRIES:     multiple of struct __hdtbl_entry
 * HYCTL_FLUSH_HDTBL:
 * HYCTL_SET_HATBL_AGING_PARAM: struct __aging_param
 * HYCTL_ADD_BRIDGE:
 * HYCTL_DEL_BRIDGE:
 * HYCTL_ADD_IF:                ifindex (u32)
 * HYCTL_DEL_IF:                ifindex (u32)
 * HYCTL_GET_BRIDGES:           multiple of struct __hybr_info
 * HYCTL_GET_PORT_LIST:         multiple of struct __brport_group
 * HYCTL_SET_EVENT_INFO:        struct __event_info
 * HYCTL_SET_BRIDGE_MODE:       bridge mode (HR or HC) (u32)
 * HYCTL_SET_BRPORT_GROUP:      group num and type (u32)
 * HYCTL_FLUSH_HATBL:
 * HYCTL_ADD_HATBL_ENTRIES:     multiple of struct __hatbl_entry
 * HYCTL_DEL_HDTBL_ENTRIES_BYID: multiple of struct __hdtbl_entry
 * HYCTL_SET_BRPORT_BCAST:      struct __brport_group
 * HYCTL_SET_TCP_SP:            __u32
 * HYCTL_SET_BRPORT_TYPE:       struct __brport_group
 *
 *
 * netlink event format:
 *  ------------------------------------------
 * | NLMSG_HEADER | EVENT_DATA |
 *  ------------------------------------------
 * HY_EVENT_ADD_HA_ENTRY:       struct __hatbl_entry
 * HY_EVENT_DEL_HA_ENTRY:       struct __hatbl_entry
 * HY_EVENT_AGEOUT_HA_ENTRIES:
 * HY_EVENT_LINK_UP:              ifindex (u32)
 * HY_EVENT_LINK_DOWN:            ifindex (u32)
 */

#ifndef _IF_HYBRID_BRIDGE_H
#define _IF_HYBRID_BRIDGE_H

#include <linux/types.h>

/* netlink define */
/* netlink port */
#define NETLINK_ATHEROS_HY 25
#define NETLINK_ATHEROS_HY_EVENT 26
#define HY_IFNAME_SZ       16

/* netlink message type */
enum
{
    HYCTL_HYFI_FIRST = 0, /* Hy-Fi bridge range 0 - 0x100 */
    HYCTL_GET_HA_TABLE = HYCTL_HYFI_FIRST,
    HYCTL_GET_HD_TABLE,
    HYCTL_GET_FDB,
    HYCTL_UPDATE_HATBL_ENTRIES,
    HYCTL_UPDATE_HDTBL_ENTRIES,
    HYCTL_ADD_HDTBL_ENTRIES,
    HYCTL_DEL_HDTBL_ENTRIES,
    HYCTL_FLUSH_HDTBL,
    HYCTL_SET_HATBL_AGING_PARAM,
    HYCTL_ADD_BRIDGE,
    HYCTL_DEL_BRIDGE,
    HYCTL_ADD_IF,
    HYCTL_DEL_IF,
    HYCTL_GET_BRIDGES,
    HYCTL_GET_PORT_LIST,
    HYCTL_SET_EVENT_INFO,
    HYCTL_SET_BRIDGE_MODE,
    HYCTL_SET_BRPORT_GROUP,
    HYCTL_FLUSH_HATBL,
    HYCTL_ADD_HATBL_ENTRIES,
    HYCTL_DEL_HDTBL_ENTRIES_BYID,
    HYCTL_SET_BRPORT_BCAST,
    HYCTL_SET_TCP_SP,
    HYCTL_SET_PATHSWITCH_PARAM,
    HYCTL_SET_BRPORT_TYPE,
    HYCTL_UPDATE_HATBL_ENTRY,
    HYCTL_DEL_FDB_ENTRY,
    HYCTL_HYFI_LAST,

    HYCTL_MC_FIRST = 0x100, /* Multicast range 0x100 - 0x200 */
    HYCTL_SET_MC_ENABLE = HYCTL_MC_FIRST,
    HYCTL_SET_MC_DEBUG,
    HYCTL_SET_MC_POLICY,
    HYCTL_SET_MC_MEMBERSHIP_INTERVAL,
    HYCTL_SET_MC_RETAG,
    HYCTL_SET_MC_ROUTER_PORT,
    HYCTL_SET_MC_ADD_ACL_RULE,
    HYCTL_SET_MC_FLUSH_ACL_RULE,
    HYCTL_SET_MC_CONVERT_ALL,
    HYCTL_SET_MC_TIMEOUT,
    HYCTL_SET_MC_M2I3_FILTER,
    HYCTL_SET_MC_TBIT,
    HYCTL_SET_MC_LOCAL_QUERY_INTERVAL,
    HYCTL_SET_MC_PSW_ENCAP,
    HYCTL_SET_MC_PSW_FLOOD,
    HYCTL_GET_MC_ACL,
    HYCTL_GET_MC_MDB,
    HYCTL_SET_MC_EXTRA_QUERY_RESPONSE_TIME,
    HYCTL_SET_MC_LOCAL_INSPECT,
    HYCTL_SET_MC_QRV_THRESHOLD,
    HYCTL_MC_LAST,

    HYCTL_PSW_FIRST = 0x200, /* Advanced path switching range 0x200 - 0x300 */
    HYCTL_SET_PSW_DEBUG = HYCTL_PSW_FIRST,
    HYCTL_SET_PSW_MSE_TIMEOUT,
    HYCTL_SET_PSW_DROP_MARKERS,
    HYCTL_SET_PSW_OLD_IF_QUIET_TIME,
    HYCTL_SET_PSW_DUP_PKT_FLUSH_QUOTA,
    HYCTL_PSW_LAST
};

/* Bridge mode:
 * The Hybrid bridge can be programmed in two modes:
 * 1. Relay override mode: Allow packet relaying from every port in every group to any other port in any other port.
 * 2. Group type relay mode: Packet relay decision is done according to the group type.
 */
#define HY_BRIDGE_MODE_RELAY_OVERRIDE        1

/* Group type:
 * A port can belong to either 2 groups:
 * 1. Relay group: Where packets are relayed from each port to every other port
 * 2. Non-relay group: Where packets are not relayed between group ports.
 *
 * Note that this has no effect in relay override bridge mode.
 */
#define HY_PORTGRP_TYPE_RELAY     1

/* The hyInterfaceType enum is deprecated and should not be used anymore.
 * It is kept here temporarily until it will be cleaned completely.
 * The following macro will break the compilation of any user space
 * application which is trying to use this enum.
 */
#ifndef __KERNEL__
#define __HY_MISUSE_PROTECT( x )    __##x
#else
#define __HY_MISUSE_PROTECT( x )   x
#endif

enum __HY_MISUSE_PROTECT( hyInterfaceType )
{
    __HY_MISUSE_PROTECT( hyInterface_WIFI_2G ) = 0, /* Always leave W2G as '0': used as index by pcw service */
    __HY_MISUSE_PROTECT( hyInterface_WIFI_5G ),
    __HY_MISUSE_PROTECT( hyInterface_HPAV ),
    __HY_MISUSE_PROTECT( hyInterface_ETH ),

    __HY_MISUSE_PROTECT( hyInterface_NumberOfChildInterfaces ), /* Use this constant for arrays containing only child interfaces */

    __HY_MISUSE_PROTECT( hyInterface_HY_BRIDGE ) = __HY_MISUSE_PROTECT( hyInterface_NumberOfChildInterfaces ),

    __HY_MISUSE_PROTECT( hyInterface_NumberOfInterfaces )
};


struct __hyctl_msg_header
{
    char  if_name[HY_IFNAME_SZ];  /* hybrid bridge name: hy0 */
    __u32 buf_len;                /* not include this header */
    __u32 tbl_offsite;            /* how many entries to skip */
    __u32 status;
    __u32 bytes_written;
    __u32 bytes_needed;
    /* __u8  data[1]; */
} __attribute__ ((packed));

#define HYMSG_HDRLEN       sizeof (struct __hyctl_msg_header)
#define HYMSG_DATA(p)      ((void*)(((char*)p) + NLMSG_LENGTH(0) + HYMSG_HDRLEN))
/* define for the status field */
enum
{
     HYCTL_STATUS_SUCCESS           = 0,
     HYCTL_STATUS_NOT_SUPPORTED     = 1,
     HYCTL_STATUS_RESOURCES         = 2,
     HYCTL_STATUS_INVALID_PARAMETER = 3,
     HYCTL_STATUS_BUFFER_OVERFLOW   = 4,
     HYCTL_STATUS_FAILURE           = 5,
     HYCTL_STATUS_NOT_FOUND         = 6,
};

#define HY_TRAFFIC_CLASS_OTHER 0
#define HY_TRAFFIC_CLASS_UDP   1

struct __aging_param
{
    __u32 aging_time;
};

struct __event_info
{
    __u32 event_pid;
};

struct __hybr_info
{
    __u32 ifindex;
    __u32 br_opmode;
    __u32 tcpsp_enable;
};

struct __brport_group
{
    __u32 ifindex;
    __u32 group_num;
    __u32 group_type;
    __u32 bcast_enable;
    __u32 port_type;
};

/* netlink event type */
enum
{
     HY_EVENT_ADD_HA_ENTRY         = 1,
     HY_EVENT_DEL_HA_ENTRY         = 2,
     HY_EVENT_AGEOUT_HA_ENTRIES    = 3,
     HY_EVENT_LINK_UP              = 4,
     HY_EVENT_LINK_DOWN            = 5,
     HY_EVENT_FDB_UPDATED          = 6,
     HY_EVENT_MDB_UPDATED          = 7,
};

struct __hfdb_entry
{
    __u8 mac_addr[6];
    __u8 port_no;
    __u8 is_local;
    __u32 ageing_timer_value;
    __u8 port_hi;
    __u8 pad0;
    __u16 unused;
} __attribute__ ((packed));

struct __hdtbl_entry
{
	__u8 mac_addr[6];
	__u8 id[6];
	__u32 udp_port;         /* udp port ifindex */
	__u32 other_port;       /* other port ifindex */
	__u32 static_entry;
} __attribute__ ((packed));

struct __hatbl_entry
{
 	__u16 hash;
    __u8  sa[6];
    __u8  da[6];
	__u8  id[6];
	__u16 sub_class;
	__u32 port;             /* port ifindex */
	__u32 age;              /* since last access */
	__u32 create_time;      /* ha entry create time */
	__u32 num_packets;
	__u32 num_bytes;
	__u32 action:3;         /* drop, throttle */
	__u32 local:1;          /* not created from HD */
	__u32 psw_enable:1;     /*Enable per stream path switch */
	__u32 static_entry:1;
        __u32 psw_use:1; /* Forward tracked packet to new interface */
	__u32 priority;

	__u32 reserved;         /* Reserved */
} __attribute__ ((packed));

#define HATBL_PRIORITY_DSCP_VALID (1 << 31)
#define HATBL_PRIORITY_8021_VALID (1 << 30)
#define HATBL_PRIORITY_8021_MASK  0x00000007
#define HATBL_PRIORITY_8021_SHIFT 0
#define HATBL_PRIORITY_DSCP_MASK  0x000001F8
#define HATBL_PRIORITY_DSCP_SHIFT 3

struct __path_switch_param
{
    __u32 enable_path_switch;
    __u32 wifi_2_q_max_len;
    __u32 wifi_2_tracking_time;
    __u32 wifi_5_q_max_len;
    __u32 wifi_5_tracking_time;
    __u32 plc_q_max_len;
    __u32 plc_tracking_time;
    __u32 eth_q_max_len;
    __u32 eth_tracking_time;
    __u32 enable_switch_markers;
};

struct __mc_param_value
{
    __u32 val;
};

enum
{
    MC_POLICY_DROP = 0,
    MC_POLICY_FLOOD
};

struct __mc_param_retag
{
    __u32 enable;
    __u32 dscp;
};

enum
{
    MC_RTPORT_FLOOD = 0,
    MC_RTPORT_DROP,
    MC_RTPORT_SPECIFY,
    MC_RTPORT_DEFAULT,
    MC_RTPORT_MAX
};
struct __mc_param_router_port
{
    __u32 type;
    __u32 ifindex;
};

enum
{
    MC_ACL_RULE_DISABLE = 0,
    MC_ACL_RULE_MULTICAST,
    MC_ACL_RULE_SWM,     /* system wide management */
    MC_ACL_RULE_MANAGEMENT,
    MC_ACL_RULE_NON_SNOOPING,
    MC_ACL_RULE_INTERNAL,
    MC_ACL_RULE_MAX
};
enum
{
    MC_ACL_PATTERN_IGMP = 0,
    MC_ACL_PATTERN_MLD,
    MC_ACL_PATTERN_MAX
};
#define MC_ACL_RULE_MAX_COUNT 8 /* 8 for IGMP, 8 for MLD */
struct __mc_param_pattern
{
    __u32 rule;
    __u8  mac[6];
    __u8  mac_mask[6];
    __u8  ip[16];
    __u8  ip_mask[16];
} __attribute__ ((packed));
struct __mc_param_acl_rule
{
    __u32 pattern_type; /* IGMP or MLD */
    struct __mc_param_pattern pattern;
} __attribute__ ((packed));

enum
{
    MC_TIMEOUT_FROM_GROUP_SPECIFIC_QUERIES = 0,
    MC_TIMEOUT_FROM_ALL_SYSTEM_QUERIES,
    MC_TIMEOUT_FROM_GROUP_MEMBERSHIP_INTERVAL
};

struct __mc_param_timeout
{
    __u32 from;
    __u32 enable;
};

#define HY_MC_SRCS_MAX      4
#define HY_MC_RT_SRCS_MAX   8
#define HY_MC_DEV_MAX       64
#define HY_MC_IF_MAX        16
#define HY_MC_IF_NODE_MAX   8
#define HY_MC_GROUP_MAX     16
#define HY_MC_IP6_SIZE      16
//Do not change the below two macro values vice versa 
#define HY_MC_EXCLUDE       1
#define HY_MC_INCLUDE       2
/* 
The above two macros HY_MC_EXCLUDE and HY_MC_INCLUDE has been modified 
to align those values with what being used in IGMP INC/EXC right now. 

Please refer below the macro values and with ieee80211_ique.h. 
#define IGMP_SNOOP_CMD_ADD_EXC_LIST  1
#define IGMP_SNOOP_CMD_ADD_INC_LIST  2
*/ 

#define HY_MC_EX_SRCS_INVAL 0xff

struct __mc_group
{
    __u32 pro;
    union {
        __u32 ip4;
        __u8  ip6[HY_MC_IP6_SIZE];
    } u;
};

struct __mc_mdb_entry
{
    struct __mc_group       group;
    __u32                   ifindex;
    __u32                   nsrcs;
    __u8                    srcs[HY_MC_SRCS_MAX * HY_MC_IP6_SIZE];
    __u32                   aging;
    __u8                    filter_mode;
    __u8                    fdb_age_out;
    __u8                    mac[6];
} __attribute__ ((packed));

struct __mc_encaptbl_dev
{
    __u8                    mac[6];
    __u16                   pad;
    __u32                   in_nsrcs;
    __u8                    in_srcs[HY_MC_RT_SRCS_MAX * HY_MC_IP6_SIZE]; /* include sources list */
    __u32                   ex_nsrcs;
    __u8                    ex_srcs[HY_MC_RT_SRCS_MAX * HY_MC_IP6_SIZE]; /* exclude sources list */
};

struct __mc_encaptbl_entry
{
    struct __mc_group        group;
    __u32                    dev_cnt;
    struct __mc_encaptbl_dev dev[HY_MC_DEV_MAX];
} __attribute__ ((packed));

struct __mc_floodtbl_entry
{
    struct __mc_group       group;
    __u32                   ifcnt;
    __u32                   ifindex[HY_MC_IF_MAX];
} __attribute__ ((packed));

struct __mc_iftbl_node
{
    __u8                     mac[6];
    __u8                     filter_mode;
    __u8                     nsrcs;
    __u8                     srcs[HY_MC_SRCS_MAX * HY_MC_IP6_SIZE];
};
struct __mc_iftbl_entry
{
    struct __mc_group        group;
    __u32                    node_cnt;
    struct __mc_iftbl_node   nodes[HY_MC_IF_NODE_MAX];
};

#ifdef __KERNEL__

struct sk_buff;
extern int (*hy_should_route_hook)(struct sk_buff *skb);

#endif

#endif
