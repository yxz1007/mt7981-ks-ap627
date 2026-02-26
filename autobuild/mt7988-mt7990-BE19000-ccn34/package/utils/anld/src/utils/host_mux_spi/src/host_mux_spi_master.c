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
#include "port_platform_interface.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <assert.h>

#define MAX_HOST_MUX_SPI_BUF_SZ     1024

#define MAX_ERROR_NUMBER        100
#define SPIS_CFG_RD_CMD         0x0a
#define SPIS_RD_CMD             0x81
#define SPIS_CFG_WR_CMD         0x0c
#define SPIS_WR_CMD             0x0e
#define SPIS_RS_CMD             0x06
#define SPIS_WS_CMD             0x08
#define SPIS_PWON_CMD           0x04
#define SPIS_PWOFF_CMD          0x02
#define SPIS_CT_CMD             0x10

#define HOST_MUX_DELAY_MS                         (5)
#define HOST_MUX_SEPHORE_TIMEOUT                  (10000)
#define VG_SPI_SLAVE_CMD_LEN                      (9) 
#define VG_SPI_SLAVE_TX_LEN_REG_OFFSET            (0x08)
#define VG_SPI_SLAVE_TX_BUF_REG_OFFSET            (0x2000)

#define VG_SPI_SLAVE_RX_LEN_REG_OFFSET            (0x04)
#define VG_SPI_SLAVE_RX_BUF_REG_OFFSET            (0x1000)

#define HOST_MUX_SPIS_STA_SLV_ON_OFFSET           (0)
#define HOST_MUX_SPIS_STA_SLV_ON_MASK             (0x1<<HOST_MUX_SPIS_STA_SLV_ON_OFFSET)

#define HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_OFFSET    (2)
#define HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK      (0x1<<HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_OFFSET)

#define HOST_MUX_SPIS_STA_RDWR_FINISH_OFFSET      (5)
#define HOST_MUX_SPIS_STA_RDWR_FINISH_MASK        (0x1<<HOST_MUX_SPIS_STA_RDWR_FINISH_OFFSET)

#define HOST_MUX_SPIS_STA_RD_ERR_OFFSET           (3)
#define HOST_MUX_SPIS_STA_RD_ERR_MASK             (0x1<<HOST_MUX_SPIS_STA_RD_ERR_OFFSET)

#define HOST_MUX_SPIS_STA_WR_ERR_OFFSET           (4)
#define HOST_MUX_SPIS_STA_WR_ERR_MASK             (0x1<<HOST_MUX_SPIS_STA_WR_ERR_OFFSET)

static volatile uint32_t g_host_mux_spi_master_power_on_counter = {0};
static void *g_host_mux_spi_master_mutex = NULL;
static void *g_host_mux_spi_master_wait_send_done_xSemaphore = NULL;
static void *g_host_mux_spi_master_wait_receive_done_xSemaphore = NULL;

static  uint8_t temp_host_mux_rx_buf[MAX_HOST_MUX_SPI_BUF_SZ];
static  uint8_t temp_host_mux_tx_buf[MAX_HOST_MUX_SPI_BUF_SZ];


static host_mux_status_t portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY(int handle, uint32_t bit_mask,uint32_t bit_value,uint32_t retry_counter);



static  host_mux_status_t   portable_HAL_SPI_POWER_ON_WITHOUT_COUNTER(int handle)
{
    uint8_t poweron_cmd;
    platform_bus_transfer_t  xfer;

    while (1) {
        poweron_cmd = SPIS_PWON_CMD;

        xfer.tx_buff = &poweron_cmd;
        xfer.tx_len  = 1;
        xfer.rx_buff = NULL;
        xfer.rx_len  = 0;
        if (0 != port_platform_bus_transfer(handle, &xfer)) {
            LOG_MUX_SPIM_E("host_mux_spi_power_on_without_counter: transfer fail\r\n", 0);
            return HOST_MUX_STATUS_ERROR;
        }
        if (HOST_MUX_STATUS_OK == portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY(handle, HOST_MUX_SPIS_STA_SLV_ON_MASK, HOST_MUX_SPIS_STA_SLV_ON_MASK , MAX_ERROR_NUMBER))
            break;
    }
    LOG_MUX_SPIM_I("portable_HAL_SPI_POWER_ON_WITHOUT_COUNTER success!!!\r\n", 0);
    return HOST_MUX_STATUS_OK;
}

static host_mux_status_t portable_HAL_SPI_POWER_ON(int handle)
{
    if (g_host_mux_spi_master_power_on_counter == 0) {
        if (HOST_MUX_STATUS_OK != portable_HAL_SPI_POWER_ON_WITHOUT_COUNTER(handle)) {
            return HOST_MUX_STATUS_ERROR;
        }
    }
    g_host_mux_spi_master_power_on_counter++;
    LOG_MUX_SPIM_I("host_mux_spi_power_on: success!!! counter=%d\r\n", 1, (int)g_host_mux_spi_master_power_on_counter);
    return HOST_MUX_STATUS_OK;
}


