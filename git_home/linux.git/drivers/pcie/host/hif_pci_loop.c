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

#include <adf_os_stdtypes.h>
#include <adf_os_types.h>
#include <adf_os_util.h>
#include <adf_os_module.h>
#include <adf_os_lock.h>
#include <adf_nbuf.h>
#include <adf_net.h>
#include <adf_net_types.h>

#include <athdefs.h>
#include <a_types.h>
#include <hif.h>
#include <linux/proc_fs.h>


typedef struct __loop_sc{
    adf_net_handle_t        net_handle;
    a_uint8_t               tx_pipe;
    a_uint8_t               rx_pipe;
    a_uint32_t              next_idx;/*next softc index*/
    adf_os_spinlock_t       lock_irq;
    adf_net_dev_info_t      dev_info;
}__loop_sc_t;

#define TEST_SEND_PKT       0
#define TEST_PIPE_NUM       0
#define TEST_DEFAULT_PIPE   0
#define TEST_MAX_DEVS       1

#define HIF_PCI_PIPE_RX0    0
#define HIF_PCI_PIPE_RX1    1
#define HIF_PCI_PIPE_TX0    0
#define HIF_PCI_PIPE_TX1    1

__loop_sc_t       loop_sc[TEST_MAX_DEVS] =   {
    { /* Pipe 0*/
//      .next_idx = 1, 
      .next_idx = 0, 
      .dev_info = { .if_name = "ld" , .dev_addr = "\0SNUL0"},
      .tx_pipe = HIF_PCI_PIPE_TX0,
      .rx_pipe = HIF_PCI_PIPE_RX0,
    }
// Pipe 1
/*    { 
      .next_idx = 0,
      .dev_info = { .if_name = "ld" , .dev_addr = "\0SNUL1"},
      .tx_pipe = HIF_PCI_PIPE_TX0,
      .rx_pipe = HIF_PCI_PIPE_RX0,
    }
*/
};

extern a_status_t hif_boot_start(HIF_HANDLE  hdl);
a_uint8_t         loop_pipe = TEST_PIPE_NUM;
HIF_HANDLE        hif_handle = NULL;

unsigned long total_rx_pkt;
unsigned long total_tx_pkt;
unsigned long total_valid_pkt;

static struct proc_dir_entry *pci_proc;

/* Test packet formation */


/**
 * @brief Function to display the counter values via cat /proc/pci_loop
 *
 */

static int ath_pci_proc_display(char *buf, char **start, off_t offset,
                                        int count, int *eof, void *data)
{
        return sprintf(buf,
                        "Total rx packet  = %li\n"
                        "Total valid rx pkt  = %li\n"
                        "Total tx packet  = %li\n",
                        total_rx_pkt, total_valid_pkt, total_tx_pkt); 
}


/******************************* DUMMY CALLS **********************/    
a_status_t
loop_cmd(adf_drv_handle_t  hdl, adf_net_cmd_t  cmd, 
         adf_net_cmd_data_t *data)
{

    return A_STATUS_OK;
}
a_status_t
loop_tx_timeout(adf_drv_handle_t  hdl)
{

    return A_STATUS_OK;
}

/**
 * @brief Test Receive Function
 * 
 * @param context
 * @param buf
 * @param pipe
 * 
 * @return A_STATUS
 */
hif_status_t
loop_recv(void *context, adf_nbuf_t buf, a_uint8_t pipe)
{
    a_uint32_t  len = 0;
    __loop_sc_t   *sc = NULL;

    /**
     * Assert if the pipe != 0 or 1
     */
    total_rx_pkt++;
    adf_os_assert(pipe == 0 || pipe == 1);

    len = adf_nbuf_len(buf);

    if (len == 0) {
        printk("Invalid packet of len 0\n");
        return HIF_ERROR;
    }

    sc = &loop_sc[0];
    adf_net_indicate_packet(sc->net_handle, buf, len);
    
    return HIF_OK;
}   

/**
 * @brief Function to transmit the packet
 * 
 * @param adf handle
 * @param buf
 * 
 * @return A_STATUS
 */
