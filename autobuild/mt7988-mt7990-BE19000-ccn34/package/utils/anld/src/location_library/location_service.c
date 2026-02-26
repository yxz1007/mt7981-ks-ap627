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
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "location_log.h"
#include "location_service.h"
#include "location_command.h"
#include "epo_host_aiding.h"
#include "epo_download.h"
#include "location_config.h"
#include "location_power.h"
#include "location_io.h"
#include "hdl_brom_demo.h"
#include "location_debug.h"

#define LOCATION_SERVICE_MSG_KEY    (1234)

#define LOCATION_SERVICE_ANLD_VERSION  "ANLD Version:V1.2\n"

typedef enum {
    PAIR_ACK                         = 1,
    PAIR_REQUEST_AIDING              = 10,
    PAIR_GET_SETTING_INFO            = 21,
    PAIR_TEST_CW_MODE                = 393,
} location_service_command_id_t;

typedef struct {
    int message_id;
    void *param;
    int param_length;
} location_service_message_struct_t;

typedef struct {
    long int msg_type;
    location_service_message_struct_t data;
} msg_struct_t;

typedef struct {
    uint16_t year;              /* > 2000 */
    uint8_t  month;             /* 1~12 */
    uint8_t  day;               /* 1~31*/
    uint8_t  hour;              /* 0~23*/
    uint8_t  min;               /* 0~59*/ 
    uint8_t  sec;               /* 0~59*/ 
} location_service_time_t;

typedef struct {
    double lat;                /* degree */
    double lon;                /* degree */
    double alt;                /* meter */
    double acc_maj;            /* meter */
    double acc_min;            /* meter */
    double bear;               /* meter */
    double acc_vert;           /* meter */
} location_service_position_t;

typedef struct {
    char utc[12];
    char latitude[15];
    char longitude[15];
    char height[10];
    char majorAxis[10];
    char minorAxis[10];
    char orientation[10];
    char vacc[12];
} location_service_location_t;


typedef struct {
    pthread_t tid;
    int msgid;
    int time_valid;
    location_service_callback_t callback_func;
    location_service_location_t location;
} location_service_cntx_t;

location_service_cntx_t g_location_service_cntx;

location_service_time_t location_service_get_utc()
{
    time_t time_sec;
    struct tm* time_utc;
    location_service_time_t utc = {0};

    time_sec = time(NULL);
    if (time_sec < 0) {
        LOC_SERV_LOGE("location_service_get_utc time error", 0);
        return utc;
    }
    time_utc = gmtime(&time_sec);
    if (time_utc == NULL) {
        LOC_SERV_LOGE("location_service_get_utc gmtime error", 0);
        return utc;
    }
    utc.year = time_utc->tm_year + 1900;
    utc.month = time_utc->tm_mon + 1;
    utc.day = time_utc->tm_mday;
    utc.hour = time_utc->tm_hour;
    utc.min = time_utc->tm_min;
    utc.sec = time_utc->tm_sec;
    return utc;
}

