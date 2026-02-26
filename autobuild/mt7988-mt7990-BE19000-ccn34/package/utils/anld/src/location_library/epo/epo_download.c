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
#include <stdint.h>

#include "location_log.h"
#include "location_timer.h"
#include "location_config.h"
#include "location_service.h"
#include "epo_host_aiding.h"
#if defined(LOCATION_SUPPORT_EPO_DOWNLOAD)
#include <curl/curl.h>
#include "mbedtls/md5.h"

#define EPO_DOWNLOAD_SUPPORT_GPS       (0x01)
#define EPO_DOWNLOAD_SUPPORT_GLONASS   (0x02)
#define EPO_DOWNLOAD_SUPPORT_BD2       (0x04)
#define EPO_DOWNLOAD_SUPPORT_GALILEO   (0x08)

#define EPO_DOWNLOAD_RETRY_NETWORK_TIMER(X) (((1 << (X)) > 8 ? 12 : (1 << (X)))*5*60*1000)

#define EPO_DOWNLOAD_FIRST_START_TIMER      (3 * 1000)
#define EPO_DOWNLOAD_EXPIRED_TIME           (47)

#define URL_LEN         (1024)
#define MD5_LEN         (16)
#define MD5_CHECK_LEN   (1024 * 2)

typedef struct{
    void *next;
    epo_demo_type_t type;
    int32_t index;
}epo_download_handle_t;


typedef struct{
    uint8_t support_type;
    epo_demo_type_t type_3d;          /*min valid hour type*/
    uint32_t trunk_num_3d;
    epo_download_handle_t *list_3d;
    epo_download_handle_t *current;
    timer_t retry_timer;
    timer_t next_download_timer;
}epo_download_cntx_t;

epo_download_cntx_t g_epo_download_cntx = {
    .retry_timer = 0,
    .next_download_timer = 0,
};

static uint32_t g_epo_download_retry_count = 0;

uint32_t epo_download_get_next_download_time(int32_t valid_hours, int32_t max_gnss_hours);
void epo_download_handle();

