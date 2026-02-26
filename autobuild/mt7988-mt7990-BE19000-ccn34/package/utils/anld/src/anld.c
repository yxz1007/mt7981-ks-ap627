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
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "location_log.h"
#include "location_service.h"


#define ANLD_BUF_LEN    (512)

#define SERVER_SUN_PATH   "/etc/default/anld/anld_sun_path"

typedef enum {
    ANLD_CTRL_SEND_MSG_TYPE_START,
    ANLD_CTRL_SEND_MSG_TYPE_STOP,
    ANLD_CTRL_SEND_MSG_TYPE_UPGRADE,
    ANLD_CTRL_SEND_MSG_TYPE_SAVE_LOCATION,
    ANLD_CTRL_SEND_MSG_TYPE_DEBUG_DISABLE,
    ANLD_CTRL_SEND_MSG_TYPE_DEBUG_SAVE_TO_FILE,
    ANLD_CTRL_SEND_MSG_TYPE_DEBUG_SEND_BY_PORT,
    ANLD_CTRL_SEND_MSG_TYPE_PAIR_COMMAND,
    ANLD_CTRL_SEND_MSG_TYPE_TIME_VALID,
} anld_ctrl_send_message_type_t;

typedef struct {
    anld_ctrl_send_message_type_t type;
    char buf[ANLD_BUF_LEN];
} anld_ctrl_send_message_struct_t;

typedef enum {
    ANLD_RESPONSE_MSG_TYPE_START,
    ANLD_RESPONSE_MSG_TYPE_STOP,
    ANLD_RESPONSE_MSG_TYPE_UPGRADE,
    ANLD_RESPONSE_MSG_TYPE_SAVE_LOCATION,
    ANLD_RESPONSE_MSG_TYPE_DEBUG_CONFIG,
    ANLD_RESPONSE_MSG_TYPE_PAIR_COMMAND,
    ANLD_RESPONSE_MSG_TYPE_NMEA,
    ANLD_RESPONSE_MSG_TYPE_TIME_VALID,
    ANLD_RESPONSE_MSG_TYPE_EPO_DOWNLOAD,
    ANLD_RESPONSE_MSG_TYPE_LOCATION,
} anld_response_msg_type_t;

typedef struct {
    anld_response_msg_type_t type;
    int state;
    char buf[ANLD_BUF_LEN];
} anld_ctrl_response_message_struct_t;


typedef struct {
    int fd;
    struct sockaddr_un client_addr;
    socklen_t client_addr_len;
} anld_cntx_t;

static anld_cntx_t g_anld_cntx; 

void anld_send_response(location_service_message_id type, int state, char *buf, int length)
{
    anld_ctrl_response_message_struct_t message;
    switch (type) {
        case LOC_SERVICE_MSG_START: {
            message.type = ANLD_RESPONSE_MSG_TYPE_START;
            break;
        }
        case LOC_SERVICE_MSG_STOP: {
            message.type = ANLD_RESPONSE_MSG_TYPE_STOP;
            break;
        }
        case LOC_SERVICE_MSG_DOWNLOAD_FIRMWARE: {
            message.type = ANLD_RESPONSE_MSG_TYPE_UPGRADE;
            break;
        }
        case LOC_SERVICE_MSG_SAVE_LOCATION: {
            message.type = ANLD_RESPONSE_MSG_TYPE_SAVE_LOCATION;
            break;
        }
        case LOC_SERVICE_MSG_DEBUG_DISABLE:
        case LOC_SERVICE_MSG_DEBUG_SAVE_TO_FILE: 
        case LOC_SERVICE_MSG_DEBUG_SEND_BY_PORT: {
            message.type = ANLD_RESPONSE_MSG_TYPE_DEBUG_CONFIG;
            break;
        }
        case LOC_SERVICE_MSG_SEND_CMD: {
            message.type = ANLD_RESPONSE_MSG_TYPE_PAIR_COMMAND;
            break;
        }
        case LOC_SERVICE_MSG_TIME_VALID: {
            message.type = ANLD_RESPONSE_MSG_TYPE_TIME_VALID;
            break;
        }
        case LOC_SERVICE_MSG_EPO_DOWNLOAD: {
            message.type = ANLD_RESPONSE_MSG_TYPE_EPO_DOWNLOAD;
            break;
        }
        case LOC_SERVICE_MSG_RECEIVE_NMEA: {
            message.type = ANLD_RESPONSE_MSG_TYPE_NMEA;
            break;
        }
        case LOC_SERVICE_MSG_LOCATION_INFO: {
            message.type = ANLD_RESPONSE_MSG_TYPE_LOCATION;
            break;
        }
        default: {
            return;
        }
    }
    message.state = state;
    memset(message.buf, 0, sizeof(message.buf));
    if (buf != NULL) {
        memcpy(message.buf, buf, length > sizeof(message.buf) ? sizeof(message.buf) : length);
    }
    if (sendto(g_anld_cntx.fd, &message, sizeof(message), 0, (struct sockaddr *)&g_anld_cntx.client_addr, g_anld_cntx.client_addr_len) == -1) {
        LOC_SERV_LOGE("anld_send_response, sendto error", 0);
    }
}

