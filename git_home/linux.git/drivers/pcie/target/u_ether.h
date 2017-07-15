/*
 * u_ether.h -- interface to USB gadget "ethernet link" utilities
 *
 * Copyright (C) 2003-2005,2008 David Brownell
 * Copyright (C) 2003-2004 Robert Schwebel, Benedikt Spranger
 * Copyright (C) 2008 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __U_ETHER_H
#define __U_ETHER_H

#include <linux/err.h>
#include <linux/if_ether.h>
#include <linux/usb/composite.h>
#include <linux/usb/cdc.h>

#define	DEFAULT_FILTER	(USB_CDC_PACKET_TYPE_BROADCAST \
			|USB_CDC_PACKET_TYPE_ALL_MULTICAST \
			|USB_CDC_PACKET_TYPE_PROMISCUOUS \
			|USB_CDC_PACKET_TYPE_DIRECTED)
struct gether {        /* updated by gether_{connect,disconnect} */
        struct eth_dev                  *ioport;

        /* endpoints handle full and/or high speeds */
        struct pci_ep                   *in_ep;
        struct pci_ep                   *out_ep;

        /* descriptors match device speed at gether_connect() time */
        bool                            is_zlp_ok;

        u16                             cdc_filter;

        /* hooks for added framing, as needed for RNDIS and EEM.
         * we currently don't support multiple frames per SKB.
         */
        u32                             header_len;
        struct sk_buff                  *(*wrap)(struct sk_buff *skb);
        int                             (*unwrap)(struct sk_buff *skb);

        /* called on network open/close */
        void                            (*open)(struct gether *);
        void                            (*close)(struct gether *);
};



/* netdev setup/teardown as directed by the gadget driver */
int gether_setup(u8 ethaddr[ETH_ALEN]);
void gether_cleanup(void);

/* connect/disconnect is handled by individual functions */
struct net_device *gether_connect(struct ath_pci_dev *);
void gether_disconnect(struct gether *);

/* Some controllers can't support CDC Ethernet (ECM) ... */
static inline bool can_support_ecm(struct usb_gadget *gadget)
{
	return true;
}

/* each configuration may bind one instance of an ethernet link */
int geth_bind_config(struct usb_configuration *c, u8 ethaddr[ETH_ALEN]);
int ecm_bind_config(struct usb_configuration *c, u8 ethaddr[ETH_ALEN]);

#ifdef CONFIG_USB_ETH_RNDIS

int rndis_bind_config(struct usb_configuration *c, u8 ethaddr[ETH_ALEN]);

#else

static inline int
rndis_bind_config(struct usb_configuration *c, u8 ethaddr[ETH_ALEN])
{
	return 0;
}

#endif

#endif /* __U_ETHER_H */
