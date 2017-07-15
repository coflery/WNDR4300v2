/* check share_info in all partitions and update AppleVolumes.default and reload afpd.
 *
 *  Copyright (C) 2008 - 2009, Delta Networks, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>

/*
  * The 'USB_Functionality_specification_v0.2.doc' is modified too much, so I don't want to
  * touch the original code .... :)
  */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>

#include "list.h"

#if 1
#define USB_DEBUGP(format, args...) printf(format, ## args)
#else
#define USB_DEBUGP(format, args...)
#endif

struct disk_partition_info
{
	struct list_head list;
	int mounted;
	int afplistupdated;
	int ishfsplus;
	char label;	/* `U` ~ ... */
	char name[15]; /* `sda1` `sda2` */
	char vendor[128];  /*device name :SigmaTel MSCN*/
	char vol_name[31]; /* Volume Name */
	unsigned long long capacity; /* capacity size in MB */
};

struct share_info
{
	struct list_head list;
	char name[];
};

#define USB_APPLE_VOLUMES_DEFAULT_CONF	"/etc/netatalk/AppleVolumes.default"
#define AVAHI_SERVICE_ADISK	"/etc/avahi/services/adisk.service"
#define TMP_AFP_LOCK  "/tmp/tmp_afp_lock"
#define SHARE_FILE_INFO "shared_usb_folder"

#define USER_ADMIN "admin"
#define USER_GUEST "guest"
#define USB_PATH_SIZE	4096

int support_disk = 0;

extern char *config_get(char* name);
extern void config_set(char *name, char *value);
extern int config_match(char *name, char *match);

static void reload_services(void)
{
	int ret;
	/* directly return ,when disable the usb network for afp access */
	/* FIXME: what is the use case??? */
//	if (config_match("usb_enableNet", "1"))
//		return;

	/* Sync with locking file, and wait 1s to not miss SIGUP for `afpd` */
	sleep(1);
	ret = system("/bin/pidof afpd > /dev/zero 2>&1");
	if (ret == 0) {
		system("/bin/kill -HUP `cat /var/run/afpd.pid` > /dev/null 2>&1");
	} else {
		system("/bin/nice -n 19 /usr/sbin/afpd -F /etc/netatalk/afpd.conf -P /var/run/afpd.pid -c 7 > /dev/null 2>&1");
	}

#if 0 /* Not required */
	/* cnid_metad */
	ret = system("/bin/pidof cnid_metad > /dev/zero 2>&1");
	if (ret != 0)
		system("/usr/sbin/cnid_metad > /dev/zero 2>&1");
#endif

	/* avahi-daemon: not required */
}

static inline char *user_name(char *code)
{
	if (*code == '1')
		return USER_ADMIN;
	else
		return USER_GUEST;
}

static void add_afpd_share_info(FILE *fp, char *displayname, char *reader, char *writer, char *path)
{
	fprintf(fp, "%s \"%s\"", path, displayname);

	/* FIXME: set proper permission and/or allow proper user */
	if (strncmp(reader, USER_GUEST, strlen(USER_GUEST)))
		fprintf(fp, " allow:@admin deny:@guest");
	else if (strncmp(writer, USER_GUEST, strlen(USER_GUEST)))
		fprintf(fp, " allow:@admin,@guest rolist:@guest");
	else
		fprintf(fp, " allow:@admin,@guest");

	fprintf(fp, " cnidscheme:cdb options:usedots,tm\n");
}

int is_sda(char * dev)
{
	int count = 0;
	FILE *fp;
	char part_name[16], line[128];
	int major, minors;
	unsigned long long capacity;

	fp = fopen("/proc/partitions", "r");
	if (fp == NULL)
		goto ret;

	/*
	 *           * major minor  #blocks  name
	 *           *
	 *           *  31     0        320 mtdblock0
	 *           * ....
	 *           *   8     0    3968000 sda
	 *           *   8     1    3963968 sda1
	 *
	 */

	while (fgets(line, sizeof(line), fp)) {
		if (sscanf(line, " %d %d %llu %[^\n ]",
					&major, &minors, &capacity, part_name) != 4)
			continue;
		if (strncmp(part_name, dev, 3))
			continue;
		else
			count++;
	}

ret:
	if (fp != NULL)
		fclose(fp);
	
	return ((count == 1)?1:0);
}

