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

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#include "epo_host_aiding.h"
#include "location_command.h"
#include "location_log.h"
#include "location_config.h"

#define GNSS_EPO_DATA_SIZE (72)

#define GNSS_EPO_GPS_SV            (32)
#define GNSS_EPO_GLONASS_SV        (24)

#define GNSS_GLONASS_EPO_BASE_ID    (64)
#define GNSS_GALILEO_EPO_BASE_ID    (100)
#define GNSS_BEIDOU_EPO_BASE_ID     (200)
#define GNSS_BEIDOU_EPO_BASE1_ID    (190)
#define GNSS_BEIDOU_EPO_BASE2_ID    (198)
#define GNSS_BEIDOU_EPO_BASE1_START (55)

#define EPO_DEMO_GR_3D_STR     "EPO_GR_3_%d.DAT"
#define EPO_DEMO_GPS_3D_STR    "EPO_GPS_3_%d.DAT"
#define EPO_DEMO_BDS_3D_STR    "EPO_BDS_3.DAT"
#define EPO_DEMO_GA_3D_STR     "EPO_GAL_3.DAT"
#define EPO_DEMO_GR_Q_STR      "QG_R.DAT"
#define EPO_DEMO_GPS_Q_STR     "QGPS.DAT"
#define EPO_DEMO_BD2_Q_STR     "QBD2.DAT"
#define EPO_DEMO_GA_Q_STR      "QGA.DAT"

typedef enum {
    GNSS_EPO_MODE_GPS = 0,
    GNSS_EPO_MODE_GLONASS,
    GNSS_EPO_MODE_GALILEO,
    GNSS_EPO_MODE_BEIDOU
} gnss_epo_mode_t;

static pthread_mutex_t g_epo_mutex;

void epo_download_get_filename(epo_demo_type_t type, uint32_t index, char *file_name)
{
    int ret;
    if (type == EPO_DEMO_TYPE_GR_3D) {
        ret = sprintf(file_name, EPO_DEMO_GR_3D_STR, (int)index);
    } else if (type == EPO_DEMO_TYPE_GPS_3D) {
        ret = sprintf(file_name, EPO_DEMO_GPS_3D_STR, (int)index);
    } else if (type == EPO_DEMO_TYPE_BDS_3D) {
        ret = sprintf(file_name, EPO_DEMO_BDS_3D_STR);
    } else if (type == EPO_DEMO_TYPE_GA_3D) {
        ret = sprintf(file_name, EPO_DEMO_GA_3D_STR);
    } else if (type == EPO_DEMO_TYPE_GR_Q) {
        ret = sprintf(file_name, EPO_DEMO_GR_Q_STR);
    } else if (type == EPO_DEMO_TYPE_GPS_Q) {
        ret = sprintf(file_name, EPO_DEMO_GPS_Q_STR);
    } else if (type == EPO_DEMO_TYPE_BD2_Q) {
        ret = sprintf(file_name, EPO_DEMO_BD2_Q_STR);
    } else if (type == EPO_DEMO_TYPE_GA_Q) {
        ret = sprintf(file_name, EPO_DEMO_GA_Q_STR);
    } else {
        return;
    }
    if (ret < 0) {
        LOC_SERV_LOGE("[EPO] epo_download_get_filename get file_name fail", 0);
    }
}

void epo_download_get_filepath(epo_demo_type_t type, uint32_t index, char *file_path)
{
    char file_name[128] = {0};
    char epo_file_path[LOCATION_CONFIG_BUF_LENGTH];
    int ret;
    epo_download_get_filename(type, index, file_name);
    location_config_get_epo_file_path(epo_file_path);
    ret = snprintf(file_path, LOCATION_CONFIG_BUF_LENGTH, "%s/%s", epo_file_path, file_name);
    if (ret < 0) {
        LOC_SERV_LOGE("[EPO] epo_download_get_filepath get path fail", 0);
    }
}

int epo_mutex_lock()
{
    return pthread_mutex_lock(&g_epo_mutex);
}

int epo_mutex_unlock()
{
    return pthread_mutex_unlock(&g_epo_mutex);
}

