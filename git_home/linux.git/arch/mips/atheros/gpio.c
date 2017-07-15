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

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/resource.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <asm/types.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/system.h>

#include <atheros.h>

#define ATH_FACTORY_RESET		0x89ABCDEF

static atomic_t ath_fr_status = ATOMIC_INIT(0);
static volatile int ath_fr_opened = 0;
static wait_queue_head_t ath_fr_wq;
#ifdef JUMPSTART_GPIO
static u_int32_t push_time = 0;
#ifdef CONFIG_MACH_QCA956x
static u_int32_t prev_push_time = 0;
#endif
#endif
struct timer_list os_timer_t;
static int initial_led_state = 0;
#ifdef POWER_ON_RLED_GPIO
#define POWER_ON_TIMEOUT            60      /* 60 * 0.5 seconds */
#define POWER_LED_BLINK_INTERVAL    500     /* microseconds */
static volatile int power_on_finish = 0;
struct timer_list power_on_timer;
static struct proc_dir_entry *power_on_proc_entry = NULL;
#endif

#define frdbg printk

#define WPS_LED_OFF	1
#define WPS_LED_ON	0

#define USB_LED_OFF 1
#define USB_LED_ON 0

#define POWER_LED_OFF 1
#define POWER_LED_ON 0

#define SIMPLE_CONFIG_OFF	1
#define SIMPLE_CONFIG_ON	2
#define SIMPLE_CONFIG_BLINK	3
#define SIMPLE_CONFIG_FAIL  4

#define OS_TIMER_FUNC(_fn)	\
	void _fn(unsigned long timer_arg)

#define OS_GET_TIMER_ARG(_arg, _type)	\
	(_arg) = (_type)(timer_arg)

#define OS_INIT_TIMER(_osdev, _timer, _fn, _arg)	\
do {							\
	init_timer(_timer);				\
	(_timer)->function = (_fn);			\
	(_timer)->data = (unsigned long)(_arg);		\
} while (0)

#define OS_SET_TIMER(_timer, _ms)	\
	mod_timer(_timer, jiffies + ((_ms)*HZ)/1000)

#define OS_CANCEL_TIMER(_timer)		del_timer(_timer)
/*
 * GPIO interrupt stuff
 */
typedef enum {
    INT_TYPE_EDGE,
    INT_TYPE_LEVEL,
} ath_gpio_int_type_t;

typedef enum {
    INT_POL_ACTIVE_LOW,
    INT_POL_ACTIVE_HIGH,
} ath_gpio_int_pol_t;
/*
** Simple Config stuff
*/
typedef irqreturn_t (*sc_callback_t) (int, void *, void *, void *);

/*
 * Multiple Simple Config callback support
 * For multiple radio scenarios, we need to post the button push to
 * all radios at the same time.  However, there is only 1 button, so
 * we only have one set of GPIO callback pointers.
 *
 * Creating a structure that contains each callback, tagged with the
 * name of the device registering the callback.  The unregister routine
 * will need to determine which element to "unregister", so the device
 * name will have to be passed to unregister also
 */

typedef struct {
	char		*name;
	sc_callback_t	registered_cb;
	void		*cb_arg1;
	void		*cb_arg2;
} multi_callback_t;

/*
 * Specific instance of the callback structure
 */
static multi_callback_t sccallback[3];
static volatile int ignore_pushbutton = 1;
static struct proc_dir_entry *simple_config_entry = NULL;
static struct proc_dir_entry *simulate_push_button_entry = NULL;
static struct proc_dir_entry *simple_config_led_entry = NULL;
static int wps_led_blinking = 0;

void ath_gpio_config_int(int gpio,
			 ath_gpio_int_type_t type,
			 ath_gpio_int_pol_t polarity)
{
	u32 val;

	/*
	 * allow edge sensitive/rising edge too
	 */
	if (type == INT_TYPE_LEVEL) {
		/* level sensitive */
		ath_reg_rmw_set(ATH_GPIO_INT_TYPE, (1 << gpio));
	} else {
		/* edge triggered */
		val = ath_reg_rd(ATH_GPIO_INT_TYPE);
		val &= ~(1 << gpio);
		ath_reg_wr(ATH_GPIO_INT_TYPE, val);
	}

	if (polarity == INT_POL_ACTIVE_HIGH) {
		ath_reg_rmw_set(ATH_GPIO_INT_POLARITY, (1 << gpio));
	} else {
		val = ath_reg_rd(ATH_GPIO_INT_POLARITY);
		val &= ~(1 << gpio);
		ath_reg_wr(ATH_GPIO_INT_POLARITY, val);
	}

	ath_reg_rmw_set(ATH_GPIO_INT_ENABLE, (1 << gpio));
}