static host_mux_status_t portable_HAL_SPI_POWER_OFF_WITHOUT_COUNTER(int handle)
{
    uint8_t poweroff_cmd;
    platform_bus_transfer_t  xfer;

    while (1) {
        poweroff_cmd = SPIS_PWOFF_CMD;

        xfer.tx_buff = &poweroff_cmd;
        xfer.tx_len  = 1;
        xfer.rx_buff = NULL;
        xfer.rx_len  = 0;

        if (0 != port_platform_bus_transfer(handle, &xfer)) {
            LOG_MUX_SPIM_E("portable_HAL_SPI_POWER_OFF_WITHOUT_COUNTER: transfer fail\r\n", 0);
            return HOST_MUX_STATUS_ERROR;
        }
        if ( HOST_MUX_STATUS_OK == portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY(handle, HOST_MUX_SPIS_STA_SLV_ON_MASK , 0, MAX_ERROR_NUMBER))
            break;
    }
    LOG_MUX_SPIM_I("portable_HAL_SPI_POWER_OFF_WITHOUT_COUNTER success!!!\r\n", 0);
    return HOST_MUX_STATUS_OK;
}


static host_mux_status_t portable_HAL_SPI_POWER_OFF(int handle)
{
    if (g_host_mux_spi_master_power_on_counter == 0) {
        //LOG_MUX_SPIM_E("portable_HAL_SPI_POWER_OFF: Assert!!!", 0);
        //assert(0);
        LOG_MUX_SPIM_E("portable_HAL_SPI_POWER_OFF: error!!!", 0);
        return HOST_MUX_STATUS_ERROR;
    }
    if (g_host_mux_spi_master_power_on_counter == 1) {
        if (HOST_MUX_STATUS_OK != portable_HAL_SPI_POWER_OFF_WITHOUT_COUNTER(handle)) {
            return HOST_MUX_STATUS_ERROR;
        }
    }
    g_host_mux_spi_master_power_on_counter--;
    LOG_MUX_SPIM_I("portable_HAL_SPI_POWER_OFF: success!!!, counter=%d\r\n", 1, (int)g_host_mux_spi_master_power_on_counter);
    return HOST_MUX_STATUS_OK;
}



static host_mux_status_t portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY(int handle, uint32_t bit_mask,uint32_t bit_value,uint32_t retry_counter)
{
    uint8_t tx_buf[2];
    uint8_t rx_buf[2];
    uint8_t status;
    uint32_t i;
    platform_bus_transfer_t  xfer;


    /* Note:
     * The value of receive_length is the valid number of bytes received plus the number of bytes to send.
     * For example, here the valid number of bytes received is 1 byte,
     * and the number of bytes to send also is 1 byte, so the receive_length is 2.
     */
    for(i=0;i<retry_counter;i++) {
        
        tx_buf[0] = SPIS_RS_CMD;
        tx_buf[1] = 0;

        xfer.tx_buff = tx_buf;
        xfer.tx_len  = 2;
        xfer.rx_buff = rx_buf;
        xfer.rx_len  = 2;
        if (0 != port_platform_bus_transfer(handle, &xfer)) {
            LOG_MUX_SPIM_E("portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY: transfer fail 0\r\n", 0);
            return HOST_MUX_STATUS_ERROR;
        }
        LOG_MUX_SPIM_I("portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY  Status receive: 0x%x\r\n", 1, rx_buf[1]);
        status = rx_buf[1];
        if (status != 0xff) {
            if (status & (HOST_MUX_SPIS_STA_RD_ERR_MASK | HOST_MUX_SPIS_STA_WR_ERR_MASK)) {
                LOG_MUX_SPIM_I("portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY,Slave tansfer Error:0x%x, need reset Slave \r\n", 1, status);
                tx_buf[0]= SPIS_WS_CMD;
                tx_buf[1] = status;

                xfer.tx_buff = tx_buf;
                xfer.tx_len  = 2;
                xfer.rx_buff = NULL;
                xfer.rx_len  = 0;
                if (0 != port_platform_bus_transfer(handle, &xfer)) {
                    LOG_MUX_SPIM_E("portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY: transfer fail 1\r\n", 0);
                    return HOST_MUX_STATUS_ERROR;
                }
                LOG_MUX_SPIM_E("Transfer error, now Master reset Slave!!!\r\n", 0);
                port_platform_delay_us(100);
                portable_HAL_SPI_POWER_OFF_WITHOUT_COUNTER(handle);
                portable_HAL_SPI_POWER_ON_WITHOUT_COUNTER(handle);
                return HOST_MUX_STATUS_ERROR;
            } else if ((bit_mask & status)== bit_value) {
                LOG_MUX_SPIM_I("portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY OK, retry counter:%d,\r\n", 1, (int)i);
                return HOST_MUX_STATUS_OK;
            } else if (((bit_mask == HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK) || (bit_mask == HOST_MUX_SPIS_STA_RDWR_FINISH_MASK)) && (status & HOST_MUX_SPIS_STA_SLV_ON_MASK) == 0) {
                LOG_MUX_SPIM_I("portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY spi slave is power off!!!!!!\r\n", 0);
                portable_HAL_SPI_POWER_OFF_WITHOUT_COUNTER(handle);
                portable_HAL_SPI_POWER_ON_WITHOUT_COUNTER(handle);
                return HOST_MUX_STATUS_ERROR;
            }
            port_platform_delay_ms(HOST_MUX_DELAY_MS);
        } else {
            /* For SPI multi-slave, SPI slave swtich MISO to Hi-Z when PWROFF command received,
            MISO may be always high or low level, so, master always read 0xFF or 0x00, we treat
            this as a valid status. */
            if (bit_value == 0) {
                return HOST_MUX_STATUS_OK;
            }
            return HOST_MUX_STATUS_ERROR;
        }
    }
    LOG_MUX_SPIM_W("portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY, retry counter:%d,\r\n", 1, (int)i);
    return HOST_MUX_STATUS_ERROR;
}


