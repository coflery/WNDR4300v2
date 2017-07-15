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
 */

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/completion.h>
#include <asm/mach-atheros/atheros.h>
#include "ath_i2c.h"

static int ath_i2c_major = CONFIG_ATH_I2C_MAJOR;
int ath_i2c_10bit_slave = 0;
int ath_i2c_10bit_master = 0;
int ath_i2c_slave_index = 0;

module_param(ath_i2c_10bit_slave, int, S_IRUGO);
module_param(ath_i2c_10bit_master, int, S_IRUGO);
module_param(ath_i2c_slave_index, int, S_IRUGO);

MODULE_AUTHOR("Charanya@Atheros");
MODULE_LICENSE("Dual BSD/GPL");

/* Tx abort source strings */
static char *tx_abort_source_str[] = {
    [TX_ABRT_7B_ADDR_NOACK]    =
        "7 bit slave address not acknowledged",
    [TX_ABRT_10ADDR1_NOACK]    =
        "10 bit slave first address byte not acknowledged",
    [TX_ABRT_10ADDR2_NOACK]    =
        "10 bit slave second address byte not acknowledged",
    [TX_ABRT_TXDATA_NOACK]     =
        "Transmitted data is not acknowledged",
    [TX_ABRT_GCALL_NOACK]      =
        "General call is not acknowledged",
    [TX_ABRT_GCALL_READ]       =
        "Read command issued after general call",
	[TX_ABRT_HS_ACKDET]        =
		"Master in HS mode and HS master code acknowledged",
	[TX_ABRT_SBYTE_ACKDET]     =
        "Masters start byte is acknowledged",
	[TX_ABRT_HS_NORSTRT]       =
		"Master sending data in HS mode with restart disabled",
    [TX_ABRT_SBYTE_NORSTRT]    =
        "User trying to send start byte when restart is disabled",
    [TX_ABRT_10B_RD_NORSTRT]   =
        "Master trying to read in 10 bit mode when restart is disabled",
    [TX_ARB_MASTER_DIS]        =
        "User attempted to use disabled master",
    [TX_ARB_LOST]              =
        "Master has lost arbitration",
	[TX_ABRT_SLVFLUSH_TXFIFO]  =
		"Slave received read when data exisits in tx fifo",
	[TX_ABRT_SLV_ARBLOST]      =
		"Slave lost bus while transmitting",
	[TX_ABRT_SLVRD_INTX]       =
		"Slave requesting data to Tx and user wrote a read command"
};

static int ath_i2c_open(struct inode *inode, struct file *filp)
{
	/* Do nothing */
	return 0;
}

/* Issue a soft reset to reset the state machine */
static void ath_i2c_soft_reset( void ) 
{
	ath_reg_wr(ATH_I2C_SRESET, I2C_SRESET);
	while(ath_reg_rd(ATH_I2C_SRESET) & I2C_SRESET);
}

/* Enable I2C. Few registers are to be
   written only with I2C disabled */
static void ath_i2c_enable(void) 
{
	ath_reg_wr(ATH_I2C_ENABLE, I2C_ENABLE);
}

/* Disable I2C */
static void ath_i2c_disable(void) 
{
	ath_reg_wr(ATH_I2C_ENABLE, I2C_DISABLE);
}

/* Function to set clk configuration. This sets the HIGH and LOW
 * periods of the clock for Standard, Fast or High speed mode */
