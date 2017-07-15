#include "hotplug.h"

extern void mount_timeover_handle(int sig);
int usb1_count = 0;
int usb2_count = 0;
time_t flash_time1 = 0;
time_t flash_time2 = 0;
//*****************************************************************************
//		#Mount a USB flashdisk
//		ACTION == add, PHYSDEVPATH ~~ "/usb[0-9]*/", DEVICENAME ~~ "^sd[a-z][0-9]+$",
//			DEVPATH is set, MAJOR is set, MINOR is set { ... }
//*****************************************************************************
static void add_usbdisk_info(struct uevent_msg *msg)
{
#define INIT_SCSI_NAME "WAIT"
	int namelen;
	char *p, *devname;
	struct list_head *pos;
	struct udisk_info *udisk;
	struct upartition_info  *upart;

	/* No item "PHYSDEVPATH=" passed from kernel, just detect if it's usb event by devpath. */
	if (strstr(msg->devpath, "/usb") == NULL)
		return;

	p = devname = msg->devpath;
	while (*p) {
		if (*p++ == '/')
			devname = p;
	}
	DEBUGP("[UEVNET %s] DEVICENAME = %s\n", msg->seqnum, devname);

	/* Device Name such as : `sda`, `sda1`, `sg0` */
	namelen = strlen(devname);
	if (strncmp(devname, "sg", 2) == 0) {
		list_for_each(pos, &disks_list) {
			udisk = list_entry(pos, struct udisk_info, list);
			if (strcmp(udisk->scsi_name, INIT_SCSI_NAME) == 0) {
				snprintf(udisk->scsi_name, sizeof(udisk->scsi_name), "%s", devname);
				udisk->finished = 1;
#if MOUNT_TIME_CHECK
				alarm(0);
#endif
				break;
			}
		}

		return;
	}

	if (strncmp(devname, "sd", 2))
		return;

	/* It is a new disk ? */
	if (namelen == 3 && 'a' <= devname[2] && devname[2] <= 'z') {
		udisk = malloc(sizeof(struct udisk_info));
		if (udisk == NULL)
			return;

		INIT_LIST_HEAD(&udisk->node);
		udisk->finished = 0;
#if MOUNT_TIME_CHECK
		signal(SIGALRM, mount_timeover_handle);
		alarm(MOUNT_TIME_OVER);
#endif
		snprintf(udisk->disk_name, sizeof(udisk->disk_name), "%s", devname);
		snprintf(udisk->scsi_name, sizeof(udisk->scsi_name), "%s", INIT_SCSI_NAME);

		list_add_tail(&udisk->list, &disks_list);
		return;
	}

	/* It is a new partition ? */
	udisk = NULL;
	list_for_each(pos, &disks_list) {
		struct udisk_info *disk = list_entry(pos, struct udisk_info, list);
		if (strncmp(disk->disk_name, devname, 3) == 0) {
			udisk = disk;
			break;
		}
	}
	if (udisk == NULL)
		return;
	upart = malloc(sizeof(struct upartition_info));
	if (upart == NULL)
		return;
	snprintf(upart->name, sizeof(upart->name), "%s", devname);
	list_add_tail(&upart->list, &udisk->node);

	if (strstr(msg->devpath, "usb1") != NULL) {
		system("echo 1 > /tmp/usb0_attached");
		usb1_count = 0;
	} else if (strstr(msg->devpath, "usb2") != NULL) {
		system("echo 1 > /tmp/usb1_attached");
		usb2_count = 0;
	}

}

void check_host_id(char *buf)
{
	char back[1024];
	char *ptr1 = NULL;
	char *ptr2 = NULL;
	unsigned int len;
	time_t tmp_time = time(NULL);
	
	if (!buf)
		return;

	strcpy(back, buf);
	ptr1 = strstr(back, "usb");
	if (!ptr1)
		return;
		
	ptr2 = strchr(ptr1, '/');
	if (!ptr2)
		return;

	len = ptr2 - ptr1;

	if (tmp_time - flash_time1 >= 10)
                usb1_count = 0;
        else if (tmp_time - flash_time2 >= 10)
                usb2_count = 0;
	
	if (len > 0) {
		if (strncmp("usb1", ptr1, len) == 0 && usb1_count == 0) {
			flash_time1 = time(NULL);
			printf("[USB] usb led 0 on(%d)\n", usb1_count);
			system("usb_led_stop 0 > /dev/null");
	                system("/usr/sbin/usb_led 0");
			usb1_count++;
		} else if (strncmp("usb2", ptr1, len) == 0 && usb2_count == 0) {
			flash_time2 = time(NULL);
			printf("[USB] usb led 1 on(%d)\n", usb2_count);
			system("usb_led_stop 1 > /dev/null");
	                system("/usr/sbin/usb_led 1");
			usb2_count++;
		}
	}
}

void dongle_attached_process(const char *path, const char *intf) 
{
	char cmd[128];
	
	if (!path && !intf) 
		return;
	
	sprintf(cmd, "/etc/usb_modem_hotplug %s on &", intf);
	system(cmd);

	memset(cmd, 0, 128);

	sprintf(cmd, "/sbin/manage_usb_led.sh %s on &", path);
	system(cmd);
}

void parse_uevent_msg(char *buf, int buf_size)
{
	int bufpos;
	struct uevent_msg msg;

	/* ONLY be interesting in 'add' */
	if (strncmp(buf, "add@", 4))
		return;

	memset(&msg, 0x00, sizeof(msg));
	for (bufpos = strlen(buf) + 1; bufpos < buf_size;) {
		char *key;

		key = &buf[bufpos];
		bufpos += strlen(key) + 1;
		DEBUGP("[UEVNET KEY]: '%s'\n", key);

		if (strncmp(key, "ACTION=", 7) == 0)
			msg.action = &key[7];
		else if (strncmp(key, "DEVPATH=", 8) == 0)
			msg.devpath = &key[8];
		else if (strncmp(key, "SUBSYSTEM=", 10) == 0)
			msg.subsystem = &key[10];
		else if (strncmp(key, "SEQNUM=", 7) == 0)
			msg.seqnum = &key[7];
		else if (strncmp(key, "PHYSDEVPATH=", 12) == 0)
			msg.physdevpath = &key[12];
		else if (strncmp(key, "MAJOR=", 6) == 0)
			msg.major = &key[6];
		else if (strncmp(key, "MINOR=", 6) == 0)
			msg.minor = &key[6];
		else if (strncmp(key, "MODALIAS=", 9) == 0)
			msg.modalias = &key[9];
		else if (strncmp(key, "INTERFACE=", 10) == 0)
                        msg.interface = &key[10];
	}

	if (msg.modalias != NULL && msg.interface != NULL) {
		dongle_attached_process(msg.devpath, msg.interface);
	}

	if (msg.seqnum == NULL ||msg.action == NULL ||strcmp(msg.action, "add"))
		return;
	if (msg.devpath == NULL ||msg.subsystem == NULL || msg.major == NULL ||msg.minor == NULL)
		return;
	check_host_id(msg.devpath);
	add_usbdisk_info(&msg);
}