int32_t epo_host_aiding_get_sv_prn(int8_t type, uint8_t *data)
{
    int32_t sv_id, sv_prn = 0;

    sv_id = data[3];

    switch(type) {
        case GNSS_EPO_MODE_GPS:
            sv_prn = sv_id;
            break;
        case GNSS_EPO_MODE_GLONASS:
            sv_prn = sv_id - GNSS_GLONASS_EPO_BASE_ID;
            break;
        case GNSS_EPO_MODE_GALILEO:
            if(sv_id == 254) {
                sv_prn = 254;
            } else {
                sv_prn = sv_id - GNSS_GALILEO_EPO_BASE_ID;
            }
            break;
        case GNSS_EPO_MODE_BEIDOU:
            if (sv_id == 255) {
                sv_prn = 255;
            } else if ((sv_id >= GNSS_BEIDOU_EPO_BASE1_ID) && (sv_id <= GNSS_BEIDOU_EPO_BASE2_ID)) {
                sv_prn = sv_id - GNSS_BEIDOU_EPO_BASE1_ID + GNSS_BEIDOU_EPO_BASE1_START;
            } else {
                sv_prn = sv_id - GNSS_BEIDOU_EPO_BASE_ID;
            }
            break;
        default:
            sv_prn = 0;
    }
    return sv_prn;
}

void epo_host_aiding_send_data(FILE *handle, int32_t number, int8_t type)
{
    char temp_buffer[352] = {0};
    uint8_t buffer[GNSS_EPO_DATA_SIZE] = {0};
    int32_t i;
    int32_t sv_prn = 0;
    int32_t ret;
    for(i = 0; i < number; i++) {
        unsigned int *epobuf = (unsigned int *)buffer;
        ret = fread(buffer, 1, GNSS_EPO_DATA_SIZE, handle);
        if (ret > 0) {
            sv_prn = epo_host_aiding_get_sv_prn(type, buffer);
            if (sprintf((char *) temp_buffer,
                    "471,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X",
                    (unsigned int)type,
                    (unsigned int)sv_prn,
                    epobuf[0], epobuf[1], epobuf[2], epobuf[3], epobuf[4], epobuf[5],
                    epobuf[6], epobuf[7], epobuf[8], epobuf[9], epobuf[10], epobuf[11],
                    epobuf[12], epobuf[13], epobuf[14], epobuf[15], epobuf[16], epobuf[17]) > 0) {
                location_command_send(temp_buffer);
            }
            memset(temp_buffer, 0, sizeof(temp_buffer));
            usleep(10000);
        } else {
            LOC_SERV_LOGE("[EPO] epo_host_aiding_send_data, read fail", 0);
            break;
        }
    }
}