void ath_i2c_set_clk(unsigned int data)
{
	unsigned int rd_data = 0;

	/* Disable I2C controller to initialize registers */
	ath_i2c_disable();
	switch(data) {
		case IC_SS_MODE_100MHZ:
			/* Set I2C_CLK_SEL bit in Switch clock spare for 100 MHZ */
			rd_data = ath_reg_rd(SWITCH_CLOCK_SPARE_ADDRESS);
			rd_data = rd_data & ~(SWITCH_CLOCK_SPARE_I2C_CLK_SEL_MASK);
			ath_reg_wr(SWITCH_CLOCK_SPARE_ADDRESS, ((rd_data | SWITCH_CLOCK_SPARE_I2C_CLK_SEL_SET(1))));
			rd_data = ath_reg_rd(ATH_I2C_I2CON);
			rd_data = rd_data & ~(ATH_I2C_CON_SPEED_MASK);
			ath_reg_wr(ATH_I2C_I2CON, (rd_data | ATH_I2C_CON_SPEED_SS));

			ath_reg_wr((ATH_I2C_SS_SCL_HCNT), ATH_I2C_SS_SCL_HCNT_100);
			ath_reg_wr((ATH_I2C_SS_SCL_LCNT), ATH_I2C_SS_SCL_LCNT_100);
			break;
		case IC_SS_MODE_40MHZ :
			/* Clear I2C_CLK_SEL bit in Switch clock spare for 40 MHZ */
			rd_data = ath_reg_rd(SWITCH_CLOCK_SPARE_ADDRESS);
			rd_data = rd_data & ~(SWITCH_CLOCK_SPARE_I2C_CLK_SEL_MASK);
			ath_reg_wr(SWITCH_CLOCK_SPARE_ADDRESS, ((rd_data | SWITCH_CLOCK_SPARE_I2C_CLK_SEL_SET(0))));
			rd_data = ath_reg_rd(ATH_I2C_I2CON);

			rd_data = rd_data & ~(ATH_I2C_CON_SPEED_MASK);
			ath_reg_wr(ATH_I2C_I2CON, (rd_data | ATH_I2C_CON_SPEED_SS));
	
			ath_reg_wr((ATH_I2C_SS_SCL_HCNT), ATH_I2C_SS_SCL_HCNT_40);
			ath_reg_wr((ATH_I2C_SS_SCL_LCNT), ATH_I2C_SS_SCL_LCNT_40);
			break;
		case IC_FS_MODE_100MHZ :
			/* Set I2C_CLK_SEL bit in Switch clock spare for 100 MHZ */
			rd_data = ath_reg_rd(SWITCH_CLOCK_SPARE_ADDRESS);
			rd_data = rd_data & ~(SWITCH_CLOCK_SPARE_I2C_CLK_SEL_MASK);
			ath_reg_wr(SWITCH_CLOCK_SPARE_ADDRESS, ((rd_data | SWITCH_CLOCK_SPARE_I2C_CLK_SEL_SET(1))));
			rd_data = ath_reg_rd(ATH_I2C_I2CON);
			rd_data = rd_data & ~(ATH_I2C_CON_SPEED_MASK);
			ath_reg_wr(ATH_I2C_I2CON, (rd_data | ATH_I2C_CON_SPEED_FS));

			ath_reg_wr((ATH_I2C_FS_SCL_HCNT), ATH_I2C_FS_SCL_HCNT_100);
			ath_reg_wr((ATH_I2C_FS_SCL_LCNT), ATH_I2C_FS_SCL_LCNT_100);
			break;
		case IC_FS_MODE_40MHZ :
			/* Clear I2C_CLK_SEL bit in Switch clock spare for 40 MHZ */
			rd_data = ath_reg_rd(SWITCH_CLOCK_SPARE_ADDRESS);
			rd_data = rd_data & ~(SWITCH_CLOCK_SPARE_I2C_CLK_SEL_MASK);
			ath_reg_wr(SWITCH_CLOCK_SPARE_ADDRESS, ((rd_data | SWITCH_CLOCK_SPARE_I2C_CLK_SEL_SET(0))));
			rd_data = ath_reg_rd(ATH_I2C_I2CON);
			rd_data = rd_data & ~(ATH_I2C_CON_SPEED_MASK);
			ath_reg_wr(ATH_I2C_I2CON, (rd_data | ATH_I2C_CON_SPEED_FS));

			ath_reg_wr((ATH_I2C_FS_SCL_HCNT), ATH_I2C_FS_SCL_HCNT_40);
			ath_reg_wr((ATH_I2C_FS_SCL_LCNT), ATH_I2C_FS_SCL_LCNT_40);
			break;
		case IC_HS_MODE_100MHZ :
			/* Set I2C_CLK_SEL bit in Switch clock spare for 100 MHZ */
			rd_data = ath_reg_rd(SWITCH_CLOCK_SPARE_ADDRESS);
			rd_data = rd_data & ~(SWITCH_CLOCK_SPARE_I2C_CLK_SEL_MASK);
			ath_reg_wr(SWITCH_CLOCK_SPARE_ADDRESS, ((rd_data | SWITCH_CLOCK_SPARE_I2C_CLK_SEL_SET(1))));

			rd_data = ath_reg_rd(ATH_I2C_I2CON);
			rd_data = rd_data & ~(ATH_I2C_CON_SPEED_MASK);
			ath_reg_wr(ATH_I2C_I2CON, (rd_data | ATH_I2C_CON_SPEED_HS));

			ath_reg_wr((ATH_I2C_HS_SCL_HCNT), ATH_I2C_HS_SCL_HCNT_100);
			ath_reg_wr((ATH_I2C_HS_SCL_LCNT), ATH_I2C_HS_SCL_LCNT_100);
			break;
		case IC_HS_MODE_40MHZ :
			/* Clear I2C_CLK_SEL bit in Switch clock spare for 100 MHZ */
			rd_data = ath_reg_rd(SWITCH_CLOCK_SPARE_ADDRESS);
			rd_data = rd_data & ~(SWITCH_CLOCK_SPARE_I2C_CLK_SEL_MASK);
			ath_reg_wr(SWITCH_CLOCK_SPARE_ADDRESS, ((rd_data | SWITCH_CLOCK_SPARE_I2C_CLK_SEL_SET(0))));
			rd_data = ath_reg_rd(ATH_I2C_I2CON);
			rd_data = rd_data & ~(ATH_I2C_CON_SPEED_MASK);
			ath_reg_wr(ATH_I2C_I2CON, (rd_data | ATH_I2C_CON_SPEED_HS));

			ath_reg_wr((ATH_I2C_HS_SCL_HCNT), ATH_I2C_HS_SCL_HCNT_40);
			ath_reg_wr((ATH_I2C_HS_SCL_LCNT), ATH_I2C_HS_SCL_LCNT_40);
			break;
	}
	ath_i2c_enable();
}	

