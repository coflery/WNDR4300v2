/////////////////////////////////////////////////////////////////////////////
/*
    no-ip.com dynamic IP update client for Linux

   Copyright 2000-2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


			written June 2000
			by johna@onevista.com

	copyright transferred from 
		One Vista Associates 
	to 
		Free Software Foundation, Inc. 
	October 6, 2000 (johna)

	+	November 4, 2000
	+	updated noip.c and Makefile to build on solaris also
	+	collapsed expanded code into strerror()
	+	suggested by rob_nielson@mailandnews.com
	
	+	December 2, 2000
	+	updated noip.c to build on BSD also
	+	changed #include <linux/if.h> to #include <net/if.h>
	+	suggested by ramanan@users.sourceforge.net

	+	April 27, 2001 (Stephane Neveu stephane@skylord.org)
	+	changed the "SAVEDIPFILE" from /tmp/no-ip_save to 
		/var/run/no-ip_save
	+	added configuration default lookup into /usr/local/etc
		if /usr/local/lib doesn't have a configuration file
	+	fix output of contextual ourname[hostnum] in the function
		handle_dynup_error() instead of the "first" name

	+	August 27, 2001 (johna)
	+	added GROUP concept
	+	removed multiple host/domain name hack (use groups)
	+	changed SAVEDIPFILE back to /tmp 
			(must be root to write in /var/run)

	+	November 22, 2002 (johna)
	+	changed vsprintf to vsnprintf to avoid buffer overflow

	+	Version 2.0 December 2002 (johna -- major rewrite)
	+	using shared memory
	+	new config file format with autoconfig (-C)
	+	multiple instances supported (-M)
	+	status available for all instances (-S)
	+	can terminate an instance (-K)
	+	can toggle debugging for an instance (-D)

	+	March 2003	(johna)
	+	bumped MAX_NET_DEVS to 24
	+	drop root privs after acquiring conf (by Michal Ambroz)
	+	added -I interface_name flag (by Clifford Kite)
	
	+	April 2003	(johna)
	+	avoid listing IPV6 devices (robc at gmx.de)
	
	+	May 2003	(johna)
	+	replaced sleep(x) with select(1,0,0,0,timeout)
	+	added getifaddrs() for recent BSD systems (Peter Stromberg)
	+	added new SIOCGIFCONF for older BSD systems (Peter Stromberg)
	
	+	November 2003 (johna)
	+	added <CR> into all http requests along with <LF>
	+	added SIGCHLD handler to reap zombies

	+	January 2004 (johna)
	+	added location logic and revamped XML parsing
	+	added User-Agent field to settings.php request
	+	changed to version 2.1

	+	January 2004 (johna)	version 2.1.1
	+	added -u, -p and -x options for LR101 project

	+	April 2004 (johna	version 2.1.2
	+	removed -Y in make install rule

	+	August 2005 (johna)	version 2.1.3
	+	added shm dump code for debugging broken libraries
	+	added -z flag to invoke shm dump code

	+	February 2006 (johna)	version 2.1.4
	+	added code to handle new pedantic version of gcc
	+	made signed/unsigned char assignments explicit

	+	February 21, 2007 - djonas	version 2.1.5
	+	updated noip2.c: added SkipHeaders() instead of the magic 6 line pass
	+	Changed to ip1.dynupdate.no-ip.com for ip retrieval

	+	August 2007 (johna)	version 2.1.6
	+	added fclose() for stdin, stdout & stderr to child
	+	made Force_Update work on 30 day intervals

	+	August 2007 (johna)	version 2.1.7
	+	fixed bug introduced in 2.1.6 where errors from multiple
	+	instances were not diplayed due to stderr being closed
	+	added version number into shared mem and -S display

	+	December 2007 (johna)	version 2.1.8  (not generally released)
	+	reworked forced update code to use 'wget' and the 
	+	hostactivate.php script at www.no-ip.com
	+	I discovered that no-ip.com still sent warning email
	+	about unused hosts when their address had not changed even 
	+	though they had been updated two days ago by this program!

	+	November 2008 (johna)	still version 2.1.8
	+	added check of returned IP address in get_our_visble_IP_addr
	+ 	hardened GetNextLine to prevent possible buffer overflow
	+	... exploit claimed by J. Random Cracker but never demonstrated
	+	it relied on DNS subversion and buffer overflow

	+	November 2008 (johna)  version 2.1.9
	+	hardened force_update() to prevent possible buffer overflow
	+	hardened autoconf() the same way
	+	patch suggested by xenomuta@phreaker.net

*/			
/////////////////////////////////////////////////////////////////////////////                                            

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <syslog.h>
#include <time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#define DEBUG			0

