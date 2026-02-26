/* Copyright Statement:
 *
 * (C) 2022  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
*/

#include "hdl_physical.h"
#if defined(LOCATION_SERIAL_PORT_TYPE_SPI) && !defined(LOCATION_SERIAL_PORT_TYPE_UART)
#ifdef HDL_ON_LINUX

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include <errno.h>
#include <poll.h>
#include <string.h>
#include <termios.h>

#include "time_utils.h"
#include "utils.h"
#include "location_spi.h"

//for brom and da use.
#define SPIS_CFG_RD_CMD         0x02
#define SPIS_RD_CMD             0x81
#define SPIS_CFG_WR_CMD         0x04
#define SPIS_WR_CMD             0x06
#define SPIS_WS_CMD             0x08
#define SPIS_RS_CMD             0x0a
#define SPIS_PWON_CMD           0x0e
#define SPIS_PWOFF_CMD          0x0c
#define SPIS_CT_CMD             0x10

//for main bin use.
// #define SPIS_CFG_RD_CMD         0x0a
// #define SPIS_RD_CMD             0x81
// #define SPIS_CFG_WR_CMD         0x0c
// #define SPIS_WR_CMD             0x0e
// #define SPIS_RS_CMD             0x06
// #define SPIS_WS_CMD             0x08
// #define SPIS_PWON_CMD           0x04
// #define SPIS_PWOFF_CMD          0x02
// #define SPIS_CT_CMD             0x10

#define SPIS_ADDRESS_ID         0x55aa0000

#define SPI_ERR_CHECK_AND_RETRY_ENABLE
#define HOST_MUX_DELAY                            (5)

#define SPIS_STA_SLV_ON_OFFSET           (0)
#define SPIS_STA_SLV_ON_MASK             (0x1<<SPIS_STA_SLV_ON_OFFSET)
#define SPIS_STA_CFG_SUCCESS_OFFSET      (1)
#define SPIS_STA_CFG_SUCCESS_MASK        (0x1<<SPIS_STA_CFG_SUCCESS_OFFSET)
#define SPIS_STA_TXRX_FIFO_RDY_OFFSET    (2)
#define SPIS_STA_TXRX_FIFO_RDY_MASK      (0x1<<SPIS_STA_TXRX_FIFO_RDY_OFFSET)
#define SPIS_STA_RD_ERR_OFFSET           (3)
#define SPIS_STA_RD_ERR_MASK             (0x1<<SPIS_STA_RD_ERR_OFFSET)
#define SPIS_STA_WR_ERR_OFFSET           (4)
#define SPIS_STA_WR_ERR_MASK             (0x1<<SPIS_STA_WR_ERR_OFFSET)
#define SPIS_STA_RDWR_FINISH_OFFSET      (5)
#define SPIS_STA_RDWR_FINISH_MASK        (0x1<<SPIS_STA_RDWR_FINISH_OFFSET)
#define SPIS_STA_TIMOUT_ERR_OFFSET       (6)
#define SPIS_STA_TIMOUT_ERR_MASK         (0x1<<SPIS_STA_TIMOUT_ERR_OFFSET)
#define SPIS_STA_CMD_ERR_OFFSET          (7)
#define SPIS_STA_CMD_ERR_MASK            (0x1<<SPIS_STA_CMD_ERR_OFFSET)
#define SPIS_STA_CFG_READ_FINISH_OFFSET  (8)
#define SPIS_STA_CFG_READ_FINISH_MASK    (0x1<<SPIS_STA_CFG_READ_FINISH_OFFSET)
#define SPIS_STA_CFG_WRITE_FINISH_OFFSET (9)
#define SPIS_STA_CFG_WRITE_FINISH_MASK   (0x1<<SPIS_STA_CFG_WRITE_FINISH_OFFSET)
#define SPIS_STA_RD_FINISH_OFFSET        (10)
#define SPIS_STA_RD_FINISH_MASK          (0x1<<SPIS_STA_RD_FINISH_OFFSET)
#define SPIS_STA_WR_FINISH_OFFSET        (11)
#define SPIS_STA_WR_FINISH_MASK          (0x1<<SPIS_STA_WR_FINISH_OFFSET)
#define SPIS_STA_POWER_OFF_OFFSET        (12)
#define SPIS_STA_POWER_OFF_MASK          (0x1<<SPIS_STA_POWER_OFF_OFFSET)
#define SPIS_STA_POWER_ON_OFFSET         (13)
#define SPIS_STA_POWER_ON_MASK           (0x1<<SPIS_STA_POWER_ON_OFFSET)


