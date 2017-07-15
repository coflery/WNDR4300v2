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

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/fcntl.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/delay.h>
#include <asm/checksum.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/smp_lock.h>
#include <linux/wait.h>
#include <linux/irq.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/dmapool.h>
#include <linux/compile.h>
#include <linux/dma-mapping.h>

#include <atheros.h>

#define ATH_HWCS_BASE_ADDRESS		(ATH_APB_BASE + 0x400000)
#define ATH_HWCS_DMATX_CONTROL0		(ATH_HWCS_BASE_ADDRESS + 0x0000)
#define ATH_HWCS_DMATX_CONTROL1		(ATH_HWCS_BASE_ADDRESS + 0x0004)
#define ATH_HWCS_DMATX_CONTROL2		(ATH_HWCS_BASE_ADDRESS + 0x0008)
#define ATH_HWCS_DMATX_CONTROL3		(ATH_HWCS_BASE_ADDRESS + 0x000c)

#define ATH_HWCS_DMATX_DESC0		(ATH_HWCS_BASE_ADDRESS + 0x0010)
#define ATH_HWCS_DMATX_DESC1		(ATH_HWCS_BASE_ADDRESS + 0x0014)
#define ATH_HWCS_DMATX_DESC2		(ATH_HWCS_BASE_ADDRESS + 0x0018)
#define ATH_HWCS_DMATX_DESC3		(ATH_HWCS_BASE_ADDRESS + 0x001c)

#define ATH_HWCS_DMATX_DESC_STATUS 	(ATH_HWCS_BASE_ADDRESS + 0x0020)

#define ATH_HWCS_DMATX_ARB_CFG		(ATH_HWCS_BASE_ADDRESS + 0x0024)

#define ATH_HWCS_RR_PKTCNT01		(ATH_HWCS_BASE_ADDRESS + 0x0028)
#define ATH_HWCS_RR_PKTCNT23		(ATH_HWCS_BASE_ADDRESS + 0x002c)

#define ATH_HWCS_TXST_PKTCNT		(ATH_HWCS_BASE_ADDRESS + 0x0030)

#define ATH_HWCS_DMARX_CONTROL		(ATH_HWCS_BASE_ADDRESS + 0x0034)
#define ATH_HWCS_DMARX_DESC		(ATH_HWCS_BASE_ADDRESS + 0x0038)

#define ATH_HWCS_DMARX_DESC_STATUS 	(ATH_HWCS_BASE_ADDRESS + 0x003c)
#define ATH_HWCS_INTR			(ATH_HWCS_BASE_ADDRESS + 0x0040)
#define ATH_HWCS_IMASK			(ATH_HWCS_BASE_ADDRESS + 0x0044)
#define ATH_HWCS_ARB_BURST		(ATH_HWCS_BASE_ADDRESS + 0x0048)
#define ATH_HWCS_RESET_DMA		(ATH_HWCS_BASE_ADDRESS + 0x0050)

#define ATH_HWCS_DMARX_ENABLE		(1u)
#define ATH_HWCS_DMATX_ENABLE		(1u)
#define ATH_HWCS_PKT_VOID		(1u << 31)
#define ATH_HWCS_TX_EOF_MASK		(1u << 27)
#define ATH_HWCS_TX_SOF_MASK		(1u << 26)
#define ATH_HWCS_INTR_ENABLE		(1u << 25)
#define ATH_HWCS_INTR_STATUS		(1u << 30)
#define ATH_HWCS_TYPE_CSUM_ONLY		(0)
#define ATH_HWCS_TYPE_WITH_CP		(1u << 28)

#define ATH_HWCS_DMAPOOL_SIZE		(4 * 1024)

#define ATH_HWCS_MAGIC			101
#define ATH_HWCS_FRAG			_IO(ATH_HWCS_MAGIC, 0)
#define ATH_HWCS_DEFRAG			_IO(ATH_HWCS_MAGIC, 1)
#define ATH_HWCS_WITHOUT_FRAG		_IO(ATH_HWCS_MAGIC, 2)
#define ATH_HWCS_ONLY			_IO(ATH_HWCS_MAGIC, 3)

/*Enabling and disabling of interrupts*/

#define ATH_HWCS_RX_INTR_MASK		(1u << 3)
#define ATH_HWCS_TX_INTR_MASK		(1u << 13)
#define ATH_HWCS_TX_BUSERR_INTR_MASK	(1u << 6)
#define ATH_HWCS_RX_BUSERR_INTR_MASK	(1u << 2)

#define ATH_HWCS_RX_INTR_STATUS		(1u << 3)
#define ATH_HWCS_TX_INTR_STATUS		(1u << 13)

#define ATH_HWCS_RX_DESC_INTR		(1u << 2)
#define ATH_HWCS_TX_DESC_INTR		(1u << 5)

