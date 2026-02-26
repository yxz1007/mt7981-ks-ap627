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

#ifndef LOCATION_CONFIG_H
#define LOCATION_CONFIG_H

/* configure GNSS chip type:3335 or 3352 */
//#define LOCATION_CHIP_AG3335
#define LOCATION_CHIP_AG3352

/* host download configuration */
#define LOCATION_DOWNLOAD_FLASH_BAUDRATE                (921600)
#define LOCATION_DOWNLOAD_DA_SUPPORT_921600

/* gnss port type */
#define LOCATION_SERIAL_PORT_TYPE_UART
//#define LOCATION_SERIAL_PORT_TYPE_SPI

#define LOCATION_SUPPORT_NOTIFY_DATA_OUTPUT
#define LOCATION_SUPPORT_EPO_DOWNLOAD

#define LOCATION_CONFIG_FILE_PATH                       "/etc/default/anld/anld.conf"

#define LOCATION_CONFIG_BUF_LENGTH  128


typedef struct {
    char path[LOCATION_CONFIG_BUF_LENGTH];
    int baudrate;
    int flow_control;  /* 0: disable flow control; 1: enable SW flow control; 2: enable HW flow control. */
} location_config_uart_struct_t;

typedef struct {
    char path[128];
    int frequencey;
    int bit_order; /*0:lsb; 1:msb*/
    int cpha;      /*0:CPHA0; 1:CPHA1*/
    int cpol;      /*0:CPOL0; 1:CPOL1*/
} location_config_spi_struct_t;

typedef struct {
    char epo_host[LOCATION_CONFIG_BUF_LENGTH];
    char epo_vendor[LOCATION_CONFIG_BUF_LENGTH];
    char epo_project[LOCATION_CONFIG_BUF_LENGTH];
    char epo_device_id[LOCATION_CONFIG_BUF_LENGTH];
} location_config_epo_struct_t;

void location_config_get_uart_port_path(char *path);
void location_config_get_uart_info(location_config_uart_struct_t *uart_info);
void location_config_get_debug_port_info(location_config_uart_struct_t *debug_info);
#if defined(LOCATION_SERIAL_PORT_TYPE_SPI)
void location_config_get_spi_info(location_config_spi_struct_t *spi_info);
#endif /* defined(LOCATION_SERIAL_PORT_TYPE_SPI) */
void location_config_get_power_control_gpio(int *pin);
void location_config_get_reset_gnss_gpio(int *pin);
void location_config_get_notify_gnss_gpio(int *pin);
void location_config_get_nmea_file_path(char *path);
void location_config_get_log_path(char *path);
void location_config_get_da_path(char *path);
void location_config_get_image_path(char *path);
void location_config_get_epo_file_path(char *path);
void location_config_get_epo_info(location_config_epo_struct_t *epo_info);

#endif /*LOCATION_CONFIG_H*/