static char *get_device_vendor(char *dev)
{
	int i,j;
	FILE *fp;
	char line[100];
	static char vendor[128];
	char path[64], *ven_mod[] = {"vendor", "model"};
	
	vendor[0] = '\0';
	
	for (i=0; i<2; i++){
		snprintf(path, sizeof(path), "/sys/block/%s/device/%s", dev, ven_mod[i]);
		if (!(fp = fopen(path, "r")))
			continue;
		fgets(line, sizeof(line), fp);
		fclose(fp);

		j = 0;
		while (line[j] != '\0' && line[j] != '\r' && line[j] != '\n')
			j++;
		line[j] = '\0';

		strcat(vendor, line);
		strcat(vendor, " ");
	}
	
	return vendor;
}

/*
  * When presenting the Capacity of a device, the appropriate units should be used.
  * If a device is 1GB in size this should be displayed as 1GB, however a 300MB device
  * should be displayed as 300MB and not 0.3GB. (29.66GB, 44.53GB).
  */
void format_capacity(char *buf, int buflen, unsigned long long megabytes)
{
	if (megabytes >= 1024) {
		unsigned long long left = ((megabytes & 0x3FF) * 100) >> 10; // (leftMB / 1024) * 100
		if (left == 0)
			snprintf(buf, buflen, "%llu GB", (megabytes >> 10));
		else
			snprintf(buf, buflen, "%llu.%02llu GB", (megabytes >> 10), left);
	} else {
		snprintf(buf, buflen, "%llu MB", megabytes);
	}
}

static int is_special_backup_format(char *dev)
{
#define DISK_FORMAT_FILE "/tmp/disk_special_format"
       int ret = 0;
       int i = 0;
       FILE *fp;
       char cmd[128], buf[256];

       memset(cmd,0,128);
       memset(buf,0,256);
       snprintf(cmd, sizeof(cmd), "/bin/mount | grep '/mnt/%s' | awk '{print$5}' > " DISK_FORMAT_FILE, dev);
       system(cmd);

       if (!(fp = fopen(DISK_FORMAT_FILE, "r")))
               return -1;

       fgets(buf, sizeof(buf), fp);
       fclose(fp);

       i = 0;
       while (buf[i] != '\0' && buf[i] != '\r' && buf[i] != '\n')
               i++;
       buf[i] = '\0';
       if ( !strcmp(buf,"hfsplus") || !strcmp(buf, "xfs") || !strcmp(buf, "ntfs")
                       || !strcmp(buf, "ext2") || !strcmp(buf, "ext3") || !strcmp(buf, "ext4") )
               ret = 1;
       printf("dev = %s, buf = %s", dev, buf);
       return ret;
}

static int is_hfsplus_formated(char *dev)
{
#define VOLUME_ID_FILE "/tmp/afp_vol_id"
	int  ret = 0;
	int  i = 0;
	char cmd[128], buf[256];
	FILE *fp;

	memset(cmd,0,128);
	memset(buf,0,256);

	snprintf(cmd, sizeof(cmd), "/usr/sbin/vol_id -t /dev/%s > " VOLUME_ID_FILE, dev);
	system(cmd);

	if (!(fp = fopen(VOLUME_ID_FILE, "r"))) {
		return -1;
	}

	fgets(buf, sizeof(buf), fp);
	fclose(fp);

	i = 0;
	while (buf[i] != '\0' && buf[i] != '\r' && buf[i] != '\n')
		i++;
	buf[i] = '\0';

        if ( !strcmp(buf,"hfsplus") || !strcmp(buf, "xfs") || !strcmp(buf, "ntfs")
                        || !strcmp(buf, "ext2") || !strcmp(buf, "ext3") || !strcmp(buf, "ext4") ) {
		ret = 1;
	}
	USB_DEBUGP("\nDev: %s vol_type: %s ret: %d\n", dev, buf, ret);	
	return ret;
}