/* Function to set 7/10 bit addressing mode
 * as a master */
static int ath_i2c_set_addrmode(unsigned int addr_mode)
{	
	int ret = 0;
    /* Addr mode can be set only if bus is not busy */
	/* Make sure that the ioctl is not called during
	   the middle of a transaction. It should be called
       only during init */
	ret = ath_i2c_bus_busy();
    if ( ret < 0 ) {
        return ret;
    }
	ath_i2c_disable();
	/* 7 bit master/10 bit master */
    if (addr_mode == ATH_I2C_7BIT_ADDR_MAS) {
        ath_reg_rmw_clear(ATH_I2C_I2CON, ATH_I2C_CON_10BITADDR_MASTER);
	}
	else if (addr_mode == ATH_I2C_10BIT_ADDR_MAS) {
		ath_reg_rmw_set(ATH_I2C_I2CON, ATH_I2C_CON_10BITADDR_MASTER);
    }
	/* Ideally these are not used since the slave
     * operates with interrupts. Hence a user interaction with
     * ioctl is not required */
	else if (addr_mode == ATH_I2C_7BIT_ADDR_SLA) {
		ath_reg_rmw_clear(ATH_I2C_I2CON, ATH_I2C_CON_10BITADDR_SLAVE);
	}
	else if (addr_mode == ATH_I2C_10BIT_ADDR_SLA) {
		 ath_reg_rmw_set(ATH_I2C_I2CON, ATH_I2C_CON_10BITADDR_SLAVE);
    }
	ath_i2c_enable();

	return 0;
}

/* Function to change I2C parameters from user space.
 * To be used only before initiating any transaction since
 * the I2C controller would be disabled and then renabled
 * to write certain registers */
static int ath_i2c_ioctl(struct inode *inode, struct file *filp,
			 unsigned int cmd, unsigned long arg)
{
	unsigned int data;
	int ret;
	switch(cmd) {
	case I2C_SET_CLK:
		data = arg;
		ath_i2c_set_clk(data);	
		break;
	case I2C_SET_ADDRMODE:
		data = arg;
		ret = ath_i2c_set_addrmode(data);
		if (ret < 0)
			return -_I2C_ERR_IOCTL;
		break;
	default:
		return -ENOTSUPP;
	}
	return 0;
}
	
/* Tasklet callback to transfer bytes more than the
 * FIFO Depth and to read the data for the corresponding
 * number of READ commands transferred 
*/
static void ath_i2c_xfer_more(unsigned long data)
{
    ath_i2c_dev_t *dev = (ath_i2c_dev_t *)data;

	if (dev == NULL)
		return;

    if (dev->trans_type == I2C_READ) {
        do_i2c_read();
	}
	/* Call low level xfer function
	 * to transfer more data bytes or more READ 
	 * commands */	
    ath_i2c_xfer();
    if (dev->tx_len != 0)
        ath_reg_wr(ATH_I2C_INTR_MASK, (ATH_I2C_INTR_MASK_STOP |
                                       ATH_I2C_INTR_MASK_TXABRT |
                                       ATH_I2C_INTR_MASK_TXEMPTY));
    else
        ath_reg_wr(ATH_I2C_INTR_MASK, (ATH_I2C_INTR_MASK_STOP |
                                       ATH_I2C_INTR_MASK_TXABRT));
}

