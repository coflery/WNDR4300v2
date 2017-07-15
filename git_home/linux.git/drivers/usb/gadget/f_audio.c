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

#ifdef WLAN_AOW_ENABLED
#include "if_aow.h"         /* include the aow_dev interface header file */
#endif


#define MAX_AUDIO_CHAN (4)
#define BUF_SIZE_FACTOR 4
#define OUT_EP_MAX_PACKET_SIZE 192
#define AUDIO_FRAME_SIZE        ((OUT_EP_MAX_PACKET_SIZE)*8)
#define AUDIO_BUF_SIZE          (AUDIO_FRAME_SIZE * BUF_SIZE_FACTOR)

static unsigned int req_count = 256;
module_param(req_count, uint, S_IRUGO);
MODULE_PARM_DESC(req_count, "ISO OUT endpoint request count");

static unsigned int req_buf_size = OUT_EP_MAX_PACKET_SIZE;
module_param(req_buf_size, uint, S_IRUGO);
MODULE_PARM_DESC(req_buf_size, "ISO OUT endpoint request buffer size");

static unsigned int g_audio_frame_size = AUDIO_FRAME_SIZE;

static unsigned int g_audio_buf_size = AUDIO_BUF_SIZE;
module_param(g_audio_buf_size, uint, S_IRUGO);
MODULE_PARM_DESC(g_audio_buf_size, "Audio buffer size");

/*
 * Note: In order to support various sample sizes, we should
 * ideally change the design of some parts of this code so 
 * that it is made generic and a profile structure is used.
 * However, for now, we only port the working solution from 2.6.15 - this
 * in turn was meant to add to the existing code there with minimum
 * potential disruption to other users of the driver.
 * In the future, the design change can be carried out by interested
 * teams if desired.
 */

#ifdef USB_24BIT_AUDIO_ENABLED

/*  OUT_EP_MAX_PACKET_SIZE_24BIT is equivalent of 
 *  OUT_EP_MAX_PACKET_SIZE, but for
 *  24-bit samples rather than the
 *  default 16-bit samples. 
 */
#define OUT_EP_MAX_PACKET_SIZE_24BIT  288   
#define AUDIO_FRAME_SIZE_24BIT         ((OUT_EP_MAX_PACKET_SIZE_24BIT) * 8)
#define AUDIO_BUF_SIZE_24BIT           (AUDIO_FRAME_SIZE_24BIT * BUF_SIZE_FACTOR)

static unsigned int req_buf_size_24bit = OUT_EP_MAX_PACKET_SIZE_24BIT;
module_param(req_buf_size_24bit, uint, S_IRUGO);
MODULE_PARM_DESC(req_buf_size_24bit, "ISO OUT endpoint request buffer size "
                                     "for 24-bit audio");

static unsigned int g_audio_frame_size_24bit =  AUDIO_FRAME_SIZE_24BIT;

static unsigned int g_audio_buf_size_24bit = AUDIO_BUF_SIZE_24BIT;
module_param(g_audio_buf_size_24bit, uint, S_IRUGO);
MODULE_PARM_DESC(g_audio_buf_size_24bit, "Audio buffer size for 24-bit audio");

static unsigned int max_audio_buf_size = AUDIO_BUF_SIZE_24BIT;
#else
static unsigned int max_audio_buf_size = AUDIO_BUF_SIZE;
#endif /* USB_24BIT_AUDIO_ENABLED */

module_param(max_audio_buf_size, uint, S_IRUGO);
MODULE_PARM_DESC(max_audio_buf_size, "Max audio buffer size");

#define MIN_FRAME_SIZE_FACTOR       (1)
#define MAX_FRAME_SIZE_FACTOR       (4)
#define DEFAULT_FRAME_SIZE_FACTOR   (4)

static int list_element_no;

#ifdef WLAN_AOW_ENABLED

#define AOWCTRL_QUEUE_LEN 5
#define DEFAULT_EXPECTED_ALT_SETTING     (7)
#define MIN_EXPECTED_ALT_SETTING         (1)
#define MAX_EXPECTED_ALT_SETTING         (8)

static unsigned int g_expected_alt_setting = DEFAULT_EXPECTED_ALT_SETTING;
unsigned int g_alt_setting_init_done = 0;

/* AoW Local Data types */
aow_dev_t usb_aow_dev;      /* USB AoW Audio device instance */

typedef struct usb_info {
    struct f_audio  *dev;
} usb_info_t;

usb_info_t usbinfo;

/* AoW Local Prototypes */
static int is_aow_wlan_calls_registered(void);
static void init_usb_aow_dev(void);
static int set_wlan_audioparams(u8 bBitResolution, u8 tSamFreq[1][3]);

#endif /* WLAN_AOW_ENABLED */

/* Use of I2S_ENABLED with USB_24BIT_AUDIO_ENABLED not (yet) supported */
#define I2S_ENABLED 1
//#undef I2S_ENABLED

#ifdef I2S_ENABLED
static int i2s_st;
static int i2s_write_cnt;
#if defined(CONFIG_MACH_AR934x) || \
    defined(CONFIG_MACH_QCA955x)
extern uint32_t ath_ref_freq;
extern void ath_ex_i2s_set_freq(uint32_t);
extern void ath_i2s_clk(unsigned long, unsigned long);
extern void ath_i2s_dpll();
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

#ifdef USB_24BIT_AUDIO_ENABLED

static struct usb_interface_descriptor as_interface_alt_5_desc = {
    .bLength =              USB_DT_INTERFACE_SIZE,
    .bDescriptorType =      USB_DT_INTERFACE,
    .bInterfaceNumber =     1,
    .bAlternateSetting =    5,
    .bNumEndpoints =        1,
    .bInterfaceClass =      USB_CLASS_AUDIO,
    .bInterfaceSubClass =   USB_SUBCLASS_AUDIOSTREAMING,
};

static const struct usb_interface_descriptor
as_interface_alt_6_desc = {
    .bLength =              USB_DT_INTERFACE_SIZE,
    .bDescriptorType =      USB_DT_INTERFACE,
    .bInterfaceNumber =     1,
    .bAlternateSetting =    6,
    .bNumEndpoints =        1,
    .bInterfaceClass =      USB_CLASS_AUDIO,
    .bInterfaceSubClass =   USB_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol =   0x0,
    .iInterface =           0,
};

static const struct usb_interface_descriptor
as_interface_alt_7_desc = {
    .bLength =              USB_DT_INTERFACE_SIZE,
    .bDescriptorType =      USB_DT_INTERFACE,
    .bInterfaceNumber =     1,
    .bAlternateSetting =    7,
    .bNumEndpoints =        1,
    .bInterfaceClass =      USB_CLASS_AUDIO,
    .bInterfaceSubClass =   USB_SUBCLASS_AUDIOSTREAMING,    
    .bInterfaceProtocol =   0x0,
    .iInterface =           0,
};

static const struct usb_interface_descriptor
as_interface_alt_8_desc = {
    .bLength =              USB_DT_INTERFACE_SIZE,
    .bDescriptorType =      USB_DT_INTERFACE,
    .bInterfaceNumber =     1,
    .bAlternateSetting =    8,
    .bNumEndpoints =        1,
    .bInterfaceClass =      USB_CLASS_AUDIO,
    .bInterfaceSubClass =   USB_SUBCLASS_AUDIOSTREAMING,
    .bInterfaceProtocol =   0x0,
    .iInterface =           0,
};

