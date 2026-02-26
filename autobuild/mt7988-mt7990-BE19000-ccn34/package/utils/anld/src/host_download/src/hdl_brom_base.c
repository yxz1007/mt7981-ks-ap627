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

#include "hdl_brom_base.h"

#include <inttypes.h>
#include <memory.h>
#include <stdio.h>

#include "config_parser.h"
#include "hdl_config.h"
#include "hdl_physical.h"
#include "time_utils.h"
#include "utils.h"
#include "location_power.h"

#ifdef HDL_ON_LINUX
#include <unistd.h>
#endif


#define HDL_WDT_REG 		(0xA2080000) //for 3335 & 3352
#define HDL_WDT_VAL 		(0x0010)     //for 3335 & 3352
#define HDL_WDT_REG2 		(0xA2080030) //only for 3352
#define HDL_WDT_VAL2 		(0x0040)     //only for 3352

#define BTROM_READ_TIMEOUT 	(1000)
#define BTROM_WRITE_TIMEOUT	(1000)

#define HDL_START_CMD1 		(0xA0)
#define HDL_START_CMD1_R 	(0x5F)
#define HDL_START_CMD2 		(0x0A)
#define HDL_START_CMD2_R 	(0xF5)
#define HDL_START_CMD3 		(0x50)
#define HDL_START_CMD3_R 	(0xAF)
#define HDL_START_CMD4 		(0x05)
#define HDL_START_CMD4_R 	(0xFA)

#define BROM_CMD_WRITE16 	(0xD2)
#define BROM_CMD_SET_BAUD 	(0xDC)
#define BROM_CMD_SEND_DA 	(0xD7)
#define BROM_CMD_JUMP_DA 	(0xD5)
#define BROM_CMD_READ_UID	(0xE9)

#define HDL_3352_DA_RUN_ADDR 	(0x04200000)
#define HDL_3335_DA_RUN_ADDR 	(0x04000000)
#define HDL_3335_DA_SPI_RUN_ADDR 	(0x04200000)

#define BAUDRATE_115200 	(115200)
#define BAUDRATE_921600 	(921600)
#define ONE_WORD_COUNT 		(0x01)
#define SIGNATURE_LENGTH 	(0x0)

#define BROM_ERROR 			(0x1000)
#define DA_PACKET_LEN 		(1024)
#define HANDSHAKE_TIMEOUT	(10000)

extern HDL_CONFIG g_hdl_config;

static bool HDL_COM_PutByte(uint8_t data, bool sw_flow_ctrl_open)
{
	return HDL_COM_PutByte_Buffer(&data, 1, sw_flow_ctrl_open);
}
static bool HDL_COM_PutData16(uint16_t data)
{
	//for bootrom use.
	if (g_hdl_config.dl_slave_phy == SLAVE_UART)
	{
		reverse((uint8_t*)(&data), 2);
	}	
	return HDL_COM_PutByte_Buffer(&data, 2, false);
}
static bool HDL_COM_PutData32(uint32_t data)
{
	//for bootrom use.
	if (g_hdl_config.dl_slave_phy == SLAVE_UART)
	{
		reverse((uint8_t*)(&data), 4);
	}	
	return HDL_COM_PutByte_Buffer(&data, 4, false);
}

