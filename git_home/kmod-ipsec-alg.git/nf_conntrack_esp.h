#ifndef _NF_CONNTRACK_ESP_H
#define _NF_CONNTRACK_ESP_H

#define ESP_REF_TMOUT   (30 * HZ)
#define ESP_CONN_TMOUT  (60 * HZ * 6)
#define ESP_TMOUT_COUNT (ESP_CONN_TMOUT/ESP_REF_TMOUT)

#define IPSEC_FREE     0
#define IPSEC_INUSE    1
#define MAX_PORTS      16
#define TEMP_SPI_START 1500

/*
union nf_conntrack_man_proto
{
	__be16 all;

	struct {
		__be16 spi;
	} esp;
};

struct nf_conntrack_tuple
{
	struct nf_conntrack_man src;

	struct {
		union nf_inet_addr u3;
		union {
			__be16 all;

			struct {
				__be16 spi;
			} esp;
		} u;

		u_int8_t protonum;

		u_int8_t dir;
	} dst;
};

we should modify nf_conntrack_tuple & nf_conntrack_man_proto as above.
but we haven't, so define below */
#define esp_spi all

struct esphdr {
	u_int32_t spi;
	u_int32_t seq;
};

struct _esp_table {
        u_int32_t l_spi;
        u_int32_t r_spi;
        u_int32_t l_ip;
        u_int32_t r_ip;
        u_int32_t timeout;
        u_int16_t tspi;
        struct ip_conntrack *ct;
        struct timer_list   refresh_timer;
        int                 timer_active;
        int                 pkt_rcvd;
        int                 ntimeouts;
        int       inuse;
};

#endif /* _NF_CONNTRACK_ESP_H */