bool epo_host_aiding_epo(int8_t type, int32_t weekno, int32_t tow)
{
    int32_t epo_gnss_hour = 0;
    int32_t current_gnss_hour = 0;
    int32_t segment = 0;
    char file_name[128];
    int32_t sv_num = 0;
    FILE *file_handle = 0;
    int32_t gps_file_type = 0; /* 0:GR, 1:GPS*/
    int32_t ret;
    current_gnss_hour = weekno * 7 * 24 + tow / 3600;

    switch (type) {
        case GNSS_EPO_MODE_GPS: {
            epo_download_get_filepath(EPO_DEMO_TYPE_GR_3D, 1, file_name);
            sv_num = GNSS_EPO_GPS_SV;
            break;
        }
        case GNSS_EPO_MODE_GLONASS: {
            epo_download_get_filepath(EPO_DEMO_TYPE_GR_3D, 1, file_name);
            sv_num = GNSS_EPO_GLONASS_SV;
            break;
        }
        case GNSS_EPO_MODE_GALILEO: {
            epo_download_get_filepath(EPO_DEMO_TYPE_GA_3D, 1, file_name);
            break;
        }
        case GNSS_EPO_MODE_BEIDOU: {
            epo_download_get_filepath(EPO_DEMO_TYPE_BDS_3D, 1, file_name);
            break;
        }
        default: {
            LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, type wrong:%d", 1, type);
            return false;
        }
    }
    file_handle = fopen(file_name, "r");
    if (file_handle == NULL) {
        if (type == GNSS_EPO_MODE_GPS) {
            epo_download_get_filepath(EPO_DEMO_TYPE_GPS_3D, 1, file_name);
            file_handle = fopen(file_name, "r");
            if (file_handle == NULL) {
                LOC_SERV_LOGE("[EPO] epo_host_aiding_epo: %d, without EPO file", 1, type);
                return false;
            }
            gps_file_type = 1;
        } else {
            LOC_SERV_LOGE("[EPO] epo_host_aiding_epo: %d, without EPO file", 1, type);
            return false;
        }
    }
    if ((type == GNSS_EPO_MODE_GALILEO) || (type == GNSS_EPO_MODE_BEIDOU)) {
        uint32_t head[18];
        memset(head, 0, sizeof(head));
        ret = fread((uint8_t *)head, 1, GNSS_EPO_DATA_SIZE, file_handle);
        if (ret <= 0) {
            LOC_SERV_LOGE("epo_host_aiding_epo, fread fail", 0);
        }
        while(head[1]) {
            head[1] &= (head[1] - 1);
            sv_num ++;
        }
        while(head[4]) {
            head[4] &= (head[4] - 1);
            sv_num ++;
        }
        ret = fread((uint8_t *)&epo_gnss_hour, 1, 4, file_handle);
        if (ret <= 0) {
            LOC_SERV_LOGE("epo_host_aiding_epo, fread fail", 0);
        }
        epo_gnss_hour = epo_gnss_hour & 0x00FFFFFF;
        segment = (current_gnss_hour - epo_gnss_hour) / 6;
        if ((segment < 0) || (segment >= 12)) {
            LOC_SERV_LOGE("[EPO] epo_host_aiding_epo: %d, invalid epo", 1, type);
            if (fclose(file_handle) != 0) {
                LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
            }
            return false;
        }
        if (fseek(file_handle, 0, SEEK_SET) != 0) {
            LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, seek fail", 0);
        }
        epo_host_aiding_send_data(file_handle, 1, type);
        if (fseek(file_handle, (segment * sv_num * GNSS_EPO_DATA_SIZE) + GNSS_EPO_DATA_SIZE, SEEK_SET) != 0) {
            if (fclose(file_handle) != 0) {
                LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
            }
            LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, seek fail", 0);
            return false;
        }
        epo_host_aiding_send_data(file_handle, sv_num, type);
        if (fclose(file_handle) != 0) {
            LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
        }
        return true;
    } else {
        ret = fread((uint8_t *)&epo_gnss_hour, 1, 4, file_handle);
        if (ret <= 0) {
            LOC_SERV_LOGE("epo_host_aiding_epo, fread fail", 0);
        }
        epo_gnss_hour = epo_gnss_hour & 0x00FFFFFF;
        segment = (current_gnss_hour - epo_gnss_hour) / 6;
        if ((segment >= 0) && (segment < 12)) {
            if (type == GNSS_EPO_MODE_GPS) {
                if (gps_file_type == 0) {
                    if (fseek(file_handle, segment * (GNSS_EPO_GPS_SV + GNSS_EPO_GLONASS_SV) * GNSS_EPO_DATA_SIZE, SEEK_SET) != 0) {
                        if (fclose(file_handle) != 0) {
                            LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
                        }
                        LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, seek fail", 0);
                        return false;
                    }
                } else {
                    if (fseek(file_handle, segment * GNSS_EPO_GPS_SV * GNSS_EPO_DATA_SIZE, SEEK_SET) != 0) {
                        if (fclose(file_handle) != 0) {
                            LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
                        }
                        LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, seek fail", 0);
                        return false;
                    }
                }
            } else if (type == GNSS_EPO_MODE_GLONASS) {
                if (fseek(file_handle, (segment * (GNSS_EPO_GPS_SV + GNSS_EPO_GLONASS_SV) * GNSS_EPO_DATA_SIZE) + (GNSS_EPO_GPS_SV * GNSS_EPO_DATA_SIZE), SEEK_SET) != 0) {
                    if (fclose(file_handle) != 0) {
                        LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
                    }
                    LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, seek fail", 0);
                    return false;
                }
            }
            epo_host_aiding_send_data(file_handle, sv_num, type);
            if (fclose(file_handle) != 0) {
                LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
            }
            return true;
        }
        if (fclose(file_handle) != 0) {
            LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
        }

        epo_download_get_filepath(EPO_DEMO_TYPE_GR_3D, (segment / 12) + 1, file_name);
        file_handle = fopen(file_name, "r");
        gps_file_type = 0;
        if (file_handle == NULL) {
            if (type == GNSS_EPO_MODE_GPS) {
                epo_download_get_filepath(EPO_DEMO_TYPE_GR_3D, (segment / 12) + 1, file_name);
                file_handle = fopen(file_name, "r");
                if (file_handle == NULL) {
                    LOC_SERV_LOGE("[EPO] epo_host_aiding_epo: %d, without EPO file", 1, type);
                    return false;
                }
                gps_file_type = 1;
            } else {
                LOC_SERV_LOGE("[EPO] epo_host_aiding_epo: %d, without EPO file", 1, type);
                return false;
            }
        }
        ret = fread((uint8_t *)&epo_gnss_hour, 1, 4, file_handle);
        if (ret <= 0) {
            LOC_SERV_LOGE("epo_host_aiding_epo, fread fail", 0);
        }
        epo_gnss_hour = epo_gnss_hour & 0x00FFFFFF;
        segment = (current_gnss_hour - epo_gnss_hour) / 6;
        if ((segment < 0) || (segment >= 12)) {
            if (fclose(file_handle) != 0) {
                LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
            }
            LOC_SERV_LOGE("[EPO] epo_host_aiding_epo: %d, invalid epo", 1, type);
            return false;
        }
        if (type == GNSS_EPO_MODE_GPS) {
            if (gps_file_type == 0) {
                if (fseek(file_handle, segment * (GNSS_EPO_GPS_SV + GNSS_EPO_GLONASS_SV) * GNSS_EPO_DATA_SIZE, SEEK_SET) != 0) {
                    if (fclose(file_handle) != 0) {
                        LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
                    }
                    LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, seek fail", 0);
                    return false;
                }
            } else {
                if (fseek(file_handle, segment * GNSS_EPO_GPS_SV * GNSS_EPO_DATA_SIZE, SEEK_SET) != 0) {
                    if (fclose(file_handle) != 0) {
                        LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
                    }
                    LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, seek fail", 0);
                    return false;
                }
            }
        } else if (type == GNSS_EPO_MODE_GLONASS) {
            if (fseek(file_handle, (segment * (GNSS_EPO_GPS_SV + GNSS_EPO_GLONASS_SV) * GNSS_EPO_DATA_SIZE) + (GNSS_EPO_GPS_SV * GNSS_EPO_DATA_SIZE), SEEK_SET) != 0) {
                if (fclose(file_handle) != 0) {
                    LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
                }
                LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, seek fail", 0);
                return false;
            }
        }
        epo_host_aiding_send_data(file_handle, sv_num, type);
        if (fclose(file_handle) != 0) {
            LOC_SERV_LOGE("[EPO] epo_host_aiding_epo, close file_handle fail", 0);
        }
        return true;
    }
}

bool epo_host_aiding(int8_t type, int32_t weekno, int32_t tow)
{
    epo_mutex_lock();

    if (epo_host_aiding_epo(type, weekno, tow)) {
        LOC_SERV_LOGI("[EPO] host aiding epo finish:%d", 1, type);
        epo_mutex_unlock();
        return true;
    }

    epo_mutex_unlock();

    return false;
}

void epo_mutex_init()
{
    if (pthread_mutex_init(&g_epo_mutex, NULL) != 0) {
        LOC_SERV_LOGE("[EPO]epo_mutex_init, mutex init fail",0);
    }
}