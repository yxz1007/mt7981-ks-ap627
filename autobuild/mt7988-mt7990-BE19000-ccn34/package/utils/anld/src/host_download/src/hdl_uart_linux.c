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

#include "hdl_physical.h"
#if defined(LOCATION_SERIAL_PORT_TYPE_UART) || !defined(LOCATION_SERIAL_PORT_TYPE_SPI)
#ifdef HDL_ON_LINUX

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "time_utils.h"
#include "utils.h"

static int mUartFd = 0;

extern HDL_CONFIG g_hdl_config;

bool btrom_uart_read(void *buffer, dSize_t length);

bool HDL_COM_Init()
{
    struct termios tty_config;

	int fd = open(g_hdl_config.io_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
	{
        LOG_E("open %s error: %s", g_hdl_config.io_path, strerror(errno));
        return false;
    }

    int result = tcgetattr(fd, &tty_config);
    if (result != 0)
	{
        LOG_E("tcgetattr failed!%d,%s", result, strerror(errno));
        close(fd);
        return false;
    }

    result = cfsetispeed(&tty_config, B115200);
    if (result != 0)
	{
        LOG_E("cfsetispeed failed!%d,%s", result, strerror(errno));
        close(fd);
        return false;
    }
    result = cfsetospeed(&tty_config, B115200);
    if (result != 0)
	{
        LOG_E("cfsetospeed failed!%d,%s", result, strerror(errno));
        close(fd);
        return false;
    }

    tty_config.c_lflag &=
        ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | NOFLSH | ISIG);
    tty_config.c_oflag &= ~(ONLCR | OPOST);
    tty_config.c_iflag &=
		~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
    tty_config.c_cflag |= CS8;
    tty_config.c_cflag &= (~CRTSCTS);

    result = tcsetattr(fd, TCSANOW, &tty_config);
    if (result != 0)
	{
        LOG_E("tcsetattr failed!%d,%s", result, strerror(errno));
        close(fd);
        return false;
    }

    mUartFd = fd;

	tcflush(fd, TCIOFLUSH);

    return true;
}

bool HDL_COM_Deinit()
{
    return close(mUartFd) == 0;
}


bool HDL_COM_SetBaudRate(uint32_t baud_rate)
{
    LOG_I("Set baudrate: %d ", baud_rate);

    speed_t speed;
    if(baud_rate == 115200)
    {
        speed = B115200;
    }
    else if(baud_rate == 921600)
    {
        speed = B921600;
    }
    else if(baud_rate == 3000000)
    {
        speed = B3000000;
    }
    else
    {
        LOG_E("Unsupport baudrate: %d ", baud_rate);
        return false;
    }

    struct termios tty_config;

    int result = tcgetattr(mUartFd, &tty_config);
    if (result != 0)
	{
        LOG_E("tcgetattr failed!%d,%s", result, strerror(errno));
        return false;
    }
    result = cfsetispeed(&tty_config, speed);
    if (result != 0)
	{
        LOG_E("cfsetispeed failed!%d,%s", result, strerror(errno));
        return false;
    }
    result = cfsetospeed(&tty_config, speed);
    if (result != 0)
	{
        LOG_E("cfsetospeed failed!%d,%s", result, strerror(errno));
        return false;
    }

    result = tcsetattr(mUartFd, TCSADRAIN, &tty_config);
    if (result != 0)
	{
        LOG_E("tcsetattr failed!%d,%s", result, strerror(errno));
        return false;
    }
    tcflush(mUartFd, TCIOFLUSH);
	LOG_I("Set baudrate Pass: %d ", baud_rate);
    return true;
}

//bool HDL_COM_GetByte(uint8_t *data)
//{
//    if (data == NULL)
//	{
//		return false;
//	}
//
//
//    // for u8 , because of handshake, we use timeout machanism
//    //not support uart flow control.
//    int res;
//    uint8_t buf[1];
//    // Step 1: Try to Read
//    res = read(mUartFd, buf, 1);
//    if (res == 1)
//    {
//        *data = buf[0];
//        return true;
//    }
//
//    // Step 2: if no responde, just wait and read again
//    struct pollfd poll_fd;
//    poll_fd.fd = mUartFd;
//    poll_fd.events = POLLIN;
//    int poll_ret = poll(&poll_fd, 1, 50);
//    if (poll_ret <= 0)
//    {
//        //LOG_E("read U8 timeout,%d", errno);
//        return false;
//    }
//
//    res = read(mUartFd, buf, 1);
//    if (res == 1)
//    {
//        *data = buf[0];
//        return true;
//    }
//    return false;
//}

uint32_t HDL_COM_GetByte_Buffer(void *buf, uint32_t length, bool sw_flow_ctrl_open)
{
    if (buf == NULL || length == 0)
    {
        return 0;
    }

    uint8_t *data = (uint8_t *)buf;

    //dSize_t has_read = 0;

    int bytes = read(mUartFd, data, length);
    if(bytes < 0 ) return 0;
    uint32_t real_data_length = bytes;
    if (sw_flow_ctrl_open && real_data_length > 0)
    {
        if (data[bytes - 1] == 0x77)
        {
            portTaskDelayMs(20);
            int bytes_more = read(mUartFd, data + bytes, 1);
            if (bytes_more > 0)
            {
                bytes += 1;
            }
        }
        real_data_length = flowctrl_decode(data, bytes);
    }

    return real_data_length;
}

uint8_t flowctrl_buf_linux[FLOW_CTRL_BUF_SIZE] = {0};
bool HDL_COM_PutByte_Buffer(const void *buf, uint32_t length, bool sw_flow_ctrl_open)
{
    if (buf == NULL || length == 0)
	{
		return false;
	}

	int real_data_lenth = length;
	const uint8_t *real_data = (const uint8_t *)buf;

    if (sw_flow_ctrl_open)
    {
        real_data_lenth = flowctrl_encode(real_data, length, flowctrl_buf_linux);
        real_data = flowctrl_buf_linux;
    }

	pTime_t current = getSystemTickMs();
    dSize_t has_write = 0;
    while (has_write < real_data_lenth &&
		(getSystemTickMs() - current) < HDL_WRITE_TIMEOUT)
	{
        int bytes = write(mUartFd, real_data + has_write, real_data_lenth - has_write);
        if (bytes > 0)
		{
            has_write += bytes;
        }
    }

	if (has_write != real_data_lenth)
	{
		return false;
	}

    return true;

}


bool HDL_COM_SetFlowCtrl(FlowControl fc)
{
    struct termios tty_config;

    int result = tcgetattr(mUartFd, &tty_config);
    if (result < 0)
	{
        LOG_E("setFlowControl tcgetattr error");
        return false;
    }

    if (fc == FC_SOFTWARE)
	{
        LOG_D("software control enable");
        tty_config.c_iflag |= (IXON | IXOFF);
        tty_config.c_cc[VSTART] = 0x11;
        tty_config.c_cc[VSTOP] = 0x13;
    }
	else if (fc == FC_HARDWARE)
	{
        // not support now.
    }
	else
	{
        tty_config.c_iflag &= ~(IXON | IXOFF);
        tty_config.c_cflag &= ~(CRTSCTS);
    }

    result = tcsetattr(mUartFd, TCSADRAIN, &tty_config);
    if (result < 0)
	{
        LOG_E("setFlowControl tcsetattr error");
        return false;
    }
    return true;
}

#endif
#endif
