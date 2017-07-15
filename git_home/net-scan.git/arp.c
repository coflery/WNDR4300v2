#include "netscan.h"

#define NEIGH_HASHMASK	0x1F

struct arp_struct
{
	struct arp_struct *next;

	struct in_addr ip;

	uint16 active;
	uint8 mac[ETH_ALEN];

	char host[MAX_HOSTNAME_LEN + 1];
};

struct arp_struct *arp_tbl[NEIGH_HASHMASK + 1];

static struct arpmsg arpreq;

int init_arp_request(char *ifname)
{
	int s;
	struct ifreq ifr;
	struct arpmsg *arp;
	
	s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (s < 0)
		return 0;
	
	arp = &arpreq;
	memset(arp, 0, sizeof(struct arpmsg));

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(s, SIOCGIFADDR, &ifr) != 0)
		return 0;
	memcpy(arp->ar_sip, &((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr, 4);
	
	if (ioctl(s, SIOCGIFHWADDR, &ifr) != 0)
		return 0;
	memset(arp->h_dest, 0xFF, 6);
	memcpy(arp->h_source, ifr.ifr_hwaddr.sa_data, 6);
	arp->h_proto = htons(ETH_P_ARP);
	arp->ar_hrd = htons(ARPHRD_ETHER);
	arp->ar_pro = htons(ETH_P_IP);
	arp->ar_hln = 6;
	arp->ar_pln = 4;
	arp->ar_op = htons(ARPOP_REQUEST);
	memcpy(arp->ar_sha, ifr.ifr_hwaddr.sa_data, 6);
	
	close(s);
	return 1;
}

/* modified from "linux-2.4.18/net/ipv4/arp.c" */
static uint32 arp_hash(uint8 *pkey)
{
#define GET_UINT32(p)	((p[0]) |(p[1] << 8) |(p[2] << 16) |(p[3] << 24))
	uint32 hash_val;

	hash_val = GET_UINT32(pkey);
	hash_val ^= hash_val >> 16;
	hash_val ^= hash_val >> 8;
	hash_val ^= hash_val >> 3;

	return hash_val & NEIGH_HASHMASK;
}

static void get_dhcp_host(char host[], struct in_addr ip, int *isrepl)
{
	FILE *tfp;
	char *ipaddr;
	char *hostname;
	char *ipstr;
	char buff[512];

	host[0] = '\0';
	ipstr = inet_ntoa(ip);
	if ((tfp = fopen(DHCP_LIST_FILE,"r")) == 0)
		return;

	while (fgets(buff, sizeof(buff), tfp)) {
		ipaddr = strtok(buff, " \t\n");
		hostname = strtok(NULL, " \t\n");
		if (ipaddr == NULL || hostname == NULL)
			continue;

		if (strcmp(ipaddr, ipstr) == 0) {
			strncpy(host, hostname, MAX_HOSTNAME_LEN);
			*isrepl = 0;
			break;
		}
	}

	fclose(tfp);
}

char *ether_etoa(uint8 *e, char *a);
void acl_update_name(uint8 *mac, char *name)
{
	char dev_mac[32];

	dni_system(NULL, "/usr/sbin/acl_update_name", ether_etoa(mac, dev_mac), name, NULL);
}

int update_arp_table(uint8 *mac, struct in_addr ip, int isrepl)
{
	uint32 i;
	char host[MAX_HOSTNAME_LEN + 1] = {0};
	struct arp_struct *u;
	
	/* check dhcp host */
	get_dhcp_host(host, ip, &isrepl);
	i = arp_hash(mac);
	/* for fix the bug-29548 */
	//for (u = arp_tbl[i]; u && memcmp(u->mac, mac, ETH_ALEN); u = u->next);
	for (u = arp_tbl[i]; u && (u->ip).s_addr != ip.s_addr; u = u->next);
	if (u) {
		if (*host) {
			strncpy(u->host, host, MAX_HOSTNAME_LEN);
			acl_update_name(u->mac, host);
		}
		u->ip = ip;              /* The IP may be changed for DHCP      */
		u->active = 1;
		return isrepl;	/* Do BIOS Name Query only ARP reply */
	}

	u = malloc(sizeof(struct arp_struct));
	if (u == 0)
		return 0;
	u->ip = ip;
	u->active = 1;
	if (*host) {
		strncpy(u->host, host, MAX_HOSTNAME_LEN);
		acl_update_name(u->mac, host);
	}
	memcpy(u->mac, mac, ETH_ALEN);
	u->next = arp_tbl[i];
	arp_tbl[i] = u;

	return isrepl;
}

static void update_name(struct in_addr ip, char *host)
{
	int i;
	struct arp_struct *u;

	for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
		for (u = arp_tbl[i]; u; u = u->next)
			if (u->ip.s_addr == ip.s_addr) {
				strncpy(u->host, host, MAX_HOSTNAME_LEN);
				acl_update_name(u->mac, host);
				return;
			}
	}
}

