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
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#ifdef CONFIG_SERIAL_8250
#include <linux/serial_8250.h>
#endif

#include <atheros.h>

void ath_sys_frequency(void);
void UartInit(void);

void ath_dispatch_wlan_intr(void)
{
	u_int32_t int_status = ath_reg_rd(ATH_GLOBAL_INT_STATUS);

#ifdef CONFIG_PCI
	if (int_status & RST_GLOBAL_INTERRUPT_STATUS_PCIE_INT_MASK) {
		do_IRQ(ATH_PCI_IRQ_BASE);
	}
#endif

#ifdef CONFIG_ATH_HAS_PCI_EP
	if (int_status & RST_GLOBAL_INTERRUPT_STATUS_PCIE_EP_INT_MASK) {
		do_IRQ(ATH_CPU_IRQ_PCI_EP);
	}
#endif
	if (int_status & RST_GLOBAL_INTERRUPT_STATUS_WMAC_INT_MASK) {
		do_IRQ(ATH_CPU_IRQ_WLAN);
	}
}

void ath_demux_usb_pciep_rc2(void)
{
	uint32_t intr = ath_reg_rd(ATH_GLOBAL_INT_STATUS);

#ifdef CONFIG_ATH_HAS_PCI_RC2
	if (intr & RST_GLOBAL_INTERRUPT_STATUS_PCIE_RC2_INT_MASK) {
		do_IRQ(ATH_PCI_RC2_IRQ);
	}
#endif
	if (intr & (RST_GLOBAL_INTERRUPT_STATUS_USB1_INT_MASK |
			RST_GLOBAL_INTERRUPT_STATUS_USB2_INT_MASK)) {
		do_IRQ(ATH_CPU_IRQ_USB);
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
	unsigned int i, j, reg = 0, dac_data = 0;

	// Start Write transaction
	reg = val ;
	for (j = 0; j < 5; j++) { ; }

	for (i = 0; i < 8; i++) {	// TRANSMIT DATA
		dac_data = 0x50000;
		if ((reg >> (7-i) & 0x1) == 0x1) {
			dac_data = dac_data | 0x1;
		} else {
			dac_data = dac_data & 0xfffffffe;
		}
		ath_reg_wr(ATH_SPI_WRITE, dac_data);

		for (j = 0; j < 5; j++) { ; }

		dac_data = dac_data | 0x50100;	// RISING_CLK
		ath_reg_wr(ATH_SPI_WRITE, dac_data);

		for (j = 0; j < 15; j++) { ; }
	}
}

unsigned int ath_spi_raw_input_u8(void)
{
	unsigned int i, j;

	for (i = 0; i < 8; i++) {	// TRANSMIT DATA

		ath_reg_wr(ATH_SPI_WRITE, 0x50100);	//CS1 = 0 , CLK=1
		for (j = 0; j < 15; j++) { ; }

		ath_reg_wr(ATH_SPI_WRITE, 0x50000);	//CS1 = 0 , CLK=0
		for (j = 0; j < 15; j++) { ; }
	}
	ath_reg_wr(ATH_SPI_WRITE, 0x70000);	//CS1 = 1 , CLK=0
	for (j = 0; j < 15; j++) { ; }

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
#if 0
	if ((ath_reg_rd(ATH_BOOTSTRAP_REG) & ATH_REF_CLK_40)) {
		ref = (40 * 1000000);
	} else {
		ref = (25 * 1000000);
	}
#endif
	ref = (40 * 1000000);
	ath_uart_freq = ath_ref_freq = ref;

#ifdef CONFIG_ATH_EMULATION
	ath_cpu_freq = 80000000;
	ath_ddr_freq = 80000000;
	ath_ahb_freq = 40000000;
#else
	clk_ctrl = ath_reg_rd(ATH_DDR_CLK_CTRL);

	pll = ath_reg_rd(ATH_PLL_CONFIG);
	out_div	= CPU_PLL_CONFIG_OUTDIV_GET(pll);
	ref_div	= CPU_PLL_CONFIG_REFDIV_GET(pll);
	nint	= CPU_PLL_CONFIG_NINT_GET(pll);
	frac	= CPU_PLL_CONFIG_NFRAC_GET(pll);
	pll	= ref >> 6;
	frac	= frac * pll / ref_div;
	printk("cpu apb ");

	ath_cpu_freq = (((nint * (ref / ref_div)) + frac) >> out_div) /
			(CPU_DDR_CLOCK_CONTROL_CPU_POST_DIV_GET(clk_ctrl) + 1);

	pll = ath_reg_rd(ATH_DDR_PLL_CONFIG);
	out_div	= DDR_PLL_CONFIG_OUTDIV_GET(pll);
	ref_div	= DDR_PLL_CONFIG_REFDIV_GET(pll);
	nint	= DDR_PLL_CONFIG_NINT_GET(pll);
	frac	= DDR_PLL_CONFIG_NFRAC_GET(pll);
	pll	= ref >> 10;
	frac	= frac * pll / ref_div;
	printk("ddr apb ");

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
	printk("%s: cpu %u ddr %u ahb %u\n", __func__,
		ath_cpu_freq / 1000000,
		ath_ddr_freq / 1000000,
		ath_ahb_freq / 1000000);
}

/*
 * EHCI (USB full speed host controller)
 */
static struct resource ath_usb_ehci_resources_1[] = {
	[0] = {
		.start = ATH_USB_EHCI_BASE_1,
		.end = ATH_USB_EHCI_BASE_1 + ATH_USB_WINDOW - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = ATH_CPU_IRQ_USB,
		.end = ATH_CPU_IRQ_USB,
		.flags = IORESOURCE_IRQ,
	},
};
static struct resource ath_usb_ehci_resources_2[] = {
	[0] = {
		.start = ATH_USB_EHCI_BASE_2,
		.end = ATH_USB_EHCI_BASE_2 + ATH_USB_WINDOW - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = ATH_CPU_IRQ_USB,
		.end = ATH_CPU_IRQ_USB,
		.flags = IORESOURCE_IRQ,
	},
};

#ifdef CONFIG_ATH_HAS_PCI_EP
/*
 * (PCI EP controller)
 */
static struct resource ath_pci_ep_resources[] = {
        [0] = {
                .start  = ATH_PCI_EP_BASE_OFF,
                .end    = ATH_PCI_EP_BASE_OFF + 0xdff - 1,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = ATH_CPU_IRQ_PCI_EP,
                .end    = ATH_CPU_IRQ_PCI_EP,
                .flags  = IORESOURCE_IRQ,
        },
};

static u64 pci_ep_dmamask = ~(u32)0;
static struct platform_device ath_pci_ep_device = {
        .name                           = "ath-pciep",
        .id                             = 0,
        .dev = {
                .dma_mask               = &pci_ep_dmamask,
                .coherent_dma_mask      = 0xffffffff,
        },
        .num_resources                  = ARRAY_SIZE(ath_pci_ep_resources),
        .resource                       = ath_pci_ep_resources,
};
#endif

/*
 * The dmamask must be set for EHCI to work
 */
static u64 ehci_dmamask = ~(u32) 0;

static struct platform_device ath_usb_ehci_device_1 = {
	.name = "ath-ehci",
	.id = 0,
	.dev = {
		.dma_mask = &ehci_dmamask,
		.coherent_dma_mask = 0xffffffff,
		},
	.num_resources = ARRAY_SIZE(ath_usb_ehci_resources_1),
	.resource = ath_usb_ehci_resources_1,
};

static struct platform_device ath_usb_ehci_device_2 = {
        .name = "ath-ehci1",
        .id = 1,
        .dev = {
                .dma_mask = &ehci_dmamask,
                .coherent_dma_mask = 0xffffffff,
                },
        .num_resources = ARRAY_SIZE(ath_usb_ehci_resources_2),
        .resource = ath_usb_ehci_resources_2,
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
	&ath_usb_ehci_device_1,
	&ath_usb_ehci_device_2,
#ifdef CONFIG_ATH_HAS_PCI_EP
	&ath_pci_ep_device
#endif
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

arch_initcall(ath_platform_init);
