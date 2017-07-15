#ifndef __HOTPLUG_H
#define __HOTPLUG_H
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include "list.h"

#if 0
#define DEBUGP(format, args...) printf(format, ## args)
#define DEBUGC(string, len)	\
	do {	\
		int __i;	\
		for (__i = 0; __i < len; __i++)	\
			printf("%c", string[__i] == '\0' ? ' ' : string[__i]);	\
		printf("\n");	\
	} while (0)
#else
#define DEBUGP(format, args...)
#define DEBUGC(format, args...)
#endif

#define UEVENT_BUFFER_SIZE		2048

#define MOUNT_TIME_CHECK	1
#define MOUNT_TIME_OVER		15

struct udisk_info
{
	struct list_head list;
	struct list_head node;

	int finished;		/* All partitions of this disk are ready? */
	char disk_name[4];	/* sda */
	char scsi_name[8];	/* sg0 */
};

struct upartition_info
{
	struct list_head list;

	char name[8]; /* 'sda1' */
};

struct uevent_msg
{
	char *action;
	char *devpath;
	char *subsystem;
	char *physdevpath;
	char *seqnum;
	char *major;
	char *minor;
	char *modalias;
	char *interface;
};

extern struct list_head disks_list;
extern void parse_uevent_msg(char *buf, int buf_size);
extern int usb1_count;
extern int usb2_count;
#endif