typedef struct {
	unsigned int	flags	: 13,
			pktSize	: 19;
} ath_hwcs_info_packed_t;

typedef union {
	ath_hwcs_info_packed_t		control;
	unsigned int			status;
} ath_hwcs_desc_info_t;

typedef struct {
	char				*buf;
	ath_hwcs_desc_info_t		info;
	void				*next;
	unsigned int			status_only;
	char				pad[16];
} ath_hwcs_desc_t __attribute__ ((aligned(0x20)));

ath_hwcs_desc_t	*ath_hwcs_tx_desc, *uncached_cksum_desc;
static int ath_start_csum = 0;

#ifdef CONFIG_ATH_HWCS_notyet
typedef struct {
	unsigned int	len,
			actual;
	unsigned short	checksum;
	char		*buf;
} ath_hwcs_node_info_t;

static DECLARE_WAIT_QUEUE_HEAD(wq);
static struct dma_pool *dmapool = NULL;

static noinline int check_pkt_void(volatile ath_hwcs_desc_t *desc)
{
	if((desc->info.status & ATH_HWCS_PKT_VOID) == 0)
		return 0;
	return 1;
}

int
do_defrag_hw(ath_hwcs_node_info_t *tx_entries, ath_hwcs_node_info_t *rx_entry, int num)
{
	int i;
	volatile ath_hwcs_desc_t *desc = NULL, *firstDesc = NULL;
	volatile ath_hwcs_desc_t *rx_desc = NULL;
	dma_addr_t rx_handle;
	dma_addr_t tx_handles[512];

	if(num > 512)
		return -EINVAL;

	if (rx_entry != NULL) {
		rx_desc = (ath_hwcs_desc_t *)dma_pool_alloc(dmapool,
				GFP_KERNEL, &rx_handle);
		rx_desc->buf = rx_entry->buf;
#ifdef CONFIG_ATH_HWCS_INT
		rx_desc->info.status = (0x0 | ATH_HWCS_PKT_VOID | ATH_HWCS_INTR_ENABLE);
		ath_reg_rmw_set(ATH_HWCS_IMASK, ATH_HWCS_RX_INTR_MASK);
#else
		rx_desc->info.status = (0x0 | ATH_HWCS_PKT_VOID);
#endif
		rx_desc->info.control.pktSize = rx_entry->len;
		rx_desc->next = (ath_hwcs_desc_t *)rx_desc;
	}

	for(i=0; i<num; i++) {
		if(NULL == desc) {

			desc = (ath_hwcs_desc_t *)dma_pool_alloc(dmapool,
					GFP_KERNEL, &tx_handles[i]);
			firstDesc = desc;
			desc->info.status = (0x0 | ATH_HWCS_TX_SOF_MASK);
		}
		else {
			desc->next = (ath_hwcs_desc_t *)dma_pool_alloc(dmapool,
					GFP_KERNEL, &tx_handles[i]);
			desc = desc->next;
			desc->info.status = (0x0);
		}
		desc->buf = tx_entries[i].buf;
		if(rx_entry != NULL)
			desc->info.status |= ATH_HWCS_TYPE_WITH_CP;
		desc->info.control.pktSize = tx_entries[i].len;
	}
	desc->next = (ath_hwcs_desc_t *)firstDesc;
#ifdef CONFIG_ATH_HWCS_INT
	if(rx_entry == NULL) {
		desc->info.status |= (ATH_HWCS_TX_EOF_MASK | ATH_HWCS_INTR_ENABLE);
		ath_reg_rmw_set(ATH_HWCS_IMASK, ATH_HWCS_TX_INTR_MASK);
	}
#else
	desc->info.status |= ATH_HWCS_TX_EOF_MASK;
#endif
	if (rx_entry != NULL) {
		ath_reg_wr(ATH_HWCS_DMARX_DESC, (unsigned int)rx_handle);
		ath_reg_wr(ATH_HWCS_DMARX_CONTROL, ATH_HWCS_DMARX_ENABLE);
	}
	ath_reg_wr(ATH_HWCS_DMATX_DESC0, (unsigned int)tx_handles[0]);
	ath_reg_wr(ATH_HWCS_DMATX_CONTROL0, ATH_HWCS_DMATX_ENABLE);

	if (rx_entry != NULL) {
#ifdef CONFIG_ATH_HWCS_INT
		wait_event_interruptible(wq, (check_pkt_void(rx_desc) == 0));
#else
		while ((check_pkt_void(rx_desc)) != 0);
#endif
		rx_entry->checksum = (rx_desc->info.control.pktSize & 0xffff);
		rx_entry->actual = (rx_desc->status_only & 0x7ffff);
	}
#ifdef CONFIG_ATH_HWCS_INT
	else {
		wait_event_interruptible(wq, (check_pkt_void(desc) == 1));
	}
#endif
	desc = firstDesc;
	for(i=0; i<num; i++) {
#ifndef CONFIG_ATH_HWCS_INT
		while ((check_pkt_void(desc)) == 0);
#endif
		tx_entries[i].checksum = (desc->info.control.pktSize & 0xffff);
		tx_entries[i].actual = (desc->status_only & 0x7ffff);
		if(tx_entries[i].actual < tx_entries[i].len)
			break;
		firstDesc = desc;
		desc = desc->next;
		dma_pool_free(dmapool, (void *)firstDesc, tx_handles[i]);
	}
	if (rx_entry != NULL) {
		dma_pool_free(dmapool, (void *)rx_desc, rx_handle);
	}
	return 0;
}

