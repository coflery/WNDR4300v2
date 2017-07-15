/* check share_info in all partitions and update smb.conf and reload smbd.
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
#if 1
#include <sys/types.h>
#include <sys/stat.h>
#include <mntent.h>
#include <sys/statfs.h>
#include <unistd.h>

#include "list.h"

#if 0
#define USB_DEBUGP(format, args...) printf(format, ## args)
#else
#define USB_DEBUGP(format, args...)
#endif

struct disk_partition_info
{
	struct list_head list;

	int mounted;
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

#define USB_SESSION		"[USB Storage]"
#define USB_INFO_FILE	".NETGEAR_disk_share_info"
#define USB_SMB_CONF	"/etc/samba/smb.conf"
#define USB_SMB_NAME	"NETGEAR WNDR4500v3"
#define TMP_SAMBA_LOCK  "/tmp/tmp_samba_lock"
#define SHARE_FILE_INFO "shared_usb_folder"

#define USER_ADMIN "admin"
#define USER_GUEST "guest"
#define USB_PATH_SIZE	4096
#define USB_STORAGE_KEYWORD "/dev/sd"

extern char *config_get(char* name);
extern void config_set(char *name, char *value);
extern int config_match(char *name, char *match);

static void reload_services(void)
{
	int ret;

       int has_usb_storage = 0;
       struct mntent *ent;
       struct statfs stat;
       FILE *fp = setmntent("/proc/mounts", "r");
       while ((ent = getmntent(fp))) {
               if (strncmp(ent->mnt_fsname, USB_STORAGE_KEYWORD, strlen(USB_STORAGE_KEYWORD)))
                       continue;

               bzero(&stat, sizeof(struct statfs));
               if (-1 == statfs(ent->mnt_dir, &stat)) {
                       continue;
               }
               has_usb_storage = 1;
               break;
       }
       endmntent(fp);

       if (!has_usb_storage){
               system("/usr/bin/killall smbd");
               system("/usr/bin/killall nmbd");
               goto outer;
       }


	/* Sync with locking file, and wait 1s to not miss SIGUP for `smbd` */
	sleep(1);
	ret = system("/bin/pidof smbd > /dev/zero 2>&1");
	if (ret == 0)
		system("/usr/bin/killall -SIGHUP smbd");
	else
		system("/bin/nice -n 19 /usr/sbin/smbd -D");

	/* NETBIOS Name */
	system("/usr/bin/killall nmbd > /dev/null 2>&1");
	system("/usr/sbin/nmbd -D");

outer:
	sleep(0);
	/* Tell 'uhttpd' to update HTTP share information */
	//system("/usr/bin/killall -SIGUSR1 uhttpd"); 
	//hidden this code, because every time call the net-cgi will update HTTP share again.
	
}

