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
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <asm/delay.h>
#include <atheros.h>
/*
 * PCI cfg an I/O routines are done by programming a
 * command/byte enable register, and then read/writing
 * the data from a data regsiter. We need to ensure
 * these transactions are atomic or we will end up
 * with corrupt data on the bus or in a driver.
 */
static DEFINE_SPINLOCK(ath_pci_lock);

extern int ath_pci_link[];

int
ath_local_read_config(int bus, int where, int size, uint32_t *value)
{
	unsigned long	flags, addr, tval, mask;
	ATH_DECL_PCI_CRP_ARR(crp_reg);

	if (!ath_pci_link[bus]) {
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	/* Make sure the address is aligned to natural boundary */
	if (where & (size - 1))
		return PCIBIOS_BAD_REGISTER_NUMBER;

	spin_lock_irqsave(&ath_pci_lock, flags);
	switch (size) {
	case 1:
		addr = where & ~3;
		mask = 0xff000000 >> ((where % 4) * 8);
		tval = ath_reg_rd(crp_reg[bus] + addr);
		tval = tval & ~mask;
		*value = (tval >> ((4 - (where % 4))*8));
		break;
	case 2:
		addr = where & ~3;
		mask = 0xffff0000 >> ((where % 4)*8);
		tval = ath_reg_rd(crp_reg[bus] + addr);
		tval = tval & ~mask;
		*value = (tval >> ((4 - (where % 4))*8));
		break;
	case 4:
		*value = ath_reg_rd(crp_reg[bus] + where);
		break;
	default:
		spin_unlock_irqrestore(&ath_pci_lock, flags);
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	spin_unlock_irqrestore(&ath_pci_lock, flags);
	return PCIBIOS_SUCCESSFUL;
}

int
ath_local_write_config(int bus, int where, int size, uint32_t value)
{
	unsigned long	flags, addr, tval, mask;
	ATH_DECL_PCI_CRP_ARR(crp_reg);

	if (!ath_pci_link[bus]) {
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	/* Make sure the address is aligned to natural boundary */
	if (where & (size - 1))
		return PCIBIOS_BAD_REGISTER_NUMBER;

	spin_lock_irqsave(&ath_pci_lock, flags);
	switch (size) {
	case 1:
		addr = (crp_reg[bus] + where) & ~3;
		mask = 0xff000000 >> ((where % 4)*8);
		tval = ath_reg_rd(addr);
		tval = tval & ~mask;
		tval |= (value << ((4 - (where % 4))*8)) & mask;
		ath_reg_wr(addr,tval);
		break;
	case 2:
		addr = (crp_reg[bus] + where) & ~3;
		mask = 0xffff0000 >> ((where % 4)*8);
		tval = ath_reg_rd(addr);
		tval = tval & ~mask;
		tval |= (value << ((4 - (where % 4))*8)) & mask;
		ath_reg_wr(addr,tval);
		break;
	case 4:
		ath_reg_wr((crp_reg[bus] + where),value);
		break;
	default:
		spin_unlock_irqrestore(&ath_pci_lock, flags);
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	spin_unlock_irqrestore(&ath_pci_lock, flags);
	return PCIBIOS_SUCCESSFUL;
}

static int
ath_pci_read_config(struct pci_bus *bus, unsigned int devfn, int where,
			int size, uint32_t *value)
{
	unsigned long	flags, addr, tval, mask;
	ATH_DECL_PCI_CFG_BASE_ARR(cfg_reg);

	if (!ath_pci_link[bus->number] || devfn) {
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	/* Make sure the address is aligned to natural boundary */
	if (where & (size - 1))
		return PCIBIOS_BAD_REGISTER_NUMBER;

	spin_lock_irqsave(&ath_pci_lock, flags);
	switch (size) {
	case 1:
		addr = where & ~3;
		mask = 0xff000000 >> ((where % 4) * 8);
		tval = ath_reg_rd(cfg_reg[bus->number] + addr);
		tval = tval & ~mask;
		*value = (tval >> ((4 - (where % 4))*8));
		break;
	case 2:
		addr = where & ~3;
		mask = 0xffff0000 >> ((where % 4)*8);
		tval = ath_reg_rd(cfg_reg[bus->number] + addr);
		tval = tval & ~mask;
		*value = (tval >> ((4 - (where % 4))*8));
		break;
	case 4:
		*value = ath_reg_rd(cfg_reg[bus->number] + where);
		if (is_ar7240()) {
			/*
			 * WAR for BAR issue - We are unable to access
			 * the PCI device spac if we set the BAR with
			 * proper base address
			 */
			if(where == 0x10) {
				ath_reg_wr((cfg_reg[bus->number] + where),
						0xffff);
			}
		}
		break;
	default:
		spin_unlock_irqrestore(&ath_pci_lock, flags);
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	spin_unlock_irqrestore(&ath_pci_lock, flags);
	return PCIBIOS_SUCCESSFUL;
}

static int
ath_pci_write_config(struct pci_bus *bus, unsigned int devfn, int where,
			int size, uint32_t value)
{
	unsigned long	flags, tval, addr, mask;
	ATH_DECL_PCI_CFG_BASE_ARR(cfg_reg);

	if (!ath_pci_link[bus->number] || devfn) {
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	/* Make sure the address is aligned to natural boundary */
	if (where & (size - 1))
		return PCIBIOS_BAD_REGISTER_NUMBER;

	spin_lock_irqsave(&ath_pci_lock, flags);
	switch (size) {
	case 1:
		addr = (cfg_reg[bus->number] + where) & ~3;
		mask = 0xff000000 >> ((where % 4)*8);
		tval = ath_reg_rd(addr);
		tval = tval & ~mask;
		tval |= (value << ((4 - (where % 4))*8)) & mask;
		ath_reg_wr(addr,tval);
		break;
	case 2:
		addr = (cfg_reg[bus->number] + where) & ~3;
		mask = 0xffff0000 >> ((where % 4)*8);
		tval = ath_reg_rd(addr);
		tval = tval & ~mask;
		tval |= (value << ((4 - (where % 4))*8)) & mask;
		ath_reg_wr(addr,tval);
		break;
	case 4:
		ath_reg_wr((cfg_reg[bus->number] + where),value);
		break;
	default:
		spin_unlock_irqrestore(&ath_pci_lock, flags);
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	spin_unlock_irqrestore(&ath_pci_lock, flags);
	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops ath_pci_ops = {
	.read	= ath_pci_read_config,
	.write	= ath_pci_write_config,
};