int32_t epo_download_utc_to_gnss_hour(int32_t iYr, int32_t iMo, int32_t iDay, int32_t iHr)
{
    int32_t iYearsElapsed; // Years since 1980
    int32_t iDaysElapsed; // Days elapsed since Jan 6, 1980
    int32_t iLeapDays; // Leap days since Jan 6, 1980
    int32_t i;
    // Number of days into the year at the start of each month (ignoring leap years)
    const unsigned short doy[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    iYearsElapsed = iYr - 1980;
    i = 0;
    iLeapDays = 0;
    while(i <= iYearsElapsed) {
        if((i % 100) == 20) {
            if((i % 400) == 20) {
                iLeapDays++;
            }
        } else if((i % 4) == 0) {
            iLeapDays++;
        }
        i++;
    }
    if((iYearsElapsed % 100) == 20) {
        if(((iYearsElapsed % 400) == 20) && (iMo <= 2)) {
            iLeapDays--;
        }
    } else if(((iYearsElapsed % 4) == 0) && (iMo <= 2)) {
        iLeapDays--;
    }
    iDaysElapsed = iYearsElapsed * 365 + (int)doy[iMo - 1] + iDay + iLeapDays - 6;
    // Convert time to GNSS weeks and seconds
    return (iDaysElapsed * 24 + iHr);
}

int32_t epo_download_get_3d_valid_hours(epo_demo_type_t type_3d, uint32_t trunk_num_3d, int32_t *max_epo_hours)
{    
    int32_t epo_gnss_hour = 0;
    int32_t current_gnss_hour = 0;
    FILE *fd;
    char file_name[128];
    int32_t i;
    int32_t max_gnss_hours = 0;
    time_t time_sec;
    struct tm* time_utc;
    int ret;

    time_sec = time(NULL);
    if (time_sec < 0) {
        LOC_SERV_LOGE("epo_download_get_3d_valid_hours time error", 0);
        return 0;
    }
    time_utc = gmtime(&time_sec);
    if (time_utc == NULL) {
        LOC_SERV_LOGE("epo_download_get_3d_valid_hours gmtime error", 0);
        return 0;
    }
    current_gnss_hour = epo_download_utc_to_gnss_hour(time_utc->tm_year + 1900, time_utc->tm_mon + 1, time_utc->tm_mday, time_utc->tm_hour);
    LOC_SERV_LOGI("[EPO]%d-%d-%d-%d", 4, time_utc->tm_year + 1900, time_utc->tm_mon + 1, time_utc->tm_mday, time_utc->tm_hour);

    if ((type_3d == EPO_DEMO_TYPE_GR_3D) || (type_3d == EPO_DEMO_TYPE_GPS_3D)) {
        for (i = 0; i < trunk_num_3d; i++) {
            epo_download_get_filepath(type_3d, i + 1, file_name);
            fd = fopen(file_name, "r");
            if (!fd) {
                continue;
            }
            ret = fread(&epo_gnss_hour, 1, 4, fd);
            if (ret <= 0) {
                LOC_SERV_LOGE("[EPO]epo_download_get_3d_valid_hours, fread fail", 0);
            }
            epo_gnss_hour = epo_gnss_hour & 0x00FFFFFF;
            if (epo_gnss_hour >= max_gnss_hours)
            {
                max_gnss_hours = epo_gnss_hour;
            }

            if (fclose(fd) != 0) {
                LOC_SERV_LOGE("[EPO]epo_download_get_3d_valid_hours fclose fail", 0);
            }
        }
    } else if ((type_3d == EPO_DEMO_TYPE_BDS_3D) || (type_3d == EPO_DEMO_TYPE_GA_3D)) {
        epo_download_get_filepath(type_3d, 0, file_name);
        fd = fopen(file_name, "r");
        if (fd != NULL) {
            if (fseek(fd, 72, SEEK_SET) != 0) {
                LOC_SERV_LOGE("epo_download_get_3d_valid_hours, fseek fail", 0);
            }
            ret = fread(&epo_gnss_hour, 1, 4, fd);
            if (ret <= 0) {
                LOC_SERV_LOGE("epo_download_get_3d_valid_hours, fread fail", 0);
            }
            max_gnss_hours = epo_gnss_hour & 0x00FFFFFF;
            if (fclose(fd) != 0) {
                LOC_SERV_LOGE("[EPO]epo_download_get_3d_valid_hours fclose fail", 0);
            }
        }
    }
    if ((max_gnss_hours + (3 * 24) - current_gnss_hour) > 0) {
        *max_epo_hours = max_gnss_hours + (3 * 24);
        return (max_gnss_hours + (3 * 24) - current_gnss_hour);
    }
    *max_epo_hours = 0;
    return 0;
}


epo_download_handle_t *epo_download_add_handle(epo_download_handle_t *head, epo_download_handle_t *handle)
{
    epo_download_handle_t *temp = head;
    if (head == NULL) {
        return handle;
    }
    while(temp->next != NULL) {
        temp = temp->next;
    }

    temp->next = handle;
    handle->next = NULL;
    return head;
}

epo_download_handle_t *epo_download_push_handle(epo_download_handle_t *head, epo_download_handle_t *handle)
{
    if (head == NULL){
        return handle;
    }
    handle->next = head;
    
    return handle;
}

epo_download_handle_t *epo_download_remove_handle(epo_download_handle_t *head, epo_download_handle_t *handle)
{
    epo_download_handle_t *temp = head;
    if (head == handle){
        return handle->next;
    }
    while(temp->next != handle){
        temp = temp->next;
    }

    temp->next = handle->next;
    return head;
}

void epo_download_get_3d_min_valid_hours(epo_download_cntx_t *cntx, int32_t *valid_epo_hours, int32_t *max_epo_hours)
{
    int32_t temp_hours = 0;
    int32_t epo_hours = 0;
    int32_t min_valid_epo_hours = 0x7fffffff;
    if ((cntx->support_type & EPO_DOWNLOAD_SUPPORT_GLONASS) != 0) {
        temp_hours = epo_download_get_3d_valid_hours(EPO_DEMO_TYPE_GR_3D, cntx->trunk_num_3d, &epo_hours);
        if (min_valid_epo_hours > temp_hours) {
            cntx->type_3d = EPO_DEMO_TYPE_GR_3D;
            min_valid_epo_hours = temp_hours;
            *max_epo_hours = epo_hours;
        }
        if (temp_hours == 0) {
            return;
        }
    } else if ((cntx->support_type & EPO_DOWNLOAD_SUPPORT_GPS) != 0) {
        temp_hours = epo_download_get_3d_valid_hours(EPO_DEMO_TYPE_GPS_3D, cntx->trunk_num_3d, &epo_hours);
        if (min_valid_epo_hours > temp_hours) {
            cntx->type_3d = EPO_DEMO_TYPE_GPS_3D;
            min_valid_epo_hours = temp_hours;
            *max_epo_hours = epo_hours;
        }
        if (temp_hours == 0) {
            return;
        }
    }
    if ((cntx->support_type & EPO_DOWNLOAD_SUPPORT_BD2) != 0) {
        temp_hours = epo_download_get_3d_valid_hours(EPO_DEMO_TYPE_BDS_3D, cntx->trunk_num_3d, &epo_hours);
        if (min_valid_epo_hours > temp_hours) {
            cntx->type_3d = EPO_DEMO_TYPE_BDS_3D;
            min_valid_epo_hours = temp_hours;
            *max_epo_hours = epo_hours;
        }
        if (temp_hours == 0) {
            return;
        }
    }
    if ((cntx->support_type & EPO_DOWNLOAD_SUPPORT_GALILEO) != 0) {
        temp_hours = epo_download_get_3d_valid_hours(EPO_DEMO_TYPE_GA_3D, cntx->trunk_num_3d, &epo_hours);
        if (min_valid_epo_hours > temp_hours) {
            cntx->type_3d = EPO_DEMO_TYPE_GA_3D;
            min_valid_epo_hours = temp_hours;
            *max_epo_hours = epo_hours;
        }
        if (temp_hours == 0) {
            return;
        }
    }
    if (min_valid_epo_hours != 0x7fffffff) {
        *valid_epo_hours = min_valid_epo_hours;
    }
}

void epo_download_md5_check(char *file_name, uint8_t *buffer)
{
    LOC_SERV_LOGI("[EPO]MD5 check", 0);
    int32_t len;
    int32_t i;
    uint8_t digest[MD5_LEN] = {0};
    mbedtls_md5_context ctx;
    uint8_t buf[MD5_CHECK_LEN] = {0};
    uint8_t digits[MD5_LEN] = "0123456789abcdef";
    FILE *fd = fopen(file_name, "r");
    if (!fd) {
        LOC_SERV_LOGE("[EPO]Open file fail, %s", 1, file_name);
        return;
    }
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts_ret(&ctx);
    while ((len = fread(buf, 1, MD5_CHECK_LEN, fd))) {
        mbedtls_md5_update_ret(&ctx, buf, len);
    }
    if (fclose(fd) != 0) {
        LOC_SERV_LOGE("[EPO]epo_download_md5_check fclose fail", 0);
    }
    mbedtls_md5_finish_ret(&ctx, digest);
    mbedtls_md5_free(&ctx);
    for (i = 0; i < MD5_LEN; i++) {
        if (sprintf((char *)buffer + 2 * i, "%c%c", digits[(digest[i] >> 4) & 0xf], digits[digest[i] & 0xf]) < 0) {
            LOC_SERV_LOGE("[EPO]epo_download_md5_check get digits fail", 0);
        }
    }
    LOC_SERV_LOGI("[EPO]MD5:%s,%s", 2, file_name, buffer);
}

int epo_download_get_etag(char *file_name, uint8_t *buffer)
{
    char *pos;
    char buf[128];
    FILE *fd = fopen(file_name, "r");
    if (!fd) {
        LOC_SERV_LOGE("[EPO]Open file fail, %s", 1, file_name);
        return -1;
    }
    while (1) {
        if (fgets(buf, sizeof(buf), fd) == NULL) {
            if (fclose(fd) != 0) {
                LOC_SERV_LOGE("[EPO]epo_download_get_etag, fclose fail", 0);
            }
            return -1;
        }
        pos = strstr(buf, "ETag: ");
        if (pos == NULL) {
            continue;
        }
        memcpy(buffer, pos + 7, 32);
        LOC_SERV_LOGI("[EPO]ETag:%s,%s", 2, file_name, buffer);
        if (fclose(fd) != 0) {
            LOC_SERV_LOGE("[EPO]epo_download_get_etag, fclose fail", 0);
        }
        return 0;
    }
}

int32_t epo_download_http(epo_download_handle_t *handle)
{
    char url[URL_LEN];
    CURL *curl_handle;
    FILE *headerfile;
    FILE *bodyfile;
    char *headerfilename = "/tmp/head.out";
    char bodyfilename[128] = {0};
    char file_name[128] = {0};
    location_config_epo_struct_t epo_info = {0};
    CURLcode status;
    epo_download_get_filename(handle->type, handle->index, file_name);
    epo_download_get_filepath(handle->type, handle->index, bodyfilename);
    location_config_get_epo_info(&epo_info);
    if (snprintf(url, sizeof(url), "http://%s/%s?vendor=%s&project=%s&device_id=%s", 
            epo_info.epo_host,
            file_name,
            epo_info.epo_vendor,
            epo_info.epo_project,
            epo_info.epo_device_id) < 0) {
        LOC_SERV_LOGE("[EPO]epo_download_http get url fail", 0);
        return -1;
    }
    curl_handle = curl_easy_init();
    if (curl_handle == NULL) {
        LOC_SERV_LOGE("[EPO]epo_download_http curl_easy_init fail, %s", 1, file_name);
        return -1;
    }
    if (curl_easy_setopt(curl_handle, CURLOPT_URL, url) != 0) {
        LOC_SERV_LOGE("[EPO]epo_download_http curl_easy_setopt url fail", 0);
    }

    epo_mutex_lock();

    headerfile = fopen(headerfilename, "wb");
    if(!headerfile) {
        curl_easy_cleanup(curl_handle);
        LOC_SERV_LOGE("[EPO]epo_download_http fail, %s", 1, headerfilename);
        epo_mutex_unlock();
        return -1;
    }
    bodyfile = fopen(bodyfilename, "wb");
    if(!bodyfile) {
        curl_easy_cleanup(curl_handle);
        if (fclose(headerfile) != 0) {
            LOC_SERV_LOGE("[EPO]epo_download_http fclose fail", 0);
        }
        LOC_SERV_LOGE("[EPO]epo_download_http open fail, %s", 1, bodyfilename);
        epo_mutex_unlock();
        return -1;
    }
    if (curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, headerfile) != 0) {
        LOC_SERV_LOGE("[EPO]epo_download_http curl_easy_setopt headerfile fail", 0);
    }
    if (curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, bodyfile) != 0) {
        LOC_SERV_LOGE("[EPO]epo_download_http curl_easy_setopt bodyfile fail", 0);
    }
    status = curl_easy_perform(curl_handle);
    if (fclose(headerfile) != 0) {
        LOC_SERV_LOGE("[EPO]epo_download_http fclose fail", 0);
    }
    if (fclose(bodyfile) != 0) {
        LOC_SERV_LOGE("[EPO]epo_download_http fclose fail", 0);
    }

    epo_mutex_unlock();

    curl_easy_cleanup(curl_handle);
    if (status != 0) {
        LOC_SERV_LOGE("[EPO]epo_download_http download fail, %s, status: %d", 2, file_name, status);
        return -1;
    }
    uint8_t digest[33] = {0};
    uint8_t etag[33] = {0};
    if (epo_download_get_etag(headerfilename, etag) != 0) {
        LOC_SERV_LOGE("[EPO]epo_download_http get etag fail, please check the http status code: %s", 1, headerfilename);
        return -1;
    }
    epo_download_md5_check(bodyfilename, digest);
    if (memcmp(etag, digest, 32) != 0) {
        LOC_SERV_LOGE("[EPO]epo_download_http MD5 check fail, ETag:%s,MD5:%s", 2, etag, digest);
        unlink(bodyfilename);
        return -1;
    }
    LOC_SERV_LOGI("[EPO]epo_download_http MD5 check success, %s", 1, file_name);
    return 0;
}