/* Interrupt handler */
irqreturn_t ath_i2c_intr(void)
{
	ath_i2c_dev_t *dev = &i2c_dev;
	unsigned char rd_data_from_reg;
	unsigned char i;
	unsigned int int_status;

	int_status = ath_reg_rd(ATH_I2C_RAW_INTR_STAT);	

	ath_reg_wr(ATH_I2C_INTR_MASK, 0);
	if (dev->master == 1) {
		/* Tx ABRT: Occurs due to one of the many reasons mentioned in
    	 * Tx ABRT Source register. Set tx_err when such an error occurs */
		if ((int_status & ATH_I2C_INTR_MASK_TXABRT) == ATH_I2C_INTR_MASK_TXABRT) {
			dev->tx_abort_source = ath_reg_rd(ATH_I2C_TX_ABRT_SOURCE);
			dev->tx_err = _I2C_ERR_ABORT;
			ath_reg_rd(ATH_I2C_CLR_TX_ABRT);
		} else if ((int_status & ATH_I2C_INTR_MASK_TXEMPTY) == ATH_I2C_INTR_MASK_TXEMPTY) {
			/* Burst transaction more than the FIFO DEPTH */
			tasklet_schedule(&dev->xfer_more);
		}
	}

	/* RxFULL: Occurs when there are entries more than the RX_TL. Used only
     * in slave mode to frame the address offset and to identify a Write/Read
     * transaction. A write transaction involves more than 2 RxFULL interrupts.
     * Read transaction involves 2 RxFull interrupts for address offsets
     * followed by Read request interrupt */

	/* The number of address offset bytes and the register space accessible
     * for read by another master is fixed for now. Scorpion I2C slave
     * requires that the address offset is of 2 bytes and can read only
     * the SLIC register space */

	if ((int_status & ATH_I2C_INTR_MASK_RXFULL) == ATH_I2C_INTR_MASK_RXFULL) {	
		/* Disable Rx Full */
		ath_reg_rmw_clear(ATH_I2C_INTR_MASK, ATH_I2C_INTR_MASK_RXFULL);
			/* Read data cmd register and get the data */
		dev->rx_data[dev->data_count] = ath_reg_rd(ATH_I2C_DATA_CMD) & ATH_I2C_DAT_MASK;
		dev->data_count++;
		ath_reg_rmw_set(ATH_I2C_INTR_MASK, ATH_I2C_INTR_MASK_RXFULL);	
	} 	

	/* RD REQ: Received whenever a master wants to read 
     * data from a slave. This should occur after receiving 2 bytes
     * of address offset */
	else if ((int_status & ATH_I2C_INTR_MASK_RDREQ) == ATH_I2C_INTR_MASK_RDREQ) {
		  /* Clear red request interrupt */
           ath_reg_rd(ATH_I2C_CLR_RD_REQ);
		/* A RD REQ was received before receiving the offset bytes */
		if (dev->data_count < MAX_ADDRESS_OFFSET_BYTES) {
			printk("No proper offset received \n");
		}
	   dev->reg_addr = ((dev->rx_data[0]) << 8 | (dev->rx_data[1]));
		/* SLIC register space is made accessible for read from a master.
         * Proper care needs to be taken not to modify any critical SLIC 
           register data */ 

	    /* Read data continuously from the offset till a STOP is received. Holds good
		 * for both single/burst transaction */
		for ( i = 0; i < MAX_READ_COUNT; i++) {
			rd_data_from_reg = ath_reg_rd((ATH_SLIC_BASE + (dev->reg_addr) + (dev->rd_req_count)));
			/* Offset address increment for a burst transaction */
			dev->rd_req_count = (dev->rd_req_count  + 4);
			ath_reg_wr(ATH_I2C_DATA_CMD, ((rd_data_from_reg) & 0xff));
		}
	}

	/* STOP: Indicates that a transaction has been completed. For 
	 * Slave Rx mode, the received bytes are written to the address offset
     * For master mode, specifies that a transaction is complete by setting the 
     * complete flag */	
	else if ((int_status & ATH_I2C_INTR_MASK_STOP) == ATH_I2C_INTR_MASK_STOP) {
		/* Slave Rx. More than 2  bytes of received data indicates a write
		 * transaction. 2 bytes addrress offset + data bytes */
		if (dev->data_count > 	(MAX_ADDRESS_OFFSET_BYTES)) {
			dev->reg_addr = ((dev->rx_data[0]) << 8 | (dev->rx_data[1]));
			for ( i = MAX_ADDRESS_OFFSET_BYTES; i < (dev->data_count); i++) {
			/* Write the data stored in the array during RX FULL interrupt
			* to SLIC registers */
				ath_reg_wr((ATH_SLIC_BASE + (dev->reg_addr)), dev->rx_data[i]); 	
				dev->reg_addr = (dev->reg_addr + 4);
			}
		}
		dev->reg_addr = 0;
		dev->data_count = 0;
		dev->rd_req_count = 0; 
		ath_reg_rd(ATH_I2C_CLR_STOP_DET);	
	}
	
	if (dev->master == 1) {
		 /* Set completion */
	    if ((int_status) &  (ATH_I2C_INTR_MASK_TXABRT | ATH_I2C_INTR_MASK_STOP)) {
			ath_reg_wr(ATH_I2C_INTR_MASK, 0);
            complete(&dev->cmd_complete);
		}
	}

	else {
		/* Enable Slave mode interrupts */
		ath_reg_wr(ATH_I2C_INTR_MASK, (ATH_I2C_INTR_MASK_STOP | 
									 	ATH_I2C_INTR_MASK_RXFULL | 
										ATH_I2C_INTR_MASK_RDREQ));

	}
	return IRQ_HANDLED;
}

/* Bus busy check. Masters should ensure that the
 * bus is checked for busy status before initiating
 * a transaction */
static int ath_i2c_bus_busy(void)
{
	int timeout = ATH_I2C_TIMEOUT;
	while ((ath_reg_rd(ATH_I2C_STATUS)) & ATH_I2C_STAT_ACTIVITY) {
		if (timeout <= 0) {
			printk("Timeout waiting for bus ready \n");
			return -_I2C_ERR_BUSY;
		}
		timeout--;
		mdelay(1);
	}
	return 0;
}

/* Read function to read the data present in the 
 * Rx FIFO. Supports burst read, that is read data len
 * greater than the Rx FIFO Depth */