int location_service_get_position(location_service_position_t *location)
{
    FILE *fp;
    char path[LOCATION_CONFIG_BUF_LENGTH] = {0};
    char buffer[256];
    char *pos;
    location_config_get_nmea_file_path(path);
    fp = fopen(path, "r");
    int i = 0;
    if (NULL == fp) {
        LOC_SERV_LOGE("location_service_get_position open fail, %s", 1, path);
        return -1;
    }
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if ((pos = strstr(buffer, "longitude=")) != NULL) {
            if (strstr(pos, "\n") != NULL) {
                pos[strlen(pos) - 1] = '\0';
            }
            if (sscanf(pos, "longitude=%lf", &(location->lon)) == EOF) {
                LOC_SERV_LOGE("location_service_get_position sscanf lon fail", 0);
            } else {
                i++;
            }
        } else if ((pos = strstr(buffer, "latitude=")) != NULL) {
            if (strstr(pos, "\n") != NULL) {
                pos[strlen(pos) - 1] = '\0';
            }
            if (sscanf(pos, "latitude=%lf", &(location->lat)) == EOF) {
                LOC_SERV_LOGE("location_service_get_position sscanf lat fail", 0);
            } else {
                i++;
            }
        } else if ((pos = strstr(buffer, "majorAxis=")) != NULL) {
            if (strstr(pos, "\n") != NULL) {
                pos[strlen(pos) - 1] = '\0';
            }
            if (sscanf(pos, "majorAxis=%lf", &(location->acc_maj)) == EOF) {
                LOC_SERV_LOGE("location_service_get_position sscanf acc_maj fail", 0);
            } else {
                i++;
            }
        } else if ((pos = strstr(buffer, "minorAxis=")) != NULL) {
            if (strstr(pos, "\n") != NULL) {
                pos[strlen(pos) - 1] = '\0';
            }
            if (sscanf(pos, "minorAxis=%lf", &(location->acc_min)) == EOF) {
                LOC_SERV_LOGE("location_service_get_position sscanf acc_min fail", 0);
            } else {
                i++;
            }
        } else if ((pos = strstr(buffer, "orientation=")) != NULL) {
            if (strstr(pos, "\n") != NULL) {
                pos[strlen(pos) - 1] = '\0';
            }
            if (sscanf(pos, "orientation=%lf", &(location->bear)) == EOF) {
                LOC_SERV_LOGE("location_service_get_position sscanf bear fail", 0);
            } else {
                i++;
            }
        } else if ((pos = strstr(buffer, "height=")) != NULL) {
            if (strstr(pos, "\n") != NULL) {
                pos[strlen(pos) - 1] = '\0';
            }
            if (sscanf(pos, "height=%lf", &(location->alt)) == EOF) {
                LOC_SERV_LOGE("location_service_get_position sscanf alt fail", 0);
            } else {
                i++;
            }
        } else if ((pos = strstr(buffer, "verticalUncertainty=")) != NULL) {
            if (strstr(pos, "\n") != NULL) {
                pos[strlen(pos) - 1] = '\0';
            }
            if (sscanf(pos, "verticalUncertainty=%lf", &(location->acc_vert)) == EOF) {
                LOC_SERV_LOGE("location_service_get_position sscanf acc_vert fail", 0);
            } else {
                i++;
            }
        }
    }
    if (fclose(fp) != 0) {
        LOC_SERV_LOGE("location_service_get_position fclose fail", 0);
    }
    if (i == 7) {
        return 0;
    }
    return -1;
}

int location_service_save_location(location_service_location_t *location)
{
    FILE *fp;
    char path[LOCATION_CONFIG_BUF_LENGTH] = {0};
    char location_buffer[512];
    int ret;
    int len;

    ret = snprintf(location_buffer, sizeof(location_buffer), "UTC=%s\n", location->utc);
    if (ret < 0) {
        LOC_SERV_LOGE("location_service_save_location get utc fail", 0);
        return -1;
    }
    len = ret;

    ret = snprintf(location_buffer + len, sizeof(location_buffer) - len, "longitude=%s\n", location->longitude);
    if (ret < 0) {
        LOC_SERV_LOGE("location_service_save_location get longitude fail", 0);
        return -1;
    }
    len += ret;

    ret = snprintf(location_buffer + len, sizeof(location_buffer) - len, "latitude=%s\n", location->latitude);
    if (ret < 0) {
        LOC_SERV_LOGE("location_service_save_location get latitude fail", 0);
        return -1;
    }
    len += ret;

    ret = snprintf(location_buffer + len, sizeof(location_buffer) - len, "majorAxis=%s\n", location->majorAxis);
    if (ret < 0) {
        LOC_SERV_LOGE("location_service_save_location get majorAxis fail", 0);
        return -1;
    }
    len += ret;

    ret = snprintf(location_buffer + len, sizeof(location_buffer) - len, "minorAxis=%s\n", location->minorAxis);
    if (ret < 0) {
        LOC_SERV_LOGE("location_service_save_location get minorAxis fail", 0);
        return -1;
    }
    len += ret;

    ret = snprintf(location_buffer + len, sizeof(location_buffer) - len, "orientation=%s\n", location->orientation);
    if (ret < 0) {
        LOC_SERV_LOGE("location_service_save_location get orientation fail", 0);
        return -1;
    }
    len += ret;

    ret = snprintf(location_buffer + len, sizeof(location_buffer) - len, "height=%s\n", location->height);
    if (ret < 0) {
        LOC_SERV_LOGE("location_service_save_location get height fail", 0);
        return -1;
    }
    len += ret;

    ret = snprintf(location_buffer + len, sizeof(location_buffer) - len, "heightType=AGL\n");
    if (ret < 0) {
        LOC_SERV_LOGE("location_service_save_location get heightType fail", 0);
        return -1;
    }
    len += ret;

    ret = snprintf(location_buffer + len, sizeof(location_buffer) - len, "verticalUncertainty=%s\n", location->vacc);
    if (ret < 0) {
        LOC_SERV_LOGE("location_service_save_location get verticalUncertainty fail", 0);
        return -1;
    }
    len += ret;

    location_config_get_nmea_file_path(path);

    fp = fopen(path, "w");
    if (NULL == fp) {
        LOC_SERV_LOGE("location_service_save_location open fail, %s", 1, path);
        return -1;
    }

    if (fwrite(location_buffer, 1, strlen(location_buffer), fp) != strlen(location_buffer)) {
        LOC_SERV_LOGE("location_service_save_location write fail", 0);
        if (fclose(fp) != 0) {
            LOC_SERV_LOGE("location_service_save_location fclose fail", 0);
        }
        return -1;
    }
    if (fclose(fp) != 0) {
        LOC_SERV_LOGE("location_service_save_location fclose fail", 0);
    }
    return 0;
}


