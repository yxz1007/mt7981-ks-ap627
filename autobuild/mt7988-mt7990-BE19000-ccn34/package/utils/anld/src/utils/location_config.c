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
#include <stdlib.h>

#include "location_config.h"
#include "location_log.h"


#define LOCATION_CONFIG_UART_PORT_PATH                  "/dev/ttyS1"
#define LOCATION_CONFIG_UART_BAUDRATE                   (921600)
#define LOCATION_CONFIG_UART_FLOW_CONTROL               (0)
#define LOCATION_CONFIG_DEBUG_PORT_PATH                 " "
#define LOCATION_CONFIG_DEBUG_PORT_BAUDRATE             (921600)
#define LOCATION_CONFIG_DEBUG_PORT_FLOW_CONTROL         (0)
#if defined(LOCATION_SERIAL_PORT_TYPE_SPI)
#define LOCATION_CONFIG_SPI_PORT_PATH                   "/dev/spidev1.0"
#define LOCATION_CONFIG_SPI_BIT_ORDER                   (0)
#define LOCATION_CONFIG_SPI_FREQUENCY                   (24000000)
#define LOCATION_CONFIG_SPI_CPHA                        (0)
#define LOCATION_CONFIG_SPI_CPOL                        (0)
#endif /* defined(LOCATION_SERIAL_PORT_TYPE_SPI) */
#define LOCATION_CONFIG_POWER_CONTROL_GPIO              (999999)
#define LOCATION_CONFIG_RESET_GNSS_GPIO                 (999999)
#define LOCATION_CONFIG_NOTIFY_GNSS_GPIO                (999999)
#define LOCATION_CONFIG_NMEA_FILE_PATH                  "/etc/default/anld/location.dat"
#define LOCATION_CONFIG_LOG_PATH                        "/etc/default/anld/log.dat"
#if defined(LOCATION_CHIP_AG3352)
#define LOCATION_CONFIG_DA_PATH                         "/etc/default/anld/da/ag3352_da.bin"
#else
#define LOCATION_CONFIG_DA_PATH                         "/etc/default/anld/da/ag3335_da.bin"
#endif /* defined LOCATION_CHIP_AG3352 */
#define LOCATION_CONFIG_IMAGE_PATH                      "/etc/default/anld/load/flash_download.cfg"
#define LOCATION_CONFIG_EPO_FILE_PATH                   "/etc/default/anld/epo"
#define LOCATION_CONFIG_EPO_HOST                        "wpepodownload.mediatek.com"
#define LOCATION_CONFIG_EPO_VENDOR                      " "
#define LOCATION_CONFIG_EPO_PROJECT                     " "
#define LOCATION_CONFIG_EPO_DEVICEID                    " "

void location_config_get_path(char *path, const char *default_path, const char *name)
{
    int ret;
    FILE *config = fopen(LOCATION_CONFIG_FILE_PATH, "r");
    if (config == NULL) {
        LOC_SERV_LOGE("[CFG]location_config_get_path, open fail", 0);
        ret = snprintf(path, LOCATION_CONFIG_BUF_LENGTH, "%s", default_path);
        if (ret < 0) {
            LOC_SERV_LOGE("location_config_get_path get path fail", 0);
        }
        return;
    }
    char *pos;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), config) != NULL) {
        if ((pos = strstr(buffer, name)) != NULL) {
            if (strstr(pos, "\n") != NULL) {
                pos[strlen(pos) - 1] = '\0';
            }
            ret = snprintf(path, LOCATION_CONFIG_BUF_LENGTH, "%s", pos + strlen(name));
            if (ret < 0) {
                LOC_SERV_LOGE("location_config_get_path get path fail", 0);
            }
            if (fclose(config) != 0) {
                LOC_SERV_LOGE("[CFG]location_config_get_path, fclose fail", 0);
            }
            return;
        }
    }
    if (fclose(config) != 0) {
        LOC_SERV_LOGE("[CFG]location_config_get_path, fclose fail", 0);
    }
    ret = snprintf(path, LOCATION_CONFIG_BUF_LENGTH, "%s", default_path);
    if (ret < 0) {
        LOC_SERV_LOGE("location_config_get_path get path fail", 0);
    }
}

void location_config_get_num(int *num, int default_num, const char *name)
{
    FILE *config = fopen(LOCATION_CONFIG_FILE_PATH, "r");
    if (config == NULL) {
        LOC_SERV_LOGE("[CFG]location_config_get_path, open fail", 0);
        *num = default_num;
        return;
    }
    char *pos;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), config) != NULL) {
        if ((pos = strstr(buffer, name)) != NULL) {
            if (strstr(pos, "\n") != NULL) {
                pos[strlen(pos) - 1] = '\0';
            }
            *num = atoi(pos + strlen(name));
            if (fclose(config) != 0) {
                LOC_SERV_LOGE("[CFG]location_config_get_num, fclose fail", 0);
            }
            return;
        }
    }
    if (fclose(config) != 0) {
        LOC_SERV_LOGE("[CFG]location_config_get_num, fclose fail", 0);
    }
    *num = default_num;
}

void location_config_get_uart_port_path(char *path)
{
    location_config_get_path(path, LOCATION_CONFIG_UART_PORT_PATH, "UART port path=");
    LOC_SERV_LOGI("[CFG]UART path:%s", 1, path);
}

void location_config_get_uart_info(location_config_uart_struct_t *uart_info)
{
    location_config_get_path(uart_info->path, LOCATION_CONFIG_UART_PORT_PATH, "UART port path=");
    location_config_get_num(&uart_info->baudrate, LOCATION_CONFIG_UART_BAUDRATE, "UART baudrate=");
    location_config_get_num(&uart_info->flow_control, LOCATION_CONFIG_UART_FLOW_CONTROL, "UART flow control=");
    LOC_SERV_LOGI("[CFG]UART path:%s", 1, uart_info->path);
    LOC_SERV_LOGI("[CFG]UART baudrate:%d", 1, uart_info->baudrate);
    LOC_SERV_LOGI("[CFG]UART flow control:%d", 1, uart_info->flow_control);
}