bool HDL_COM_GetData16(uint16_t* data)
{
	uint16_t value = 0;
	pTime_t current = getSystemTickMs();
	uint32_t read_len = HDL_COM_GetByte_Buffer(&value, sizeof(value), false);
	while (read_len == 0)
	{
		portTaskDelayMs(20);
		read_len = HDL_COM_GetByte_Buffer(&value, sizeof(value), false);
		if (read_len > 0)
		{
			break;
		}
		if ((getSystemTickMs() - current) > BTROM_READ_TIMEOUT)
		{
			break;
		}
	}
	if (read_len == 0)
	{
		return false;
	}
	if (g_hdl_config.dl_slave_phy == SLAVE_UART)
	{
		reverse((uint8_t*)(&value), sizeof(value));
	}	
	*data = value;
	return true;
}
bool HDL_COM_GetData32(uint32_t* data)
{
	uint32_t value = 0;
	uint32_t read_len = HDL_COM_GetByte_Buffer(&value, sizeof(value), false);
	if (read_len == 0)
	{
		portTaskDelayMs(20);
		read_len = HDL_COM_GetByte_Buffer(&value, sizeof(value), false);
		if (read_len == 0)
		{
			return false;
		}
	}
	if (g_hdl_config.dl_slave_phy == SLAVE_UART)
	{
		reverse((uint8_t*)(&value), sizeof(value));
	}	
	*data = value;
	return true;
}
bool echoU8(uint8_t sendChar, uint8_t receiveChar)
{
	LOG_I("echoU8 Send 0x%X", sendChar);

    if (!HDL_COM_PutByte(sendChar, false))
	{
        return false;
    }
	portTaskDelayMs(20);
	uint8_t value = 0;
    uint32_t read_len = HDL_COM_GetByte_Buffer(&value, sizeof(value), false);
    if(read_len == 0)
    {
        portTaskDelayMs(20);
		read_len = HDL_COM_GetByte_Buffer(&value, sizeof(value), false);
		if (read_len == 0)
		{
			return false;
		}
	}
	else
	{
		portTaskDelayMs(10);
	}
	LOG_I("echoU8 receiveChar 0x%X", receiveChar);
	return receiveChar == value;
}

bool echoU16(uint16_t sendChar, uint16_t receiveChar)
{
	LOG_I("echoU16 Send 0x%X", sendChar);

    if (!HDL_COM_PutData16(sendChar))
	{
        return false;
    }

	uint16_t value = 0;
	if (!HDL_COM_GetData16(&value))
	{
		return false;
	}

	return receiveChar == value;
}

bool echoU32(uint32_t sendChar, uint32_t receiveChar)
{
	LOG_I("echoU32 Send 0x%X", sendChar);

	if (!HDL_COM_PutData32(sendChar))
	{
		return false;
	}

	uint32_t value = 0;
	if (!HDL_COM_GetData32(&value))
	{
		return false;
	}

	if (receiveChar == value)
	{
		return true;
	}
	else
	{
		LOG_E("echoU32 want receiveChar 0x%X8,  receiveChar 0x%X8", receiveChar, value);
		return false;
	}
}

bool hdl_brom_start()
{
	pTime_t current = 0;
	uint8_t handshakePassCount = 0;

brom_start_retry:

    location_power_off_device();
    portTaskDelayMs(500);
    current = getSystemTickMs();
	handshakePassCount = 0;

    LOG_I("Please reboot chip again:");
    location_power_on_device();
    while (true)
	{
        if (echoU8(HDL_START_CMD1, HDL_START_CMD1_R))
		{
			handshakePassCount++;
            break;
        }
        if (getSystemTickMs() - current > HANDSHAKE_TIMEOUT)
		{
			LOG_E("Handshake timeout, please retry!");
            return false;
        }
    }

    if (echoU8(HDL_START_CMD2, HDL_START_CMD2_R))
	{
        handshakePassCount++;
    }
	else
	{
		portTaskDelayMs(2000);
        goto brom_start_retry;
	}

    if (echoU8(HDL_START_CMD3, HDL_START_CMD3_R))
	{
        handshakePassCount++;
    }
	else
	{
		portTaskDelayMs(2000);
        goto brom_start_retry;
	}

    if (echoU8(HDL_START_CMD4, HDL_START_CMD4_R))
	{
        handshakePassCount++;
    }
	else
	{
		portTaskDelayMs(2000);
        goto brom_start_retry;
	}
	portTaskDelayMs(20);
	return (handshakePassCount == 4);
}