#define ENCRYPT			1
#define FORCE_UPDATE		0

#define MAX(x,y)		(((x)>(y))?(x):(y))

#define READ_TIMEOUT		90
#define WRITE_TIMEOUT		60
#define CONNECT_TIMEOUT		60
#define FORCE_INTERVAL		(60*60*24) //one day

#define IPLEN			16
#define LINELEN 	        256
#define BIGBUFLEN		16384


#define NOIP_NAME		"dynupdate.no-ip.com"
#define UPD_NAME		"www.no-ip.com"
#define UPD_SCRIPT		"hostactive.php"
#define NOIP_IP_SCRIPT	"ip1.dynupdate.no-ip.com"
#define NOIP_IPCAST_SERVER	"ipcast1.dynupdate.no-ip.com"
#define NOIP_IPCAST_PORT	8253
#define CLIENT_IP_PORT		8245

#define MAX_TRANS_ID	65535
#define TIME_OUT		10

#define VERSION			"2.1.9"
#define USER_AGENT		"User-Agent: Linux-DUC/"VERSION
#define SETTING_SCRIPT		"settings.php?"
#define USTRNG			"username="
#define PWDSTRNG		"&pass="
#if ENCRYPT
  #define REQUEST		"requestL="
#else
  #define REQUEST		""
#endif
  #define UPDATE_SCRIPT		"ducupdate.php"

#ifdef DEBUG
  #define OPTCHARS		"CYU:Fc:dD:g:hp:o:u:x:SMi:K:I:z"
#else
  #define OPTCHARS		"CYU:Fc:g:hp:o:u:x:SMi:K:I:z"
#endif
#define ARGU			(1<<3)
#define ARGI			(1<<4)
#define ARGu			(1<<5)
#define ARGp			(1<<6)
#define ARGo			(1<<7)

#define NODNSGROUP		"@@NO_GROUP@@"
#define HOST			1
#define GROUP			2
#define DOMAIN			3
#define MAX_DEVLEN		16
#define B64MOD			4
#define WGET_PGM		"/usr/bin/wget"

#define SPACE			' '
#define EQUALS			'='
#define COMMENT			'#'
#define COMMA                   ','

#define ALREADYSET               0
#define SUCCESS                  1
#define BADHOST                  2
#define BADPASSWD                3
#define BADUSER                  4
#define BADGRP                  10
#define SUCCESSGRP              11
#define ALREADYSETGRP           12
//#define RELEASEDISABLED         99

#define UNKNOWNERR		-1
#define FATALERR		-1
#define NOHOSTLOOKUP		-2
#define SOCKETFAIL		-3
#define CONNTIMEOUT		-4
#define CONNFAIL		-5
#define READTIMEOUT		-6
#define READFAIL		-7
#define WRITETIMEOUT		-8
#define WRITEFAIL		-9

#define DNI_NOIP_SUPORT 1

#ifdef AES_LONG
typedef unsigned long u32;
#else
typedef unsigned int u32;
#endif
typedef unsigned short u16;
typedef unsigned char u8;

int	timed_out		=	0;
int	port_to_use		=	CLIENT_IP_PORT;
int	socket_fd		=	-1;
int     offset                  =       0;
int	update_cycle		=	0;
char	*ourname		=	NULL;
char	request[LINELEN];
char	*supplied_host_group	=	NULL;
char	*supplied_username	=	NULL;
char	*supplied_password	=	NULL;
char    *user_agent  =  NULL;
char	IPaddress[IPLEN];
char	login[LINELEN];
char	password[LINELEN];
char	device[LINELEN];
char    buffer[BIGBUFLEN];
char    lastIP[IPLEN];

struct	sigaction sig;

typedef unsigned int size_t;

struct DEVICE_INFO {
	char *keyword;
	char *var;
	char *def;
};

typedef struct _DNS_HDR {
	u16 trs_id; //Transaction ID.
	u16 flags; //Flags, set as 0x0100 for query.
	u16 qdcount; //Question count.
	u16 ancount; //Answer RRs.
	u16 nscount; //Authority RRs.
	u16 arcount; //Additional RRs.
} DNS_HDR;