void location_config_get_debug_port_info(location_config_uart_struct_t *debug_info)
{
    location_config_get_path(debug_info->path, LOCATION_CONFIG_DEBUG_PORT_PATH, "debug port path=");
    location_config_get_num(&debug_info->baudrate, LOCATION_CONFIG_DEBUG_PORT_BAUDRATE, "debug port baudrate=");
    location_config_get_num(&debug_info->flow_control, LOCATION_CONFIG_DEBUG_PORT_FLOW_CONTROL, "debug port flow control=");
    LOC_SERV_LOGI("[CFG]debug port path:%s", 1, debug_info->path);
    LOC_SERV_LOGI("[CFG]debug port baudrate:%d", 1, debug_info->baudrate);
    LOC_SERV_LOGI("[CFG]debug port flow control:%d", 1, debug_info->flow_control);
}

#if defined(LOCATION_SERIAL_PORT_TYPE_SPI)
void location_config_get_spi_info(location_config_spi_struct_t *spi_info)
{
    location_config_get_path(spi_info->path, LOCATION_CONFIG_SPI_PORT_PATH, "SPI port path=");
    location_config_get_num(&spi_info->frequencey, LOCATION_CONFIG_SPI_FREQUENCY, "SPI frequency=");
    location_config_get_num(&spi_info->bit_order, LOCATION_CONFIG_SPI_BIT_ORDER, "SPI bit order=");
    location_config_get_num(&spi_info->cpha, LOCATION_CONFIG_SPI_CPHA, "SPI CPHA=");
    location_config_get_num(&spi_info->cpol, LOCATION_CONFIG_SPI_CPOL, "SPI CPOL=");
    LOC_SERV_LOGI("[CFG]spi port path:%s", 1, spi_info->path);
    LOC_SERV_LOGI("[CFG]spi frequencey:%d", 1, spi_info->frequencey);
    LOC_SERV_LOGI("[CFG]spi bit order:%d", 1, spi_info->bit_order);
    LOC_SERV_LOGI("[CFG]spi cpha:%d", 1, spi_info->cpha);
    LOC_SERV_LOGI("[CFG]spi cpol:%d", 1, spi_info->cpol);
}
#endif /* defined(LOCATION_SERIAL_PORT_TYPE_SPI) */

void location_config_get_power_control_gpio(int *pin)
{
    location_config_get_num(pin, LOCATION_CONFIG_POWER_CONTROL_GPIO, "Power control gpio=");
    LOC_SERV_LOGI("[CFG]Power control gpio:%d", 1, *pin);
}

void location_config_get_reset_gnss_gpio(int *pin)
{
    location_config_get_num(pin, LOCATION_CONFIG_RESET_GNSS_GPIO, "Reset GNSS gpio=");
    LOC_SERV_LOGI("[CFG]Reset GNSS gpio:%d", 1, *pin);
}

void location_config_get_notify_gnss_gpio(int *pin)
{
    location_config_get_num(pin, LOCATION_CONFIG_NOTIFY_GNSS_GPIO, "Notify GNSS gpio=");
    LOC_SERV_LOGI("[CFG]Notify GNSS gpio:%d", 1, *pin);
}

void location_config_get_nmea_file_path(char *path)
{
    location_config_get_path(path, LOCATION_CONFIG_NMEA_FILE_PATH, "NMEA file path=");
    LOC_SERV_LOGI("[CFG]nmea path:%s", 1, path);
}

void location_config_get_log_path(char *path)
{
    location_config_get_path(path, LOCATION_CONFIG_LOG_PATH, "LOG path=");
    LOC_SERV_LOGI("[CFG]log path:%s", 1, path);
}

void location_config_get_da_path(char *path)
{
    location_config_get_path(path, LOCATION_CONFIG_DA_PATH, "DA path=");
    LOC_SERV_LOGI("[CFG]DA path:%s", 1, path);
}

void location_config_get_image_path(char *path)
{
    location_config_get_path(path, LOCATION_CONFIG_IMAGE_PATH, "image path=");
    LOC_SERV_LOGI("[CFG]image path:%s", 1, path);
}

void location_config_get_epo_file_path(char *path)
{
    location_config_get_path(path, LOCATION_CONFIG_EPO_FILE_PATH, "EPO file path=");
    LOC_SERV_LOGI("[CFG]EPO file path:%s", 1, path);
}

void location_config_get_epo_info(location_config_epo_struct_t *epo_info)
{
    location_config_get_path(epo_info->epo_host, LOCATION_CONFIG_EPO_HOST, "EPO host=");
    location_config_get_path(epo_info->epo_vendor, LOCATION_CONFIG_EPO_VENDOR, "EPO vendor=");
    location_config_get_path(epo_info->epo_project, LOCATION_CONFIG_EPO_PROJECT, "EPO project=");
    location_config_get_path(epo_info->epo_device_id, LOCATION_CONFIG_EPO_DEVICEID, "EPO device id=");
    LOC_SERV_LOGI("[CFG]EPO host:%s", 1, epo_info->epo_host);
    LOC_SERV_LOGI("[CFG]EPO vendor:%s", 1, epo_info->epo_vendor);
    LOC_SERV_LOGI("[CFG]EPO project:%s", 1, epo_info->epo_project);
    LOC_SERV_LOGI("[CFG]EPO device id:%s", 1, epo_info->epo_device_id);
}
