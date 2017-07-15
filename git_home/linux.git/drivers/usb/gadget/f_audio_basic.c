/*
 * f_audio.c -- USB Audio class function driver
  *
 * Copyright (C) 2008 Bryan Wu <cooloney@kernel.org>
 * Copyright (C) 2008 Analog Devices, Inc
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <asm/atomic.h>
#include <linux/compiler.h>
#include <asm/unaligned.h>
#include <linux/list.h>
#include <linux/proc_fs.h>

#include "u_audio.h"

#define OUT_EP_MAX_PACKET_SIZE 192
#define MAX_AUDIO_CHAN (4)
#define BUF_SIZE_FACTOR 4
#define AUDIO_FRAME_SIZE ((OUT_EP_MAX_PACKET_SIZE)*8)
static int req_buf_size = OUT_EP_MAX_PACKET_SIZE;
module_param(req_buf_size, int, S_IRUGO);
MODULE_PARM_DESC(req_buf_size, "ISO OUT endpoint request buffer size");

static int req_count = 256;
module_param(req_count, int, S_IRUGO);
MODULE_PARM_DESC(req_count, "ISO OUT endpoint request count");

static int audio_buf_size = (AUDIO_FRAME_SIZE*BUF_SIZE_FACTOR);
module_param(audio_buf_size, int, S_IRUGO);
MODULE_PARM_DESC(audio_buf_size, "Audio buffer size");

#define I2S_ENABLED 1

#ifdef I2S_ENABLED
static int i2s_st;
static int i2s_write_cnt;
#if defined(CONFIG_MACH_AR934x) || \
    defined(CONFIG_MACH_QCA955x)
extern uint32_t ath_ref_freq;
extern void ath_ex_i2s_set_freq(uint32_t);
extern void ath_i2s_clk(unsigned long, unsigned long);
//extern void ath_i2s_dpll();
extern int  ath_ex_i2s_open(void);
extern void ath_ex_i2s_close(void);
extern void ath_ex_i2s_write(size_t , const char *, int );
extern void ath_i2s_dma_start(int);
#else
extern void ar7240_i2s_clk(unsigned long, unsigned long);
extern void ar7242_i2s_clk(unsigned long, unsigned long);
extern int  ar7242_i2s_open(void);
extern void ar7242_i2s_close(void);
extern void ar7242_i2s_write(size_t , const char *, int );
extern void ar7240_i2sound_dma_start(int);
#endif
#endif
/*
 * DESCRIPTORS ... most are static, but strings and full
 * configuration descriptors are built on demand.
 */

/*
 * We have two interfaces- AudioControl and AudioStreaming
 * TODO: only supcard playback currently
 */
#define F_AUDIO_AC_INTERFACE	0
#define F_AUDIO_AS_INTERFACE	1
#define F_AUDIO_NUM_INTERFACES	1

static off_t count_audio_playback;
/* B.3.1  Standard AC Interface Descriptor */
static struct usb_interface_descriptor ac_interface_desc __initdata = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bInterfaceNumber = 	0,
	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOCONTROL,
	.bInterfaceProtocol = 	0x0,
	.iInterface =		0x0,
};

DECLARE_USB_AC_HEADER_DESCRIPTOR(2);

#define USB_DT_AC_HEADER_LENGTH	USB_DT_AC_HEADER_SIZE(F_AUDIO_NUM_INTERFACES)
#define UAC_DT_TOTAL_LENGTH (USB_DT_AC_HEADER_LENGTH + \
	USB_DT_AC_INPUT_TERMINAL_SIZE\
       + USB_DT_AC_OUTPUT_TERMINAL_SIZE)
/* B.3.2  Class-Specific AC Interface Descriptor */
static struct usb_ac_header_descriptor_2 ac_header_desc = {
	.bLength =		USB_DT_AC_HEADER_LENGTH,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	HEADER,
	.bcdADC =		__constant_cpu_to_le16(0x0100),
	.wTotalLength =		__constant_cpu_to_le16(UAC_DT_TOTAL_LENGTH),
	.bInCollection =	F_AUDIO_NUM_INTERFACES,
	.baInterfaceNr = {
//		[0] =		F_AUDIO_AC_INTERFACE,
		[0] =		F_AUDIO_AS_INTERFACE,
	}
};