int location_service_get_location_string(location_service_location_t *location, char *buffer, int length)
{
    location_command_contx nmea_list;
    int len = 0;
    if ((strstr(buffer, "GGA") == NULL) && (strstr(buffer, "EPE") == NULL) && (strstr(buffer, "GST") == NULL)) {
        return -1;
    }
    
    location_command_get_param((uint8_t *)buffer, length, &nmea_list);

    if (nmea_list.param_num <= 1) {
        return -1;
    }
    
    if ((strstr(buffer, "GGA") != NULL) && (nmea_list.param_num >= 10)) {
        char *temp;
        char deg_s[15] = {0};
        char min_s[15] = {0};
        char flag[3] = {0};
        double deg;
        double min;
        memcpy(flag, nmea_list.param[3], 2);
        temp = strchr(nmea_list.param[2], '.');
        if (temp == NULL){
            return -1;
        }
        len = temp - nmea_list.param[2];
        if (len < 2) {
            return -1;
        }
        memcpy(deg_s, nmea_list.param[2], len - 2);
        deg_s[len - 2] = 0;
        memcpy(min_s, nmea_list.param[2] + len - 2, strlen(nmea_list.param[2]) - len + 2 + 1);
        if (sscanf(min_s, "%lf", &min) == EOF) {
            LOC_SERV_LOGE("location_service_get_location_string sscanf latitude min fail", 0);
        }
        if (sscanf(deg_s, "%lf", &deg) == EOF) {
            LOC_SERV_LOGE("location_service_get_location_string sscanf latitude deg fail", 0);
        }
        deg += min / 60;
        if (snprintf(location->latitude, sizeof(location->latitude), "%s%lf", flag[0] == 'N' ? "" : "-", deg) < 0) {
            LOC_SERV_LOGE("location_service_get_location_string get latitude fail", 0);
        }
        
        memset(flag, 0, sizeof(flag));
        memset(deg_s, 0, sizeof(deg_s));
        memset(min_s, 0, sizeof(min_s));
        memcpy(flag, nmea_list.param[5], 2);
        temp = strchr(nmea_list.param[4], '.');
        if (temp == NULL) {
            return -1;
        }
        len = temp - nmea_list.param[4];
        if (len < 2){
            return -1;
        }
        memcpy(deg_s, nmea_list.param[4], len - 2);
        deg_s[len - 2] = 0;
        memcpy(min_s, nmea_list.param[4] + len - 2, strlen(nmea_list.param[4]) - len + 2 + 1);
        if (sscanf(min_s, "%lf", &min) == EOF) {
            LOC_SERV_LOGE("location_service_get_location_string sscanf longitude min fail", 0);
        }
        if (sscanf(deg_s, "%lf", &deg) == EOF) {
            LOC_SERV_LOGE("location_service_get_location_string sscanf longitude deg fail", 0);
        }
        deg += min / 60;
        if (snprintf(location->longitude, sizeof(location->longitude), "%s%lf", flag[0] == 'E' ? "" : "-", deg) < 0) {
            LOC_SERV_LOGE("location_service_get_location_string get longitude fail", 0);
        }

        memset(deg_s, 0, sizeof(deg_s));
        memset(min_s, 0, sizeof(min_s));
        memcpy(deg_s, nmea_list.param[9], strlen(nmea_list.param[9]) + 1 > sizeof(deg_s) ? sizeof(deg_s) : strlen(nmea_list.param[9]) + 1);
        memcpy(min_s, nmea_list.param[11], strlen(nmea_list.param[11]) + 1 > sizeof(min_s) ? sizeof(min_s) : strlen(nmea_list.param[11]) + 1);
        if (sscanf(min_s, "%lf", &min) == EOF) {
            LOC_SERV_LOGE("location_service_get_location_string sscanf height min fail", 0);
        }
        if (sscanf(deg_s, "%lf", &deg) == EOF) {
            LOC_SERV_LOGE("location_service_get_location_string sscanf height deg fail", 0);
        }
        deg += min;
        if (sprintf(location->height, "%lf", deg) < 0) {
            LOC_SERV_LOGE("location_service_get_location_string get height fail", 0);
        }

        memcpy(location->utc, nmea_list.param[1], strlen(nmea_list.param[1]) + 1 > sizeof(location->utc) ? sizeof(location->utc) : strlen(nmea_list.param[1]) + 1);
    }
    if (strstr(buffer, "GST") != NULL && (nmea_list.param_num >= 3)) {
        memcpy(location->majorAxis, nmea_list.param[3], strlen(nmea_list.param[3]) + 1 > sizeof(location->majorAxis) ? sizeof(location->majorAxis) : strlen(nmea_list.param[3]) + 1);
        memcpy(location->minorAxis, nmea_list.param[4], strlen(nmea_list.param[4]) + 1 > sizeof(location->minorAxis) ? sizeof(location->minorAxis) : strlen(nmea_list.param[4]) + 1);
        memcpy(location->orientation, nmea_list.param[5], strlen(nmea_list.param[5]) + 1 > sizeof(location->orientation) ? sizeof(location->orientation) : strlen(nmea_list.param[5]) + 1);
    } 
    if (strstr(buffer, "EPE") != NULL && (nmea_list.param_num >= 3)) {
        memcpy(location->vacc, nmea_list.param[2], strlen(nmea_list.param[2]) + 1 > sizeof(location->vacc) ? sizeof(location->vacc) : strlen(nmea_list.param[2]) + 1);
    }
    return 0;
}