void epo_download_handle_result(epo_download_handle_t *handle, int32_t result)
{ 
    LOC_SERV_LOGI("[EPO]epo_download_handle_result:[%d,%d] %d", 3, handle->type, handle->index, result);
    if ((result != 0) && (g_epo_download_retry_count < 4)) {
        if ((handle->type >= EPO_DEMO_TYPE_3D_START)
            &&(handle->type <= EPO_DEMO_TYPE_3D_END)) {
            if (g_epo_download_cntx.current == handle) {
                g_epo_download_cntx.current = NULL;
            }
            g_epo_download_cntx.list_3d = epo_download_push_handle(g_epo_download_cntx.list_3d, handle);
        }
        if (location_timer_is_active(g_epo_download_cntx.retry_timer) != 0) {
            location_timer_start(g_epo_download_cntx.retry_timer, EPO_DOWNLOAD_RETRY_NETWORK_TIMER(g_epo_download_retry_count));
            LOC_SERV_LOGI("[EPO]epo_download_handle_result,start retry timer:%d", 1, (EPO_DOWNLOAD_RETRY_NETWORK_TIMER(g_epo_download_retry_count)));
            g_epo_download_retry_count ++;
        }
    } else {
        if (g_epo_download_cntx.current == handle) {
            g_epo_download_cntx.current = NULL;
        }
        free(handle);
        g_epo_download_cntx.current = NULL;
        g_epo_download_retry_count = 0;
        epo_download_handle();
    }
}