#define INPUT_TERMINAL_ID	1
#define OUTPUT_TERMINAL_ID	2
static struct usb_input_terminal_descriptor input_terminal_desc = {
	.bLength =		USB_DT_AC_INPUT_TERMINAL_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	INPUT_TERMINAL,
	.bTerminalID =		INPUT_TERMINAL_ID,
	.wTerminalType =	__constant_cpu_to_le16(USB_AC_TERMINAL_STREAMING),
	.bAssocTerminal =	OUTPUT_TERMINAL_ID,
 	.bNrChannels = 			0x8,
	.wChannelConfig =	__constant_cpu_to_le16(0x063F),
};
#ifdef FEATURE_UNIT_SUPPORTED

DECLARE_USB_AC_FEATURE_UNIT_DESCRIPTOR(0);

#define FEATURE_UNIT_ID		2
static struct usb_ac_feature_unit_descriptor_0 feature_unit_desc = {
	.bLength		= USB_DT_AC_FEATURE_UNIT_SIZE(0),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= FEATURE_UNIT,
	.bUnitID		= FEATURE_UNIT_ID,
	.bSourceID		= INPUT_TERMINAL_ID,
	.bControlSize		= 2,
	.bmaControls[0]		= (FU_MUTE | FU_VOLUME),
};

static struct usb_audio_control mute_control = {
	.list = LIST_HEAD_INIT(mute_control.list),
	.name = "Mute Control",
	.type = MUTE_CONTROL,
	/* Todo: add real Mute control code */
	.set = generic_set_cmd,
	.get = generic_get_cmd,
};

static struct usb_audio_control volume_control = {
	.list = LIST_HEAD_INIT(volume_control.list),
	.name = "Volume Control",
	.type = VOLUME_CONTROL,
	/* Todo: add real Volume control code */
	.set = generic_set_cmd,
	.get = generic_get_cmd,
};

static struct usb_audio_control_selector feature_unit = {
	.list = LIST_HEAD_INIT(feature_unit.list),
	.id = FEATURE_UNIT_ID,
	.name = "Mute & Volume Control",
	.type = FEATURE_UNIT,
	.desc = (struct usb_descriptor_header *)&feature_unit_desc,
};
#endif
static struct usb_output_terminal_descriptor output_terminal_desc = {
	.bLength		= USB_DT_AC_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= OUTPUT_TERMINAL,
	.bTerminalID		= OUTPUT_TERMINAL_ID,
	.wTerminalType		= USB_AC_OUTPUT_TERMINAL_SPEAKER,
    #ifdef FEATURE_UNIT_SUPPORTED
    .bAssocTerminal =       FEATURE_UNIT_ID,
    .bSourceID =            FEATURE_UNIT_ID,
    #else
    .bAssocTerminal =       INPUT_TERMINAL_ID,
    .bSourceID =            INPUT_TERMINAL_ID,
    #endif
};

/* B.4.1  Standard AS Interface Descriptor */
static struct usb_interface_descriptor as_interface_alt_0_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bInterfaceNumber = 	1,
	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = 	0x0,
	.iInterface =		    0,
};

static struct usb_interface_descriptor as_interface_alt_1_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bInterfaceNumber = 	1,
	.bAlternateSetting =	1,
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};
static const struct usb_interface_descriptor
as_interface_alt_2_desc = {
	.bLength =		        USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	    USB_DT_INTERFACE,
	.bInterfaceNumber = 	1,
	.bAlternateSetting = 	2,
	.bNumEndpoints =	    1,
	.bInterfaceClass =	    USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = 	0x0,
	.iInterface =		    0,
};


static const struct usb_interface_descriptor
as_interface_alt_3_desc = {
	.bLength =		        USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	    USB_DT_INTERFACE,
	.bInterfaceNumber = 	1,
	.bAlternateSetting = 	3,
	.bNumEndpoints =	    1,
	.bInterfaceClass =	    USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,	
	.bInterfaceProtocol = 	0x0,
	.iInterface =		    0,
};

static const struct usb_interface_descriptor
as_interface_alt_4_desc = {
	.bLength =		        USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	    USB_DT_INTERFACE,
	.bInterfaceNumber = 	1,
	.bAlternateSetting = 	4,
	.bNumEndpoints =	    1,
	.bInterfaceClass =	    USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = 	0x0,
	.iInterface =		    0,
};

/* B.4.2  Class-Specific AS Interface Descriptor */
static struct usb_as_header_descriptor as_header_desc = {
	.bLength =		USB_DT_AS_HEADER_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	AS_GENERAL,
	.bTerminalLink =	INPUT_TERMINAL_ID,
	.bDelay =		1,
	.wFormatTag =		__constant_cpu_to_le16(USB_AS_AUDIO_FORMAT_TYPE_I_PCM),
};

