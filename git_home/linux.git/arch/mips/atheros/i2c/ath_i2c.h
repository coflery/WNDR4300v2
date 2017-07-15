/* * Copyright (c) 2014 Qualcomm Atheros, Inc.
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

#ifndef __ATH_I2C_H
#define __ATH_I2C_H

#define ATH_I2C_REG_BASE                    (0xB8018000)

/* IOCTL Commands */
#define I2C_MAGIC        121
#define I2C_SET_CLK      _IOW(I2C_MAGIC, 1, int)
#define I2C_SET_ADDRMODE _IOW(I2C_MAGIC, 2, int)

/* I2C Register Offsets */
#define ATH_I2C_I2CON                       (ATH_I2C_REG_BASE + 0x00)       
#define ATH_I2C_TAR                         (ATH_I2C_REG_BASE + 0x04)       
#define ATH_I2C_SAR                         (ATH_I2C_REG_BASE + 0x08)       
#define ATH_I2C_HS_MADDR                    (ATH_I2C_REG_BASE + 0x0C)
#define ATH_I2C_DATA_CMD                    (ATH_I2C_REG_BASE + 0x10)
#define ATH_I2C_SS_SCL_HCNT                 (ATH_I2C_REG_BASE + 0x14)
#define ATH_I2C_SS_SCL_LCNT                 (ATH_I2C_REG_BASE + 0x18)
#define ATH_I2C_FS_SCL_HCNT                 (ATH_I2C_REG_BASE + 0x1C)
#define ATH_I2C_FS_SCL_LCNT                 (ATH_I2C_REG_BASE + 0x20)
#define ATH_I2C_HS_SCL_HCNT                 (ATH_I2C_REG_BASE + 0x24)
#define ATH_I2C_HS_SCL_LCNT                 (ATH_I2C_REG_BASE + 0x28)
#define ATH_I2C_RAW_INTR_STAT               (ATH_I2C_REG_BASE + 0x2C)
#define ATH_I2C_INTR_MASK                   (ATH_I2C_REG_BASE + 0x30)
#define ATH_I2C_INTR_STAT                   (ATH_I2C_REG_BASE + 0x34)
#define ATH_I2C_RX_TL                       (ATH_I2C_REG_BASE + 0x38)
#define ATH_I2C_TX_TL                       (ATH_I2C_REG_BASE + 0x3c)
#define ATH_I2C_CLR_INTR                    (ATH_I2C_REG_BASE + 0x40)
#define ATH_I2C_CLR_RX_UNDER                (ATH_I2C_REG_BASE + 0x44)
#define ATH_I2C_CLR_RX_OVER                 (ATH_I2C_REG_BASE + 0x48)
#define ATH_I2C_CLR_TX_OVER                 (ATH_I2C_REG_BASE + 0x4c)
#define ATH_I2C_CLR_RD_REQ                  (ATH_I2C_REG_BASE + 0x50)
#define ATH_I2C_CLR_TX_ABRT                 (ATH_I2C_REG_BASE + 0x54)
#define ATH_I2C_CLR_RX_DONE                 (ATH_I2C_REG_BASE + 0x58)
#define ATH_I2C_CLR_ACTIVITY                (ATH_I2C_REG_BASE + 0x5C)
#define ATH_I2C_CLR_STOP_DET                (ATH_I2C_REG_BASE + 0x60)
#define ATH_I2C_CLR_START_DET               (ATH_I2C_REG_BASE + 0x64)
#define ATH_I2C_CLR_GEN_CALL                (ATH_I2C_REG_BASE + 0x68)
#define ATH_I2C_ENABLE                      (ATH_I2C_REG_BASE + 0x6C)
#define ATH_I2C_STATUS                      (ATH_I2C_REG_BASE + 0x70)
#define ATH_I2C_TXFLR                       (ATH_I2C_REG_BASE + 0x74)
#define ATH_I2C_RXFLR                       (ATH_I2C_REG_BASE + 0x78)
#define ATH_I2C_SRESET                      (ATH_I2C_REG_BASE + 0x7C)
#define ATH_I2C_TX_ABRT_SOURCE              (ATH_I2C_REG_BASE + 0x80)


