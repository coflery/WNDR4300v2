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
#include <asm/serial.h>

#include <linux/tty.h>
#include <linux/time.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/miscdevice.h>

#include <asm/mach-atheros/atheros.h>
#include <asm/delay.h>

#define ATH_DEFAULT_WD_TMO	(20ul * USEC_PER_SEC)

#define FACTORY_RESET		0x89ABCDEF

#define ATH_GPIO_RESET	21

#ifdef ATH_WDT_TEST_CODE
#	define wddbg printk
#else
#	define wddbg(junk, ...)
#endif /* ATH_WDT_TEST_CODE 8 */

extern uint32_t ath_ahb_freq;

typedef struct {
	int open:1, can_close:1, tmo, action;
	wait_queue_head_t wq;
} ath_wdt_t;

static ath_wdt_t wdt_softc_array;

static ath_wdt_t *wdt = &wdt_softc_array;

irqreturn_t ath_wdt_isr(int, void *);

#ifdef ATH_WDT_TEST_CODE
/* Return the value present in the watchdog register */
static inline uint32_t ath_get_wd_timer(void)
{
	uint32_t val;

	val = (uint32_t) ath_reg_rd(ATH_WATCHDOG_TMR);
	val = (val * USEC_PER_SEC) / ath_ahb_freq;

	return val;
}
#endif /* ATH_WDT_TEST_CODE */

/* Set the timeout value in the watchdog register */
void ath_set_wd_timer(uint32_t usec /* micro seconds */)
{
#if defined(CONFIG_MACH_QCA956x)
        uint32_t vTmpusec = 0;
        uint32_t vCnt = 0;
#endif
#if defined(CONFIG_MACH_AR934x) || defined(CONFIG_MACH_QCA955x) || \
    defined(CONFIG_MACH_QCA953x) || defined(CONFIG_MACH_QCA956x)
	usec = usec * (ath_ref_freq / USEC_PER_SEC);
#else
	usec = usec * (ath_ahb_freq / USEC_PER_SEC);
#endif

	wddbg("%s: 0x%08x\n", __func__, usec);

#if defined(CONFIG_MACH_QCA956x)
        //EV[131847], wait WDT timer write in( wait limit time )
        vTmpusec = ath_reg_rd(ATH_WATCHDOG_TMR);
        ath_reg_wr(ATH_WATCHDOG_TMR, usec);

        while(ath_reg_rd(ATH_WATCHDOG_TMR) <= vTmpusec)
        {
            vCnt++;
            if(vCnt >= 100)
            {
                 printk("%s: Write WDT Timer fail !\n",__func__);
                 break;
            }
        }
#else
	ath_reg_wr(ATH_WATCHDOG_TMR, usec);
#endif
		
}

int ath_set_wd_timer_action(uint32_t val)
{
	if (val & ~ATH_WD_ACT_MASK) {
		return EINVAL;
	}

	wdt->action = val;

	/*
	 * bits  : 31 30 - 2 0-1
	 * access: RO  rsvd  Action
	 *
	 * Since bit 31 is read only and rest of the bits
	 * are zero, don't have to do a read-modify-write
	 */
	ath_reg_wr(ATH_WATCHDOG_TMR_CONTROL, val);
	return 0;
}

#ifdef ATH_WDT_TEST_CODE
static inline uint32_t ath_get_wd_timer_action(void)
{
	return (uint32_t) (ath_reg_rd(ATH_WATCHDOG_TMR_CONTROL) &
			   ATH_WD_ACT_MASK);
}

static inline uint32_t ath_get_wd_timer_last(void)
{
	return ((uint32_t) (ath_reg_rd(ATH_WATCHDOG_TMR_CONTROL) &
			    ATH_WD_LAST_MASK) >> ATH_WD_LAST_SHIFT);
}
#endif /* ATH_WDT_TEST_CODE */

#ifndef CONFIG_WATCHDOG_RESET_TIMER
irqreturn_t ath_wdt_isr(int cpl, void *dev_id)
{
	unsigned delay;
	extern int ath_gpio_in_val(int);

#define UDELAY_COUNT 4000

	wddbg("%s: invoked\n", __func__);

	for (delay = UDELAY_COUNT; delay; delay--) {
		if (ath_gpio_in_val(ATH_GPIO_RESET)) {
			break;
		}
		udelay(1000);
	}

	wddbg("%s: %d", __func__, delay);

	if (!delay) {
		wake_up(&wdt->wq);
	} else {
		extern void ath_restart(char *);
		ath_restart(NULL);
	}
	return IRQ_HANDLED;
}
#endif /* CONFIG_WATCHDOG_RESET_TIMER */

