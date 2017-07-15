 /*
 *  Copyright (C) 2013, Delta Networks, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statfs.h>
#include "list.h"

int scan_disk_entries()
{
	FILE *fp;
	struct statfs statbuf;
	int major,minors;
	int have_disk_mouted = 0;
	unsigned long long capacity;
	char mnt_path[32];
	char *s, part_name[128], line[256];

	fp = fopen("/proc/partitions","r");
	if (fp == NULL )
		return 0;

	/*
	  * major minor  #blocks  name
	  *
	  *  31     0        320 mtdblock0
	  * ....
	  *   8     0    3968000 sda
	  *   8     1    3963968 sda1
	  */
	while (fgets(line,sizeof(line),fp)) {
		if (sscanf(line, " %d %d %llu %[^\n ]",&major, &minors, &capacity, part_name) != 4)
			continue;
		if (strncmp(part_name, "sd", 2))
			continue;
		for (s = part_name; *s; s++);
		if (s[-1] >= 'a' && s[-1] <= 'z' )
			continue;
		capacity >>= 10;        /* unit: 1KB .. >> 1   size /512 (long *arg) */
		if (capacity == 0)
			continue; /*It indicates that this partition should be an extended partition. */
		/* SEE: hotplug2.mount ==> mount /dev/$1 /mnt/$1 */
		snprintf(mnt_path, sizeof(mnt_path), "/mnt/%s", part_name);
		/* NO Disk, the mount point directory is NOT removed, this magic value is `0x858458F6` */
		if (statfs(mnt_path, &statbuf) == 0 && (unsigned int)statbuf.f_type != 0x858458F6)
			have_disk_mouted = 1;
	}		
	fclose(fp);
	return have_disk_mouted;
}

#define USB_LED_STATE   "/proc/usbled/state"

int main(int argc, char **argv)
{
	struct timeval timo;
	FILE *fp;
	char state[8];
	int i=0,disk_mouted = 0;
	int hostId;
	
	if (!argv[1])
		return;

	if (strncmp(argv[1], "0", 1) == 0)
		hostId = 0;
	else if (strncmp(argv[1], "1", 1) == 0)
		hostId = 1;
	else if (strncmp(argv[1], "2", 1) == 0)
		hostId = 2;
	else if (strncmp(argv[1], "3", 1) == 0)
		hostId = 3;

	daemon(1, 1);
	disk_mouted = scan_disk_entries();
	printf("\ndisk_mouted=%d, host_id=%d\n",disk_mouted, hostId);
#if 0
	fp=fopen(USB_LED_STATE,"r");
	if(fp != NULL){
		fgets(state,sizeof(state),fp);
		fclose(fp);
	}
	else {
		printf("Can not open file:%s\n",USB_LED_STATE);
		return -1;
	}
#endif
	/* argv[1]: 0 turn off usbled,1 turn on usbled.
	 * state[0]: 0 usb led is off,1 usb led is on.
	 * NETGEAR spec. V11 (120312), on page 253. It define the USB LED will turn on/turn off
	 * every 0.5 second till 5 seconds, each time there is a USB device connected.
	 */
	 if(hostId == 0 || hostId == 1){
		/* 1.when a usb storage is connected,we should turn on the usb led.
		 * 2.if the usb led is already on,it should not blink.
		 */
		 if (hostId == 0)
		 	strncpy(state, config_get("usb_led0"), 2);
		 else if (hostId == 1)
		 	strncpy(state, config_get("usb_led1"), 2);
			
		 if(strncmp(state, "on", 2)){
		 	i = 0;
			while (i++ < 50) {
				timo.tv_sec = 0;
				timo.tv_usec = 500000;
				if(i%2) {
					if (hostId == 0)
						system("/sbin/ledcontrol -n usb0 -c green -s on");
					else if (hostId == 1)
						system("/sbin/ledcontrol -n usb1 -c green -s on");
				}
				else {
					if (hostId == 0)
                                                system("/sbin/ledcontrol -n usb0 -c green -s off");
                                        else if (hostId == 1)
                                                system("/sbin/ledcontrol -n usb1 -c green -s off");
				}
				select(1, NULL, NULL, NULL, &timo);
			}
			if (hostId == 0)
                                system("/sbin/ledcontrol -n usb0 -c green -s off");
                        else if (hostId == 1)
                                system("/sbin/ledcontrol -n usb1 -c green -s off");
		}
		/* To avoid USB LED be turned off by accident,when we turn on the led,
		 * run a process to set "/proc/usbled/state" periodically
		 */
	} else if ((hostId == 2 || hostId == 3) && disk_mouted) {
		if (hostId == 2) {
	                 system("config set usb_led0=\"on\"");
                }
                else if (hostId == 3) {
                         system("config set usb_led1=\"on\"");
                }
		for (;;) {
			timo.tv_sec = 1;
			timo.tv_usec = 0;
			if (hostId == 2) {
	                        system("/sbin/ledcontrol -n usb0 -c green -s on");
			}
                        else if (hostId == 3) {
                                system("/sbin/ledcontrol -n usb1 -c green -s on");
			}
			select(1, NULL, NULL, NULL, &timo);
		}
	}

	return 0;

}