void do_i2c_read(void)	
{
	int rx_threshold;
	ath_i2c_dev_t *dev = &i2c_dev;
	unsigned long rx_buf_len;
	unsigned char *rd_buf;	

	rx_threshold = ath_reg_rd(ATH_I2C_RXFLR);
	rx_buf_len = dev->rx_len;
	rd_buf = dev->rx_buffer;	

	/* Read the DATA_CMD register to get the data in the Rx FIFO */
	for(; rx_buf_len > 0 && rx_threshold > 0; rx_buf_len--, rx_threshold--)
		*rd_buf++ = ath_reg_rd(ATH_I2C_DATA_CMD);

	dev->rx_len = rx_buf_len;

	if (rx_buf_len > 0) {
		dev->rx_buffer = rd_buf;
	}
}

/* Low level xfer function to transmit required
 * bytes/commands. This function is required for both
 * write/read transaction since a READ transaction also
 * involves writing of offset bytes and READ commands.
 * The first gets called from the write function as well as 
 * from the tasklet to transmit data more than the FIFO DEPTH */
int ath_i2c_xfer(void)
{
    ath_i2c_dev_t *dev = &i2c_dev;
    int ret;
    int tx_threshold, tx_cur_entries;
    int rx_threshold, rx_cur_entries;
	unsigned long tx_buf_len;
	int offset_write = 0;

    tx_cur_entries = ath_reg_rd(ATH_I2C_TXFLR);
    tx_threshold = TX_FIFO_DEPTH - tx_cur_entries;

    rx_cur_entries = ath_reg_rd(ATH_I2C_RXFLR);
    rx_threshold = RX_FIFO_DEPTH - rx_cur_entries;

    if (dev->first == 1) {
    /* Check if the I2C bus is busy before initiating
     * any transaction. The SDA and SCL would be high
     * before the start of a transaction. The master
     * can then pull the SDA low to aquire the bus */
        ret = ath_i2c_bus_busy();
            if ( ret < 0 ) {
                goto do_xfer_done;
             }
 		 /* Disable I2C */
        ath_i2c_disable();

        /* Follow master read sequence as per protocol. Send I2C slave
         * address first, write the offset_size offset bytes,
         * initiate a read sequence by writing READ command for data_size
         * times and read data_size bytes from the buffer.

        * This is covered under Master Transmit and Master Receive
           section in the data sheet */

        /* Write Slave addr */
        ath_reg_wr(ATH_I2C_TAR, dev->slave);

        ath_i2c_enable();

    }

	/* Handle burst transactions of more than the FIFO DEPTH using the 
	 * TX EMPTY interrupt. When FIFO_DEPTH bytes are transmitted, wait
     * for TX EMPTY and then keep transmitting further bytes with the
	 * xfer_more tasklet */
    tx_buf_len = dev->tx_len;
    while(tx_buf_len > 0 && tx_threshold > 0 && rx_threshold > 0) {
        if (dev->trans_type == I2C_WRITE) {
            if ((dev->tx_off_buf == NULL) || (dev->tx_buffer == NULL))  {
                ret = _I2C_ERR_PARAM;
                goto do_xfer_done;
            }
            if (dev->first == 1) {
           	 /* The first burst transaction involves writing the
                offset bytes before the transmit data */
				ath_reg_wr((ATH_I2C_DATA_CMD),  *(dev->tx_off_buf++));
				offset_write++;
			}
			else {
	            ath_reg_wr(ATH_I2C_DATA_CMD, *(dev->tx_buffer++));
			}

        } else if (dev->trans_type == I2C_READ) {
            if ((dev->rx_off_buf == NULL) || (dev->rx_buffer == NULL))  {
                ret = _I2C_ERR_PARAM;
                goto do_xfer_done;
            }
            if (dev->first == 1) {
			/* The first burst transaction involves writing the
			   offset bytes before the READ command */
               	ath_reg_wr((ATH_I2C_DATA_CMD),  *(dev->rx_off_buf++));
				offset_write++;
            }
			else {
	            ath_reg_wr(ATH_I2C_DATA_CMD, ATH_I2C_CMD_READ);	
	            rx_threshold--;
			}
	}	
	tx_threshold--;
	tx_buf_len--;
	if (offset_write == dev->off_len)	
		dev->first = 0;
    }
    /* Enable Required interrupts */
    ath_reg_wr(ATH_I2C_INTR_MASK, (ATH_I2C_INTR_MASK_STOP |
                                   ATH_I2C_INTR_MASK_TXABRT));

	/* More data to be transmitted */
    if (tx_buf_len > 0) {
        ath_reg_rmw_set(ATH_I2C_INTR_MASK, ATH_I2C_INTR_MASK_TXEMPTY);
	}
	dev->tx_len = tx_buf_len;
    ret = dev->tx_len;
do_xfer_done:
    return ret;

}

/* Copy required paramters from the user, check for errors
 * and call the low level bus specific xfer function.
 * Called through the write() system call from the application */