void ath_gpio_config_output(int gpio)
{
#if defined(CONFIG_MACH_AR934x) || \
    defined(CONFIG_MACH_QCA955x) || \
    defined(CONFIG_MACH_QCA953x) || \
	defined(CONFIG_MACH_QCA956x)
	ath_reg_rmw_clear(ATH_GPIO_OE, (1 << gpio));
#else
	ath_reg_rmw_set(ATH_GPIO_OE, (1 << gpio));
#endif
}
EXPORT_SYMBOL(ath_gpio_config_output);

void ath_gpio_config_input(int gpio)
{
#if defined(CONFIG_MACH_AR934x) || \
    defined(CONFIG_MACH_QCA955x) || \
    defined(CONFIG_MACH_QCA953x) || \
	defined(CONFIG_MACH_QCA956x)
	ath_reg_rmw_set(ATH_GPIO_OE, (1 << gpio));
#else
	ath_reg_rmw_clear(ATH_GPIO_OE, (1 << gpio));
#endif
}
EXPORT_SYMBOL(ath_gpio_config_input);

void ath_gpio_out_val(int gpio, int val)
{
	if (val & 0x1) {
		ath_reg_rmw_set(ATH_GPIO_OUT, (1 << gpio));
	} else {
		ath_reg_rmw_clear(ATH_GPIO_OUT, (1 << gpio));
	}
}
EXPORT_SYMBOL(ath_gpio_out_val);

int ath_gpio_in_val(int gpio)
{
	return ((1 << gpio) & (ath_reg_rd(ATH_GPIO_IN)));
}
EXPORT_SYMBOL(ath_gpio_in_val);

static void
ath_gpio_intr_enable(unsigned int irq)
{
	ath_reg_rmw_set(ATH_GPIO_INT_MASK,
				(1 << (irq - ATH_GPIO_IRQ_BASE)));
}

static void
ath_gpio_intr_disable(unsigned int irq)
{
	ath_reg_rmw_clear(ATH_GPIO_INT_MASK,
				(1 << (irq - ATH_GPIO_IRQ_BASE)));
}

static unsigned int
ath_gpio_intr_startup(unsigned int irq)
{
	ath_gpio_intr_enable(irq);
	return 0;
}

static void
ath_gpio_intr_shutdown(unsigned int irq)
{
	ath_gpio_intr_disable(irq);
}

static void
ath_gpio_intr_ack(unsigned int irq)
{
	ath_gpio_intr_disable(irq);
}

static void
ath_gpio_intr_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED | IRQ_INPROGRESS)))
		ath_gpio_intr_enable(irq);
}

static int
ath_gpio_intr_set_affinity(unsigned int irq, const struct cpumask *dest)
{
	/*
	 * Only 1 CPU; ignore affinity request
	 */
	return 0;
}

struct irq_chip /* hw_interrupt_type */ ath_gpio_intr_controller = {
	.name = "ATH GPIO",
	.startup = ath_gpio_intr_startup,
	.shutdown = ath_gpio_intr_shutdown,
	.enable = ath_gpio_intr_enable,
	.disable = ath_gpio_intr_disable,
	.ack = ath_gpio_intr_ack,
	.end = ath_gpio_intr_end,
	.eoi = ath_gpio_intr_end,
	.set_affinity = ath_gpio_intr_set_affinity,
};

void ath_gpio_irq_init(int irq_base)
{
	int i;

	for (i = irq_base; i < irq_base + ATH_GPIO_IRQ_COUNT; i++) {
		irq_desc[i].status = IRQ_DISABLED;
		irq_desc[i].action = NULL;
		irq_desc[i].depth = 1;
		//irq_desc[i].chip = &ath_gpio_intr_controller;
		set_irq_chip_and_handler(i, &ath_gpio_intr_controller,
					 handle_percpu_irq);
	}
}