#endif /* USB_24BIT_AUDIO_ENABLED */


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

#ifdef USB_24BIT_AUDIO_ENABLED

static struct usb_as_formate_type_i_discrete_descriptor_1 as_type_i_5_desc = {
    .bLength =            USB_AS_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
    .bDescriptorType =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = FORMAT_TYPE,
    .bFormatType =        USB_AS_FORMAT_TYPE_I,
    .bNrChannels =        2,
    .bSubframeSize =      3,
    .bBitResolution =     0x18,
    .bSamFreqType =       1,
    .tSamFreq =           {
                                [0][0]    = 0x80,
                                [0][1]    = 0xBB,
                          }
};
static struct usb_as_formate_type_i_discrete_descriptor_1
as_type_i_6_desc= {
    .bLength =            USB_AS_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
    .bDescriptorType =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = FORMAT_TYPE,

    .bFormatType =        USB_AS_FORMAT_TYPE_I,
    .bNrChannels =        4,
    .bSubframeSize =      3,
    .bBitResolution =     0x18,
    .bSamFreqType =       1,
    .tSamFreq =           {
                                [0][0]    = 0x80,
                                [0][1]    = 0xBB,
                          }
};

static struct usb_as_formate_type_i_discrete_descriptor_1
as_type_i_7_desc= {
    .bLength =            USB_AS_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
    .bDescriptorType =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = FORMAT_TYPE,

    .bFormatType =        USB_AS_FORMAT_TYPE_I,
    .bNrChannels =        6,
    .bSubframeSize =      3,
    .bBitResolution =     0x18,
    .bSamFreqType =       1,
    .tSamFreq =           {
                                [0][0]    = 0x80,
                                [0][1]    = 0xBB,
                          }
};

static struct usb_as_formate_type_i_discrete_descriptor_1
as_type_i_8_desc= {
    .bLength =            USB_AS_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
    .bDescriptorType =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype = FORMAT_TYPE,

    .bFormatType =        USB_AS_FORMAT_TYPE_I,
    .bNrChannels =        8,
    .bSubframeSize =      3,
    .bBitResolution =     0x18,
    .bSamFreqType =       1,
    .tSamFreq =           {
                                [0][0]    = 0x80,
                                [0][1]    = 0xBB,
                          }
};

#endif /* USB_24BIT_AUDIO_ENABLED */


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

#ifdef USB_24BIT_AUDIO_ENABLED

static struct usb_endpoint_descriptor as_out_ep_alt_5_desc = {
    .bLength =            USB_DT_ENDPOINT_AUDIO_SIZE,
    .bDescriptorType =    USB_DT_ENDPOINT,
    .bEndpointAddress =   3,
    .bmAttributes =       USB_AS_ENDPOINT_ADAPTIVE
                          | USB_ENDPOINT_XFER_ISOC,
    .wMaxPacketSize =     __constant_cpu_to_le16(OUT_EP_MAX_PACKET_SIZE_24BIT),
    .bInterval =          4,
    .bRefresh =           0,
};

static struct usb_endpoint_descriptor
as_out_ep_alt_6_desc = {
    .bLength =            USB_DT_ENDPOINT_AUDIO_SIZE,
    .bDescriptorType =    USB_DT_ENDPOINT,

    .bEndpointAddress =   3,
    .bmAttributes =       USB_AS_ENDPOINT_ADAPTIVE | USB_ENDPOINT_XFER_ISOC,
    .wMaxPacketSize =      __constant_cpu_to_le16((2 * OUT_EP_MAX_PACKET_SIZE_24BIT)),
    .bInterval =          4,
    .bRefresh =           0,
};

static struct usb_endpoint_descriptor
as_out_ep_alt_7_desc = {
    .bLength =            USB_DT_ENDPOINT_AUDIO_SIZE,
    .bDescriptorType =    USB_DT_ENDPOINT,

    .bEndpointAddress =   3,
    .bmAttributes =       USB_AS_ENDPOINT_ADAPTIVE | USB_ENDPOINT_XFER_ISOC,
    .wMaxPacketSize =     __constant_cpu_to_le16((3 * OUT_EP_MAX_PACKET_SIZE_24BIT)),
    .bInterval =          4,
    .bRefresh =           0,
};

static struct usb_endpoint_descriptor
as_out_ep_alt_8_desc = {
    .bLength =            USB_DT_ENDPOINT_AUDIO_SIZE,
    .bDescriptorType =    USB_DT_ENDPOINT,

    .bEndpointAddress =   3,
    .bmAttributes =       USB_AS_ENDPOINT_ADAPTIVE | USB_ENDPOINT_XFER_ISOC,
    .wMaxPacketSize =      __constant_cpu_to_le16((4 * OUT_EP_MAX_PACKET_SIZE_24BIT)),
    .bInterval =          4,
    .bRefresh =           0,
};

#endif /* USB_24BIT_AUDIO_ENABLED */

/* Class-specific AS ISO OUT Endpoint Descriptor */
static struct usb_as_iso_endpoint_descriptor as_iso_out_desc __initdata = {
	.bLength =		USB_AS_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType =	USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype =	EP_GENERAL,
	.bmAttributes = 	0,
	.bLockDelayUnits =	0,
	.wLockDelay =		__constant_cpu_to_le16(0),
};

#ifdef WLAN_AOW_ENABLED

#define AOWCTRL_IN_EP_INTERVAL          3
#define AOWCTRL_OUT_EP_INTERVAL         4
#define AOWCTRL_PROT_BYTECOUNT          512

static struct usb_interface_descriptor
vs_aowctrl_intf = {
        .bLength =              USB_DT_INTERFACE_SIZE,
        .bDescriptorType =      USB_DT_INTERFACE,
        .bInterfaceNumber =     2,
        .bAlternateSetting =    0,

        .bNumEndpoints =        2,
        .bInterfaceClass =      USB_CLASS_AUDIO,
        .bInterfaceSubClass =   USB_SUBCLASS_VENDOR_SPEC,
        .bInterfaceProtocol =   0x0,
        .iInterface =           0x0,
};

static struct usb_endpoint_descriptor
vs_aowctrl_out_ep_desc = {
        .bLength =              USB_DT_ENDPOINT_SIZE,
        .bDescriptorType =      USB_DT_ENDPOINT,

        .bEndpointAddress =     USB_DIR_OUT | 1,
        .bmAttributes =         USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize =       __constant_cpu_to_le16 (AOWCTRL_PROT_BYTECOUNT),
        .bInterval =            AOWCTRL_OUT_EP_INTERVAL,
};

static struct usb_endpoint_descriptor
vs_aowctrl_in_ep_desc = {
        .bLength =              USB_DT_ENDPOINT_SIZE,
        .bDescriptorType =      USB_DT_ENDPOINT,

        .bEndpointAddress =     USB_DIR_IN | 1,
        .bmAttributes =         USB_ENDPOINT_XFER_BULK,
        .wMaxPacketSize =       __constant_cpu_to_le16 (AOWCTRL_PROT_BYTECOUNT),
        .bInterval =            AOWCTRL_IN_EP_INTERVAL,
};

#endif /* WLAN_AOW_ENABLED */


#define MULTIPLE_SETTINGS_SUPPORTED
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

