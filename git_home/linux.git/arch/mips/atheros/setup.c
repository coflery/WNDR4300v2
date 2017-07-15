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
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <linux/serial_8250.h>
#include <linux/console.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/pci.h>
#include <linux/mtd/mtd.h>

#include <asm/reboot.h>
#include <asm/io.h>
#include <asm/time.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/reboot.h>
#include <asm/system.h>
#include <asm/serial.h>
#include <asm/traps.h>

#include <atheros.h>

#if 0				/* For WLAN debug */
u_int32_t wasp_loop = 0;
EXPORT_SYMBOL(wasp_loop);

u_int32_t g_rxbuf_cnt = 0;
u_int32_t g_rxadded = 0;
u_int32_t g_rxdepth = 0;
u_int32_t g_rxtail = 0;
u_int32_t g_rxhead = 0;
u_int32_t g_rxbuf[512];
u_int32_t g_rxdp[512];
u_int32_t g_rxfifodepth[512];
EXPORT_SYMBOL(g_rxbuf_cnt);
EXPORT_SYMBOL(g_rxadded);
EXPORT_SYMBOL(g_rxdepth);
EXPORT_SYMBOL(g_rxtail);
EXPORT_SYMBOL(g_rxhead);
EXPORT_SYMBOL(g_rxbuf);
EXPORT_SYMBOL(g_rxdp);
EXPORT_SYMBOL(g_rxfifodepth);
#endif

uint32_t ath_cpu_freq, ath_ahb_freq, ath_ddr_freq,
	ath_ref_freq, ath_uart_freq;
uint32_t serial_inited;

static int __init ath_init_ioc(void);
void serial_print(const char *fmt, ...);
void writeserial(char *str, int count);
void ath_sys_frequency(void);

void UartInit(void);

/*
 * Export AHB freq value to be used by Ethernet MDIO.
 */
EXPORT_SYMBOL(ath_ahb_freq);

/*
 * Export Ref freq value to be used by I2S module.
 */
EXPORT_SYMBOL(ath_ref_freq);

void ath_restart(char *command)
{
	for (;;) {
		if (is_ar934x_10()) {
                	ath_reg_wr(ATH_GPIO_OE, ath_reg_rd(ATH_GPIO_OE) & (~(1 << 17)));
		} else {
			ath_reg_wr(ATH_RESET, RST_RESET_FULL_CHIP_RESET_MASK);
		}
	}
}

void ath_halt(void)
{
	printk(KERN_NOTICE "\n** You can safely turn off the power\n");
	while (1) ;
}

void ath_power_off(void)
{
	ath_halt();
}

const char
*get_system_type(void)
{
	extern uint32_t ath_otp_read(uint32_t addr);
#ifdef CONFIG_ATH_EMULATION
#	define	ath_sys_type(x)		x " emu"
#else
#	define	ath_sys_type(x)		x
#	define	ath_sys_type_otp(x)	x " wmac not present"
#endif

	/*
	 * Make sure WMAC is enabled.
	 * Read Memory Address 0 of OTP and Check if WMAC is Disabled
	 * If Disabled, do not initialize WMAC
	 */
#ifdef ATH_OTP_MEM_0FFSET_ZERO
	if (ath_otp_read(ATH_OTP_MEM_0FFSET_ZERO) & ATH_OTP_WMAC_DISABLED) {
		return ath_sys_type_otp(CONFIG_ATH_SYS_TYPE);
	} else
#endif
	{
		return ath_sys_type(CONFIG_ATH_SYS_TYPE);
	}
}
EXPORT_SYMBOL(get_system_type);

int
valid_wmac_num(u_int16_t wmac_num)
{
	return (wmac_num == 0);
}

/*
 * HOWL has only one wmac device, hence the following routines
 * ignore the wmac_num parameter
 */
int
get_wmac_irq(u_int16_t wmac_num)
{
	return ATH_CPU_IRQ_WLAN;
}

unsigned long
get_wmac_base(u_int16_t wmac_num)
{
	return KSEG1ADDR(ATH_WMAC_BASE);
}

unsigned long
get_wmac_mem_len(u_int16_t wmac_num)
{
	return ATH_WMAC_LEN;
}

EXPORT_SYMBOL(valid_wmac_num);
EXPORT_SYMBOL(get_wmac_irq);
EXPORT_SYMBOL(get_wmac_base);
EXPORT_SYMBOL(get_wmac_mem_len);

