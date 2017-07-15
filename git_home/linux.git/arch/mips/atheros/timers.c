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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/io.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/ctype.h>

#include <atheros.h>

#define ADH_MAX_WRITE_SIZE	32
#define ADH_PROC_ENTRY		"ath-ddr-hog"

typedef struct {
	struct proc_dir_entry	*proc;
	uint8_t			buf[ADH_MAX_WRITE_SIZE];
} ath_ddr_hog_t;

static ath_ddr_hog_t	adh_softc, *adh = &adh_softc;

#ifdef CONFIG_ATH_TURN_ON_DDR_HOG

#define DDR_HOG_WRITE_REGION	((252 << 20) | KSEG0)
#define DDR_HOG_WRITE_END	(DDR_HOG_WRITE_REGION + 0x40)
#define DDR_START_ADDRESS	__stringify(DDR_HOG_WRITE_REGION)
#define DDR_END_ADDRESS		__stringify(DDR_HOG_WRITE_END)
#define DDR_INCR_COUNT		__stringify(32)
#define UNCACHED_SRAM_BASE	(ATH_SRAM_BASE | KSEG1)

noinline void
ath_ddr_hog_infinite(void)
{
	asm(
	"start_ddr_txns_infinite:\n"
	"	li	$t3,"	__stringify(RST_GENERAL_BASE) "\n"
	"	li	$t4,	0xffffffff\n"
	"	sw	$t4,	0xa0($t3)\n"
	"	li	$t3,"	DDR_START_ADDRESS "\n"
	"	li	$t4,"	DDR_END_ADDRESS "\n"
	"_outer_ddr_rw_loop_infinite:\n"
	"	ori	$t0,	$t3,	0\n"
	"	addiu	$t8,	$t0,	0x8000\n"
	"	li	$t1,	0x10000\n"
	"_inner_ddr_rw_loop_infinite:\n"
	"	sw	$t2,	0x0($t0)\n"
	"	sw	$t5,	0x2000($t0)\n"
	"	sw	$t6,	0x4000($t0)\n"
	"	sw	$t7,	0x6000($t0)\n"
	"	sw	$t2,	0x0($t8)\n"
	"	sw	$t5,	0x2000($t8)\n"
	"	sw	$t6,	0x4000($t8)\n"
	"	sw	$t7,	0x6000($t8)\n"
	"	b _inner_ddr_rw_loop_infinite\n"
	"	addiu	$t1,	$t1, -1\n"
	"	nop\n"
	"	bnez	$t1,	_inner_ddr_rw_loop_infinite\n"
	"	nop\n"
	"_changes_before_outer_loop_infinite:\n"
	"	addiu	$t3,	$t3,"	DDR_INCR_COUNT "\n"
	"	li	$t2,"	__stringify(UNCACHED_SRAM_BASE) "\n"
	"	li	$t3,"	__stringify(RST_GENERAL_BASE) "\n"
	"	lw	$t4,	0x9c($t3)\n"
	"	sw	$t4,	0x700($t2)\n"
	"	nop\n"
	"	nop\n");
}

