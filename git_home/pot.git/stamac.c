/**
 * Record MAC address of the first Wi-Fi STA that connects to the router after it came out from the factory.
 *
 * That is the starting address of this 6 bytes is (the starting address of the POT section) + 2048 + 4.
 * Basically it works this way:
 * Every time when there is a Wi-Fi STA connected to a router since it boots up, 
 * the router checks whether there is a MAC address stored on the 6 bytes (i.e. whether the first byte of the 6 bytes is 0xff).
 * If the 6 bytes do not record a MAC address information, the router stores the MAC address of this Wi-Fi STA to the 6 bytes.
 * If there is already a MAC address stored on the 6 bytes, then no action is performed.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "config.h"

void usage(char *name)
{
	printf("\nUsage: %s <get | set>\n"
		"  get - display the MAC address of the first Wi-Fi STA that connects to\n"
		"        the router after it came out from the factory to 'stdout'.\n"
		"  set xx:xx:xx:xx:xx:xx - record specified MAC address to POT partition,\n"
		"        if thers is a MAC address existed yet, just display\n"
		"        it without any write action.\n", name);
	exit(0);
}

int main(int argc, char *argv[])
{
	int devfd = 0;
	int sign = 0; /* 0 - get, 1 - set */
	unsigned char mac[6];
	const unsigned char nomac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	int byte0, byte1, byte2, byte3, byte4, byte5;

	daemon(1, 1);

	if ((argc != 2) && (argc != 3)) {
		usage(argv[0]);
	} else {
		if (!strcmp(argv[1], "get")) {
			if (argc != 2)
				usage(argv[0]);
			sign = 0;
		}
		else if (!strcmp(argv[1], "set")) {
			if (argc != 3)
				usage(argv[0]);
			sign = 1;
		}
		else
			usage(argv[0]);
	}

	devfd = open(POT_MTD, O_RDWR | O_SYNC);
	if (0 > devfd) {
		printf("stamac: open mtd POT error!\n");
		return -1;
	}

	lseek(devfd, STAMAC_POSTION, SEEK_SET);
	read(devfd, mac, sizeof(mac));
	if (0 == sign) {
		if (!memcmp(nomac, mac, sizeof(mac)))
			printf("MAC address of 1st STA connected: 00-00-00-00-00-00\n");
		else
			printf("MAC address of 1st STA connected: %02x-%02x-%02x-%02x-%02x-%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	} else {
		if (memcmp(nomac, mac, sizeof(mac))) {
			printf("stamac: one MAC address existed in POT partition, it's %02x-%02x-%02x-%02x-%02x-%02x\n", 
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		} else {
			if (6 != sscanf(argv[2], "%x:%x:%x:%x:%x:%x", &byte0, &byte1, &byte2, &byte3, &byte4, &byte5))
				printf("stamac: the MAC address you specified is wrong, must be same as 00:11:22:33:44:55\n");
			else {
				mac[0] = byte0;
				mac[1] = byte1;
				mac[2] = byte2;
				mac[3] = byte3;
				mac[4] = byte4;
				mac[5] = byte5;
				lseek(devfd, -6, SEEK_CUR);
				write(devfd, mac, sizeof(mac));
				printf("stamac: STA MAC address record finished, it's %02x-%02x-%02x-%02x-%02x-%02x\n",
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			}
		}
	}
	close(devfd);
	return 0;
}