static void  get_disk_volume(struct disk_partition_info *disk, char *part_name)
{
#define VOLUME_ID_FILE "/tmp/afp_vol_id"
	int i;
	FILE *fp;
	char *buf, cmd[128];

	buf = disk->vol_name;
	buf[0] = '\0';

	snprintf(cmd, sizeof(cmd), "/usr/sbin/vol_id -L /dev/%s > " VOLUME_ID_FILE, part_name);
	system(cmd);

	if (!(fp = fopen(VOLUME_ID_FILE, "r")))
		goto ret;

	fgets(buf, sizeof(disk->vol_name), fp);
	fclose(fp);

	i = 0;
	while (buf[i] != '\0' && buf[i] != '\r' && buf[i] != '\n')
		i++;
	buf[i] = '\0';

ret:
	/*
	 * If Volume Name is empty, then use <USB Device Letter> Drive (<Capacity>)
	 * e.g U Drive (512MB)
	 */

	if (buf[0] == '\0') {
		char capacity[32];

		format_capacity(capacity, sizeof(capacity), disk->capacity);
		snprintf(buf, sizeof(disk->vol_name), "%c Drive (%s)", disk->label, capacity);
	}
}

static void scan_disk_entries(struct list_head *head)
{
	FILE *fp;
	struct statfs statbuf;
	int i = 0, major,minors, cnt;
	char buf[8];
	FILE *afp = NULL;;
	int have_disk_mouted = 0, have_hfsplus_disk_mounted = 0;
	unsigned long long capacity;
	char mnt_path[32],*vendor = NULL;
	char *s, part_name[128], line[256];
	struct disk_partition_info *partinfo;

	fp = fopen("/proc/partitions","r");
	if (fp == NULL )
		return;

	/*
	  * major minor  #blocks  name
	  *
	  *  31     0        320 mtdblock0
	  * ....
	  *   8     0    3968000 sda
	  *   8     1    3963968 sda1
	  */
	while (fgets(line,sizeof(line),fp)) {
		if (sscanf(line, " %d %d %llu %[^\n ]",
				&major, &minors, &capacity, part_name) != 4)
			continue;
		if (strncmp(part_name, "sd", 2))
			continue;
		for (s = part_name; *s; s++)
			;
		if (!isdigit(s[-1])) {
			vendor = get_device_vendor(part_name);
	
			if (!is_sda(part_name))
				continue;
		}

		capacity >>= 10;        /* unit: 1KB .. >> 1   size /512 (long *arg) */
		if (capacity == 0)
			continue; /*It indicates that this partition should be an extended partition. */

		partinfo = malloc(sizeof(struct disk_partition_info));
		if (partinfo == NULL)
			continue;

		/* SEE: hotplug2.mount ==> mount /dev/$1 /mnt/$1 */
		snprintf(mnt_path, sizeof(mnt_path), "/mnt/%s", part_name);
		/* NO Disk, the mount point directory is NOT removed, this magic value is `0x858458F6` */
		if (statfs(mnt_path, &statbuf) == 0 && (unsigned int)statbuf.f_type != 0x858458F6)
			partinfo->mounted = 1, have_disk_mouted = 1;
		else
			partinfo->mounted = 0;
		partinfo->afplistupdated = 0;
                if ( (is_hfsplus_formated(part_name)) || (is_special_backup_format(part_name)) ){
                        printf("This HDD format can support the feature of Time Machine... ...\n");
                        partinfo->ishfsplus = 1;
			if (partinfo->mounted)
				support_disk++;
                }else{
                        printf("[Waring]: This HDD format failed to support the Time Machine!!!!!!\n");
                        partinfo->ishfsplus = 0;
                }
		if (partinfo->ishfsplus == 1)
			have_hfsplus_disk_mounted = 1;
		partinfo->capacity = capacity;
		partinfo->label = 'U' - i;
		snprintf(partinfo->name, sizeof(partinfo->name),"%s", part_name);				
		if (vendor)
			strcpy(partinfo->vendor,vendor);
		
		get_disk_volume(partinfo, part_name);
		
		list_add_tail(&partinfo->list, head);
		i++;

		USB_DEBUGP("[USB-AFP]: Found partition %s, mounted %s!!!\n", part_name,
					partinfo->mounted ? "Yes" : "No");
	}		

	fclose(fp);
	USB_DEBUGP("[USB-AFP]: Total %d partitions are FOUND!\n", i);

	if (support_disk == 0) {
                system("killall avahi-daemon");
        } else if (support_disk >= 1 && have_hfsplus_disk_mounted) {
                system("ps|grep avahi-daemon|grep -v -c grep > /tmp/avahi_count");
                afp = fopen("/tmp/avahi_count", "r");
                if (!afp)
                        return;
	                fgets(buf, 8, afp);
	                fclose(afp);
	                cnt = atoi(buf);
	                printf("[USB-AFP] Exist avahi daemon num %d !\n", cnt);
	                if (cnt == 0)
                        system("avahi-daemon -f /etc/avahi/avahi-daemon.conf -D");
        }
	
	if (have_disk_mouted) {
		/* reload avahi with afpd and adisk services */
		if (have_hfsplus_disk_mounted)
			system("/usr/sbin/avahi-afpd-name afpd");
		else
			system("rm /etc/avahi/services/afpd.service > /dev/null 2>&1");
		system("/usr/sbin/avahi-afpd-name adisk");
		system("echo \"    <txt-record>sys=waMA=$(/bin/config get wan_factory_mac),adVF=0x1000</txt-record>\" >> /etc/avahi/services/adisk.service");
	} else {
		/* reload avahi without afpd and adisk services */
		system("rm /etc/avahi/services/afpd.service > /dev/null 2>&1");
		system("rm /etc/avahi/services/adisk.service > /dev/null 2>&1");
	}
}

