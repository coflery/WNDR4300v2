/*
 * mtd - simple memory technology device manipulation tool
 *
 * Copyright (C) 2005 Waldemar Brodkorb <wbx@dass-it.de>,
 *	                  Felix Fietkau <nbd@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The code is based on the linux-mtd examples.
 */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "config.h"

static void update_to_file(int potval)
{
	FILE *fp;

	if (potval > POT_MAX_VALUE)
		potval = POT_MAX_VALUE;

	fp = fopen(POT_FILENAME, "w");
	if (fp == NULL) {
		printf("Can't open " POT_FILENAME "!\n");
		return;
	}

	fprintf(fp, "%d", potval);
	fclose(fp);
}

/*
  * NETGEAR SPEC v1.6
  * ------------------------------------------------------------------------------------
  * In order to minimize the risk of corrupting the flash, a simple bit write operation (not erase / write) 
  * should be used to change the POT value counter. For example, if the original state of the flash is all 
  * zeroes, whenever we increment the POT counter / value, we simply set the lowest bit to 1. The next 
  * increment will set the next bit to 1. Therefore, we will need 4320 bits or 540 bytes to store up to three 
  * days of POT value. Since we are not using the complete erase and write operation, there is less chance 
  * of flash corruption.
  * ------------------------------------------------------------------------------------
  *
  * And the DUT's original state of MTD flash is all ones: 0xFFFFFFFF, 0xFFFFFFFF ....
  */
int main(int argc, char **argv)
{
	int devfd;
	int pot_value;
	int word_len;
	int bitone_len;
	struct timeval timo;
	unsigned int word_value;

	printf("POT is Running...\n");

	daemon(1, 1);

	devfd = open(POT_MTD,O_RDWR);
	if (devfd < 0) {
		printf("Open mtd POT error!\n");
		return -1;
	}

	/* 
	  * count the number of word (4 bytes), not 0x00000000 at the head, and read the 
	  * first word which isn't ZERO.
	  */
	word_len = -1;
	do{
		word_len++;
		read(devfd, &word_value, sizeof(word_value));
	} while (word_value == 0x00000000);

	/* point to first word which isn't 0x00000000 */
	lseek(devfd, -4, SEEK_CUR);

	if (word_value == 0xFFFFFFFF)
		bitone_len = 32;
	else {
		bitone_len = 0;
		do {
			bitone_len++;
			word_value = word_value >> 1;
		} while (word_value);
	}

	pot_value = (word_len * 32) + (32 - bitone_len);
	if (pot_value >= POT_MAX_VALUE)
		goto fin;

	update_to_file(pot_value);
	for (;;) {
		timo.tv_sec = POT_RESOLUTION  * 60;
		timo.tv_usec = 1;
		select(1, NULL, NULL, NULL, &timo);

		/* need verify the result of read & write operation ?_? */
		read(devfd, &word_value, sizeof(word_value));
		lseek(devfd, -4, SEEK_CUR);
		word_value = word_value >> 1;
		write(devfd, &word_value, sizeof(word_value));

		if (word_value != 0x00000000)
			lseek(devfd, -4, SEEK_CUR);

		pot_value++;
		if (pot_value >= POT_MAX_VALUE)
			goto fin;
		update_to_file(pot_value);
	}

fin:
	close(devfd);
	update_to_file(pot_value);
	printf("POT is Finished!!!\n");
	return 0;
}