bool hdl_brom_set_register(uint32_t addr, uint16_t val)
{
	LOG_I("hdl_brom_set_register addr(0x%X) val(0x%X)", addr, val);

	if (!echoU8(BROM_CMD_WRITE16, BROM_CMD_WRITE16))
	{
		LOG_E("hdl_brom_set_register echoU8(0x%X) error", BROM_CMD_WRITE16);
        return false;
    }
    if (!echoU32(addr, addr))
	{
		LOG_E("hdl_brom_set_register echoU32(0x%X) error", addr);
        return false;
    }
    if (!echoU32(ONE_WORD_COUNT, ONE_WORD_COUNT))
	{
		LOG_E("hdl_brom_set_register echoU32(0x%X) error", ONE_WORD_COUNT);
        return false;
    }

	uint16_t ret = 0;
    if (!HDL_COM_GetData16(&ret))
	{
		LOG_E("hdl_brom_set_register readU16 error");
        return false;
    }
	else
	{
		if (ret >= BROM_ERROR)
		{
			LOG_E("hdl_brom_set_register get status error %d", ret);
			return false;
		}
	}

    if (!echoU16(val, val))
	{
		LOG_E("hdl_brom_set_register echoU16(0x%X) error", val);
        return false;
    }

    if (!HDL_COM_GetData16(&ret))
	{
		LOG_E("hdl_brom_set_register readU16 error");
        return false;
    }
	else
	{
		if (ret >= BROM_ERROR)
		{
			LOG_E("hdl_brom_set_register get status error %d", ret);
			return false;
		}
	}

    return true;
}

bool hdl_brom_disable_wdt()
{
	if ((g_hdl_config.chip == CHIP_AG3352) || (g_hdl_config.chip == CHIP_AG3335))
	{
		if (!hdl_brom_set_register(HDL_WDT_REG, HDL_WDT_VAL))
		{
			return false;
		}
		if (g_hdl_config.chip == CHIP_AG3352)
		{
			if (!hdl_brom_set_register(HDL_WDT_REG2, HDL_WDT_VAL2))
			{
				return false;
			}
		}
	}
	return true;
}

bool hdl_brom_set_baudrate(uint32_t baudrate)
{
    if (!echoU8(BROM_CMD_SET_BAUD, BROM_CMD_SET_BAUD))
	{
		LOG_E("hdl_brom_set_baudrate echoU8(0x%X) error", BROM_CMD_SET_BAUD);
        return false;
    }

    if (!echoU32(baudrate, baudrate))
	{
		LOG_E("hdl_brom_set_baudrate echoU32(0x%x) error", baudrate);
        return false;
    }

	uint16_t ret = 0;
    if (!HDL_COM_GetData16(&ret))
	{
		LOG_E("hdl_brom_set_baudrate readU16 error %d", ret);
        return false;
    }
	else
	{
		if (ret >= BROM_ERROR)
		{
			LOG_E("hdl_brom_set_baudrate get status error %d", ret);
			return false;
		}
	}

	// set host baudrate
	if (!HDL_COM_SetBaudRate(baudrate))
	{
		return false;
	}
	portTaskDelayMs(100);

	return true;
}

uint16_t computeDAChecksum(uint8_t* buf, uint32_t buf_len)
{
	if (buf == NULL || buf_len == 0)
	{
		return 0;
	}

	uint16_t checksum = 0;
	uint32_t i = 0;
	for (i = 0; i < buf_len / 2; i++)
	{
		checksum ^= *(uint16_t*)(buf + i * 2);
	}
	if ((buf_len % 2) == 1) {
		checksum ^= buf[i * 2];
	}
	return checksum;
}

