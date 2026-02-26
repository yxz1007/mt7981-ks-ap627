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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include "location_log.h"
#include "location_gpio.h"


int location_gpio_config(int pin, int dir, int value, int edge)
{
    int fd;
    int ret;
    char path[64];
    char pin_str[10];
    char dir_str[10];
    char dir_ori[10];
    char value_str[10];
    char edge_str[10];
    char edge_ori[10];
    ret = sprintf(pin_str, "%d", pin);
    if (ret < 0) {
        LOC_SERV_LOGE("[GPIO]location_gpio_config get pin fail", 0);
        return -1;
    }

    if (dir != -1) {
        if (dir == LOCATION_GPIO_DIR_OUTPUT) {
            ret = sprintf(dir_str, "out");
            if (ret < 0) {
                LOC_SERV_LOGE("[GPIO]location_gpio_config get dir out fail", 0);
                return -1;
            }
        } else {
            ret = sprintf(dir_str, "in");
            if (ret < 0) {
                LOC_SERV_LOGE("[GPIO]location_gpio_config get dir in fail", 0);
                return -1;
            }
        }
        ret = snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
        if (ret < 0) {
            LOC_SERV_LOGE("[GPIO]location_gpio_config get dir path fail", 0);
            return -1;
        }
        fd = open(path, O_RDWR);
        if (fd < 0) {
            LOC_SERV_LOGE("[GPIO]location_gpio_config open fail: direction", 0);
            return -1;
        }
        memset(dir_ori, 0, sizeof(dir_ori));
        ret = read(fd, dir_ori, sizeof(dir_ori) - 1);
        dir_ori[sizeof(dir_ori) - 1] = '\0';
        dir_ori[strlen(dir_ori) - 1] = '\0';
        if ((ret < 0) || (strcmp(dir_ori, dir_str) != 0)) {
            LOC_SERV_LOGE("[GPIO]%s", 1, dir_str);
            if (write(fd, dir_str, strlen(dir_str)) < 0) {
                LOC_SERV_LOGE("[GPIO]location_gpio_config write fail: direction", 0);
                close(fd);
                return -1;
            }
        }
        close(fd);
    }

    if (value != -1) {
        ret = sprintf(value_str, "%d", value);
        if (ret < 0) {
            LOC_SERV_LOGE("[GPIO]location_gpio_config get vaule fail", 0);
            return -1;
        }
        ret = snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
        if (ret < 0) {
            LOC_SERV_LOGE("[GPIO]location_gpio_config get value path fail", 0);
            return -1;
        }
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            LOC_SERV_LOGE("[GPIO]location_gpio_config open fail: value", 0);
            return -1;
        }
        if (write(fd, value_str, strlen(value_str)) < 0) {
            LOC_SERV_LOGE("[GPIO]location_gpio_config write fail: value", 0);
            close(fd);
            return -1;
        }
        close(fd);
    }

    if (edge != -1) {
        switch (edge) {
            case LOCATION_GPIO_EDGE_NONE:
                ret = sprintf(edge_str, "none");
                break;
            case LOCATION_GPIO_EDGE_RISING:
                ret = sprintf(edge_str, "rising");
                break;
            case LOCATION_GPIO_EDGE_FALLING:
                ret = sprintf(edge_str, "falling");
                break;
            case LOCATION_GPIO_EDGE_BOTH:
                ret = sprintf(edge_str, "both");
                break;
            default:
                ret = sprintf(edge_str, "none");
                break;
        }
        if (ret < 0) {
            LOC_SERV_LOGE("[GPIO]location_gpio_config get edge fail", 0);
            return -1;
        }
        ret = snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/edge", pin);
        if (ret < 0) {
            LOC_SERV_LOGE("[GPIO]location_gpio_config get edge path fail", 0);
            return -1;
        }
        fd = open(path, O_RDWR);
        if (fd < 0) {
            LOC_SERV_LOGE("[GPIO]location_gpio_config open fail: edge", 0);
            return -1;
        }
        memset(edge_ori, 0, sizeof(edge_ori));
        ret = read(fd, edge_ori, sizeof(edge_ori) - 1);
        edge_ori[sizeof(edge_ori) - 1] = '\0';
        edge_ori[strlen(edge_ori) - 1] = '\0';
        if ((ret < 0) || (strcmp(edge_ori, edge_str) != 0)) {
            LOC_SERV_LOGE("[GPIO]%s", 1, edge_str);
            if (write(fd, edge_str, strlen(edge_str)) < 0) {
                LOC_SERV_LOGE("[GPIO]location_gpio_config write fail: edge", 0);
                close(fd);
                return -1;
            }
        }
        close(fd);
    }
    return 0;
}

int location_gpio_export(int pin)
{
    int fd;
    char pin_str[10];
    char path[64];
    int ret;
    ret = sprintf(path, "/sys/class/gpio/gpio%d", pin);
    if (ret < 0) {
        LOC_SERV_LOGE("[GPIO]location_gpio_export get path fail", 0);
        return -1;
    }
    if (access(path, F_OK) == 0) {
        return 0;
    }
    ret = sprintf(pin_str, "%d", pin);
    if (ret < 0) {
        LOC_SERV_LOGE("[GPIO]location_gpio_export get pin fail", 0);
        return -1;
    }
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        LOC_SERV_LOGE("[GPIO]location_gpio_export open fail: export", 0);
        return -1;
    }
    if (write(fd, pin_str, strlen(pin_str)) < 0) {
        LOC_SERV_LOGE("[GPIO]location_gpio_export write fail: export", 0);
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int location_gpio_unexport(int pin)
{
    int fd;
    char pin_str[10];
    char path[64];
    int ret;
    ret = sprintf(path, "/sys/class/gpio/gpio%d", pin);
    if (ret < 0) {
        LOC_SERV_LOGE("[GPIO]location_gpio_unexport get path fail", 0);
        return -1;
    }
    if (access(path, F_OK) != 0) {
        LOC_SERV_LOGI("[GPIO]location_gpio_unexport, not exist", 0);
        return 0;
    }
    ret = sprintf(pin_str, "%d", pin);
    if (ret < 0) {
        LOC_SERV_LOGE("[GPIO]location_gpio_unexport get pin fail", 0);
        return -1;
    }
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        LOC_SERV_LOGE("[GPIO]location_gpio_unexport open fail: unexport", 0);
        return -1;
    }
    if (write(fd, pin_str, strlen(pin_str)) < 0) {
        LOC_SERV_LOGE("[GPIO]location_gpio_unexport write fail: unexport", 0);
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int location_gpio_set_output(int pin, int value)
{
    if (location_gpio_export(pin) != 0) {
        return -1;
    }
    return location_gpio_config(pin, LOCATION_GPIO_DIR_OUTPUT, value, -1);
}

int location_gpio_set_direction(int pin, location_gpio_dir_t dir)
{
    if (location_gpio_export(pin) != 0) {
        return -1;
    }
    return location_gpio_config(pin, dir, -1, -1);
}

int location_gpio_set_edge(int pin, location_gpio_edge_t edge)
{
    if (location_gpio_export(pin) != 0) {
        return -1;
    }
    return location_gpio_config(pin, -1, -1, edge);
}