void portable_HOST_MUX_SPI_MASTER_wait_for_send_done()
{
    if(port_platform_take_samphore(g_host_mux_spi_master_wait_send_done_xSemaphore, HOST_MUX_SEPHORE_TIMEOUT)) {

    } else {
        //LOG_MUX_SPIM_E("Assert!!!", 0);
        //assert(0);
        LOG_MUX_SPIM_E("portable_HOST_MUX_SPI_MASTER_wait_for_send_done: error!!!", 0);
        return;
    }
}

void portable_HOST_MUX_SPI_MASTER_wait_for_receive_done()
{
    if(port_platform_take_samphore( g_host_mux_spi_master_wait_receive_done_xSemaphore, HOST_MUX_SEPHORE_TIMEOUT)) {

    } else {
        //LOG_MUX_SPIM_E("Assert!!!", 0);
        //assert(0);
        LOG_MUX_SPIM_E("portable_HOST_MUX_SPI_MASTER_wait_for_receive_done: error!!!", 0);
        return;
    }
}

void portable_HOST_MUX_SPI_MASTER_RECEIVE_CB(void *user_data)
{
    port_platform_give_samphore(g_host_mux_spi_master_wait_receive_done_xSemaphore);
}

void portable_HOST_MUX_SPI_MASTER_SEND_CB(void *user_data)
{
    port_platform_give_samphore(g_host_mux_spi_master_wait_send_done_xSemaphore);
}



int portable_HAL_SPI_MASTER_INIT(platform_bus_config_t *config)
{
    int  handle;

    if(g_host_mux_spi_master_mutex == NULL)
        g_host_mux_spi_master_mutex = port_platform_create_mutex();
    if (g_host_mux_spi_master_mutex == NULL) {
        LOG_MUX_SPIM_E( "g_host_mux_spi_master_mutex create error\r\n", 0);
        return -1;
    }
    if(g_host_mux_spi_master_wait_send_done_xSemaphore == NULL)
        g_host_mux_spi_master_wait_send_done_xSemaphore = port_platform_create_semphore();
    if (g_host_mux_spi_master_wait_send_done_xSemaphore == NULL) {
        LOG_MUX_SPIM_E( "g_host_mux_spi_master_wait_send_done_xSemaphore create error\r\n", 0);
        return -1;
    }
    if(g_host_mux_spi_master_wait_receive_done_xSemaphore == NULL)
        g_host_mux_spi_master_wait_receive_done_xSemaphore = port_platform_create_semphore();
    if (g_host_mux_spi_master_wait_receive_done_xSemaphore == NULL) {
        LOG_MUX_SPIM_E( "g_host_mux_spi_master_wait_receive_done_xSemaphore create error\r\n", 0);
        return -1;
    }

    handle = port_platform_bus_init(PLATFORM_BUS_SPI, config);
    return handle;
}


host_mux_status_t portable_HAL_SPI_MASTER_DEINIT(int handle)
{
    if( port_platform_bus_deinit(handle) < 0) {
        LOG_MUX_SPIM_E( "portable_HAL_SPI_MASTER_DEINIT: bus de-init err\r\n", 0);
        return HOST_MUX_STATUS_ERROR;
    }
    port_platform_destroy_mutex(&g_host_mux_spi_master_mutex);
    port_platform_destroy_samphore(&g_host_mux_spi_master_wait_send_done_xSemaphore);
    port_platform_destroy_samphore(&g_host_mux_spi_master_wait_receive_done_xSemaphore);
    g_host_mux_spi_master_power_on_counter = 0;
    return HOST_MUX_STATUS_OK;
}