static inline int duplicate_share_name(char *name, struct list_head *head)
{
	struct list_head *pos;
	struct share_info *share;

	list_for_each(pos, head) {
		share = list_entry(pos, struct share_info, list);
		if (strcmp(share->name, name) == 0)
			return 1;
	}

	return 0;
}

static inline void add_share_info_list(char *name, struct list_head *head)
{
	struct share_info *share;

	share = malloc(sizeof(struct share_info) + strlen(name) + 1);
	if (share == NULL)
		return;
	strcpy(share->name, name);
	list_add_tail(&share->list, head);
}

/* encode string to xml-string */
static char *xml_encode(char *share_name)
{
	int i = 0;
	int output_len = 0;
	int temp_expansion_len = 0;
	char *encoded_xml_string = NULL;
	char temp_expansion[10];
	char *p1 = NULL;

	p1 = share_name;
	output_len = (int)strlen(share_name) * 6;

	encoded_xml_string = (char *) malloc(output_len + 1);
	if (encoded_xml_string == NULL)
		return NULL;

	while (*p1) {
		/* alpha-numeric characters don't get encoded */
		if ((*p1 >= '0' && *p1 <= '9') || (*p1 >= 'A' && *p1 <= 'Z') || (*p1 >= 'a' && *p1 <= 'z')) {
			encoded_xml_string[i++] = *p1;

		/* spaces, hyphens, periods, underscores and colons don't get encoded */
		} else if ((*p1 == ' ') || (*p1 == '-') || (*p1 == '.') || (*p1 == '_') || (*p1 == ':')) {
			encoded_xml_string[i++] = *p1;

		/* ',' char encoded as "\," */
		} else if (*p1 == ',') {
			if (i < (output_len - 2)) {
				strcpy(&encoded_xml_string[i], "\\,");
				i += 2;
			}

		/* for simplicity, all other chars represented by their numeric value */
		} else {
			snprintf(temp_expansion, 9, "&#%d;", (unsigned char)(*p1));
			temp_expansion_len = (int)strlen(temp_expansion);
			if (i < (output_len - temp_expansion_len)) {
				strcpy(&encoded_xml_string[i], temp_expansion);
				i += temp_expansion_len;
			}
		}
		p1++;
	}

	encoded_xml_string[i] = '\0';
	return encoded_xml_string;
}

