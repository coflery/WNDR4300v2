/*
 * Copyright (C) 2001-2004 by David Brownell
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* this file is part of ehci-hcd.c */

/*-------------------------------------------------------------------------*/

/*
 * EHCI hardware queue manipulation ... the core.  QH/QTD manipulation.
 *
 * Control, bulk, and interrupt traffic all use "qh" lists.  They list "qtd"
 * entries describing USB transactions, max 16-20kB/entry (with 4kB-aligned
 * buffers needed for the larger number).  We use one QH per endpoint, queue
 * multiple urbs (all three types) per endpoint.  URBs may need several qtds.
 *
 * ISO traffic uses "ISO TD" (itd, and sitd) records, and (along with
 * interrupts) needs careful scheduling.  Performance improvements can be
 * an ongoing challenge.  That's in "ehci-sched.c".
 *
 * USB 1.1 devices are handled (a) by "companion" OHCI or UHCI root hubs,
 * or otherwise through transaction translators (TTs) in USB 2.0 hubs using
 * (b) special fields in qh entries or (c) split iso entries.  TTs will
 * buffer low/full speed data so the host collects it at high speed.
 */

/*-------------------------------------------------------------------------*/

/* fill a qtd, returning how much of the buffer we were able to queue up */

#ifdef CONFIG_ATH_USB_DMA_TO_SRAM

#if defined(CONFIG_MACH_QCA953x) || \
    defined(CONFIG_MACH_QCA956x)
#define ATH_SRAM_SIZE           (8 << 10)
#define ATH_MAX_SIZE_PER_QTD    (4 << 10)
#define ATH_MAX_SIZE_PER_BUF    (1 << 10)
#else
#define ATH_SRAM_SIZE		(32 << 10)
#define ATH_MAX_SIZE_PER_QTD	(16 << 10)
#define ATH_MAX_SIZE_PER_BUF	( 4 << 10)
#endif

#if CONFIG_ATH_USB_SRAM_NUM_IO < 1	|| \
    CONFIG_ATH_USB_SRAM_NUM_IO > (ATH_SRAM_SIZE / ATH_MAX_SIZE_PER_QTD)
#	error "Incorrect setting for CONFIG_ATH_USB_SRAM_NUM_IO"
#endif

#define ATH_BULK_IN_MASK	(0xc0000000u | USB_DIR_IN)
#define ath_usb_bulk_in(p)	(((p) & ATH_BULK_IN_MASK) == ATH_BULK_IN_MASK)
#define ath_sram_bulk_in(u)	(ath_usb_bulk_in((u)->pipe) && \
				 ((u)->transfer_buffer_length > 256))
#define ATH_SRAM_DMA_BASE	((dma_addr_t)0x1d000000u)

/*
 * Size of partition for each i/o. The qTD buffer pointers
 * keep wrapping around within this.
 */
#define ATH_SRAM_PART_SIZE	(ATH_SRAM_SIZE / CONFIG_ATH_USB_SRAM_NUM_IO)

typedef struct {
	struct urb		*urb;
	unsigned char		*tb;
	dma_addr_t		start,
				urb_next,
				qtd_next;
	int			pending;
	//struct ehci_qtd		*qtd;
	wait_queue_head_t	free;
} ath_qtd_urb_t;

ath_qtd_urb_t	ath_qtd_urb[CONFIG_ATH_USB_SRAM_NUM_IO];

#define ATH_AQU_SRAM_BASE(i)	(ATH_SRAM_DMA_BASE + (ATH_SRAM_PART_SIZE * i))

DECLARE_WAIT_QUEUE_HEAD(aqu_free);
#if 1
static atomic_t aqu_inuse = ATOMIC_INIT(0);
#define count_read(a)	atomic_read(&a)
#define count_inc(a)	atomic_inc(&a)
#define count_dec(a)	atomic_dec(&a)
#else
static unsigned aqu_inuse;
#define count_read(a)	(a)
#define count_inc(a)	(a ++)
#define count_dec(a)	(a --)
#endif

#define ATH_NUM_QTD_URB		(sizeof(ath_qtd_urb) / sizeof(ath_qtd_urb[0]))

// For debug purpose.
// Can get the address from /proc/kallsyms and do md/mm
EXPORT_SYMBOL(ath_qtd_urb);
EXPORT_SYMBOL(aqu_inuse);

#define use_chksum_acc	1

#if use_chksum_acc
typedef struct {
	char		*buf;

// PktVoid[31]		Set to 1 to discard the desc and stop the chain      
// EndOfFrame[27]	Last byte of this buffer marks the end of the segment
// StrtOfFrame[26]	Start of segment
// OffType[30:28]	00 : Checksum only, 01 : Rx Dma
// PktSize[18:0]	Size of packet

	uint32_t	ctrl;
#define CHKSUM_ACC_PKT_VOID	(1 << 31)
#define CHKSUM_ACC_EOF		(1 << 27)
#define CHKSUM_ACC_SOF		(1 << 26)
#define CHKSUM_ACC_TYPE(a)	(((a) << 28) & (0x7 << 28))
#define CHKSUM_ACC_PKT_SIZE(x)	(((1 << 19) - 1) & (x))

#define CHKSUM_ACC_TX_CTRL	((CHKSUM_ACC_EOF | CHKSUM_ACC_SOF | CHKSUM_ACC_TYPE(1)) & (~CHKSUM_ACC_PKT_VOID))
#define CHKSUM_ACC_RX_CTRL	(CHKSUM_ACC_EOF | CHKSUM_ACC_SOF | CHKSUM_ACC_PKT_VOID)

	void		*next;
	uint32_t 	status;
	char		pad[16];
} ath_hwcs_desc_t __attribute__ ((aligned(0x20)));

ath_hwcs_desc_t __cs_tx, __cs_rx;

volatile ath_hwcs_desc_t *cs_tx, *cs_rx;

#define CHKSUM_ACC_DMATX_CONTROL0_ADDRESS		0x18400000
#define CHKSUM_ACC_DMATX_DESC0_ADDRESS			0x18400010
#define CHKSUM_ACC_DMATX_DESC_STATUS_ADDRESS		0x18400020
#define CHKSUM_ACC_DMARX_CONTROL_ADDRESS		0x18400034
#define CHKSUM_ACC_DMARX_DESC_ADDRESS			0x18400038
#define CHKSUM_ACC_DMARX_DESC_STATUS_ADDRESS		0x1840003c
#define CHKSUM_ACC_DMARX_DESC_STATUS_DESC_INTR_MASK	0x00000004

int	first_cs_done;

#endif

static void ath_init_qtd_urb(void)
{
	int		i;
	ath_qtd_urb_t	*aqu;

	// Following is not needed since it is in bss?
	// memset(ath_qtd_urb, 0, sizeof(ath_qtd_urb));

	aqu = ath_qtd_urb;
	for (i = 0; (i < ATH_NUM_QTD_URB); i++, aqu++) {
		init_waitqueue_head(&aqu->free);
	}

#if use_chksum_acc
	cs_tx = (void *)KSEG1ADDR(&__cs_tx),
	cs_rx = (void *)KSEG1ADDR(&__cs_rx);
	__cs_tx.next = (void *)virt_to_phys(&__cs_tx);
	__cs_rx.next = (void *)virt_to_phys(&__cs_rx);

	cs_tx->ctrl = CHKSUM_ACC_TX_CTRL;
	cs_rx->ctrl = CHKSUM_ACC_RX_CTRL;
#endif
}