/*
    This is Master read data from slave:
    Step0: Power ON SPI slave
    Step1:Master read Slave the register of <VG_SPI_SLAVE_TX_LEN_REG_OFFSET> with the len of 4
         Step1_a: Master send Config Read (CR) cmd to slave:
                    Master send to slave 9 bytes cmd: [0]:SPIS_CFG_RD_CMD, [1:4]VG_SPI_SLAVE_TX_LEN_REG_OFFSET,[5:8] len-1
         Step1_b: Wait for SPI slave CR ready,check HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK.
         Step1_c: Master receive the value of <VG_SPI_SLAVE_TX_LEN_REG_OFFSET>
               Master receive 4 bytes, it's the data len which Slave prepared.
         Step1_d: Master query and check Slave_Tx done status,check HOST_MUX_SPIS_STA_RDWR_FINISH_MASK.
    Step2:Master read Slave the register of <VG_SPI_SLAVE_TX_BUF_REG_OFFSET> with the len of data_buffer_len.
            Step2_a: Master send Config Read (CR) cmd to slave:
               Master send to slave 9 bytes cmd: [0]:SPIS_CFG_RD_CMD, [1:4]VG_SPI_SLAVE_TX_BUF_REG_OFFSET,[5:8] len-1
            Step2_b: Wait for SPI slave CR ready,check HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK.
            Step2_c: Master receive <data_buffer_len> data.
                    Master receive <data_buffer_len> bytes, it's the data which Slave prepared
            Step2_d:Master and check Slave_Tx done status,check HOST_MUX_SPIS_STA_RDWR_FINISH_MASK.
     Step3: Power OFF
*/