DECLARE_USB_AS_FORMAT_TYPE_I_DISCRETE_DESC(1);

static struct usb_as_formate_type_i_discrete_descriptor_1 as_type_i_1_desc = {
	.bLength =		USB_AS_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	FORMAT_TYPE,
	.bFormatType =		USB_AS_FORMAT_TYPE_I,
	.bNrChannels = 		2,
	.bSubframeSize =	2,
	.bBitResolution =	16,
	.bSamFreqType =		1,
	.tSamFreq = 		    {
	                        	[0][0]	= 0x80,
	                        	[0][1]	= 0xBB,
                        	}
};

#ifdef MULTIPLE_SETTINGS_SUPPORTED
static struct usb_as_formate_type_i_discrete_descriptor_1
as_type_i_2_desc= {
	.bLength = 		    USB_AS_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType = 	    USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = 	FORMAT_TYPE,

	.bFormatType = 		    USB_AS_FORMAT_TYPE_I,
	.bNrChannels = 		    4,
	.bSubframeSize = 	    2,
	.bBitResolution = 	    0x10,
	.bSamFreqType = 	    1,
	.tSamFreq = 		    {
	                        	[0][0]	= 0x80,
	                        	[0][1]	= 0xBB,
	                        }
};

static struct usb_as_formate_type_i_discrete_descriptor_1
as_type_i_3_desc= {
	.bLength = 		USB_AS_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType = 	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = 	FORMAT_TYPE,

	.bFormatType = 		USB_AS_FORMAT_TYPE_I,
	.bNrChannels = 		6,
	.bSubframeSize = 	2,
	.bBitResolution = 	0x10,
	.bSamFreqType = 	1,
	.tSamFreq = 		{
		[0][0]	= 0x80,
		[0][1]	= 0xBB,
	}
};

static struct usb_as_formate_type_i_discrete_descriptor_1
as_type_i_4_desc= {
	.bLength = 		USB_AS_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType = 	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = 	FORMAT_TYPE,

	.bFormatType = 		USB_AS_FORMAT_TYPE_I,
	.bNrChannels = 		8,
	.bSubframeSize = 	2,
	.bBitResolution = 	0x10,
	.bSamFreqType = 	1,
	.tSamFreq = 		{
		[0][0]	= 0x80,
		[0][1]	= 0xBB,
	}
};
#endif
/* Standard ISO OUT Endpoint Descriptor */
static struct usb_endpoint_descriptor as_out_ep_alt_1_desc = {
	.bLength =		USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	3,
	.bmAttributes =		USB_AS_ENDPOINT_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize =	__constant_cpu_to_le16(OUT_EP_MAX_PACKET_SIZE),
	.bInterval =		4,
	.bRefresh = 		0,
};
static struct usb_endpoint_descriptor
as_out_ep_alt_2_desc = {
	.bLength =			USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	3,
	.bmAttributes =		USB_AS_ENDPOINT_ADAPTIVE | USB_ENDPOINT_XFER_ISOC,
    	.wMaxPacketSize =   __constant_cpu_to_le16((2*OUT_EP_MAX_PACKET_SIZE)),
	.bInterval = 		4,
	.bRefresh = 		0,
};

static struct usb_endpoint_descriptor
as_out_ep_alt_3_desc = {
	.bLength =			USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	3,
	.bmAttributes =		USB_AS_ENDPOINT_ADAPTIVE | USB_ENDPOINT_XFER_ISOC,
  	.wMaxPacketSize =  	__constant_cpu_to_le16((3*OUT_EP_MAX_PACKET_SIZE)),
	.bInterval = 		4,
	.bRefresh = 		0,
};

static struct usb_endpoint_descriptor
as_out_ep_alt_4_desc = {
	.bLength =			USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	3,
	.bmAttributes =		USB_AS_ENDPOINT_ADAPTIVE | USB_ENDPOINT_XFER_ISOC,
    .wMaxPacketSize =  	__constant_cpu_to_le16((4*OUT_EP_MAX_PACKET_SIZE)),
	.bInterval = 		4,
	.bRefresh = 		0,
};

/* Class-specific AS ISO OUT Endpoint Descriptor */
static struct usb_as_iso_endpoint_descriptor as_iso_out_desc __initdata = {
	.bLength =		USB_AS_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType =	USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype =	EP_GENERAL,
	.bmAttributes = 	0,
	.bLockDelayUnits =	0,
	.wLockDelay =		__constant_cpu_to_le16(0),
};

