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

#ifndef __HDL_CHANNEL_PHYSICAL_H__
#define __HDL_CHANNEL_PHYSICAL_H__

#include <stdbool.h>
#include <stdint.h>

#include "hdl_config.h"

#define HDL_WRITE_TIMEOUT	(1000)

bool HDL_COM_Init();
bool HDL_COM_Deinit();

uint32_t HDL_COM_GetByte_Buffer(void *buf, uint32_t length, bool sw_flow_ctrl_open);
bool HDL_COM_PutByte_Buffer(const void *buf, uint32_t length, bool sw_flow_ctrl_open);

bool HDL_COM_SetBaudRate(uint32_t baud_rate);
bool HDL_COM_SetFlowCtrl(FlowControl fc);




/** @brief SPI master send and receive configuration structure. */
typedef struct {
    uint8_t* send_data;                               /**< Data buffer to send. */
    uint32_t send_length;                             /**< The number of bytes to send. */
    uint8_t* receive_buffer;                          /**< Received data buffer, this parameter cannot be NULL. */
    uint32_t receive_length;                          /**< The valid number of bytes received with the number of bytes to send. */
} hal_spi_master_send_and_receive_config_t;

typedef enum {
    SPI_SLAVE_STATUS_OK,
    SPI_SLAVE_STATUS_CHECK_TIMEOUT,
    SPI_SLAVE_STATUS_RDWR_ERROR,
    SPI_SLAVE_STATUS_CMD_ERROR,
} spi_slave_status_t;

typedef enum {
    HDL_SPI_MASTER_STATUS_ERROR = -2,
    HDL_SPI_MASTER_STATUS_TIMEOUT = -1,
    HDL_SPI_MASTER_STATUS_OK = 0,
} hdl_spi_master_status_t;

#endif //__HDL_CHANNEL_PHYSICAL_H__