void epo_download_handle()
{
    int32_t ret;
    if (location_timer_is_active(g_epo_download_cntx.retry_timer) == 0) {
        location_timer_stop(g_epo_download_cntx.retry_timer);
    }
    if (g_epo_download_cntx.list_3d != NULL) {
        g_epo_download_cntx.current = g_epo_download_cntx.list_3d;
        g_epo_download_cntx.list_3d = epo_download_remove_handle(g_epo_download_cntx.list_3d, g_epo_download_cntx.list_3d);
        ret = epo_download_http(g_epo_download_cntx.current);
        int32_t *state = (int32_t *)malloc(sizeof(int32_t));
        if (state != NULL) {
            *state = ret;
            location_service_task_send_message(LOC_SERVICE_MSG_EPO_DOWNLOAD, state, sizeof(int32_t));
        }
        epo_download_handle_result(g_epo_download_cntx.current, ret);
    } else {
        uint32_t expiry_time = 0;
        int32_t max_epo_hours = 0;
        int32_t valid_epo_hours = 0;
        if (location_timer_is_active(g_epo_download_cntx.next_download_timer) == 0) {
            return;
        }
        epo_download_get_3d_min_valid_hours(&g_epo_download_cntx, &valid_epo_hours, &max_epo_hours);
        LOC_SERV_LOGI("[EPO]valid:%d,max:%d", 2, valid_epo_hours, max_epo_hours);
        expiry_time = epo_download_get_next_download_time(valid_epo_hours, max_epo_hours);
        location_timer_start(g_epo_download_cntx.next_download_timer, expiry_time);
        LOC_SERV_LOGI("[EPO]epo_download_handle, start next download timer:%d", 1, expiry_time);
    }
}