noinline void
ath_ddr_hog_all_banks(void)
{
	asm(
	"start_ddr_txns_inf:\n"
	"	li	$t3,"	__stringify(RST_GENERAL_BASE) "\n"
	"	li	$t4,	0xffffffff\n"
	"	sw	$t4,	0xa0($t3)\n"
	"	li	$t3,"	DDR_START_ADDRESS "\n"
	"	li	$t4,"	DDR_END_ADDRESS "\n"
	"_outer_ddr_rw_loop_inf:\n"
	"	ori	$t0,	$t3,	0\n"
	"	addiu	$t8,	$t0,	0x8000\n"
	"	li	$t1,	0x10000\n"
	"_inner_ddr_rw_loop_inf:\n"
	"	sw	$t2,	0x0($t0)\n"
	"	sw	$t2,	0x200($t0)\n"
	"	sw	$t2,	0x400($t0)\n"
	"	sw	$t2,	0x600($t0)\n"

	"	sw	$t5,	0x2000($t0)\n"
	"	sw	$t5,	0x2200($t0)\n"
	"	sw	$t5,	0x2400($t0)\n"
	"	sw	$t5,	0x2600($t0)\n"

	"	sw	$t6,	0x4000($t0)\n"
	"	sw	$t6,	0x4200($t0)\n"
	"	sw	$t6,	0x4400($t0)\n"
	"	sw	$t6,	0x4600($t0)\n"

	"	sw	$t7,	0x6000($t0)\n"
	"	sw	$t7,	0x6200($t0)\n"
	"	sw	$t7,	0x6400($t0)\n"
	"	sw	$t7,	0x6600($t0)\n"

	"	sw	$t2,	0x0($t8)\n"
	"	sw	$t2,	0x200($t8)\n"
	"	sw	$t2,	0x400($t8)\n"
	"	sw	$t2,	0x600($t8)\n"

	"	sw	$t5,	0x2000($t8)\n"
	"	sw	$t5,	0x2200($t8)\n"
	"	sw	$t5,	0x2400($t8)\n"
	"	sw	$t5,	0x2600($t8)\n"

	"	sw	$t6,	0x4000($t8)\n"
	"	sw	$t6,	0x4200($t8)\n"
	"	sw	$t6,	0x4400($t8)\n"
	"	sw	$t6,	0x4600($t8)\n"

	"	sw	$t7,	0x6000($t8)\n"
	"	sw	$t7,	0x6200($t8)\n"
	"	sw	$t7,	0x6400($t8)\n"
	"	sw	$t7,	0x6600($t8)\n"

	//"	b _inner_ddr_rw_loop_inf\n"
	"	addiu	$t1,	$t1, -1\n"
	"	nop\n"
	"	bnez	$t1,	_inner_ddr_rw_loop_inf\n"
	"	nop\n"
	"_changes_before_outer_loop_inf:\n"
	"	addiu	$t3,	$t3,"	DDR_INCR_COUNT "\n"
	"	li	$t2,"	__stringify(UNCACHED_SRAM_BASE) "\n"
	"	li	$t3,"	__stringify(RST_GENERAL_BASE) "\n"
	"	lw	$t4,	0x9c($t3)\n"
	"	sw	$t4,	0x700($t2)\n"
	"	nop\n"
	"	nop\n");
}

noinline void
ath_ddr_hog(void)
{
	asm(
	"start_ddr_txns:\n"
	"	li	$t3,"	__stringify(RST_GENERAL_BASE) "\n"
	"	li	$t4,	0xffffffff\n"
	"	sw	$t4,	0xa0($t3)\n"
	"	li	$t3,"	DDR_START_ADDRESS "\n"
	"	li	$t4,"	DDR_END_ADDRESS "\n"
	"_outer_ddr_rw_loop:\n"
	"	ori	$t0,	$t3,	0\n"
	"	addiu	$t8,	$t0,	0x8000\n"
	"	li	$t1,	0x10000\n"
	"_inner_ddr_rw_loop:\n"
	"	sw	$t2,	0x0($t0)\n"
	"	sw	$t5,	0x2000($t0)\n"
	"	sw	$t6,	0x4000($t0)\n"
	"	sw	$t7,	0x6000($t0)\n"
	"	sw	$t2,	0x0($t8)\n"
	"	sw	$t5,	0x2000($t8)\n"
	"	sw	$t6,	0x4000($t8)\n"
	"	sw	$t7,	0x6000($t8)\n"
	//"	b _inner_ddr_rw_loop\n"
	"	addiu	$t1,	$t1, -1\n"
	"	nop\n"
	"	bnez	$t1,	_inner_ddr_rw_loop\n"
	"	nop\n"
	"_changes_before_outer_loop:\n"
	"	addiu	$t3,	$t3,"	DDR_INCR_COUNT "\n"
	"	li	$t2,"	__stringify(UNCACHED_SRAM_BASE) "\n"
	"	li	$t3,"	__stringify(RST_GENERAL_BASE) "\n"
	"	lw	$t4,	0x9c($t3)\n"
	"	sw	$t4,	0x700($t2)\n"
	"	nop\n"
	"	nop\n");
}

