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
#ifndef _PORTABLE_HOST_MUX_H_
#define _PORTABLE_HOST_MUX_H_
#include "host_mux_types.h"
#include <stdio.h>

#define     LOG_MUX_SPIM_I(fmt, cnt, ...)      printf(fmt, ##__VA_ARGS__)
#define     LOG_MUX_SPIM_W(fmt, cnt, ...)      printf(fmt, ##__VA_ARGS__)
#define     LOG_MUX_SPIM_E(fmt, cnt, ...)      printf(fmt, ##__VA_ARGS__)


typedef enum {
    PLATFORM_BUS_I2C = 0,
    PLATFORM_BUS_SPI,
} platform_bus_type_t;

typedef struct {
    platform_bus_type_t type;
    int fd;
} platform_bus_cntx_t;

typedef struct {
    uint32_t  frequency;
    uint8_t   bit_order; /*0:lsb; 1:msb*/
    uint8_t   cpha;      /*0:CPHA0; 1:CPHA1*/
    uint8_t   cpol;      /*0:CPOL0; 1:CPOL1*/
    char      path[128];
} platform_bus_spi_config_t;


typedef struct {
    uint32_t   frequency; /*0:lsb; 1:msb*/
} platform_bus_i2c_config_t;


typedef union {
    platform_bus_spi_config_t spi_config;
    platform_bus_i2c_config_t i2c_config;
} platform_bus_config_t;


typedef struct {
    uint8_t *rx_buff;
    uint8_t *tx_buff;
    uint32_t rx_len;
    uint32_t tx_len;
    uint8_t  priv_data;  /* for I2C, this field is slave address*/
} platform_bus_transfer_t;

void    *port_platform_malloc(int size);
void    port_platform_free(void *pv);
int     port_platform_bus_init(platform_bus_type_t type, platform_bus_config_t *config);
int     port_platform_bus_deinit(int  handle);
int     port_platform_bus_transfer(int  handle, platform_bus_transfer_t *xfer);
int     port_platform_delay_us(uint32_t us);
int     port_platform_delay_ms(uint32_t ms);
void   *port_platform_create_semphore();
void    port_platform_destroy_samphore(void **handle);
int     port_platform_give_samphore(void *handle);
int     port_platform_take_samphore(void *handle, uint32_t timeout);
void   *port_platform_create_mutex();
void    port_platform_destroy_mutex(void **handle);
int     port_platform_give_mutex(void *handle);
int     port_platform_take_mutex(void *handle);



#endif