static int update_adisk(int file_fmt, char *share_name)
{
	static int cnt = 0;
	FILE *adisk_conf_fp = NULL;
	char *encoded_share_name = NULL;

	if((adisk_conf_fp = fopen(AVAHI_SERVICE_ADISK, "a") ) == NULL) {
		USB_DEBUGP("[USB-AFP]: Unable To Open Adisk Service File.....\n");
		return -1;
	}

	fseek(adisk_conf_fp, 0, SEEK_END);

	encoded_share_name = xml_encode(share_name);
	if (file_fmt == 1) {
		/* For HFS+ File System that will be shown in the TimeMachine available disk list */
		fprintf(adisk_conf_fp, "    <txt-record>dk%d=adVF=0x1003,adVN=%s,adVU=</txt-record>\n", cnt++, (encoded_share_name ? encoded_share_name : share_name));
	}
#if 0
	else {
		/* For other file system that will be not appear in TimeMachine disk list,
		 * but can be viewed in Finder window */
		fprintf(adisk_conf_fp, "    <txt-record>dk%d=adVF=0x1002,adVN=%s,adVU=</txt-record>\n", cnt++, (encoded_share_name ? encoded_share_name : share_name));
	}
#endif
	if (encoded_share_name)
		free(encoded_share_name);

	fclose(adisk_conf_fp);
	return 0;
}

static int commit_adisk()
{
	char cmd[256];
	
	memset(cmd,0,256);
	sprintf(cmd,"echo -e \"  </service>\n</service-group>\" >> %s", AVAHI_SERVICE_ADISK);
	system(cmd);
}