void epo_download_timer_retry_network_func(union sigval value)
{
    epo_download_handle();
}

void epo_download_timer_next_download_3d_func(union sigval value)
{
    int32_t i;
    if ((g_epo_download_cntx.type_3d == EPO_DEMO_TYPE_BDS_3D) || ((g_epo_download_cntx.type_3d == EPO_DEMO_TYPE_GA_3D))) {
        epo_download_handle_t *handle = (epo_download_handle_t *)malloc(sizeof(epo_download_handle_t));
        if (handle != NULL) {
            handle->type = g_epo_download_cntx.type_3d;
            handle->index = 1;
            handle->next = NULL;
            g_epo_download_cntx.list_3d = epo_download_add_handle(g_epo_download_cntx.list_3d, handle);
        }
    } else {
        for (i = 0; i < g_epo_download_cntx.trunk_num_3d; i++) {
            epo_download_handle_t *handle = (epo_download_handle_t *)malloc(sizeof(epo_download_handle_t));
            if (handle != NULL) {
                handle->type = g_epo_download_cntx.type_3d;
                handle->index = i + 1;
                handle->next = NULL;
                g_epo_download_cntx.list_3d = epo_download_add_handle(g_epo_download_cntx.list_3d, handle);
            }
        }
    }
    epo_download_handle();
}