void
ath_gpio_set_fn(int gpio, int fn)
{
#define gpio_fn_reg(x)	((x) / 4)
#define gpio_lsb(x)	(((x) % 4) * 8)
#define gpio_msb(x)	(gpio_lsb(x) + 7)
#define gpio_mask(x)	(0xffu << gpio_lsb(x))
#define gpio_set(x, f)	(((f) & 0xffu) << gpio_lsb(x))

	uint32_t *reg = ((uint32_t *)GPIO_OUT_FUNCTION0_ADDRESS) +
					gpio_fn_reg(gpio);

	ath_reg_wr(reg, (ath_reg_rd(reg) & ~gpio_mask(gpio)) | gpio_set(gpio, fn));
}

int32_t register_simple_config_callback(char *cbname, void *callback, void *arg1, void *arg2)
{
	int i;
	int cbnum = sizeof(sccallback)/sizeof(multi_callback_t);

	printk("SC Callback Registration for %s\n", cbname);

	for (i = 0; i< cbnum; i++)
	{
		if (!sccallback[i].name) {
			sccallback[i].name = (char*)kmalloc(strlen(cbname), GFP_KERNEL);
			strcpy(sccallback[i].name, cbname);
			sccallback[i].registered_cb = (sc_callback_t) callback;
			sccallback[i].cb_arg1 = arg1;
			sccallback[i].cb_arg2 = arg2;
			break;
		}
	}

	if (i == cbnum)
	{
		printk("@@@@ Failed SC Callback Registration for %s\n", cbname);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(register_simple_config_callback);

int32_t unregister_simple_config_callback(char *cbname)
{
	int i;
	int cbnum = sizeof(sccallback)/sizeof(multi_callback_t);

	for (i = 0; i< cbnum; i++)
	{
		if (sccallback[i].name && strcmp(sccallback[i].name, cbname) == 0) {
			kfree(sccallback[i].name);
			sccallback[i].name = NULL;
			sccallback[i].registered_cb = NULL;
			sccallback[i].cb_arg1 = NULL;
			sccallback[i].cb_arg2 = NULL;
			break;
		}
	}

	if (i == cbnum){
		printk("!&!&!&!& ERROR: Unknown callback name %s\n", cbname);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(unregister_simple_config_callback);

#ifdef WPS_LED_GPIO 
#define WPS_TIME_OUT 120
static OS_TIMER_FUNC(wps_led_blink)
{
	static int WPSled = WPS_LED_ON, sec = 0;
	ath_gpio_out_val(WPS_LED_GPIO, WPSled);
	WPSled = !WPSled;
	sec++;
	if (sec < WPS_TIME_OUT) {
		OS_SET_TIMER(&os_timer_t, 1000);
	} else {
		sec = 0;
		wps_led_blinking = 0;
		OS_CANCEL_TIMER(&os_timer_t);
		ath_gpio_out_val(WPS_LED_GPIO, initial_led_state);
	}
}

static OS_TIMER_FUNC(wps_led_fail)
{
	static int WPSled = WPS_LED_ON, sec = 0;
	ath_gpio_out_val(WPS_LED_GPIO, WPSled);
	WPSled = !WPSled;
	sec++;
	if (sec < 250 * 5) {//Keep blinking for 250 seconds & timer callback kicks in every 200 ms
		OS_SET_TIMER(&os_timer_t, 200);
	} else {
		sec = 0;
		wps_led_blinking = 0;
		OS_CANCEL_TIMER(&os_timer_t);
		ath_gpio_out_val(WPS_LED_GPIO, initial_led_state);
	}
}

static OS_TIMER_FUNC(wps_led_success)
{
    wps_led_blinking = 0;
    OS_CANCEL_TIMER(&os_timer_t);
    ath_gpio_out_val(WPS_LED_GPIO, initial_led_state);
}
#endif /* ifdef WPS_LED_GPIO */

#ifdef JUMPSTART_GPIO
int ath_simple_config_invoke_cb(int simplecfg_only, int irq_enable, int cpl)
{
	int i;
	int cbnum = sizeof(sccallback)/sizeof(multi_callback_t);

	printk("%s: sc %d, irq %d, ignorepb %d, jiffies %lu\n", __func__,
		simplecfg_only, irq_enable, ignore_pushbutton, jiffies);
	if (simplecfg_only) {
		if (ignore_pushbutton) {
#ifndef CONFIG_MACH_QCA956x
			ath_gpio_config_int(JUMPSTART_GPIO, INT_TYPE_LEVEL,
						INT_POL_ACTIVE_HIGH);
#endif
			ignore_pushbutton = 0;
			push_time = jiffies;
			return IRQ_HANDLED;
		}

		ath_gpio_config_int(JUMPSTART_GPIO, INT_TYPE_LEVEL,
					INT_POL_ACTIVE_LOW);
		ignore_pushbutton = 1;
	}

	if (irq_enable)
		local_irq_enable();

	if (push_time) {
#ifdef CONFIG_MACH_QCA956x
        /* ignore continuous interrupt in short time */
        u_int32_t diff_time;

        if (push_time >= prev_push_time)
            diff_time = push_time - prev_push_time;
        else
            diff_time = prev_push_time - push_time; 

        if (diff_time < 50)
            return IRQ_HANDLED;

        prev_push_time = push_time;
#endif
		push_time = jiffies - push_time;
	}
	printk ("simple_config callback.. push dur in sec %d\n", push_time/HZ);


	for (i = 0; i< cbnum; i++)
	{
		if (sccallback[i].registered_cb) {
			if (sccallback[i].cb_arg2) {
				*(u_int32_t *)sccallback[i].cb_arg2 = push_time/HZ;
			}
			sccallback[i].registered_cb (cpl, sccallback[i].cb_arg1, NULL, sccallback[i].cb_arg2);
		}

	}

	return IRQ_HANDLED;
}

/*
 * Irq for front panel SW jumpstart switch
 * Connected to XSCALE through GPIO4
 */
irqreturn_t jumpstart_irq(int cpl, void *dev_id)
{
#ifndef AP_RESET_GPIO
	unsigned int delay;
	if (atomic_read(&ath_fr_status)) {
		local_irq_disable();

#define UDELAY_COUNT 4000
		push_time = jiffies;

		for (delay = UDELAY_COUNT; delay; delay--) {
			if (ath_gpio_in_val(JUMPSTART_GPIO)) {
				break;
			}
			udelay(1000);
		}

		if (!delay) {
			atomic_dec(&ath_fr_status);
			/*
			 * since we are going to reboot the board, we
			 * don't need the interrupt handler anymore,
			 * so disable it.
			 */
			disable_irq(ATH_GPIO_IRQn(JUMPSTART_GPIO));
			wake_up(&ath_fr_wq);
			printk("\nath: factory configuration restored..\n");
			local_irq_enable();
			return IRQ_HANDLED;
		} else {
			return (ath_simple_config_invoke_cb
				(0, 1, cpl));
		}
	} else
#endif
		return (ath_simple_config_invoke_cb(1, 0, cpl));
}
#endif /* ifdef JUMPSTART_GPIO */

#ifdef AP_RESET_GPIO
irqreturn_t ath_reset_irq(int cpl, void *dev_id)
{
    local_irq_disable();

    if(push_time == 0){
        ath_gpio_config_int(AP_RESET_GPIO, INT_TYPE_LEVEL,
                 INT_POL_ACTIVE_HIGH);
        push_time = jiffies;
		local_irq_enable();
        return IRQ_HANDLED;
    }else{
        ath_gpio_config_int(AP_RESET_GPIO, INT_TYPE_LEVEL,
                INT_POL_ACTIVE_LOW);
        push_time = jiffies - push_time;
    }

    if(push_time/HZ > 3){
		/*
		 * since we are going to reboot the board, we
		 * don't need the interrupt handler anymore,
		 * so disable it.
		 */
		disable_irq(ATH_GPIO_IRQn(AP_RESET_GPIO));
		wake_up(&ath_fr_wq);
		printk("\nath: factory configuration restored..\n");
        push_time = 0;
		local_irq_enable();
		return IRQ_HANDLED;
	} else if(push_time/HZ < 1){
        push_time = 0;
		local_irq_enable();
		return IRQ_HANDLED;
    }else{
		extern void ath_restart(char *);
		ath_restart(NULL);
		return IRQ_HANDLED;
	}
}
#endif

static int push_button_read(char *page, char **start, off_t off,
				int count, int *eof, void *data)
{
	return 0;
}

static int push_button_write(struct file *file, const char *buf,
				unsigned long count, void *data)
{
	int i;
	int cbnum = sizeof(sccallback)/sizeof(multi_callback_t);

	for (i = 0; i< cbnum; i++){
		if (sccallback[i].registered_cb) {
			sccallback[i].registered_cb (0, sccallback[i].cb_arg1, 0, sccallback[i].cb_arg2);
		}
	}
	return count;
}

typedef enum {
	LED_STATE_OFF = 1,
	LED_STATE_ON = 2,
	LED_STATE_BLINKING = 3,
} led_state_e;

#ifdef WPS_LED_GPIO 
static led_state_e simple_config_led_state = LED_STATE_OFF;

static int gpio_simple_config_led_read(char *page, char **start, off_t off,
					int count, int *eof, void *data)
{
	return sprintf(page, "%d\n", simple_config_led_state);
}

static int gpio_simple_config_led_write(struct file *file, const char *buf,
					unsigned long count, void *data)
{
	u_int32_t val;

	if (sscanf(buf, "%d", &val) != 1)
		return -EINVAL;
    
    if(val == SIMPLE_CONFIG_BLINK){
        if( ath_gpio_in_val(WPS_LED_GPIO) == 0 ){
            initial_led_state = WPS_LED_ON;
        }else{ 
            initial_led_state = WPS_LED_OFF;
        }
    }

	if ((val == SIMPLE_CONFIG_BLINK) && !wps_led_blinking) { /* wps LED blinking */
		wps_led_blinking = 1;
		simple_config_led_state = SIMPLE_CONFIG_BLINK;
		ath_gpio_out_val(WPS_LED_GPIO, WPS_LED_ON);
		OS_CANCEL_TIMER(&os_timer_t);
		OS_INIT_TIMER(NULL, &os_timer_t, wps_led_blink, &os_timer_t);
		OS_SET_TIMER(&os_timer_t, 1000);
	} else if (val == SIMPLE_CONFIG_FAIL) {	/* WPS failed */
		wps_led_blinking = 0;
		simple_config_led_state = SIMPLE_CONFIG_FAIL;
		ath_gpio_out_val(WPS_LED_GPIO, WPS_LED_ON);
		OS_CANCEL_TIMER(&os_timer_t);
		OS_INIT_TIMER(NULL, &os_timer_t, wps_led_fail, &os_timer_t);
		OS_SET_TIMER(&os_timer_t, 200);
	} else if (val == SIMPLE_CONFIG_ON) {	/* WPS Success */
		wps_led_blinking = 0;
		simple_config_led_state = SIMPLE_CONFIG_ON;
		OS_CANCEL_TIMER(&os_timer_t);
		ath_gpio_out_val(WPS_LED_GPIO, WPS_LED_ON);
		OS_INIT_TIMER(NULL, &os_timer_t, wps_led_success, &os_timer_t);
		OS_SET_TIMER(&os_timer_t, 120000);
	} else if (val == SIMPLE_CONFIG_OFF) {	/* wps LED off */
		wps_led_blinking = 0;
		simple_config_led_state = SIMPLE_CONFIG_OFF;
		OS_CANCEL_TIMER(&os_timer_t);
		ath_gpio_out_val(WPS_LED_GPIO, initial_led_state);
	}

	return count;
}
#endif /* ifdef WPS_LED_GPIO */

void ap_usb_led_on(void)
{
#ifdef CONFIG_MACH_AR934x
#if !defined(CONFIG_I2S) && defined(AP_USB_LED_GPIO)
	unsigned int rddata;

	if (AP_USB_LED_GPIO == 4) {
		rddata = ath_reg_rd(ATH_GPIO_OUT_FUNCTION1); //87- for USB suspend
		rddata = rddata & 0xffffff00;
		rddata = rddata | ATH_GPIO_OUT_FUNCTION1_ENABLE_GPIO_4(0x0);
		ath_reg_wr(ATH_GPIO_OUT_FUNCTION1, rddata);
	} else if (AP_USB_LED_GPIO == 11) {
		rddata = ath_reg_rd(ATH_GPIO_OUT_FUNCTION2); //87- for USB suspend
		rddata = rddata & 0x00ffffff;
		rddata = rddata | ATH_GPIO_OUT_FUNCTION2_ENABLE_GPIO_11(0x0);
		ath_reg_wr(ATH_GPIO_OUT_FUNCTION2, rddata);
	}

	ath_reg_rmw_clear(ATH_GPIO_OE, (1<<AP_USB_LED_GPIO));
	ath_reg_rmw_clear(ATH_GPIO_OUT, (1<<AP_USB_LED_GPIO));
#endif
#else
#	ifdef AP_USB_LED_GPIO
	ath_gpio_config_output(AP_USB_LED_GPIO);
	ath_gpio_set_fn(AP_USB_LED_GPIO, 0);
	ath_gpio_out_val(AP_USB_LED_GPIO, USB_LED_ON);
#	endif
#endif
}

EXPORT_SYMBOL(ap_usb_led_on);

void ap_usb_led_off(void)
{
#ifdef CONFIG_MACH_AR934x
#if !defined(CONFIG_I2S) && defined(AP_USB_LED_GPIO)
	ath_reg_rmw_set(ATH_GPIO_OUT, (1<<AP_USB_LED_GPIO));
#endif
#else
#	ifdef AP_USB_LED_GPIO
	ath_gpio_out_val(AP_USB_LED_GPIO, USB_LED_OFF);
#	endif
#endif
}
EXPORT_SYMBOL(ap_usb_led_off);

void ap_usb_host_led_on(int gpio)
{
#if defined(AP_USB1_LED_GPIO) || defined(AP_USB2_LED_GPIO)
    ath_gpio_config_output(gpio);
    ath_gpio_set_fn(gpio, 0);
    ath_gpio_out_val(gpio, USB_LED_ON);
#endif
}

EXPORT_SYMBOL(ap_usb_host_led_on);

void ap_usb_host_led_off(int gpio)
{
#if defined(AP_USB1_LED_GPIO) || defined(AP_USB2_LED_GPIO)
    ath_gpio_out_val(gpio, USB_LED_OFF);
#endif
}

EXPORT_SYMBOL(ap_usb_host_led_off);

void ath_config_slave_mdio_gpios(void)
{
	unsigned int mask;

	/* Slave MDIO phy address setting */
	ath_reg_wr(MDIO_PHY_ADDR_ADDRESS, ATH_MDIO_PHY_ADDR);

	/* Set the Clock Divisor */
	mask = ath_reg_rd(ATH_MDIO_MAC_MII_MGMT_CFG) & ~(0xf);
	mask |= ATH_MDIO_MGMT_CFG_CLK_DIV_28;
	ath_reg_wr(ATH_MDIO_MAC_MII_MGMT_CFG, mask);

#ifndef CONFIG_MACH_QCA956x
	/* Set External MDIO Multiplexing Register */
	mask = ath_reg_rd(ATH_GPIO_IN_ENABLE3) &
		~(GPIO_IN_ENABLE3_BOOT_EXT_MDC_MASK |
		GPIO_IN_ENABLE3_BOOT_EXT_MDO_MASK);
	mask |= GPIO_IN_ENABLE3_BOOT_EXT_MDC_SET(ATH_MDC_GPIO_PIN) |
		GPIO_IN_ENABLE3_BOOT_EXT_MDO_SET(ATH_MDIO_GPIO_PIN);
	ath_reg_wr(ATH_GPIO_IN_ENABLE3, mask);
#endif
	/* Configure the Direction of GPIO Pins */
	ath_gpio_config_input(ATH_MDC_GPIO_PIN);
	ath_gpio_config_output(ATH_MDIO_GPIO_PIN);
	/* Configure GPIO Output function as GPIOs */
	ath_gpio_set_fn(ATH_MDIO_GPIO_PIN, ATH_MDIO_OUPUT_FUNC);
}
EXPORT_SYMBOL(ath_config_slave_mdio_gpios);

#ifdef POWER_ON_RLED_GPIO
static int power_on_finish_read(char *page, char **start, off_t off,
        int count, int *eof, void *data)
{
	return sprintf(page, "%d\n", power_on_finish);
}

static int power_on_finish_write(struct file *file, const char *buf,
        unsigned long count, void *data)
{
	u_int32_t val;

	if (sscanf(buf, "%d", &val) != 1)
		return -EINVAL;

    power_on_finish = 1;

	return count;
}

static OS_TIMER_FUNC(power_led_blink)
{
	static int power_led_status = POWER_LED_OFF, power_on_timeout = 0;

    OS_CANCEL_TIMER(&power_on_timer);

    if (power_on_finish) {
		ath_gpio_out_val(POWER_ON_GLED_GPIO, POWER_LED_ON);
    } else if (++power_on_timeout >= POWER_ON_TIMEOUT) {
        ath_gpio_out_val(POWER_ON_GLED_GPIO, POWER_LED_OFF);  
        ath_gpio_config_input(POWER_ON_GLED_GPIO);
        ath_gpio_config_output(POWER_ON_RLED_GPIO);
        ath_gpio_out_val(POWER_ON_RLED_GPIO, POWER_LED_ON);  
    } else {
		ath_gpio_out_val(POWER_ON_GLED_GPIO, power_led_status);
	    power_led_status = !power_led_status;
		OS_SET_TIMER(&power_on_timer, POWER_LED_BLINK_INTERVAL);
    }
}
#endif

#ifdef WPS_LED_GPIO
static int create_simple_config_led_proc_entry(void)
{
	if (simple_config_entry != NULL) {
		printk("Already have a proc entry for /proc/simple_config!\n");
		return -ENOENT;
	}

	simple_config_entry = proc_mkdir("simple_config", NULL);
	if (!simple_config_entry)
		return -ENOENT;

	simulate_push_button_entry = create_proc_entry("push_button", 0644,
							simple_config_entry);
	if (!simulate_push_button_entry)
		return -ENOENT;

	simulate_push_button_entry->write_proc = push_button_write;
	simulate_push_button_entry->read_proc = push_button_read;

	simple_config_led_entry = create_proc_entry("simple_config_led", 0644,
							simple_config_entry);
	if (!simple_config_led_entry)
		return -ENOENT;

	simple_config_led_entry->write_proc = gpio_simple_config_led_write;
	simple_config_led_entry->read_proc = gpio_simple_config_led_read;

	/* configure gpio as outputs */
	ath_gpio_config_output(WPS_LED_GPIO);

	/* switch off the led */
	ath_gpio_out_val(WPS_LED_GPIO, WPS_LED_OFF);

#ifdef POWER_ON_RLED_GPIO
	power_on_proc_entry = create_proc_entry("power_on_finish", 0644,
							simple_config_entry);
	if (!power_on_proc_entry)
		return -ENOENT;

	power_on_proc_entry->write_proc = power_on_finish_write;
	power_on_proc_entry->read_proc = power_on_finish_read;
#endif
	return 0;
}
#endif /* ifdef WPS_LED_GPIO */

static int
athfr_open(struct inode *inode, struct file *file)
{
	if (MINOR(inode->i_rdev) != FACTORY_RESET_MINOR) {
		return -ENODEV;
	}

	if (ath_fr_opened) {
		return -EBUSY;
	}

	ath_fr_opened = 1;
	return nonseekable_open(inode, file);
}

static int
athfr_close(struct inode *inode, struct file *file)
{
	if (MINOR(inode->i_rdev) != FACTORY_RESET_MINOR) {
		return -ENODEV;
	}

	ath_fr_opened = 0;
	return 0;
}

static ssize_t
athfr_read(struct file *file, char *buf, size_t count, loff_t * ppos)
{
	return -ENOTSUPP;
}

static ssize_t
athfr_write(struct file *file, const char *buf, size_t count, loff_t * ppos)
{
	return -ENOTSUPP;
}

static int
athfr_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case ATH_FACTORY_RESET:
#ifndef AP_RESET_GPIO
		atomic_inc(&ath_fr_status);
#endif
		sleep_on(&ath_fr_wq);
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

static struct file_operations athfr_fops = {
	read:	athfr_read,
	write:	athfr_write,
	ioctl:	athfr_ioctl,
	open:	athfr_open,
	release:athfr_close
};

static struct miscdevice athfr_miscdev = {
	FACTORY_RESET_MINOR,
	"Factory reset",
	&athfr_fops
};

int __init ath_simple_config_init(void)
{
#ifdef CONFIG_CUS100
	u32 mask = 0;
#endif

#ifdef JUMPSTART_GPIO
	int req;
#endif
	int ret;
#ifdef AP_RESET_GPIO
    int req2;
#endif
	ret = misc_register(&athfr_miscdev);

	if (ret < 0) {
		printk("*** ath misc_register failed %d *** \n", ret);
		return -1;
	}

#ifdef AP_RESET_GPIO
    ath_gpio_config_input(AP_RESET_GPIO);
    ath_gpio_config_int(AP_RESET_GPIO, INT_TYPE_LEVEL, INT_POL_ACTIVE_LOW);
    printk("%s (%s) AP_RESET_GPIO: %d\n", __FILE__, __func__, AP_RESET_GPIO);
#endif

#ifdef JUMPSTART_GPIO
#ifdef CONFIG_CUS100
	mask = ath_reg_rd(ATH_MISC_INT_MASK);
	ath_reg_wr(ATH_MISC_INT_MASK, mask | (1 << 2));
	ath_gpio_config_int(JUMPSTART_GPIO, INT_TYPE_LEVEL,
				INT_POL_ACTIVE_HIGH);
	ath_gpio_intr_enable(JUMPSTART_GPIO);
	ath_gpio_config_input(JUMPSTART_GPIO);
#else
	ath_gpio_config_input(JUMPSTART_GPIO);
	/* configure Jumpstart GPIO as level triggered interrupt */
	ath_gpio_config_int(JUMPSTART_GPIO, INT_TYPE_LEVEL,
				INT_POL_ACTIVE_LOW);
	printk("%s (%s) JUMPSTART_GPIO: %d\n", __FILE__, __func__,
		JUMPSTART_GPIO);
#ifndef CONFIG_MACH_AR934x
	ath_reg_rmw_clear(ATH_GPIO_FUNCTIONS, (1 << 2));
	ath_reg_rmw_clear(ATH_GPIO_FUNCTIONS, (1 << 16));
	ath_reg_rmw_clear(ATH_GPIO_FUNCTIONS, (1 << 20));
#endif
#endif
#if 0
	req = request_irq(ATH_GPIO_IRQn(JUMPSTART_GPIO), jumpstart_irq, 0,
#ifdef AP_RESET_GPIO
			"SW JUMPSTART", NULL);
#else
			"SW JUMPSTART/FACTORY RESET", NULL);
#endif
	if (req != 0) {
		printk("request_irq for jumpstart failed (error %d)\n", req);
		misc_deregister(&athfr_miscdev);
		ath_gpio_intr_shutdown(ATH_GPIO_IRQn(JUMPSTART_GPIO));
		return -1;
	}
#endif
#endif /* #ifdef JUMPSTART_GPIO */
#ifdef AP_RESET_GPIO
    req2 = request_irq(ATH_GPIO_IRQn(AP_RESET_GPIO), ath_reset_irq, 0,
            "FACTORY RESET", NULL);
    if (req2 != 0) {
        printk("request_irq for factory reset failed (error %d)\n", req);
        misc_deregister(&athfr_miscdev);
        free_irq(req, NULL);
        return -1;
    }
#endif

#ifdef ATH_S17INT_GPIO
    ath_gpio_config_input(ATH_S17INT_GPIO);
	/* configure S17 interrupt GPIO as level triggered interrupt */
	ath_gpio_config_int(ATH_S17INT_GPIO, INT_TYPE_LEVEL,
				INT_POL_ACTIVE_LOW);
	printk("%s (%s) ATH_S17INT_GPIO: %d\n", __FILE__, __func__,
		ATH_S17INT_GPIO);
#endif

#if !defined(CONFIG_I2S) && defined(AP_USB_LED_GPIO)
	ath_gpio_config_output(AP_USB_LED_GPIO);
#endif
	init_waitqueue_head(&ath_fr_wq);

#ifdef WPS_LED_GPIO
	create_simple_config_led_proc_entry();
#endif

#ifdef POWER_ON_GLED_GPIO
	printk("%s (%s) POWER_ON_GLED_GPIO: %d\n", __FILE__, __func__, POWER_ON_GLED_GPIO);
    ath_gpio_config_output(POWER_ON_GLED_GPIO);
    ath_gpio_out_val(POWER_ON_GLED_GPIO, POWER_LED_ON);
#endif

#ifdef POWER_ON_RLED_GPIO
	printk("%s (%s) POWER_ON_RLED_GPIO: %d\n", __FILE__, __func__, POWER_ON_RLED_GPIO);
    ath_gpio_config_output(POWER_ON_RLED_GPIO);
    ath_gpio_out_val(POWER_ON_RLED_GPIO, POWER_LED_OFF);
    OS_INIT_TIMER(NULL, &power_on_timer, power_led_blink, NULL);
    OS_SET_TIMER(&power_on_timer, POWER_LED_BLINK_INTERVAL);
#endif
	/* Disable JTAG port functionality to enable GPIO functionality */
        ath_reg_rmw_set(ATH_GPIO_FUNCTIONS, (1 << 1));

	return 0;
}

/*
 * used late_initcall so that misc_register will succeed
 * otherwise, misc driver won't be in a initializated state
 * thereby resulting in misc_register api to fail.
 */
#if !defined(CONFIG_ATH_EMULATION)
late_initcall(ath_simple_config_init);
#endif