static ath_qtd_urb_t * ath_get_aqu(struct ehci_qtd *qtd)
{
	int		i;
	ath_qtd_urb_t	*aqu;
	struct urb	*urb = qtd->urb;

retry:
	aqu = ath_qtd_urb;

	wait_event(aqu_free, (count_read(aqu_inuse) < ATH_NUM_QTD_URB));

	for (i = 0; (i < ATH_NUM_QTD_URB) && aqu->urb; i++, aqu++) {
		if (aqu->urb->dev == urb->dev) {
			if (aqu->urb->transfer_buffer == urb->transfer_buffer) {
				printk("[%d-%s]", aqu - ath_qtd_urb, urb->dev->devpath);
				return aqu;
			}
			wait_event(aqu->free, !aqu->urb);
			/*
			 * This device has completed an i/o now. Retry from the
			 * beginning. Try to give a chance to other devices too
			 * if any.
			 */
			goto retry;
		}
	}

	if (i == ATH_NUM_QTD_URB) {
		goto retry;
	}

	aqu->urb = urb;
	aqu->start =
	aqu->urb_next =
	aqu->qtd_next = ATH_AQU_SRAM_BASE(i);
	aqu->pending = urb->transfer_buffer_length;
	aqu->tb = urb->transfer_buffer;
	count_inc(aqu_inuse);
	printk("(%d-%s)", aqu - ath_qtd_urb, urb->dev->devpath);
	return aqu;
}

static ath_qtd_urb_t * ath_urb_to_aqu(struct urb *urb)
{
	int		i;
	ath_qtd_urb_t	*aqu;

	aqu = ath_qtd_urb;
	for (i = 0; (i < ATH_NUM_QTD_URB); i++, aqu++) {
		if (aqu->urb == urb) {
			return aqu;
		}
	}
	return NULL;
}

#define ath_qtd_to_aqu(qtd)	ath_urb_to_aqu(qtd->urb)

#define ath_aqu_to_urb(aqu)	((aqu)->urb)

#define ath_inc_sram_addr(a, f)					\
do {								\
	(a)->f += ATH_MAX_SIZE_PER_BUF;				\
	if ((a)->f >= ((a)->start + ATH_SRAM_PART_SIZE)) {	\
		(a)->f = (a)->start;				\
	}							\
} while (0)

static inline ath_qtd_urb_t * ath_get_sram(struct ehci_qtd *qtd)
{
	ath_qtd_urb_t	*aqu = ath_get_aqu(qtd);

	return aqu;

}

#define copy_va(a)	((unsigned char *)KSEG1ADDR(a))
static inline void ath_purge_aqu(ath_qtd_urb_t *aqu)
{
	if (!aqu) {
		return;
	}
	printk("<%d-%s>", aqu - ath_qtd_urb, aqu->urb->dev->devpath);
	aqu->urb = NULL;
	aqu->start =
	aqu->urb_next =
	aqu->qtd_next = 0;
	aqu->pending = 0;
	aqu->tb = NULL;
	count_dec(aqu_inuse);
	wake_up(&aqu->free);
	wake_up(&aqu_free);
	return;
}
static inline int ath_copy_to_urb(ath_qtd_urb_t *aqu, struct ehci_hcd *ehci)
{
	dma_addr_t	d = aqu->urb_next;
	unsigned	len;

	if (aqu->pending > ATH_MAX_SIZE_PER_QTD) {
		len = ATH_MAX_SIZE_PER_QTD;
	} else {
		len = aqu->pending;
	}
#if use_chksum_acc
	if (first_cs_done) {
		while (!(cs_rx->status & (1 << 31)));
		first_cs_done = 1;
	}

	cs_tx->buf = (void *)d;
	cs_tx->ctrl = CHKSUM_ACC_TX_CTRL | CHKSUM_ACC_PKT_SIZE(len);

	cs_rx->buf = (void *)(aqu->urb->transfer_dma + (aqu->tb - (unsigned char *)aqu->urb->transfer_buffer));
	cs_rx->ctrl = CHKSUM_ACC_RX_CTRL | CHKSUM_ACC_PKT_SIZE(len);

	ath_reg_wr(CHKSUM_ACC_DMATX_DESC0_ADDRESS, virt_to_phys(&__cs_tx));
	ath_reg_wr(CHKSUM_ACC_DMARX_DESC_ADDRESS, virt_to_phys(&__cs_rx));
        ath_reg_wr(CHKSUM_ACC_DMATX_CONTROL0_ADDRESS, 0x1); // enable dma tx
        ath_reg_wr(CHKSUM_ACC_DMARX_CONTROL_ADDRESS, 0x1); // enable dma rx
#else
	memcpy(copy_va(aqu->tb), copy_va(d), len);
#endif
	aqu->tb += len;
	ath_inc_sram_addr(aqu, urb_next);
	aqu->pending -= len;
	if (aqu->pending <= 0) {
		ath_purge_aqu(aqu);
		return 0;
	}
	return 1;
}

static inline int ath_per_qtd_ioc(struct ehci_qh *qh, struct ehci_hcd *ehci)
{
	struct ehci_qtd		*qtd;
	struct urb		*urb;
	ath_qtd_urb_t		*aqu;
	if (list_empty (&qh->qtd_list)) {
		return 0;
	}

	qtd = list_first_entry (&qh->qtd_list, struct ehci_qtd, qtd_list);
	urb = qtd->urb;

	if (!ath_sram_bulk_in(urb)) {
		return 0;
	}
	aqu = ath_urb_to_aqu(urb);
	if (aqu) {
		return ath_copy_to_urb(aqu, ehci);
	}
	return 0;
}
#endif	/* CONFIG_ATH_USB_DMA_TO_SRAM */

