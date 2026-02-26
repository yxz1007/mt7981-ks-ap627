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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "location_log.h"
#include "location_serial_port.h"
#include "location_command.h"

int32_t location_command_get_ascii_checksum(int8_t *buffer, int32_t buffer_len)
{
    int8_t *ind;
    uint8_t checkSumL = 0, checkSumR;
    int32_t checksum = 0;

    ind = buffer;
    while (ind - buffer < buffer_len) {
        checkSumL ^= *ind;
        ind++;
    }

    checkSumR = checkSumL & 0x0F;
    checkSumL = (checkSumL >> 4) & 0x0F;
    checksum = checkSumL * 16;
    checksum = checksum + checkSumR;
    return checksum;
}

void location_command_get_param(uint8_t *command, int32_t length, location_command_contx *contx)
{
    int32_t i, j, k;
    int32_t checksum_l, checksum_r;

    for (i = 5, j = 0, k = 0; i < length; i++) {
        if ((command[i] == ',') || (command[i] == '*')) {
            contx->param[j][k] = 0;
            j++;
            k = 0;
        } else {
            contx->param[j][k] = command[i];
            k++;
        }
    }

    contx->param[j][k] = 0;
    contx->param_num = j;

    checksum_l = contx->param[j][0] >= 'A' ? contx->param[j][0] - 'A' + 10 : contx->param[j][0] - '0';
    checksum_r = contx->param[j][1] >= 'A' ? contx->param[j][1] - 'A' + 10 : contx->param[j][1] - '0';

    contx->checksum = checksum_l * 16 + checksum_r;
}

int location_command_send(char *cmd_buf)
{
    int32_t checksum = 0;
    char temp_buffer[352] = {0};
    char command[359] = {0};
    int ret;

    ret = snprintf(temp_buffer, sizeof(temp_buffer), "PAIR%s", cmd_buf);
    if (ret < 0) {
        LOC_SERV_LOGE("location_command_send get temp_buffer fail", 0);
        return -1;
    }

    checksum = location_command_get_ascii_checksum((int8_t*)temp_buffer, strlen(temp_buffer));

    ret = snprintf(command, sizeof(command), "$%s*%02X\r\n", temp_buffer, (int)checksum);
    if (ret < 0) {
        LOC_SERV_LOGE("location_command_send get command buffer fail", 0);
        return -1;
    }
    LOC_SERV_LOGI("location_command_send, %s", 1, command);
    if (location_serial_port_gnss_write((uint8_t *)command, strlen(command)) > 0) {
        return 0;
    }
    return -1;
}