int
do_frag_hw(ath_hwcs_node_info_t *rx_entries, ath_hwcs_node_info_t *tx_entry, int num)
{
	int i;
	volatile ath_hwcs_desc_t *desc = NULL, *firstDesc = NULL;
	volatile ath_hwcs_desc_t *tx_desc = NULL;

	dma_addr_t tx_handle;
	dma_addr_t rx_handles[512];

	tx_desc = (ath_hwcs_desc_t *)dma_pool_alloc(dmapool,
			GFP_KERNEL, &tx_handle);

	tx_desc->buf = tx_entry->buf;
	if (num != 0) {
		tx_desc->info.status = (0x0 | ATH_HWCS_TX_SOF_MASK | ATH_HWCS_TX_EOF_MASK |
				ATH_HWCS_TYPE_WITH_CP);
	} else {
#ifdef CONFIG_ATH_HWCS_INT
		tx_desc->info.status = (0x0 | ATH_HWCS_TX_SOF_MASK | ATH_HWCS_TX_EOF_MASK |
				ATH_HWCS_TYPE_CSUM_ONLY |
				ATH_HWCS_INTR_ENABLE);
		ath_reg_rmw_set(ATH_HWCS_IMASK, ATH_HWCS_TX_INTR_MASK);
#else
		tx_desc->info.status = (0x0 | ATH_HWCS_TX_SOF_MASK | ATH_HWCS_TX_EOF_MASK |
				ATH_HWCS_TYPE_CSUM_ONLY);
#endif
	}

	tx_desc->info.control.pktSize = tx_entry->len;
	tx_desc->next = (ath_hwcs_desc_t *)tx_desc;

	for(i=0; i<num; i++) {
		if(NULL == desc) {
			desc = (ath_hwcs_desc_t *)dma_pool_alloc(dmapool,
					GFP_KERNEL, &rx_handles[i]);
			firstDesc = desc;
		}
		else {
			desc->next = (ath_hwcs_desc_t *)dma_pool_alloc(dmapool, GFP_KERNEL, &rx_handles[i]);
			desc = desc->next;
		}
		desc->buf = rx_entries[i].buf;
		desc->info.status = (0x0 | ATH_HWCS_PKT_VOID);
		desc->info.control.pktSize = rx_entries[i].len;
	}
	if (num != 0) {
		desc->next = (ath_hwcs_desc_t *)firstDesc;
#ifdef CONFIG_ATH_HWCS_INT
		ath_reg_rmw_set(ATH_HWCS_IMASK, ATH_HWCS_RX_INTR_MASK);
		desc->info.status |= ATH_HWCS_INTR_ENABLE;
#endif
		ath_reg_wr(ATH_HWCS_DMARX_DESC, (unsigned int)rx_handles[0]);
		ath_reg_wr(ATH_HWCS_DMARX_CONTROL, ATH_HWCS_DMARX_ENABLE);
	}
	ath_reg_wr(ATH_HWCS_DMATX_DESC0, (unsigned int)tx_handle);
	ath_reg_wr(ATH_HWCS_DMATX_CONTROL0, ATH_HWCS_DMATX_ENABLE);
#ifdef CONFIG_ATH_HWCS_INT
	if(num == 0)
		wait_event_interruptible(wq, (check_pkt_void(tx_desc) == 1));
#else
	while ((check_pkt_void(tx_desc)) == 0);
#endif
	if (num != 0) {

#ifdef CONFIG_ATH_HWCS_INT
		wait_event_interruptible(wq, (check_pkt_void(desc) == 0));
#endif
		desc = firstDesc;
		for(i=0; i<num; i++) {
#ifndef CONFIG_ATH_HWCS_INT
			while ((check_pkt_void(desc)) != 0);
#endif
			rx_entries[i].checksum = (desc->info.control.pktSize & 0xffff);
			rx_entries[i].actual = (desc->status_only & 0x7ffff);
#if 0
			if(rx_entries[i].actual < rx_entries[i].len) {
				retval = -1;
				break;
			}
#endif
			firstDesc = desc;
			desc = desc->next;
			dma_pool_free(dmapool, (void *)firstDesc, rx_handles[i]);
		}
	}
	tx_entry->checksum = (tx_desc->info.control.pktSize & 0xffff);
	tx_entry->actual = (tx_desc->status_only & 0x7ffff);
	dma_pool_free(dmapool, (void *)tx_desc, tx_handle);
	return 0;
}
#endif