static volatile bool g_spi_transaction_finish;
static uint8_t g_transfer_tx_buffer[4096 + 1];
static uint8_t g_transfer_rx_buffer[4096 + 1];
//static uint8_t g_bit_reverse_buffer[4096 + 1];

static int mSpiFd = 0;

#define LOCATION_SPI_TRANSFER_MAX_SIZE       (4095) 

extern HDL_CONFIG g_hdl_config;

int spi_master_transfer(hal_spi_master_send_and_receive_config_t *xfer)
{
    uint32_t len;
    int ret;
    len = xfer->receive_length > xfer->send_length ? xfer->receive_length : xfer->send_length;
// TODO: if host is MSB, bit reverse send_data
    ret = location_spi_transfer(mSpiFd, xfer->send_data, xfer->receive_buffer, len);
// TODO: if host is MSB, bit reverse receive_buffer
    if (ret > 0) {
        return 0;
    }
    return -1;
}


// /**
// *@brief     In this function we notice spi driver owner that spi transfer has been completed.
// *@param[in] event: event of spi driver.
// *@param[in] user_data: pointer to the data that registered to spi driver.
// *@return    None.
// */
// static void spi_master_dma_callback(hal_spi_master_callback_event_t event, void *user_data)
// {
//     HDL_LOGI("[SPIM] spi_master_dma_callback enter, event = %d", event);
//     g_spi_transaction_finish = TRUE;
// }

/**
*@brief     In this function we query status of spi slaver.
*@param[in] status: Pointer to the result of spi slaver's status.
*@return    None.
*/
static void spi_master_query_slave_status(uint8_t* status)
{
    //portTaskDelayMs(10);
    uint8_t status_cmd[2];
    status_cmd[0] = SPIS_RS_CMD;
    status_cmd[1] = 0x00;
    uint8_t status_receive[2];
    hal_spi_master_send_and_receive_config_t spi_send_and_receive_config;

    /* Note:
     * The value of receive_length is the valid number of bytes received plus the number of bytes to send.
     * For example, here the valid number of bytes received is 1 byte,
     * and the number of bytes to send also is 1 byte, so the receive_length is 2.
     */
    status_receive[1] = 0;
    spi_send_and_receive_config.receive_length = 2;
    spi_send_and_receive_config.send_length = 2;
    spi_send_and_receive_config.send_data = status_cmd;
    spi_send_and_receive_config.receive_buffer = status_receive;
    if (0 != spi_master_transfer(&spi_send_and_receive_config))
    {
        LOG_E("[SPIM] SPI master query status of slaver failed");
        return;
    }
#ifdef HOST_SPI_LOG_ENABLE
    LOG_I("[SPIM] Status receive: 0x%x", status_receive[1]);
#endif
    * status = status_receive[1];
}

#ifdef HOST_SPI_LOG_ENABLE
static uint32_t command_poweron_counter = 0;
static uint32_t command_poweroff_counter = 0;
static uint32_t command_config_read_counter = 0;
static uint32_t command_read_counter = 0;
static uint32_t command_config_write_counter = 0;
static uint32_t command_write_counter = 0;
static uint32_t command_write_status_counter = 0;
#endif

static void spi_master_poweroff_slave(void)
{
    uint8_t poweroff_cmd;
    poweroff_cmd = SPIS_PWOFF_CMD;

    hal_spi_master_send_and_receive_config_t xfer;
    xfer.send_data = &poweroff_cmd;
    xfer.send_length = 1;
    xfer.receive_buffer = NULL;
    xfer.receive_length = 0;

#ifdef HOST_SPI_LOG_ENABLE
    LOG_I("[SPIM] SPI master starts to send PWOFF<0x%x> command to slave, counter=%d", SPIS_PWOFF_CMD, command_poweroff_counter++);
#endif    
    if (0 != spi_master_transfer(&xfer))
    {
        LOG_E("[SPIM] SPI master query status of slaver failed");
        return;
    }
}