//#define MULTIPLE_SETTINGS_SUPPORTED
static struct usb_descriptor_header *f_audio_desc[] __initdata = {
    (struct usb_descriptor_header *)&ac_interface_desc,
    (struct usb_descriptor_header *)&ac_header_desc,

    (struct usb_descriptor_header *)&input_terminal_desc,
    (struct usb_descriptor_header *)&output_terminal_desc,
#ifdef FEATURE_UNIT_SUPPORTED
    (struct usb_descriptor_header *)&feature_unit_desc,
#endif
	(struct usb_descriptor_header *) &as_interface_alt_0_desc,
	(struct usb_descriptor_header *) &as_interface_alt_1_desc,
	(struct usb_descriptor_header *) &as_header_desc,
	(struct usb_descriptor_header *) &as_type_i_1_desc,
	(struct usb_descriptor_header *) &as_out_ep_alt_1_desc,
	(struct usb_descriptor_header *) &as_iso_out_desc,

#ifdef MULTIPLE_SETTINGS_SUPPORTED
	(struct usb_descriptor_header *) &as_interface_alt_2_desc,
	(struct usb_descriptor_header *) &as_header_desc,
	(struct usb_descriptor_header *) &as_type_i_2_desc,
	(struct usb_descriptor_header *) &as_out_ep_alt_2_desc,
	(struct usb_descriptor_header *) &as_iso_out_desc,

	(struct usb_descriptor_header *) &as_interface_alt_3_desc,
	(struct usb_descriptor_header *) &as_header_desc,
	(struct usb_descriptor_header *) &as_type_i_3_desc,
	(struct usb_descriptor_header *) &as_out_ep_alt_3_desc,
	(struct usb_descriptor_header *) &as_iso_out_desc,

	(struct usb_descriptor_header *) &as_interface_alt_4_desc,
	(struct usb_descriptor_header *) &as_header_desc,
	(struct usb_descriptor_header *) &as_type_i_4_desc,
	(struct usb_descriptor_header *) &as_out_ep_alt_4_desc,
	(struct usb_descriptor_header *) &as_iso_out_desc,
#endif
	NULL,
};

/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1

static char manufacturer[50];

