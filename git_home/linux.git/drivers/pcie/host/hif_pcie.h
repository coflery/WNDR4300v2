/*
 * Copyright (c) 2013 Qualcomm Atheros, Inc.
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
 */

#ifndef __HIF_PCI_H
#define __HIF_PCI_H

#include  "hif_pcie_dma.h"

#define HIF_PCI_MAX_DEVS        4
#define MAX_TXDESC_SHIFT        8
#define MAX_RXDESC_SHIFT        9
#define HIF_PCI_MAX_TX_DESC     (1 << MAX_TXDESC_SHIFT)
#define HIF_PCI_MAX_RX_DESC     (1 << MAX_RXDESC_SHIFT)
#define MAX_NBUF_SIZE           1600

// Low power mode registers 
#define HIF_PCIE_RC_ASPM_SUPPORT                        0x180c007c  
#define HIF_PCIE_RC_SUPP_L0                             (1 << 10)  
#define HIF_PCIE_RC_SUPP_L1                             (1 << 11)  

#define HIF_PCIE_RC_ASPM_ENABLE                         0x180c0080  
#define HIF_PCIE_RC_EN_L0                               (1 << 0)  
#define HIF_PCIE_RC_EN_L1                               (1 << 1)  

#define HIF_PCIE_EP_ASPM_ENABLE                         0x14000080  
#define HIF_PCIE_EP_EN_L0                               (1 << 0)  
#define HIF_PCIE_EP_EN_L1                               (1 << 1)  

// Spare registers for L1 aspm
#define HIF_PCIE_HOST_INTF_PM_CTRL			0x10004014
#define HIF_PCIE_HOST_INTF_PM_CTRL_EN			(1 << 19)
#define HIF_PCIE_HOST_INTF_SPARE			0x10004070
#define HIF_PCIE_HOST_INTF_SPARE_EN			(1 << 0)


#define HIF_PCIE_RC_STATE				0x180f001c
#define HIF_PCIE_EP_STATE				0x180600bc

#endif
