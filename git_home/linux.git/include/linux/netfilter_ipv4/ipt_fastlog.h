#ifndef _IPT_FASTLOG_H
#define _IPT_FASTLOG_H

#define IPT_MAX_FASTLOG_PREFIX_LEN 94
#define LOG_PORT	0x0001
#define LOG_SPORT	0x0002	/* we only care the source port */

struct ipt_fastlog_info {
	unsigned short flags;

	char prefix[IPT_MAX_FASTLOG_PREFIX_LEN];
};

#endif /*_IPT_FASTLOG_H*/