static struct usb_string strings_dev[] = {
	[STRING_MANUFACTURER_IDX].s = manufacturer,
	[STRING_PRODUCT_IDX].s = DRIVER_DESC,
	{  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *audio_strings[] = {
	&stringtab_dev,
	NULL,
};

/*
 * This function is an ALSA sound card following USB Audio Class Spec 1.0.
 */

/*-------------------------------------------------------------------------*/
struct f_audio_buf {
	u8 *buf;
	int actual;
	struct list_head list;
};

static struct f_audio_buf *f_audio_buffer_alloc(int buf_size)
{
	struct f_audio_buf *copy_buf;

	copy_buf = kzalloc(sizeof *copy_buf, GFP_ATOMIC);
	if (!copy_buf)
		return NULL;

	copy_buf->buf = kzalloc(buf_size, GFP_ATOMIC);
	if (!copy_buf->buf) {
		kfree(copy_buf);
		return NULL;
	}

	return copy_buf;
}

static void f_audio_buffer_free(struct f_audio_buf *audio_buf)
{
	kfree(audio_buf->buf);
	kfree(audio_buf);
}
/*-------------------------------------------------------------------------*/

struct f_audio {
	struct gaudio			card;

	/* endpoints handle full and/or high speeds */
	struct usb_ep			*out_ep;
	struct usb_endpoint_descriptor	*out_desc;

	spinlock_t			lock;
	struct f_audio_buf *copy_buf;
	struct work_struct playback_work;
	struct list_head play_queue;

    	/*Queue for maintaining request buffers.*/
	struct list_head req_queue;
	/* Control Set command */
	struct list_head cs;
	u8 set_cmd;
	u8			interface;
	u8			altSetting;
	u8			curAltSetting;
	unsigned 		urb_created;
	struct usb_audio_control *set_con;
};

static inline struct f_audio *func_to_audio(struct usb_function *f)
{
	return container_of(f, struct f_audio, card.func);
}


static u32 dst[MAX_AUDIO_CHAN][AUDIO_FRAME_SIZE/sizeof(u32)];
//#define BUFDUMP_ENABLED_WQ 1
//__attribute_used__ noinline static int audio_playback(struct audio_dev *audio, void *buf, int count)
static int audio_playback(struct f_audio *audio, void *buf, int count)
{
    int i, offset;
    int altSetting;
    #ifdef WLAN_AOW_ENEBLED
    off_t tsf;
    #endif
    u32 *src;
    int cnt, loop_count, extra;
    altSetting = (int)(audio->altSetting);
    src = buf;
    if((altSetting >4) || (altSetting < 1)) {
    return 0;
    }
    if(count < audio_buf_size) {
	cnt = count/(AUDIO_FRAME_SIZE*altSetting);
	extra = count%(AUDIO_FRAME_SIZE);

    } else
    {
    	cnt = BUF_SIZE_FACTOR/altSetting;
	    extra = 0;
    }
    count_audio_playback++;
    while (cnt--) {
  	    for (offset = 0; offset < (AUDIO_FRAME_SIZE/4); offset++) {
  	    for (i = 0; i < altSetting; i++)
		    dst[i][offset] = *src++;
        }
#ifdef WLAN_AOW_ENEBLED
       	wlan_get_tsf(&tsf);
  	    for (i = 0; i < altSetting; i++) {
        	wlan_aow_tx((char *)&(dst[i][0]), AUDIO_FRAME_SIZE, i, tsf);
        }
#endif
#ifdef I2S_ENABLED
        if (i2s_st) {
#if defined(CONFIG_MACH_AR934x) || \
    defined(CONFIG_MACH_QCA955x)
	        ath_ex_i2s_open();
            ath_ex_i2s_set_freq(48000);
#else
	        ar7242_i2s_open();
            ar7240_i2s_clk(63565868, 9091);
#endif
            i2s_st = i2s_write_cnt = 0;
        }
#if defined(CONFIG_MACH_AR934x) || \
    defined(CONFIG_MACH_QCA955x)
	    ath_ex_i2s_write(AUDIO_FRAME_SIZE, (char *)&(dst[0][0]), 1);
#else
	ar7242_i2s_write(AUDIO_FRAME_SIZE, (char *)&(dst[0][0]), 1);
#endif
#endif
    }

    if(extra != 0) {
	cnt = extra/altSetting;
	loop_count = cnt/4;
  	for (offset = 0; offset < loop_count; offset++) {
            for (i = 0; i < altSetting; i++) {
                dst[i][offset] = *src++;
            }
        }

#ifdef WLAN_AOW_ENEBLED
  	for (i = 0; i < altSetting; i++) {
    		wlan_aow_tx((char *)&(dst[i][0]), cnt, i, tsf);
    	}
#endif
#ifdef I2S_ENABLED
#if defined(CONFIG_MACH_AR934x) || \
    defined(CONFIG_MACH_QCA955x)
	ath_ex_i2s_write(cnt, (char *)&(dst[0][0]), 1);
#else
	ar7242_i2s_write(cnt, (char *)&(dst[0][0]), 1);
#endif
#endif
    }
    return 0;
}


/*-------------------------------------------------------------------------*/

static void f_audio_playback_work(struct work_struct *data)
{
	struct f_audio *audio = container_of(data, struct f_audio,
					playback_work);
	struct f_audio_buf *play_buf;

	spin_lock_irq(&audio->lock);
	if (list_empty(&audio->play_queue)) {
		spin_unlock_irq(&audio->lock);
		return;
	}
	play_buf = list_first_entry(&audio->play_queue,
			struct f_audio_buf, list);
	list_del(&play_buf->list);
	spin_unlock_irq(&audio->lock);

	audio_playback(audio, play_buf->buf, play_buf->actual);
#if 0
	u_audio_playback(&audio->card, play_buf->buf, play_buf->actual);
#endif
	f_audio_buffer_free(play_buf);

	return;
}

static int f_audio_out_ep_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_audio *audio = req->context;
	struct usb_composite_dev *cdev = audio->card.func.config->cdev;
	struct f_audio_buf *copy_buf = audio->copy_buf;
	int err;

	if (!copy_buf)
		return -EINVAL;

	/* Copy buffer is full, add it to the play_queue */
	if (audio_buf_size - copy_buf->actual < req->actual) {
		memcpy(copy_buf->buf + copy_buf->actual, req->buf, (audio_buf_size - copy_buf->actual));
		copy_buf->actual += (audio_buf_size - copy_buf->actual);
		list_add_tail(&copy_buf->list, &audio->play_queue);
		schedule_work(&audio->playback_work);
		req->actual -= (audio_buf_size - copy_buf->actual);
		req->buf += (audio_buf_size - copy_buf->actual);
		copy_buf = f_audio_buffer_alloc(audio_buf_size);
		if (copy_buf == NULL ) {
            		audio->copy_buf = NULL;
		}
	}

    if ( audio->copy_buf != NULL) {
	memcpy(copy_buf->buf + copy_buf->actual, req->buf, req->actual);
	copy_buf->actual += req->actual;
	audio->copy_buf = copy_buf;

	err = usb_ep_queue(ep, req, GFP_ATOMIC);
	    if (err) {
		ERROR(cdev, "%s queue req: %d\n", ep->name, err);
        }
    }

	return 0;

}

static void f_audio_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_audio *audio = req->context;
	int status = req->status;
	u32 data = 0;
	struct usb_ep *out_ep = audio->out_ep;

