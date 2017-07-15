#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define ROOTFS_LEN 0x200000

#define DYNAMIC_CRC_TABLE 1 

#ifdef DYNAMIC_CRC_TABLE
static int crc_table_empty = 1;
static unsigned long crc_table[256];
static void make_crc_table (void);


static void make_crc_table()
{
  unsigned long c;
  int n, k;
  unsigned long poly;
  static const unsigned char p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};
  poly = 0L;
  for (n = 0; n < sizeof(p)/sizeof(unsigned char); n++)
    poly |= 1L << (31 - p[n]);

  for ( n = 0;n < 256;n++)
  {
    c = (unsigned long )n;
    for(k = 0; k < 8;k++)
     c = c & 1 ? poly ^ (c >> 1):c >> 1;
    crc_table[n] = c;
  }
  crc_table_empty = 0;
}

#endif
#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);




unsigned long crc32(unsigned long crc,const unsigned char *buf ,unsigned int len)

{
  

#ifdef DYNAMIC_CRC_TABLE
	if (crc_table_empty)
	make_crc_table();
   #endif
	crc = crc ^ 0xffffffffL;
	while (len >=8)
	{
	DO8(buf);
	len -=8;
	}
	if(len) do {
	  DO1(buf);
	}while(--len);
	return crc ^ 0xffffffffL;
}

int main(int argc,char **argv)

{
	
	int fd;
	unsigned char *mybuf;
	unsigned long checksum;
	
	if(argc !=2){
	printf("a single argument is required !\n");
	exit(1);
	}
	
	if((fd = open(argv[1],O_RDONLY))== -1)
	{
	printf("Open %s Error!",argv[1]);
	exit(1);
	}	
	/* malloc memory for crc check*/
	if((mybuf = (unsigned char *)malloc(ROOTFS_LEN)) == NULL){
	printf("malloc error\n");
	exit(1);
	}
	/* initialize the malloced memory */
	memset(mybuf,0,ROOTFS_LEN);

	if(read(fd, mybuf,ROOTFS_LEN)== -1){
		printf("malloc error\n");
		exit(1);
	}

	/* crc checksum */
	checksum = crc32(0,mybuf,ROOTFS_LEN);
	fprintf(stderr, "crc32 checksum of %s is :%08x\n",argv[1],checksum);
	putchar(checksum >> 24);
	putchar((checksum % 0x1000000) >> 16);
	putchar((checksum % 0x10000) >> 8);
	putchar(checksum % 0x100);
	free(mybuf);
	close(fd);
	return 0;
}