#ifdef USB_24BIT_AUDIO_ENABLED
    (struct usb_descriptor_header *) &as_interface_alt_5_desc,
	(struct usb_descriptor_header *) &as_header_desc,
	(struct usb_descriptor_header *) &as_type_i_5_desc,
	(struct usb_descriptor_header *) &as_out_ep_alt_5_desc,
	(struct usb_descriptor_header *) &as_iso_out_desc,
    
    (struct usb_descriptor_header *) &as_interface_alt_6_desc,
	(struct usb_descriptor_header *) &as_header_desc,
	(struct usb_descriptor_header *) &as_type_i_6_desc,
	(struct usb_descriptor_header *) &as_out_ep_alt_6_desc,
	(struct usb_descriptor_header *) &as_iso_out_desc,

	(struct usb_descriptor_header *) &as_interface_alt_7_desc,
	(struct usb_descriptor_header *) &as_header_desc,
	(struct usb_descriptor_header *) &as_type_i_7_desc,
	(struct usb_descriptor_header *) &as_out_ep_alt_7_desc,
	(struct usb_descriptor_header *) &as_iso_out_desc,

	(struct usb_descriptor_header *) &as_interface_alt_8_desc,
	(struct usb_descriptor_header *) &as_header_desc,
	(struct usb_descriptor_header *) &as_type_i_8_desc,
	(struct usb_descriptor_header *) &as_out_ep_alt_8_desc,
	(struct usb_descriptor_header *) &as_iso_out_desc,
#endif /* USB_24BIT_AUDIO_ENABLED */
#endif /* MULTIPLE_SETTINGS_SUPPORTED */
#ifdef WLAN_AOW_ENABLED
    (struct usb_descriptor_header *) &vs_aowctrl_intf,
    (struct usb_descriptor_header *) &vs_aowctrl_out_ep_desc,
    (struct usb_descriptor_header *) &vs_aowctrl_in_ep_desc,
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
	u_int64_t tsf;
	struct list_head list;
};

#ifdef WLAN_AOW_ENABLED
struct aowctrl_buf {
        u8 *buf;
        int actual;
        struct list_head list;
};
#endif

static struct f_audio_buf *f_audio_buffer_alloc(int buf_size)
{
	struct f_audio_buf *copy_buf;

	copy_buf = kzalloc(sizeof *copy_buf, GFP_ATOMIC);
	if (!copy_buf)
		return (struct f_audio_buf *)-ENOMEM;

	copy_buf->buf = kzalloc(buf_size, GFP_ATOMIC);
	if (!copy_buf->buf) {
		kfree(copy_buf);
		return (struct f_audio_buf *)-ENOMEM;
	}

	return copy_buf;
}

static void f_audio_buffer_free(struct f_audio_buf *audio_buf)
{
	kfree(audio_buf->buf);
	kfree(audio_buf);
}

#ifdef WLAN_AOW_ENABLED
static struct aowctrl_buf* aowctrl_buffer_alloc(int buf_size)
{
    struct aowctrl_buf *outbuf;

    outbuf = kzalloc(sizeof(struct aowctrl_buf), GFP_ATOMIC);
    if (NULL == outbuf) {
        return NULL;
    }

    outbuf->buf = kzalloc(buf_size, GFP_ATOMIC);
    if (NULL == outbuf->buf) {
        kfree(outbuf);
        return NULL;
    }

    return outbuf;
}

#endif /* WLAN_AOW_ENABLED */


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

#ifdef WLAN_AOW_ENABLED
    spinlock_t                      aowctrl_lock;
    struct usb_request              *aowctrl_out_req, *aowctrl_in_req;
    struct usb_ep                   *aowctrl_out_ep, *aowctrl_in_ep;
    struct usb_endpoint_descriptor  *aowctrl_out_desc, *aowctrl_in_desc;
    struct list_head                aowctrl_queue;
    struct list_head                aowctrl_free_queue;
    
    struct work_struct              aowctrl_work;

#endif
};

static inline struct f_audio *func_to_audio(struct usb_function *f)
{
	return container_of(f, struct f_audio, card.func);
}


#ifdef USB_24BIT_AUDIO_ENABLED 
u16 dst_for24bit[MAX_AUDIO_CHAN][AUDIO_FRAME_SIZE_24BIT/sizeof(u16)];
#endif /* USB_24BIT_AUDIO_ENABLED */
static u32 dst[MAX_AUDIO_CHAN][AUDIO_FRAME_SIZE/sizeof(u32)];