	switch (status) {

	case 0:				/* normal completion? */
		if (ep == out_ep)
			f_audio_out_ep_complete(ep, req);
		else if (audio->set_con) {
			memcpy(&data, req->buf, req->length);
			audio->set_con->set(audio->set_con, audio->set_cmd,
					le16_to_cpu(data));
			audio->set_con = NULL;
		}
		break;
	default:
		break;
	}
}

static int audio_set_intf_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_audio		*audio = func_to_audio(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	u8			id = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
	u16			len = le16_to_cpu(ctrl->wLength);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u8			con_sel = (w_value >> 8) & 0xFF;
	u8			cmd = (ctrl->bRequest & 0x0F);
	struct usb_audio_control_selector *cs;
	struct usb_audio_control *con;

	DBG(cdev, "bRequest 0x%x, w_value 0x%04x, len %d, entity %d\n",
			ctrl->bRequest, w_value, len, id);

	list_for_each_entry(cs, &audio->cs, list) {
		if (cs->id == id) {
			list_for_each_entry(con, &cs->control, list) {
				if (con->type == con_sel) {
					audio->set_con = con;
					break;
				}
			}
			break;
		}
	}

	audio->set_cmd = cmd;
	req->context = audio;
	req->complete = f_audio_complete;

	return len;
}

static int audio_get_intf_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_audio		*audio = func_to_audio(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u8			id = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
	u16			len = le16_to_cpu(ctrl->wLength);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u8			con_sel = (w_value >> 8) & 0xFF;
	u8			cmd = (ctrl->bRequest & 0x0F);
	struct usb_audio_control_selector *cs;
	struct usb_audio_control *con;

	DBG(cdev, "bRequest 0x%x, w_value 0x%04x, len %d, entity %d\n",
			ctrl->bRequest, w_value, len, id);

	list_for_each_entry(cs, &audio->cs, list) {
		if (cs->id == id) {
			list_for_each_entry(con, &cs->control, list) {
				if (con->type == con_sel && con->get) {
					value = con->get(con, cmd);
					break;
				}
			}
			break;
		}
	}

	req->context = audio;
	req->complete = f_audio_complete;
	memcpy(req->buf, &value, len);

	return len;
}

static int
f_audio_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	/* composite driver infrastructure handles everything except
	 * Audio class messages; interface activation uses set_alt().
	 */
	switch (ctrl->bRequestType) {
	case USB_AUDIO_SET_INTF:
		value = audio_set_intf_req(f, ctrl);
		break;

	case USB_AUDIO_GET_INTF:
		value = audio_get_intf_req(f, ctrl);
		break;

	default:
		ERROR(cdev, "invalid control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		DBG(cdev, "audio req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "audio response on err %d\n", value);
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}

static void free_out_ep_reqs(struct f_audio *audio)
{
    struct usb_request  *req;

	while (!list_empty(&audio->req_queue)) {
		req = list_first_entry(&(audio->req_queue), struct usb_request, list);
		list_del (&req->list);
		kfree(req->buf);
        usb_ep_dequeue(audio->out_ep, req);
		usb_ep_free_request (audio->out_ep, req);
	}
}