ssize_t ath_i2c_write(struct file * filp, const char __user * buf,
                         size_t count, loff_t * f_pos)
{ 
    ath_i2c_dev_t *dev = &i2c_dev;	
	i2c_rw_t wrparam;
	int ret = 0, wr_bytes;	
	char *offset_ptr;
	char *data_ptr;

	mutex_lock(&dev->i2c_lock);
	INIT_COMPLETION(dev->cmd_complete);

	/* Rx FULL interrupt required only for slave mode operation */
	ath_reg_rmw_clear(ATH_I2C_INTR_MASK, ATH_I2C_INTR_MASK_RXFULL);

	/* Device is a master and should initiate transaction
     * with a slave device */
	dev->master = 1;
	/* The first burst */ 
	dev->first = 1;
	/* Transaction type: Write Transaction */
	dev->trans_type = I2C_WRITE;
	/* Initialize tx err to No Error */
	dev->tx_err = 0;

	/* Copy write parameters from user. The params contain the 
	   target slave addr, number of bytes of offset, the offset pointer ,
	   number of bytes of data and the data pointer */
	if (copy_from_user(&wrparam, (i2c_rw_t *)buf, sizeof(i2c_rw_t))) {
		ret = -EINVAL;
		return ret;
	} else {
		/* Allocate needed memories to hold the offsets and data */
		offset_ptr = kmalloc(wrparam.offset_size, GFP_KERNEL);
		if (offset_ptr == NULL) {
			ret = -ENOMEM;
			return ret;
		}
		/* Cleanly handle failures in copy to make sure there are
		   no allocated dangling buffers */
		if( copy_from_user(offset_ptr,  wrparam.offset, wrparam.offset_size) ) {
			ret = -EINVAL;
			kfree(offset_ptr);
			return ret;
		}
		wrparam.offset = offset_ptr;
		data_ptr = kmalloc(wrparam.data_size, GFP_KERNEL);
		if (data_ptr == NULL) {
			/* Free offset ptr */
			kfree(offset_ptr);
			ret = -ENOMEM;
			return ret;
		}	
		if( copy_from_user(data_ptr,  wrparam.data, wrparam.data_size) ) {
			ret = -EINVAL;
			kfree(offset_ptr);
			kfree(data_ptr);
			return ret;
		}
		wrparam.data = data_ptr;

		/* Make a copy of the write parameters in the
		 * dev structure */
		dev->slave = wrparam.slave;
		dev->tx_buffer = wrparam.data;
        dev->tx_off_buf = wrparam.offset;
        dev->data_len = wrparam.data_size;
        dev->off_len = wrparam.offset_size;
		/* The total tx len includes  the offset len and
		 * the data len */
		dev->tx_len = (dev->data_len + dev->off_len); 

		/* Call the low level xfer function to transmit both	
		 * the offset bytes and the data bytes */
		ret = ath_i2c_xfer();
		if (ret < 0) {
			goto wr_done;
		}
		else {
			/* Save return value from xfer function */
			wr_bytes = ret;
		}
	    /* Wait for tx to complete. Completion happens when a STOP or TX ABRT
    	 * is received on the bus */
	    ret = wait_for_completion_interruptible_timeout(&dev->cmd_complete, HZ);
    	if (ret == 0) {
        	printk("controller timed out\n");
	        ath_i2c_init_reg();
    	    ret = -ETIMEDOUT;
        	goto wr_done;
   		 }

	    /* A faulty completion due to TX ABRT. Return the reason
    	 * for Tx ABRT */
	    if (dev->tx_err == _I2C_ERR_ABORT) {
    	    int i = 0;
        	unsigned long source = dev->tx_abort_source;
	        for_each_bit(i, &source, ARRAY_SIZE(tx_abort_source_str)) {
            	printk(" %s %s\n", __func__, tx_abort_source_str[i]);
	        }
    	    ret = -EIO;
        	goto wr_done;
	    }
	}
	/* A clean return. Number of bytes remaining to be written */
	ret = wr_bytes; 
wr_done:
    /* Enable RX FULL interrupt since it is required 
     * for slave mode operation */
	ath_reg_rmw_set(ATH_I2C_INTR_MASK, ATH_I2C_INTR_MASK_RXFULL);
	dev->master = 0;
	dev->first = 0;
	dev->trans_type = 0;
	mutex_unlock(&dev->i2c_lock);
	kfree(offset_ptr);
	kfree(data_ptr);
	return ret;
}


/* Function to copy the paramters from user space
 * and call the low level bus specific transfer function 
 */
