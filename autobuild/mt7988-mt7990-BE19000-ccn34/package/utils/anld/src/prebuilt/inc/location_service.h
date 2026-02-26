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

#ifndef LOCATION_SERVICE_H
#define LOCATION_SERVICE_H
#include <stdint.h>

typedef enum {
    LOC_SERVICE_MSG_START,
    LOC_SERVICE_MSG_STOP,
    LOC_SERVICE_MSG_DOWNLOAD_FIRMWARE,
    LOC_SERVICE_MSG_SAVE_LOCATION,
    LOC_SERVICE_MSG_DEBUG_DISABLE,
    LOC_SERVICE_MSG_DEBUG_SAVE_TO_FILE,
    LOC_SERVICE_MSG_DEBUG_SEND_BY_PORT,
    LOC_SERVICE_MSG_SEND_CMD,
    LOC_SERVICE_MSG_TIME_VALID,
    LOC_SERVICE_MSG_EPO_DOWNLOAD,
    LOC_SERVICE_MSG_RECEIVE_CUST_DATA,
    LOC_SERVICE_MSG_RECEIVE_NMEA,
    LOC_SERVICE_MSG_RECEIVE_CMD_RSP,
    LOC_SERVICE_MSG_RECEIVE_BINARY,
    LOC_SERVICE_MSG_LOCATION_INFO,
} location_service_message_id;

typedef void (*location_service_callback_t)(location_service_message_id type, int state, char *buf, int length);

void location_service_task_send_message(int32_t message_id, void *param, int32_t param_length);
void location_service_init(location_service_callback_t callback_function);
#endif /*LOCATION_SERVICE_H*/