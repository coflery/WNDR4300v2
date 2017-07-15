/* udev-106 hotplug2-0.9
  *
  * add@/block/sdc					DEVPATH=/block/sdc
  * add@/block/sdc/sdc1			DEVPATH=/block/sdc/sdc1
  * add@/block/sdc/sdc2			DEVPATH=/block/sdc/sdc2
  * add@/class/scsi_generic/sg2		DEVPATH=/class/scsi_generic/sg2
  *
  * 7052 ==================== sda1 ====================
  * 7054 ==================== sdb3 ====================
  * 7055 ==================== sdb5 ====================
  * 7056 ==================== sdb6 ====================
  * 7057 ==================== sdb7 ====================
  * 7058 ==================== sdb8 ====================
  * 7060 ==================== sdc1 ====================
  * 7061 ==================== sdc2 ====================
  * 7106 ==================== sg0 ====================
  * 7107 ==================== sg1 ====================
  * 7108 ==================== sg2 ====================
  *
  */

#include "hotplug.h"

struct list_head disks_list = LIST_HEAD_INIT(disks_list);

static void sigchld(int sig)
{
	pid_t pid;
	int status;

	for (;;) {
		pid = waitpid(-1, &status, WNOHANG);
		if (pid <= 0)
			break;
		DEBUGP("NET-DISK: Reaping child %d: status %d\n", pid, status);
	}
}

static int init_netlink_socket(void)
{
	int netlink_socket;
	struct sockaddr_nl snl;
	int buffersize = 2 * 1024 * 1024;

	memset(&snl, 0x00, sizeof(struct sockaddr_nl));
	snl.nl_family = AF_NETLINK;
	snl.nl_pid = getpid();
	snl.nl_groups = 1;
	netlink_socket = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT); 
	if (netlink_socket == -1) {
		DEBUGP("Failed socket: %s.\n", strerror(errno));
		return -1;
	}

	/* We're trying to override buffer size. If we fail, we attempt to set a big buffer and pray. */
	if (setsockopt(netlink_socket, SOL_SOCKET, SO_RCVBUFFORCE, &buffersize, sizeof(buffersize))) {
		DEBUGP("Failed setsockopt SO_RCVBUFFORCE: %s. (non-critical)\n", strerror(errno));

		/* Somewhat safe default. */
		buffersize = 106496;
		setsockopt(netlink_socket, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize));
	}

	if (bind(netlink_socket, (struct sockaddr *) &snl, sizeof(struct sockaddr_nl))) {
		DEBUGP("Failed bind: %s.\n", strerror(errno));
		close(netlink_socket);
		return -1;
	}

	return netlink_socket;
}

