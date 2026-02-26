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
#include <stdint.h>
#include <stdio.h>

#include "location_spi.h"
#include "location_log.h"

/* SPI MODE from linux/spi/spidev.h
* SPI_CPHA		0x01
* SPI_CPOL		0x02
*
* SPI_MODE_0        (0|0)
* SPI_MODE_1        (0|SPI_CPHA)
* SPI_MODE_2        (SPI_CPOL|0)
* SPI_MODE_3        (SPI_CPOL|SPI_CPHA)
*
* SPI_CS_HIGH       0x04
* SPI_LSB_FIRST     0x08
* SPI_3WIRE         0x10
* SPI_LOOP          0x20
* SPI_NO_CS         0x40
* SPI_READY         0x80
* SPI_TX_DUAL       0x100
* SPI_TX_QUAD       0x200
* SPI_RX_DUAL       0x400
* SPI_RX_QUAD       0x800
*/

int location_spi_config(int fd, location_spi_config_struct_t *config)
{
    int ret;

    ret = ioctl(fd, SPI_IOC_WR_MODE32, &config->mode);
    if (ret == -1) {
        LOC_SERV_LOGE("[spi]can't set spi mode: %d", 1, config->mode);
        return -1;
    }

    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &config->bits_per_word);
    if (ret == -1) {
        LOC_SERV_LOGE("[spi]can't set bits per word: %d", 1, config->bits_per_word);
        return -1;
    }

    ret = ioctl(fd, SPI_IOC_WR_LSB_FIRST, &config->lsb_first);
    if (ret == -1) {
        LOC_SERV_LOGE("[spi]can't set LSB or MSB: %d", 1, config->lsb_first);
        return -1;
    }

    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &config->speed);
    if (ret == -1) {
        LOC_SERV_LOGE("[spi]can't set max speed hz: %d", 1, config->speed);
        return -1;
    }

    return 0;
}

int location_spi_transfer(int fd, uint8_t const *tx, uint8_t const *rx, uint32_t len)
{
    int ret;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = len,
    };
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0) {
        LOC_SERV_LOGE("[spi]can't send spi message", 0);
        return -1;
    }
    return ret;
}

int location_spi_read(int fd, uint8_t *buf, uint32_t len)
{
    return read(fd, buf, len);
}

int location_spi_write(int fd, uint8_t const *buf, uint32_t len)
{
    return write(fd, buf, len);
}

int location_spi_init(int *fd, location_spi_config_struct_t *config)
{
    *fd = open(config->path, O_RDWR);
    if (*fd < 0) {
        LOC_SERV_LOGE("[spi]location_spi_init, open fail", 0);
        return -1;
    }
    if (location_spi_config(*fd, config) == -1) {
        close(*fd);
        return -1;
    }
    return 0;
}

int location_spi_deinit(int *fd)
{
    return close(*fd);
}