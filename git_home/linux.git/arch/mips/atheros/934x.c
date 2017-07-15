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
#include <linux/kernel.h>
#include <asm/byteorder.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/bitops.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#ifdef CONFIG_SERIAL_8250
#include <linux/serial_8250.h>
#endif

#include <atheros.h>

#ifdef CONFIG_ATHRS_HW_CSUM
#include <asm/checksum.h>
csum_hw_ops *csum_hw = NULL;
EXPORT_SYMBOL(csum_hw);
#endif

#define ATH_PCI_EP_BASE_OFF 0x18127000
void ath_sys_frequency(void);
void UartInit(void);

void ath_dispatch_wlan_intr(void)
{
	u_int32_t int_status = ath_reg_rd(ATH_WMAC_INT_STATUS);
#ifdef CONFIG_PCI
	if (int_status & ATH_PCIE_INT_MASK) {
		do_IRQ(ATH_PCI_IRQ_DEV0);
	}
#endif
	if (int_status & ATH_AHB_WMAC_INT_MASK) {
		do_IRQ(ATH_CPU_IRQ_WLAN);
	}
}

unsigned int ath_slic_cntrl_rd(void)
{
	return ath_reg_rd(ATH_SLIC_CTRL);
}
void ath_slic_cntrl_wr(unsigned int val)
{
	ath_reg_wr(ATH_SLIC_CTRL, val);
}

void ath_spi_raw_output_u8(unsigned char val)
{
	int ii;
	unsigned int cs;

	cs = ath_reg_rd(ATH_SPI_WRITE) & ~(ATH_SPI_D0_HIGH | ATH_SPI_CLK_HIGH);
	for (ii = 7; ii >= 0; ii--) {
		unsigned char jj = (val >> ii) & 1;
		ath_reg_wr_nf(ATH_SPI_WRITE, cs | jj);
		ath_reg_wr_nf(ATH_SPI_WRITE, cs | jj | ATH_SPI_CLK_HIGH );
	}
}

unsigned int ath_spi_raw_input_u8(void)
{
	int ii;
	unsigned int cs;

	cs = ath_reg_rd(ATH_SPI_WRITE) & ~(ATH_SPI_D0_HIGH | ATH_SPI_CLK_HIGH);

	for (ii = 7; ii>=0 ; ii--) {
		ath_reg_wr_nf(ATH_SPI_WRITE, cs );
		ath_reg_wr_nf(ATH_SPI_WRITE, cs | ATH_SPI_CLK_HIGH );
	}

	return ath_reg_rd(ATH_SPI_RD_STATUS) & 0xff;
}

void UartInit(void)
{
	int freq, div;
	extern uint32_t serial_inited;

	ath_sys_frequency();

	freq = ath_uart_freq;

	div = freq / (ATH_CONSOLE_BAUD * 16);

	/* set DIAB bit */
	UART_WRITE(OFS_LINE_CONTROL, 0x80);

	/* set divisor */
	UART_WRITE(OFS_DIVISOR_LSB, (div & 0xff));
	UART_WRITE(OFS_DIVISOR_MSB, (div >> 8) & 0xff);

	// UART16550_WRITE(OFS_DIVISOR_LSB, 0x61);
	// UART16550_WRITE(OFS_DIVISOR_MSB, 0x03);

	/* clear DIAB bit */
	UART_WRITE(OFS_LINE_CONTROL, 0x00);

	/* set data format */
	UART_WRITE(OFS_DATA_FORMAT, 0x3);

	UART_WRITE(OFS_INTR_ENABLE, 0);

	serial_inited = 1;
}

