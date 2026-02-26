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
#include <sys/msg.h>
#include <pthread.h>
#include <stdlib.h>
#include "location_log.h"
#include "location_debug.h"
#include "location_config.h"
#include "location_serial_port.h"

#define LOCATION_DEBUG_TO_POWERGPS

#define LOCATION_DEBUG_MSG_KEY    (1235)

typedef struct {
    location_debug_config_t debug_config;
    FILE *log_file;
    int msgid;
    pthread_t tid_tx;
    #if defined(LOCATION_DEBUG_TO_POWERGPS)
    pthread_t tid_rx;
    #endif /* defined(LOCATION_DEBUG_TO_POWERGPS) */
} location_debug_struct_t;

typedef struct {
    void *param;
    int param_length;
} location_debug_message_struct_t;

typedef struct {
    long int msg_type;
    location_debug_message_struct_t data;
} debug_msg_struct_t;

static location_debug_struct_t g_location_debug_cnxt = {
    LOCATION_DEBUG_DISABLE,
    NULL,
    -1,
    0,
    #if defined(LOCATION_DEBUG_TO_POWERGPS)
    0,
    #endif /* defined(LOCATION_DEBUG_TO_POWERGPS) */
};


void location_debug_handle(unsigned char *buffer, int data_length)
{
    if (g_location_debug_cnxt.debug_config == LOCATION_DEBUG_DISABLE) {
        return;
    }
    if (g_location_debug_cnxt.debug_config == LOCATION_DEBUG_SAVE_TO_FILE) {
        if (g_location_debug_cnxt.log_file == NULL) {
            return;
        }
        if (fwrite(buffer, 1, data_length,g_location_debug_cnxt.log_file) != data_length) {
            LOC_SERV_LOGE("[debug] write to file fail", 0);
        }
        LOC_SERV_LOGI("[debug]len:%d", 1, data_length);
        return;
    }
    if (g_location_debug_cnxt.debug_config == LOCATION_DEBUG_SEND_BY_PORT) {
        debug_msg_struct_t item;
        unsigned char *buf = (unsigned char *)malloc(data_length);
        if (buf == NULL) {
            LOC_SERV_LOGE("[debug] location_debug_handle, malloc fail", 0);
            return;
        }
        memcpy(buf, buffer, data_length);
        item.msg_type = 1;
        item.data.param = buf;
        item.data.param_length = data_length;
        if (msgsnd(g_location_debug_cnxt.msgid, &item, sizeof(item.data), 0) == -1) {
            LOC_SERV_LOGE("[debug]location_debug_handle, send Fail!", 0);
        }
        return;
    }
}

void *location_debug_tx_task(void *arg)
{
    LOC_SERV_LOGI("[debug]location_debug_tx_task", 0);
    debug_msg_struct_t item;
    while(1) {
        if (msgrcv(g_location_debug_cnxt.msgid, &item, sizeof(item.data), 0, 0) != -1) {
            location_serial_port_debug_write((unsigned char *)item.data.param, item.data.param_length);
            LOC_SERV_LOGI("[debug]len:%d", 1, item.data.param_length);
            free(item.data.param);
        }
    }
}

#if defined(LOCATION_DEBUG_TO_POWERGPS)
void *location_debug_rx_task(void *arg)
{
    int len = 0;
    LOC_SERV_LOGI("[debug]location_debug_rx_task", 0);
    unsigned char buf[256];
    memset(buf, 0, sizeof(buf));
    while(1) {
        len = location_serial_port_debug_read(buf, sizeof(buf));
        if (len > 0) {
            location_serial_port_gnss_write(buf, len);
        }
    }
}
#endif /* defined(LOCATION_DEBUG_TO_POWERGPS) */