#ifdef CONFIG_SERIAL_8250
void __init ath_serial_setup(void)
{
	struct uart_port p;

	memset(&p, 0, sizeof(p));

	p.flags = (UPF_BOOT_AUTOCONF | UPF_SKIP_TEST);
	p.iotype = UPIO_MEM32;
	p.uartclk = ath_ahb_freq;
	p.irq = ATH_MISC_IRQ_UART;
	p.regshift = 2;
	p.mapbase = (u32) KSEG1ADDR(ATH_UART_BASE);
	p.membase = (void __iomem *)p.mapbase;

	if (early_serial_setup(&p) != 0)
		printk(KERN_ERR "early_serial_setup failed\n");

	serial_print("%s: early_serial_setup done..\n", __func__);

}
#endif

unsigned int __cpuinit get_c0_compare_int(void)
{
	//printk("%s: returning timer irq : %d\n",__func__, ATH_CPU_IRQ_TIMER);
	return ATH_CPU_IRQ_TIMER;
}

#ifdef CONFIG_MACH_QCA956x
static void __init fix_ev124229(void)
{
	ath_reg_rmw_set(RST_MISC_INTERRUPT_MASK_ADDRESS, RST_MISC_INTERRUPT_MASK_MIPS_SI_TimerInt_MASK_MASK);
}
#endif

void __init plat_time_init(void)
{
	mips_hpt_frequency = ath_cpu_freq / 2;
#ifdef CONFIG_MACH_QCA956x
	late_time_init = fix_ev124229;
#endif

	printk("%s: plat time init done\n", __func__);
}

#ifdef CONFIG_MACH_QCA955x
void ath_pci_reg_status(int id, u_int32_t int_status, u_int32_t g_reset_addr, u_int32_t app_addr,
				u_int32_t uer_reg, u_int32_t cer_reg)
{
	u_int32_t val;

	val = ath_reg_rd(int_status);
	printk(" PCI Intr register of RC%d : %08x\n", id, val);
	printk(" RC%d: PCI link down is %d and link req reset %d\n", id,
			PCIE_INT_MASK_LINK_DOWN_GET(val),
			PCIE_INT_MASK_LINK_REQ_RST_GET(val));
	printk(" PCI link status from global reg : %d\n",
			(ath_reg_rd(g_reset_addr) & 0x1 == 0x1));
	printk(" PCI Err counter registers are RC%d(%08x) : %08x\n", id, app_addr,
			ath_reg_rd(app_addr));
	printk(" PCI Err RC%d(%08x) : %08x,   (%08x) : %08x\n", id, uer_reg, ath_reg_rd(uer_reg),
			cer_reg, ath_reg_rd(cer_reg));
}
#endif

int ath_be_handler(struct pt_regs *regs, int is_fixup)
{
#ifdef CONFIG_MACH_AR934x
	printk("ath data bus error: cause 0x%x epc 0x%x\nrebooting...", read_c0_cause(), read_c0_epc());
	ath_restart(NULL);
#else
	printk("ath data bus error: cause %#x\n", read_c0_cause());
#endif

#ifdef CONFIG_MACH_QCA955x
	printk(" ####PCI link status on data bus error ####\n");

	ath_pci_reg_status(0, PCIE_INT_STATUS_ADDRESS, PCIE_RESET_ADDRESS,
				PCIE_AER_ADDRESS, PCIE_UER_ADDRESS, PCIE_CER_ADDRESS);
	ath_pci_reg_status(1, PCIE_INT_STATUS_ADDRESS_2, PCIE_RESET_ADDRESS_2,
				PCIE_AER_ADDRESS_2, PCIE_UER_ADDRESS_2, PCIE_CER_ADDRESS_2);
	printk("      ###################\n");
#endif
	return (is_fixup ? MIPS_BE_FIXUP : MIPS_BE_FATAL);
}

void __init plat_mem_setup(void)
{
#if 1
	board_be_handler = ath_be_handler;
#endif
	_machine_restart = ath_restart;
	_machine_halt = ath_halt;
	pm_power_off = ath_power_off;

	/*
	 ** early_serial_setup seems to conflict with serial8250_register_port()
	 ** In order for console to work, we need to call register_console().
	 ** We can call serial8250_register_port() directly or use
	 ** platform_add_devices() function which eventually calls the
	 ** register_console(). AP71 takes this approach too. Only drawback
	 ** is if system screws up before we register console, we won't see
	 ** any msgs on the console.  System being stable now this should be
	 ** a special case anyways. Just initialize Uart here.
	 */

#ifdef CONFIG_SERIAL_8250
	UartInit();
#endif

#ifdef CONFIG_MACH_AR933x
	/* clear wmac reset */
	ath_reg_wr(ATH_RESET,
		      (ath_reg_rd(ATH_RESET) & (~ATH_RESET_WMAC)));
#endif
	serial_print("Booting %s\n", get_system_type());
}

/*
 * -------------------------------------------------
 * Early printk hack
 */
u8 UartGetPoll(void) __attribute__ ((weak));
void UartPut(u8 byte) __attribute__ ((weak));

