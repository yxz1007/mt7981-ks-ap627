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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "port_platform_interface.h"
#include "location_spi.h"

void *port_platform_malloc(int size)
{
    return malloc(size);
}

void port_platform_free(void *pv)
{
    free(pv);
}

int port_platform_bus_init(platform_bus_type_t type, platform_bus_config_t *config)
{
    if (type == PLATFORM_BUS_SPI) {
        int fd = -1;
        location_spi_config_struct_t spi_config;
        spi_config.mode = config->spi_config.cpha * (0x01) + config->spi_config.cpol * (0x02);
        spi_config.lsb_first = (config->spi_config.bit_order == 0) ? 1 : 0;
        spi_config.bits_per_word = 8;
        spi_config.speed = config->spi_config.frequency;
        if (snprintf(spi_config.path, sizeof(spi_config.path), "%s", config->spi_config.path) < 0) {
            LOG_MUX_SPIM_E("port_platform_bus_init, get path fail\n", 0);
        }
        if (location_spi_init(&fd, &spi_config) == -1) {
            return -1;
        }
        return fd;
    }
    return -1;
}

int port_platform_bus_deinit(int  handle)
{
    return location_spi_deinit(&handle);
}

int port_platform_bus_transfer(int  handle, platform_bus_transfer_t *xfer)
{
    uint32_t len;
    int ret;
    len = xfer->rx_len > xfer->tx_len ? xfer->rx_len : xfer->tx_len;
    ret = location_spi_transfer(handle, xfer->tx_buff, xfer->rx_buff, len);
    if (ret > 0) {
        return 0;
    }
    return -1;
}

int port_platform_delay_us(uint32_t us)
{
    return usleep(us);
}

int port_platform_delay_ms(uint32_t ms)
{
    return usleep(ms * 1000);
}


void *port_platform_create_semphore()
{
    sem_t *sem = (sem_t *)malloc(sizeof(sem_t));
    if (sem == NULL) {
        return NULL;
    }
    if (sem_init(sem, 0, 1) != 0) {
        free(sem);
        return NULL;
    }
    return (void *)sem;
}

int port_platform_give_samphore(void *handle)
{
    if (NULL ==  handle) {
        return -1;
    }
    return sem_post((sem_t *)handle);
}

int port_platform_take_samphore(void *handle, uint32_t timeout)
{
    if (NULL ==  handle) {
        return -1;
    }
    return sem_wait((sem_t *)handle);
}

void port_platform_destroy_samphore(void **handle)
{
    if (NULL == *handle) {
        return;
    }
    if (sem_destroy((sem_t *)*handle) == 0) {
        free(*handle);
        *handle = NULL;
    }
}

void *port_platform_create_mutex()
{
    pthread_mutex_t *mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (mutex == NULL) {
        return NULL;
    }
    if (pthread_mutex_init(mutex, NULL) != 0) {
        free(mutex);
        return NULL;
    }
    return (void *)mutex;
}

int port_platform_give_mutex(void *handle)
{
    if (NULL ==  handle) {
        return -1;
    }
    return pthread_mutex_unlock((pthread_mutex_t *)handle);
}

int port_platform_take_mutex(void *handle)
{
    if (NULL ==  handle) {
        return -1;
    }
    return pthread_mutex_lock((pthread_mutex_t *)handle);
}

void port_platform_destroy_mutex(void **handle)
{
    if (NULL == *handle) {
        return;
    }
    if (pthread_mutex_destroy((pthread_mutex_t *)*handle) == 0) {
        free(*handle);
        *handle = NULL;
    }
}