int location_service_start()
{
    LOC_SERV_LOGI("location_service_start", 0);

    int len = sizeof(LOCATION_SERVICE_ANLD_VERSION);
    char *version = (char *)malloc(len);
    if (version != NULL) {
        if (sprintf(version, LOCATION_SERVICE_ANLD_VERSION) > 0) {
            location_service_task_send_message(LOC_SERVICE_MSG_RECEIVE_NMEA, version, len);
        } else {
            free(version);
        }
    }

    location_debug_start();
    if (location_io_thread_init() != 0 ) {
        return -1;
    }
    return location_power_on_device();
}

int location_service_stop()
{
    LOC_SERV_LOGI("location_service_stop", 0);
    location_power_off_device();
    location_debug_close();
    return location_io_thread_deinit();
}

int location_service_download()
{
    LOC_SERV_LOGI("location_service_download", 0);
    return firmware_download();
}

void location_service_cust_data_callback(unsigned char *buffer, int data_length)
{
    //LOC_SERV_LOGI("[cust data] %s, %d", 2, buffer, data_length);
}

void location_service_nmea_callback(char *buffer, int data_length)
{
    if (location_service_get_location_string(&g_location_service_cntx.location, buffer, data_length) == 0) {
        if (strstr(buffer, "GGA") != NULL) {
            char buf[256];
            memset(buf, 0, sizeof(buf));
            memcpy(buf, &g_location_service_cntx.location, sizeof(g_location_service_cntx.location) > 256 ? 256 : sizeof(g_location_service_cntx.location));
            g_location_service_cntx.callback_func(LOC_SERVICE_MSG_LOCATION_INFO, 0, buf, sizeof(buf));
        }
    }
}