u8 UartGetPoll()
{
	while ((UART_READ(OFS_LINE_STATUS) & 0x1) == 0) ;
	return UART_READ(OFS_RCV_BUFFER);
}

void UartPut(u8 byte)
{
	if (!serial_inited) {
		serial_inited = 1;
		UartInit();
	}

	while (((UART_READ(OFS_LINE_STATUS)) & 0x20) == 0x0) ;
	UART_WRITE(OFS_SEND_BUFFER, byte);
}

#ifdef CONFIG_EARLY_PRINTK
void prom_putchar(u8 byte)
{
	UartPut(byte);
}
#endif

extern int vsprintf(char *buf, const char *fmt, va_list args);
static char sprint_buf[1024];

void serial_print(const char *fmt, ...)
{
	va_list args;
	int n;

	va_start(args, fmt);
	n = vsprintf(sprint_buf, fmt, args);
	va_end(args);
	writeserial(sprint_buf, n);
}

void writeserial(char *str, int count)
{
	int i;
	for (i = 0; i <= count; i++)
		UartPut(str[i]);

	UartPut('\r');
	memset(str, '\0', 1024);
	return;
}

unsigned int ath_serial_in(int offset)
{
	return UART_READ(offset);
}

void ath_serial_out(int offset, int value)
{
	UART_WRITE(offset, (u8) value);
}

#include <asm/uaccess.h>
#define M_PERFCTL_EVENT(event)          ((event) << 5)
unsigned int clocks_at_start;

void start_cntrs(unsigned int event0, unsigned int event1)
{
	write_c0_perfcntr0(0x00000000);
	write_c0_perfcntr1(0x00000000);
	/*
	 * go...
	 */
	write_c0_perfctrl0(0x80000000 | M_PERFCTL_EVENT(event0) | 0xf);
	write_c0_perfctrl1(0x00000000 | M_PERFCTL_EVENT(event1) | 0xf);
}

void stop_cntrs(void)
{
	write_c0_perfctrl0(0);
	write_c0_perfctrl1(0);
}

void read_cntrs(unsigned int *c0, unsigned int *c1)
{
	*c0 = read_c0_perfcntr0();
	*c1 = read_c0_perfcntr1();
}

static int ath_ioc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t
ath_ioc_read(struct file *file, char *buf, size_t count, loff_t * ppos)
{

	unsigned int c0, c1, ticks = (read_c0_count() - clocks_at_start);
	char str[256];
	unsigned int secs = ticks / mips_hpt_frequency;

	read_cntrs(&c0, &c1);
	stop_cntrs();
	sprintf(str, "%d secs (%#x) event0:%#x event1:%#x", secs, ticks, c0,
		c1);
	copy_to_user(buf, str, strlen(str));

	return (strlen(str));
}

#if 0
static void ath_dcache_test(void)
{
	int i, j;
	unsigned char p;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < (10 * 1024); j++) {
			p = *((unsigned char *)0x81000000 + j);
		}
	}
}
#endif

static ssize_t
ath_ioc_write(struct file *file, const char *buf, size_t count,
		 loff_t * ppos)
{
	int event0, event1;

	sscanf(buf, "%d:%d", &event0, &event1);
	printk("\nevent0 %d event1 %d\n", event0, event1);

	clocks_at_start = read_c0_count();
	start_cntrs(event0, event1);

	return (count);
}

struct file_operations ath_ioc_fops = {
      open:ath_ioc_open,
      read:ath_ioc_read,
      write:ath_ioc_write,
};

/*
 * General purpose ioctl i/f
 */
static int __init ath_init_ioc()
{
	static int _mymajor;

	_mymajor = register_chrdev(77, "ATH_GPIOC", &ath_ioc_fops);

	if (_mymajor < 0) {
		printk("Failed to register GPIOC\n");
		return _mymajor;
	}

	printk("ATH GPIOC major %d\n", _mymajor);
	return 0;
}

device_initcall(ath_init_ioc);

#ifdef CONFIG_MTD
/*
 * NAND Read API for Calibration data
 *
 */
int
ath_nand_local_read(u_char *cal_part,loff_t from, size_t len,
		size_t *retlen, u_char *buf)
{
	int	ret;
	struct mtd_info *mtd;

	if (!len || !retlen) return (0);

	printk("Reading Flash for Calibraton data from 0x%llx and partition name is %s\n",from,cal_part);

	mtd = get_mtd_device_nm(cal_part);
	if (mtd == ERR_PTR(-ENODEV)) {
		printk("MTD partition doesn't exist \n");
		ret = -EIO;
		return ret;
	}
	ret = mtd->read(mtd, from, len, retlen, buf);
	return ret;
}
EXPORT_SYMBOL(ath_nand_local_read);
#endif