void
ath_sys_frequency(void)
{
#if !defined(CONFIG_ATH_EMULATION)
	uint32_t pll, out_div, ref_div, nint, frac, clk_ctrl;
#endif
	uint32_t ref;

	if (ath_cpu_freq)
		return;

	if ((ath_reg_rd(ATH_BOOTSTRAP_REG) & ATH_REF_CLK_40)) {
		ref = (40 * 1000000);
	} else {
		ref = (25 * 1000000);
	}

	ath_uart_freq = ath_ref_freq = ref;

#ifdef CONFIG_ATH_EMULATION
	ath_cpu_freq = 80000000;
	ath_ddr_freq = 80000000;
	ath_ahb_freq = 40000000;
#else
	printk("%s: ", __func__);

	clk_ctrl = ath_reg_rd(ATH_DDR_CLK_CTRL);

	pll = ath_reg_rd(CPU_DPLL2_ADDRESS);
	if (CPU_DPLL2_LOCAL_PLL_GET(pll)) {
		out_div	= CPU_DPLL2_OUTDIV_GET(pll);

		pll = ath_reg_rd(CPU_DPLL_ADDRESS);
		nint = CPU_DPLL_NINT_GET(pll);
		frac = CPU_DPLL_NFRAC_GET(pll);
		ref_div = CPU_DPLL_REFDIV_GET(pll);
		pll = ref >> 18;
		frac	= frac * pll / ref_div;
		printk("cpu srif ");
	} else {
		pll = ath_reg_rd(ATH_PLL_CONFIG);
		out_div	= CPU_PLL_CONFIG_OUTDIV_GET(pll);
		ref_div	= CPU_PLL_CONFIG_REFDIV_GET(pll);
		nint	= CPU_PLL_CONFIG_NINT_GET(pll);
		frac	= CPU_PLL_CONFIG_NFRAC_GET(pll);
		pll = ref >> 6;
		frac	= frac * pll / ref_div;
		printk("cpu apb ");
	}
	ath_cpu_freq = (((nint * (ref / ref_div)) + frac) >> out_div) /
			(CPU_DDR_CLOCK_CONTROL_CPU_POST_DIV_GET(clk_ctrl) + 1);

	pll = ath_reg_rd(DDR_DPLL2_ADDRESS);
	if (DDR_DPLL2_LOCAL_PLL_GET(pll)) {
		out_div	= DDR_DPLL2_OUTDIV_GET(pll);

		pll = ath_reg_rd(DDR_DPLL_ADDRESS);
		nint = DDR_DPLL_NINT_GET(pll);
		frac = DDR_DPLL_NFRAC_GET(pll);
		ref_div = DDR_DPLL_REFDIV_GET(pll);
		pll = ref >> 18;
		frac	= frac * pll / ref_div;
		printk("ddr srif ");
	} else {
		pll = ath_reg_rd(ATH_DDR_PLL_CONFIG);
		out_div	= DDR_PLL_CONFIG_OUTDIV_GET(pll);
		ref_div	= DDR_PLL_CONFIG_REFDIV_GET(pll);
		nint	= DDR_PLL_CONFIG_NINT_GET(pll);
		frac	= DDR_PLL_CONFIG_NFRAC_GET(pll);
		pll = ref >> 10;
		frac	= frac * pll / ref_div;
		printk("ddr apb ");
	}
	ath_ddr_freq = (((nint * (ref / ref_div)) + frac) >> out_div) /
			(CPU_DDR_CLOCK_CONTROL_DDR_POST_DIV_GET(clk_ctrl) + 1);

	if (CPU_DDR_CLOCK_CONTROL_AHBCLK_FROM_DDRPLL_GET(clk_ctrl)) {
		ath_ahb_freq = ath_ddr_freq /
			(CPU_DDR_CLOCK_CONTROL_AHB_POST_DIV_GET(clk_ctrl) + 1);
	} else {
		ath_ahb_freq = ath_cpu_freq /
			(CPU_DDR_CLOCK_CONTROL_AHB_POST_DIV_GET(clk_ctrl) + 1);
	}
#endif
	printk("cpu %u ddr %u ahb %u\n", 
		ath_cpu_freq / 1000000,
		ath_ddr_freq / 1000000,
		ath_ahb_freq / 1000000);
}

/*
 * EHCI (USB full speed host controller)
 */
static struct resource ath_usb_ehci_resources[] = {
	[0] = {
		.start = ATH_USB_EHCI_BASE,
		.end = ATH_USB_EHCI_BASE + ATH_USB_WINDOW - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = ATH_CPU_IRQ_USB,
		.end = ATH_CPU_IRQ_USB,
		.flags = IORESOURCE_IRQ,
	},
};

