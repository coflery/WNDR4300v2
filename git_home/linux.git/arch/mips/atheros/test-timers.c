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

#include <linux/console.h>
#include <linux/proc_fs.h>
#include <asm/serial.h>

#include <linux/tty.h>
#include <linux/time.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/miscdevice.h>
#include <linux/ctype.h>

#include <asm/mach-atheros/atheros.h>
#include <asm/delay.h>

#define ATH_DEFAULT_WD_TMO	(20ul * USEC_PER_SEC)

#define wddbg(junk, ...)

extern uint32_t ath_ahb_freq;

#define ATH_TEST_TIMER_STR_MAX		10
char wdtbuf[ATH_TEST_TIMER_STR_MAX], timerbuf[ATH_TEST_TIMER_STR_MAX];

char *
ath_str_to_u(const char *s, uint32_t *u)
{
	int	i;

	*u = 0;
	for (i = 0; isdigit(s[i]) && i < ATH_TEST_TIMER_STR_MAX; i ++) {
		*u = (*u * 10) + (s[i] - '0');
	}
	return s + i;
}

int ath_wdt_read(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	strncpy(buf, wdtbuf, sizeof(wdtbuf));
	return strlen(wdtbuf);
}

int ath_wdt_write(struct file *file, const char *buf, unsigned long count, void *data)
{
	uint32_t	tmp;
	const char	*p;

	strncpy(wdtbuf, buf, sizeof(wdtbuf));

	ath_reg_wr(RST_WATCHDOG_TIMER_CONTROL_ADDRESS,
			RST_WATCHDOG_TIMER_CONTROL_ACTION_SET(ATH_WD_ACT_NONE));

	p = ath_str_to_u(buf, &tmp);

	if (tmp > (~0u / ath_ref_freq)) {
		printk("Value (%u) greater than max (%u)\n",
			tmp, (~0u / ath_ref_freq));
		return -EINVAL;
	}

	ath_reg_wr(RST_WATCHDOG_TIMER_ADDRESS, tmp * ath_ref_freq);

	switch (*p) {
		case 'n': tmp = ATH_WD_ACT_NMI; break;
		case 'i': tmp = ATH_WD_ACT_GP_INTR; break;
		case 'r': tmp = ATH_WD_ACT_RESET; break;
		case 'c': tmp = ATH_WD_ACT_NONE; break;
		default: return -EINVAL;
	}

	ath_reg_wr(RST_WATCHDOG_TIMER_CONTROL_ADDRESS,
			RST_WATCHDOG_TIMER_CONTROL_ACTION_SET(tmp));

	return strlen(wdtbuf);
}


int ath_timer_read(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	strncpy(buf, timerbuf, sizeof(timerbuf));
	return strlen(timerbuf);
}

irqreturn_t ath_timer_isr(int cpl, void *dev_id)
{

	ath_reg_rmw_clear(ATH_MISC_INT_STATUS,
			RST_MISC_INTERRUPT_MASK_TIMER2_MASK_MASK);

	printk("%s: invoked 0x%x 0x%x\n", __func__,
		ath_reg_rd(RST_GENERAL_TIMER2_RELOAD_ADDRESS),
		ath_reg_rd(RST_MISC_INTERRUPT_MASK_ADDRESS));

	return IRQ_HANDLED;
}

int ath_timer_write(struct file *file, const char *buf, unsigned long count, void *data)
{
	static int	ret = 1;
	uint32_t	tmp;

	strncpy(timerbuf, buf, sizeof(timerbuf));

	ath_str_to_u(buf, &tmp);
	if (tmp > (~0u / ath_ref_freq)) {
		printk("Value (%u) greater than max (%u)\n",
			tmp, (~0u / ath_ref_freq));
		return -EINVAL;
	}

	ath_reg_wr(RST_GENERAL_TIMER2_RELOAD_ADDRESS, tmp * ath_ref_freq);

	if (ret && (ret = request_irq(ATH_MISC_IRQ_TIMER2,
			       ath_timer_isr,
			       IRQF_DISABLED, "TEST: Timer", NULL))) {
		printk("%s: timer request_irq %d\n", __func__, ret);
	}

	ath_reg_rmw_set(RST_MISC_INTERRUPT_MASK_ADDRESS,
			RST_MISC_INTERRUPT_MASK_TIMER2_MASK_MASK);
	printk("%s: invoked 0x%x 0x%x\n", __func__,
		ath_reg_rd(RST_GENERAL_TIMER2_RELOAD_ADDRESS),
		ath_reg_rd(RST_MISC_INTERRUPT_MASK_ADDRESS));
	return strlen(timerbuf);
}

irqreturn_t ath_wdt_isr(int cpl, void *dev_id)
{
	printk("%s: invoked\n", __func__);

	return IRQ_HANDLED;
}

int ath_timer_init(void)
{
	struct proc_dir_entry	*proc;
	int	ret;

	proc = create_proc_entry("wdt", 0, NULL);
	if (!proc) {
		printk("create proc entry for wdt failed\n");
		return -EINVAL;
	}

	proc->read_proc = ath_wdt_read;
	proc->write_proc = ath_wdt_write;
	proc->size = sizeof(wdtbuf);
	printk("------------ /proc/wdt created\n");

	if ((ret = request_irq(ATH_MISC_IRQ_WATCHDOG,
			       ath_wdt_isr,
			       0, "TEST: WDT", NULL))) {
		printk("%s: wdt request_irq %d\n", __func__, ret);
		return ret;
	}

	proc = create_proc_entry("timer", 0, NULL);
	if (!proc) {
		printk("create proc entry for timer failed\n");
		return -EINVAL;
	}

	proc->read_proc = ath_timer_read;
	proc->write_proc = ath_timer_write;
	proc->size = sizeof(timerbuf);
	printk("------------ /proc/timer created\n");

	return 0;
}

late_initcall(ath_timer_init);