ssize_t ath_i2c_read(struct file * filp, char __user * buf,
                         size_t count, loff_t * f_pos)
{
	ath_i2c_dev_t *dev = &i2c_dev;
	i2c_rw_t rdparam;
	int ret = 0;
	char *offset_ptr;
	char *param_base;

	/* Device is a master since the read is
     * triggered from user space */
	dev->master = 1;
	/* The first burst */
	dev->first = 1;
	/* Transaction type: A read transaction */
	dev->trans_type = I2C_READ;
	/* Rx Full interrupt is required only for slave mode
     * operation */
	ath_reg_rmw_clear(ATH_I2C_INTR_MASK, ATH_I2C_INTR_MASK_RXFULL);
	/* Initialize tx err to no errros */
	dev->tx_err = 0;

	mutex_lock(&dev->i2c_lock);
	INIT_COMPLETION(dev->cmd_complete);

	/* Copy Parameters from user. The params contain the
       target slave addr, number of bytes of offset, the offset pointer ,
       number of bytes of data and the data pointer where read data 
	   is to be stored */
	if ( copy_from_user(&rdparam, (i2c_rw_t *)buf, sizeof(i2c_rw_t)) ) {
		ret = -EINVAL;
		return ret;
	} else {
		/* Allocate required memories */
		offset_ptr = kmalloc(rdparam.offset_size, GFP_KERNEL);
		if (offset_ptr == NULL) {
			ret = -ENOMEM;
			return ret;
		}

		if( copy_from_user(offset_ptr,  rdparam.offset, rdparam.offset_size) ) {
			ret = -EINVAL;
			kfree(offset_ptr);
			return ret;
		}
		rdparam.offset = offset_ptr;
		rdparam.data = kmalloc((rdparam.data_size) , GFP_KERNEL);
		if (rdparam.data == NULL) {
			/* Free offset ptr */
			kfree(offset_ptr);
			ret = -ENOMEM;
			return ret;
		}
		param_base = rdparam.data;

		/* Make a copy of the required params in the
  		 * dev structure */
		dev->slave = rdparam.slave;
		dev->rx_buffer = rdparam.data;
		dev->rx_off_buf = rdparam.offset;
		dev->data_len = rdparam.data_size;
		dev->off_len = rdparam.offset_size;
		/* Total Transmit len includes both the 
		 * offset len and the data len */
		dev->tx_len = (dev->data_len + dev->off_len);
		dev->rx_len = dev->data_len;

		/* Call xfer function to transmit the data 
		 * out. Typically this involves transmitting the 
		 * address offset and the READ commands to the slave
         */
		ret = ath_i2c_xfer();
		if (ret < 0) {
			goto rd_done;
		}

		/* Wait for tx to complete. Completion happens when a STOP or TX ABRT
         * is received on the bus */
	    ret = wait_for_completion_interruptible_timeout(&dev->cmd_complete, HZ);
    	if (ret == 0) {
        	printk("controller timed out\n");
	        ath_i2c_init_reg();
    	    ret = -ETIMEDOUT;
        	goto rd_done;
	    }

		/* Read the data in the Rx FIFO only if the transmission
		 * of offset bytes and READ commands are successful */
		if (dev->tx_err == 0) {
			do {
				do_i2c_read();
			}while(dev->rx_len > 0);	
		}	

    	/* A faulty completion due to TX ABRT. Return the reason
	     * for Tx ABRT */
    	if (dev->tx_err == _I2C_ERR_ABORT) {
        	int i = 0;
	        unsigned long source = dev->tx_abort_source;
    	    	for_each_bit(i, &source, ARRAY_SIZE(tx_abort_source_str)) {
        	    printk(" %s %s\n", __func__, tx_abort_source_str[i]);
        	}	
	        ret = -EIO;
    	    goto rd_done;
	    }
		
		/* Copt the read data to user space */
		if ( copy_to_user(((i2c_rw_t *)buf)->data, param_base, rdparam.data_size) ) {
        	ret  = -EINVAL;
            goto rd_done;
        }
	}

	/* Clean return. Return the number of bytes remaining to be
	 * read */
	ret = dev->rx_len;
rd_done:
	/* Enable RxFULL interrupt since it it required for
     * slave mode */
	ath_reg_rmw_set(ATH_I2C_INTR_MASK, ATH_I2C_INTR_MASK_RXFULL);
	dev->master = 0;
	dev->first = 0;
	dev->trans_type = 0;
	mutex_unlock(&dev->i2c_lock);
	kfree(offset_ptr);
	kfree(rdparam.data);
	return ret;
}

/* File operations structure */
struct file_operations  ath_i2c_fops = {
        .owner   = THIS_MODULE,
        .read    = ath_i2c_read,
        .write   = ath_i2c_write,
        .ioctl   = ath_i2c_ioctl,
        .open    = ath_i2c_open,
};

/* Function to initialize GPIOs and registers 
 * with required defaults */