/*
 * (PCI EP controller)
 */
static struct resource ath_pci_ep_resources[] = {
	[0] = {
		.start	= ATH_PCI_EP_BASE_OFF,
		.end	= ATH_PCI_EP_BASE_OFF + 0xdff - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= ATH_CPU_IRQ_PCI_EP,
		.end	= ATH_CPU_IRQ_PCI_EP,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 pci_ep_dmamask = ~(u32)0;
static struct platform_device ath_pci_ep_device = {
	.name				= "ath-pciep",
	.id				= 0,
	.dev = {
		.dma_mask		= &pci_ep_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources			= ARRAY_SIZE(ath_pci_ep_resources),
	.resource			= ath_pci_ep_resources,
};

/*
 * The dmamask must be set for EHCI to work
 */
static u64 ehci_dmamask = ~(u32) 0;

static struct platform_device ath_usb_ehci_device = {
	.name = "ath-ehci",
	.id = 0,
	.dev = {
		.dma_mask = &ehci_dmamask,
		.coherent_dma_mask = 0xffffffff,
		},
	.num_resources = ARRAY_SIZE(ath_usb_ehci_resources),
	.resource = ath_usb_ehci_resources,
};

#ifdef CONFIG_SERIAL_8250
static struct resource ath_uart_resources[] = {
	{
	 .start = ATH_UART_BASE,
	 .end = ATH_UART_BASE + 0x0fff,
	 .flags = IORESOURCE_MEM,
	 },
};

extern unsigned int ath_serial_in(int offset);
extern void ath_serial_out(int offset, int value);
unsigned int ath_plat_serial_in(struct uart_port *up, int offset)
{
	return ath_serial_in(offset);
}

void ath_plat_serial_out(struct uart_port *up, int offset, int value)
{
	ath_serial_out(offset, value);

}

static struct plat_serial8250_port ath_uart_data[] = {
	{
	 .mapbase = (u32) KSEG1ADDR(ATH_UART_BASE),
	 .membase = (void __iomem *)((u32) (KSEG1ADDR(ATH_UART_BASE))),
	 .irq = ATH_MISC_IRQ_UART,
	 .flags = (UPF_BOOT_AUTOCONF | UPF_SKIP_TEST),
	 .iotype = UPIO_MEM32,
	 .regshift = 2,
	 .uartclk = 0,		/* ath_ahb_freq, */
	},
	{},
};

static struct platform_device ath_uart = {
	.name = "serial8250",
	.id = 0,
	.dev.platform_data = ath_uart_data,
	.num_resources = 1,
	.resource = ath_uart_resources
};
#endif

static struct platform_device *ath_platform_devices[] __initdata = {
#ifdef CONFIG_SERIAL_8250
	&ath_uart,
#endif
	&ath_usb_ehci_device,
    &ath_pci_ep_device
};

extern void ath_serial_setup(void);
extern void ath_set_wd_timer(uint32_t usec /* micro seconds */);
extern int ath_set_wd_timer_action(uint32_t val);

void
ath_aphang_timer_fn(struct timer_list *timer)
{
	static int times;
	if (times == 0) {
		ath_set_wd_timer_action(ATH_WD_ACT_NONE);
		ath_set_wd_timer(5 * USEC_PER_SEC);
		ath_set_wd_timer_action(ATH_WD_ACT_RESET);
	}
	times = (times + 1) % HZ;
}

int ath_platform_init(void)
{
	int ret;

#ifdef CONFIG_SERIAL_8250
	ath_uart_data[0].uartclk = ath_uart_freq;
#endif

	ret = platform_add_devices(ath_platform_devices,
				ARRAY_SIZE(ath_platform_devices));

	if (ret < 0) {
		printk("%s: failed %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

#ifndef CONFIG_WATCHDOG_RESET_TIMER

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

static void athwdt_timer_action(unsigned long dummy);

static DEFINE_TIMER(athwdt_timer, athwdt_timer_action, 0, 0);
static struct proc_dir_entry *panic_entry;


#define ATHWDT_WATCHDOT_TIMER_DEFAULT 	(10000000)                          /* 10s */
#define ATHWDT_KERNEL_TIMER             (jiffies + ((1000) * HZ) / 1000)    /*  1s */

static void athwdt_timer_action(unsigned long dummy)
{
	ath_set_wd_timer(ATHWDT_WATCHDOT_TIMER_DEFAULT);
	ath_set_wd_timer_action(ATH_WD_ACT_RESET);
	mod_timer(&athwdt_timer, ATHWDT_KERNEL_TIMER);
}

int debug_panic_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	panic("debug panic!\n");
	return(count);
}

static int debug_panic_event(struct notifier_block *blk, unsigned long event, void *ptr)
{
#if 1
	void (*p)(void);

	p = 0xbd00009c;
	/* p = 0xbfc00000; */

	(*p)();
#endif

	printk(KERN_EMERG "%s:%d: here jiffies:%llu\n", __func__, __LINE__, jiffies);
	del_timer(&athwdt_timer);
	ath_set_wd_timer(1000000);
	ath_set_wd_timer_action(ATH_WD_ACT_RESET);
	return NOTIFY_DONE;
}

static struct notifier_block debug_panic_block = {
	debug_panic_event,
	NULL,
	INT_MAX /* try to do it first */
};


void athwdt_timer_init(void)
{
	uint32_t* p;
	printk(KERN_EMERG "%s:%d: here jiffies:%llu\n", __func__, __LINE__, jiffies);
	ath_set_wd_timer(ATHWDT_WATCHDOT_TIMER_DEFAULT * 6);
	ath_set_wd_timer_action(ATH_WD_ACT_RESET);
	mod_timer(&athwdt_timer, ATHWDT_KERNEL_TIMER);
	panic_entry = create_proc_entry("debug_panic", 0644, NULL);
	panic_entry->nlink = 1;
	panic_entry->write_proc = debug_panic_proc_write;

	atomic_notifier_chain_register(&panic_notifier_list,
	                &debug_panic_block);

}

#else
#include <linux/notifier.h>

static void athwdt_timer_action(unsigned long dummy);

static DEFINE_TIMER(athwdt_timer, athwdt_timer_action, 0, 0);

#define ATHWDT_WATCHDOT_TIMER_DEFAULT   (1500000)                          /* 1.5s */
#define ATHWDT_KERNEL_TIMER             (jiffies + ((1000) * HZ) / 1000)    /*  1s */

static void athwdt_timer_action(unsigned long dummy)
{
	ath_set_wd_timer(ATHWDT_WATCHDOT_TIMER_DEFAULT);
	mod_timer(&athwdt_timer, ATHWDT_KERNEL_TIMER);
}

static int athwdt_panic_event(struct notifier_block *blk, unsigned long event, void *ptr)
{
	del_timer(&athwdt_timer);
	ath_set_wd_timer(1);
	return NOTIFY_DONE;
}

static struct notifier_block athwdt_panic_block = {
	athwdt_panic_event,
	NULL,
	INT_MAX /* try to do it first */
};

irqreturn_t athwdt_isr(int cpl, void *dev_id)
{
	extern void ath_restart(char *);

	ath_restart(NULL);

        return IRQ_HANDLED;
}


int __init athwdt_init(void)
{
	int ret;

	ath_set_wd_timer(ATHWDT_WATCHDOT_TIMER_DEFAULT * 6);
	ath_set_wd_timer_action(ATH_WD_ACT_GP_INTR);
	mod_timer(&athwdt_timer, ATHWDT_KERNEL_TIMER);

	atomic_notifier_chain_register(&panic_notifier_list, &athwdt_panic_block);

	if ((ret = request_irq(ATH_MISC_IRQ_WATCHDOG,
	                       athwdt_isr,
	                       0, "Watchdog Timer", NULL))) {
		printk("%s: request_irq %d\n", __func__, ret);
		return ret;
	}


	return 0;
}

#endif /* CONFIG_WATCHDOG_RESET_TIMER */
arch_initcall(ath_platform_init);
