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
#include <fcntl.h>      /* File control definitions */
#include <unistd.h>     /* UNIX standard function definitions */
#include <termios.h>    /* POSIX terminal control definitions */
#include <stdlib.h>

#include "location_log.h"
#include "location_uart.h"

#define LOCATION_UART_MAX_BUFFER_SIZE    (1024 * 15)

speed_t location_uart_get_baudrate(int baudrate)
{
    speed_t baud;
    switch (baudrate) {
        case 110:
            baud = B110;
            break;
        case 300:
            baud = B300;
            break;
        case 1200:
            baud = B1200;
            break;
        case 2400:
            baud = B2400;
            break;
        case 4800:
            baud = B4800;
            break;
        case 9600:
            baud = B9600;
            break;
        case 19200:
            baud = B19200;
            break;
        case 38400:
            baud = B38400;
            break;
        case 57600:
            baud = B57600;
            break;
        case 115200:
            baud = B115200;
            break;
        case 230400:
            baud = B230400;
            break;
        case 460800:
            baud = B460800;
            break;
        case 921600:
            baud = B921600;
            break;
        default:
            baud = B115200;
            break;
    }
    return baud;
}

int location_uart_flowctrl_encode(unsigned char *buf, unsigned int len, unsigned char *dest_buf)
{
    if (buf == NULL || len == 0 || dest_buf == NULL) {
        return 0;
    }

    int i = 0;
    int j = 0;
    int dest_len = len;

    for (i = 0; i < len; i++) {
        if (buf[i] == 0x77) {                            
            dest_buf[j] = 0x77;                
            dest_buf[j + 1] = 0x88;               
            j += 2;
            dest_len++;
        } else if (buf[i] == 0x13) {
            dest_buf[j] = 0x77;                
            dest_buf[j + 1] = 0xEC;               
            j += 2;
            dest_len++;         
        } else if (buf[i] == 0x11) {     
            dest_buf[j] = 0x77;
            dest_buf[j + 1] = 0xEE;
            j += 2;
            dest_len++;         
        } else {
            dest_buf[j] = buf[i];             
            j++;
        }
    }
    return dest_len;
}

int location_uart_flowctrl_decode(unsigned char *buf, unsigned int len, int *mark)
{
    if (buf == NULL || len == 0) {
        return 0;
    }

    int i = 0;
    int j = 0;

    int dest_len = len;
    if (*mark == 1) {
        dest_len++;
    }
    for (i = 0; i < len; i++) {
        if (buf[i] == 0x77) {
            *mark = 1;
            continue;
        }
        if (*mark) {
            if (buf[i] == 0x88) {
                buf[j] = 0x77;
                dest_len--;
            } else if (buf[i] == 0xEE) {
                buf[j] = 0x11;
                dest_len--;
            } else if (buf[i] == 0xEC) {
                buf[j] = 0x13;
                dest_len--;
            } else {
                LOC_SERV_LOGE("[UART]unrecognized char 0x%x", 1, buf[i]);
            }
            *mark = 0;
        } else {
            buf[j] = buf[i];
        }
        j++;
    }
    if (*mark == 1) {
        dest_len--;
    }
    memset(&buf[dest_len], 0, len - dest_len);
    return dest_len;
}

int location_uart_config(int fd, int baudrate, int flow_control)
{
    struct termios options;
    if (tcgetattr(fd, &options) != 0) {
        LOC_SERV_LOGE("[UART]location_uart_config, get options error", 0);
        return -1;
    }
    cfsetspeed(&options, location_uart_get_baudrate(baudrate));                      /* set baudrate */
    if (fcntl(fd, F_SETFL, 0) == -1) {
        LOC_SERV_LOGW("[UART]location_uart_config, set blocking fail", 0);
    }
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | NOFLSH | ISIG);
    options.c_oflag &= ~(ONLCR | OPOST);
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
    options.c_cc[VMIN]  = 1;
    options.c_cc[VTIME] = 0;
    if (flow_control == 0) {
        options.c_cflag &= ~CRTSCTS;
        options.c_iflag &= ~(IXON | IXOFF);
    } else if (flow_control == 1) {
        options.c_cflag &= ~CRTSCTS;
        options.c_iflag |= (IXON | IXOFF);
        options.c_cc[VSTART] = 0x11;
        options.c_cc[VSTOP] = 0x13;
    } else if (flow_control == 2) {
        options.c_cflag |= CRTSCTS;
        options.c_iflag &= ~(IXON | IXOFF);
    }

    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        LOC_SERV_LOGE("[UART]location_uart_config, set uart error", 0);
        return -1;
    }
    return 0;
}

int location_uart_read(unsigned char *buf, unsigned int len, location_uart_cntx_t uart, int *mark)
{
    int ret;
    if (uart.fd == -1) {
        LOC_SERV_LOGE("[UART]location_uart_read fail, uart not open", 0);
        return 0;
    }
    ret = read(uart.fd, buf, len);
    if (ret <= 0 || uart.flow_control != 1) {
        return ret;
    }
    return location_uart_flowctrl_decode(buf, ret, mark);
}

int location_uart_write(unsigned char *buf, unsigned int len, location_uart_cntx_t uart)
{
    if (uart.fd == -1) {
        LOC_SERV_LOGE("[UART]location_uart_write fail, uart not open", 0);
        return 0;
    }
    if (uart.flow_control == 1) {
        unsigned char *dest_buf = (unsigned char *)malloc(len * 2 > LOCATION_UART_MAX_BUFFER_SIZE ? LOCATION_UART_MAX_BUFFER_SIZE : len * 2);
        if (dest_buf == NULL) {
            LOC_SERV_LOGE("location_uart_write, malloc fail", 0);
            return -1;
        }
        len = location_uart_flowctrl_encode(buf, len, dest_buf);
        len = write(uart.fd, dest_buf, len);
        free(dest_buf);
        return len;
    }
    return write(uart.fd, buf, len);
}

int location_uart_init(location_uart_cntx_t *uart, location_config_uart_struct_t *uart_info)
{
    if (uart->fd != -1) {
        LOC_SERV_LOGE("[UART]location_uart_init, uart exist", 0);
        return 0;
    }
    uart->fd = open(uart_info->path, O_RDWR | O_NOCTTY | O_NDELAY);   /* O_NOCTTY: doesn't want to be the "controlling terminal".
                                                               * O_NDELAY: doesn't care what state the DCD signal line is in - whether the other end of the port is up and running.*/
    uart->flow_control = uart_info->flow_control;
    if (uart->fd == -1) {
        LOC_SERV_LOGE("[UART]location_uart_init, open fail:%s", 1, uart_info->path);
        return -1;
    } else {
        if (location_uart_config(uart->fd, uart_info->baudrate, uart_info->flow_control) != 0) {
            close(uart->fd);
            uart->fd = -1;
            return -1;
        }
    }
    return 0;
}

int location_uart_deinit(int *fd)
{
    int ret;
    if (*fd == -1) {
        return 0;
    }
    ret =  close(*fd);
    if (ret == -1) {
        LOC_SERV_LOGE("[UART]location_uart_deinit, fail", 0);
        return -1;
    }
    *fd = -1;
    return 0;
}
