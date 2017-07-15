#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"

/******************************************************/

struct mtd_info_user
{
	unsigned char	type;
	unsigned int	flags;
	unsigned int	size;			/* Total size of the MTD */
	unsigned int	erasesize;
	unsigned int	oobblock;	/* Size of OOB blocks (e.g. 512) */
	unsigned int	oobsize;		/* Amount of OOB data per block (e.g. 16) */
	unsigned int	ecctype;
	unsigned int	eccsize;
};

struct erase_info_user 
{
	unsigned int	start;
	unsigned int	length;
};

#define MEMGETINFO 	_IOR('M', 1, struct mtd_info_user)
#define MEMERASE		_IOW('M', 2, struct erase_info_user)

/******************************************************/

static void erase_pot_mtd(char *mtd)
{
	int devfd;
	struct mtd_info_user mtdInfo;
	struct erase_info_user mtdEraseInfo;

	devfd = open(mtd, O_RDWR |O_SYNC);
	if (devfd < 0)
		return;

	if (ioctl(devfd, MEMGETINFO, &mtdInfo) == 0) {
		mtdEraseInfo.start = 0;
		mtdEraseInfo.length = mtdInfo.erasesize;	
		ioctl(devfd, MEMERASE, &mtdEraseInfo);
	}

	close(devfd);
}

static void set_potval(char *value)
{
	int devfd;
	int pot_value, len;
	unsigned int word;

	pot_value = atoi(value);
	if (pot_value < 0 || pot_value > POT_MAX_VALUE)
		return;

	/* stop the POT Demo firstly. */
	system("/usr/bin/killall potd 2> /dev/null");

	erase_pot_mtd(POT_MTD);

	/****************************************/
	devfd = open(POT_MTD,O_RDWR);
	if (devfd < 0)
		goto start;

	len = pot_value >> 5;
	word = 0x00000000;
	while (len--)
		write(devfd, &word, sizeof(word));
	len = pot_value & 31;
	word = 0xFFFFFFFF >> len;
	write(devfd, &word, sizeof(word));

	close(devfd);
	/****************************************/

start:
	system("/usr/sbin/potd");
}

time_t get_ntpsynctime(void)
{
	time_t ntp = 0;
	int fd = 0;

	fd = open(POT_MTD, O_RDWR | O_SYNC);
	if (0 > fd) {
		printf("potval: open mtd POT error!\n");
		ntp = 0xffffffff;
	} else {
		lseek(fd, 2048, SEEK_SET);
		read(fd, &ntp, sizeof(ntp));
		close(fd);
	}
	return ntp;
}

void get_stamac(unsigned char* mac)
{
	int fd = 0;

	if (!mac)
		return;

	fd = open(POT_MTD, O_RDWR | O_SYNC);
	if (0 > fd) {
		printf("potval: open mtd POT error!\n");
		memset(mac, 0xff, 6);
	} else {
		lseek(fd, STAMAC_POSTION, SEEK_SET);
		read(fd, mac, 6);
		close(fd);
	}
}

int main(int argc, char **argv)
{
	FILE *fp;
	fd_set readable;
	int r, listen_fd, conn_fd;
	char recvbuf[128], potval[128];
	struct sockaddr_in addr;
	time_t ntptime;
	char strtime[64] = {0};
	unsigned char mac[6];
	char timezone[64];
        char tz_env[64];
	const unsigned char nomac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		printf("error socket");
		return -1;
	}

	fcntl(listen_fd, F_SETFD, 1);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(POT_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
	    listen(listen_fd, 10) < 0) {
		printf("Can't bind the POT socket");
		close(listen_fd);
		return -1;
	}

	printf("The POT-(Get/Set) Demo is Running ...\n");
	daemon(1, 1);

	for (;;) {
		FD_ZERO(&readable);
		FD_SET(listen_fd, &readable);

		if (select(listen_fd + 1, &readable, NULL, NULL, NULL) < 1 ||
		    (conn_fd = accept(listen_fd, NULL, NULL)) < 0)
			continue;

		memset(potval, 0x00, sizeof(potval));
		memset(strtime, 0x00, sizeof(strtime));
		if ((fp = fopen(POT_FILENAME, "r")) == NULL)
			goto cont;
		if (!fgets(potval, sizeof(potval), fp))
			potval[0] = '\0';
		fclose(fp);
		strcat(potval, "NTP");
		ntptime = get_ntpsynctime();
		if (0xffffffff == ntptime) {
			strcpy(strtime, "00-00-00");
		} else {
			memset(timezone, 0, 64);
                        strcpy(timezone, config_get("time_zone"));
                        sprintf(tz_env, "TZ=%s", timezone);
                        putenv(tz_env);
			printf("Current NTP time_zone = %s \n",timezone);
			strftime(strtime, sizeof(strtime), "%T, %b %d, %Y", localtime(&ntptime));
		}
		strcat(potval, strtime);

		get_stamac(mac);
		if(!memcmp(nomac, mac, 6))
			memset(mac, 0, 6);
		sprintf(potval + strlen(potval), "MAC%02x-%02x-%02x-%02x-%02x-%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		send(conn_fd, potval, strlen(potval), 0);
		r = recv(conn_fd, recvbuf, sizeof(recvbuf) - 1, 0);
		if (r < 1)
			goto cont;
		recvbuf[r] = '\0';
		/* printf("POT Recv'd Data : %s\n", recvbuf); */
		if (strcmp(recvbuf, "get"))
			set_potval(recvbuf);
	cont:
		close(conn_fd);
	}

	shutdown(listen_fd, 2);
	close(listen_fd);
	return 0;
}