void update_bios_name(uint8 *mac, char *host, struct in_addr ip)
{
	uint32 i;
	struct arp_struct *u;
	
	i = arp_hash(mac);
	for (u = arp_tbl[i]; u && memcmp(u->mac, mac, ETH_ALEN); u = u->next);

	if (u == 0) {
		update_name(ip, host); /* try it by IP address */
		return;
	}
	
	strncpy(u->host, host, MAX_HOSTNAME_LEN);
	acl_update_name(u->mac, host);
}

void recv_bios_pack(char *buf, int len, struct in_addr from)
{
#define HDR_SIZE		sizeof(struct nb_response_header)
	uint16 num;
	uint8 *p, *e;
	struct nb_response_header *resp;

	if (len < HDR_SIZE)
		return;
	
	resp = (struct nb_response_header *)buf;
	num = resp->num_names;
	p = (uint8*)&buf[HDR_SIZE];
	e = p + (num * 18);
	/* unique name, workstation service - this is computer name */
	for (; p < e; p += 18)
		if (p[15] == 0 && (p[16] & 0x80) == 0)
			break;
	if (p == e)
		return;
	update_bios_name(e, (char *)p, from);
}

char *ether_etoa(uint8 *e, char *a)
{
	static char hexbuf[] = "0123456789ABCDEF";
	
	int i, k;

	for (k = 0, i = 0; i < 6; i++) {
		a[k++] = hexbuf[(e[i] >> 4) & 0xF];
		a[k++] = hexbuf[(e[i]) & 0xF];
		a[k++]=':';
	}
	
	a[--k] = 0;
	
	return a;
}

/*
 * xss Protection 
 * < -> &lt;
 * > -> &gt;
 * ( -> &#40;
 * ) -> &#41;
 * " -> &#34;
 * ' -> &#39;
 * # -> &#35;
 * & -> &#38;
 */
char *host_stod(char *s)
{//change special character to ordinary string
	static char str[MAX_HOSTNAME_LEN*5 + 1 ];
	char c, *p;

	p = str;
        while((c = *s++) != '\0') {
                if(c == '"'){
                        *p++ = '&'; *p++ = '#'; *p++ = '3'; *p++ = '4'; *p++ = ';';
                } else if( c == '(' ){
                        *p++ = '&'; *p++ = '#'; *p++ = '4'; *p++ = '0'; *p++ = ';';
                } else if( c == ')' ){
                        *p++ = '&'; *p++ = '#'; *p++ = '4'; *p++ = '1'; *p++ = ';';
                } else if( c == '#' ){
                        *p++ = '&'; *p++ = '#'; *p++ = '3'; *p++ = '5'; *p++ = ';';
                } else if( c == '&' ){
                        *p++ = '&'; *p++ = '#'; *p++ = '3'; *p++ = '8'; *p++ = ';';
                } else if( c == '<' ){
                        *p++ = '&'; *p++ = 'l'; *p++ = 't'; *p++ = ';';
                } else if( c == '>' ){
                        *p++ = '&'; *p++ = 'g'; *p++ = 't'; *p++ = ';';
                } else if (c == '\'') {
                        *p++ = '&'; *p++ = '#'; *p++ = '3'; *p++ = '9'; *p++ = ';';
                }
                else {
                        *p++ = c;
                }
        }
        *p = '\0';

	return str;	
}