static int
qtd_fill(struct ehci_hcd *ehci, struct ehci_qtd *qtd, dma_addr_t buf,
		  size_t len, int token, int maxpacket)
{
	int	i, count;
	u64	addr = buf;

#ifdef CONFIG_ATH_USB_DMA_TO_SRAM
#	define buf_per_qtd	(ATH_MAX_SIZE_PER_QTD / ATH_MAX_SIZE_PER_BUF)
	ath_qtd_urb_t	*aqu = NULL;

	if (ath_sram_bulk_in(qtd->urb)) {
		aqu = ath_get_sram(qtd);
		buf = aqu->qtd_next;
		ath_inc_sram_addr(aqu, qtd_next);
	}
	addr = buf;
#else
#	define buf_per_qtd	5
#endif

	/* one buffer entry per 4K ... first might be short or unaligned */
	qtd->hw_buf[0] = cpu_to_hc32(ehci, (u32)addr);
	qtd->hw_buf_hi[0] = cpu_to_hc32(ehci, (u32)(addr >> 32));
	count = 0x1000 - (buf & 0x0fff);	/* rest of that page */
	if (likely (len < count))		/* ... iff needed */
		count = len;
	else {
#ifdef CONFIG_ATH_USB_DMA_TO_SRAM
		if (aqu) {
			buf = aqu->qtd_next;
			ath_inc_sram_addr(aqu, qtd_next);
		} else {
#endif
			buf +=  0x1000;
			buf &= ~0x0fff;
#ifdef CONFIG_ATH_USB_DMA_TO_SRAM
		}
#endif

		/* per-qtd limit: from 16K to 20K (best alignment) */
		for (i = 1; count < len && i < buf_per_qtd; i++) {
			addr = buf;
			qtd->hw_buf[i] = cpu_to_hc32(ehci, (u32)addr);
			qtd->hw_buf_hi[i] = cpu_to_hc32(ehci,
					(u32)(addr >> 32));
#ifdef CONFIG_ATH_USB_DMA_TO_SRAM
			if (aqu) {
				buf = aqu->qtd_next;
				ath_inc_sram_addr(aqu, qtd_next);
			} else {
#endif
				buf += 0x1000;
#ifdef CONFIG_ATH_USB_DMA_TO_SRAM
			}
#endif
			if ((count + 0x1000) < len)
				count += 0x1000;
			else
				count = len;
		}

		/* short packets may only terminate transfers */
		if (count != len)
			count -= (count % maxpacket);
	}
	qtd->hw_token = cpu_to_hc32(ehci, (count << 16) | token
#ifdef CONFIG_ATH_USB_DMA_TO_SRAM
							| QTD_IOC
#endif
			);
	qtd->length = count;

	return count;
}

/*-------------------------------------------------------------------------*/

static inline void
qh_update (struct ehci_hcd *ehci, struct ehci_qh *qh, struct ehci_qtd *qtd)
{
	/* writes to an active overlay are unsafe */
	BUG_ON(qh->qh_state != QH_STATE_IDLE);

	qh->hw_qtd_next = QTD_NEXT(ehci, qtd->qtd_dma);
	qh->hw_alt_next = EHCI_LIST_END(ehci);

	/* Except for control endpoints, we make hardware maintain data
	 * toggle (like OHCI) ... here (re)initialize the toggle in the QH,
	 * and set the pseudo-toggle in udev. Only usb_clear_halt() will
	 * ever clear it.
	 */
	if (!(qh->hw_info1 & cpu_to_hc32(ehci, 1 << 14))) {
		unsigned	is_out, epnum;

		is_out = !(qtd->hw_token & cpu_to_hc32(ehci, 1 << 8));
		epnum = (hc32_to_cpup(ehci, &qh->hw_info1) >> 8) & 0x0f;
		if (unlikely (!usb_gettoggle (qh->dev, epnum, is_out))) {
			qh->hw_token &= ~cpu_to_hc32(ehci, QTD_TOGGLE);
			usb_settoggle (qh->dev, epnum, is_out, 1);
		}
	}

	/* HC must see latest qtd and qh data before we clear ACTIVE+HALT */
	wmb ();
	qh->hw_token &= cpu_to_hc32(ehci, QTD_TOGGLE | QTD_STS_PING);
}

/* if it weren't for a common silicon quirk (writing the dummy into the qh
 * overlay, so qh->hw_token wrongly becomes inactive/halted), only fault
 * recovery (including urb dequeue) would need software changes to a QH...
 */
static void
qh_refresh (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	struct ehci_qtd *qtd;

	if (list_empty (&qh->qtd_list))
		qtd = qh->dummy;
	else {
		qtd = list_entry (qh->qtd_list.next,
				struct ehci_qtd, qtd_list);
		/* first qtd may already be partially processed */
		if (cpu_to_hc32(ehci, qtd->qtd_dma) == qh->hw_current)
			qtd = NULL;
	}

	if (qtd)
		qh_update (ehci, qh, qtd);
}

/*-------------------------------------------------------------------------*/

static void qh_link_async(struct ehci_hcd *ehci, struct ehci_qh *qh);

static void ehci_clear_tt_buffer_complete(struct usb_hcd *hcd,
		struct usb_host_endpoint *ep)
{
	struct ehci_hcd		*ehci = hcd_to_ehci(hcd);
	struct ehci_qh		*qh = ep->hcpriv;
	unsigned long		flags;

	spin_lock_irqsave(&ehci->lock, flags);
	qh->clearing_tt = 0;
	if (qh->qh_state == QH_STATE_IDLE && !list_empty(&qh->qtd_list)
			&& HC_IS_RUNNING(hcd->state))
		qh_link_async(ehci, qh);
	spin_unlock_irqrestore(&ehci->lock, flags);
}

static void ehci_clear_tt_buffer(struct ehci_hcd *ehci, struct ehci_qh *qh,
		struct urb *urb, u32 token)
{

	/* If an async split transaction gets an error or is unlinked,
	 * the TT buffer may be left in an indeterminate state.  We
	 * have to clear the TT buffer.
	 *
	 * Note: this routine is never called for Isochronous transfers.
	 */
	if (urb->dev->tt && !usb_pipeint(urb->pipe) && !qh->clearing_tt) {
#ifdef DEBUG
		struct usb_device *tt = urb->dev->tt->hub;
		dev_dbg(&tt->dev,
			"clear tt buffer port %d, a%d ep%d t%08x\n",
			urb->dev->ttport, urb->dev->devnum,
			usb_pipeendpoint(urb->pipe), token);
#endif /* DEBUG */
		if (!ehci_is_TDI(ehci)
				|| urb->dev->tt->hub !=
				   ehci_to_hcd(ehci)->self.root_hub) {
			if (usb_hub_clear_tt_buffer(urb) == 0)
				qh->clearing_tt = 1;
		} else {

			/* REVISIT ARC-derived cores don't clear the root
			 * hub TT buffer in this way...
			 */
		}
	}
}

static int qtd_copy_status (
	struct ehci_hcd *ehci,
	struct urb *urb,
	size_t length,
	u32 token
)
{
	int	status = -EINPROGRESS;

	/* count IN/OUT bytes, not SETUP (even short packets) */
	if (likely (QTD_PID (token) != 2))
		urb->actual_length += length - QTD_LENGTH (token);

	/* don't modify error codes */
	if (unlikely(urb->unlinked))
		return status;

	/* force cleanup after short read; not always an error */
	if (unlikely (IS_SHORT_READ (token)))
		status = -EREMOTEIO;

	/* serious "can't proceed" faults reported by the hardware */
	if (token & QTD_STS_HALT) {
		if (token & QTD_STS_BABBLE) {
			/* FIXME "must" disable babbling device's port too */
			status = -EOVERFLOW;
		/* CERR nonzero + halt --> stall */
		} else if (QTD_CERR(token)) {
			status = -EPIPE;

		/* In theory, more than one of the following bits can be set
		 * since they are sticky and the transaction is retried.
		 * Which to test first is rather arbitrary.
		 */
		} else if (token & QTD_STS_MMF) {
			/* fs/ls interrupt xfer missed the complete-split */
			status = -EPROTO;
		} else if (token & QTD_STS_DBE) {
			status = (QTD_PID (token) == 1) /* IN ? */
				? -ENOSR  /* hc couldn't read data */
				: -ECOMM; /* hc couldn't write data */
		} else if (token & QTD_STS_XACT) {
			/* timeout, bad CRC, wrong PID, etc */
			ehci_dbg(ehci, "devpath %s ep%d%s 3strikes\n",
				urb->dev->devpath,
				usb_pipeendpoint(urb->pipe),
				usb_pipein(urb->pipe) ? "in" : "out");
			status = -EPROTO;
		} else {	/* unknown */
			status = -EPROTO;
		}

		ehci_vdbg (ehci,
			"dev%d ep%d%s qtd token %08x --> status %d\n",
			usb_pipedevice (urb->pipe),
			usb_pipeendpoint (urb->pipe),
			usb_pipein (urb->pipe) ? "in" : "out",
			token, status);
	}

	return status;
}

