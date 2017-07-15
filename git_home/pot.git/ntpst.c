/**
 * Record first NTP Sync Timestamp.
 *
 * It's a track record of the date/time on POT partition fot the first time
 * when the router power on and get time from NTP server.
 *
 * The first 4 bytes after the 2KBytes(2048 bytes) of POT section stores the
 * timestamp in the number of seconds since year 1970 in the GMT time.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>

#include "config.h"

#define NTPST_POSTION	2048

void usage(char *name)
{
	printf("\nUsage: %s <get | set>\n"
		"  get - display the contents of the NTP Sync Timestamp to 'stdout'.\n"
		"  set - record now time to POT partition as a NTP Sync Timestamp,\n"
		"        if thers is a NTP Sync Timestamp existed yet, just display\n"
		"        it without any write action.\n", name);
	exit(0);
}

int main(int argc, char *argv[])
{
	int devfd = 0;
	time_t ntptime = 0;
	int sign = 0; /* 0 - get, 1 - set */
	char str[128] = {0};
	char timezone[64];
        char tz_env[64];

	daemon(1, 1);

	if (2 != argc) {
		usage(argv[0]);
	} else {
		if (!strcmp(argv[1], "get"))
			sign = 0;
		else if (!strcmp(argv[1], "set"))
			sign = 1;
		else
			usage(argv[0]);
	}

	devfd = open(POT_MTD, O_RDWR | O_SYNC);
	if (0 > devfd) {
		printf("NTPST: open mtd POT error!\n");
		return -1;
	}

	lseek(devfd, NTPST_POSTION, SEEK_SET);
	read(devfd, &ntptime, sizeof(ntptime));
	if (0 == sign) {
		if (0xFFFFFFFF == ntptime) {
			printf("NTP synchronized date/time: 00-00-00\n");
		} else {
			memset(timezone, 0, 64);
                        strcpy(timezone, config_get("time_zone"));
                        sprintf(tz_env, "TZ=%s", timezone);
                        putenv(tz_env);

			strftime(str, sizeof(str), "%T, %b %d, %Y",localtime(&ntptime));
			printf("NTP synchronized date/time: %s\n", str);
		}
	} else {
		if (0xFFFFFFFF != ntptime) {
			strftime(str, sizeof(str), "%T, %b %d, %Y", localtime(&ntptime));
			printf("NTPST: one NTP Sync Timestamp existed in POT partition, it's %s\n", str);
		} else {
			ntptime = time(NULL);
			lseek(devfd, -4, SEEK_CUR);
			write(devfd, &ntptime, sizeof(ntptime));
			strftime(str, sizeof(str), "%T, %b %d, %Y", localtime(&ntptime));
			printf("NTPST: NTP Sync Timestamp record finished, it's %s\n", str);
		}
	}
	close(devfd);
	return 0;
}