host_mux_status_t portable_HAL_SPI_MASTER_RX(int handle, uint8_t *buf, uint32_t *receive_done_data_len)
{

    uint8_t *request_cmd = NULL;
    uint32_t receive_reg_value = 0;
    uint32_t length = 0, offset = 0;
    platform_bus_transfer_t  xfer;
    host_mux_status_t status = 0;


    /*Step0: Power ON SPI slave*/
    port_platform_take_mutex(g_host_mux_spi_master_mutex);
    portable_HAL_SPI_POWER_ON(handle);
    port_platform_give_mutex(g_host_mux_spi_master_mutex);

//mux_spi_master_demo_receive_restart:

    /*Step1:Master read Slave the register of <VG_SPI_SLAVE_TX_LEN_REG_OFFSET> with the len of 4*/
    {
        port_platform_take_mutex(g_host_mux_spi_master_mutex);
        /* Step1_a: Master send Config Read (CR) cmd to slave:
                    Master send to slave 9 bytes cmd: [0]:SPIS_CFG_RD_CMD, [1:4]VG_SPI_SLAVE_TX_LEN_REG_OFFSET,[5:8] len-1*/
        length = 4;
        offset = VG_SPI_SLAVE_TX_LEN_REG_OFFSET;
        request_cmd    = (uint8_t *) temp_host_mux_tx_buf;
        request_cmd[0] = SPIS_CFG_RD_CMD;
        request_cmd[1] = offset & 0xff;
        request_cmd[2] = (offset >> 8) & 0xff;
        request_cmd[3] = (offset >> 16) & 0xff;
        request_cmd[4] = (offset >> 24) & 0xff;
        request_cmd[5] = (length - 1) & 0xff;
        request_cmd[6] = ((length - 1) >> 8) & 0xff;
        request_cmd[7] = ((length - 1) >> 16) & 0xff;
        request_cmd[8] = ((length - 1) >> 24) & 0xff;
        LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step1_a: Send VG_SPI_SLAVE_CRD_CMD.\r\n", 0);
        LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step1_a: Master want to send 9B\r\n", 0);

        xfer.tx_buff = request_cmd;
        xfer.tx_len  = VG_SPI_SLAVE_CMD_LEN;
        xfer.rx_buff = NULL;
        xfer.rx_len  = 0;

        if (0 != port_platform_bus_transfer(handle, &xfer)) {
            LOG_MUX_SPIM_E("mux_spi_master_demo_receive-Step1_a:: SPI master send err, status:%d \r\n", 1, status);
            //LOG_MUX_SPIM_E("Assert!!!", 0); 
            //assert(0);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            return HOST_MUX_STATUS_ERROR;
        } else {
            LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step1_a: Send VG_SPI_SLAVE_CRD_CMD success!!!\r\n", 0);
        }

        /* Step1_b: Wait for SPI slave CR ready,check HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK*/
        LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step1_b: wait slave CR done...\r\n", 0);
        if(portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY(handle, HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK,
            HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK,MAX_ERROR_NUMBER)==HOST_MUX_STATUS_ERROR)
        {
            LOG_MUX_SPIM_E("mux_spi_master_demo_receive-Step1_b: #### too many ERROR, now go to restart!!!!!\r\n", 0);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            //goto mux_spi_master_demo_receive_restart;
            return HOST_MUX_STATUS_ERROR;
        }

        /*Step1_c: Master receive the value of <VG_SPI_SLAVE_TX_LEN_REG_OFFSET>
                Master receive 4 bytes, it's the data len which Slave prepared.*/
        LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step1_c: Receive SPI slave Tx_len Reg value. \r\n", 0);
        LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step1_c: Master want to receive 4B.\r\n", 0);
        
        request_cmd  = (uint8_t *) temp_host_mux_tx_buf;
        memset(temp_host_mux_tx_buf, 0, sizeof(temp_host_mux_tx_buf));
        request_cmd[0] = SPIS_RD_CMD;

        xfer.tx_buff = request_cmd;
        xfer.tx_len  = 4 + 1;
        xfer.rx_buff = (uint8_t *)temp_host_mux_rx_buf;
        xfer.rx_len  = 4 + 1;

        if (0 != port_platform_bus_transfer(handle, &xfer)) {
            LOG_MUX_SPIM_E("mux_spi_master_demo_receive-Step1_c : SPI master receive err,status:%d\r\n", 1, 0);
            //LOG_MUX_SPIM_E("Assert!!!", 0); 
            //assert(0);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            return HOST_MUX_STATUS_ERROR;
        }
        receive_reg_value = temp_host_mux_rx_buf[1] | (temp_host_mux_rx_buf[2]<<8) | (temp_host_mux_rx_buf[3]<<16)|(temp_host_mux_rx_buf[4]<<24);
        LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step1_c: Receive SPI slave Tx_len Reg value:0x%x. success!!!\r\n", 1, (unsigned int)receive_reg_value);
        if(receive_reg_value > *receive_done_data_len){
            receive_reg_value = *receive_done_data_len;
            LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step1_c: slave data len too big, request %d B firstly;\r\n", 1, (int)*receive_done_data_len);
        }
        if (receive_reg_value == 0) {
            *receive_done_data_len = 0;
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            return HOST_MUX_STATUS_OK;
        }
        port_platform_give_mutex(g_host_mux_spi_master_mutex);
    }

    /*Step2:Master read Slave the register of <VG_SPI_SLAVE_TX_BUF_REG_OFFSET> with the len of data_buffer_len. */
    {
        port_platform_take_mutex(g_host_mux_spi_master_mutex);

        /*  Step2_a: Master send Config Read (CR) cmd to slave:
                     Master send to slave 9 bytes cmd: [0]:SPIS_CFG_RD_CMD, [1:4]VG_SPI_SLAVE_TX_BUF_REG_OFFSET,[5:8] len-1*/
        length = receive_reg_value;
        offset = VG_SPI_SLAVE_TX_BUF_REG_OFFSET;
        request_cmd    = (uint8_t *)temp_host_mux_tx_buf;

        request_cmd[0] = SPIS_CFG_RD_CMD;
        request_cmd[1] = offset & 0xff;
        request_cmd[2] = (offset >> 8) & 0xff;
        request_cmd[3] = (offset >> 16) & 0xff;
        request_cmd[4] = (offset >> 24) & 0xff;
        request_cmd[5] = (length - 1) & 0xff;
        request_cmd[6] = ((length - 1) >> 8) & 0xff;
        request_cmd[7] = ((length - 1) >> 16) & 0xff;
        request_cmd[8] = ((length - 1) >> 24) & 0xff;
        xfer.tx_buff = request_cmd;
        xfer.tx_len  = VG_SPI_SLAVE_CMD_LEN;
        xfer.rx_buff = NULL;
        xfer.rx_len  = 0;
        LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step2_a: send VG_SPI_SLAVE_RD_CMD.\r\n", 0);
        LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step2_a: Master want to send 9B cmd.\r\n", 0);

        if (0 != port_platform_bus_transfer(handle, &xfer)) {
            LOG_MUX_SPIM_E("mux_spi_master_demo_receive-Step2_a: SPI master send err, try again...status:%d \r\n", 1, 0);
            //LOG_MUX_SPIM_E("Assert!!!", 0); 
            //assert(0);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            return HOST_MUX_STATUS_ERROR;
        } else {
            LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step2_a: Send VG_SPI_SLAVE_RD_CMD. success!!!\r\n", 0);
        }

        /* Step2_b: Wait for SPI slave CR ready*/
        LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step2_b: wait slave CR done...\r\n", 0);
        if(portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY(handle, HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK,
        HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK,MAX_ERROR_NUMBER)==HOST_MUX_STATUS_ERROR)
        {
            LOG_MUX_SPIM_E("mux_spi_master_demo_receive-Step2_b: #### too many ERROR, now go to restart!!!!!\r\n", 0);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            //goto mux_spi_master_demo_receive_restart;
            return HOST_MUX_STATUS_ERROR;
        }

        /*Step2_c: Master receive <data_buffer_len> data.
                    Master receive <data_buffer_len> bytes, it's the data which Slave prepared.*/

        LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step2_c: Master want to receive:%d\r\n", 1, (int)receive_reg_value);
        uint8_t *temp_buf = (uint8_t *)port_platform_malloc((int)receive_reg_value + 1);
        if (temp_buf == NULL) {
            LOG_MUX_SPIM_E("mux_spi_master_demo_receive-Step2_C: malloc buffer fail\r\n", 0);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            return HOST_MUX_STATUS_ERROR;
        }

        request_cmd    = (uint8_t *)temp_host_mux_tx_buf;
        request_cmd[0] = SPIS_RD_CMD;

        xfer.tx_buff = request_cmd;
        xfer.tx_len  = 1;
        xfer.rx_buff = temp_buf;
        xfer.rx_len  = receive_reg_value + 1;

        if (0 != port_platform_bus_transfer(handle, &xfer)) {
            LOG_MUX_SPIM_E("mux_spi_master_demo_receive-Step2_a: SPI master send err, try again...status:%d \r\n", 1, 0);
            //LOG_MUX_SPIM_E("Assert!!!", 0); 
            //assert(0);
            port_platform_free(temp_buf);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            return HOST_MUX_STATUS_ERROR;
        } else {
            /*Step2_d:Master and check Slave_Tx done status,check HOST_MUX_SPIS_STA_RDWR_FINISH_MASK.*/
            if(portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY(handle, HOST_MUX_SPIS_STA_RDWR_FINISH_MASK,
            HOST_MUX_SPIS_STA_RDWR_FINISH_MASK,MAX_ERROR_NUMBER) == HOST_MUX_STATUS_ERROR)
            {
                LOG_MUX_SPIM_E("mux_spi_master_demo_receive-Step2_d: #### too many ERROR, now go to restart!!!!!\r\n", 0);
                port_platform_free(temp_buf);
                portable_HAL_SPI_POWER_OFF(handle);
                port_platform_give_mutex(g_host_mux_spi_master_mutex);
                //goto mux_spi_master_demo_receive_restart;
                return HOST_MUX_STATUS_ERROR;
            }
            LOG_MUX_SPIM_I("mux_spi_master_demo_receive-Step2_c: Receive Tx_buff data. success\r\n", 0);
        }
        memcpy(buf, &temp_buf[1], receive_reg_value);
        port_platform_free(temp_buf);
        *receive_done_data_len = receive_reg_value;
        port_platform_give_mutex(g_host_mux_spi_master_mutex);
    }

   /*Step3: Power OFF SPI slave*/
    {
        port_platform_take_mutex(g_host_mux_spi_master_mutex);
        portable_HAL_SPI_POWER_OFF(handle);
        port_platform_give_mutex(g_host_mux_spi_master_mutex);
    }
    return HOST_MUX_STATUS_OK;
}