static void
ehci_urb_done(struct ehci_hcd *ehci, struct urb *urb, int status)
__releases(ehci->lock)
__acquires(ehci->lock)
{
	if (likely (urb->hcpriv != NULL)) {
		struct ehci_qh	*qh = (struct ehci_qh *) urb->hcpriv;

		/* S-mask in a QH means it's an interrupt urb */
		if ((qh->hw_info2 & cpu_to_hc32(ehci, QH_SMASK)) != 0) {

			/* ... update hc-wide periodic stats (for usbfs) */
			ehci_to_hcd(ehci)->self.bandwidth_int_reqs--;
		}
		qh_put (qh);
	}

	if (unlikely(urb->unlinked)) {
		COUNT(ehci->stats.unlink);
	} else {
		/* report non-error and short read status as zero */
		if (status == -EINPROGRESS || status == -EREMOTEIO)
			status = 0;
		COUNT(ehci->stats.complete);
	}

#ifdef EHCI_URB_TRACE
	ehci_dbg (ehci,
		"%s %s urb %p ep%d%s status %d len %d/%d\n",
		__func__, urb->dev->devpath, urb,
		usb_pipeendpoint (urb->pipe),
		usb_pipein (urb->pipe) ? "in" : "out",
		status,
		urb->actual_length, urb->transfer_buffer_length);
#endif

	/* complete() can reenter this HCD */
	usb_hcd_unlink_urb_from_ep(ehci_to_hcd(ehci), urb);
	spin_unlock (&ehci->lock);
	usb_hcd_giveback_urb(ehci_to_hcd(ehci), urb, status);
	spin_lock (&ehci->lock);
}

static void start_unlink_async (struct ehci_hcd *ehci, struct ehci_qh *qh);
static void unlink_async (struct ehci_hcd *ehci, struct ehci_qh *qh);

static void intr_deschedule (struct ehci_hcd *ehci, struct ehci_qh *qh);
static int qh_schedule (struct ehci_hcd *ehci, struct ehci_qh *qh);

/*
 * Process and free completed qtds for a qh, returning URBs to drivers.
 * Chases up to qh->hw_current.  Returns number of completions called,
 * indicating how much "real" work we did.
 */
static unsigned
qh_completions (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	struct ehci_qtd		*last = NULL, *end = qh->dummy;
	struct list_head	*entry, *tmp;
	int			last_status = -EINPROGRESS;
	int			stopped;
	unsigned		count = 0;
	u8			state;
	__le32			halt = HALT_BIT(ehci);

	if (unlikely (list_empty (&qh->qtd_list)))
		return count;

	/* completions (or tasks on other cpus) must never clobber HALT
	 * till we've gone through and cleaned everything up, even when
	 * they add urbs to this qh's queue or mark them for unlinking.
	 *
	 * NOTE:  unlinking expects to be done in queue order.
	 */
	state = qh->qh_state;
	qh->qh_state = QH_STATE_COMPLETING;
	stopped = (state == QH_STATE_IDLE);

	/* remove de-activated QTDs from front of queue.
	 * after faults (including short reads), cleanup this urb
	 * then let the queue advance.
	 * if queue is stopped, handles unlinks.
	 */
	list_for_each_safe (entry, tmp, &qh->qtd_list) {
		struct ehci_qtd	*qtd;
		struct urb	*urb;
		u32		token = 0;

		qtd = list_entry (entry, struct ehci_qtd, qtd_list);
		urb = qtd->urb;

		/* clean up any state from previous QTD ...*/
		if (last) {
			if (likely (last->urb != urb)) {
				ehci_urb_done(ehci, last->urb, last_status);
				count++;
				last_status = -EINPROGRESS;
			}
			ehci_qtd_free (ehci, last);
			last = NULL;
		}

		/* ignore urbs submitted during completions we reported */
		if (qtd == end)
			break;

		/* hardware copies qtd out of qh overlay */
		rmb ();
		token = hc32_to_cpu(ehci, qtd->hw_token);

		/* always clean up qtds the hc de-activated */
 retry_xacterr:
		if ((token & QTD_STS_ACTIVE) == 0) {

			/* on STALL, error, and short reads this urb must
			 * complete and all its qtds must be recycled.
			 */
			if ((token & QTD_STS_HALT) != 0) {

				/* retry transaction errors until we
				 * reach the software xacterr limit
				 */
				if ((token & QTD_STS_XACT) &&
						QTD_CERR(token) == 0 &&
						++qh->xacterrs < QH_XACTERR_MAX &&
						!urb->unlinked) {
					ehci_dbg(ehci,
	"detected XactErr len %zu/%zu retry %d\n",
	qtd->length - QTD_LENGTH(token), qtd->length, qh->xacterrs);

					/* reset the token in the qtd and the
					 * qh overlay (which still contains
					 * the qtd) so that we pick up from
					 * where we left off
					 */
					token &= ~QTD_STS_HALT;
					token |= QTD_STS_ACTIVE |
							(EHCI_TUNE_CERR << 10);
					qtd->hw_token = cpu_to_hc32(ehci,
							token);
					wmb();
					qh->hw_token = cpu_to_hc32(ehci, token);
					goto retry_xacterr;
				}
				stopped = 1;

			/* magic dummy for some short reads; qh won't advance.
			 * that silicon quirk can kick in with this dummy too.
			 *
			 * other short reads won't stop the queue, including
			 * control transfers (status stage handles that) or
			 * most other single-qtd reads ... the queue stops if
			 * URB_SHORT_NOT_OK was set so the driver submitting
			 * the urbs could clean it up.
			 */
			} else if (IS_SHORT_READ (token)
					&& !(qtd->hw_alt_next
						& EHCI_LIST_END(ehci))) {
				stopped = 1;
				goto halt;
			}

		/* stop scanning when we reach qtds the hc is using */
		} else if (likely (!stopped
				&& HC_IS_RUNNING (ehci_to_hcd(ehci)->state))) {
			break;

		/* scan the whole queue for unlinks whenever it stops */
		} else {
			stopped = 1;

			/* cancel everything if we halt, suspend, etc */
			if (!HC_IS_RUNNING(ehci_to_hcd(ehci)->state))
				last_status = -ESHUTDOWN;

			/* this qtd is active; skip it unless a previous qtd
			 * for its urb faulted, or its urb was canceled.
			 */
			else if (last_status == -EINPROGRESS && !urb->unlinked)
				continue;

			/* qh unlinked; token in overlay may be most current */
			if (state == QH_STATE_IDLE
					&& cpu_to_hc32(ehci, qtd->qtd_dma)
						== qh->hw_current) {
				token = hc32_to_cpu(ehci, qh->hw_token);

				/* An unlink may leave an incomplete
				 * async transaction in the TT buffer.
				 * We have to clear it.
				 */
				ehci_clear_tt_buffer(ehci, qh, urb, token);
			}

			/* force halt for unlinked or blocked qh, so we'll
			 * patch the qh later and so that completions can't
			 * activate it while we "know" it's stopped.
			 */
			if ((halt & qh->hw_token) == 0) {
halt:
				qh->hw_token |= halt;
				wmb ();
			}
		}

		/* unless we already know the urb's status, collect qtd status
		 * and update count of bytes transferred.  in common short read
		 * cases with only one data qtd (including control transfers),
		 * queue processing won't halt.  but with two or more qtds (for
		 * example, with a 32 KB transfer), when the first qtd gets a
		 * short read the second must be removed by hand.
		 */
		if (last_status == -EINPROGRESS) {
			last_status = qtd_copy_status(ehci, urb,
					qtd->length, token);
			if (last_status == -EREMOTEIO
					&& (qtd->hw_alt_next
						& EHCI_LIST_END(ehci)))
				last_status = -EINPROGRESS;

			/* As part of low/full-speed endpoint-halt processing
			 * we must clear the TT buffer (11.17.5).
			 */
			if (unlikely(last_status != -EINPROGRESS &&
					last_status != -EREMOTEIO))
				ehci_clear_tt_buffer(ehci, qh, urb, token);
		}

		/* if we're removing something not at the queue head,
		 * patch the hardware queue pointer.
		 */
		if (stopped && qtd->qtd_list.prev != &qh->qtd_list) {
			last = list_entry (qtd->qtd_list.prev,
					struct ehci_qtd, qtd_list);
			last->hw_next = qtd->hw_next;
		}

		/* remove qtd; it's recycled after possible urb completion */
		list_del (&qtd->qtd_list);
		last = qtd;

		/* reinit the xacterr counter for the next qtd */
		qh->xacterrs = 0;
	}

	/* last urb's completion might still need calling */
	if (likely (last != NULL)) {
		ehci_urb_done(ehci, last->urb, last_status);
		count++;
		ehci_qtd_free (ehci, last);
	}

	/* restore original state; caller must unlink or relink */
	qh->qh_state = state;

	/* be sure the hardware's done with the qh before refreshing
	 * it after fault cleanup, or recovering from silicon wrongly
	 * overlaying the dummy qtd (which reduces DMA chatter).
	 */
	if (stopped != 0 || qh->hw_qtd_next == EHCI_LIST_END(ehci)) {
		switch (state) {
		case QH_STATE_IDLE:
			qh_refresh(ehci, qh);
			break;
		case QH_STATE_LINKED:
			/* We won't refresh a QH that's linked (after the HC
			 * stopped the queue).  That avoids a race:
			 *  - HC reads first part of QH;
			 *  - CPU updates that first part and the token;
			 *  - HC reads rest of that QH, including token
			 * Result:  HC gets an inconsistent image, and then
			 * DMAs to/from the wrong memory (corrupting it).
			 *
			 * That should be rare for interrupt transfers,
			 * except maybe high bandwidth ...
			 */
			if ((cpu_to_hc32(ehci, QH_SMASK)
					& qh->hw_info2) != 0) {
				intr_deschedule (ehci, qh);
				(void) qh_schedule (ehci, qh);
			} else
				unlink_async (ehci, qh);
			break;
		/* otherwise, unlink already started */
		}
	}

	return count;
}