uint16_t upcase_table[131072] __attribute__ ((aligned (4096))),
	lowcase_table[131072] __attribute__ ((aligned (4096)));

#define SSVAL(a, b, c)	do { (*(((uint16_t *)(a)) + b)) = (uint16_t)(c); } while (0)
#define UCS2_CHAR(x)	((x) << 8)

void adh_samba_hog(void)
{
	int cntr = 0;

	ath_reg_wr(RST_GENERAL_TIMER3_RELOAD_ADDRESS, 0xffffffff);

	for (cntr = 0; cntr < 0x10000; cntr++) {
		uint16_t v;
		SSVAL(&v, 0, cntr);
		upcase_table[v] = cntr;
	}
	for (cntr = 0; cntr < 256; cntr++) {
		uint16_t v;
		SSVAL(&v, 0, UCS2_CHAR(cntr));
		upcase_table[v] = UCS2_CHAR(islower(cntr) ? toupper(cntr) : cntr);
	}

	for (cntr = 0; cntr < 0x10000; cntr++) {
		uint16_t v;
		SSVAL(&v, 0, cntr);
		lowcase_table[v] = cntr;
	}
	for (cntr = 0; cntr < 256; cntr++) {
		uint16_t v;
		SSVAL(&v, 0, UCS2_CHAR(cntr));
		lowcase_table[v] = UCS2_CHAR(isupper(cntr) ? tolower(cntr) : cntr);
	}

	cntr = ath_reg_rd(RST_GENERAL_TIMER3_ADDRESS);
	printk("Samba 0x%08x...\n", cntr);
}

int adh_read_proc(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	return sprintf(buf, "%u", adh->buf[0]);
}

int adh_write_proc(struct file *file, const char *buf, unsigned long count, void *data)

{
	if (count > ADH_MAX_WRITE_SIZE)
		count = ADH_MAX_WRITE_SIZE;

	if (copy_from_user(adh->buf, buf, count))
		return -EFAULT;

	switch (adh->buf[0]) {
	case 'i': ath_ddr_hog_infinite(); break;
	case 'd': ath_ddr_hog(); break;
	case 'a': ath_ddr_hog_all_banks(); break;
	case 's': adh_samba_hog(); break;
	default: printk("invalid input\n");
	}

	return count;
}

void adh_create_proc_entry(void)
{
	adh->proc = create_proc_entry(ADH_PROC_ENTRY, 0, NULL);
	if (!adh->proc) {
		printk("create_proc_entry failed: " ADH_PROC_ENTRY "\n");
		return;
	}
	adh->proc->read_proc = adh_read_proc;
	adh->proc->write_proc = adh_write_proc;
	adh->proc->size = ADH_MAX_WRITE_SIZE;
	printk("/proc/" ADH_PROC_ENTRY " created\n");
}
#endif	/* CONFIG_ATH_TURN_ON_DDR_HOG */

int ath_timer_init(void)
{
#if CONFIG_ATH_DDR_RELEASE_TIMER
	/*
	 * An ISR is not registered to handle interrupts generated by this
	 * timer. arch/mips/kernel/genex.S:handle_int handles this directly
	 */
	ath_reg_wr(RST_GENERAL_TIMER2_RELOAD_ADDRESS,
		CONFIG_ATH_DDR_RELEASE_TIMER * (ath_ref_freq / 1000000));
	ath_reg_rmw_set(RST_MISC_INTERRUPT_MASK_ADDRESS,
			RST_MISC_INTERRUPT_MASK_TIMER2_MASK_MASK);
#endif /* CONFIG_ATH_DDR_RELEASE_TIMER */

#ifdef CONFIG_ATH_TURN_ON_DDR_HOG
	adh_create_proc_entry();
#endif /* CONFIG_ATH_TURN_ON_DDR_HOG */

	return 0;
}

late_initcall(ath_timer_init);