a_status_t
loop_tx(adf_drv_handle_t  hdl, adf_nbuf_t  buf)
{
    __loop_sc_t   *sc = (__loop_sc_t  *)hdl;
    unsigned long  flags = 0;
    hif_status_t status;

    total_tx_pkt++;
    adf_os_spin_lock_irq(&sc->lock_irq, flags);

    status = HIFSend(hif_handle, sc->tx_pipe, NULL, buf);
    if (status != HIF_OK) {
        adf_net_stop_queue(sc->net_handle);
        adf_os_spin_unlock_irq(&sc->lock_irq, flags);
        return A_STATUS_ENOMEM;
    }
        
    adf_os_spin_unlock_irq(&sc->lock_irq, flags);
    
    return A_STATUS_OK;
}

/**
 * @brief Function called via transmit done interrupt
 * 
 * @param context information
 * @param buf
 * 
 * @return A_STATUS
 */
hif_status_t
loop_xmit_done(void *context, adf_nbuf_t buf)
{
    __loop_sc_t   *sc;

    sc = context;
    adf_nbuf_free(buf);
    if (adf_net_queue_stopped(sc->net_handle)) {
    	adf_net_wake_queue(sc->net_handle);
    }
    return HIF_OK;
}
/******************************* Open & Close *************************/
a_status_t
loop_open(adf_drv_handle_t  hdl)
{
    __loop_sc_t   *sc = (__loop_sc_t  *)hdl;

    adf_net_start_queue(sc->net_handle);
    
//    HIFStart(hif_handle);

    return A_STATUS_OK;
}
void
loop_close(adf_drv_handle_t  hdl)
{  
    __loop_sc_t   *sc = (__loop_sc_t  *)hdl;
    
    adf_net_stop_queue(sc->net_handle);
}
/*************************** Device Inserted & Removed handler ********/
A_STATUS
loop_create_dev(a_uint8_t   dev_num)
{
    __loop_sc_t     *sc = &loop_sc[dev_num];
    adf_dev_sw_t    sw = {0};


    sw.drv_open         = loop_open;
    sw.drv_close        = loop_close;
    sw.drv_cmd          = loop_cmd;
    sw.drv_tx           = loop_tx;
    sw.drv_tx_timeout   = loop_tx_timeout;


    sc->net_handle = adf_net_dev_create(sc, &sw, &sc->dev_info);

    adf_net_ifname(sc->net_handle);
    if ( !sc->net_handle ) {
        adf_os_print("Failed to create netdev\n");
        return A_NO_MEMORY;
    }
    
    adf_os_spinlock_init(&sc->lock_irq);
    return A_OK;
}

hif_status_t
loop_insert_dev(HIF_HANDLE hif, adf_os_handle_t os_hdl)
{
    HTC_CALLBACKS   pkt_cb = {0};
    A_STATUS        ret = A_OK;
    a_uint8_t       i = 0;

    for (i = 0; i < TEST_MAX_DEVS; i++) {
        if((ret = loop_create_dev(i)) != A_OK)
            return ret;
    }

    hif_handle = hif;

    pkt_cb.Context              = &loop_sc;
    pkt_cb.rxCompletionHandler  = loop_recv;
    pkt_cb.txCompletionHandler  = loop_xmit_done;

    
    HIFPostInit(hif_handle, NULL, &pkt_cb);
    hif_boot_start(hif);

//    HIFStart(hif_handle);
//    adf_os_print("HIF start now.... \n");
              
    return HIF_OK;    
}

hif_status_t
loop_remove_dev(void *ctx)
{
    a_uint8_t   i = 0;

    HIFShutDown(hif_handle);

    for( i = 0; i < TEST_MAX_DEVS; i++)
        adf_net_dev_delete(loop_sc[i].net_handle);
    
    return HIF_OK;
}
/************************** Module Init & Exit ********************/
int
loop_init(void)
{
    HTC_DRVREG_CALLBACKS cb;

    cb.deviceInsertedHandler = loop_insert_dev;
    cb.deviceRemovedHandler  = loop_remove_dev;
    create_proc_read_entry("pci_loop", 0, pci_proc,
                           ath_pci_proc_display, NULL);

    HIF_register(&cb);
        
    return 0;
}
void
loop_exit(void)
{
    loop_remove_dev(NULL);
}
adf_os_export_symbol(loop_open);
adf_os_export_symbol(loop_close);
adf_os_export_symbol(loop_tx);
adf_os_export_symbol(loop_cmd);
adf_os_export_symbol(loop_tx_timeout);

adf_os_virt_module_init(loop_init);
adf_os_virt_module_exit(loop_exit);