/*-------------------------------------------------------------------------*/

// high bandwidth multiplier, as encoded in highspeed endpoint descriptors
#define hb_mult(wMaxPacketSize) (1 + (((wMaxPacketSize) >> 11) & 0x03))
// ... and packet size, for any kind of endpoint descriptor
#define max_packet(wMaxPacketSize) ((wMaxPacketSize) & 0x07ff)

/*
 * reverse of qh_urb_transaction:  free a list of TDs.
 * used for cleanup after errors, before HC sees an URB's TDs.
 */
static void qtd_list_free (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*qtd_list
) {
	struct list_head	*entry, *temp;

	list_for_each_safe (entry, temp, qtd_list) {
		struct ehci_qtd	*qtd;

		qtd = list_entry (entry, struct ehci_qtd, qtd_list);
#ifdef CONFIG_ATH_USB_DMA_TO_SRAM
		/*
		 * Try to catch all error cases where qtd gets freed
		 * without getting completed via IOC interrupt
		 */
		if (ath_sram_bulk_in(qtd->urb)) {
			ath_purge_aqu(ath_qtd_to_aqu(qtd));
		}
#endif /* CONFIG_ATH_USB_DMA_TO_SRAM */
		list_del (&qtd->qtd_list);
		ehci_qtd_free (ehci, qtd);
	}
}

/*
 * create a list of filled qtds for this URB; won't link into qh.
 */