typedef struct _DNS_QER {
	u16 qtype;
	u16 qclass;
} DNS_QER;

typedef struct _DNS_ANS {
	u16 nmoffset;
	u16 antype;
	u16 anclass;
	u32 ttl;
	u16 rdlen;
	u8 addr[4];
} __attribute__ ((packed)) DNS_ANS;


unsigned char DecodeTable[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,  0, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
};

unsigned char EncodeTable[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

///////////////////////////////////////////////////////////////////////////
void	show_ddns_status(int flag,char *host);
void	process_options(int argc, char *argv[]);
void	alarm_handler(int signum);
void	getip(char *dest, char *device);
int	detecte_wan_IPaddr(char *wan_ip, char *device);
int	run_as_background();
int	Sleep(int seconds);
int	Read(int sock, char *buf, size_t count);
int	Write(int fd, char *buf, size_t count);
int	Connect(int port);
int	converse_with_web_server();
int	dynamic_update();
int	GetNextLine(char *dest);
void SkipHeaders();
void	url_encode(char *in, char *out);
void	get_credentials(char *l, char *p);
int	autoconf();
int     bencode(const char *s, char *dst);
int     bdecode(char *in, char *out);
///////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	struct passwd;

	port_to_use = CLIENT_IP_PORT;
        timed_out = 0;
        sig.sa_flags = 0;
        sigemptyset(&sig.sa_mask);
        sig.sa_handler = alarm_handler;
        sigaction(SIGALRM,&sig,NULL);
	*device = 0;
	process_options(argc, argv);
	daemon(1,1);
	run_as_background();	// get shmem failure
	return 0;
}
///////////////////////////////////////////////////////////////////////////
void show_ddns_status(int flag,char host[])
{
	FILE *fp;
	if(!(fp = fopen("/tmp/ez-ipupd.status", "w")))
	{
		printf("can not creat /tmp/ez-ipupd.status\n");
		return;
	}
	fprintf(fp,"%d",flag);
	fclose(fp);
	switch(flag) {
		case 1:
			syslog(LOG_INFO, "[Dynamic DNS] host name %s registeration successful", host);
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			syslog(LOG_INFO, "[Dynamic DNS] host name %s registeration failure,", host);
			break;
		default:
			break;
	}
	if (flag >= 1 || flag <= 5)
		system("date > /tmp/ez-ipupd.time");
}
void process_options(int argc, char *argv[])
{
	extern  int     optind, opterr;
	extern  char    *optarg;
	int     c, have_args = 0;

	while ((c = getopt(argc, argv, OPTCHARS)) != EOF)	{
		switch (c) {
		case 'U':
			update_cycle = atoi(optarg);
			have_args |= ARGU;
			break;
		case 'g':
			user_agent = optarg;
			break;
		case 'o':
			supplied_host_group = optarg;
			have_args |= ARGo;
			break;
		case 'u':
			supplied_username = optarg;
			have_args |= ARGu;
			break;
		case 'p':
			supplied_password = optarg;
			have_args |= ARGp;
			break;
		case 'h':
			exit(0);
			break;
		case 'I':
			strcpy(device, optarg);
			have_args |= ARGI;
			break;
		default:
			exit(0);
		}
	}
	if (have_args & ARGu) {
	    if (!(have_args & ARGp)) {
		exit(1);
	    }
	}
	if (have_args & ARGo) {
		if (!(have_args & ARGo)) {
			exit(1);
		}
	}
	if (have_args & ARGp) {
	    if (!(have_args & ARGu)) {
		exit(1);
	    }
	}
	if (argc - optind) {
	    exit(1);
	}
	return;
}
///////////////////////////////////////////////////////////////////////////
void alarm_handler(int signum)	// entered on SIGALRM
{
	timed_out = 1;
}
///////////////////////////////////////////////////////////////////////////

/**
 * Detecte WAN IP address by DNS method.
 * If success, save ip to wan_ip.
 */