int anld_init()
{
    struct sockaddr_un server_addr;
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) {
        LOC_SERV_LOGE("anld_init, socket error", 0);
        return -1;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    int ret = snprintf(server_addr.sun_path, sizeof(server_addr.sun_path), SERVER_SUN_PATH);
    if (ret < 0) {
        LOC_SERV_LOGE("anld_init get sun_path fail", 0);
    }
    unlink(SERVER_SUN_PATH);
    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        LOC_SERV_LOGE("anld_init, bind error", 0);
        close(fd);
        return -1;
    }
    return fd;
}

int main()
{
    anld_ctrl_send_message_struct_t message;
    g_anld_cntx.fd = anld_init();
    if (g_anld_cntx.fd < 0) {
        return -1;
    }
    memset(&g_anld_cntx.client_addr, 0, sizeof(g_anld_cntx.client_addr));
    g_anld_cntx.client_addr_len = sizeof(g_anld_cntx.client_addr);
    location_service_init(anld_send_response);
    while (1) {
        if (recvfrom(g_anld_cntx.fd, &message, sizeof(message), 0, (struct sockaddr *)&g_anld_cntx.client_addr, &g_anld_cntx.client_addr_len) == -1) {
            LOC_SERV_LOGE("recv error", 0);
            close(g_anld_cntx.fd);
            return -1;
        }

        switch (message.type) {
            case ANLD_CTRL_SEND_MSG_TYPE_START: 
                location_service_task_send_message(LOC_SERVICE_MSG_START, NULL, 0);
                break;
            case ANLD_CTRL_SEND_MSG_TYPE_STOP: 
                location_service_task_send_message(LOC_SERVICE_MSG_STOP, NULL, 0);
                break;
            case ANLD_CTRL_SEND_MSG_TYPE_UPGRADE:
                location_service_task_send_message(LOC_SERVICE_MSG_DOWNLOAD_FIRMWARE, NULL, 0);
                break;
            case ANLD_CTRL_SEND_MSG_TYPE_SAVE_LOCATION:
                location_service_task_send_message(LOC_SERVICE_MSG_SAVE_LOCATION, NULL, 0);
                break;
            case ANLD_CTRL_SEND_MSG_TYPE_DEBUG_DISABLE:
                location_service_task_send_message(LOC_SERVICE_MSG_DEBUG_DISABLE, NULL, 0);
                break;
            case ANLD_CTRL_SEND_MSG_TYPE_DEBUG_SAVE_TO_FILE:
                location_service_task_send_message(LOC_SERVICE_MSG_DEBUG_SAVE_TO_FILE, NULL, 0);
                break;
            case ANLD_CTRL_SEND_MSG_TYPE_DEBUG_SEND_BY_PORT:
                location_service_task_send_message(LOC_SERVICE_MSG_DEBUG_SEND_BY_PORT, NULL, 0);
                break;
            case ANLD_CTRL_SEND_MSG_TYPE_PAIR_COMMAND: {
                message.buf[ANLD_BUF_LEN - 1] = 0;
                int length = strlen(message.buf);
                char *buf = (char *)malloc(length + 1);
                if (buf == NULL) {
                    LOC_SERV_LOGE("[anld]cmd buf, malloc fail", 0);
                    break;
                }
                memcpy(buf, message.buf, length);
                buf[length] = 0;
                location_service_task_send_message(LOC_SERVICE_MSG_SEND_CMD, buf, length);
                break;
            }
            case ANLD_CTRL_SEND_MSG_TYPE_TIME_VALID:
                location_service_task_send_message(LOC_SERVICE_MSG_TIME_VALID, NULL, 0);
                break;
            default: {
                LOC_SERV_LOGE("[anld]unkown message type:%d", 1, message.type);
                break;
            }
        }
    }
    close(g_anld_cntx.fd);
    return 1;
}

