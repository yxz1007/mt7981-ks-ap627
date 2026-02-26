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

#include "location_log.h"
#include "location_config.h"
#include "location_uart.h"
#include "location_serial_port.h"

#if defined(LOCATION_SERIAL_PORT_TYPE_SPI)
#include "host_mux.h"
#define LOCATION_SPI_TRANSFER_MAX_SIZE     (4095)
static platform_bus_cntx_t g_location_mux_gnss_cntx = {PLATFORM_BUS_SPI, -1};
#else
static location_uart_cntx_t g_location_uart_gnss_cntx = {-1, 0};
#endif /* defined(LOCATION_SERIAL_PORT_TYPE_SPI) */

static location_uart_cntx_t g_location_uart_debug_cntx = {-1, 0};

#if defined(LOCATION_SUPPORT_NOTIFY_DATA_OUTPUT)
#include <unistd.h>
#include "location_gpio.h"
static int notify_gnss_wakeup_pin;

void location_serial_port_notify_data_output()
{
    location_gpio_set_output(notify_gnss_wakeup_pin,0);
    usleep(10000);
    location_gpio_set_output(notify_gnss_wakeup_pin,1);
}
#endif /* defined(LOCATION_SUPPORT_NOTIFY_DATA_OUTPUT) */

int location_serial_port_gnss_read(unsigned char *buf, unsigned int len)
{
    #if defined(LOCATION_SERIAL_PORT_TYPE_SPI)
    unsigned int receive_len;
    receive_len = len > LOCATION_SPI_TRANSFER_MAX_SIZE ? LOCATION_SPI_TRANSFER_MAX_SIZE : len;
    if (host_mux_rx(g_location_mux_gnss_cntx, buf, &receive_len) == HOST_MUX_STATUS_ERROR) {
        return 0;
    }
    return receive_len;
    #else
    static int mark = 0;
    return location_uart_read(buf, len, g_location_uart_gnss_cntx, &mark);
    #endif /* defined(LOCATION_SERIAL_PORT_TYPE_SPI) */
}

int location_serial_port_gnss_write(unsigned char *buf, unsigned int len)
{
    #if defined(LOCATION_SERIAL_PORT_TYPE_SPI)
    unsigned int send_len;
    send_len = len > LOCATION_SPI_TRANSFER_MAX_SIZE ? LOCATION_SPI_TRANSFER_MAX_SIZE : len;
    if (host_mux_tx(g_location_mux_gnss_cntx, buf, &send_len) == HOST_MUX_STATUS_ERROR) {
        return 0;
    }
    return send_len;
    #else
    #if defined(LOCATION_SUPPORT_NOTIFY_DATA_OUTPUT)
    location_serial_port_notify_data_output();
    #endif /* defined(LOCATION_SUPPORT_NOTIFY_DATA_OUTPUT) */
    return location_uart_write(buf, len, g_location_uart_gnss_cntx);
    #endif /* defined(LOCATION_SERIAL_PORT_TYPE_SPI) */
}

int location_serial_port_gnss_init()
{
    #if defined(LOCATION_SERIAL_PORT_TYPE_SPI)
    location_config_spi_struct_t spi_info;
    platform_bus_config_t config;
    location_config_get_spi_info(&spi_info);
    if (snprintf(config.spi_config.path, sizeof(config.spi_config.path), "%s", spi_info.path) < 0) {
        LOC_SERV_LOGE("location_serial_port_gnss_init, get path fail", 0);
    }
    config.spi_config.frequency = spi_info.frequencey;
    config.spi_config.bit_order = spi_info.bit_order;
    config.spi_config.cpha = spi_info.cpha;
    config.spi_config.cpol = spi_info.cpol;
    return host_mux_init(&g_location_mux_gnss_cntx, &config);
    #else
    location_config_uart_struct_t uart_info;
    location_config_get_uart_info(&uart_info);
    #if defined(LOCATION_SUPPORT_NOTIFY_DATA_OUTPUT)
    location_config_get_notify_gnss_gpio(&notify_gnss_wakeup_pin);
    #endif /* defined(LOCATION_SUPPORT_NOTIFY_DATA_OUTPUT) */
    return location_uart_init(&g_location_uart_gnss_cntx, &uart_info);
    #endif /* defined(LOCATION_SERIAL_PORT_TYPE_SPI) */
}

int location_serial_port_gnss_deinit()
{
    #if defined(LOCATION_SERIAL_PORT_TYPE_SPI)
    return host_mux_deinit(&g_location_mux_gnss_cntx);
    #else
    return location_uart_deinit(&g_location_uart_gnss_cntx.fd);
    #endif /* defined(LOCATION_SERIAL_PORT_TYPE_SPI) */
}

int location_serial_port_debug_read(unsigned char *buf, unsigned int len)
{
    static int mark = 0;
    return location_uart_read(buf, len, g_location_uart_debug_cntx, &mark);
}

int location_serial_port_debug_write(unsigned char *buf, unsigned int len)
{
    return location_uart_write(buf, len, g_location_uart_debug_cntx);
}

int location_serial_port_debug_init()
{
    location_config_uart_struct_t debug_info;
    location_config_get_debug_port_info(&debug_info);
    return location_uart_init(&g_location_uart_debug_cntx, &debug_info);
}

int location_serial_port_debug_deinit()
{
    return location_uart_deinit(&g_location_uart_debug_cntx.fd);
}

