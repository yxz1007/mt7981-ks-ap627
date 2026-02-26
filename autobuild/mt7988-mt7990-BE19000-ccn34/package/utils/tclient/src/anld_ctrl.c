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
#include <fcntl.h>      /* File control definitions */
#include <unistd.h>     /* UNIX standard function definitions */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "anld_ctrl.h"

#define SERVER_SUN_PATH   "/etc/default/anld/anld_sun_path"
#define CLIENT_SUN_PATH   "/etc/default/anld/client_sun_path"

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
} anld_ctrl_send_message_type;

typedef struct {
    anld_ctrl_send_message_type type;
    char buf[ANLD_BUF_LEN];
} anld_ctrl_send_message_struct_t;

typedef struct {
    pthread_t tid;
    int fd;
    anld_ctrl_callback_t callback_func;
    struct sockaddr_un addr;
    socklen_t len;
} anld_ctrl_cntx_t;

static anld_ctrl_cntx_t anld_ctrl_cntx;

void *anld_ctrl_rx_fun(void *arg)
{
    anld_ctrl_response_message_struct_t message;
    while(1) {
        if (-1 == recvfrom(anld_ctrl_cntx.fd, &message, sizeof(message), 0, (struct sockaddr *)&(anld_ctrl_cntx.addr), &(anld_ctrl_cntx.len))) {
            printf("anld_ctrl_rx_fun, recv error\n");
            return NULL;
        }
        anld_ctrl_cntx.callback_func(&message);
    }
}

int anld_ctrl_send(anld_ctrl_send_message_type type, char *buf)
{
    anld_ctrl_send_message_struct_t message;
    int ret;
    message.type = type;
    memset(message.buf, 0, sizeof(message.buf));
    if (buf != NULL) {
        ret = snprintf(message.buf, sizeof(message.buf), "%s", buf);
        if (ret < 0) {
            printf("anld_ctrl_send get buf fail\n");
        }
    }
    return sendto(anld_ctrl_cntx.fd, &message, sizeof(message), 0, (struct sockaddr *)&anld_ctrl_cntx.addr, anld_ctrl_cntx.len);
}

int anld_ctrl_start()
{
    return anld_ctrl_send(ANLD_CTRL_SEND_MSG_TYPE_START, NULL);
}

int anld_ctrl_stop()
{
    return anld_ctrl_send(ANLD_CTRL_SEND_MSG_TYPE_STOP, NULL);
}

int anld_ctrl_upgrade()
{
    return anld_ctrl_send(ANLD_CTRL_SEND_MSG_TYPE_UPGRADE, NULL);
}

int anld_ctrl_save_location()
{
    return anld_ctrl_send(ANLD_CTRL_SEND_MSG_TYPE_SAVE_LOCATION, NULL);
}

int anld_ctrl_debug_config(anld_ctrl_debug_config_t type)
{
    if (type == ANLD_CTRL_DEBUG_CONFIG_DISABLE) {
        return anld_ctrl_send(ANLD_CTRL_SEND_MSG_TYPE_DEBUG_DISABLE, NULL);
    }
    if (type == ANLD_CTRL_DEBUG_CONFIG_SAVE_TO_FILE) {
        return anld_ctrl_send(ANLD_CTRL_SEND_MSG_TYPE_DEBUG_SAVE_TO_FILE, NULL);
    }
    if (type == ANLD_CTRL_DEBUG_CONFIG_SEND_BY_PORT) {
        return anld_ctrl_send(ANLD_CTRL_SEND_MSG_TYPE_DEBUG_SEND_BY_PORT, NULL);
    }
    printf("anld_ctrl_debug_config, type:%d\n", type);
    return -1;
}

int anld_ctrl_send_pair_command(char *buf)
{
    return anld_ctrl_send(ANLD_CTRL_SEND_MSG_TYPE_PAIR_COMMAND, buf);
}

int anld_ctrl_send_time_valid()
{
    return anld_ctrl_send(ANLD_CTRL_SEND_MSG_TYPE_TIME_VALID, NULL);
}

int anld_ctrl_init(anld_ctrl_callback_t callback_function)
{
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (-1 == fd) {
        printf("anld_ctrl_init, socket error\n");
        return -1;
    }
    struct sockaddr_un client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sun_family = AF_UNIX;
    strncpy(client_addr.sun_path, CLIENT_SUN_PATH, sizeof(client_addr.sun_path));
    socklen_t client_addr_len = sizeof(client_addr);
    unlink(CLIENT_SUN_PATH);
    if (-1 == bind(fd, (struct sockaddr *)&client_addr, client_addr_len)) {
        printf("anld_ctrl_init, bind error\n");
        close(fd);
        return -1;
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SERVER_SUN_PATH, sizeof(server_addr.sun_path));
    socklen_t server_addr_len = sizeof(server_addr);

    anld_ctrl_cntx.fd = fd;
    anld_ctrl_cntx.addr = server_addr;
    anld_ctrl_cntx.len = server_addr_len;
    anld_ctrl_cntx.callback_func = callback_function;

    pthread_t tid;
    if (pthread_create(&tid, NULL, anld_ctrl_rx_fun, NULL) != 0) {
        printf("anld_ctrl_init, pthread_create error\n");
        close(fd);
        return -1;
    }
    anld_ctrl_cntx.tid = tid;
    return 0;
}

void anld_ctrl_deinit()
{
    pthread_cancel(anld_ctrl_cntx.tid);
    pthread_join(anld_ctrl_cntx.tid, NULL);
    close(anld_ctrl_cntx.fd);
}