static void add_smbd_global(FILE *fp)
{
	char *p;

	if (config_match("ap_mode", "0") && config_match("bridge_mode", "0")) {
		char add_iface[] = "lo";
		int s = system("ifconfig LeafNets >/dev/null 2>&1");
		/* need to add "br0" */
		if (config_invmatch("usb_enableNet", "1"))
			sprintf(add_iface, "%s %s", add_iface, "br0");
		/* need to add LeafNets */
		if (!s)
			sprintf(add_iface, "%s %s", add_iface, "LeafNets");
		fprintf(fp, "[global]\n"
			"  interfaces=%s\n", add_iface);
                fprintf(fp, "/%s\n", config_get("lan_netmask"));
        /* in AP mode static IP mode */
        } else if (config_match("ap_mode", "1") && config_match("ap_ether_ip_assign", "0")) {
                fprintf(fp, "[global]\n"
                        "  interfaces=br0 %s", config_get("ap_ipaddr"));
                fprintf(fp, "/%s\n", config_get("ap_netmask"));
        /* in AP mode DHCP mode */
        } else if (config_match("ap_mode", "1") && config_match("ap_ether_ip_assign", "1")) {
                fprintf(fp, "[global]\n"
                        "  interfaces=br0 %s", config_get("ap_dhcp_ipaddr"));
                fprintf(fp, "/%s\n", config_get("ap_dhcp_netmask"));
        /* in bridge mode static IP mode */
        } else if (config_match("bridge_mode", "1") && config_match("bridge_ether_ip_assign", "0")) {
                fprintf(fp, "[global]\n"
                        "  interfaces=br0 %s", config_get("bridge_ipaddr"));
                fprintf(fp, "/%s\n", config_get("bridge_netmask"));
        /* in bridge mode DHCP mode */
        } else if (config_match("bridge_mode", "1") && config_match("bridge_ether_ip_assign", "1")) {
                fprintf(fp, "[global]\n"
                        "  interfaces=br0 %s", config_get("ap_dhcp_ipaddr"));
                fprintf(fp, "/%s\n", config_get("ap_dhcp_netmask"));
        }

	p = config_get("usb_workGroup");
	if (*p == '\0')
		p = "Workgroup";
	fprintf(fp, "  workgroup = %s\n", p);

	p = config_get("usb_deviceName");
	if (*p == '\0')
		p = "WNDR4500v3";
	fprintf(fp, "  netbios name = %s\n", p);

	fprintf(fp, "  bind interfaces only = yes\n"
			"  server string = " USB_SMB_NAME "\n"
			"  socket options = TCP_NODELAY\n"
			"  security = user\n"
			"  host msdfs = no\n"	/* Fix [BUG 12866] */
			"  hostname lookups = no\n"
			"  load printers = no\n"
			"  printing = bsd\n"
			"  printcap name = /dev/null\n"
			"  disable spoolss = yes\n"
 			"  guest account=guest\n"
			"  encrypt passwords = yes\n"
			"  name resolve order = lmhosts hosts bcast\n"
			"  smb passwd file = /etc/samba/smbpasswd\n"
			"  display charset = UTF-8\n"
			"  unix charset = UTF-8\n"
			"  dos charset = UTF-8\n"
		        "  map to guest = bad user\n"
			"  follow symlinks = no\n"
			"\n");
}

static inline char *user_name(char *code)
{
	if (*code == '1')
		return USER_ADMIN;
	else
		return USER_GUEST;
}

#if 0
/* convert "&nbsp;" to ' ' */
static void unescape_folder(char *folder)
{
	char *p1, *p2;
	p1 = p2 = folder;

	while (*p1) {
		if (*p1 == '&' && strncmp(p1, "&nbsp;", 6) == 0) {
			p1 += 6; *p2++ = ' ';
		} else {
			*p2++ = *p1++;
		}
	}

	*p2 = '\0';
}
#endif

static void add_smbd_share_info(FILE *fp, char *displayname, char *reader, char *writer, char *path)
{
	fprintf(fp, "[%s]\n"
		"  path=%s\n"
		"  read only=yes\n"
		"  force user=root\n"
		"  browsable=yes\n",
		displayname, path);	

	if (strncmp(reader, USER_GUEST, strlen(USER_GUEST)))
		fprintf(fp, "  valid users=@%s,@%s\n  write list=@%s\n  public=no\n\n", reader, writer, writer);
	else if (strncmp(writer, USER_GUEST, strlen(USER_GUEST))) {
		fprintf(fp, "  guest ok=no\n");
		fprintf(fp, "  write list=@%s\n\n", writer);
	}
	else
		fprintf(fp, "  read only=no\n  guest ok=yes\n\n");
}

#if 0
static int valid_shareinfo_file(char *path)
{
	FILE *fp;
	int valid = 0;
	char buf[USB_PATH_SIZE * 2];

	fp = fopen(path, "r");
	if (fp == NULL)
		return 0;
	while (fgets(buf, sizeof(buf), fp)){
		if (strstr(buf, USB_SESSION)) {
			valid = 1;
			break;
		} 
	}
	fclose(fp);

	return valid;	
}