static void spi_master_poweron_slave(void)
{
    uint8_t poweron_cmd;
    poweron_cmd = SPIS_PWON_CMD;

    hal_spi_master_send_and_receive_config_t xfer;
    uint32_t max_try_count = 500;
    uint32_t try_count = 0;

    uint8_t status_receive = 0;
    do
    {
        xfer.send_data = &poweron_cmd;
        xfer.send_length = 1;
        xfer.receive_buffer = NULL;
        xfer.receive_length = 0;
#ifdef HOST_SPI_LOG_ENABLE        
        LOG_I("[SPIM] SPI master starts to send PWON<0x%x> command to slave, counter=%d", SPIS_PWON_CMD, command_poweron_counter++);
#endif        
        if (0 != spi_master_transfer(&xfer))
        {
            LOG_E("SPI master query status of slaver failed");
            return;
        }
        try_count++;
        if(try_count > max_try_count)
        {
            LOG_E("SPI master query status of slaver failed(timeout)");
            return;
        }
        portTaskDelayMs(1);
        spi_master_query_slave_status(&status_receive);
        if (status_receive == 0x81) status_receive = 0;
    } while ((status_receive & (uint8_t)SPIS_STA_SLV_ON_MASK) != (uint8_t)SPIS_STA_SLV_ON_MASK);
}

static void spi_master_reset_slave(void)
{
    spi_master_poweroff_slave();
    portTaskDelayMs(1);
    spi_master_poweron_slave();
    portTaskDelayMs(10);
}

static spi_slave_status_t spi_master_query_and_check_slave_status(uint8_t status_mask, uint32_t retry_counter, uint32_t wait_time_ms)
{
    uint8_t status_receive = 0;
    uint8_t status_send[2];
    uint32_t i;

    hal_spi_master_send_and_receive_config_t xfer;
    xfer.send_data = status_send;
    xfer.send_length = 2;
    xfer.receive_buffer = NULL;
    xfer.receive_length = 0;

    for (i = 0; i < retry_counter; i++) {
        if (wait_time_ms > 0)
            portTaskDelayMs(wait_time_ms);

        spi_master_query_slave_status(&status_receive);
        if (status_receive != 0xFF) {
            if ((SPIS_STA_WR_ERR_MASK | SPIS_STA_RD_ERR_MASK) & status_receive) {
#ifdef HOST_SPI_LOG_ENABLE                
                LOG_I("[SPIM] WR_ERR or RD_ERR occured, clear error and retry, counter=%d", command_write_status_counter++);
#endif                
                status_send[0] = SPIS_WS_CMD;
                status_send[1] = status_receive & (SPIS_STA_WR_ERR_MASK | SPIS_STA_RD_ERR_MASK);
                if (0 != spi_master_transfer(&xfer))
                {
                    LOG_E("[SPIM] SPI master query status of slaver failed");
                }
                spi_master_reset_slave();
                return SPI_SLAVE_STATUS_RDWR_ERROR;
            }
            else if (SPIS_STA_CMD_ERR_MASK & status_receive) {
                return SPI_SLAVE_STATUS_CMD_ERROR;
            }
            else if ((status_receive & status_mask) == status_mask) {
                return SPI_SLAVE_STATUS_OK;
            }
            else {
#ifdef HOST_SPI_LOG_ENABLE                
                LOG_I("[SPIM] NOT Ready Status=0x%x, counter=%d", status_receive, i);
#endif                
            }
        }
        else {
            LOG_E("[SPIM] Error Status=0x%x , counter=%d", status_receive, i);
        }
    }
    return SPI_SLAVE_STATUS_CHECK_TIMEOUT;
}