bool hdl_brom_send_da()
{
	bool ret_flag = false;

	char buffer[DA_PACKET_LEN] = {0};
    int total_sent_len = 0;
    int checksum = 0;
    int readStep = DA_PACKET_LEN;
	uint16_t btrom_crc = 0;
	size_t result;
	uint16_t ret = 0;

    const int da_len = getFileSize(g_hdl_config.da_path);
	if (da_len == 0)
	{
		LOG_E("DA len is zero!");
        return ret_flag;
	}

    FILE *fda = fopen(g_hdl_config.da_path, "rb");
    if (fda == NULL)
	{
        LOG_E("Open da file %s fail", g_hdl_config.da_path);
        return ret_flag;
    }

    if (!echoU8(BROM_CMD_SEND_DA, BROM_CMD_SEND_DA))
	{
        LOG_E("hdl_brom_send_da echoU8(0x%X) error", BROM_CMD_SEND_DA);
		goto OUT;
    }
	if (g_hdl_config.chip == CHIP_AG3352)
	{
		if (!echoU32(HDL_3352_DA_RUN_ADDR, HDL_3352_DA_RUN_ADDR))
		{
			LOG_E("hdl_brom_send_da echoU32(0x%X) error", HDL_3352_DA_RUN_ADDR);
			goto OUT;
		}
	}
	else if (g_hdl_config.chip == CHIP_AG3335 && g_hdl_config.dl_slave_phy == SLAVE_SPI)
	{		
		if (!echoU32(HDL_3335_DA_SPI_RUN_ADDR, HDL_3335_DA_SPI_RUN_ADDR))
		{
			LOG_E("hdl_brom_send_da echoU32(0x%X) error", HDL_3335_DA_SPI_RUN_ADDR);
			goto OUT;
		}
	}
	else
	{
		if (!echoU32(HDL_3335_DA_RUN_ADDR, HDL_3335_DA_RUN_ADDR))
		{
			LOG_E("hdl_brom_send_da echoU32(0x%X) error", HDL_3335_DA_RUN_ADDR);
			goto OUT;
		}
	}

	if (!echoU32(da_len, da_len))
	{
        LOG_E("hdl_brom_send_da echoU32(0x%X) error", da_len);
		goto OUT;
    }

    if (!echoU32(SIGNATURE_LENGTH, SIGNATURE_LENGTH))
	{
        LOG_E("hdl_brom_send_da echoU32(0x%X) error", SIGNATURE_LENGTH);
		goto OUT;
    }

    if (!HDL_COM_GetData16(&ret))
	{
		LOG_E("hdl_brom_send_da readU16 error");
		goto OUT;
    }
	else
	{
		if (ret >= BROM_ERROR)
		{
			LOG_E("hdl_brom_send_da get status error %d (0x%X)", ret, ret);
			goto OUT;
		}
	}


    while (total_sent_len < da_len)
	{
        result = fread(buffer, readStep, 1, fda);
		if (result == 0)
		{
			LOG_E("Reading error ( %d )", (int)result);
			//goto OUT;
		}
		HDL_COM_PutByte_Buffer((uint8_t *)buffer, readStep, false);

        checksum ^= computeDAChecksum((uint8_t *)buffer, readStep);
        total_sent_len += readStep;

        LOG_I("SEND %d bytes OK,total %d", readStep, total_sent_len);

        if (da_len - total_sent_len < readStep) {
            readStep = da_len - total_sent_len;
        }

        portTaskDelayMs(10);
    }

    if (!HDL_COM_GetData16(&btrom_crc))
	{
		LOG_E("hdl_brom_send_da read btrom_crc error");
		goto OUT;
    }
	else
	{
		if (checksum != btrom_crc)
		{
			LOG_E("btrom checksum fail, 0x%x/0x%x", checksum, btrom_crc);
			goto OUT;
		}
		else
		{
			LOG_I("btrom response checksum 0x%x", btrom_crc);
		}
	}

	ret = 0;
    if (!HDL_COM_GetData16(&ret))
	{
		LOG_E("hdl_brom_send_da readU16 error");
		goto OUT;
    }
	else
	{
		if (ret >= BROM_ERROR)
		{
			LOG_E("hdl_brom_send_da get status error %d", ret);
			goto OUT;
		}
	}

    LOG_I("send DA successful");
    ret = true;

OUT:
	if (fclose(fda) == EOF)
	{
		return ret;
	}
	return ret;
}