static struct list_head *
qh_urb_transaction (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*head,
	gfp_t			flags
) {
	struct ehci_qtd		*qtd, *qtd_prev;
	dma_addr_t		buf;
	int			len, maxpacket;
	int			is_input;
	u32			token;

	/*
	 * URBs map to sequences of QTDs:  one logical transaction
	 */
	qtd = ehci_qtd_alloc (ehci, flags);
	if (unlikely (!qtd))
		return NULL;
	list_add_tail (&qtd->qtd_list, head);
	qtd->urb = urb;

	token = QTD_STS_ACTIVE;
	token |= (EHCI_TUNE_CERR << 10);
	/* for split transactions, SplitXState initialized to zero */

	len = urb->transfer_buffer_length;
	is_input = usb_pipein (urb->pipe);
	if (usb_pipecontrol (urb->pipe)) {
		/* SETUP pid */
		qtd_fill(ehci, qtd, urb->setup_dma,
				sizeof (struct usb_ctrlrequest),
				token | (2 /* "setup" */ << 8), 8);

		/* ... and always at least one more pid */
		token ^= QTD_TOGGLE;
		qtd_prev = qtd;
		qtd = ehci_qtd_alloc (ehci, flags);
		if (unlikely (!qtd))
			goto cleanup;
		qtd->urb = urb;
		qtd_prev->hw_next = QTD_NEXT(ehci, qtd->qtd_dma);
		list_add_tail (&qtd->qtd_list, head);

		/* for zero length DATA stages, STATUS is always IN */
		if (len == 0)
			token |= (1 /* "in" */ << 8);
	}

	/*
	 * data transfer stage:  buffer setup
	 */
	buf = urb->transfer_dma;

	if (is_input)
		token |= (1 /* "in" */ << 8);
	/* else it's already initted to "out" pid (0 << 8) */

	maxpacket = max_packet(usb_maxpacket(urb->dev, urb->pipe, !is_input));

	/*
	 * buffer gets wrapped in one or more qtds;
	 * last one may be "short" (including zero len)
	 * and may serve as a control status ack
	 */
	for (;;) {
		int this_qtd_len;

		this_qtd_len = qtd_fill(ehci, qtd, buf, len, token, maxpacket);
		len -= this_qtd_len;
		buf += this_qtd_len;

		/*
		 * short reads advance to a "magic" dummy instead of the next
		 * qtd ... that forces the queue to stop, for manual cleanup.
		 * (this will usually be overridden later.)
		 */
		if (is_input)
			qtd->hw_alt_next = ehci->async->hw_alt_next;

		/* qh makes control packets use qtd toggle; maybe switch it */
		if ((maxpacket & (this_qtd_len + (maxpacket - 1))) == 0)
			token ^= QTD_TOGGLE;

		if (likely (len <= 0))
			break;

		qtd_prev = qtd;
		qtd = ehci_qtd_alloc (ehci, flags);
		if (unlikely (!qtd))
			goto cleanup;
		qtd->urb = urb;
		qtd_prev->hw_next = QTD_NEXT(ehci, qtd->qtd_dma);
		list_add_tail (&qtd->qtd_list, head);
	}

	/*
	 * unless the caller requires manual cleanup after short reads,
	 * have the alt_next mechanism keep the queue running after the
	 * last data qtd (the only one, for control and most other cases).
	 */
	if (likely ((urb->transfer_flags & URB_SHORT_NOT_OK) == 0
				|| usb_pipecontrol (urb->pipe)))
		qtd->hw_alt_next = EHCI_LIST_END(ehci);

	/*
	 * control requests may need a terminating data "status" ack;
	 * bulk ones may need a terminating short packet (zero length).
	 */
	if (likely (urb->transfer_buffer_length != 0)) {
		int	one_more = 0;

		if (usb_pipecontrol (urb->pipe)) {
			one_more = 1;
			token ^= 0x0100;	/* "in" <--> "out"  */
			token |= QTD_TOGGLE;	/* force DATA1 */
		} else if (usb_pipebulk (urb->pipe)
				&& (urb->transfer_flags & URB_ZERO_PACKET)
				&& !(urb->transfer_buffer_length % maxpacket)) {
			one_more = 1;
		}
		if (one_more) {
			qtd_prev = qtd;
			qtd = ehci_qtd_alloc (ehci, flags);
			if (unlikely (!qtd))
				goto cleanup;
			qtd->urb = urb;
			qtd_prev->hw_next = QTD_NEXT(ehci, qtd->qtd_dma);
			list_add_tail (&qtd->qtd_list, head);

			/* never any data in such packets */
			qtd_fill(ehci, qtd, 0, 0, token, 0);
		}
	}

	/* by default, enable interrupt on urb completion */
	if (likely (!(urb->transfer_flags & URB_NO_INTERRUPT)))
		qtd->hw_token |= cpu_to_hc32(ehci, QTD_IOC);
	return head;

cleanup:
	qtd_list_free (ehci, urb, head);
	return NULL;
}

/*-------------------------------------------------------------------------*/

// Would be best to create all qh's from config descriptors,
// when each interface/altsetting is established.  Unlink
// any previous qh and cancel its urbs first; endpoints are
// implicitly reset then (data toggle too).
// That'd mean updating how usbcore talks to HCDs. (2.7?)


/*
 * Each QH holds a qtd list; a QH is used for everything except iso.
 *
 * For interrupt urbs, the scheduler must set the microframe scheduling
 * mask(s) each time the QH gets scheduled.  For highspeed, that's
 * just one microframe in the s-mask.  For split interrupt transactions
 * there are additional complications: c-mask, maybe FSTNs.
 */
static struct ehci_qh *
qh_make (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	gfp_t			flags
) {
	struct ehci_qh		*qh = ehci_qh_alloc (ehci, flags);
	u32			info1 = 0, info2 = 0;
	int			is_input, type;
	int			maxp = 0;
	struct usb_tt		*tt = urb->dev->tt;

	if (!qh)
		return qh;

	/*
	 * init endpoint/device data for this QH
	 */
	info1 |= usb_pipeendpoint (urb->pipe) << 8;
	info1 |= usb_pipedevice (urb->pipe) << 0;

	is_input = usb_pipein (urb->pipe);
	type = usb_pipetype (urb->pipe);
	maxp = usb_maxpacket (urb->dev, urb->pipe, !is_input);

	/* 1024 byte maxpacket is a hardware ceiling.  High bandwidth
	 * acts like up to 3KB, but is built from smaller packets.
	 */
	if (max_packet(maxp) > 1024) {
		ehci_dbg(ehci, "bogus qh maxpacket %d\n", max_packet(maxp));
		goto done;
	}

	/* Compute interrupt scheduling parameters just once, and save.
	 * - allowing for high bandwidth, how many nsec/uframe are used?
	 * - split transactions need a second CSPLIT uframe; same question
	 * - splits also need a schedule gap (for full/low speed I/O)
	 * - qh has a polling interval
	 *
	 * For control/bulk requests, the HC or TT handles these.
	 */
	if (type == PIPE_INTERRUPT) {
		qh->usecs = NS_TO_US(usb_calc_bus_time(USB_SPEED_HIGH,
				is_input, 0,
				hb_mult(maxp) * max_packet(maxp)));
		qh->start = NO_FRAME;

		if (urb->dev->speed == USB_SPEED_HIGH) {
			qh->c_usecs = 0;
			qh->gap_uf = 0;

			qh->period = urb->interval >> 3;
			if (qh->period == 0 && urb->interval != 1) {
				/* NOTE interval 2 or 4 uframes could work.
				 * But interval 1 scheduling is simpler, and
				 * includes high bandwidth.
				 */
				dbg ("intr period %d uframes, NYET!",
						urb->interval);
				goto done;
			}
		} else {
			int		think_time;

			/* gap is f(FS/LS transfer times) */
			qh->gap_uf = 1 + usb_calc_bus_time (urb->dev->speed,
					is_input, 0, maxp) / (125 * 1000);

			/* FIXME this just approximates SPLIT/CSPLIT times */
			if (is_input) {		// SPLIT, gap, CSPLIT+DATA
				qh->c_usecs = qh->usecs + HS_USECS (0);
				qh->usecs = HS_USECS (1);
			} else {		// SPLIT+DATA, gap, CSPLIT
				qh->usecs += HS_USECS (1);
				qh->c_usecs = HS_USECS (0);
			}

			think_time = tt ? tt->think_time : 0;
			qh->tt_usecs = NS_TO_US (think_time +
					usb_calc_bus_time (urb->dev->speed,
					is_input, 0, max_packet (maxp)));
			qh->period = urb->interval;
		}
	}

	/* support for tt scheduling, and access to toggles */
	qh->dev = urb->dev;

	/* using TT? */
	switch (urb->dev->speed) {
	case USB_SPEED_LOW:
		info1 |= (1 << 12);	/* EPS "low" */
		/* FALL THROUGH */

	case USB_SPEED_FULL:
		/* EPS 0 means "full" */
		if (type != PIPE_INTERRUPT)
			info1 |= (EHCI_TUNE_RL_TT << 28);
		if (type == PIPE_CONTROL) {
			info1 |= (1 << 27);	/* for TT */
			info1 |= 1 << 14;	/* toggle from qtd */
		}
		info1 |= maxp << 16;

		info2 |= (EHCI_TUNE_MULT_TT << 30);

		/* Some Freescale processors have an erratum in which the
		 * port number in the queue head was 0..N-1 instead of 1..N.
		 */
		if (ehci_has_fsl_portno_bug(ehci))
			info2 |= (urb->dev->ttport-1) << 23;
		else
			info2 |= urb->dev->ttport << 23;

		/* set the address of the TT; for TDI's integrated
		 * root hub tt, leave it zeroed.
		 */
		if (tt && tt->hub != ehci_to_hcd(ehci)->self.root_hub)
			info2 |= tt->hub->devnum << 16;

		/* NOTE:  if (PIPE_INTERRUPT) { scheduler sets c-mask } */

		break;

	case USB_SPEED_HIGH:		/* no TT involved */
		info1 |= (2 << 12);	/* EPS "high" */
		if (type == PIPE_CONTROL) {
			info1 |= (EHCI_TUNE_RL_HS << 28);
			info1 |= 64 << 16;	/* usb2 fixed maxpacket */
			info1 |= 1 << 14;	/* toggle from qtd */
			info2 |= (EHCI_TUNE_MULT_HS << 30);
		} else if (type == PIPE_BULK) {
			info1 |= (EHCI_TUNE_RL_HS << 28);
			/* The USB spec says that high speed bulk endpoints
			 * always use 512 byte maxpacket.  But some device
			 * vendors decided to ignore that, and MSFT is happy
			 * to help them do so.  So now people expect to use
			 * such nonconformant devices with Linux too; sigh.
			 */
			info1 |= max_packet(maxp) << 16;
			info2 |= (EHCI_TUNE_MULT_HS << 30);
		} else {		/* PIPE_INTERRUPT */
			info1 |= max_packet (maxp) << 16;
			info2 |= hb_mult (maxp) << 30;
		}
		break;
	default:
		dbg ("bogus dev %p speed %d", urb->dev, urb->dev->speed);
done:
		qh_put (qh);
		return NULL;
	}

	/* NOTE:  if (PIPE_INTERRUPT) { scheduler sets s-mask } */

	/* init as live, toggle clear, advance to dummy */
	qh->qh_state = QH_STATE_IDLE;
	qh->hw_info1 = cpu_to_hc32(ehci, info1);
	qh->hw_info2 = cpu_to_hc32(ehci, info2);
	usb_settoggle (urb->dev, usb_pipeendpoint (urb->pipe), !is_input, 1);
	qh_refresh (ehci, qh);
	return qh;
}