/**
*@brief     In this function the SPI master read the request from the SPI slave.
*@param[in] buffer: the buffer to store the request information from the spi slave.
*@param[in] length: length of the buffer.
*@return    None.
*/
static hdl_spi_master_status_t spi_master_read_data(uint8_t* buffer, uint32_t length)
{
    uint8_t cfg_rd_cmd[9];
#ifndef SPI_ERR_CHECK_AND_RETRY_ENABLE
    uint8_t status_receive = 0;
#endif
    hal_spi_master_send_and_receive_config_t spi_send_and_receive_config;
    spi_slave_status_t slave_status;
    uint32_t crd_retry_counter = 0;
    uint32_t crd_retry_retry_counter = 0;
    uint32_t rd_retry_counter = 0;
#define CRD_RETRY_MAX   20
#define RD_RETRY_MAX   5

#define CRD_CHECK_RETRY_MAX      5
#define CRD_CHECK_RETRY_DELAY_MS 3 //80
#define RD_CHECK_RETRY_MAX       100
#define RD_CHECK_RETRY_DELAY_MS  1

    //The max CRD retry time is (CRD_CHECK_RETRY_DELAY_MS * CRD_CHECK_RETRY_MAX * CRD_RETRY_MAX)*CRD_RETRY_RETRY_MAX
#define CRD_RETRY_RETRY_MAX 2

    cfg_rd_cmd[0] = SPIS_CFG_RD_CMD;
    cfg_rd_cmd[1] = SPIS_ADDRESS_ID & 0xff;
    cfg_rd_cmd[2] = (SPIS_ADDRESS_ID >> 8) & 0xff;
    cfg_rd_cmd[3] = (SPIS_ADDRESS_ID >> 16) & 0xff;
    cfg_rd_cmd[4] = (SPIS_ADDRESS_ID >> 24) & 0xff;
    cfg_rd_cmd[5] = (length - 1) & 0xff;
    cfg_rd_cmd[6] = ((length - 1) >> 8) & 0xff;
    cfg_rd_cmd[7] = ((length - 1) >> 16) & 0xff;
    cfg_rd_cmd[8] = ((length - 1) >> 24) & 0xff;

crd_retry:
#ifdef HOST_SPI_LOG_ENABLE
    LOG_I("[SPIM] SPI master starts to send CR<0x%x> command to slave, length=%d counter=%d", \
        SPIS_CFG_RD_CMD, length, command_config_read_counter++);
    LOG_I("[SPIM]   CRD: re-retry max=%d(%dms), retry max=%d, check max=%d, check delay_ms=%d", \
        CRD_RETRY_RETRY_MAX, (CRD_CHECK_RETRY_DELAY_MS * CRD_CHECK_RETRY_MAX * CRD_RETRY_MAX + CRD_RETRY_MAX * 10) * CRD_RETRY_RETRY_MAX, \
        CRD_RETRY_MAX, CRD_CHECK_RETRY_MAX, CRD_CHECK_RETRY_DELAY_MS);
#endif

#ifdef SPI_ERR_CHECK_AND_RETRY_ENABLE
    spi_send_and_receive_config.receive_length = 0;
    spi_send_and_receive_config.send_length = 9;
    spi_send_and_receive_config.send_data = cfg_rd_cmd;
    spi_send_and_receive_config.receive_buffer = NULL;
    if (0 != spi_master_transfer(&spi_send_and_receive_config))
    {
        LOG_E("[SPIM] SPI master send CR command failed");
        return HDL_SPI_MASTER_STATUS_ERROR;
    }
    slave_status = spi_master_query_and_check_slave_status(SPIS_STA_TXRX_FIFO_RDY_MASK, CRD_CHECK_RETRY_MAX, CRD_CHECK_RETRY_DELAY_MS);
    if (slave_status != SPI_SLAVE_STATUS_OK) {
#ifdef HOST_SPI_LOG_ENABLE        
        LOG_I("[SPIM] 1. SPI master crd_retry %d", slave_status);
#endif        
        crd_retry_counter++;
        if (crd_retry_counter > CRD_RETRY_MAX) {
            crd_retry_counter = 0;
            crd_retry_retry_counter++;
            if (crd_retry_retry_counter >= CRD_RETRY_RETRY_MAX) {
                LOG_E("1. SPI master CRD retry timeout, read failed\r\n");
                return HDL_SPI_MASTER_STATUS_TIMEOUT;
            }
            spi_master_reset_slave();
#ifdef HOST_SPI_LOG_ENABLE            
            LOG_I("1. SPI master crd_retry %d, reset slave", slave_status);
#endif            
            /* if issue can not be fixed by reset here, the slave should be hardware reset. and re-download the FW.*/
        }
        goto crd_retry;
    }
    crd_retry_counter = 0;
#else
    do
    {
        spi_send_and_receive_config.receive_length = 0;
        spi_send_and_receive_config.send_length = 9;
        spi_send_and_receive_config.send_data = cfg_rd_cmd;
        spi_send_and_receive_config.receive_buffer = NULL;
        if (0 != spi_master_transfer(&spi_send_and_receive_config))
        {
            LOG_E("[SPIM] SPI master send CR command failed");
            return HDL_SPI_MASTER_STATUS_ERROR;
        }
    }

    portTaskDelayMs(5); //100
    spi_master_query_slave_status(&status_receive);
}
while ((status_receive & (uint8_t)SPIS_STA_TXRX_FIFO_RDY_MASK) != (uint8_t)SPIS_STA_TXRX_FIFO_RDY_MASK)
;
#endif
g_spi_transaction_finish = false;
g_transfer_tx_buffer[0] = SPIS_RD_CMD;
memset(buffer, 0, length);
spi_send_and_receive_config.receive_length = length + 1;
spi_send_and_receive_config.send_length = 1;
spi_send_and_receive_config.send_data = g_transfer_tx_buffer;
spi_send_and_receive_config.receive_buffer = g_transfer_rx_buffer;
rd_retry:
#ifdef HOST_SPI_LOG_ENABLE
LOG_I("[SPIM] SPI master starts to send RD<0x%x> command to slave, counter=%d", SPIS_RD_CMD, command_read_counter++);
LOG_I("[SPIM]   RD: retry max=%d, check max=%d, check delay_ms=%d", \
    RD_RETRY_MAX, RD_CHECK_RETRY_MAX, RD_CHECK_RETRY_DELAY_MS);