static void run_action(struct udisk_info *udisk)
{
	char cmd[256], tmp[8];
	struct list_head *pos;
	struct upartition_info *upart;
	int usb0_attached = 0, usb1_attached = 0;
	FILE *fp = NULL;
	
	printf("Run Action !\n");
	fp = fopen("/tmp/usb0_attached", "r");
	if (fp != NULL) {
		fgets(tmp, 8, fp);
		if (strncmp(tmp, "1", 1) == 0) {
			usb0_attached = 1;
		} else {
			usb0_attached = 0;
		}
		fclose(fp);
	} else
		usb0_attached = 0;

	memset(tmp, 0, 8);
	fp = NULL;

	fp = fopen("/tmp/usb1_attached", "r");
        if (fp != NULL) {
                fgets(tmp, 8, fp);
                if (strncmp(tmp, "1", 1) == 0) {
                        usb1_attached = 1;
		} else {
			usb1_attached = 0;
		}
                fclose(fp);
        } else
                usb1_attached = 0;



	system("/sbin/cmddlna stop");
	/* wait some time before starting to mount partitions ... */
	sleep(2);

	printf("\n\n");
	list_for_each(pos, &udisk->node) {
		upart = list_entry(pos, struct upartition_info, list);
		printf("[*** USB Storage ***] Mounting USB Disk Partition %s ...\n", upart->name);

		snprintf(cmd, sizeof(cmd), "/sbin/hotplug2.mount %s", upart->name);
		system(cmd);
		snprintf(cmd, sizeof(cmd), "/usr/sbin/usb_cfg approve %s", upart->name);
		system(cmd);
	}

	if (list_empty(&udisk->node)) { /* No partitions, try to use disk name such as 'sda' to mount ... */
		printf("[*** USB Storage ***] Mounting USB Disk Partition %s ...\n", udisk->disk_name);
	
	snprintf(cmd, sizeof(cmd), "/sbin/hotplug2.mount %s", udisk->disk_name);
	system(cmd);
	snprintf(cmd, sizeof(cmd), "/usr/sbin/usb_cfg approve %s", udisk->disk_name);
	system(cmd);
	}

	printf("[*** USB Storage ***] Mounted the Disk %s <--> %s successfully!!!\n",
				udisk->disk_name, udisk->scsi_name);

	/* Turn usbled on/off */
	//system("/usr/bin/killall usbled");
	printf("usb0[%d], usb1[%d] !\n", usb0_attached, usb1_attached);	
	if (usb0_attached == 1) {
		printf("[USB] usb led 0 KEEP ON !\n");
		system("usb_led_stop 0 2> /dev/null");
		system("usb_led_stop 4 2> /dev/null");
		system("/usr/sbin/usb_led 2");
		system("cat /dev/null > /tmp/usb0_attached");
		usb0_attached = 0;
	}
	if (usb1_attached == 1) {
		printf("[USB] usb led 1 KEEP ON !\n");
		system("usb_led_stop 1 2> /dev/null");
		system("usb_led_stop 5 2> /dev/null");
		system("/usr/sbin/usb_led 3");
		system("cat /dev/null > /tmp/usb1_attached");
		usb1_attached = 0;
	}

	/* Update the share information when the full disk is mounted. */
	snprintf(cmd, sizeof(cmd), "/usr/sbin/update_smb %s", udisk->disk_name);
	system(cmd);
	/* Update the AFP share information when the full disk is mounted. */
        snprintf(cmd, sizeof(cmd), "/usr/sbin/update_afp %s", udisk->disk_name);
        system(cmd);
	
	system("/sbin/cmdftp start 2> /dev/null");
	system("/usr/sbin/chkfuppes 2> /dev/null");
	system("/sbin/cmddlna start 2> /dev/null");
	system("/usr/sbin/green_download.sh start &");

	exit(0);
}

static void run_mounting_disk(void)
{
	struct udisk_info *udisk;
	struct upartition_info *upart;
	struct list_head *pos, *n, *iter, *nxt;

	list_for_each_safe(pos, n, &disks_list) {
		udisk = list_entry(pos, struct udisk_info, list);
		if (!udisk->finished)
			continue;

		if (fork() == 0) {
			run_action(udisk);
			system("cat /dev/null > /tmp/usb0_attached");
		        system("cat /dev/null > /tmp/usb1_attached");
		}
		list_del(pos);
		list_for_each_safe(iter, nxt, &udisk->node) {
			upart = list_entry(iter, struct upartition_info, list);
			list_del(iter);
			free(upart);
		}
		free(udisk);
	}
}

#if MOUNT_TIME_CHECK
void mount_timeover_handle(int sig)
{
#define INIT_SCSI_NAME "WAIT"
	struct list_head *pos;
	struct udisk_info *udisk;

	list_for_each(pos, &disks_list) {
		udisk = list_entry(pos, struct udisk_info, list);
		if (strcmp(udisk->scsi_name, INIT_SCSI_NAME) == 0) {
			snprintf(udisk->scsi_name, sizeof(udisk->scsi_name), "%s", "X-DISK");
			udisk->finished = 1;
		}
	}

	run_mounting_disk();
}
#endif

