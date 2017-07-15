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
    unsigned int    rdata, i;
	unsigned int	cpu_pll_low_int, cpu_pll_low_frac, cpu_pll_high_int, cpu_pll_high_frac;
	unsigned int	ddr_pll_low_int, ddr_pll_low_frac, ddr_pll_high_int, ddr_pll_high_frac;
	unsigned int	cpu_clk_low, cpu_clk_high;
	unsigned int	ddr_clk_low, ddr_clk_high;
	unsigned int	ahb_clk_low, ahb_clk_high;
	/* CPU_DDR_CLOCK_CONTROL */
	unsigned int	ahbclk_from_ddrpll, ahb_post_div, ddr_post_div, cpu_post_div;
	unsigned int	cpu_ddr_clk_from_cpupll, cpu_ddr_clk_from_ddrpll;
	unsigned int	ahb_pll_bypass, ddr_pll_bypass, cpu_pll_bypass;
	/* CPU_PLL_CONFIG, CPU_PLL_CONFIG1, CPU_PLL_DITHER1, CPU_PLL_DITHER2 */
	unsigned int cpu_pllpwd, cpu_outdiv, cpu_Refdiv, cpu_Nint;
	unsigned int cpu_dither_en, cpu_NFrac_Min, cpu_NFrac_Max;
    unsigned int cpu_NFrac_Min_17_5, cpu_NFrac_Min_4_0;
    unsigned int cpu_NFrac_Max_17_5, cpu_NFrac_Max_4_0;
	/* DDR_PLL_CONFIG, DDR_PLL_CONFIG1, DDR_PLL_DITHER1, DDR_PLL_DITHER2 */
	unsigned int ddr_pllpwd, ddr_outdiv, ddr_Refdiv, ddr_Nint;
	unsigned int ddr_dither_en, ddr_NFrac_Min, ddr_NFrac_Max;	
    unsigned int ddr_NFrac_Min_17_5, ddr_NFrac_Min_4_0;
    unsigned int ddr_NFrac_Max_17_5, ddr_NFrac_Max_4_0;
#endif
	uint32_t ref_clk;

#if defined(CONFIG_ATH_EMULATION)
	ref_clk = (40 * 1000000);
#else
	if ((ath_reg_rd(RST_BOOTSTRAP_ADDRESS) & ATH_REF_CLK_40)) {
		ref_clk = (40 * 1000000);
	} else {
		ref_clk = (25 * 1000000);
	}
#endif	
	ath_uart_freq = ath_ref_freq = ref_clk;
#ifdef CONFIG_ATH_EMULATION
	ath_cpu_freq = 80000000;
	ath_ddr_freq = 80000000;
	ath_ahb_freq = 40000000;