#endif
    if (0 != spi_master_transfer(&spi_send_and_receive_config))
{
    LOG_E("[SPIM] SPI master send dma failed");
    return HDL_SPI_MASTER_STATUS_ERROR;
}

#ifdef SPI_ERR_CHECK_AND_RETRY_ENABLE
slave_status = spi_master_query_and_check_slave_status(SPIS_STA_RDWR_FINISH_MASK, RD_CHECK_RETRY_MAX, RD_CHECK_RETRY_DELAY_MS);
if (slave_status != SPI_SLAVE_STATUS_OK) {
    if (slave_status == SPI_SLAVE_STATUS_RDWR_ERROR) {
#ifdef HOST_SPI_LOG_ENABLE            
        LOG_I("[SPIM] 2. SPI master crd_retry %d", slave_status);
#endif            
        goto crd_retry;
    }
    else {
        rd_retry_counter++;
        if (rd_retry_counter > RD_RETRY_MAX) {
            rd_retry_counter = 0;
            spi_master_reset_slave();
#ifdef HOST_SPI_LOG_ENABLE                
            LOG_I("[SPIM] 2. SPI master crd_retry %d, reset slave", slave_status);
#endif                
            goto crd_retry;
        }
#ifdef HOST_SPI_LOG_ENABLE            
        LOG_I("[SPIM] 2. SPI master rd_retry %d", slave_status);
#endif            
        goto rd_retry;
    }
}
rd_retry_counter = 0;
#else
do {
    spi_master_query_slave_status(&status_receive);
} while ((status_receive & (uint8_t)SPIS_STA_RDWR_FINISH_MASK) != (uint8_t)SPIS_STA_RDWR_FINISH_MASK);
#endif
memcpy(buffer, &(g_transfer_rx_buffer[1]), length);
return HDL_SPI_MASTER_STATUS_OK;
}