/* Note: 24-bit audio not tested with BUFDUMP_ENABLED_WQ*/
//#define BUFDUMP_ENABLED_WQ 1
//__attribute_used__ noinline static int audio_playback(struct audio_dev *audio, void *buf, int count)
static int audio_playback(struct f_audio *audio, void *buf, int count, u_int64_t time_stamp)
{
    int i, offset;
    int altSetting;
    int num_chan_pairs = 0;
    u32 *src;
#ifdef USB_24BIT_AUDIO_ENABLED
    u16 *src_for24bit;
#endif /* USB_24BIT_AUDIO_ENABLED */
    int reqd_audio_buf_size = 0;
    int reqd_audio_frame_size = 0;
    int cnt, loop_count, extra;

#ifdef WLAN_AOW_ENABLED

#ifdef WLAN_AOW_TXSCHED_ENABLED
    int send_data_called = 0;
#endif

    /* return if the wlan calls are not registered */
    if (!is_aow_wlan_calls_registered())
        return 0;
#endif  /* WLAN_AOW_ENABLED */

    altSetting = (int)(audio->altSetting);
    
#ifdef USB_24BIT_AUDIO_ENABLED
    if((altSetting > 8) || (altSetting < 1)) {
#else
    if((altSetting >4) || (altSetting < 1)) {
#endif /* USB_24BIT_AUDIO_ENABLED */
        return 0;
    }

#ifdef USB_24BIT_AUDIO_ENABLED
    if (altSetting > 4) {
        src_for24bit = buf;
        num_chan_pairs = altSetting - 4;
        reqd_audio_buf_size = g_audio_buf_size_24bit;
        reqd_audio_frame_size = g_audio_frame_size_24bit; 
    } else {
#endif /* USB_24BIT_AUDIO_ENABLED */
        src = buf;
        num_chan_pairs = altSetting; 
        reqd_audio_buf_size = g_audio_buf_size;
        reqd_audio_frame_size = g_audio_frame_size; 
#ifdef USB_24BIT_AUDIO_ENABLED
    }
#endif /* USB_24BIT_AUDIO_ENABLED */

    if(count < reqd_audio_buf_size) {
	    cnt = count / (reqd_audio_frame_size * num_chan_pairs);
    	extra = count % reqd_audio_frame_size;
    } else
    {
    	cnt = BUF_SIZE_FACTOR / num_chan_pairs;
	    extra = 0;
    }
    count_audio_playback++;

#ifdef USB_24BIT_AUDIO_ENABLED
    if (altSetting > 4) {
        
        while (cnt--) {
            for (offset = 0; offset < (reqd_audio_frame_size / 2); offset += 3) {
                for (i = 0; i < num_chan_pairs; i++) {
                    dst_for24bit[i][offset] = *src_for24bit++;
                    dst_for24bit[i][offset + 1] = *src_for24bit++;
                    dst_for24bit[i][offset + 2] = *src_for24bit++;
                }
            }

            #ifdef WLAN_AOW_ENABLED
            //usb_aow_dev.tx.get_tsf(&tsf);
            for (i = 0; i < num_chan_pairs; i++) {
               usb_aow_dev.tx.send_data((char *)&(dst_for24bit[i][0]), reqd_audio_frame_size, i, time_stamp);
            #ifdef WLAN_AOW_TXSCHED_ENABLED
               send_data_called = 1;
            #endif
            } 
            #endif
        }

        if(extra != 0) {
            cnt = extra/num_chan_pairs;
            loop_count = cnt / 2;

            for (offset = 0; offset < loop_count; offset += 3) {
                for (i = 0; i < num_chan_pairs; i++) {
                    dst_for24bit[i][offset] = *src_for24bit++;
                    dst_for24bit[i][offset + 1] = *src_for24bit++;
                    dst_for24bit[i][offset + 2] = *src_for24bit++;
                }
            }
        
            #ifdef WLAN_AOW_ENABLED
            for (i = 0; i < num_chan_pairs; i++) {
                usb_aow_dev.tx.send_data((char *)&(dst_for24bit[i][0]), cnt, i, time_stamp);
	        #ifdef WLAN_AOW_TXSCHED_ENABLED
            	send_data_called = 1;
	        #endif
            }
            #endif
        }
	
	#ifdef WLAN_AOW_ENABLED
	#ifdef WLAN_AOW_TXSCHED_ENABLED
    	if (send_data_called) {
        	usb_aow_dev.tx.dispatch_data();
    	}
	#endif
	#endif

    } else {
#endif /* USB_24BIT_AUDIO_ENABLED */
        
        /* The below code will not be under an else when USB_24BIT_AUDIO_ENABLED
           is disabled. */

        while (cnt--) {
            for (offset = 0; offset < (reqd_audio_frame_size / 4); offset++) {
            for (i = 0; i < num_chan_pairs; i++)
                dst[i][offset] = *src++;
            }

#ifdef WLAN_AOW_ENABLED
            //usb_aow_dev.tx.get_tsf(&tsf);
            
            for (i = 0; i < num_chan_pairs; i++) {
               usb_aow_dev.tx.send_data((char *)&(dst[i][0]), reqd_audio_frame_size, i, time_stamp);
	        #ifdef WLAN_AOW_TXSCHED_ENABLED
               send_data_called = 1;
	        #endif
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
#if !defined(CONFIG_MACH_AR934x) && \
	!defined(CONFIG_MACH_QCA955x)	
            ar7242_i2s_write(reqd_audio_frame_size, (char *)&(dst[0][0]), 1);
#endif
#endif
        }
#ifdef I2S_ENABLED
#if defined(CONFIG_MACH_AR934x) || \
    defined(CONFIG_MACH_QCA955x)
	ath_ex_i2s_write(AUDIO_FRAME_SIZE, (char *)&(dst[0][0]), 1);
#else
	ar7242_i2s_write(AUDIO_FRAME_SIZE, (char *)&(dst[0][0]), 1);
#endif
#endif

        if(extra != 0) {
            cnt = extra/num_chan_pairs;
            loop_count = cnt/4;
            for (offset = 0; offset < loop_count; offset++) {
                for (i = 0; i < num_chan_pairs; i++) {
                    dst[i][offset] = *src++;
                }
            }

#ifdef WLAN_AOW_ENABLED
            for (i = 0; i < num_chan_pairs; i++) {
                usb_aow_dev.tx.send_data((char *)&(dst[i][0]), cnt, i, time_stamp);
            #ifdef WLAN_AOW_TXSCHED_ENABLED
                send_data_called = 1;
            #endif
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

#ifdef WLAN_AOW_ENABLED
        #ifdef WLAN_AOW_TXSCHED_ENABLED
        if (send_data_called) {
            usb_aow_dev.tx.dispatch_data();
        }
        #endif
#endif

#ifdef USB_24BIT_AUDIO_ENABLED
    }
#endif /* USB_24BIT_AUDIO_ENABLED */

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
  
    do {
        spin_lock_irq(&audio->lock);
        play_buf = list_first_entry(&(audio->play_queue), struct f_audio_buf, list);
        list_element_no--;
        list_del(&play_buf->list);
        spin_unlock_irq(&audio->lock);
    
        audio_playback(audio, play_buf->buf, play_buf->actual, play_buf->tsf);
#if 0
	u_audio_playback(&audio->card, play_buf->buf, play_buf->actual);
#endif
        f_audio_buffer_free(play_buf);
    } while (list_element_no);

	return;
}

#ifdef WLAN_AOW_ENABLED
static void aowctrl_work(struct work_struct *data)
{
    unsigned long long tsf = 0;
    struct aowctrl_buf *outbuf;
	struct f_audio *audio = container_of(data,
                                         struct f_audio,
                                         aowctrl_work);
    unsigned long irqflags;

    spin_lock_irqsave(&audio->aowctrl_lock, irqflags);
    if (list_empty(&audio->aowctrl_queue)) {
        spin_unlock_irqrestore(&audio->aowctrl_lock, irqflags);
        return;
    }
    
    outbuf = list_first_entry(&(audio->aowctrl_queue), struct aowctrl_buf, list);
    list_del(&outbuf->list);
    spin_unlock_irqrestore(&audio->aowctrl_lock, irqflags);
    
    //usb_aow_dev.tx.get_tsf(&tsf);
    usb_aow_dev.tx.send_ctrl((char *)outbuf->buf, outbuf->actual, tsf);

    spin_lock_irqsave(&audio->aowctrl_lock, irqflags);
    list_add_tail(&outbuf->list, &audio->aowctrl_free_queue);
    spin_unlock_irqrestore(&audio->aowctrl_lock, irqflags);

    return;
}

static void destroy_aowctrl_queues(struct f_audio *dev)
{
    struct aowctrl_buf *outbuf = NULL;
 
    spin_lock(&dev->aowctrl_lock);
	
    while (!list_empty(&dev->aowctrl_queue)) {
	    outbuf = list_first_entry(&(dev->aowctrl_queue), struct aowctrl_buf, list);
	    list_del(&outbuf->list);

        if (NULL == outbuf) {
            continue;
        }

        if (outbuf->buf) {
            kfree(outbuf->buf);
        }

        kfree(outbuf);
    }
	
    while (!list_empty(&dev->aowctrl_free_queue)) {
	    outbuf = list_first_entry(&(dev->aowctrl_free_queue), struct aowctrl_buf, list);
	    list_del(&outbuf->list);

        if (NULL == outbuf) {
            continue;
        }

        if (outbuf->buf) {
            kfree(outbuf->buf);
        }

        kfree(outbuf);
    }
    
    spin_unlock(&dev->aowctrl_lock);

    return;
}
#endif /* WLAN_AOW_ENABLED */

static int f_audio_out_ep_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_audio *audio = req->context;
	struct usb_composite_dev *cdev = audio->card.func.config->cdev;
	struct f_audio_buf *copy_buf = audio->copy_buf;
	int err;
    unsigned int reqd_audio_buf_size;
    int num_chan_pairs = 0;

	if (!copy_buf)
		return -EINVAL;
    
    /* WAR for audio problems when num_chan_pairs < 4
       We schedule playback_work once we have 
       (g_audio_frame_size * num_chan_pairs) bytes or
       (g_audio_frame_size_24bit * num_chan_pairs) bytes.
       Ideally, we should be changing the value of g_audio_buf_size
       to correspond to the current altSetting, so that space is 
       conserved. However, this will involve critical code changes
       which we don't want to jump into in one stroke.
       Besides, the most common use case is with num_chan_pairs == 4,
       so the space requirements with the current value of g_audio_buf_size
       are bound to hold in most cases, anyway */

#ifdef USB_24BIT_AUDIO_ENABLED
    if (audio->altSetting > 4) {
        num_chan_pairs = audio->altSetting - 4;
        reqd_audio_buf_size = g_audio_frame_size_24bit * num_chan_pairs; 
    } else {
#endif /* USB_24BIT_AUDIO_ENABLED */
        num_chan_pairs = audio->altSetting;
        reqd_audio_buf_size = g_audio_frame_size * num_chan_pairs; 
#ifdef USB_24BIT_AUDIO_ENABLED
    }
#endif /* USB_24BIT_AUDIO_ENABLED */


	/* Copy buffer is full, or has requisite no. of bytes,
       so add it to the play_queue */
     {
        u_int16_t cpyLength = reqd_audio_buf_size - copy_buf->actual;
        if (cpyLength > req->actual) {
            cpyLength = req->actual;
        }
        req->actual -= cpyLength;
        memcpy(copy_buf->buf + copy_buf->actual,
        req->buf,
        cpyLength);
        copy_buf->actual += cpyLength;
        if (copy_buf->actual == reqd_audio_buf_size) {
#ifdef WLAN_AOW_ENABLED
            usb_aow_dev.tx.get_tsf(&copy_buf->tsf);
#endif
            list_element_no++;
            list_add_tail(&copy_buf->list, &audio->play_queue);
            schedule_work(&audio->playback_work);
            copy_buf = f_audio_buffer_alloc(max_audio_buf_size); 
            if (copy_buf < 0) {
                audio->copy_buf = NULL;
                return -ENOMEM;
            }
        }
    }
    if (req->actual) {
        memcpy(copy_buf->buf, req->buf, req->actual);
        copy_buf->actual += req->actual;
    }
    audio->copy_buf = copy_buf;

	err = usb_ep_queue(ep, req, GFP_ATOMIC);
	if (err)
		ERROR(cdev, "%s queue req: %d\n", ep->name, err);

	return 0;

}

#ifdef WLAN_AOW_ENABLED
static int f_audio_aowctrl_out_ep_complete(struct usb_ep *ep, struct usb_request *req)
{
    struct f_audio *audio = req->context;
    struct usb_composite_dev *cdev = audio->card.func.config->cdev;
    int err;
    struct aowctrl_buf *outbuf = NULL;
	
    if (req->status != 0) {
        ERROR(cdev, "%s queue req status: %d\n", ep->name, req->status);
        /* But we don't give up. */
        goto requeue;
    }
    
	if (list_empty(&audio->aowctrl_free_queue)) {
        ERROR(cdev, "%s queue - Out of buffers for Control packets\n", ep->name);
        /* But we don't give up. */
        goto requeue;
    }
	
    /* Remove from free queue */	
    outbuf = list_first_entry(&(audio->aowctrl_free_queue), struct aowctrl_buf, list);
    list_del(&outbuf->list);
        
    if (NULL == outbuf) {
        ERROR(cdev, "%s queue - Could not get valid buffer for Control packet\n", ep->name);
        /* But we don't give up. */
        goto requeue;
    } 
    
    memcpy(outbuf->buf, req->buf, req->actual);
    outbuf->actual = req->actual;
    list_add_tail(&outbuf->list, &audio->aowctrl_queue);
    schedule_work(&audio->aowctrl_work);

requeue:
    err = usb_ep_queue(ep, req, GFP_ATOMIC);
    
    if (err) {
        ERROR(cdev, "%s queueing error: %d\n", ep->name, err);
        return err;
    }

    return 0;
}

static int f_audio_aowctrl_in_ep_complete(struct usb_ep *ep, struct usb_request *req)
{
    struct f_audio *audio = req->context;
    struct usb_composite_dev *cdev = audio->card.func.config->cdev;
    
    if (req->status < 0) {
        ERROR(cdev, "%s queue req: %d\n", ep->name, req->status);
        return req->status;
    }

    return 0;
}
#endif


static void f_audio_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_audio *audio = req->context;
	int status = req->status;
	u32 data = 0;
	struct usb_ep *out_ep = audio->out_ep;
#ifdef WLAN_AOW_ENABLED
    struct usb_ep *aowctrl_out_ep = audio->aowctrl_out_ep;
    struct usb_ep *aowctrl_in_ep = audio->aowctrl_in_ep;
#endif

	switch (status) {

	case 0:				/* normal completion? */
		if (ep == out_ep) {
            spin_lock(&audio->lock);
			f_audio_out_ep_complete(ep, req);
            spin_unlock(&audio->lock);
        }
#ifdef WLAN_AOW_ENABLED
        else if (ep == aowctrl_out_ep) {
            spin_lock (&audio->aowctrl_lock);
            f_audio_aowctrl_out_ep_complete(ep, req);
            spin_unlock (&audio->aowctrl_lock);
        }
        else if (ep == aowctrl_in_ep) {
            spin_lock (&audio->aowctrl_lock);
            f_audio_aowctrl_in_ep_complete(ep, req);
            spin_unlock (&audio->aowctrl_lock);
        }
#endif
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

#ifdef WLAN_AOW_ENABLED
static void free_aowctrl_ep_reqs(struct f_audio *audio)
{
    if (audio->aowctrl_out_req) {
        if (audio->aowctrl_out_req->buf) {
            kfree(audio->aowctrl_out_req->buf);
            audio->aowctrl_out_req->buf = NULL;
        }
        
        usb_ep_dequeue(audio->aowctrl_out_ep, audio->aowctrl_out_req);
        usb_ep_free_request(audio->aowctrl_out_ep, audio->aowctrl_out_req);
        audio->aowctrl_out_req = NULL;
    }

    if (audio->aowctrl_in_req) {
        if (audio->aowctrl_in_req->buf) {
            kfree(audio->aowctrl_in_req->buf);
            audio->aowctrl_in_req->buf = NULL;
        }
        
        usb_ep_dequeue(audio->aowctrl_in_ep, audio->aowctrl_in_req);
        usb_ep_free_request (audio->aowctrl_in_ep, audio->aowctrl_in_req);
        audio->aowctrl_in_req = NULL;
    }
}

static int set_wlan_audioparams(u8 bBitResolution, u8 tSamFreq[1][3])
{
    audio_type_t audiotype;

    /* Return if the wlan calls are not registered. */
    if (!is_aow_wlan_calls_registered()) {
        return 0;
    }

    /* TODO: As and when new sample sizes and rates are added, modify
       the below checks. */

    if ((0x80 == tSamFreq[0][0]) && (0xBB == tSamFreq[0][1])) {
        /* 48 KHz */
        
        switch (bBitResolution)
        {
            case 16:
                audiotype = SAMP_RATE_48k_SAMP_SIZE_16;
                break;

            case 24:
                audiotype = SAMP_RATE_48k_SAMP_SIZE_24;
                break;

            default:
                printk("%s: Unsupported sample size. bBitResolution=%u.\n",
                       __func__,
                       bBitResolution);
                return -EINVAL;
        }
    } else {
        printk("%s: Unsupported sampling rate. "
               "tSamFreq[0][0]=0x%x, tSamFreq[0][1]=0x%x\n",
               __func__,
               tSamFreq[0][0],
               tSamFreq[0][1]);
        return -EINVAL;
    }

    usb_aow_dev.tx.set_audioparams(audiotype);

    return 0;
}
#endif /* WLAN_AOW_ENABLED */ 



static int f_audio_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_audio		*audio = func_to_audio(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_ep *out_ep = audio->out_ep;
	struct usb_request *req;
    int selected_req_buf_size = 0;
    int num_chan_pairs = 0;
	int i = 0, err = 0;

#ifdef WLAN_AOW_ENABLED
    struct usb_ep *aowctrl_out_ep = audio->aowctrl_out_ep;
    struct usb_ep *aowctrl_in_ep = audio->aowctrl_in_ep;

    struct usb_request *aowctrl_out_req = NULL;
    struct usb_request *aowctrl_in_req = NULL;

    u8 bBitResolution = 0;
    u8 tSamFreq[1][3] = {
                            [0][0] = 0,
                            [0][1] = 0,
                        };           
#endif

	DBG(cdev, "intf %d, alt %d\n", intf, alt);
	ERROR(cdev, "intf %d, alt %d\n", intf, alt);

	if (intf == 1) {
        spin_lock(&audio->lock);
		
        if (alt != 0) {
                if(!(audio->copy_buf)) {
                    audio->copy_buf = f_audio_buffer_alloc(max_audio_buf_size);
                }
	            
                if (audio->copy_buf == -ENOMEM) {
                    audio->copy_buf = NULL;
                    spin_unlock(&audio->lock);
                    return -ENOMEM;
                }
#ifdef WLAN_AOW_ENABLED
                printk("altsetting requested=%d alsetting expected=%d\n",
                       alt,
                       g_expected_alt_setting);

                if((1 != audio->urb_created) && (alt == g_expected_alt_setting)) { 
#else
		    	if((1 != audio->urb_created) && (audio->altSetting != alt)) {
#endif
			    usb_ep_disable(out_ep);
			    free_out_ep_reqs(audio);
			    audio->altSetting = (u8)alt;
			    audio->curAltSetting = (u8)alt;
        	            switch(alt) {
                	    case 1:
                    		audio->out_desc = &as_out_ep_alt_1_desc;
#ifdef WLAN_AOW_ENABLED
                            bBitResolution = as_type_i_1_desc.bBitResolution; 
                            tSamFreq[0][0] = as_type_i_1_desc.tSamFreq[0][0]; 
                            tSamFreq[0][1] = as_type_i_1_desc.tSamFreq[0][1]; 
#endif /* WLAN_AOW_ENABLED */
                    		break;
	                    case 2:
                    		audio->out_desc = &as_out_ep_alt_2_desc;
#ifdef WLAN_AOW_ENABLED
                            bBitResolution = as_type_i_2_desc.bBitResolution; 
                            tSamFreq[0][0] = as_type_i_2_desc.tSamFreq[0][0]; 
                            tSamFreq[0][1] = as_type_i_2_desc.tSamFreq[0][1]; 
#endif /* WLAN_AOW_ENABLED */
                    		break;
                	    case 3:
                    		audio->out_desc = &as_out_ep_alt_3_desc;
#ifdef WLAN_AOW_ENABLED
                            bBitResolution = as_type_i_3_desc.bBitResolution; 
                            tSamFreq[0][0] = as_type_i_3_desc.tSamFreq[0][0]; 
                            tSamFreq[0][1] = as_type_i_3_desc.tSamFreq[0][1]; 
#endif /* WLAN_AOW_ENABLED */
                    		break;
                	    case 4:
                    		audio->out_desc = &as_out_ep_alt_4_desc;
#ifdef WLAN_AOW_ENABLED
                            bBitResolution = as_type_i_4_desc.bBitResolution; 
                            tSamFreq[0][0] = as_type_i_4_desc.tSamFreq[0][0]; 
                            tSamFreq[0][1] = as_type_i_4_desc.tSamFreq[0][1]; 
#endif /* WLAN_AOW_ENABLED */
                    	 	break;
#ifdef USB_24BIT_AUDIO_ENABLED
                	     case 5:
                    		audio->out_desc = &as_out_ep_alt_5_desc;
#ifdef WLAN_AOW_ENABLED
                            bBitResolution = as_type_i_5_desc.bBitResolution; 
                            tSamFreq[0][0] = as_type_i_5_desc.tSamFreq[0][0]; 
                            tSamFreq[0][1] = as_type_i_5_desc.tSamFreq[0][1]; 
#endif /* WLAN_AOW_ENABLED */
                    		break;
	                    case 6:
                    		audio->out_desc = &as_out_ep_alt_6_desc;
#ifdef WLAN_AOW_ENABLED
                            bBitResolution = as_type_i_6_desc.bBitResolution; 
                            tSamFreq[0][0] = as_type_i_6_desc.tSamFreq[0][0]; 
                            tSamFreq[0][1] = as_type_i_6_desc.tSamFreq[0][1]; 
#endif /* WLAN_AOW_ENABLED */
                    		break;
                	    case 7:
                    		audio->out_desc = &as_out_ep_alt_7_desc;
#ifdef WLAN_AOW_ENABLED
                            bBitResolution = as_type_i_7_desc.bBitResolution; 
                            tSamFreq[0][0] = as_type_i_7_desc.tSamFreq[0][0]; 
                            tSamFreq[0][1] = as_type_i_7_desc.tSamFreq[0][1]; 
#endif /* WLAN_AOW_ENABLED */
                    		break;
                	    case 8:
                    		audio->out_desc = &as_out_ep_alt_8_desc;
#ifdef WLAN_AOW_ENABLED
                            bBitResolution = as_type_i_8_desc.bBitResolution; 
                            tSamFreq[0][0] = as_type_i_8_desc.tSamFreq[0][0]; 
                            tSamFreq[0][1] = as_type_i_8_desc.tSamFreq[0][1]; 
#endif /* WLAN_AOW_ENABLED */
                    	 	break;
#endif /* USB_24BIT_AUDIO_ENABLED */
                	    default:
                    		ERROR (cdev, "Invalid Alternate Setting: intf %d, alt %d\n", intf, alt);
                	    }

			    usb_ep_enable(out_ep, audio->out_desc);
			    out_ep->driver_data = audio;
#ifdef WLAN_AOW_ENABLED
                set_wlan_audioparams(bBitResolution, tSamFreq);
#endif
         	    
#ifdef USB_24BIT_AUDIO_ENABLED
                if (alt > 4) {
                    selected_req_buf_size = req_buf_size_24bit;
                    num_chan_pairs = alt - 4;
                } else {
#endif /* USB_24BIT_AUDIO_ENABLED */
                    selected_req_buf_size = req_buf_size;
                    num_chan_pairs = alt;
#ifdef USB_24BIT_AUDIO_ENABLED
                }
#endif /* USB_24BIT_AUDIO_ENABLED */

			    /*
			    * allocate a bunch of read buffers
			    * and queue them all at once.
			    */
			    for (i = 0; i < req_count && err == 0; i++) {
				req = usb_ep_alloc_request(out_ep, GFP_ATOMIC);
				if (req) {
					req->buf = kzalloc(selected_req_buf_size * num_chan_pairs,
							GFP_ATOMIC);
					if (req->buf) {
						req->length = selected_req_buf_size * num_chan_pairs;
						req->context = audio;
						req->complete =
							f_audio_complete;
						err = usb_ep_queue(out_ep,
							req, GFP_ATOMIC);
						if (err)
							ERROR(cdev,
							"%s queue req: %d\n",
							out_ep->name, err);
					} else
						err = -ENOMEM;
				} else
					err = -ENOMEM;
				        list_add_tail(&req->list, &audio->req_queue);
			    }
			    audio->urb_created = 1;
		      }
		} else {
			struct f_audio_buf *copy_buf = audio->copy_buf;
		    	audio->curAltSetting = alt;
			if (copy_buf) {
                		if (copy_buf->actual != 0) {
		    			audio->copy_buf = NULL;
			        list_element_no++;
					list_add_tail(&copy_buf->list,
						&audio->play_queue);
					schedule_work(&audio->playback_work);
				}
			}
		}

        spin_unlock(&audio->lock);
	}
#ifdef WLAN_AOW_ENABLED
    else if (intf == 2) {
        spin_lock(&audio->aowctrl_lock);
        
        /* We have only altsetting 0 */
        if (alt != 0) {
            ERROR (cdev, "Invalid Alternate Setting: intf %d, alt %d\n", intf, alt);
            err = -EINVAL;
            spin_unlock(&audio->aowctrl_lock);
            goto done;
        }

        free_aowctrl_ep_reqs(audio);
       
        /* Initialization for OUT transfers */

        audio->aowctrl_out_desc = &vs_aowctrl_out_ep_desc;
        
        usb_ep_enable(aowctrl_out_ep, audio->aowctrl_out_desc);
        
        aowctrl_out_ep->driver_data = audio;

        aowctrl_out_req = usb_ep_alloc_request(aowctrl_out_ep, GFP_ATOMIC);
        if (aowctrl_out_req) {
                aowctrl_out_req->buf = kzalloc(AOWCTRL_PROT_BYTECOUNT, GFP_ATOMIC);
                
                if (aowctrl_out_req->buf) {
                    aowctrl_out_req->length = AOWCTRL_PROT_BYTECOUNT;
                    aowctrl_out_req->context = audio;
                    aowctrl_out_req->complete = f_audio_complete;
                    
                    err = usb_ep_queue(aowctrl_out_ep,
                                       aowctrl_out_req,
                                       GFP_ATOMIC);
                    if (err) {
                        ERROR(cdev,
                              "%s queue req: %d\n",
                              aowctrl_out_ep->name,
                              err);
                        spin_unlock(&audio->aowctrl_lock);
                        goto done;
                    }
                } else {
                    err = -ENOMEM;
                    spin_unlock(&audio->aowctrl_lock);
                    goto done;
                }
         } else {
                 err = -ENOMEM;
                 spin_unlock(&audio->aowctrl_lock);
                 goto done;
         }

         audio->aowctrl_out_req = aowctrl_out_req;

         
         /* Initialization for IN transfers */
         
         audio->aowctrl_in_desc = &vs_aowctrl_in_ep_desc;
        
         usb_ep_enable(aowctrl_in_ep, audio->aowctrl_in_desc);

         aowctrl_in_ep->driver_data = audio;

         aowctrl_in_req = usb_ep_alloc_request(aowctrl_in_ep, GFP_ATOMIC);
         if (aowctrl_in_req) {
                aowctrl_in_req->buf = kzalloc(AOWCTRL_PROT_BYTECOUNT, GFP_ATOMIC);
                
                if (aowctrl_in_req->buf) {
                    aowctrl_in_req->length = AOWCTRL_PROT_BYTECOUNT;
                    aowctrl_in_req->context = audio;
                    aowctrl_in_req->complete = f_audio_complete;
                } else {
                    err = -ENOMEM;
                    spin_unlock(&audio->aowctrl_lock);
                    goto done;
                }
         } else {
                 err = -ENOMEM;
                 spin_unlock(&audio->aowctrl_lock);
                 goto done;
         }

         audio->aowctrl_in_req = aowctrl_in_req;
    }

    g_alt_setting_init_done = 1;

#endif /* WLAN_AOW_ENABLED */

done:
	//ERROR(cdev, "TP4: intf %d, alt %d Err = %d\n", intf, alt, err);

	return err;
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
	//struct gaudio *card = &audio->card;
	//u8 *sam_freq;
	//int rate;

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
#ifdef WLAN_AOW_ENABLED
    struct usb_ep       *aowctrl_out_ep = NULL, *aowctrl_in_ep = NULL;
#endif


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

#ifdef WLAN_AOW_ENABLED
    status = usb_interface_id(c, f);
    if (status < 0)
        goto fail;
    vs_aowctrl_intf.bInterfaceNumber = status;

    status = -ENODEV;
 
    aowctrl_out_ep = usb_ep_autoconfig(cdev->gadget, &vs_aowctrl_out_ep_desc);

    if (!aowctrl_out_ep) {
        goto fail;
    }

    audio->aowctrl_out_ep = aowctrl_out_ep;
    aowctrl_out_ep->driver_data = cdev;    /* claim */
    
    aowctrl_in_ep = usb_ep_autoconfig(cdev->gadget, &vs_aowctrl_in_ep_desc);

    if (!aowctrl_in_ep)
        goto fail;
    
    audio->aowctrl_in_ep = aowctrl_in_ep;
    aowctrl_in_ep->driver_data = cdev;    /* claim */

    usbinfo.dev = audio;
#endif


	status = -ENOMEM;

	/* supcard all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */

	/* copy descriptors, and track endpoint copies */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		c->highspeed = true;
		f->hs_descriptors = usb_copy_descriptors(f_audio_desc);
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
	usb_free_descriptors(f->descriptors);
	kfree(audio);

#ifdef WLAN_AOW_ENABLED
    free_aowctrl_ep_reqs(audio);
    //TODO: Check on the below
    //cancel_delayed_work(&audio->aowctrl_work);
    flush_scheduled_work();
    destroy_aowctrl_queues(audio);
#endif

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
    len += sprintf(buf+len,"\nAlternate Setting = %li Current Alt Setting = %li\n", \
			audio->altSetting, audio->curAltSetting);
    return len;
}
#define BUFDUMP_ENABLED_WQ 1
static int gaudio_readdata_procmem(char *buf, char **start, off_t offset,
                                int count, int *eof, void *data)
{
    int i, len = 0;
    char *src;

    src = &(dst[0][0]);
    #ifdef BUFDUMP_ENABLED_WQ
    for(i=0; i < count; i++) {
        len += sprintf(buf+len,"%x", src[i]);
    }
    #endif
    return len;
}

#ifdef WLAN_AOW_ENABLED

/*-----------------------------------------------------------------------------
 *  The following APIs are exposed for WLAN driver as a part of AoW feature
 *-----------------------------------------------------------------------------*/

/**
 * @brief      Initialize the USB AoW Device
 */
static void init_usb_aow_dev(void)
{
    usb_aow_dev.tx.set_audioparams = NULL;
    usb_aow_dev.tx.send_data = NULL;
#ifdef WLAN_AOW_TXSCHED_ENABLED
    usb_aow_dev.tx.dispatch_data = NULL;
#endif
    usb_aow_dev.tx.send_ctrl = NULL;
    usb_aow_dev.tx.get_tsf   = NULL;
}


/**
 * @brief      Checks whether WLAN APIs are registered with USB or not.
 * 
 * @return     1 if WLAN APIs are registered, 0 if not.
 */
static int is_aow_wlan_calls_registered(void)
{
   int is_registered = 0; 

   if ((usb_aow_dev.tx.set_audioparams != NULL) &&
       (usb_aow_dev.tx.send_data != NULL) && 
#ifdef WLAN_AOW_TXSCHED_ENABLED
       (usb_aow_dev.tx.dispatch_data != NULL) &&
#endif
       (usb_aow_dev.tx.send_ctrl != NULL) &&
       (usb_aow_dev.tx.get_tsf != NULL)) {
        is_registered = 1;
   }

    return is_registered;
}    

/**
 * @brief      Accept a control packet from WLAN and transmit it to host.
 * 
 * @param[in]  data  The packet data.
 * @param[in]  len   Length of thLength of the data.
 * 
 * @return     0 on success, negative value on error.
 */
int usb_aow_rx_ctrl(char* data, int len)
{
    int err;
    struct f_audio *audio = usbinfo.dev;
	struct usb_composite_dev *cdev = audio->card.func.config->cdev;

    if (!g_alt_setting_init_done || !(audio->aowctrl_in_req)) {
        return -EINVAL;
    }

    audio->aowctrl_in_req->length = len;
    memcpy(audio->aowctrl_in_req->buf, data, len);


    if (-EINPROGRESS == audio->aowctrl_in_req->status) {
        ERROR(cdev, "Cannot accept AoW Control IN request since one is already queued\n");
        return -EINPROGRESS;
    }

    err = usb_ep_queue(audio->aowctrl_in_ep, audio->aowctrl_in_req, GFP_ATOMIC);
    if (err) {
        ERROR(cdev, "%s queue req: %d\n", audio->aowctrl_in_ep->name, err);
        return err;
    }

    return 0;
}

/**
 * @brief                 Set frame size to be used.
 * 
 * @param[in]  framesize  Frame size multiplier.
 *                        Valid values: 1 (2 ms frames)
 *                                    : 2 (4 ms frames)
 *                                    : 3 (6 ms frames)
 *                                    : 4 (8 ms frames)
 * @return     0 on success, negative value on error.
 */
int usb_aow_set_frame_size(unsigned int framesize)
{
    if ((framesize < MIN_FRAME_SIZE_FACTOR) || 
        (framesize > MAX_FRAME_SIZE_FACTOR)) {
        return -EINVAL;
    }

    g_audio_frame_size = (AUDIO_FRAME_SIZE/MAX_FRAME_SIZE_FACTOR) * framesize;
    g_audio_buf_size = g_audio_frame_size * BUF_SIZE_FACTOR;
#ifdef USB_24BIT_AUDIO_ENABLED 
    g_audio_frame_size_24bit = (AUDIO_FRAME_SIZE_24BIT/MAX_FRAME_SIZE_FACTOR) * framesize;
    g_audio_buf_size_24bit = g_audio_frame_size_24bit * BUF_SIZE_FACTOR;
#endif

    return 0;
}

/**
 * @brief    Set alt setting to expect.
 *
 * This is a temporary requirement to work around some USB issues
 * related to tear-down and setup of requests on varying alt settings.
 * 
 * @param[in]  altsetting  Alt setting to expect. Valid values: 1-8
 *
 * @return     0 on success, negative value on failure.
 */

int usb_aow_set_exptd_alt_setting(unsigned int altsetting)
{
    if ((altsetting < MIN_EXPECTED_ALT_SETTING) || 
        (altsetting > MAX_EXPECTED_ALT_SETTING)) {
        return -EINVAL;
    }

    g_expected_alt_setting = altsetting;

    return 0;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  aow_register_wlan_calls_to_usb
 *  Description:  Register WLAN Transmit related calls, USB device will use them to
 *                send data to WLAN device. WLAN will call this device on bootup
 * =====================================================================================
 */

int aow_register_wlan_calls_to_usb(void* set_audioparams,
                                   void* tx_data,
#ifdef WLAN_AOW_TXSCHED_ENABLED
                                   void* dispatch_data,
#endif        
                                   void* tx_ctrl,
                                   void* get_tsf)
{
    usb_aow_dev.tx.set_audioparams = set_audioparams;
    usb_aow_dev.tx.send_data = tx_data;
#ifdef WLAN_AOW_TXSCHED_ENABLED
    usb_aow_dev.tx.dispatch_data = dispatch_data;
#endif        
    usb_aow_dev.tx.send_ctrl = tx_ctrl;
    usb_aow_dev.tx.get_tsf = get_tsf;

    return 0;

}EXPORT_SYMBOL(aow_register_wlan_calls_to_usb);    
#endif



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
#ifdef WLAN_AOW_ENABLED
    struct aowctrl_buf 	*outbuf = NULL;
    int j;
    
    init_usb_aow_dev();
    wlan_aow_register_calls_to_usb(&usb_aow_dev);
    aow_register_usb_calls_to_wlan(NULL,
                                   usb_aow_rx_ctrl,
                                   usb_aow_set_frame_size,
                                   usb_aow_set_exptd_alt_setting);
#endif  /* WLAN_AOW_ENABLED */


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
#ifdef WLAN_AOW_ENABLED
    INIT_LIST_HEAD(&audio->aowctrl_queue);
    INIT_LIST_HEAD(&audio->aowctrl_free_queue);

    for(j = 0; j < AOWCTRL_QUEUE_LEN; j++)
    {
        outbuf = aowctrl_buffer_alloc(AOWCTRL_PROT_BYTECOUNT);

        if (NULL == outbuf) {
            return -ENOMEM;
        }

        list_add_tail(&outbuf->list, &audio->aowctrl_free_queue);
    }
#endif

	spin_lock_init(&audio->lock);
#ifdef WLAN_AOW_ENABLED
    spin_lock_init (&audio->aowctrl_lock);
#endif

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
	audio->card.func.setup = f_audio_setup;
	audio->card.func.disable = f_audio_disable;
	audio->out_desc = &as_out_ep_alt_1_desc;
#ifdef WLAN_AOW_ENABLED
    audio->aowctrl_out_desc = &vs_aowctrl_out_ep_desc;
    audio->aowctrl_in_desc  = &vs_aowctrl_in_ep_desc;
#endif

	control_selector_init(audio);

	INIT_WORK(&audio->playback_work, f_audio_playback_work);
#ifdef WLAN_AOW_ENABLED
    INIT_WORK(&audio->aowctrl_work, aowctrl_work);
#endif

	status = usb_add_function(c, &audio->card.func);
	if (status)
		goto add_fail;

	INFO(c->cdev, "max_audio_buf_size %d, req_buf_size %d, req_count %d\n",
		max_audio_buf_size, req_buf_size, req_count);

	return status;

add_fail:
	gaudio_cleanup(&audio->card);
setup_fail:
	kfree(audio);
	return status;
}