/* I2C control register bit definitions */
#define ATH_I2C_CON_SLAVE_DISABLE               (0x1 << 6)
#define ATH_I2C_CON_RESTART_EN                  (0x1 << 5)
#define ATH_I2C_CON_10BITADDR_MASTER            (0x1 << 4)
#define ATH_I2C_CON_10BITADDR_SLAVE             (0x1 << 3)
#define ATH_I2C_CON_SPEED_HS                    (0x3 << 1)
#define ATH_I2C_CON_SPEED_FS                    (0x2 << 1)
#define ATH_I2C_CON_SPEED_SS                    (0x1 << 1)
#define ATH_I2C_CON_SPEED_MASK                  (0x3 << 1)
#define ATH_I2C_CON_MASTER_MODE                 (0x1 << 0)

/* Register Values */
#define I2C_ENABLE                              (1 << 0)
#define I2C_DISABLE                             (0 << 0)
#define I2C_SRESET                              (1 << 0)
#define I2C_BASE_SLAVE_ADDR                     0x20
#define I2C_10BIT_BASE_ADDR                     0x180 
/* Set addrmode IOCTL values */
#define ATH_I2C_7BIT_ADDR_MAS                   0
#define ATH_I2C_10BIT_ADDR_MAS                  1
#define ATH_I2C_7BIT_ADDR_SLA                   2
#define ATH_I2C_10BIT_ADDR_SLA                  3
/* Clock related macros. To be used to I2C_SET_CLK ioctl */
#define IC_HS_MODE_100MHZ                       1
#define IC_FS_MODE_100MHZ                       2
#define IC_SS_MODE_100MHZ                       3
#define IC_HS_MODE_40MHZ                        4
#define IC_FS_MODE_40MHZ                        5
#define IC_SS_MODE_40MHZ                        6

/*I2C Status Register*/
#define ATH_I2C_STAT_RFF                        (0x1 << 4)
#define ATH_I2C_STAT_RFNE                       (0x1 << 3)
#define ATH_I2C_STAT_TFE                        (0x1 << 2)
#define ATH_I2C_STAT_TFNF                       (0x1 << 1)
#define ATH_I2C_STAT_ACTIVITY                   (0x1 << 0)

/* Intr mask bits */
#define ATH_I2C_INTR_MASK_RXFULL                (0x1 << 2)
#define ATH_I2C_INTR_MASK_RDREQ                 (0x1 << 5)
#define ATH_I2C_INTR_MASK_RXDONE                (0x1 << 7)
#define ATH_I2C_INTR_MASK_START                 (0x1 << 10)
#define ATH_I2C_INTR_MASK_STOP                  (0x1 << 9)
#define ATH_I2C_INTR_MASK_ACTIVITY              (0x1 << 8)
#define ATH_I2C_INTR_MASK_TXABRT                (0x1 << 6)
#define ATH_I2C_INTR_MASK_TXEMPTY               (0x1 << 4)
#define ATH_I2C_INTR_MASK_GENCALL               (0x1 << 11)
#define ATH_I2C_INTR_MASK_RXUNDER               (0x1 << 0)
#define ATH_I2C_INTR_MASK_RXOVER                (0x1 << 1)
#define ATH_I2C_INTR_MASK_TXOVER                (0x1 << 3)
/* Clock Cnt values */
#define ATH_I2C_SS_SCL_HCNT_40                  0xA0
#define ATH_I2C_SS_SCL_LCNT_40                  0xBC
#define ATH_I2C_SS_SCL_HCNT_100                 0x190
#define ATH_I2C_SS_SCL_LCNT_100                 0x1D6
#define ATH_I2C_FS_SCL_HCNT_40                  0x18
#define ATH_I2C_FS_SCL_LCNT_40                  0x34
#define ATH_I2C_FS_SCL_HCNT_100                 0x3C
#define ATH_I2C_FS_SCL_LCNT_100                 0x82
#define ATH_I2C_HS_SCL_HCNT_40                  0x5
#define ATH_I2C_HS_SCL_LCNT_40                  0xD
#define ATH_I2C_HS_SCL_HCNT_100                 0xC
#define ATH_I2C_HS_SCL_LCNT_100                 0x20