uint32_t epo_download_get_next_download_time(int32_t valid_hours, int32_t max_gnss_hours)
{
    uint32_t expiry_time;
    if ((valid_hours == 0) || (max_gnss_hours == 0)) {
        LOC_SERV_LOGI("[EPO]epo_download_get_next_download_time:%d [%d,%d]", 3, EPO_DOWNLOAD_FIRST_START_TIMER, valid_hours, max_gnss_hours);
        return  EPO_DOWNLOAD_FIRST_START_TIMER;
    }
    if (valid_hours > 72) {
        LOC_SERV_LOGE("[EPO]valid hours > 72, check local time or epo file!", 0);
        valid_hours = 72;
    }
    expiry_time = ((valid_hours - EPO_DOWNLOAD_EXPIRED_TIME) * 3600) + ((valid_hours & 0x07) * 60 * 7) + ((max_gnss_hours & 0x0f) * 4);

    if (expiry_time > 0) {
        /* add for random download timing... for avoid all device download on same time. */
        expiry_time += ((expiry_time & 0x07) + (valid_hours & 0x07)) * 3600;
        expiry_time *= 1000;
    } else {
        expiry_time = EPO_DOWNLOAD_RETRY_NETWORK_TIMER(g_epo_download_retry_count);
    }
    LOC_SERV_LOGI("[EPO]epo_download_get_next_download_time:%d [%d,%d]", 3, expiry_time, valid_hours, max_gnss_hours);
    return expiry_time;
}
#endif /* defined(LOCATION_SUPPORT_EPO_DOWNLOAD) */

void epo_download_init(uint8_t support_type, uint32_t trunk_num_3d)
{
    #if defined(LOCATION_SUPPORT_EPO_DOWNLOAD)
    int32_t valid_epo_hours = 0;
    int32_t max_epo_hours = 0;
    int32_t next_download_timer = EPO_DOWNLOAD_FIRST_START_TIMER;

    LOC_SERV_LOGI("[EPO]epo_download_init:%d,%d", 2, support_type, trunk_num_3d);

    if ((location_timer_is_active(g_epo_download_cntx.retry_timer) == 0) 
        || (location_timer_is_active(g_epo_download_cntx.next_download_timer) == 0)) {
        LOC_SERV_LOGI("[EPO]epo_download_init, already init",0);
        return;
    }
    g_epo_download_cntx.support_type = support_type;
    g_epo_download_cntx.type_3d = EPO_DEMO_TYPE_GR_3D;
    g_epo_download_cntx.trunk_num_3d = trunk_num_3d;

    epo_download_get_3d_min_valid_hours(&g_epo_download_cntx, &valid_epo_hours, &max_epo_hours);
    LOC_SERV_LOGI("[EPO]valid:%d,max:%d", 2, valid_epo_hours, max_epo_hours);
    if (valid_epo_hours > EPO_DOWNLOAD_EXPIRED_TIME) {
        next_download_timer = epo_download_get_next_download_time(valid_epo_hours, max_epo_hours);
    }
    g_epo_download_cntx.retry_timer = location_timer_create(epo_download_timer_retry_network_func);
    g_epo_download_cntx.next_download_timer = location_timer_create(epo_download_timer_next_download_3d_func);
    location_timer_start(g_epo_download_cntx.next_download_timer, next_download_timer);
    LOC_SERV_LOGI("[EPO]epo_download_init, start next download timer:%d", 1, next_download_timer);
    #endif /* defined(LOCATION_SUPPORT_EPO_DOWNLOAD) */
}

void epo_download_deinit()
{
    #if defined(LOCATION_SUPPORT_EPO_DOWNLOAD)
    LOC_SERV_LOGI("[EPO]epo_download_deinit", 0);
    while (g_epo_download_cntx.list_3d != NULL) {
        epo_download_handle_t *handle_p = g_epo_download_cntx.list_3d;
        g_epo_download_cntx.list_3d = epo_download_remove_handle(g_epo_download_cntx.list_3d, handle_p);
        free(handle_p);
    }
    if (g_epo_download_cntx.current != NULL) {
        epo_download_handle_result(g_epo_download_cntx.current, 0);
    }
    location_timer_delete(g_epo_download_cntx.retry_timer);
    location_timer_delete(g_epo_download_cntx.next_download_timer);
    g_epo_download_cntx.retry_timer = 0;
    g_epo_download_cntx.next_download_timer = 0;
    #endif /* defined(LOCATION_SUPPORT_EPO_DOWNLOAD) */
}
