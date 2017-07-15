/* 
 * Copyright (c) 2014 Qualcomm Atheros, Inc.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 */

//#include <linux/config.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/resource.h>
#include <linux/proc_fs.h>

#include <linux/console.h>
#include <asm/serial.h>

#include <linux/tty.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>

#include <atheros.h>

/*  
 * hw_board_type 
 * 0 : WNDR4500v3
 * 1 : WNDR4300v2
 * 2 : WNDR4520
 * .........
 */
int hw_board_type = 0;
EXPORT_SYMBOL(hw_board_type);

static const char *hw_board_id[3] = {"WNDR4500v3", "WNDR4300v2", "WNDR4520" };

static int board_entry_read(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	return sprintf(buf, "%s\n", hw_board_id[hw_board_type]);
}

static int board_entry_write(struct file *file, const char *buf, unsigned long count, void *data)
{
	char new_board_id[16];

	memset(new_board_id, 0, 16);

	if (sscanf(buf, "%s", new_board_id) != 1)
		return -EINVAL;

	if (strcmp(new_board_id, hw_board_id[0]) == 0)
		hw_board_type = 0;
	else if (strcmp(new_board_id, hw_board_id[1]) == 0)
		hw_board_type = 1;
	else if (strcmp(new_board_id, hw_board_id[2]) == 0)
		hw_board_type = 2;

	return count;
}

int __init ath_simple_hw_board_type_init(void)
{
	struct proc_dir_entry *hw_board_entry = NULL;

	hw_board_entry = create_proc_entry ("hw_board_type", 0644, NULL);

	if (!hw_board_entry)
		return -ENOENT;

	hw_board_entry->write_proc = board_entry_write;
	hw_board_entry->read_proc = board_entry_read;

	return 0;
}

late_initcall(ath_simple_hw_board_type_init);