/*
    This is Master write data to slave:
    Step1:Master read Slave the register of <VG_SPI_SLAVE_RX_LEN_REG_OFFSET> with the len of 4
            Step1_a: Master send Config Read (CR) cmd to slave:
                Master send to slave 9 bytes cmd: [0]:SPIS_CFG_RD_CMD, [1:4]VG_SPI_SLAVE_RX_LEN_REG_OFFSET,[5:8] len-1
            Step1_b: Wait for SPI slave CR ready
            Step1_c: Master receive the value of <VG_SPI_SLAVE_RX_LEN_REG_OFFSET>
                     Master receive 4 bytes, it's the data len which Slave prepared.
            Step1_d: Master query and check Slave_Tx done status,check HOST_MUX_SPIS_STA_RDWR_FINISH_MASK.
    Step2:Master read Slave the register of <VG_SPI_SLAVE_RX_BUF_REG_OFFSET> with the len of data_buffer_len which master want to write.
            Step2_a: Master send Config Write (CW) cmd to slave:
                Master send to slave9 bytes cmd: [0]:SPIS_CFG_RD_CMD, [1:4]VG_SPI_SLAVE_TX_BUF_REG_OFFSET,[5:8] len-1
            Step2_b: Wait for SPI slave CR ready
            Step2_c: Master send <data_buffer_len> data.
                Slave address is DEMO_SPI_SLAVE_ADDRESS_W
                Master send <data_buffer_len> bytes, it's the data which master want to write.
            Step2_d:Master and check Slave_Rx done status,check HOST_MUX_SPIS_STA_RDWR_FINISH_MASK.
*/