void location_service_cmd_rsp_callback(unsigned char *buffer, int data_length)
{
    
    location_command_contx contx;
    int32_t id;
    if ((buffer == NULL) || (data_length < 10)) {
        return;
    }
    LOC_SERV_LOGI("%s, %d", 2, buffer, data_length);
    location_command_get_param(buffer, data_length, &contx);

    id = atoi(contx.param[0]);
    switch (id) {
        #if 0
        case PAIR_ACK: {
            if (atoi(contx.param[1]) == 2) {
                location_command_send("382,1");
            }
            break;
        }
        #endif
        case PAIR_REQUEST_AIDING: {
            if (contx.param_num < 3) {
                LOC_SERV_LOGE("location_demo_app_cmd_rsp_callback, param number error: %d", 1, id);
                break;
            }
            if (atoi(contx.param[1]) == 0) {
                int32_t type;
                int32_t weekno;
                int32_t tow;
                type = atoi(contx.param[2]);
                weekno = atoi(contx.param[3]);
                tow = atoi(contx.param[4]);
                epo_host_aiding(type, weekno, tow);
            } else if (atoi(contx.param[1]) == 1) {
                if (g_location_service_cntx.time_valid == 1) {
                    char temp_buffer[100] = {0};
                    location_service_time_t time;
                    time = location_service_get_utc();
                    if (sprintf(temp_buffer, "590,%04d,%02d,%02d,%02d,%02d,%02d",
                            time.year, time.month, time.day, time.hour, time.min, time.sec) > 0) {
                        location_command_send(temp_buffer);
                    }
                }
            }
            else if (atoi(contx.param[1]) == 2) {
                char temp_buffer[100] = {0};
                int ret;
                location_service_position_t position = {0};
                ret = location_service_get_position(&position);
                if (ret == 0) {
                    if (sprintf(temp_buffer, "600,%.6f,%.6f,%.1f,%.1f,%.1f,%.1f,%.1f",
                            position.lat,
                            position.lon,
                            position.alt,
                            position.acc_maj,
                            position.acc_min,
                            position.bear,
                            position.acc_vert) > 0) {
                        location_command_send(temp_buffer);
                    }
                }
                LOC_SERV_LOGI("aiding reference position: %.6f,%.6f,%.1f,%.1f,%.1f,%.1f,%.1f", 7,
                        position.lat,
                        position.lon,
                        position.alt,
                        position.acc_maj,
                        position.acc_min,
                        position.bear,
                        position.acc_vert);
            }
            break;
        }
        case PAIR_GET_SETTING_INFO: {
            g_location_service_cntx.callback_func(LOC_SERVICE_MSG_RECEIVE_NMEA, 0, (char *)buffer, data_length);
            break;
        }
        case PAIR_TEST_CW_MODE: {
            g_location_service_cntx.callback_func(LOC_SERVICE_MSG_SEND_CMD, 0, (char *)buffer, data_length);
        }
        default: {
            break;
        }   
    }
}

void location_service_binary_callback(unsigned char *buffer, int data_length)
{
    LOC_SERV_LOGI("[binary data]%d", 1, data_length);
}

void location_service_task_send_message(int32_t message_id, void *param, int32_t param_length)
{
    msg_struct_t item;
    item.msg_type = 1;
    item.data.message_id = message_id;
    item.data.param = param;
    item.data.param_length = param_length;
    
    if (msgsnd(g_location_service_cntx.msgid, &item, sizeof(item.data), 0) == -1) {
        if (item.data.param != NULL) {
            free(item.data.param);
        }
        LOC_SERV_LOGE("location_service_task_send_message, Fail!", 0);
    }
}