int detecte_wan_IPaddr(char *wan_ip, char *device)
{
	int fd, x;
	u16 tranS_id, tranA_id;
	struct in_addr saddr;
	struct sockaddr_in addr;
	char resName[] = {0x03, 'w', 'a', 'n', 0x02, 'i', 'p', 0x00};
	struct hostent *host;
	char tmp_ip[16];

	// Clear DNS Resolve Cache and Re-Configure DNS Server before gethostbyname().
	res_init();
	getip(tmp_ip,device);
	if (NULL == *tmp_ip)
		return -1;	
	host = gethostbyname(NOIP_IPCAST_SERVER);
	if (NULL == host){
		show_ddns_status(2,tmp_ip);
		return -1;
	}

	memcpy(&saddr.s_addr, host->h_addr_list[0], 4);
	memset((char *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(NOIP_IPCAST_PORT);
	addr.sin_addr.s_addr = saddr.s_addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0){
		show_ddns_status(2,tmp_ip);
		return -1;
	}

	//Encapsulate DNS Query packet.
	srand(time(NULL));
	tranS_id = rand() % MAX_TRANS_ID;
	memset(buffer, 0, BIGBUFLEN);

	DNS_HDR *dnsHdr = (DNS_HDR *)(buffer);
	dnsHdr->trs_id = htons(tranS_id);
	dnsHdr->flags = htons(0x0100);
	dnsHdr->qdcount = htons(1);
	dnsHdr->ancount = 0;
	dnsHdr->nscount = 0;
	dnsHdr->arcount = 0;

	strncpy(buffer + sizeof(DNS_HDR), resName, sizeof(resName));

	DNS_QER *dnsQer = (DNS_QER *)(buffer + sizeof(DNS_HDR) + sizeof(resName));
	dnsQer->qtype = htons(1);
	dnsQer->qclass = htons(1);

	size_t qerLen = sizeof(DNS_HDR) + sizeof(resName) + sizeof(DNS_QER);

	alarm(TIME_OUT);
	x = connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
	alarm(0);

	if (x < 0){
		show_ddns_status(2,tmp_ip);
		goto err;
	}

	if ((x = Write(fd, buffer, qerLen)) <= 0){
		show_ddns_status(5,tmp_ip);
		goto err;
	}

	memset(buffer, 0, BIGBUFLEN);
	if ((x = Read(fd, buffer, BIGBUFLEN - 2)) <= 0){
		show_ddns_status(5,tmp_ip);
		goto err;
	}

	dnsHdr = (DNS_HDR *)(buffer);
	tranA_id = ntohs(dnsHdr->trs_id);

	if (tranA_id != tranS_id){
		show_ddns_status(9,tmp_ip);
		goto err;
	}

	//Check the highest bit, 0 for Query; 1 for Reply.
	if (ntohs(dnsHdr->flags) & 0x8000){
		DNS_ANS *dnsAns = (DNS_ANS *)(buffer + qerLen);
		if (ntohs(dnsHdr->ancount) > 0 && 1 == ntohs(dnsAns->antype))
		{
			sprintf(wan_ip, "%d.%d.%d.%d", dnsAns->addr[0], dnsAns->addr[1],
				dnsAns->addr[2], dnsAns->addr[3]);
			goto ret;
		}
		else{
			show_ddns_status(9,tmp_ip);
			goto err;
		}
	} else{
		show_ddns_status(9,tmp_ip);
		goto err;
	}

ret:
	close(fd);
	return 0;

err:
	close(fd);
	return -1;
}

void getip(char *p, char *device)
{
	int	fd;
	struct  sockaddr_in *sin;
	struct	ifreq ifr;
	struct	in_addr z;

	*p = 0;		// remove old address
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	    return;
	}
	strcpy(ifr.ifr_name, device);
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0)  {
	    close(fd);
	    return;
	}
	if ((ifr.ifr_flags & IFF_UP) == 0)  {
// No longer print message when interface down  (johna 6-28-00)
	    close(fd);
	    return;
	}
	if (ioctl(fd, SIOCGIFADDR, &ifr) != 0) {
	    close(fd);
	    return;
	}
	close(fd);
	sin = (struct sockaddr_in *)&ifr.ifr_addr;
	z = sin->sin_addr;
	strcpy(p, inet_ntoa(z));
}
int set_conf()
{
	char  tbuf[LINELEN];
	memset(lastIP, 0, IPLEN);
	memcpy(lastIP, "0.0.0.0", 7);

        if (!(*device))            // user supplied interface (pppx, etc)
        {
                printf("NO interface!");
                return -1;
        }
        get_credentials(login, password);
	sprintf(buffer, "%s%s%s%s", USTRNG, login, PWDSTRNG, password);
	snprintf(tbuf, LINELEN-1, "&h[]=%s", supplied_host_group);
	strcat(buffer, tbuf);
	bencode(buffer,request);
}