void ath_i2c_init_reg(void)
{
	unsigned int rddata = 0;
	unsigned int base_slave_addr = 0;

    /* Initialize the GPIOs. GPIO 19 and 20 are used for SCL and SDA 
     * respectively. This is board specific */
	rddata = ath_reg_rd(ATH_GPIO_IN_ENABLE4) & ~(0x00ff0000);
	ath_reg_wr(ATH_GPIO_IN_ENABLE4, rddata | GPIO_IN_ENABLE4_I2C_CLK_SET(0x13));

	rddata = ath_reg_rd(ATH_GPIO_IN_ENABLE4) & ~(0xff000000);
	ath_reg_wr(ATH_GPIO_IN_ENABLE4, rddata | GPIO_IN_ENABLE4_I2C_DATA_SET(0x14));

	rddata = ((ath_reg_rd(ATH_GPIO_OUT_FUNCTION4)) & (~(GPIO_OUT_FUNCTION4_ENABLE_GPIO_19_MASK)));
	ath_reg_wr(ATH_GPIO_OUT_FUNCTION4, (rddata | GPIO_OUT_FUNCTION4_ENABLE_GPIO_19_SET(0x7)));

	rddata = ((ath_reg_rd(ATH_GPIO_OUT_FUNCTION5)) & (~(GPIO_OUT_FUNCTION5_ENABLE_GPIO_20_MASK)));
	ath_reg_wr(ATH_GPIO_OUT_FUNCTION5, (rddata | GPIO_OUT_FUNCTION5_ENABLE_GPIO_20_SET(0x6)));

	ath_reg_wr(ATH_GPIO_OE, ath_reg_rd(ATH_GPIO_OE)& (~0x180000));

	/* Issue a Soft reset */
	ath_i2c_soft_reset();

	/* Disable I2C controller to initialize registers.
     * Few registers should be written only with the
     * controller disabled */
	ath_i2c_disable();

	/* Set Default Parameters for Control and Clock 
	 * Clock 40 MHZ, Master mode, Restart Enable ,
     * Slow Speed */

	ath_reg_wr((ATH_I2C_I2CON), ATH_I2C_DEF_CON);	
	ath_reg_wr((ATH_I2C_SS_SCL_HCNT), ATH_I2C_SS_SCL_HCNT_40);
	ath_reg_wr((ATH_I2C_SS_SCL_LCNT), ATH_I2C_SS_SCL_LCNT_40);
	base_slave_addr = I2C_BASE_SLAVE_ADDR;

	/* Master to address a 7/10 bit slave??? */
	if (ath_i2c_10bit_master) {
		ath_reg_rmw_set(ATH_I2C_I2CON, ATH_I2C_CON_10BITADDR_MASTER);
	}
 
	/* Slave to respond to a 7/10 bit address
	 * access from a master??? */	
	if (ath_i2c_10bit_slave) {
		ath_reg_rmw_set(ATH_I2C_I2CON, ATH_I2C_CON_10BITADDR_SLAVE);
		base_slave_addr = I2C_10BIT_BASE_ADDR;
	}

	/* Write our slave address to which we respond to any access
     * from a master. An ACK is sent whenever another master puts this 
     * address on the I2C bus. The base slave address is fixed
	 * as 0x20 and can be modified with mod param */
	ath_reg_wr(ATH_I2C_SAR, base_slave_addr + ath_i2c_slave_index);

	/* Set Rx Threshold so that the slave can respond when there is
	 * incoming data from the master. Trigger an Rx FULL interrupt when
     * there is 1 data in the receive buffer. Needed here since for slave
     * mode there is no interaction from the app for read/write and is purely
     * based on interrupts */
	ath_reg_wr(ATH_I2C_RX_TL, 0x0);
	
    /* Enable only interrupts required for slave mode. Master mode
	 * interrupts are to be enabled in the respective 
     * read/write routines */

	ath_reg_wr(ATH_I2C_INTR_MASK, (ATH_I2C_INTR_MASK_RXFULL | ATH_I2C_INTR_MASK_RDREQ | 
					               ATH_I2C_INTR_MASK_STOP | ATH_I2C_INTR_MASK_TXABRT)); 

	/* Enable the controller */
	ath_i2c_enable();
}

/* Cleanup module called during rmmod */
void ath_i2c_cleanup_module(void)
{
	ath_i2c_dev_t *dev = &i2c_dev;
	free_irq(dev->irq, NULL);
	unregister_chrdev(ath_i2c_major, "ath_i2c");
}

/* Init module called during insmod */
int __init ath_i2c_init_module(void)
{
	int result = 0;
	ath_i2c_dev_t *dev = &i2c_dev;;
                
	result = register_chrdev(ath_i2c_major, "ath_i2c",
                             &ath_i2c_fops);

	if (result > 0 ) {
		ath_i2c_major = result;
	}else if (result < 0) {
		printk(KERN_WARNING "ath_i2c: can't get major %d\n",
                             ath_i2c_major);
		goto err_reg;
	}

	/* Initialize dev structure */
	dev->irq          = ATH_MISC_IRQ_I2C;
	dev->data_count   = 0;
	dev->reg_addr     = 0;
	dev->rd_req_count = 0;
	dev->tx_abort_source = 0;
	dev->tx_err       = 0;
	dev->master       = 0;

	/* For synchronization */
    init_completion(&dev->cmd_complete);
	
	/* Initialize tasklet for burst transactions */
	tasklet_init(&dev->xfer_more, ath_i2c_xfer_more, (unsigned long) dev);
	mutex_init(&dev->i2c_lock);

	/* Disable all interrupts */
	ath_reg_wr(ATH_I2C_INTR_MASK, 0);

	/* Resigter ISR */
	result = request_irq(dev->irq, (void *) ath_i2c_intr, IRQF_DISABLED,
                        "ath_i2c", NULL);
	if (result) {
		printk(KERN_INFO "i2c: can't get assigned irq %d returns %d\n",
                          dev->irq, result);
		goto err_irq;
	}

	/* GPIO and reg init */
	ath_i2c_init_reg();
	return 0;

	/* Cleanly handle errors encountered while inserting module  */
err_irq:
	unregister_chrdev(ath_i2c_major, "ath_i2c");
err_reg:
	return result; 
}

module_init(ath_i2c_init_module);
module_exit(ath_i2c_cleanup_module);