#else
	rdata = ath_reg_rd(CPU_DDR_CLOCK_CONTROL_ADDRESS);
	ahbclk_from_ddrpll	= CPU_DDR_CLOCK_CONTROL_AHBCLK_FROM_DDRPLL_GET(rdata);
	ahb_post_div		= CPU_DDR_CLOCK_CONTROL_AHB_POST_DIV_GET(rdata);
	ddr_post_div		= CPU_DDR_CLOCK_CONTROL_DDR_POST_DIV_GET(rdata);
	cpu_post_div		= CPU_DDR_CLOCK_CONTROL_CPU_POST_DIV_GET(rdata);
	cpu_ddr_clk_from_cpupll = CPU_DDR_CLOCK_CONTROL_CPU_DDR_CLK_FROM_CPUPLL_GET(rdata);
	cpu_ddr_clk_from_ddrpll = CPU_DDR_CLOCK_CONTROL_CPU_DDR_CLK_FROM_DDRPLL_GET(rdata);
	ahb_pll_bypass		= CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_GET(rdata);
	ddr_pll_bypass		= CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_GET(rdata);
	cpu_pll_bypass		= CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_GET(rdata);

	if (ahb_pll_bypass) {
		ath_ahb_freq = ref_clk / (ahb_post_div + 1);
        //*ahb_clk_h = ref_clk / (ahb_post_div + 1);
	}

	if (ddr_pll_bypass) {
		ath_ddr_freq = ref_clk;
		//*ddr_clk_h = ref_clk;
	}

	if (cpu_pll_bypass) {
		ath_cpu_freq = ref_clk;
		//*cpu_clk_h = ref_clk;
	}

	if (ahb_pll_bypass && ddr_pll_bypass && cpu_pll_bypass) {
		return;
	}

	rdata = ath_reg_rd(CPU_PLL_CONFIG_ADDRESS);
	cpu_pllpwd	= CPU_PLL_CONFIG_PLLPWD_GET(rdata);
	cpu_outdiv	= CPU_PLL_CONFIG_OUTDIV_GET(rdata);
	cpu_Refdiv	= CPU_PLL_CONFIG_REFDIV_GET(rdata);

	rdata = ath_reg_rd(CPU_PLL_CONFIG1_ADDRESS);
	cpu_Nint	= CPU_PLL_CONFIG1_NINT_GET(rdata);

	rdata = ath_reg_rd(CPU_PLL_DITHER1_ADDRESS);
	cpu_dither_en	= CPU_PLL_DITHER1_DITHER_EN_GET(rdata);
	cpu_NFrac_Min	= CPU_PLL_DITHER1_NFRAC_MIN_GET(rdata);
    cpu_NFrac_Min_17_5 = (cpu_NFrac_Min >> 5) & 0x1fff;
    cpu_NFrac_Min_4_0  = cpu_NFrac_Min & 0x1f;

	rdata = ath_reg_rd(CPU_PLL_DITHER1_ADDRESS);
	cpu_NFrac_Max	= CPU_PLL_DITHER2_NFRAC_MAX_GET(rdata);
    cpu_NFrac_Max_17_5 = (cpu_NFrac_Max >> 5) & 0x1fff;
    cpu_NFrac_Max_4_0  = cpu_NFrac_Max & 0x1f;

	rdata = ath_reg_rd(DDR_PLL_CONFIG_ADDRESS);
	ddr_pllpwd	= DDR_PLL_CONFIG_PLLPWD_GET(rdata);
	ddr_outdiv	= DDR_PLL_CONFIG_OUTDIV_GET(rdata);
	ddr_Refdiv	= DDR_PLL_CONFIG_REFDIV_GET(rdata);

	rdata = ath_reg_rd(DDR_PLL_CONFIG1_ADDRESS);
	ddr_Nint	= DDR_PLL_CONFIG1_NINT_GET(rdata);

	rdata = ath_reg_rd(DDR_PLL_DITHER1_ADDRESS);
	ddr_dither_en	= DDR_PLL_DITHER1_DITHER_EN_GET(rdata);
	ddr_NFrac_Min	= DDR_PLL_DITHER1_NFRAC_MIN_GET(rdata);
    ddr_NFrac_Min_17_5 = (ddr_NFrac_Min >> 5) & 0x1fff;
    ddr_NFrac_Min_4_0  = ddr_NFrac_Min & 0x1f;

	rdata = ath_reg_rd(DDR_PLL_DITHER1_ADDRESS);
	ddr_NFrac_Max	= DDR_PLL_DITHER2_NFRAC_MAX_GET(rdata);
    ddr_NFrac_Max_17_5 = (ddr_NFrac_Max >> 5) & 0x1fff;
    ddr_NFrac_Max_4_0  = ddr_NFrac_Max & 0x1f;

	/* CPU PLL */
    i = (ref_clk/cpu_Refdiv);

    cpu_pll_low_int  = i*cpu_Nint;
    cpu_pll_high_int = cpu_pll_low_int;

    cpu_pll_low_frac = (i/(25*32))*((cpu_NFrac_Min_17_5*25 + cpu_NFrac_Min_4_0)/(8192/32));
    cpu_pll_high_frac = (i/(25*32))*((cpu_NFrac_Max_17_5*25 + cpu_NFrac_Max_4_0)/(8192/32));

    if (!cpu_dither_en || cpu_pll_high_frac <= cpu_pll_low_frac) {
        cpu_pll_high_frac = cpu_pll_low_frac;
    }

     /* DDR PLL */
    i = (ref_clk/ddr_Refdiv);

    ddr_pll_low_int  = i*ddr_Nint;
    ddr_pll_high_int = ddr_pll_low_int;

    ddr_pll_low_frac = (i/(25*32))*((ddr_NFrac_Min_17_5*25 + ddr_NFrac_Min_4_0)/(8192/32));
    ddr_pll_high_frac = (i/(25*32))*((ddr_NFrac_Max_17_5*25 + ddr_NFrac_Max_4_0)/(8192/32));

    if (!ddr_dither_en || ddr_pll_high_frac <= ddr_pll_low_frac) {
        ddr_pll_high_frac = ddr_pll_low_frac;
    }

    /* CPU Clock, DDR Clock, AHB Clock (before post div) */
    if (cpu_ddr_clk_from_cpupll) {
        cpu_clk_low  = cpu_pll_low_int + cpu_pll_low_frac;
        cpu_clk_high = cpu_pll_high_int + cpu_pll_high_frac;

        if (cpu_outdiv != 0) {
            cpu_clk_low  /= (2*cpu_outdiv);
            cpu_clk_high /= (2*cpu_outdiv);
        }

        ddr_clk_low  = cpu_clk_low;
        ddr_clk_high = cpu_clk_high;
    } else if (cpu_ddr_clk_from_ddrpll) {
        ddr_clk_low  = ddr_pll_low_int + ddr_pll_low_frac;
        ddr_clk_high = ddr_pll_high_int + ddr_pll_high_frac;

        if (ddr_outdiv != 0) {
            ddr_clk_low  /= (2*ddr_outdiv);
            ddr_clk_high /= (2*ddr_outdiv);
        }

        cpu_clk_low  = ddr_clk_low;
        cpu_clk_high = ddr_clk_high;
    } else {
        cpu_clk_low  = cpu_pll_low_int + cpu_pll_low_frac;
        cpu_clk_high = cpu_pll_high_int + cpu_pll_high_frac;
        ddr_clk_low  = ddr_pll_low_int + ddr_pll_low_frac;
        ddr_clk_high = ddr_pll_high_int + ddr_pll_high_frac;

        if (cpu_outdiv != 0) {
            cpu_clk_low  /= (2*cpu_outdiv);
            cpu_clk_high /= (2*cpu_outdiv);
        }

        if (ddr_outdiv != 0) {
            ddr_clk_low  /= (2*ddr_outdiv);
            ddr_clk_high /= (2*ddr_outdiv);
        }
    }

    if (ahbclk_from_ddrpll) {
        ahb_clk_low  = ddr_clk_low;
        ahb_clk_high = ddr_clk_high;
    } else {
        ahb_clk_low  = cpu_clk_low;
        ahb_clk_high = cpu_clk_high;
    }

    /* CPU Clock, DDR Clock, AHB Clock */
    cpu_clk_low  /= (cpu_post_div + 1);
    cpu_clk_high /= (cpu_post_div + 1);
    ddr_clk_low  /= (ddr_post_div + 1);
    ddr_clk_high /= (ddr_post_div + 1);
    ahb_clk_low  /= (ahb_post_div + 1);
    ahb_clk_high /= (ahb_post_div + 1);

	ath_cpu_freq = cpu_clk_low;
	ath_ddr_freq = ddr_clk_low;
	ath_ahb_freq = ahb_clk_low;
	//*cpu_clk_h = cpu_clk_high;
	//*ddr_clk_h = ddr_clk_high;
	//*ahb_clk_h = ahb_clk_high;
#endif

	printk("%s: cpu %u ddr %u ahb %u\n", __func__,
		ath_cpu_freq / 1000000,
		ath_ddr_freq / 1000000,
		ath_ahb_freq / 1000000);
    return;

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
