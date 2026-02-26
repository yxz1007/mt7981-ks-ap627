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

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "location_service.h"
#include "location_log.h"
#include "location_serial_port.h"
#include "location_command.h"
#include "location_debug.h"

#define LOCATION_IO_RX_BUFFER_SIZE (15*1024)

static pthread_t g_io_tid = 0;
static uint8_t g_io_rx_buffer[LOCATION_IO_RX_BUFFER_SIZE];


#define LOCATION_IO_CMD_START   (0x24)
#define LOCATION_IO_CMD_STAR    (0x2A)
#define LOCATION_IO_CMD_END1    (0x0D)
#define LOCATION_IO_CMD_END2    (0x0A)
#define LOCATION_IO_DATA_START1 (0x04)
#define LOCATION_IO_DATA_START2 (0x24)
#define LOCATION_IO_DATA_END1   (0xAA)
#define LOCATION_IO_DATA_END2   (0x44)

#define LOCATION_COMMAND_TALK_ID           "PAIR"
#define LOCATION_NMEA_GPS_TALK_ID          "GP"
#define LOCATION_NMEA_GLO_TALK_ID          "GL"
#define LOCATION_NMEA_GAL_TALK_ID          "GA"
#define LOCATION_NMEA_BDS_TALK_ID          "GB"
#define LOCATION_NMEA_NIC_TALK_ID          "GI"
#define LOCATION_NMEA_MULTI_GNSS_TALK_ID   "GN"


typedef enum {
    LOCATION_IO_DATA_PARSE_NONE,
    LOCATION_IO_DATA_PARSE_CMD_NMEA_START,
    LOCATION_IO_DATA_PARSE_BINARY_START,
} location_io_data_parse_t;

typedef enum {
    LOCATION_IO_DATA_TYPE_NONE,
    LOCATION_IO_DATA_TYPE_NMEA,
    LOCATION_IO_DATA_TYPE_CMD_RSP,
    LOCATION_IO_DATA_TYPE_BINARY,
} location_io_data_type_t;

void location_io_data_handle(location_io_data_type_t type, uint8_t *buffer, int32_t length)
{
    int32_t message_id;
    if (length <= 0) {
        return;
    }

    uint8_t *buf = (uint8_t *)malloc(length + 1);
    if (buf == NULL) {
        LOC_SERV_LOGE("[IO] location_io_data_handle, malloc fail", 0);
        return;
    }
    memcpy(buf, buffer, length);
    buf[length] = 0;

    switch (type) {
        case LOCATION_IO_DATA_TYPE_NONE: {
            message_id = LOC_SERVICE_MSG_RECEIVE_CUST_DATA;
            break;
        }
        case LOCATION_IO_DATA_TYPE_NMEA: {
            message_id = LOC_SERVICE_MSG_RECEIVE_NMEA;
            break;
        }
        case LOCATION_IO_DATA_TYPE_CMD_RSP: {
            message_id = LOC_SERVICE_MSG_RECEIVE_CMD_RSP;
            break;
        }
        case LOCATION_IO_DATA_TYPE_BINARY: {
            message_id = LOC_SERVICE_MSG_RECEIVE_BINARY;
            break;
        }
        default:
            free(buf);
            return;
    }
    location_service_task_send_message(message_id, buf, length);
}


uint8_t location_io_get_binary_checksum(uint8_t *buffer, int32_t buffer_len)
{
    uint8_t checksum = 0;
    uint8_t *pheader = NULL;
    uint16_t i;

    if (NULL == buffer) {
        return 0;
    }
    pheader = buffer;
    for (i = 0; i < buffer_len; i++) {
        checksum ^= *pheader;
        pheader++;
    }
    return checksum;
}