static int athwdt_open(struct inode *inode, struct file *file)
{
	wddbg("%s: called\n", __func__);

	if (MINOR(inode->i_rdev) != WATCHDOG_MINOR) {
		return -ENODEV;
	}

	if (wdt->open) {
		return -EBUSY;
	}

	wdt->open = 1;
	wdt->tmo = ATH_DEFAULT_WD_TMO;
	wdt->action = ATH_WD_ACT_NONE;
	wdt->can_close = 0;
	init_waitqueue_head(&wdt->wq);

	ath_set_wd_timer(wdt->tmo);
	ath_set_wd_timer_action(ATH_WD_ACT_NONE);

	return nonseekable_open(inode, file);
}

static int athwdt_close(struct inode *inode, struct file *file)
{
	wddbg("%s: called\n", __func__);

	if (MINOR(inode->i_rdev) != WATCHDOG_MINOR) {
		return -ENODEV;
	}

	if (!wdt->can_close) {
		wddbg("%s: clearing action\n", __func__);
		ath_set_wd_timer_action(ATH_WD_ACT_NONE);
	} else {
		wddbg("%s: not clearing action\n", __func__);
	}
	wdt->open = 0;
	return 0;
}

static ssize_t
athwdt_read(struct file *file, char *buf, size_t count, loff_t * ppos)
{
	wddbg("%s: called\n", __func__);

	return -ENOTSUPP;
}

static int
athwdt_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int ret = 0;

	wddbg("%s: called\n", __func__);

	switch (cmd) {
	case FACTORY_RESET:
		wddbg("%s: intr action\n", __func__);

		if ((ret = request_irq(ATH_MISC_IRQ_WATCHDOG,
				       ath_wdt_isr,
				       0, "Watchdog Timer", wdt))) {
			wddbg("%s: request_irq %d\n", __func__, ret);
			return ret;
		}

		ath_set_wd_timer_action(ATH_WD_ACT_GP_INTR);
		sleep_on(&wdt->wq);
		free_irq(ATH_MISC_IRQ_WATCHDOG, wdt);
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

static ssize_t
athwdt_write(struct file *file, const char *buf, size_t count, loff_t * ppos)
{
	int i;
	char c;

	wddbg("%s: called\n", __func__);

	for (i = 0; i != count; i++) {
		if (get_user(c, buf + i)) {
			return -EFAULT;
		}

		if (c == 'V') {
			wdt->can_close = 1;
			break;
		}
	}

	if (i) {
		ath_set_wd_timer(wdt->tmo);
		return 1;
	}

	return 0;
}

static struct file_operations athwdt_fops = {
      read:athwdt_read,
      write:athwdt_write,
      ioctl:athwdt_ioctl,
      open:athwdt_open,
      release:athwdt_close
};

static struct miscdevice athwdt_miscdev = {
	WATCHDOG_MINOR,
	"watchdog",
	&athwdt_fops
};

#ifdef CONFIG_WATCHDOG_RESET_TIMER
extern int athwdt_init(void);
#else
int __init athwdt_init(void)
{
	int		ret;
	uint32_t	*sram = KSEG1ADDR(ATH_SRAM_BASE + 0x934);

	extern void ath_gpio_config_input(int);
#ifdef CONFIG_MACH_AR934x
	extern void athwdt_timer_init(void);
#endif

	printk("%s: Registering WDT ", __func__);
	if ((ret = misc_register(&athwdt_miscdev))) {
		printk("failed %d\n", ret);
		return ret;
	} else {
		printk("success\n");
	}

	if (ath_reg_rd(RST_WATCHDOG_TIMER_CONTROL_ADDRESS) &
		RST_WATCHDOG_TIMER_CONTROL_LAST_MASK) {
		*sram = (*sram + 1);
		printk("Watchdog Reset count: %u\n", *sram);
	} else {
		*sram = 0;
	}


#ifdef CONFIG_MACH_AR934x
	athwdt_timer_init();
#endif
	ath_gpio_config_input(ATH_GPIO_RESET);

	return 0;
}
#endif /* CONFIG_WATCHDOG_RESET_TIMER */

late_initcall(athwdt_init);