/*
 * Start checksum engine
 */
__sum16 ath_hwcs_start(void *buf, int len)
{
	// Initialize descriptor with buffer address, packet size
	//ath_hwcs_tx_desc->buf = (char *)dma_map_single(NULL, buf, len, DMA_TO_DEVICE);
	ath_hwcs_tx_desc->buf = (char *)virt_to_phys(buf);
	ath_hwcs_tx_desc->status_only = 0;
	ath_hwcs_tx_desc->info.status = (ATH_HWCS_TX_SOF_MASK | ATH_HWCS_TX_EOF_MASK | ATH_HWCS_INTR_ENABLE);
	ath_hwcs_tx_desc->info.control.pktSize = len;

	dma_cache_wback((unsigned long)ath_hwcs_tx_desc, sizeof(ath_hwcs_desc_t));
	udelay(1); /* delay is required to get the status properly between cache flush and DMA enable */ 
	// Enable DMA packet transfer
	ath_reg_wr(ATH_HWCS_DMATX_CONTROL0, ATH_HWCS_DMATX_ENABLE);
	ath_start_csum = 1;

	return 0;
}

inline unsigned short
ath_hwcs_get_csum_from_desc(ath_hwcs_desc_t *d)
{
#if 0
	dma_cache_inv((unsigned long)ath_hwcs_tx_desc, sizeof(ath_hwcs_desc_t));
	return (unsigned short)((ath_hwcs_tx_desc->info.control.pktSize) & 0xffff);
#else
	return (unsigned short)((uncached_cksum_desc->info.control.pktSize) & 0xffff);
#endif
}

unsigned
ath_hwcs_get(void)
{
	unsigned status;

	if (!ath_start_csum)
		return 0x0bad0000u;

	// Wait for masked intr
	while (!((status = ath_reg_rd(ATH_HWCS_DMATX_DESC_STATUS)) & ATH_HWCS_TX_DESC_INTR));

	// ack the intr
	ath_reg_wr(ATH_HWCS_DMATX_DESC_STATUS, status & ATH_HWCS_TX_DESC_INTR);
	ath_start_csum = 0;

#ifdef CONFIG_ATH_HWCS_debug
	if (((uncached_cksum_desc->info.control.pktSize) & 0xffff) !=
		((ath_hwcs_tx_desc->info.control.pktSize) & 0xffff)) {
		printk("cache prob uncached read 0x%x cached read 0x%x\n",
			((uncached_cksum_desc->info.control.pktSize) & 0xffff),
			((ath_hwcs_tx_desc->info.control.pktSize) & 0xffff));
	}
#endif

	return ath_hwcs_get_csum_from_desc(ath_hwcs_tx_desc);
}

int ath_hwcs_init(void)
{
	dma_addr_t pa;

#ifdef CONFIG_ATH_HWCS_notyet
	if (!dmapool) {
		dmapool = dma_pool_create("csum_hw_accel", NULL, sizeof(ath_hwcs_desc_t),
					(size_t)4, (size_t)ATH_HWCS_DMAPOOL_SIZE);

		if (!dmapool)
			return -1;
	}
#endif

	ath_hwcs_tx_desc = kmalloc(sizeof(ath_hwcs_desc_t), GFP_DMA);
	// Setup checksum descriptor
	pa = dma_map_single(NULL, ath_hwcs_tx_desc, sizeof(ath_hwcs_desc_t), DMA_TO_DEVICE);
	ath_hwcs_tx_desc->next = (ath_hwcs_desc_t *)pa;
	uncached_cksum_desc = (ath_hwcs_desc_t *)KSEG1ADDR(virt_to_phys(ath_hwcs_tx_desc));

	// Weight for channels
	ath_reg_wr(ATH_HWCS_DMATX_ARB_CFG, (63 << 8));

	// Tx checksum interrupt mask
	ath_reg_rmw_set(ATH_HWCS_IMASK, ATH_HWCS_TX_INTR_MASK);

	// Initialize Tx descriptor address
	ath_reg_wr(ATH_HWCS_DMATX_DESC0, pa);

	printk("%s: Init done ...\n", __func__);

	return 0;
}

#ifdef CONFIG_ATH_HWCS_notyet
void ath_hwcs_exit(void)
{
	dma_pool_destroy(dmapool);
	kfree(ath_hwcs_tx_desc);
}
#endif

late_initcall(ath_hwcs_init);