bool hdl_brom_jump_da()
{
    if (!echoU8(BROM_CMD_JUMP_DA, BROM_CMD_JUMP_DA))
	{
        LOG_E("hdl_brom_jump_da echoU8(0x%X) error", BROM_CMD_JUMP_DA);
        return false;
	}

	if (g_hdl_config.chip == CHIP_AG3352)
	{
		if (!echoU32(HDL_3352_DA_RUN_ADDR, HDL_3352_DA_RUN_ADDR))
		{
			LOG_E("hdl_brom_jump_da echoU32(0x%X) error", HDL_3352_DA_RUN_ADDR);
			return false;
		}
	}
	else if(g_hdl_config.chip == CHIP_AG3335 && g_hdl_config.dl_slave_phy == SLAVE_SPI)
	{
		if (!echoU32(HDL_3335_DA_SPI_RUN_ADDR, HDL_3335_DA_SPI_RUN_ADDR))
		{
			LOG_E("hdl_brom_jump_da echoU32(0x%X) error", HDL_3335_DA_SPI_RUN_ADDR);
			return false;
		}
	}
	else
	{
		if (!echoU32(HDL_3335_DA_RUN_ADDR, HDL_3335_DA_RUN_ADDR))
		{
			LOG_E("hdl_brom_jump_da echoU32(0x%X) error", HDL_3335_DA_RUN_ADDR);
			return false;
		}
	}

	uint16_t ret = 0;
    if (!HDL_COM_GetData16(&ret))
	{
		LOG_E("hdl_brom_jump_da readU16 error");
        return false;
    }
	else
	{
		if (ret >= BROM_ERROR)
		{
			LOG_E("hdl_brom_jump_da get status error %d", ret);
			return false;
		}
	}
if(g_hdl_config.dl_slave_phy == SLAVE_UART)
{
	if (g_hdl_config.dl_da_support_921600 == true)
	{
		// set host baudrate
		if (!HDL_COM_SetBaudRate(BAUDRATE_115200))
		{
			return false;
		}
		portTaskDelayMs(100);
	}
}
	LOG_I("Jump to DA OK");

    return true;
}

bool BRom_1Bytes_req_19Bytes_rsp(uint8_t value, uint8_t* retdata)
{
	//portTaskDelayMs(1);

	if (!HDL_COM_PutByte(value, false))
	{
		return false;
	}
	portTaskDelayMs(100);
	pTime_t current = 0;


	current = getSystemTickMs();

	uint8_t bufbrom[64];
	uint32_t total = 0;

	uint8_t* pdata = (uint8_t*)retdata;	
	
	while (true)
	{
		if (getSystemTickMs() - current >= 500)
		{
			LOG_E("BRom_1Bytes_req_19Bytes_rsp timeout, please retry!");
			return false;
		}

		uint32_t numRead = HDL_COM_GetByte_Buffer(&bufbrom[total], 19, false);

		if (numRead == 0)
		{
			continue;
		}

		total += numRead;

		if (total > 1)
		{
			uint8_t res_data = bufbrom[0];
			if (res_data != value)
			{
				LOG_E("BRom_1Bytes_req_19Bytes_rsp, result = fail, input_value = 0x%X, output_value = 0x%X, error_message = input output mismatch", value, res_data);
				return false;
			}
		}

		if (total >= 19)
		{
			int ret = (pdata[17] << 8) + pdata[18];
			if (ret <= BROM_ERROR)
			{
				LOG_E("BRom_1Bytes_req_19Bytes_rsp, result = fail, error_code = 0x%X, error_message = response error code", ret);
			}

			memcpy(pdata, &bufbrom[1], 16);

			return true;
		}

	}

	return false;

}

bool hdl_brom_read_uid(uint8_t* retdata)
{
	return BRom_1Bytes_req_19Bytes_rsp(BROM_CMD_READ_UID, retdata);
}