/////////////////////////////////////////////////////////////////////////
int run_as_background()
{
	int	delay = update_cycle,return_error = 0,update_num = 0;
	int	force_update = FORCE_INTERVAL; //Force to update.

	set_conf();
	while (1) {
		if (strcmp("ppp0", device) == 0)
			getip(IPaddress, device);
		else
			detecte_wan_IPaddr(IPaddress, device);

		if (*IPaddress ){
			if(strcmp(IPaddress, lastIP) || force_update <= 0) { 
				return_error = dynamic_update();
				if (return_error == SUCCESS ||return_error == SUCCESSGRP || 
					return_error == ALREADYSET || return_error== ALREADYSETGRP){ 
					strncpy(lastIP, IPaddress, IPLEN);
					show_ddns_status(1,IPaddress);
					delay = update_cycle;
					force_update = FORCE_INTERVAL;
				}else if(return_error == BADPASSWD || return_error == BADUSER){
					show_ddns_status(3,IPaddress);
					exit(1);
				}else if(return_error == BADHOST){
					show_ddns_status(4,IPaddress);
					exit(1);
				}else{
					system("echo 5 > /tmp/ez-ipupd.status");
					update_num++;
					if(update_num == 6)
						show_ddns_status(5,IPaddress);
					if(update_num < 6){
						delay = 30; // wait 30 seconds
					}else if(update_num >= 6){
						update_num = 0;
						delay = 60 * 60;
					}
				}
			}else 
				update_num = 0;
		}else{
			delay = update_cycle;
			show_ddns_status(2,"0.0.0.0");
			force_update = FORCE_INTERVAL;
		}
		if(force_update >= 0)
			force_update -= delay;
		Sleep(delay);
	}
	return SUCCESS;
}
/////////////////////////////////////////////////////////////////////////
int Sleep(int seconds)		// some BSD systems don't interrupt sleep!
{
        struct  timeval timeout;

        timeout.tv_sec = seconds;
        timeout.tv_usec = 0;
        return select(1, 0, 0, 0, &timeout);
 
}
/////////////////////////////////////////////////////////////////////////
int Read(int sock, char *buf, size_t count)
{
	size_t bytes_read = 0;
	int i;
	
	timed_out = 0;
	while (bytes_read < count) {
		alarm(READ_TIMEOUT);
		i = read(sock, buf, (count - bytes_read));
		alarm(0);
		if (timed_out) { 
		    if (bytes_read) {
			syslog(LOG_WARNING,"Short read from %s", NOIP_NAME);
			return bytes_read;
		    } else
			return READTIMEOUT;
		}
		if (i < 0)
			return READFAIL;
		if (i == 0)
			return bytes_read;
		bytes_read += i;
		buf += i;
	}
	return count;
}
///////////////////////////////////////////////////////////////////////////
int Write(int fd, char *buf, size_t count)
{
	size_t bytes_sent = 0;
	int i;

	timed_out = 0;
	while (bytes_sent < count) {
		alarm(WRITE_TIMEOUT);
		i = write(fd, buf, count - bytes_sent);
		alarm(0);
		if (timed_out)
			return WRITETIMEOUT;
		if (i <= 0) 
			return WRITEFAIL;
		bytes_sent += i;
		buf += i;
	}
	return count;
}
///////////////////////////////////////////////////////////////////////////
int Connect(int port)
{
	int	fd, i;
	struct	in_addr saddr;
	struct	sockaddr_in addr;
	struct	hostent *host;

	host = gethostbyname(NOIP_NAME);
	if (!host)
		return NOHOSTLOOKUP;
	memcpy(&saddr.s_addr, host->h_addr_list[0], 4);
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = saddr.s_addr;
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return SOCKETFAIL;
	timed_out = 0;
	alarm(CONNECT_TIMEOUT);
	i = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	alarm(0);
	if (i < 0)  {
	    if (timed_out)
		i = CONNTIMEOUT;
	    else
		i = CONNFAIL;
	    close(fd);		// remove old socket
	    return i;
	}
	socket_fd = fd;
	return SUCCESS;
}
/////////////////////////////////////////////////////////////////////////
int converse_with_web_server()
{
	int	x;
	char *p,*q;
	if ((x = Write(socket_fd, buffer, strlen(buffer))) <= 0) {
	    close(socket_fd);
	    return x;
	}
	if ((x = Read(socket_fd, buffer, BIGBUFLEN - 2)) < 0) {
	    close(socket_fd);
	    return x;
	}
	buffer[x++] = 0;		// terminate the response

/*
 	//debug 
	p = buffer;                         // point at the first line
	while ((q = strchr(p, '\n'))) {
		*q = 0;
		printf("< %s\n", p);   // print the line
		*q++ = '\n';
		p = q;                          // point at next line
	}
	if (*p)
		printf("< %s\n", p);   // display last line
*/

	return x;
}
/////////////////////////////////////////////////////////////////////////
int dynamic_update()
{
	int	i, x, is_group, retval=-1, response;
	char	*p, *pos, tbuf[LINELEN],gname[LINELEN];

	//retval = SUCCESS;
	if ((x = Connect(port_to_use)) != SUCCESS) {
	    return x;
	}    
	// set up text for web page update
	bdecode(request, buffer);
	// IPaddress has already been validated as to length and contents
	sprintf(tbuf, "&ip=%s", IPaddress);
	strcat(buffer, tbuf);
	// use latter half of big buffer
	pos = &buffer[BIGBUFLEN / 2];
	strcpy(pos,buffer);
	bencode(buffer, pos);
	sprintf(buffer, 
	    "GET http://%s/%s?%s%s HTTP/1.0\r\nhost:%s\nUser-Agent:NETGEAR-%s\r\n\r\n",
		NOIP_NAME, UPDATE_SCRIPT, REQUEST, pos, NOIP_NAME, user_agent);
	x = converse_with_web_server();
	if (x < 0){
	    return x;
	}
	close(socket_fd);

	// analyze results
	offset = 0;
	SkipHeaders();
	response = 0;
	while (GetNextLine(tbuf)) {
	    p = ourname = tbuf;	// ourname points at the host/group or error
	    is_group = 1;
	    while (*p && (*p != ':')) {
		if (*p == '.')
		    is_group = 0;
		p++;
	    }
	    if (!*p) { 		// at end of string without finding ':'
		p = "-1";	// unknown error
		ourname = "something";
	    }
	    if (*p == ':') {
		*p++ = 0;		// now p points at return code
		i = atoi(p);
		if (is_group) {
		    snprintf(gname, LINELEN-1, "group[%s]", ourname);
		    ourname = gname;
		}
		response++;
	    }
	    retval = atoi(p);
	}
	//only one host.
	if (response != 1) {
	    retval = UNKNOWNERR;
	    printf("\nUNKNOWNERR!\n");
	}
	return retval;
}
/////////////////////////////////////////////////////////////////////////////
void SkipHeaders()
{
	char *p;

	// global offset into buffer, set here and used only by GetNextLine !
	offset = 0;

	// position after first blank line 
	p = buffer;
	for(; *p; p++) {
		if(strncmp(p, "\r\n\r\n", 4) == 0) {
			offset += 4;
			break;
		}
		offset++;
	}
	return;
}
//////////////////////////////////////////////////////////////////////////
int GetNextLine(char *dest)
{
        char    *p = &buffer[offset];
        char    *q = dest;
	char	*z = &dest[LINELEN-1];

        while (*p && (*p <= ' ')) {     // despace & ignore blank lines
            p++;
            offset++;
        }
        while ((*p) && (q < z)) {
            *q++ = (*p) & 0x7f;		// ASCII charset for text
            offset++;
            if (*p == '\n')  {
                *q = 0;
//fprintf(stderr, "LINE = %s", dest);
                return 1;		// we have a line
            } else
		p++;
        }
	if (q > dest) {			// newline not found
	    if (q == z)
		q--;			// backup for newline and null
	    *q++ = '\n';		// add '\n' 
	    *q = 0;			// and null
	    return 1;			// we have a line
	}
        return 0;			// no line available
}
///////////////////////////////////////////////////////////////////////////