/* Max data */
#define MAX_ADDRESS_OFFSET_BYTES                2
#define MAX_LEN_BYTES                           1
#define MAX_READ_COUNT                          10
#define MAX_LEN                                 12

/* RX/TX Data Buffer and Command Register*/
#define ATH_I2C_CMD_READ                        (0x1 << 8)
#define ATH_I2C_DAT_MASK                        0xff

/* I2C Error values */
#define _I2C_ERR_PARAM                          0x00000001
#define _I2C_ERR_BUSY                           0x00000002
#define _I2C_ERR_IOCTL                          0x00000004
#define _I2C_ERR_ABORT                          0x00000008

/* TIMEOUT */
#define ATH_I2C_TIMEOUT                         20 /* ms */
#define ATH_I2C_DEF_CON (ATH_I2C_CON_MASTER_MODE | ATH_I2C_CON_SPEED_SS | ATH_I2C_CON_RESTART_EN)

/* Tx ABRT sources */
#define TX_ABRT_7B_ADDR_NOACK   0
#define TX_ABRT_10ADDR1_NOACK   1
#define TX_ABRT_10ADDR2_NOACK   2
#define TX_ABRT_TXDATA_NOACK    3
#define TX_ABRT_GCALL_NOACK     4
#define TX_ABRT_GCALL_READ      5
#define TX_ABRT_HS_ACKDET       6
#define TX_ABRT_SBYTE_ACKDET    7
#define TX_ABRT_HS_NORSTRT      8
#define TX_ABRT_SBYTE_NORSTRT   9
#define TX_ABRT_10B_RD_NORSTRT  10
#define TX_ARB_MASTER_DIS       11
#define TX_ARB_LOST             12
#define TX_ABRT_SLVFLUSH_TXFIFO 13
#define TX_ABRT_SLV_ARBLOST     14
#define TX_ABRT_SLVRD_INTX      15

#define I2C_WRITE               1
#define I2C_READ                2

#define TX_FIFO_DEPTH           16
#define RX_FIFO_DEPTH           64

/* I2C structure to hold variables */
typedef struct ath_i2c_dev {
	char                    *name;
	int                     irq;
	unsigned long           data_count;
	unsigned short          reg_addr;
	unsigned int            rd_req_count;
	unsigned char           rx_data[MAX_LEN];
	unsigned short          tx_abort_source;
	unsigned short          master;
	unsigned short          tx_err;
	unsigned short          first;
	unsigned short          trans_type;
	unsigned char           *tx_buffer;
	unsigned char           *rx_buffer;
	unsigned char           *tx_off_buf;
	unsigned char           *rx_off_buf;
	unsigned long           data_len;
	unsigned long           off_len;
	unsigned long           tx_len, rx_len;
	unsigned short          slave;
	struct completion       cmd_complete;
	struct tasklet_struct   xfer_more;
	struct mutex            i2c_lock;
}ath_i2c_dev_t;


static ath_i2c_dev_t i2c_dev;

/* Structure to hold I2C params for Write/Read */
typedef struct ath_i2c_rw_t {
	unsigned short  slave;                                                          /* Slave-address */
	unsigned long   offset_size;                                                    /* Cmd size */
	unsigned char   *offset;                                                        /* Cmd */
	unsigned long   data_size;                                                      /* Data size */
	unsigned char   *data;                                                          /* Data */
} i2c_rw_t;

/* Function prototypes */
void ath_i2c_init_reg(void);
static int ath_i2c_bus_busy(void);
void do_i2c_read(void);
int ath_i2c_xfer(void);

#endif /* __ATH_I2C_H */