uint32_t location_io_parse_data(uint8_t *buffer, uint32_t length, location_io_data_type_t *type)
{
    uint32_t i;
    location_io_data_parse_t status = LOCATION_IO_DATA_PARSE_NONE;
    *type = LOCATION_IO_DATA_TYPE_NONE;
    for (i = 0; i < length; i++) {
        if ((buffer[i] == LOCATION_IO_DATA_START1)
            && ((i + 1) < length)
            && (buffer[i + 1] == LOCATION_IO_DATA_START2)) {
            if (i > 0) {
                return i;
            }
            status = LOCATION_IO_DATA_PARSE_BINARY_START;
            break;
        } else if (buffer[i] == LOCATION_IO_CMD_START) {
            if (i > 0) {
                return i;
            }
            status = LOCATION_IO_DATA_PARSE_CMD_NMEA_START;
            break;
        }
    }    
    if (status == LOCATION_IO_DATA_PARSE_BINARY_START) {
        int32_t data_length;
        uint8_t checksum_i;
        uint8_t checksum_o;
        if (length < 9) {
            return 0;
        }  
        data_length = *(uint16_t *)&buffer[4];
        if (length < (data_length + 9)) {
            return 0;
        }

        if ((buffer[data_length + 7] != LOCATION_IO_DATA_END1)
            || (buffer[data_length + 8] != LOCATION_IO_DATA_END2)) {
            return 2;
        }
        
        checksum_i = buffer[data_length + 6];
        checksum_o = location_io_get_binary_checksum(buffer + 2, data_length + 4);
        if (checksum_i != checksum_o) {
            return data_length + 9;
        }
        *type = LOCATION_IO_DATA_TYPE_BINARY;
        return data_length + 9;
    }
    if (status == LOCATION_IO_DATA_PARSE_CMD_NMEA_START) {
        if (length < 11) {
            return 0;
        }
        if (strncmp((char *)buffer + 1, LOCATION_COMMAND_TALK_ID, 4) == 0) {
            *type = LOCATION_IO_DATA_TYPE_CMD_RSP;
        } else if ((strncmp((char *)buffer + 1, LOCATION_NMEA_GPS_TALK_ID, 2) == 0)
                    || (strncmp((char *)buffer + 1, LOCATION_NMEA_GLO_TALK_ID, 2) == 0)
                    || (strncmp((char *)buffer + 1, LOCATION_NMEA_BDS_TALK_ID, 2) == 0)
                    || (strncmp((char *)buffer + 1, LOCATION_NMEA_GAL_TALK_ID, 2) == 0)
                    || (strncmp((char *)buffer + 1, LOCATION_NMEA_NIC_TALK_ID, 2) == 0)
                    || (strncmp((char *)buffer + 1, LOCATION_NMEA_MULTI_GNSS_TALK_ID, 2) == 0)) {
            *type = LOCATION_IO_DATA_TYPE_NMEA;
        } else {
            return 1;
        }
        for (i = 0; i < length; i++) {
            if (buffer[i] == LOCATION_IO_CMD_STAR) {
                int32_t checksum1, checksum2, checksumd;
                if ((i + 4 + 1) > length) {
                    *type = LOCATION_IO_DATA_TYPE_NONE;
                    return 0;
                }
                if ((buffer[i + 3] != LOCATION_IO_CMD_END1) || (buffer[i + 4] != LOCATION_IO_CMD_END2)
                    || !((buffer[i + 1] >= '0' && buffer[i + 1] <= '9') || (buffer[i + 1] >= 'A' && buffer[i + 1] <= 'F'))
                    || !((buffer[i + 2] >= '0' && buffer[i + 2] <= '9') || (buffer[i + 2] >= 'A' && buffer[i + 2] <= 'F'))) {
                    *type = LOCATION_IO_DATA_TYPE_NONE;
                    return 1;
                }
                checksum1 = buffer[i + 1] >= 'A' ? buffer[i + 1] - 'A' + 10 : buffer[i + 1] - '0';
                checksum2 = buffer[i + 2] >= 'A' ? buffer[i + 2] - 'A' + 10 : buffer[i + 2] - '0';
                checksumd = location_command_get_ascii_checksum((int8_t *)buffer + 1, i - 1);
                if (checksumd != ((checksum1 * 16) + checksum2)) {
                    *type = LOCATION_IO_DATA_TYPE_NONE;
                    return i + 4 + 1;
                }
                return i + 4 + 1;
            }
        }
        *type = LOCATION_IO_DATA_TYPE_NONE;
        return 0;
    }
    if ((status == LOCATION_IO_DATA_PARSE_NONE) && (length < 2)) {
        return 0;
    }

    return length;
}

void *location_io_rx_task(void *arg)
{
    int read_len = 0;
    int offset = 0;
    int parse_len = 0;
    location_io_data_type_t type;
    LOC_SERV_LOGI("[IO]location_io_rx_task", 0);

    if (location_serial_port_gnss_init() < 0) {
        return NULL;
    }
    memset(g_io_rx_buffer, 0, LOCATION_IO_RX_BUFFER_SIZE);
    while(1) {
        read_len = location_serial_port_gnss_read(g_io_rx_buffer + offset, LOCATION_IO_RX_BUFFER_SIZE - offset);
        if (read_len > 0) {
            location_debug_handle(g_io_rx_buffer + offset, read_len);
            offset += read_len;
            if (offset > LOCATION_IO_RX_BUFFER_SIZE) {
                LOC_SERV_LOGE("[IO] location_io_rx_task, read len wrong:%d, %d", 2, read_len, offset);
                offset = LOCATION_IO_RX_BUFFER_SIZE;
            }
            while(1) {
                parse_len = location_io_parse_data(g_io_rx_buffer, offset, &type);
                if (parse_len == 0) {
                    if (offset >= LOCATION_IO_RX_BUFFER_SIZE) {
                        type = LOCATION_IO_DATA_TYPE_NONE;
                        parse_len = LOCATION_IO_RX_BUFFER_SIZE;
                    } else {
                        break;
                    }
                }
                location_io_data_handle(type, g_io_rx_buffer, parse_len);
                if (parse_len != LOCATION_IO_RX_BUFFER_SIZE) {
                    memmove(g_io_rx_buffer, g_io_rx_buffer + parse_len, offset - parse_len);
                }
                offset -= parse_len;
            }
        } else {
            usleep(50 * 1000);
        }
    }
}

int location_io_thread_init()
{
    int ret;

    if (g_io_tid != 0) {
        LOC_SERV_LOGI("[IO]location_io_thread_init, tid exist", 0);
        return 0;
    }
    ret = pthread_create(&g_io_tid, NULL, location_io_rx_task, NULL);
    if (ret != 0) {
        LOC_SERV_LOGE("[IO]location_io_thread_init, thread create fail", 0);
        return -1;
    }
    return 0;
}

int location_io_thread_deinit()
{
    int ret = 0;
    
    if (location_serial_port_gnss_deinit() != 0) {
        ret = -1;
    }
    if (g_io_tid != 0) {
        if (pthread_cancel(g_io_tid) != 0) {
            LOC_SERV_LOGE("[IO]location_io_thread_deinit, fail", 0);
            ret = -1;
        } else {
            pthread_join(g_io_tid, NULL);
            g_io_tid = 0;
        }
    }
    return ret;
}