int disattached_host_id(char *buf)
{
	char back[1024];
        char *ptr1 = NULL;
	char *ptr2 = NULL;
        unsigned int len;

        if (!buf)
                return -1;

        strcpy(back, buf);
        ptr1 = strstr(back, "usb");
	if (!ptr1)
		return -1;

        ptr2 = strchr(ptr1, '/');
	if (!ptr2)
		return -1;

        len = ptr2 - ptr1;

        if (len > 0) {
                if (strncmp("usb1", ptr1, len) == 0)
                        return 0;
                else if (strncmp("usb2", ptr1, len) == 0)
                        return 1;
        }

	return -1;
}

void check_and_stop_led(int id)
{
	char buf[512];
	char *ptr;
	FILE *fp;
	char *host[2] = {"usb1", "usb2"};
	int need_stop = 1;

	system("ls -l /sys/block/ | grep ath-ehci > /tmp/usb_block");
	fp = fopen("/tmp/usb_block", "r");
	if (fp != NULL) {
		while(fgets(buf, 512, fp) != NULL) {
			ptr = strstr(buf, host[id]);
			if (ptr != NULL) {
				printf("%s has device %\n", host[id], ptr);
				ptr = strstr(ptr, "sd");
				if (ptr != NULL)
					need_stop = 0;
			}
		}
	}

	if (need_stop) {
		printf("USB led %d off !\n", id);
		if (id == 0) {
			system("usb_led_stop 2 2> /dev/null");
			system("ledcontrol -n usb0 -c green -s off");
			system("config set usb_led0=\"off\"");
			usb1_count = 0;
		} else if (id == 1) {
			system("usb_led_stop 3 2> /dev/null");
                        system("ledcontrol -n usb1 -c green -s off");
                        system("config set usb_led1=\"off\"");
			usb2_count = 0;
		}
	}
}

void dongle_disattached_process(const char *path, const char *intf)
{
        char cmd[128];

        if (!path && !intf)
                return;

        sprintf(cmd, "/etc/usb_modem_hotplug %s off &", intf);
        system(cmd);

        memset(cmd, 0, 128);

        sprintf(cmd, "/sbin/manage_usb_led.sh %s off &", path);
        system(cmd);
}

void check_disattached_issue(char *buf, int buf_size)
{	
	int bufpos;
        struct uevent_msg msg;
	int host_id;

	if (strncmp("remove@", buf, 7))
		return;

	memset(&msg, 0x00, sizeof(msg));
	for (bufpos = strlen(buf) + 1; bufpos < buf_size; ) {

		char *key;
		
		key = &buf[bufpos];
		bufpos += strlen(key) + 1;
		DEBUGP("[RemoveKEY]: %s \n", key);

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
                dongle_disattached_process(msg.devpath, msg.interface);
        }

	if (msg.seqnum == NULL ||msg.action == NULL ||strcmp(msg.action, "remove"))
                return;
        if (msg.devpath == NULL ||msg.subsystem == NULL || msg.major == NULL ||msg.minor == NULL)
                return;

	if ((host_id = disattached_host_id(msg.devpath)) < 0) {
		printf("Disattached host id error !\n");
		return;
	}

	printf("HOST %d has device disattached !\n", host_id);
	check_and_stop_led(host_id);
}

int main(int argc, char **argv)
{
	int size;
	int netlink_socket;
	static char buffer[UEVENT_BUFFER_SIZE + 512];

	netlink_socket = init_netlink_socket();
	if (netlink_socket == -1) {
		printf("Failed to open NETLINK_KOBJECT_UEVENT netlink socket!\n");
		return -1;
	}

	printf("USB Storage daemon is Running ... \n");
	system("config set usb_led0=\"\"");
	system("config set usb_led1=\"\"");
	daemon(1, 1);
	signal(SIGCHLD, sigchld);

	while (1) {
		size = recv(netlink_socket, buffer, sizeof(buffer) - 1, 0);
		if (size <  0) {
			if (errno != EINTR)
				printf("Unable to receive kernel netlink message: %s\n", strerror(errno));
			continue;
		}

		buffer[size] = '\0';
		DEBUGC(buffer, size);
		
		check_disattached_issue(buffer, size);
		parse_uevent_msg(buffer, size);
		run_mounting_disk();
	}

	return 0;
}