void location_service_message_handle(location_service_message_struct_t *message)
{
    LOC_SERV_LOGI("location_service_message_handle:%d", 1, message->message_id);
    int state;
    switch (message->message_id) {
        case LOC_SERVICE_MSG_START: {
            state = location_service_start();
            g_location_service_cntx.callback_func(message->message_id, state, NULL, 0);
            break;
        }
        case LOC_SERVICE_MSG_STOP: {
            state = location_service_stop();
            g_location_service_cntx.callback_func(message->message_id, state, NULL, 0);
            break;
        }
        case LOC_SERVICE_MSG_DOWNLOAD_FIRMWARE: {
            location_service_stop();
            state = location_service_download();
            location_power_off_device();
            g_location_service_cntx.callback_func(message->message_id, state, NULL, 0);
            break;
        }
        case LOC_SERVICE_MSG_SAVE_LOCATION: {
            state = location_service_save_location(&g_location_service_cntx.location);
            g_location_service_cntx.callback_func(message->message_id, state, NULL, 0);
            break;
        }
        case LOC_SERVICE_MSG_DEBUG_DISABLE: {
            location_debug_disable();
            state = 0;
            g_location_service_cntx.callback_func(message->message_id, state, NULL, 0);
            break;
        }
        case LOC_SERVICE_MSG_DEBUG_SAVE_TO_FILE: {
            state = location_debug_open(LOCATION_DEBUG_SAVE_TO_FILE);
            g_location_service_cntx.callback_func(message->message_id, state, NULL, 0);
            break;
        }
        case LOC_SERVICE_MSG_DEBUG_SEND_BY_PORT: {
            state = location_debug_open(LOCATION_DEBUG_SEND_BY_PORT);
            g_location_service_cntx.callback_func(message->message_id, state, NULL, 0);
            break;
        }
        case LOC_SERVICE_MSG_TIME_VALID: {
            g_location_service_cntx.time_valid = 1;
            epo_download_init(15, 1);
            state = 0;
            g_location_service_cntx.callback_func(message->message_id, state, NULL, 0);
            break;
        }
        case LOC_SERVICE_MSG_EPO_DOWNLOAD: {
            state = *(int *)message->param;
            g_location_service_cntx.callback_func(message->message_id, state, NULL, 0);
            break;
        }
        case LOC_SERVICE_MSG_SEND_CMD: {
            state = location_command_send((char *)message->param);
            g_location_service_cntx.callback_func(message->message_id, state, NULL, 0);
            break;
        }
        case LOC_SERVICE_MSG_RECEIVE_CUST_DATA:
            location_service_cust_data_callback((unsigned char *)message->param, message->param_length);
            break;
        case LOC_SERVICE_MSG_RECEIVE_NMEA:
            location_service_nmea_callback((char *)message->param, message->param_length);
            state = 0;
            g_location_service_cntx.callback_func(message->message_id, state, (char *)message->param, message->param_length);
            break;
        case LOC_SERVICE_MSG_RECEIVE_CMD_RSP:
            location_service_cmd_rsp_callback((unsigned char *)message->param, message->param_length);
            break;
        case LOC_SERVICE_MSG_RECEIVE_BINARY:
            location_service_binary_callback((unsigned char *)message->param, message->param_length);
            break;
        default:
            break;
    }
    if (message->param != NULL) {
        free(message->param);
    }
}

void *location_service_task_main(void *arg)
{
    msg_struct_t item;
    LOC_SERV_LOGI("location_service_task_main", 0);
    while (1) {
        if (msgrcv(g_location_service_cntx.msgid, &item, sizeof(item.data), 0, 0) != -1) {
            location_service_message_handle(&item.data);
        }
    }
}

void location_service_init(location_service_callback_t callback_function)
{
    char path[LOCATION_CONFIG_BUF_LENGTH] = {0};
    int ret;
    ret = msgget((key_t)LOCATION_SERVICE_MSG_KEY, IPC_EXCL);
    if (ret != -1) {
        msgctl(ret, IPC_RMID, NULL);
    }
    g_location_service_cntx.msgid = msgget((key_t)LOCATION_SERVICE_MSG_KEY, 0666 | IPC_CREAT | IPC_EXCL);
    if (g_location_service_cntx.msgid == -1) {
        LOC_SERV_LOGE("location_service_init, msgid fail", 0);
    }

    location_config_get_nmea_file_path(path);
    unlink(path);
    epo_mutex_init();
    g_location_service_cntx.time_valid = 0;
    g_location_service_cntx.callback_func = callback_function;
    pthread_create(&g_location_service_cntx.tid, NULL, location_service_task_main, NULL);
}