static int f_audio_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_audio		*audio = func_to_audio(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_ep *out_ep = audio->out_ep;
	struct usb_request *req;
	int i = 0, err = 0;

	DBG(cdev, "intf %d, alt %d\n", intf, alt);
	ERROR(cdev, "intf %d, alt %d\n", intf, alt);

	if (intf == 1) {
		if (alt != 0) {
			audio->curAltSetting = (u8)alt; 
			if(!(audio->copy_buf)) {
				audio->copy_buf = f_audio_buffer_alloc(audio_buf_size);
			}
			if (audio->copy_buf == NULL) { 
				return -ENOMEM;
			}
			if((1 != audio->urb_created) && (audio->altSetting != alt)) {
				usb_ep_disable(out_ep);
				free_out_ep_reqs(audio);
				audio->altSetting = (u8)alt;
				audio->curAltSetting = (u8)alt;
				switch(alt) {
					case 1:
						audio->out_desc = &as_out_ep_alt_1_desc;
						break;
					case 2:
						audio->out_desc = &as_out_ep_alt_2_desc;
						break;
					case 3:
						audio->out_desc = &as_out_ep_alt_3_desc;
						break;
					case 4:
						audio->out_desc = &as_out_ep_alt_4_desc;
						break;
					default:
						ERROR (cdev, "Invalid Alternate Setting: intf %d, alt %d\n", intf, alt);
				}

				usb_ep_enable(out_ep, audio->out_desc);
				out_ep->driver_data = audio;

				/*
				 * allocate a bunch of read buffers
				 * and queue them all at once.
				 */
				for (i = 0; i < req_count && err == 0; i++) {
					req = usb_ep_alloc_request(out_ep, GFP_ATOMIC);
					if (req) {
						req->buf = kzalloc(req_buf_size,
								GFP_ATOMIC);
						if (req->buf) {
							req->length = req_buf_size;
							req->context = audio;
							req->complete =
								f_audio_complete;
							err = usb_ep_queue(out_ep,
									req, GFP_ATOMIC);
							if (err) {
								ERROR(cdev, "%s queue req: %d\n", out_ep->name, err);
							}
						} else {
							err = -ENOMEM;
						}
					} else {
						err = -ENOMEM;
					}
				        list_add_tail(&req->list, &audio->req_queue);
				}
					audio->urb_created = 1;
				}
		} else {
			struct f_audio_buf *copy_buf;

			while (!list_empty(&audio->play_queue)) {
				struct f_audio_buf *play_buf;
				play_buf = list_first_entry(&audio->play_queue,
						struct f_audio_buf, list);
				list_del(&play_buf->list);
				f_audio_buffer_free(play_buf);
			}
			copy_buf = audio->copy_buf;
			audio->curAltSetting = alt;
			if (copy_buf) {
				if (copy_buf->actual != 0) {
					audio->copy_buf = NULL;
					list_add_tail(&copy_buf->list,
							&audio->play_queue);
					schedule_work(&audio->playback_work);
				}
			}
		}
	}

	ERROR(cdev, "TP4: intf %d, alt %d Err = %d\n", intf, alt, err);
	return err;
}


static int f_audio_get_alt(struct usb_function *f, unsigned intf)
{
	
	struct f_audio		*audio = func_to_audio(f);
	int sts;
	if(!intf) {
		sts = 0;
	}
	else {
		sts = audio->curAltSetting;
	}
	return sts;
}

static void f_audio_disable(struct usb_function *f)
{
	struct f_audio		*audio = func_to_audio(f);
	printk(KERN_ALERT "Inside f_audio_disable\n");
	audio->urb_created = 0;
	return;
}

/*-------------------------------------------------------------------------*/

static void f_audio_build_desc(struct f_audio *audio)
{
//	struct gaudio *card = &audio->card;
//	u8 *sam_freq;
//	int rate;

	/* Set channel numbers */
//	input_terminal_desc.bNrChannels = u_audio_get_playback_channels(card);
//	as_type_i_1_desc.bNrChannels = u_audio_get_playback_channels(card);

	/* Set sample rates */
//	rate = u_audio_get_playback_rate(card);
//	sam_freq = as_type_i_1_desc.tSamFreq[0];
//	memcpy(sam_freq, &rate, 3);

	/* Todo: Set Sample bits and other parameters */

	return;
}

/* audio function driver setup/binding */
static int __init
f_audio_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_audio		*audio = func_to_audio(f);
	int			status;
	struct usb_ep		*ep;

	f_audio_build_desc(audio);

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	ac_interface_desc.bInterfaceNumber = status;

	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	as_interface_alt_0_desc.bInterfaceNumber = status;
	as_interface_alt_1_desc.bInterfaceNumber = status;

	status = -ENODEV;

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &as_out_ep_alt_1_desc);
	if (!ep)
		goto fail;
	audio->out_ep = ep;
	ep->driver_data = cdev;	/* claim */

	status = -ENOMEM;

	/* supcard all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */

	/* copy descriptors, and track endpoint copies */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		c->highspeed = true;
		c->fullspeed = true;
		f->hs_descriptors = usb_copy_descriptors(f_audio_desc);
		f->descriptors = usb_copy_descriptors(f_audio_desc);
	} else 
		f->descriptors = usb_copy_descriptors(f_audio_desc);
	
	return 0;