int open_arp_socket(struct sockaddr *me)
{
	int s;
	int buffersize = 200 * 1024;
	
	s = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP));
	if (s < 0)
		return -1;

	/* We're trying to override buffer size  to set a bigger buffer. */
	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize)))
		fprintf(stderr, "setsocketopt error!\n");

	me->sa_family = PF_PACKET;
	strncpy(me->sa_data, ARP_IFNAME, 14);
	if (bind(s, me, sizeof(*me)) < 0)
		return -1;
	if (init_arp_request(ARP_IFNAME) == 0)
		return -1;
	
	return s;
}

int recv_arp_pack(struct arpmsg *arpkt, struct in_addr *send_ip)
{
	static uint8 zero[6] = { 0, 0, 0, 0, 0, 0 };
	
	struct in_addr src_ip;

	if (arpkt->ar_op != htons(ARPOP_REQUEST) && arpkt->ar_op != htons(ARPOP_REPLY))
		return 0;
	if (arpkt->ar_hrd != htons(ARPHRD_ETHER) ||arpkt->ar_pro != htons(ETH_P_IP))
		return 0;
	if (arpkt->ar_pln != 4 ||arpkt->ar_hln != ETH_ALEN)
		return 0;

	/*
	  * If It is Gratuitous ARP message, ignore it for Home Router passing Xbox test,
	  * else we need change the `udhcpd` code about `checking IP used` too much
	  * to pass `XBox DHCP Lease Test`. The normal ARP message ==MAY BE== all
	  * right for Attached Devices function.... &_&.
	  */
	if (memcmp(arpkt->ar_sip, arpkt->ar_tip, 4) == 0)
		return 0;

	memcpy(&src_ip, arpkt->ar_sip, 4);
	if (src_ip.s_addr == 0 ||memcmp(arpkt->ar_sha, zero, 6) == 0)
		return 0;

	*send_ip = src_ip;
	return update_arp_table(arpkt->ar_sha, src_ip, arpkt->ar_op == htons(ARPOP_REPLY));
}

void remove_disconn_dhcp(struct in_addr ip)
{
	int i, k, result;
	int target = 0;
	int target_num = 0;
	FILE *fp;
	fpos_t pos_w,pos_r,pos;
	char ipaddr[32];
	char line[512];
	char list_str[512];

	if ( !(fp = fopen (DHCP_LIST_FILE,"r")))
		return;
	
	while(fgets(line, sizeof(line), fp) != NULL) {
		result = sscanf(line, "%s%s", ipaddr,list_str);
		if (result == 2){
			if(memcmp(inet_ntoa(ip), ipaddr, strlen(ipaddr)) == 0) {
				target = 1;
				break;
			}
		}
		target_num ++;
	}
	fclose(fp);

	if (target != 1)
		return;

	if ( !(fp = fopen (DHCP_LIST_FILE,"r+")))
		return;
	for (i = 0; i < target_num; i++)
		fgets(line,sizeof(line),fp);
	
	/* save the file pointer position */
	fgetpos (fp,&pos_w);
	/* position the delete line */
	fgets(line,sizeof(line),fp);
	fgetpos (fp,&pos_r);
	pos = pos_r;

	while (1)
	{
		/* set a new file position */ 
		fsetpos (fp,&pos);
		if (fgets(line,sizeof(line),fp) ==NULL) 
			break;
		fgetpos (fp,&pos_r);
		pos = pos_w;
		fsetpos (fp,&pos);
		fprintf(fp,"%s",line);
		fgetpos (fp,&pos_w);
		pos = pos_r;
	}
	pos = pos_w;
	fsetpos (fp,&pos);
	k = strlen(line);
	for (i=0;i<k;i++) 
		fputc(0x20,fp);
	
	fclose(fp);
}

		
void strupr(char *str)
{
	for(;*str != '\0'; str++)
	{
		if(*str >= 97 && *str <= 122)
			*str = (*str)-32;
	}
}