int location_debug_start()
{
    if (g_location_debug_cnxt.debug_config == LOCATION_DEBUG_SAVE_TO_FILE) {
        if (g_location_debug_cnxt.log_file != NULL) {
            LOC_SERV_LOGI("[debug]location_debug_start, already open", 0);
            return 0;
        }
        char path[LOCATION_CONFIG_BUF_LENGTH] = {0};
        location_config_get_log_path(path);
        g_location_debug_cnxt.log_file = fopen(path, "ab");
        if (!g_location_debug_cnxt.log_file) {
            LOC_SERV_LOGE("[debug]location_debug_start fail, %s", 1, path);
            return -1;
        }
    }
    if (g_location_debug_cnxt.debug_config == LOCATION_DEBUG_SEND_BY_PORT) {
        int ret;
        if (location_serial_port_debug_init() < 0) {
            return -1;
        }
        if (g_location_debug_cnxt.msgid == -1) {
            ret = msgget((key_t)LOCATION_DEBUG_MSG_KEY, IPC_EXCL);
            if (ret != -1) {
                LOC_SERV_LOGI("[debug]location_debug_start, msgid exist:%d", 1, ret);
                g_location_debug_cnxt.msgid = ret;
            } else {
                g_location_debug_cnxt.msgid = msgget((key_t)LOCATION_DEBUG_MSG_KEY, 0666 | IPC_CREAT | IPC_EXCL);
                if (g_location_debug_cnxt.msgid == -1) {
                    LOC_SERV_LOGE("[debug]location_debug_start, msgid fail", 0);
                    return -1;
                }
                LOC_SERV_LOGI("[debug]location_debug_start, creat msg:%d", 1, g_location_debug_cnxt.msgid);
            }
        } else {
            LOC_SERV_LOGI("[debug]location_debug_start, msgid exist", 0);
        }

        if (g_location_debug_cnxt.tid_tx != 0) {
            LOC_SERV_LOGI("[debug]location_debug_start, tx tid exist", 0);
        } else {
            if (pthread_create(&g_location_debug_cnxt.tid_tx, NULL, location_debug_tx_task, NULL) != 0) {
                LOC_SERV_LOGE("[debug]location_debug_start, tx thread create fail", 0);
                return -1;
            }
        }
        #if defined(LOCATION_DEBUG_TO_POWERGPS)
        if (g_location_debug_cnxt.tid_rx != 0) {
            LOC_SERV_LOGI("[debug]location_debug_start, rx tid exist", 0);
            return 0;
        }
        if (pthread_create(&g_location_debug_cnxt.tid_rx, NULL, location_debug_rx_task, NULL) != 0) {
            LOC_SERV_LOGE("[debug]location_debug_start, rx thread create fail", 0);
            return -1;
        }
        return 0;
        #endif /* defined(LOCATION_DEBUG_TO_POWERGPS) */
    }
    return 0;
}

void location_debug_disable()
{
    location_debug_close();
    g_location_debug_cnxt.debug_config = LOCATION_DEBUG_DISABLE;
}

int location_debug_open(location_debug_config_t config)
{
    if (config == g_location_debug_cnxt.debug_config) {
        return 0;
    }
    location_debug_close();
    g_location_debug_cnxt.debug_config = config;
    return location_debug_start();
}

void location_debug_close()
{
    if (g_location_debug_cnxt.debug_config == LOCATION_DEBUG_SAVE_TO_FILE) {
        if (g_location_debug_cnxt.log_file != NULL) {
            if (fclose(g_location_debug_cnxt.log_file) == 0) {
                g_location_debug_cnxt.log_file = NULL;
            }
        }
    } else if (g_location_debug_cnxt.debug_config == LOCATION_DEBUG_SEND_BY_PORT) {
        location_serial_port_debug_deinit();
        if (g_location_debug_cnxt.tid_tx != 0) {
            if (pthread_cancel(g_location_debug_cnxt.tid_tx) != 0) {
                LOC_SERV_LOGE("[IO]location_debug_close, fail", 0);
            } else {
                pthread_join(g_location_debug_cnxt.tid_tx, NULL);
                g_location_debug_cnxt.tid_tx = 0;
            }
        }
        if (g_location_debug_cnxt.msgid != -1) {
            msgctl(g_location_debug_cnxt.msgid, IPC_RMID, NULL);
            g_location_debug_cnxt.msgid = -1;
        }
        #if defined(LOCATION_DEBUG_TO_POWERGPS)
        if (g_location_debug_cnxt.tid_rx != 0) {
            if (pthread_cancel(g_location_debug_cnxt.tid_rx) != 0) {
                LOC_SERV_LOGE("[IO]location_debug_close, fail", 0);
            } else {
                pthread_join(g_location_debug_cnxt.tid_rx, NULL);
                g_location_debug_cnxt.tid_rx = 0;
            }
        }
        #endif /* defined(LOCATION_DEBUG_TO_POWERGPS) */
    }
}