static void load_share_info(FILE *fp, char *diskname)
{
	int no_shareinfo = 1; /* If `diskname` is not NULL, check if there is no share info in disk */
	int num_mounted_disk = 0;
	char cmd[128];

	struct share_info *shareinfo;
	struct disk_partition_info *diskinfo;
	struct list_head disk_lists, share_lists, *pos, *nxt;

	INIT_LIST_HEAD(&disk_lists);
	INIT_LIST_HEAD(&share_lists);

	scan_disk_entries(&disk_lists);

	USB_DEBUGP("[USB-AFP]: Loading USB share information ......diskname: %s\n", diskname);
	list_for_each(pos, &disk_lists) {
		int j;
		char name[64],*val,oneline[1024];
		char *volumeName,*deviceName;
		char fullpath[USB_PATH_SIZE],dupshare[USB_PATH_SIZE];
		char *sep, *t_share_name, *folderName, *readAccess, *writeAccess;
		char  share_name[128];

		diskinfo = list_entry(pos, struct disk_partition_info, list);
		if (!diskinfo->mounted)
			continue;

		sep = "*\n";
		for (j=0; ;j++) {
			sprintf(name, SHARE_FILE_INFO"%d",j);
			val = config_get(name);
			if (*val == '\0')
				break;

			strcpy(oneline, val);

			t_share_name = strtok(oneline, sep); /* share name */
			folderName = strtok(NULL, sep);	/* folder name */
			readAccess = strtok(NULL, sep);	/* readAccess*/
			writeAccess = strtok(NULL, sep);	/* writeAccess */
			volumeName = strtok(NULL, sep);   /* volumeName*/
			deviceName = strtok(NULL, sep);    /* deviceName */

			memset(share_name, 0, 128);
			sprintf(share_name, "%s", t_share_name);

			if (share_name == NULL || folderName == NULL || readAccess == NULL ||writeAccess == NULL ||
				volumeName == NULL || deviceName == NULL )
				continue;

			if (strcmp(volumeName, diskinfo->vol_name) || strcmp(deviceName, diskinfo->vendor))
				continue;

			if (duplicate_share_name(share_name, &share_lists)) {
				// Fixme: if volume name also different then dupplicate.
				snprintf(dupshare, sizeof(dupshare), "%s(%c)", share_name, diskinfo->label);
				memset(share_name, 0, 128);
				sprintf(share_name,"%s", dupshare);
			}

			add_share_info_list(share_name, &share_lists);

			snprintf(fullpath, sizeof(fullpath), "/mnt/%s%s", diskinfo->name, folderName);

			readAccess = user_name(readAccess);
			writeAccess = user_name(writeAccess);

			USB_DEBUGP("[USB-AFP]: AFPInfo %s Folder:%s Reader:%s Writer: %s\n", share_name, folderName, readAccess, writeAccess);

			if (diskinfo->ishfsplus) {
				add_afpd_share_info(fp, share_name, readAccess, writeAccess, fullpath);
				update_adisk(1,share_name);
			} else {
				update_adisk(0,share_name);
			}

			diskinfo->afplistupdated = 1;
			num_mounted_disk++;
			USB_DEBUGP("\nIn First Step: valume: %s afplist: %d", diskinfo->vol_name, diskinfo->afplistupdated);

			if (diskname != NULL && strncmp(diskinfo->name, diskname, 3) == 0)
				no_shareinfo = 0;
		}
	}
#if 0
	if ( diskname != NULL) {
		int i;
		char value[256],name[64],*val;
		char share_name[64], folder_path[64], dupshare[USB_PATH_SIZE];

		USB_DEBUGP("[USB-AFP]: Disk %s has no share information!\n", diskname);
		list_for_each(pos, &disk_lists) {
			diskinfo = list_entry(pos, struct disk_partition_info, list);
			if (!diskinfo->mounted || diskinfo->afplistupdated || strncmp(diskinfo->name, diskname, 3))
				continue;

			USB_DEBUGP("\nIn Next Step: valume: %s afplist: %d", diskinfo->vol_name, diskinfo->afplistupdated);
			snprintf(share_name, sizeof(share_name),"%c_Drive", diskinfo->label);

			snprintf(value,sizeof(value),"%s*/*0*0*%s*%s",share_name,diskinfo->vol_name,diskinfo->vendor);

			for (i=0;i<100 ;i++){
				sprintf(name, SHARE_FILE_INFO"%d", i);
				val = config_get(name);
				if (*val == '\0')
					break;
			}
			config_set(name,value);
			snprintf(folder_path, sizeof(folder_path), "/mnt/%s/", diskinfo->name);

			if (diskinfo->ishfsplus) {
				add_afpd_share_info(fp, share_name, USER_GUEST, USER_GUEST, folder_path);
				update_adisk(1,share_name);
			}
			else {
				update_adisk(0,share_name);
			}
			num_mounted_disk++;
		}
	}
#endif
	list_for_each_safe(pos, nxt, &disk_lists) {
		diskinfo = list_entry(pos, struct disk_partition_info, list);
		list_del(pos);
		free(diskinfo);
	}

	list_for_each_safe(pos, nxt, &share_lists) {
		shareinfo = list_entry(pos, struct share_info, list);
		list_del(pos);
		free(shareinfo);
	}

	if (num_mounted_disk > 0) {
		commit_adisk();
	}
}

void cleanup(int signal)
{
	printf("Try to recover from endless waiting.\n");
	reload_services();
	unlink(TMP_AFP_LOCK);
	exit(1);
}

int check_afp_locked(void)
{
	return (!access(TMP_AFP_LOCK, F_OK));
}

int main(int argc, char**argv)
{
	FILE *fp, *filp;
	char *diskname = NULL;
	struct timeval currenttime, newtime;

	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);

	gettimeofday(&currenttime, NULL);

	while (check_afp_locked()) {
		gettimeofday(&newtime, NULL);
		/* the longest waiting time is 30s, avoid endless waiting */
		if ((newtime.tv_sec - currenttime.tv_sec) > 30)
			cleanup(0);
		sleep(1);
	}

	/* create lock file */
	filp = fopen(TMP_AFP_LOCK, "w+");
	if (filp)
		fclose(filp);
	else {
		printf("error when creating afp_lock file!\n");
		return 1;
	}

	fp = fopen(USB_APPLE_VOLUMES_DEFAULT_CONF, "w");
	if (fp == NULL)
		goto unlock;

	if (argc == 2 && strlen(argv[1]) == 3 && strncmp(argv[1], "sd", 2) == 0)
		diskname = argv[1];	/* sd[a-z] */

	load_share_info(fp, diskname);

	fclose(fp);

	reload_services();

unlock:	
	unlink(TMP_AFP_LOCK);
	return 0;
}