void show_arp_table(void)
{
	int i, j, fd_flag;
	FILE *fp, *fw;
	char mac[32];
	struct arp_struct *u;
	struct arp_struct **pprev;
	struct in_addr dhcp_host[256];
	char buffer[512];
	char *ipaddr;

	fp = fopen(ARP_FILE, "w");
	if (fp == 0) 
		return;
	
	for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
		for (pprev = &arp_tbl[i], u = *pprev; u; ) {
			if (u->active == 0) {
				remove_disconn_dhcp(u->ip);
				*pprev = u->next;
				free(u);
				u = *pprev;
				continue;
			}

			/* for GUI dealing easily:  &lt;unknown&gt;   <----> <unknown>*/
			fprintf(fp, "%s %s %s @#$&*!\n",
				inet_ntoa(u->ip), ether_etoa(u->mac, mac),
				u->host[0] == '\0' ? "&lt;unknown&gt;" : host_stod(u->host));
			
			pprev = &u->next;
			u = *pprev;
		}
	}


	if (fw = fopen(WLAN_STA_FILE, "r")) {
		while (fgets(buffer, sizeof(buffer), fw)) {
			fd_flag = 0;
			for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
				for (pprev = &arp_tbl[i], u = *pprev; u; ) {
					ether_etoa(u->mac, mac);
					strupr(buffer);
					if(!strncmp(mac, buffer, strlen(mac))) {
						fd_flag = 1;
						break;
					}
					pprev = &u->next;
					u = *pprev;
				}
			}
			if(!fd_flag) {
				strncpy(mac, buffer, 17);
				mac[17]='\0';
				strupr(mac);
				fprintf(fp, "%s %s %s @#$&*!\n",
					"&lt;unknown&gt", mac , "&lt;unknown&gt;");
			}
		}
		fclose(fw);
	}	

	fclose(fp);
	
	/* for fix bug 31698,remove hosts which can't be found in the arp_tbl[] from dhcpd_hostlist*/
	j = 0;
	if (fp = fopen(DHCP_LIST_FILE,"r")) {
		while (fgets(buffer, sizeof(buffer), fp)) {
			ipaddr = strtok(buffer, " \t\n");
			if (ipaddr && inet_aton(ipaddr, &dhcp_host[j]) != 0)
				j++;
		}
		fclose(fp);
	}

	for(j--;j >= 0; j--) {
		for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
			for (u = arp_tbl[i]; u && memcmp(&u->ip, &dhcp_host[j], sizeof(&u->ip)); u = u->next);
			if (u) break;
		}
		if (!u) remove_disconn_dhcp(dhcp_host[j]);
	}
}

/* To fix bug 22146, add function reset_arp_table, it can set active status of all nodes in the arp_tbl to 0 */
void reset_arp_table()
{
	int i;
	struct arp_struct *u;
	
	for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
		for (u = arp_tbl[i]; u; u = u->next) {
			u->active = 0;
		}
	}
}

void scan_arp_table(int sock, struct sockaddr *me)
{
	int i;
	int count = 0;
	struct itimerval tv;
	struct arpmsg *req;
	struct arp_struct *u;
	char *ipaddr;
	char buffer[512];
	struct in_addr addr;
	FILE *fp;
	
	while (count != 3) {
		count++;
		req = &arpreq;
		for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
			for (u = arp_tbl[i]; u; u = u->next) {
				memcpy(req->ar_tip, &u->ip, 4);
				sendto(sock, req, sizeof(struct arpmsg), 0, me, sizeof(struct sockaddr));
			}
		}
		/**
		 * For beta issue: TD-23
		 * If use Ixia with some virtual DHCP clients to test "Attached Device" feature,
		 * Ixia could not send arp packet actively, we need request all IPs that DHCP server
		 * assigned while user refresh "Attached Device" table.
		 * We just request all IPs in "/tmp/dhcpd_hostlist" that were not recorded in 'arp_tbl'.
		 */
		if (fp = fopen(DHCP_LIST_FILE,"r")) {
			while (fgets(buffer, sizeof(buffer), fp)) {
				ipaddr = strtok(buffer, " \t\n");
				if (ipaddr && inet_aton(ipaddr, &addr) != 0) {
					for (i = 0; i < (NEIGH_HASHMASK + 1); i++) {
						for (u = arp_tbl[i]; u && memcmp(&u->ip, &addr, sizeof(addr)); u = u->next);
						if (u) break;
					}
					if (u) continue;
					memcpy(req->ar_tip, &addr, 4);
					sendto(sock, req, sizeof(struct arpmsg), 0, me, sizeof(struct sockaddr));
				}
			}
			fclose(fp);
		}
		if(count < 3)
			usleep(500000);
	}
	
	/* show the result after 3s */
	tv.it_value.tv_sec = 3;
	tv.it_value.tv_usec = 0;
	tv.it_interval.tv_sec = 0;
	tv.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &tv, 0);
}

