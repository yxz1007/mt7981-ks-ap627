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

#ifndef _HDL_DA_CMD_H_
#define _HDL_DA_CMD_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define FLASH_ID_LEN				(3)
#define FW_PACKET_LEN 				(4096)
#define DA_SEND_PACKET_LEN 			FW_PACKET_LEN
#define DA_RECV_PACKET_LEN 			(2048) //DA_SEND_PACKET_LEN

typedef struct
{
	const char image_name[256];
    uint32_t begin_address;
} image_info_t;

typedef struct
{
    uint32_t flash_base_addr;
    uint32_t flash_size;
	uint8_t flash_id[FLASH_ID_LEN];
} hdl_da_report_t;

typedef enum
{
    E_DONE,
    E_GET_RACE_TIMEOUT,
    E_RACE_ID_ERROR,
    E_DA_RES_ERROR,
} RES_ERROR_CODE;

// Public API
bool hdl_sync_with_da(hdl_da_report_t *da_report);
bool hdl_da_format(const uint32_t format_flash_addr, const uint32_t format_len);
bool download_image(const image_info_t *image_info);
bool check_image_crc(const image_info_t* image_info);
bool hdl_da_readback(const char* host_file_path, const uint32_t flash_addr, const uint32_t len);
bool hdl_da_read_otp(const char* host_file_path, const uint32_t otp_addr, const uint32_t len);
bool hdl_finish_race(bool enable);

int hdl_dfu_start_race(uint8_t flag, uint8_t* response);
int hdl_dfu_reset_race(uint8_t flag, uint8_t* response);
int hdl_flash_address_race(uint32_t* base_addr);
int hdl_flash_size_race(uint32_t* size);
int hdl_flash_id_race(uint8_t* flash_id);

#endif //_HDL_DA_CMD_H_