/**
*@brief     In this function the SPI master write the data to the SPI slave.
*@param[in] buffer: the buffer to store the data needed to sent to the spi slave.
*@param[in] length: length of the buffer.
*@return    None.
*/
static hdl_spi_master_status_t spi_master_write_data(uint8_t* buffer, uint32_t length)
{
    uint8_t cfg_wr_cmd[9];
#ifndef SPI_ERR_CHECK_AND_RETRY_ENABLE
    uint8_t status_receive = 0;
#endif
    //LOG_I("spi_master_write_data buffer[0]: 0x%X", buffer[0]);
    hal_spi_master_send_and_receive_config_t spi_send_and_receive_config;
    spi_slave_status_t slave_status;
    uint32_t cwr_retry_counter = 0;
    uint32_t cwr_retry_retry_counter = 0;
    uint32_t wr_retry_counter = 0;

#define CWR_RETRY_MAX  20
#define WR_RETRY_MAX   5

#define CWR_CHECK_RETRY_MAX      5
#define CWR_CHECK_RETRY_DELAY_MS 3   //5
#define WR_CHECK_RETRY_MAX       100
#define WR_CHECK_RETRY_DELAY_MS  3   //5

    //The max CWR retry time is (CWR_CHECK_RETRY_DELAY_MS * CWR_CHECK_RETRY_MAX * CWR_RETRY_MAX)*CWR_RETRY_RETRY_MAX
#define CWR_RETRY_RETRY_MAX 10

    cfg_wr_cmd[0] = SPIS_CFG_WR_CMD;
    cfg_wr_cmd[1] = SPIS_ADDRESS_ID & 0xff;
    cfg_wr_cmd[2] = (SPIS_ADDRESS_ID >> 8) & 0xff;
    cfg_wr_cmd[3] = (SPIS_ADDRESS_ID >> 16) & 0xff;
    cfg_wr_cmd[4] = (SPIS_ADDRESS_ID >> 24) & 0xff;
    cfg_wr_cmd[5] = (length - 1) & 0xff;
    cfg_wr_cmd[6] = ((length - 1) >> 8) & 0xff;
    cfg_wr_cmd[7] = ((length - 1) >> 16) & 0xff;
    cfg_wr_cmd[8] = ((length - 1) >> 24) & 0xff;
cwr_retry:
#ifdef HOST_SPI_LOG_ENABLE
    LOG_I("[SPIM] SPI master starts to send CWR<0x%x> command to slave, length=%d counter=%d", \
        SPIS_CFG_WR_CMD, length, command_config_write_counter++);
    LOG_I("[SPIM]   CWR: re-retry max=%d(%dms), retry max=%d, check max=%d, check delay_ms=%d", \
        CWR_RETRY_RETRY_MAX, (CWR_CHECK_RETRY_DELAY_MS * CWR_CHECK_RETRY_MAX * CWR_RETRY_MAX + CWR_RETRY_MAX * 10) * CWR_RETRY_RETRY_MAX, \
        CWR_RETRY_MAX, CWR_CHECK_RETRY_MAX, CWR_CHECK_RETRY_DELAY_MS);
#endif         

#ifdef SPI_ERR_CHECK_AND_RETRY_ENABLE
    spi_send_and_receive_config.receive_length = 0;
    spi_send_and_receive_config.send_length = 9;
    spi_send_and_receive_config.send_data = cfg_wr_cmd;
    spi_send_and_receive_config.receive_buffer = NULL;
    if (0 != spi_master_transfer(&spi_send_and_receive_config))
    {
        LOG_E("[SPIM] SPI master send CWR command failed");
        return HDL_SPI_MASTER_STATUS_ERROR;
    }
    slave_status = spi_master_query_and_check_slave_status(SPIS_STA_TXRX_FIFO_RDY_MASK, CWR_CHECK_RETRY_MAX, CWR_CHECK_RETRY_DELAY_MS);
    if (slave_status != SPI_SLAVE_STATUS_OK) {
#ifdef HOST_SPI_LOG_ENABLE        
        LOG_I("[SPIM] 1. SPI master cwr_retry %d", slave_status);
#endif        
        cwr_retry_counter++;
        if (cwr_retry_counter > CWR_RETRY_MAX) {
            cwr_retry_counter = 0;
            cwr_retry_retry_counter++;
            if (cwr_retry_retry_counter > CWR_RETRY_RETRY_MAX) {
                LOG_E("[SPIM] 1. SPI master CWR retry timeout, write failed\r\n");
                return HDL_SPI_MASTER_STATUS_TIMEOUT;
            }
            spi_master_reset_slave();
#ifdef HOST_SPI_LOG_ENABLE            
            LOG_I("[SPIM] 1. SPI master cwr_retry %d, reset slave", slave_status);
#endif            
            /* if issue can not be fixed by reset here, the slave should be hardware reset. */
        }
        goto cwr_retry;
    }
    cwr_retry_counter = 0;
#else
    while ((status_receive & (uint8_t)SPIS_STA_TXRX_FIFO_RDY_MASK) != (uint8_t)SPIS_STA_TXRX_FIFO_RDY_MASK)
    {
        spi_send_and_receive_config.receive_length = 0;
        spi_send_and_receive_config.send_length = 9;
        spi_send_and_receive_config.send_data = cfg_wr_cmd;
        spi_send_and_receive_config.receive_buffer = NULL;
        if (0 != spi_master_transfer(&spi_send_and_receive_config))
        {
            LOG_E("[SPIM] SPI master send CWR command failed");
            return HDL_SPI_MASTER_STATUS_ERROR;
        }

        for (int i = 0; i < 10; i++)
        {
            portTaskDelayMs(5);
            spi_master_query_slave_status(&status_receive);
            if (status_receive != 0xFF && ((status_receive & (uint8_t)SPIS_STA_TXRX_FIFO_RDY_MASK) == (uint8_t)SPIS_STA_TXRX_FIFO_RDY_MASK))
            {
                break;
            }
        }
    }
#endif
    g_spi_transaction_finish = false;
    g_transfer_tx_buffer[0] = SPIS_WR_CMD;
    memcpy(&(g_transfer_tx_buffer[1]), buffer, length);
wr_retry:
#ifdef HOST_SPI_LOG_ENABLE
    LOG_I("[SPIM] SPI master starts to send WR<0x%x> command to slave, counter=%d", SPIS_WR_CMD, command_write_counter++);
    LOG_I("[SPIM]   WR: retry max=%d, check max=%d, check delay_ms=%d", \
        WR_RETRY_MAX, WR_CHECK_RETRY_MAX, WR_CHECK_RETRY_DELAY_MS);
#endif

    spi_send_and_receive_config.receive_length = 0;
    spi_send_and_receive_config.send_length = length + 1;
    spi_send_and_receive_config.send_data = g_transfer_tx_buffer;
    spi_send_and_receive_config.receive_buffer = NULL;
    if (0 != spi_master_transfer(&spi_send_and_receive_config))
    {
        LOG_E("[SPIM] SPI master send WR command failed");
        return HDL_SPI_MASTER_STATUS_ERROR;
    }


#ifdef SPI_ERR_CHECK_AND_RETRY_ENABLE
    slave_status = spi_master_query_and_check_slave_status(SPIS_STA_RDWR_FINISH_MASK, WR_CHECK_RETRY_MAX, WR_CHECK_RETRY_DELAY_MS);
    if (slave_status != SPI_SLAVE_STATUS_OK) {
        if (slave_status == SPI_SLAVE_STATUS_RDWR_ERROR) {
#ifdef HOST_SPI_LOG_ENABLE            
            LOG_I("[SPIM] 2. SPI master cwr_retry %d", slave_status);
#endif            
            goto cwr_retry;
        }
        else {
            wr_retry_counter++;
            if (wr_retry_counter > WR_RETRY_MAX) {
                wr_retry_counter = 0;
                spi_master_reset_slave();
#ifdef HOST_SPI_LOG_ENABLE                
                LOG_I("[SPIM] 2. SPI master cwr_retry %d, reset slave", slave_status);
#endif                
                goto cwr_retry;
            }
#ifdef HOST_SPI_LOG_ENABLE            
            LOG_I("[SPIM] 2. SPI master wr_retry %d", slave_status);
#endif            
            goto wr_retry;
        }
    }
    wr_retry_counter = 0;
#else
    do {
        spi_master_query_slave_status(&status_receive);
    } while ((status_receive & (uint8_t)SPIS_STA_RDWR_FINISH_MASK) != (uint8_t)SPIS_STA_RDWR_FINISH_MASK);
#endif
    return HDL_SPI_MASTER_STATUS_OK;
}