/*-------------------------------------------------------------------------*/

/* move qh (and its qtds) onto async queue; maybe enable queue.  */

static void qh_link_async (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	__hc32		dma = QH_NEXT(ehci, qh->qh_dma);
	struct ehci_qh	*head;

	/* Don't link a QH if there's a Clear-TT-Buffer pending */
	if (unlikely(qh->clearing_tt))
		return;

	/* (re)start the async schedule? */
	head = ehci->async;
	timer_action_done (ehci, TIMER_ASYNC_OFF);
	if (!head->qh_next.qh) {
		u32	cmd = ehci_readl(ehci, &ehci->regs->command);

		if (!(cmd & CMD_ASE)) {
			/* in case a clear of CMD_ASE didn't take yet */
			(void)handshake(ehci, &ehci->regs->status,
					STS_ASS, 0, 150);
			cmd |= CMD_ASE | CMD_RUN;
			ehci_writel(ehci, cmd, &ehci->regs->command);
			ehci_to_hcd(ehci)->state = HC_STATE_RUNNING;
			/* posted write need not be known to HC yet ... */
		}
	}

	/* clear halt and/or toggle; and maybe recover from silicon quirk */
	if (qh->qh_state == QH_STATE_IDLE)
		qh_refresh (ehci, qh);

	/* splice right after start */
	qh->qh_next = head->qh_next;
	qh->hw_next = head->hw_next;
	wmb ();

	head->qh_next.qh = qh;
	head->hw_next = dma;

	qh_get(qh);
	qh->xacterrs = 0;
	qh->qh_state = QH_STATE_LINKED;
	/* qtd completions reported later by interrupt */
}

/*-------------------------------------------------------------------------*/

/*
 * For control/bulk/interrupt, return QH with these TDs appended.
 * Allocates and initializes the QH if necessary.
 * Returns null if it can't allocate a QH it needs to.
 * If the QH has TDs (urbs) already, that's great.
 */
static struct ehci_qh *qh_append_tds (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*qtd_list,
	int			epnum,
	void			**ptr
)
{
	struct ehci_qh		*qh = NULL;
	__hc32			qh_addr_mask = cpu_to_hc32(ehci, 0x7f);

	qh = (struct ehci_qh *) *ptr;
	if (unlikely (qh == NULL)) {
		/* can't sleep here, we have ehci->lock... */
		qh = qh_make (ehci, urb, GFP_ATOMIC);
		*ptr = qh;
	}
	if (likely (qh != NULL)) {
		struct ehci_qtd	*qtd;

		if (unlikely (list_empty (qtd_list)))
			qtd = NULL;
		else
			qtd = list_entry (qtd_list->next, struct ehci_qtd,
					qtd_list);

		/* control qh may need patching ... */
		if (unlikely (epnum == 0)) {

                        /* usb_reset_device() briefly reverts to address 0 */
                        if (usb_pipedevice (urb->pipe) == 0)
                                qh->hw_info1 &= ~qh_addr_mask;
		}

		/* just one way to queue requests: swap with the dummy qtd.
		 * only hc or qh_refresh() ever modify the overlay.
		 */
		if (likely (qtd != NULL)) {
			struct ehci_qtd		*dummy;
			dma_addr_t		dma;
			__hc32			token;

			/* to avoid racing the HC, use the dummy td instead of
			 * the first td of our list (becomes new dummy).  both
			 * tds stay deactivated until we're done, when the
			 * HC is allowed to fetch the old dummy (4.10.2).
			 */
			token = qtd->hw_token;
			qtd->hw_token = HALT_BIT(ehci);
			wmb ();
			dummy = qh->dummy;

			dma = dummy->qtd_dma;
			*dummy = *qtd;
			dummy->qtd_dma = dma;

			list_del (&qtd->qtd_list);
			list_add (&dummy->qtd_list, qtd_list);
			list_splice_tail(qtd_list, &qh->qtd_list);

			ehci_qtd_init(ehci, qtd, qtd->qtd_dma);
			qh->dummy = qtd;

			/* hc must see the new dummy at list end */
			dma = qtd->qtd_dma;
			qtd = list_entry (qh->qtd_list.prev,
					struct ehci_qtd, qtd_list);
			qtd->hw_next = QTD_NEXT(ehci, dma);

			/* let the hc process these next qtds */
			wmb ();
			dummy->hw_token = token;

			urb->hcpriv = qh_get (qh);
		}
	}
	return qh;
}

/*-------------------------------------------------------------------------*/