host_mux_status_t portable_HAL_SPI_MASTER_TX(int handle, uint8_t*buf, uint32_t *send_done_data_len)
{
    uint8_t *request_cmd;
    uint32_t receive_reg_value = 0;
    uint32_t length = 0,offset = 0;
    platform_bus_transfer_t  xfer;
    host_mux_status_t status = 0;

   /*Step0: Power ON SPI slave*/
    port_platform_take_mutex(g_host_mux_spi_master_mutex);
    portable_HAL_SPI_POWER_ON(handle);
    port_platform_give_mutex(g_host_mux_spi_master_mutex);
    request_cmd = (uint8_t *)temp_host_mux_tx_buf;

//mux_spi_master_demo_send_restart:

    /*Step1:Master read Slave the register of <VG_SPI_SLAVE_RX_LEN_REG_OFFSET> with the len of 4 */
    {
        port_platform_take_mutex(g_host_mux_spi_master_mutex);
        /*Step1_a: Master send Config Read (CR) cmd to slave:
                   Master send to slave 9 bytes cmd: [0]:SPIS_CFG_RD_CMD, [1:4]VG_SPI_SLAVE_RX_LEN_REG_OFFSET,[5:8] len-1*/
        length = 4;
        offset = VG_SPI_SLAVE_RX_LEN_REG_OFFSET;
        request_cmd[0] = SPIS_CFG_RD_CMD;
        request_cmd[1] = offset & 0xff;
        request_cmd[2] = (offset >> 8) & 0xff;
        request_cmd[3] = (offset >> 16) & 0xff;
        request_cmd[4] = (offset >> 24) & 0xff;
        request_cmd[5] = (length - 1) & 0xff;
        request_cmd[6] = ((length - 1) >> 8) & 0xff;
        request_cmd[7] = ((length - 1) >> 16) & 0xff;
        request_cmd[8] = ((length - 1) >> 24) & 0xff;
        LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step1_a: Send VG_SPI_SLAVE_CRD_CMD.\r\n", 0);
        LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step1_a: Master want to send 9B cmd.\r\n", 0);

        xfer.tx_buff = request_cmd;
        xfer.tx_len  = VG_SPI_SLAVE_CMD_LEN;
        xfer.rx_buff = NULL;
        xfer.rx_len  = 0;

        if (0 != port_platform_bus_transfer(handle, &xfer)) {
            LOG_MUX_SPIM_E("mux_spi_master_demo_send-Step1_a: SPI master send err, try again...status:%d \r\n", 1, status);
            //LOG_MUX_SPIM_E("Assert!!!", 0); 
            //assert(0);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            return HOST_MUX_STATUS_ERROR;
        } else {
            LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step1_a: Send VG_SPI_SLAVE_CRD_CMD success!!!\r\n", 0);
        }

        /* Step1_b: Wait for SPI slave CR ready*/
        LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step1_b: wait slave CR done...\r\n", 0);
        if(portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY(handle, HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK,
        HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK,MAX_ERROR_NUMBER)== HOST_MUX_STATUS_ERROR)
        {
            LOG_MUX_SPIM_E("mux_spi_master_demo_send-Step1_b: #### too many ERROR, now go to restart!!!!!\r\n", 0);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            //goto mux_spi_master_demo_send_restart;
            return HOST_MUX_STATUS_ERROR;
        }

        /*Step1_c: Master receive the value of <VG_SPI_SLAVE_RX_LEN_REG_OFFSET>
                        Master receive 4 bytes, it's the data len which Slave prepared.*/
        LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step1_c: receive SPI slave Tx_len Reg value.\r\n", 0);
        LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step1_c: Master want to receive 4B.\r\n", 0);

        memset(temp_host_mux_tx_buf, 0, sizeof(temp_host_mux_tx_buf));
        request_cmd[0] = SPIS_RD_CMD;

        xfer.tx_buff = request_cmd;
        xfer.tx_len  = 1;
        xfer.rx_buff = (uint8_t *)temp_host_mux_rx_buf;
        xfer.rx_len  = 4 + 1;

        if (0 != port_platform_bus_transfer(handle, &xfer)) {
            LOG_MUX_SPIM_E("mux_spi_master_demo_send-Step1_c: SPI master receive err, try again...status:%d\r\n", 1, status);
            //LOG_MUX_SPIM_E("Assert!!!", 0); 
            //assert(0);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            return HOST_MUX_STATUS_ERROR;
        } else {
            /*Step1_d: Master query and check Slave_Tx done status,check HOST_MUX_SPIS_STA_RDWR_FINISH_MASK.*/
            if(portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY(handle, HOST_MUX_SPIS_STA_RDWR_FINISH_MASK,
            HOST_MUX_SPIS_STA_RDWR_FINISH_MASK,MAX_ERROR_NUMBER) == HOST_MUX_STATUS_ERROR)
            {
                LOG_MUX_SPIM_E("mux_spi_master_demo_send-Step1_d: #### too many ERROR, now go to restart!!!!!\r\n", 0);
                portable_HAL_SPI_POWER_OFF(handle);
                port_platform_give_mutex(g_host_mux_spi_master_mutex);
                //goto mux_spi_master_demo_send_restart;
                return HOST_MUX_STATUS_ERROR;
            }
            receive_reg_value = temp_host_mux_rx_buf[1] | (temp_host_mux_rx_buf[2]<<8) | (temp_host_mux_rx_buf[3]<<16)|(temp_host_mux_rx_buf[4]<<24);
            LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step1_c: Receive SPI slave Rx_len Reg value:0x%x. success!!!\r\n", 1, (unsigned int)receive_reg_value);
        }

        if (receive_reg_value > *send_done_data_len) {
            LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step1_c: Receive SPI slave Rx_len Reg value:%d, but master just want to send:%d\r\n", 2, (int)receive_reg_value, (int)*send_done_data_len);
            receive_reg_value = *send_done_data_len;
        }
        if (receive_reg_value == 0) {
                *send_done_data_len = 0;
                portable_HAL_SPI_POWER_OFF(handle);
                port_platform_give_mutex(g_host_mux_spi_master_mutex);
                return HOST_MUX_STATUS_OK;
        }
        port_platform_give_mutex(g_host_mux_spi_master_mutex);
    }

    /*Step2:Master read Slave the register of <VG_SPI_SLAVE_RX_BUF_REG_OFFSET> with the len of data_buffer_len which master want to write. */
    {
        port_platform_take_mutex(g_host_mux_spi_master_mutex);

        /* Step2_a: Master send Config Write (CW) cmd to slave:
                Master send to slave9 bytes cmd: [0]:SPIS_CFG_RD_CMD, [1:4]VG_SPI_SLAVE_TX_BUF_REG_OFFSET,[5:8] len-1*/
        length = receive_reg_value;
        offset = VG_SPI_SLAVE_RX_BUF_REG_OFFSET;
        request_cmd[0] = SPIS_CFG_WR_CMD;
        request_cmd[1] = offset & 0xff;
        request_cmd[2] = (offset >> 8) & 0xff;
        request_cmd[3] = (offset >> 16) & 0xff;
        request_cmd[4] = (offset >> 24) & 0xff;
        request_cmd[5] = (length - 1) & 0xff;
        request_cmd[6] = ((length - 1) >> 8) & 0xff;
        request_cmd[7] = ((length - 1) >> 16) & 0xff;
        request_cmd[8] = ((length - 1) >> 24) & 0xff;
        LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step2_a: send VG_SPI_SLAVE_WD_CMD.\r\n", 0);
        LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step2_a:Master want to send 8B \r\n", 0);
        xfer.tx_buff = request_cmd;
        xfer.tx_len  = VG_SPI_SLAVE_CMD_LEN;
        xfer.rx_buff = NULL;
        xfer.rx_len  = 0;

        if (0 != port_platform_bus_transfer(handle, &xfer)) {
            LOG_MUX_SPIM_E("mux_spi_master_demo_send-Step2_a: SPI master send err, try again...status:%d \r\n", 1, status);
            //LOG_MUX_SPIM_E("Assert!!!", 0); 
            //assert(0);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            return HOST_MUX_STATUS_ERROR;
        } else {
            LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step2_a: Send VG_SPI_SLAVE_RD_CMD. success!!!\r\n", 0);
        }

        /* Step2_b: Wait for SPI slave CR ready*/
        LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step2_b: wait slave CR done...\r\n", 0);
        if(portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY(handle, HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK,
            HOST_MUX_SPIS_STA_TXRX_FIFO_RDY_MASK,MAX_ERROR_NUMBER)==HOST_MUX_STATUS_ERROR)
        {
            LOG_MUX_SPIM_E("mux_spi_master_demo_send-Step1_d: #### too many ERROR, now go to restart!!!!!\r\n", 0);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            //goto mux_spi_master_demo_send_restart;
            return HOST_MUX_STATUS_ERROR;
        }
        /*Step2_c: Master send <data_buffer_len> data.
            Slave address is DEMO_SPI_SLAVE_ADDRESS_W
            Master send <data_buffer_len> bytes, it's the data which master want to write.*/
        request_cmd[0] = SPIS_WR_CMD;
        memcpy(&request_cmd[1], buf, receive_reg_value);
        xfer.tx_buff   = request_cmd;
        xfer.tx_len    = receive_reg_value + 1;
        xfer.rx_buff   = NULL;
        xfer.rx_len    = 0;

        if (0 != port_platform_bus_transfer(handle, &xfer)) {
            LOG_MUX_SPIM_E("mux_spi_master_demo_send-Step2_c: send fail!!!status:%d\r\n", 1, status);
            //LOG_MUX_SPIM_E("Assert!!!", 0); 
            //assert(0);
            portable_HAL_SPI_POWER_OFF(handle);
            port_platform_give_mutex(g_host_mux_spi_master_mutex);
            return HOST_MUX_STATUS_ERROR;
        } else {
            /*Step2_d:Master and check Slave_Rx done status,check HOST_MUX_SPIS_STA_RDWR_FINISH_MASK.*/
            if(portable_HOST_SPI_MASTER_QUERY_SLAVE_STATUS_WITH_RETRY(handle, HOST_MUX_SPIS_STA_RDWR_FINISH_MASK,
            HOST_MUX_SPIS_STA_RDWR_FINISH_MASK,MAX_ERROR_NUMBER)==HOST_MUX_STATUS_ERROR)
            {
                LOG_MUX_SPIM_E("mux_spi_master_demo_receive-Step2_d: #### too many ERROR, now go to restart!!!!!\r\n", 0);
                portable_HAL_SPI_POWER_OFF(handle);
                port_platform_give_mutex(g_host_mux_spi_master_mutex);
                //goto mux_spi_master_demo_send_restart;
                return HOST_MUX_STATUS_ERROR;
            }
            LOG_MUX_SPIM_I("mux_spi_master_demo_send-Step2_c: receive Tx_buff data. success\r\n", 0);
        }
        *send_done_data_len = receive_reg_value;
        port_platform_give_mutex(g_host_mux_spi_master_mutex);
    }
    /* Step3: Power OFF SPI slave */
    {
        port_platform_take_mutex(g_host_mux_spi_master_mutex);
        portable_HAL_SPI_POWER_OFF(handle);
        port_platform_give_mutex(g_host_mux_spi_master_mutex);
    }
    return HOST_MUX_STATUS_OK;
}