static void creat_new_shareinfo_file(char *path)
{
	FILE *fp;

	fp = fopen(path, "w");
	if (fp == NULL) {
		USB_DEBUGP("[USB-SMB]: Can't creat new share info file:%s\n", path);
		return;
	}

	fprintf(fp, USB_SESSION "\n"
			"\n"
			"[UPnP Server]\n"
			"\n");
	fclose(fp);
}
#endif

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
	
	for(i=0; i<2; i++){
		snprintf(path, sizeof(path), "/sys/block/%s/device/%s", dev, ven_mod[i]);
		if(!(fp = fopen(path, "r")))
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

static void  get_disk_volume(struct disk_partition_info *disk, char *part_name)
{
	/*Module net-cgi and samba-script will read and write the same file "/tmp/vol_id", 
	  sometimes it will cause conflict. So change it to "/tmp/vol_id1"*/
#define VOLUME_ID_FILE "/tmp/vol_id1"
	int i;
	FILE *fp;
	char *buf, cmd[128];
        buf = disk->vol_name;
        buf[0] = '\0';
        snprintf(cmd, sizeof(cmd), "/usr/sbin/vol_id -L /dev/%s > " VOLUME_ID_FILE, part_name);
        system(cmd);
        if(!(fp = fopen(VOLUME_ID_FILE, "r")))
                goto ret;
        fgets(buf, sizeof(disk->vol_name), fp);
        fclose(fp);

        i = 0;
        while (buf[i] != '\0' && buf[i] != '\r' && buf[i] != '\n')
                i++;
        buf[i] = '\0';

ret:
        /*
 *           * If Volume Name is empty, then use <USB Device Letter> Drive (<Capacity>)
 *                     * e.g U Drive (512MB)
 *                               */
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
	int i = 0, major,minors;
	int have_disk_mouted = 0;
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
		if (!isdigit(s[-1])){ 
			vendor = get_device_vendor(part_name);
	
			if(!is_sda(part_name))
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
		partinfo->capacity = capacity;
		partinfo->label = 'U' - i;
		snprintf(partinfo->name, sizeof(partinfo->name),"%s", part_name);				
		if(vendor)
			strcpy(partinfo->vendor,vendor);
		
		get_disk_volume(partinfo, part_name);
		
		list_add_tail(&partinfo->list, head);
		i++;

		USB_DEBUGP("[USB-SMB]: Found partition %s, mounted %s!!!\n", part_name,
					partinfo->mounted ? "Yes" : "No");
	}		

	fclose(fp);
	USB_DEBUGP("[USB-SMB]: Total %d partitions are FOUND!\n", i);

#if 0 /* ASL */
       if (have_disk_mouted)
               turn_usb_led(1);
       else
               turn_usb_led(0);
#else
       if (have_disk_mouted) {
               /* reload avahi with smbd services */
	       system("/usr/sbin/avahi-afpd-name smbd");
       } else {
               /* reload avahi without smbd services */
               system("rm /etc/avahi/services/smbd.service > /dev/null 2>&1");
       }
#endif
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

static void load_share_info(FILE *fp, char *diskname)
{
	/* FILE *infofp;
	struct stat statbuf;
	int usb_session;*/	/* `USB_SESSION` in share information file is found.  */
	int no_shareinfo = 1; /* If `diskname` is not NULL, check if there is no share info in disk */
	/* char *shsep = "*\n";
	char infopath[128], buf[USB_PATH_SIZE * 2];
	*/

	struct share_info *shareinfo;
	struct disk_partition_info *diskinfo;
	struct list_head disk_lists, share_lists, *pos, *nxt;

	INIT_LIST_HEAD(&disk_lists);
	INIT_LIST_HEAD(&share_lists);

	scan_disk_entries(&disk_lists);

	USB_DEBUGP("[USB-SMB]: Loading USB share information ......\n");
	list_for_each(pos, &disk_lists) {
		diskinfo = list_entry(pos, struct disk_partition_info, list);
		if (!diskinfo->mounted)
			continue;
#if 0 // save in disk 
		snprintf(infopath, sizeof(infopath), "/mnt/%s/" USB_INFO_FILE, diskinfo->name);
		infofp = fopen(infopath, "r");
		if (infofp == NULL)
			continue;

		usb_session = 0;
		// share information like: TMD*/home*1*0
		while (fgets(buf, sizeof(buf), infofp)) {
			if (strncmp(buf, USB_SESSION, sizeof(USB_SESSION) - 1) == 0 ) {
				usb_session = 1;	/* Find storage label, start to read share folders info */
				continue;
			}
			if (usb_session == 0)
				continue;
			if (strcmp(buf, "\n") == 0)
				break; 			/* Find the end of storage, stop reading share folders info */

			char *displayname, *foldername, *reader, *writer;
			char dupshare[USB_PATH_SIZE], fullpath[USB_PATH_SIZE];

			displayname = strtok(buf, shsep);
			foldername = strtok(NULL, shsep);
			reader = strtok(NULL, shsep);
			writer = strtok(NULL, shsep);
			if (displayname == NULL ||foldername == NULL ||reader == NULL ||writer == NULL)
				continue;

			USB_DEBUGP("[USB-SMB]: ShareInfo %s Folder:%s Reader:%s Writer: %s\n",
							displayname, foldername, reader, writer);

			/* Check the shared directory exists or not ... */
			unescape_folder(foldername);
			snprintf(fullpath, sizeof(fullpath), "/mnt/%s%s", diskinfo->name, foldername);
			if (stat(fullpath, &statbuf) ||!S_ISDIR(statbuf.st_mode))
				continue;

			if (duplicate_share_name(displayname, &share_lists)) {
				snprintf(dupshare, sizeof(dupshare), "%s(%c)", displayname, diskinfo->label);
				displayname = dupshare;
			}
			add_share_info_list(displayname, &share_lists);
	
			reader = user_name(reader);
			writer = user_name(writer);	

			USB_DEBUGP("[USB-SMB]: SMBInfo %s Folder:%s Reader:%s Writer: %s\n",
								displayname, foldername, reader, writer);

			add_smbd_share_info(fp, displayname, reader, writer, fullpath);
			if (diskname != NULL && strncmp(diskinfo->name, diskname, 3) == 0)
				no_shareinfo = 0;
		}	

		fclose(infofp);
#else //save in config

		int j;
		char name[64],*val,oneline[1024];
		char *volumeName,*secondvolumeName,*deviceName,*serialNumber,*partition;
		char fullpath[USB_PATH_SIZE],dupshare[USB_PATH_SIZE];
		char *sep, *share_name, *folderName, *readAccess, *writeAccess;

		sep = "*\n";
		for(j=0; ;j++){
			sprintf(name,SHARE_FILE_INFO"%d",j);
			val = config_get(name);
			if(*val == '\0')
				break;

			strcpy(oneline,val);

			share_name = strtok(oneline, sep);/* share name */
			folderName = strtok(NULL, sep);	/* folder name */
			readAccess = strtok(NULL, sep);	/* readAccess*/
			writeAccess = strtok(NULL, sep);	/* writeAccess */
			volumeName = strtok(NULL, sep);   /* volumeName*/
			deviceName = strtok(NULL, sep);    /* deviceName */
			serialNumber = strtok(NULL, sep);  /* serialNumber */
			partition = strtok(NULL, sep);     /* partition  */
	
			if (share_name == NULL ||folderName == NULL ||
			          readAccess == NULL ||writeAccess == NULL ||
			          volumeName == NULL || deviceName == NULL )
					continue;			

			if( strcmp(volumeName,diskinfo->vol_name) || strcmp(deviceName,diskinfo->vendor)||
				strcmp(serialNumber,diskinfo->serialNumber) || strcmp(partition,diskinfo->partition))
					continue;

                        if (duplicate_share_name(share_name, &share_lists)) {
                                snprintf(dupshare, sizeof(dupshare), "%s(%c)", share_name, diskinfo->label);
                                share_name = dupshare;
                        }
                        add_share_info_list(share_name, &share_lists);

			snprintf(fullpath, sizeof(fullpath), "/mnt/%s%s", diskinfo->name, folderName);
	
			readAccess = user_name(readAccess);
			writeAccess = user_name(writeAccess);
				
			USB_DEBUGP("[USB-SMB]: SMBInfo %s Folder:%s Reader:%s Writer: %s\n",
							share_name, folderName, readAccess, writeAccess);

			add_smbd_share_info(fp, share_name, readAccess, writeAccess, fullpath);

                        if (diskname != NULL && strncmp(diskinfo->name, diskname, 3) == 0)
                                no_shareinfo = 0;
		}
#endif
	}

	if (no_shareinfo && diskname != NULL) {
		USB_DEBUGP("[USB-SMB]: Disk %s has no share information!\n", diskname);
		int i;
		//FILE *tmpfp;
		char value[256],name[64],*val;
		// char tmp_path[128], share_name[64], folder_path[64];
		char share_name[64], folder_path[64];

		list_for_each(pos, &disk_lists) {
			diskinfo = list_entry(pos, struct disk_partition_info, list);
			if (!diskinfo->mounted ||strncmp(diskinfo->name, diskname, 3))
				continue;
#if 0 //save in disk
			/* Creat a temporary file to save changed share info */
			snprintf(tmp_path, sizeof(tmp_path), "/mnt/%s/.usb_xxx_share", diskinfo->name);
			tmpfp = fopen(tmp_path, "w");
			if (tmpfp == NULL)
				continue;

			snprintf(infopath, sizeof(infopath), "/mnt/%s/" USB_INFO_FILE, diskinfo->name);
			if (!valid_shareinfo_file(infopath)) {
				USB_DEBUGP("[USB-SMB]: The USB share file %s is NOT valid or NOT exists!\n", infopath);
				creat_new_shareinfo_file(infopath);
			}
			infofp = fopen(infopath, "r");
			if (infofp == NULL) {
				fclose(tmpfp);
				continue;
			}

			if (diskinfo->label == 'U')	
				snprintf(share_name, sizeof(share_name), "USB_Storage");
			else
				snprintf(share_name, sizeof(share_name),"%c_Drive", diskinfo->label);

			while (fgets(buf, sizeof(buf), infofp)){
				if (strstr(buf, USB_SESSION)) {
					fputs(buf, tmpfp);
					fprintf(tmpfp, "%s*/*0*0\n", share_name);
				} else {
					fputs(buf, tmpfp);
				}
			}

			fclose(infofp);
			fclose(tmpfp);
			unlink(infopath);
			rename(tmp_path, infopath);
#else //save in config 
                        if (diskinfo->label == 'U')
                                snprintf(share_name, sizeof(share_name), "USB_Storage");
                        else
                                snprintf(share_name, sizeof(share_name),"%c_Drive", diskinfo->label);

			snprintf(value,sizeof(value),"%s*/*0*0*%s*%s",share_name,diskinfo->vol_name,diskinfo->vendor);

			for(i=0;i<100 ;i++){
				sprintf(name,SHARE_FILE_INFO"%d", i);
				val = config_get(name);
				if(*val == '\0')
					break;
			}
			config_set(name,value);

#endif 
			snprintf(folder_path, sizeof(folder_path), "/mnt/%s/", diskinfo->name);
			add_smbd_share_info(fp, share_name, USER_GUEST, USER_GUEST, folder_path);
		}
	}

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
}

void cleanup(int signal)
{
	printf("Try to recover from endless waiting.\n");
	reload_services();
	unlink(TMP_SAMBA_LOCK);
	exit(1);
}

int check_samba_locked(void)
{
	return (!access(TMP_SAMBA_LOCK, F_OK));
}

int main(int argc, char**argv)
{
	FILE *fp, *filp;
	char *diskname = NULL;
	struct timeval currenttime, newtime;

	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);

	gettimeofday(&currenttime, NULL);

	while(check_samba_locked()) {
		gettimeofday(&newtime, NULL);
		/* the longest waiting time is 30s, avoid endless waiting */
		if ((newtime.tv_sec - currenttime.tv_sec)>30)  
			cleanup(0);
		sleep(1);
	}

	/* create lock file */
	filp = fopen(TMP_SAMBA_LOCK, "w+");
	if (filp)
		fclose(filp);
	else {
		perror("error when creating samba_lock file!\n");
		return 1;
	}

	int ret;
	char cmd[128];
	sprintf(cmd, "/bin/grep br0 %s > /dev/zero 2>&1", USB_SMB_CONF);
	ret = system(cmd);
	if ((ret==0 && config_match("usb_enableNet", "1")) ||
            (ret!=0 && !config_match("usb_enableNet", "1")))
		system("killall smbd");

	fp = fopen(USB_SMB_CONF, "w");
	if (fp == NULL)
		goto unlock;

	if (argc == 2 && strlen(argv[1]) == 3 && strncmp(argv[1], "sd", 2) == 0)
		diskname = argv[1];	/* sd[a-z] */

	add_smbd_global(fp);
	load_share_info(fp, diskname);

	fclose(fp);

	reload_services();

unlock:	
	unlink(TMP_SAMBA_LOCK);
	return 0;
}

#else

#define SMB_CONF "/etc/samba/smb.conf"
#define TMP_SAMBA_LOCK  "/tmp/tmp_samba_lock"
#define MAX_CHAR  1024

extern char *config_get(char* name);
char *saveptr1, *saveptr2;

struct partition {
  char share[MAX_CHAR];
  struct partition * next;
};

FILE * conf_fp;

int add_smb_global(void)
{
  char *br0_ip = config_get("lan_ipaddr");
  char *br0_mask;

  conf_fp=fopen(SMB_CONF, "w");
  if (conf_fp == NULL) {
    printf("Error opening %s\n", SMB_CONF);
    return 1;
  }
  
  fprintf(conf_fp, "[global]\n");
  fprintf(conf_fp, "  interfaces=br0 %s", br0_ip);
  
  br0_mask=config_get("lan_netmask");
  
  fprintf(conf_fp, "/%s\n", br0_mask);
  fprintf(conf_fp, "  bind interfaces only = yes\n");
  fprintf(conf_fp, "  workgroup = Workgroup\n");
  fprintf(conf_fp, "  server string = NETGEAR WNDR4500v3\n");
  fprintf(conf_fp, "  socket options = TCP_NODELAY\n");
  fprintf(conf_fp, "  security = user\n");
  fprintf(conf_fp, "  host msdfs = no\n");  /* fix bug 12866 */
  fprintf(conf_fp, "  hostname lookups = no\n");
  fprintf(conf_fp, "  load printers = no\n");
  fprintf(conf_fp, "  printing = bsd\n");
  fprintf(conf_fp, "  printcap name = /dev/null\n");
  fprintf(conf_fp, "  disable spoolss = yes\n");
  fprintf(conf_fp, "  guest account=guest\n");
  fprintf(conf_fp, "  encrypt passwords = yes\n");
  fprintf(conf_fp, "  name resolve order = lmhosts hosts bcast\n");
  fprintf(conf_fp, "  smb passwd file = /etc/samba/smbpasswd\n");
  fprintf(conf_fp, "  display charset = UTF-8\n");
  fprintf(conf_fp, "  unix charset = UTF-8\n");
  fprintf(conf_fp, "  dos charset = UTF-8\n");
  fprintf(conf_fp, "  map to guest = bad user\n");
  fprintf(conf_fp, "\n");

  fclose(conf_fp);
  return 0;
}


int check_writable(char* group)
{
  char* admin_group, *p, *group_des, *group_rights;
  char params[MAX_CHAR];
  int i=1, wcount=0;

  if (strcmp(group, "admin")==0) {
    return 1;
  }

  while (1) {
    sprintf(params, "admin_group%d", i);
    i++;
    admin_group=config_get(params);
    if (*admin_group=='\0')
      break;

    p=strtok_r(admin_group, " ", &saveptr2);
    if (strcmp(p, group))
      continue;

    if (p)
      group_des=strtok_r(NULL, " ", &saveptr2);

    if (group_des == NULL) {
      printf("bad configuration found\n");
      continue;
    }

    group_rights=strtok_r(NULL, " ", &saveptr2);
    
    if (!group_rights) { /* no group description */
      group_rights=group_des; 
    }
    
    if (strcmp(group_rights, "R&W")==0)
      wcount++;
      
  }

  return wcount;
}

void process_group(char* groups)
{
  char * grp;
  int grp_count=0;
  char valid_users[MAX_CHAR];
  char write_list[MAX_CHAR];
  int wgrp_count=0;

  memset((void*)valid_users, '\0', MAX_CHAR);
  memset((void*)write_list, '\0', MAX_CHAR);

  grp = strtok_r(groups, ",", &saveptr1);
  if (grp) {
    fprintf(conf_fp, "  valid users=");
    grp_count++;
  }

  while(grp) {
    if (grp_count > 1)
      fprintf(conf_fp, ",");

    if (check_writable(grp)) {
      wgrp_count++;
      if (wgrp_count == 1) {
	strcpy(write_list, "  write list=@");
	strcat(write_list, grp);
      }  else {
	strcat(write_list, ",@");
	strcat(write_list, grp);
      }
    } 
    
    fprintf(conf_fp, "@%s", grp);
    grp_count++;
    grp=strtok_r(NULL, ",", &saveptr1);
  }

  fprintf(conf_fp, "\n");

  if (wgrp_count)
    fprintf(conf_fp, "%s\n", write_list);  
}

void reload_samba(void)
{
  int ret;

  ret = system("pidof smbd > /dev/zero 2>&1");
  if (ret == 0) {
    system("killall -SIGHUP smbd");
  } else {
    system("smbd -D");
  }

  /* Tell 'uhttpd' to update HTTP share information */
  system("killall -SIGUSR1 uhttpd");
}

struct partition * alloc_partition_info()
{
  struct partition *pp;

  pp=(struct partition *)malloc(sizeof(struct partition));
  return pp;
}

/*  
 *  function compliment convert "&nbsp" to ' ' 
 *
 */

void charToSpace(char * orig , char * dest)
{
	char * porig = orig ;
	char * pdest = dest ;
	char ch ,tmp[7] , * ptmp;

	while( * porig != '\0'){
		ch = *porig ;
		if( ch == '&' ){
			ptmp = porig ;
			tmp[0] = *ptmp ++;
			tmp[1] = *ptmp ++;
			tmp[2] = *ptmp ++;
			tmp[3] = *ptmp ++;
			tmp[4] = *ptmp ++;
			tmp[5] = *ptmp ++;
			tmp[6] = '\0';
			if( strcmp (tmp , "&nbsp;") == 0){
				porig += 6;
				*pdest++ = ' ';
			}
			else
				*pdest ++ = *porig ++;
		}
		else
			*pdest ++ = *porig ++;
	}

	*pdest = '\0' ;
}

void check_add_shares(char* devicename)
{
  FILE * fp;
  char line[MAX_CHAR], true_display[MAX_CHAR];
  char fullfp[MAX_CHAR];
  char * display_name, *partition, *location, *groups;
  char * saveptr_main, *saveptr_mounts;
  char * p;
  struct partition *pp, *pp2, *head;

  conf_fp=fopen(SMB_CONF, "a");
  if (conf_fp == NULL) {
    printf("Error opening %s\n", SMB_CONF);
    return;
  }

  head = NULL;
  pp = NULL;

  if (strcmp(devicename, "all") == 0) { /* check all mounted partitions */
    fp = fopen("/proc/mounts", "r");
    if (fp == NULL) {
      printf(" Can't open /proc/mounts.\n");
      goto check_out;
    }

    while (fgets(line, MAX_CHAR, fp)) {
      p = strtok_r(line, " ", &saveptr_mounts);
      if (strncmp(p, "/dev/sd", 7))
	continue;
      
      if (p)
	p=strtok_r(NULL, " ", &saveptr_mounts);
      
      if (p == NULL)
	continue;

      pp2 = alloc_partition_info();
      if (pp2 == NULL) {
	printf("alloc buffer failed.\n");
	goto check_out;
      }
      if (head == NULL) {
	head = pp2;
	pp = pp2;
      } else {
	pp->next=pp2;
	pp = pp2;
      }

      memset((void*)pp2, 0, sizeof(struct partition));      
      strcpy(pp->share, p);
    }
    fclose(fp);

  } else { /* just check one partition --- devicename */
    head = alloc_partition_info();
    if (head == NULL) {
      printf("alloc buffer failed.\n");
      goto check_out;
    } 
    memset((void*)head, 0, sizeof(struct partition));
    sprintf(head->share, "/tmp/mnt/%s", devicename);
  }
  
  pp=head;
  
  while(pp) {
    sprintf(fullfp, "%s/.wndr4500v3_disk_share_info", pp->share);
    if (access(fullfp, F_OK) == 0) {
      fp = fopen(fullfp, "r");
      while(fgets(line, MAX_CHAR, fp)) {
	if (line[strlen(line)-1] == 0xa) /* delete the last 0xA */
	  line[strlen(line)-1]='\0';

	display_name = strtok_r(line, " ", &saveptr_main);
	if (display_name)
	  partition = strtok_r(NULL, " ", &saveptr_main);
	if (partition){
	     char tmp[512];
	     location = strtok_r(NULL, " ", &saveptr_main);
             strcpy(tmp,location);
	     charToSpace(tmp,location);
	}
	if (groups)
	  groups = strtok_r(NULL, " ", &saveptr_main);
	
	if (groups == NULL) {
	  printf("Bad format in %s \n", fullfp);
	  continue;
	}
	  
	p= strstr(display_name, "disk_sharefolder");
	if (!p) {
	  printf("Bad format in %s \n", fullfp);
	  continue;
	}

	while ((*p != '=')&&(*p))
	  p++;
	if ((*p) && (*p++)) {
	  strcpy(true_display, p);
	}
	else {
	  printf("Bad format in %s \n", fullfp);
	  continue;
	}  
	
	fprintf(conf_fp, "[%s]\n", true_display);
	fprintf(conf_fp, "  path=%s%s\n", pp->share, location);
	fprintf(conf_fp, "  read only=yes\n");
	process_group(groups);
	fprintf(conf_fp, "  browsable=yes\n");
	fprintf(conf_fp, "  public=no\n\n");
      }
    }
    pp=pp->next;
  }  
 check_out:
  fclose(conf_fp);
}

int check_samba_locked(void)
{
  return (!access(TMP_SAMBA_LOCK, F_OK));
}

void usage(void)
{
  printf("Usage:\n");
  printf("update_smb [-a devicename]\n");
}

void cleanup(int signal)
{
  printf("Try to recover from endless waiting.\n");
  reload_samba();
  unlink(TMP_SAMBA_LOCK);
  exit(1);
}

int main(int argc, char** argv)
{
  FILE * filp;
  int action, ret=1;
  char oneline[MAX_CHAR];
  struct timeval currenttime, newtime;

  conf_fp=NULL;

  signal(SIGINT, cleanup);
  signal(SIGTERM, cleanup);

  gettimeofday(&currenttime, NULL);

  while(check_samba_locked()) {
    gettimeofday(&newtime, NULL);
    if ((newtime.tv_sec - currenttime.tv_sec)>30)  /* the longest waiting time is 30s, avoid endless waiting */
      cleanup(0);
    sleep(1);  
  }

  /* create lock file */
  filp=fopen(TMP_SAMBA_LOCK, "w+");
  if (filp)
    fclose(filp);
  else {
    perror("error when creating samba_lock file!\n");
    return 1;
  }

  if (argc == 1) {
    if (add_smb_global())
      goto out;
    check_add_shares("all");
    ret = 0;
    goto out;
  } 

  while ((action=getopt(argc, argv, "a:h")) != -1) {
    switch (action) {
    case 'a':
      conf_fp=fopen(SMB_CONF, "r");
      if (conf_fp == NULL) {
	printf("Error opening %s\n", SMB_CONF);
	goto out;
      }
      if (fgets(oneline, MAX_CHAR, conf_fp)) {
	if (strstr(oneline, "[global]") == NULL) 
	  if (add_smb_global())
	    goto out;
      }	
      fclose(conf_fp);
      check_add_shares(optarg);
      ret = 0;
      break;
    case 'h':
      usage();
      ret = 1;
      break;
    default:
      usage();
      ret = 1;
      break;
    }
  } 
 out:
  if (ret == 0) 
    reload_samba();
  unlink(TMP_SAMBA_LOCK);
  return ret;
}

#endif