static int
submit_async (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*qtd_list,
	gfp_t			mem_flags
) {
	struct ehci_qtd		*qtd;
	int			epnum;
	unsigned long		flags;
	struct ehci_qh		*qh = NULL;
	int			rc;

	qtd = list_entry (qtd_list->next, struct ehci_qtd, qtd_list);
	epnum = urb->ep->desc.bEndpointAddress;

#ifdef EHCI_URB_TRACE
	ehci_dbg (ehci,
		"%s %s urb %p ep%d%s len %d, qtd %p [qh %p]\n",
		__func__, urb->dev->devpath, urb,
		epnum & 0x0f, (epnum & USB_DIR_IN) ? "in" : "out",
		urb->transfer_buffer_length,
		qtd, urb->ep->hcpriv);
#endif

	spin_lock_irqsave (&ehci->lock, flags);
	if (unlikely(!test_bit(HCD_FLAG_HW_ACCESSIBLE,
			       &ehci_to_hcd(ehci)->flags))) {
		rc = -ESHUTDOWN;
		goto done;
	}
	rc = usb_hcd_link_urb_to_ep(ehci_to_hcd(ehci), urb);
	if (unlikely(rc))
		goto done;

	qh = qh_append_tds(ehci, urb, qtd_list, epnum, &urb->ep->hcpriv);
	if (unlikely(qh == NULL)) {
		usb_hcd_unlink_urb_from_ep(ehci_to_hcd(ehci), urb);
		rc = -ENOMEM;
		goto done;
	}

	/* Control/bulk operations through TTs don't need scheduling,
	 * the HC and TT handle it when the TT has a buffer ready.
	 */
	if (likely (qh->qh_state == QH_STATE_IDLE))
		qh_link_async(ehci, qh);
 done:
	spin_unlock_irqrestore (&ehci->lock, flags);
	if (unlikely (qh == NULL))
		qtd_list_free (ehci, urb, qtd_list);
	return rc;
}

/*-------------------------------------------------------------------------*/

/* the async qh for the qtds being reclaimed are now unlinked from the HC */

static void end_unlink_async (struct ehci_hcd *ehci)
{
	struct ehci_qh		*qh = ehci->reclaim;
	struct ehci_qh		*next;

	iaa_watchdog_done(ehci);

	// qh->hw_next = cpu_to_hc32(qh->qh_dma);
	qh->qh_state = QH_STATE_IDLE;
	qh->qh_next.qh = NULL;
	qh_put (qh);			// refcount from reclaim

	/* other unlink(s) may be pending (in QH_STATE_UNLINK_WAIT) */
	next = qh->reclaim;
	ehci->reclaim = next;
	qh->reclaim = NULL;

	qh_completions (ehci, qh);

	if (!list_empty (&qh->qtd_list)
			&& HC_IS_RUNNING (ehci_to_hcd(ehci)->state))
		qh_link_async (ehci, qh);
	else {
		/* it's not free to turn the async schedule on/off; leave it
		 * active but idle for a while once it empties.
		 */
		if (HC_IS_RUNNING (ehci_to_hcd(ehci)->state)
				&& ehci->async->qh_next.qh == NULL)
			timer_action (ehci, TIMER_ASYNC_OFF);
	}
	qh_put(qh);			/* refcount from async list */

	if (next) {
		ehci->reclaim = NULL;
		start_unlink_async (ehci, next);
	}
}

/* makes sure the async qh will become idle */
/* caller must own ehci->lock */

static void start_unlink_async (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	int		cmd = ehci_readl(ehci, &ehci->regs->command);
	struct ehci_qh	*prev;

#ifdef DEBUG
	assert_spin_locked(&ehci->lock);
	if (ehci->reclaim
			|| (qh->qh_state != QH_STATE_LINKED
				&& qh->qh_state != QH_STATE_UNLINK_WAIT)
			)
		BUG ();
#endif

	/* stop async schedule right now? */
	if (unlikely (qh == ehci->async)) {
		/* can't get here without STS_ASS set */
		if (ehci_to_hcd(ehci)->state != HC_STATE_HALT
				&& !ehci->reclaim) {
			/* ... and CMD_IAAD clear */
			ehci_writel(ehci, cmd & ~CMD_ASE,
				    &ehci->regs->command);
			wmb ();
			// handshake later, if we need to
			timer_action_done (ehci, TIMER_ASYNC_OFF);
		}
		return;
	}

	qh->qh_state = QH_STATE_UNLINK;
	ehci->reclaim = qh = qh_get (qh);

	prev = ehci->async;
	while (prev->qh_next.qh != qh)
		prev = prev->qh_next.qh;

	prev->hw_next = qh->hw_next;
	prev->qh_next = qh->qh_next;
	wmb ();

	/* If the controller isn't running, we don't have to wait for it */
	if (unlikely(!HC_IS_RUNNING(ehci_to_hcd(ehci)->state))) {
		/* if (unlikely (qh->reclaim != 0))
		 *	this will recurse, probably not much
		 */
		end_unlink_async (ehci);
		return;
	}

	cmd |= CMD_IAAD;
	ehci_writel(ehci, cmd, &ehci->regs->command);
	(void)ehci_readl(ehci, &ehci->regs->command);
	iaa_watchdog_start(ehci);
}

/*-------------------------------------------------------------------------*/

static void scan_async (struct ehci_hcd *ehci)
{
	struct ehci_qh		*qh;
	enum ehci_timer_action	action = TIMER_IO_WATCHDOG;

	ehci->stamp = ehci_readl(ehci, &ehci->regs->frame_index);
	timer_action_done (ehci, TIMER_ASYNC_SHRINK);
rescan:
	qh = ehci->async->qh_next.qh;
	if (likely (qh != NULL)) {
		do {
#ifdef CONFIG_ATH_USB_DMA_TO_SRAM
			if (ath_per_qtd_ioc(qh, ehci)) {
				goto next_qh;
			}
#endif /* CONFIG_ATH_USB_DMA_TO_SRAM */
			/* clean any finished work for this qh */
			if (!list_empty (&qh->qtd_list)
					&& qh->stamp != ehci->stamp) {
				int temp;

				/* unlinks could happen here; completion
				 * reporting drops the lock.  rescan using
				 * the latest schedule, but don't rescan
				 * qhs we already finished (no looping).
				 */
				qh = qh_get (qh);
				qh->stamp = ehci->stamp;
				temp = qh_completions (ehci, qh);
				qh_put (qh);
				if (temp != 0) {
					goto rescan;
				}
			}

			/* unlink idle entries, reducing DMA usage as well
			 * as HCD schedule-scanning costs.  delay for any qh
			 * we just scanned, there's a not-unusual case that it
			 * doesn't stay idle for long.
			 * (plus, avoids some kind of re-activation race.)
			 */
			if (list_empty(&qh->qtd_list)
					&& qh->qh_state == QH_STATE_LINKED) {
				if (!ehci->reclaim
					&& ((ehci->stamp - qh->stamp) & 0x1fff)
						>= (EHCI_SHRINK_FRAMES * 8))
					start_unlink_async(ehci, qh);
				else
					action = TIMER_ASYNC_SHRINK;
			}
#ifdef CONFIG_ATH_USB_DMA_TO_SRAM
next_qh:
#endif
			qh = qh->qh_next.qh;
		} while (qh);
	}
	if (action == TIMER_ASYNC_SHRINK)
		timer_action (ehci, TIMER_ASYNC_SHRINK);
}