void url_encode(char *p_in, char *p_out)
{
        unsigned char ch, *in, *out;

	in  = (unsigned char *)p_in;
	out = (unsigned char *)p_out;

        while ((ch = *in++)) {
            switch(ch) {
                case ' ': case '"': case '#': case '$': case '%': 
		case '&': case '+': case ',': case '/': case ':': 
		case ';': case '<': case '=': case '>': case '?': 
		case '@':  case '[': case '\\': case ']': case '^':
                case '`': case '{': case '|': case '}': case '~':
			*out++ = '%';
			sprintf((char *)out, "%2.2x", ch);
			out += 2;
			break;
                default: 
		       if ((ch & 0x80) || (ch < 0x20)) {
                           *out++ = '%';
                           sprintf((char *)out, "%2.2x", ch);
                           out += 2;
                       } else
                           *out++ = ch;
                       break;
            }
        }
        *out = 0;
}
/////////////////////////////////////////////////////////////////////////////
void get_credentials(char *l, char *p)
{
	if (supplied_username) {		// have both uname/passwd
	    url_encode(supplied_username, l);
	    url_encode(supplied_password, p);
	    return;
	}
}
/////////////////////////////////////////////////////////////////////////

int bencode(const char *p_s, char *p_dst)
{                         // http basic authorization encoding (base64)
#if ENCRYPT
        int n, n3byt, k, i, nrest, dstlen;
	unsigned char *s, *dst;

	s  	= (unsigned char *)p_s;
	dst	= (unsigned char *)p_dst;
        n  	= strlen(p_s);
        n3byt   = n / 3; 
        k       = n3byt * 3; 
        nrest   = n % 3;
        i       = 0;
        dstlen  = 0;

        while (i < k) {
          dst[dstlen++] = EncodeTable[(( s[i]  & 0xFC)>>2)];
          dst[dstlen++] = EncodeTable[(((s[i]  & 0x03)<<4)|((s[i+1]& 0xF0)>>4))];
          dst[dstlen++] = EncodeTable[(((s[i+1]& 0x0F)<<2)|((s[i+2]& 0xC0)>>6))];
          dst[dstlen++] = EncodeTable[(  s[i+2]& 0x3F)];
          i += 3;
        }
        if (nrest == 2) {
            dst[dstlen++] = EncodeTable[(( s[k]& 0xFC) >>2)];
            dst[dstlen++] = EncodeTable[(((s[k]& 0x03)<<4)|((s[k+1]& 0xF0)>>4))];
            dst[dstlen++] = EncodeTable[(( s[k+1] & 0x0F) <<2)]; 
        } else {
            if (nrest==1) {
                dst[dstlen++] = EncodeTable[((s[k] & 0xFC) >>2)];
                dst[dstlen++] = EncodeTable[((s[k] & 0x03) <<4)];
            }
        }
	// pad to multiple of 4 per RFC 1341
        while (dstlen % B64MOD)
             dst[dstlen++] = '=';
        dst[dstlen] = 0;
	return dstlen;
#else
        strcpy(p_dst, p_s);
        return strlen(p_s);

#endif
}

