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
#include <linux/pci.h>
#include <atheros.h>

/*
 * PCI IRQ map
 */
int __init
pcibios_map_irq(const struct pci_dev *dev, uint8_t slot, uint8_t pin)
{
	extern struct pci_ops ath_pci_ops;

	pr_debug("fixing irq for slot %d pin %d\n", slot, pin);

	if (dev->bus->number >= 0 &&
	    dev->bus->number < ATH_MAX_PCI_BUS) {
		printk("%s: IRQ %d for bus %d\n", __func__,
			ATH_PCI_IRQ_BASE + dev->bus->number,
			dev->bus->number);
#ifndef CONFIG_PCI_INIT_IN_MONITOR

#define ATH_PCI_CMD_INIT	(PCI_COMMAND_MEMORY |		\
				 PCI_COMMAND_MASTER |		\
				 PCI_COMMAND_INVALIDATE |	\
				 PCI_COMMAND_PARITY |		\
				 PCI_COMMAND_SERR |		\
				 PCI_COMMAND_FAST_BACK)

		/*
		 * clear any lingering errors and register core error IRQ
		 */
		ath_pci_ops.write(dev->bus, 0,
				PCI_COMMAND, 4, ATH_PCI_CMD_INIT);
#endif
		return (ATH_PCI_IRQ_BASE + dev->bus->number);
	} else {
		printk("%s: Unknown bus no. %d!\n",
			__func__, dev->bus->number);
		return -1;
	}
}

int
pcibios_plat_dev_init(struct pci_dev *dev)
{
	return 0;
}