fail:

	return status;
}

static void
f_audio_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_audio		*audio = func_to_audio(f);

	ERROR(cdev, "Inside f_audio_unbind\n");
	audio->urb_created = 0;
	usb_free_descriptors(f->hs_descriptors);
	usb_free_descriptors(f->descriptors);
	kfree(audio);
}

/*-------------------------------------------------------------------------*/

/* Todo: add more control selecotor dynamically */
int __init control_selector_init(struct f_audio *audio)
{
	INIT_LIST_HEAD(&audio->cs);
#ifdef FEATURE_UNIT_SUPPORTED
	list_add(&feature_unit.list, &audio->cs);

	INIT_LIST_HEAD(&feature_unit.control);
	list_add(&mute_control.list, &feature_unit.control);
	list_add(&volume_control.list, &feature_unit.control);
	volume_control.data[_CUR] = 0xffc0;
	volume_control.data[_MIN] = 0xe3a0;
	volume_control.data[_MAX] = 0xfff0;
	volume_control.data[_RES] = 0x0030;
#endif

	return 0;
}
static int gaudio_read_procmem(char *buf, char **start, off_t offset,
                                int count, int *eof, void *data)
{
    struct f_audio *audio = (struct f_audio *)data;
    int len = 0;
    len += sprintf(buf+len,"\nAudio playback count = %li\n", count_audio_playback);
    len += sprintf(buf+len,"\nAlternate Setting = %i Current Alt Setting = %i\n", \
			audio->altSetting, audio->curAltSetting);
    return len;
}
#define BUFDUMP_ENABLED_WQ 1
static int gaudio_readdata_procmem(char *buf, char **start, off_t offset,
                                int count, int *eof, void *data)
{
    int i, len = 0;
    char *src;

    src = (char*)(&dst[0][0]);
    #ifdef BUFDUMP_ENABLED_WQ
    for(i=0; i < count; i++) {
        len += sprintf(buf+len,"%x", src[i]);
    }
    #endif
    return len;
}

/**
 * audio_bind_config - add USB audio fucntion to a configuration
 * @c: the configuration to supcard the USB audio function
 * Context: single threaded during gadget setup
 *
 * Returns zero on success, else negative errno.
 */
int __init audio_bind_config(struct usb_configuration *c)
{
	struct f_audio *audio;
	int status;

	/* allocate and initialize one new instance */
	audio = kzalloc(sizeof *audio, GFP_KERNEL);
	if (!audio)
		return -ENOMEM;

	audio->card.func.name = "g_audio";
	audio->card.gadget = c->cdev->gadget;

    	create_proc_read_entry("gaudio", 0, NULL, gaudio_read_procmem, audio);
    	create_proc_read_entry("auddata", 0, NULL, gaudio_readdata_procmem, audio);
	INIT_LIST_HEAD(&audio->play_queue);
	INIT_LIST_HEAD(&audio->req_queue);
	spin_lock_init(&audio->lock);
#ifdef REMOVED_ALSA
	/* set up ASLA audio devices */
	status = gaudio_setup(&audio->card);
	if (status < 0)
		goto setup_fail;
#endif

#ifdef I2S_ENABLED
	//ar7242_i2s_open();
	//ar7240_i2s_clk(63565868, 9091);
	i2s_st = 1;
	i2s_write_cnt = 0;
#endif
	audio->card.func.strings = audio_strings;
	audio->card.func.bind = f_audio_bind;
	audio->card.func.unbind = f_audio_unbind;
	audio->card.func.set_alt = f_audio_set_alt;
	audio->card.func.get_alt = f_audio_get_alt;
	audio->card.func.setup = f_audio_setup;
	audio->card.func.disable = f_audio_disable;
	audio->out_desc = &as_out_ep_alt_1_desc;

	control_selector_init(audio);

	INIT_WORK(&audio->playback_work, f_audio_playback_work);

	status = usb_add_function(c, &audio->card.func);
	if (status)
		goto add_fail;

	INFO(c->cdev, "audio_buf_size %d, req_buf_size %d, req_count %d\n",
		audio_buf_size, req_buf_size, req_count);

	return status;

add_fail:
	gaudio_cleanup(&audio->card);
#ifdef REMOVED_ALSA
setup_fail:
#endif
	kfree(audio);
	return status;
}