////////////////////////////////////////////////////////////////////////////

int bdecode(char *p_in, char *p_out)
{
#if ENCRYPT
	unsigned char *in, *out;
        unsigned char *p, *q, d1, d2, d3, d4;
        int x;

	in  = (unsigned char *)p_in;
	out = (unsigned char *)p_out;
        x = strlen(p_in);

        p = q = &in[x];
        while (x % B64MOD) {      // pad to a multiple of four (if malformed)
           *p++ = '=';
           x++;
        }
	*p = 0;
        do {
            d1 = DecodeTable[(*in++ & 0x7f)];
            d2 = DecodeTable[(*in++ & 0x7f)];
            d3 = DecodeTable[(*in++ & 0x7f)];
            d4 = DecodeTable[(*in++ & 0x7f)];
            if ((d1 | d2 | d3 | d4) & 0x80) {   // error exit 
		*q = 0;				// replace original null
                return -1;
	    }
            *out++ =  (d1 << 2)         | (d2 >> 4);
            *out++ = ((d2 << 4) & 0xF0) | (d3 >> 2);
            *out++ = ((d3 << 6) & 0xC0) |  d4;
            x -= B64MOD;
        } while (x > 0);
        *out = 0;
	*q = 0;					// replace original null
#else
	strcpy(p_out, p_in);
#endif
	return 0;
}