bool HDL_COM_Init()
{
    location_spi_config_struct_t spi_config;
    int fd = 0;

    spi_config.mode = 0;
    spi_config.lsb_first = 1;             // TODO: if host is MSB, spi_config.lsb_first = 0
    spi_config.bits_per_word = 8;
    spi_config.speed = g_hdl_config.clock_frequency;
    if (snprintf(spi_config.path, sizeof(spi_config.path), "%s", g_hdl_config.io_path) < 0) {
        LOG_E("[SPIM] HDL_COM_Init, get path fail");
    }

    if (location_spi_init(&fd, &spi_config) == -1) {
        return false;
    }
    mSpiFd = fd;
    //spi_master_reset_slave();
    return true;
}
bool HDL_COM_Deinit()
{
    return close(mSpiFd) == 0;
}

uint32_t HDL_COM_GetByte_Buffer(void* buf, uint32_t length, bool sw_flow_ctrl_open)
{
    if (buf == NULL || length == 0)
    {
        return 0;
    }
    uint8_t temp_data[LOCATION_SPI_TRANSFER_MAX_SIZE] = { 0 };
    uint32_t len = length;
    uint32_t i = 0;
    while (len > LOCATION_SPI_TRANSFER_MAX_SIZE)
    {
        //memset(temp_data, 0, LOCATION_SPI_TRANSFER_MAX_SIZE * sizeof(uint8_t));
        memset(temp_data, 0, LOCATION_SPI_TRANSFER_MAX_SIZE);
        spi_master_read_data(temp_data, LOCATION_SPI_TRANSFER_MAX_SIZE);
        memcpy(buf + i * LOCATION_SPI_TRANSFER_MAX_SIZE, temp_data, LOCATION_SPI_TRANSFER_MAX_SIZE);
        i++;
        len -= LOCATION_SPI_TRANSFER_MAX_SIZE;
    }
    //memset(temp_data, 0, LOCATION_SPI_TRANSFER_MAX_SIZE * sizeof(uint8_t));
    memset(temp_data, 0, LOCATION_SPI_TRANSFER_MAX_SIZE);
    spi_master_read_data(temp_data, len);
    //memcpy(buf + i * _SPI_TRANSFER_MAX_SIZE, temp_data, LOCATION_SPI_TRANSFER_MAX_SIZE);
    memcpy(buf + i * LOCATION_SPI_TRANSFER_MAX_SIZE, temp_data, len);


    //spi_master_read_data(buf, length);
    return length;
}

uint8_t flowctrl_buf_linux[FLOW_CTRL_BUF_SIZE] = { 0 };
bool HDL_COM_PutByte_Buffer(const void* buf, uint32_t length, bool sw_flow_ctrl_open)
{
    if (buf == NULL || length == 0)
    {
        return false;
    }
    uint8_t* real_data = (uint8_t*)buf;
    //spi_master_write_data(real_data, length);

    uint32_t len = length;
    uint32_t i = 0;
    while (len > LOCATION_SPI_TRANSFER_MAX_SIZE)
    {
        spi_master_write_data(real_data + i * LOCATION_SPI_TRANSFER_MAX_SIZE, LOCATION_SPI_TRANSFER_MAX_SIZE);
        i++;
        len -= LOCATION_SPI_TRANSFER_MAX_SIZE;
    }
    spi_master_write_data(real_data + i * LOCATION_SPI_TRANSFER_MAX_SIZE, len);

    return true;
}

bool HDL_COM_SetBaudRate(uint32_t baud_rate)
{
	return true;
}
bool HDL_COM_SetFlowCtrl(FlowControl fc)
{
	return true;
}

#endif
#endif
