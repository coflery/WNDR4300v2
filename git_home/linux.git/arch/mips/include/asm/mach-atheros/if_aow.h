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

/**
 * @brief Audio sampling rate and sample size supported 
 */

typedef enum _audio_type {
    SAMP_RATE_48k_SAMP_SIZE_16=0,    
    SAMP_RATE_48k_SAMP_SIZE_24,
    SAMP_MAX_RATE_NUM_ELEMENTS,    
} audio_type_t;

/* Keep this in sync with ATH_SUPPORT_AOW_TXSCHED 
   This sync requirement is temporary, since the AoW
   Tx scheduling feature will become permanent once it
   matures. */
#define WLAN_AOW_TXSCHED_ENABLED  1

/*
 * =====================================================================================
 *        Struct :  i2s_rx_dev
 *  Description  :  Holds I2S related function pointers
 * =====================================================================================
 */
typedef struct i2s_rx_dev {
    int (*open)(void);
    int (*close)(void);
    int (*dma_start)(int);
    int (*dma_resume)(int);
    int (*dma_pause)(void);
    int (*get_tx_sample_count)(void);
    int (*get_rx_sample_count)(void);
    int (*clk)(void);
    int (*write)(int len, const char* data);
    int (*read)(int len, const char* data);
    int (*fifo_reset)(void);
    int (*set_dsize)(u_int16_t size);
    int (*is_desc_busy)(struct file *filp);
    int (*get_used_desc_count)(void);

}i2s_rx_dev_t;


/*
 * =====================================================================================
 *       Struct:  aow_tx_dev
 *  Description:  Encapsulates the AoW Transmit node
 * =====================================================================================
 */
typedef struct aow_tx_dev {
    void (*set_audioparams)(audio_type_t audiotype);
    int (*send_data)(char *data, int len, int channel, unsigned long long tsf);
#ifdef WLAN_AOW_TXSCHED_ENABLED
    int (*dispatch_data)(void);
#endif
    unsigned int (*send_ctrl)(unsigned char *data, unsigned int len, unsigned long long tsf);
    void (*get_tsf)(unsigned long long* tsf);
}aow_tx_dev_t;    


/*
 * =====================================================================================
 *       Struct:  aow_rx_dev
 *  Description:  Encapsulates the AoW Receive node
 * =====================================================================================
 */
typedef struct aow_rx_dev {
    /* TODO: Add a function to set audio parameters, if required */
    int (*recv_data)(char *data, int len);
    int (*recv_ctrl)(char* data, int len);
    int (*set_frame_size)(unsigned int size);
    /* set_alt_setting is temporary */
    int (*set_alt_setting)(unsigned int altsetting);
    i2s_rx_dev_t i2s_ctrl;
}aow_rx_dev_t;    


/*
 * =====================================================================================
 *       Struct:  aow_dev
 *  Description:  Encapsulates the conceptual AoW device made of tx and rx
 * =====================================================================================
 */

typedef struct aow_dev {
    aow_tx_dev_t tx;
    aow_rx_dev_t rx;
}aow_dev_t;   

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  aow_register_wlan_calls_to_i2s
 *  Description:  Part of I2S API, Wlan will use this api to register it's function to
 *                I2S device
 *                WLAN device will register function to be used for informing it about
 *                audio parameters
 *                WLAN device will register transmit data function
 *                WLAN device will register dispatch data function
 *                WLAN device will register transmit ctrl function
 *                WLAN device will register get tsf function
 *
 * =====================================================================================
 */

int aow_register_wlan_calls_to_i2s(void* set_audioparams,
                                   void* tx_data,
#ifdef WLAN_AOW_TXSCHED_ENABLED
                                   void* dispatch_data,
#endif                            
                                   void* tx_ctrl,
                                   void* get_tsf);


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  aow_register_wlan_calls_to_usb
 *  Description:  Part of USB API, Wlan will use this api to register it's function to
 *                USB device
 *                WLAN device will register function to be used for informing it about
 *                audio parameters
 *                WLAN device will register transmit data function
 *                WLAN device will register dispatch data function
 *                WLAN device will register transmit ctrl function              
 *                WLAN device will register get tsf function
 *
 * =====================================================================================
 */
int aow_register_wlan_calls_to_usb(void* set_audioparams,
                                   void* tx_data,
#ifdef WLAN_AOW_TXSCHED_ENABLED
                                   void* dispatch_data,
#endif                            
                                   void* tx_ctrl,
                                   void* get_tsf);


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  aow_register_i2s_calls_to_wlan
 *  Description:  Part of I2S API, Wlan will use this api to register I2S functions to
 *                WLAN device
 *
 *                I2S device will register the following APIs 
 *                - open
 *                - close
 *                - start
 *                - resume
 *                - get_sample_count
 *                - pause
 *                - clk
 *                - write
 *                - read
 *                - fifo_reset
 * =====================================================================================
 */

int aow_register_i2s_calls_to_wlan(aow_dev_t* dev);


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  aow_register_usb_calls_to_wlan
 *  Description:  Part of USB API, wlan will use this api to register USB functions to
 *                WLAN device
 *              
 *                TBD
 *
 * =====================================================================================
 */
int aow_register_usb_calls_to_wlan(void* recv_data,
                                   void* recv_ctrl,
                                   void* set_frame_size,
                                   void* set_alt_setting);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  wlan_aow_register_calls_to_usb
 *  Description:  Part of WLAN API. Register WLAN calls to USB
 * =====================================================================================
 */

int wlan_aow_register_calls_to_usb(aow_dev_t* dev);
