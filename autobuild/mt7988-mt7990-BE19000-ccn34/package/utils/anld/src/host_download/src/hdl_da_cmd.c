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

#include "hdl_da_cmd.h"

#include <inttypes.h>
#include <memory.h>
#include <stdlib.h>

#include "config_parser.h"
#include "crc32.h"
#include "hdl_physical.h"
#include "pool.h"
#include "time_utils.h"
#include "utils.h"

#define SLIDING_WINDOW_COUNT		(1) //1 for SPI & I2C, 3 for UART

#define Combo_16Bit(byte1, byte2) ((byte2<<8) + byte1)
#define Combo_32Bit(byte1, byte2, byte3, byte4) ((byte4<<24) + (byte3<<16) + (byte2<<8) + byte1)

#define DA_READ_TIMEOUT 	(1000)
#define DA_FORMAT_TIMEOUT 	(45000)

#define LEN_4K (0x1000) 
#define LEN_64K (0x10000)
#define DA_S_DONE (0x00)
#define RACE_DA_GET_FLASH_ADDRESS (0x210E)
#define RACE_DA_GET_FLASH_SIZE (0x210F)
#define RACE_DA_GET_FLASH_ID (0x2110)
#define RACE_DA_WRITE_BYTES (0x2100)
#define RACE_DA_READ_BYTES (0x2101)
#define RACE_DA_ERASE_BYTES (0x2104)
#define RACE_DA_READ_OTP (0x210D)
#define RACE_DA_BAUDRATE (0x2115)
#define RACE_DA_FLOW_CTRL (0x2116)
#define RACE_DA_DATA_RANGE_CRC (0x211A)
#define RACE_DA_RESET (0x2106)
#define RACE_DA_FINISH (0x2106)
#define RACE_DA_LOG_RESPONSE (0x0F10)

#define RACE_DFU_START (0x211B)
#define RACE_DFU_RESET (0x211C)

#ifdef FLOW_CTRL_OPEN
	bool flow_ctrl_open = true;
#else
	bool flow_ctrl_open = false;
#endif

typedef struct
{
	uint32_t addr;
	uint32_t size;
	bool send_done;
	bool recv_done;
} format_block_info_t;

typedef struct
{
	uint32_t addr;
	bool send_done;
	bool recv_done;
} download_block_info_t;

#pragma pack(1)

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t enable_;
}RACE_FLOW_CTRL_SEND;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t status_;
}RACE_FLOW_CTRL_RES;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint32_t rate_;
}RACE_BD_SEND;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t status_;
}RACE_BD_RES;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
}RACE_ADDR_SEND;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t status_;
	uint32_t addr_;
}RACE_ADDR_RES;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
}RACE_SIZE_SEND;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t status_;
	uint32_t size_;
}RACE_SIZE_RES;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
}RACE_ID_SEND;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t status_;
	uint8_t flash_id_[3];
}RACE_ID_RES;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint32_t addr_;
	uint32_t size_;
	uint32_t crc_;
}RACE_FM_SEND;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t status_;
	uint32_t addr_;
}RACE_FM_RES;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint32_t addr_;
	uint16_t size_;
	uint8_t buf_[DA_SEND_PACKET_LEN];
	uint32_t crc_;
}RACE_DL_SEND;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint32_t addr_;
	uint16_t size_;
	uint32_t crc_;
}RACE_RB_SEND;

typedef struct {
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint32_t addr_;
	uint32_t size_;
	uint32_t crc_;
} RACE_OTP_SEND;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t status_;
	uint32_t addr_;
	uint32_t crc_;
	uint8_t buf_[DA_RECV_PACKET_LEN];
}RACE_RB_RES;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t status_;
	uint32_t addr_;
}RACE_DL_RES;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t flag_;
}RACE_RST_SEND;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t status_;
}RACE_RST_RES;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t status_;
	uint32_t crc_;
}RACE_CHECK_CRC_RES;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint32_t data_addr_;
	uint32_t data_len_;
	uint32_t data_crc_;
	uint32_t crc_;
}RACE_CHECK_CRC_SEND;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t flag_;
	uint32_t crc_;
}RACE_DFU_START_SEND;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t status_;
	uint8_t current;
}RACE_DFU_START_RES;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t flag_;
	uint32_t crc_;
}RACE_DFU_RESET_SEND;

typedef struct
{
	uint8_t head_;
	uint8_t type_;
	uint16_t len_;
	uint16_t id_;
	uint8_t status_;
}RACE_DFU_RESET_RES;

#pragma pack()

extern HDL_CONFIG g_hdl_config;

int parsing_race()
{
    S_POOL* pPool = get_rx_pool();

	const uint8_t race_min_len = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t); // head(2B)+len(2B)+id(2B)

//todo: try to print log.
    // find head
    bool bFindHead = false;
	while (pPool->size_ >= race_min_len)
    {        
        if (pPool->pool[0] != 0x05) // not find 0x05
        {
            pop_pool(pPool, 1);
        }
        else    //find 0x05
        {
            //if (pPool->pool[1] != 0x5B) // find 0x05, but next byte is not 0x5B
			if (!((pPool->pool[1] == 0x5B) || (pPool->pool[1] == 0x5D))) // find 0x05, but next byte is not 0x5B or 0x5D
            {
                pop_pool(pPool, 1);
            }
            else    // find 0x055B or 0x055D
            {
                bFindHead = true;
                break;
            }
        }
    }
    
    // check len
    if (bFindHead)
    {
        const uint16_t payload_size = Combo_16Bit(pPool->pool[2], pPool->pool[3]);
     
        if (payload_size > MAX_PAYLOAD_SIZE)
        {
            pop_pool(pPool, sizeof(uint8_t)+sizeof(uint8_t));   // remove head(1B) and type(1B)
            return 0;
        }

		// head(1B)+type(1B)+len(2B)+payload_size
        const int race_length = sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint16_t)+payload_size;

        // data not enough
        if (pPool->size_ < race_length)  
        {
            return 0;
        }

		if(pPool->pool[1] == 0x5D)
		{
			/*
			uint8_t head_;
			uint8_t type_;
			uint16_t length_;
			uint16_t id_;
			uint8_t cpu_id_;
			uint8_t reserve_;
			uint32_t timestamp_;
			*/
			const uint16_t log_id = Combo_16Bit(pPool->pool[4], pPool->pool[5]);
			if (log_id == RACE_DA_LOG_RESPONSE)
			{
				int len = race_length - 11;
				char *con = malloc(len + 1);
				if (con == NULL)
				{
					LOG_E("parsing_race malloc con fail");
					return 0;
				}
				memset(con, 0, len + 1);
				memcpy(con, &pPool->pool[12], race_length - 12);

				//char log_str[race_length - 11];//error C2057
				//memset(&log_str, 0, race_length - 11);
				//memcpy(log_str, &pPool->pool[12], race_length - 12);
				LOG_I("[DA Log] %s", con);
				pop_pool(pPool, race_length);
				free(con);
				con = NULL;
				return 0;
			}
		}
        
        return race_length;
    }
    
    return 0;
}

static uint8_t g_rx_buf[RACE_MAX_SIZE+1] = {0};
bool get_race(int *race_len, uint16_t length)
{
	if(length > RACE_MAX_SIZE) return false;

	pTime_t current = getSystemTickMs();
	while ((getSystemTickMs() - current) < DA_READ_TIMEOUT)
	{
		S_POOL *pPool = get_rx_pool();
		int read_len = 0;
		if (g_hdl_config.dl_slave_phy == SLAVE_SPI)
		{
			read_len = HDL_COM_GetByte_Buffer(g_rx_buf, length, flow_ctrl_open);
		}
		else
		{
			read_len = HDL_COM_GetByte_Buffer(g_rx_buf, RACE_MAX_SIZE, flow_ctrl_open);
		}
		//int read_len = HDL_COM_GetByte_Buffer(g_rx_buf,RACE_MAX_SIZE,flow_ctrl_open);
		//int read_len = HDL_COM_GetByte_Buffer(g_rx_buf,length,flow_ctrl_open);
		//int read_len = da_uart_read(g_rx_buf, RACE_MAX_SIZE);
		//int read_len = HDL_COM_GetByte_Buffer(g_rx_buf, 11, flow_ctrl_open);
		if (read_len <= 0 && pPool->size_ == 0)
		{
			continue;
		}
				
		if (pPool->size_ + read_len >= POOL_BUF_SIZE)
		{
			LOG_E("read_dma_to_pool overflow, need to clear rx pool!");
			init_rx_pool();
			continue;			
		}

		if (read_len >0)
		{
			push_pool(pPool, g_rx_buf, read_len);
			memset(g_rx_buf, 0, read_len);
		}		

		int len = parsing_race();		
		if (len > 0)
		{
			*race_len = len;
			return true;
		}
	}

	return false;
}

int get_response(uint16_t id, uint8_t *buf, uint16_t length)
{
	int ret = E_DONE;
	int race_len = 0; 
	S_POOL* pPool = get_rx_pool();
	uint16_t res_id = 0;
	uint8_t result = 0;

	if (!get_race(&race_len, length))
	{
		return E_GET_RACE_TIMEOUT;
	}
	res_id = Combo_16Bit(pPool->pool[4], pPool->pool[5]);		
	if (res_id != id)
	{
		ret = E_RACE_ID_ERROR;
		goto OUT;
	}

	result = pPool->pool[6];
	if (DA_S_DONE != result)
	{
		ret = E_DA_RES_ERROR;
		goto OUT;
	}

	if (res_id == RACE_DA_GET_FLASH_ADDRESS ||
		res_id == RACE_DA_GET_FLASH_SIZE ||
		res_id == RACE_DA_WRITE_BYTES ||
		res_id == RACE_DA_ERASE_BYTES ||
		res_id == RACE_DA_DATA_RANGE_CRC)
	{
		memcpy(buf, &pPool->pool[7], 4);
	}
	else if (res_id == RACE_DA_GET_FLASH_ID)
	{
		memcpy(buf, &pPool->pool[7], 3);
	}
	else if (res_id == RACE_DFU_START)
	{
		memcpy(buf, &pPool->pool[6], 2);
	}
	else if (res_id == RACE_DFU_RESET)
	{
		memcpy(buf, &pPool->pool[6], 1);
	}

OUT:
	pop_pool(pPool, race_len);
	return ret;
}


int get_response_for_flash_id(uint16_t id, uint8_t *buf)
{
	int ret = E_DONE;
	int race_len = 0; 
	S_POOL* pPool = get_rx_pool();
	uint16_t res_id = 0;
	uint8_t result = 0;
	
	RACE_ID_RES res = {0};

	if (!get_race(&race_len, sizeof(res)))
	{
		return E_GET_RACE_TIMEOUT;
	}
	res_id = Combo_16Bit(pPool->pool[4], pPool->pool[5]);
	if (res_id != id)
	{
		ret = E_RACE_ID_ERROR;
		goto OUT;
	}

	result = pPool->pool[6];
	if (DA_S_DONE != result)
	{
		ret = E_DA_RES_ERROR;
		goto OUT;
	}

	if (res_id == RACE_DA_GET_FLASH_ID)
	{
		memcpy(buf, &pPool->pool[7], 3);
	}

OUT:
	pop_pool(pPool, race_len);
	return ret;
}


int get_readback_response(uint16_t id, uint32_t *addr, uint32_t *crc, uint8_t *buf, uint16_t length)
{
	int ret = E_DONE;
	int race_len = 0; 
	S_POOL* pPool = get_rx_pool();
	uint8_t result = 0;
	uint16_t res_id = 0;

	if (!get_race(&race_len,length))
	{
		return E_GET_RACE_TIMEOUT;
	}
	res_id = Combo_16Bit(pPool->pool[4], pPool->pool[5]);
	if (res_id != id)
	{
		ret = E_RACE_ID_ERROR;
		goto OUT;
	}

	result = pPool->pool[6];
	if (DA_S_DONE != result)
	{
		ret = E_DA_RES_ERROR;
		goto OUT;
	}

	memcpy(addr, &pPool->pool[7], 4);
	memcpy(crc, &pPool->pool[11], 4);
	memcpy(buf, &pPool->pool[15], race_len - 15);

OUT:
	pop_pool(pPool, race_len);
	return ret;
}

int hdl_dfu_start_race(uint8_t flag, uint8_t* response)
{
	LOG_I("hdl_dfu_start_race start");

	// send
	RACE_DFU_START_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_) + sizeof(send.flag_) + sizeof(send.crc_);
	send.id_ = RACE_DFU_START;
	send.flag_ = flag;
	send.crc_ = 0;

	send.crc_ = CRC32((const uint8_t*)&send, sizeof(send) - sizeof(send.crc_));
	HDL_COM_PutByte_Buffer((uint8_t*)&send, sizeof(send), flow_ctrl_open);

	// return get_response(send.id_, response);
	int ret = get_response(send.id_, response, 0); //0, only for dfu
	return ret;
}

int hdl_dfu_reset_race(uint8_t flag, uint8_t* response)
{
	LOG_I("hdl_dfu_reset_race start");

	// send
	RACE_DFU_RESET_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_) + sizeof(send.flag_) + sizeof(send.crc_);
	send.id_ = RACE_DFU_RESET;
	send.flag_ = flag;
	send.crc_ = 0;

	send.crc_ = CRC32((const uint8_t*)&send, sizeof(send) - sizeof(send.crc_));
	HDL_COM_PutByte_Buffer((uint8_t*)&send, sizeof(send), flow_ctrl_open);

	// return get_response(send.id_, response);
	int ret = get_response(send.id_, response, 0); // 0, only for DFU
	return ret;
}

int hdl_flash_address_race(uint32_t *base_addr)
{
   	LOG_I("hdl_da_get_flash_address start");

	if (base_addr == NULL)
	{
		return false;
	}

	// send
	RACE_ADDR_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_);
	send.id_ = RACE_DA_GET_FLASH_ADDRESS;
	HDL_COM_PutByte_Buffer((uint8_t *)&send, sizeof(send), flow_ctrl_open);

	RACE_ADDR_RES res = {0};
	return get_response(send.id_, (uint8_t *)base_addr, sizeof(res));
}

int hdl_flash_size_race(uint32_t *size)
{
	LOG_I("hdl_da_get_flash_size start");

	if (size == NULL)
	{
		return false;
	}

	// send
	RACE_SIZE_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_);
	send.id_ = RACE_DA_GET_FLASH_SIZE;
	HDL_COM_PutByte_Buffer((uint8_t *)&send, sizeof(send), flow_ctrl_open);

	RACE_SIZE_RES res = {0};
	return get_response(send.id_, (uint8_t *)size, sizeof(res));
}

int hdl_flash_id_race(uint8_t *flash_id)
{
	LOG_I("hdl_da_get_flash_id start");

	if (flash_id == NULL)
	{
		return false;
	}
	// send
	RACE_ID_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_);
	send.id_ = RACE_DA_GET_FLASH_ID;
	HDL_COM_PutByte_Buffer((uint8_t *)&send, sizeof(send), flow_ctrl_open);

	return get_response_for_flash_id(send.id_, (uint8_t *)flash_id);
}

bool hdl_speedup_baudrate_race(const uint32_t baudrate)
{
	LOG_I("hdl_speedup_slave_baudrate start");

	// send
	RACE_BD_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_)+sizeof(send.rate_);
	send.id_ = RACE_DA_BAUDRATE;
	send.rate_ = baudrate;
	HDL_COM_PutByte_Buffer((uint8_t *)&send, sizeof(send), flow_ctrl_open);

	RACE_BD_RES res = {0};
	int rtn = get_response(send.id_, NULL,sizeof(res));
	if (rtn != E_DONE)
	{
		LOG_E("hdl_speedup_slave_baudrate error, ErrorCode:%d", rtn);		
		return false;
	}

	LOG_I("hdl_speedup_slave_baudrate res done");
	return true;

}

bool hdl_finish_race(bool enable)
{
	// send
	RACE_RST_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_)+sizeof(send.flag_);
	send.id_ = RACE_DA_FINISH;
	send.flag_ = enable;
	HDL_COM_PutByte_Buffer((uint8_t *)&send, sizeof(send), flow_ctrl_open);

	RACE_RST_RES res = {0};
	int rtn = get_response(send.id_, NULL,sizeof(res));
	if (rtn != E_DONE)
	{
        LOG_E("hdl_finish_race error, ErrorCode:%d", rtn);
		return false;
	}

	return true;
 
}

bool hdl_open_slave_flow_ctrl()
{
    LOG_I("hdl_open_slave_flow_ctrl start");

	// send
	RACE_FLOW_CTRL_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_)+sizeof(send.enable_);
	send.id_ = RACE_DA_FLOW_CTRL;
	send.enable_ = 0x01;
	HDL_COM_PutByte_Buffer((uint8_t *)&send, sizeof(send), false);

	RACE_FLOW_CTRL_RES res = {0};
	int rtn = get_response(send.id_, NULL,sizeof(res));
	if (rtn != E_DONE)
	{
        LOG_E("hdl_open_slave_flow_ctrl error, ErrorCode:%d", rtn);
		return false;
	}

	LOG_I("hdl_open_slave_flow_ctrl res done");
	return true;
}

bool hdl_sync_with_da(hdl_da_report_t *da_report)
{
	if (da_report == NULL)
	{
		return false;
	}

	// init RX buffer
	init_rx_pool();

	// need DA init done
	portTaskDelayMs(100);


#ifdef FLOW_CTRL_OPEN
	if (g_hdl_config.dl_slave_phy == SLAVE_UART)
	{
		// set slave flow ctrl
		if (!hdl_open_slave_flow_ctrl())
		{
			return false;
		}

		// set host flow ctrl
		if (!HDL_COM_SetFlowCtrl(FC_SOFTWARE))
		{
			return false;
		}
		portTaskDelayMs(100);
	}
#endif

	if (g_hdl_config.dl_slave_phy == SLAVE_UART)
	{
		// set slave baudrate
		if (!hdl_speedup_baudrate_race(g_hdl_config.dl_flash_baudrate))
		{
			return false;
		}

		// set host baudrate
		if (!HDL_COM_SetBaudRate(g_hdl_config.dl_flash_baudrate))
		{
			return false;
		}
		portTaskDelayMs(100);
	}
	
	int rtn = hdl_flash_address_race(&da_report->flash_base_addr);
	if (rtn != E_DONE)
	{
        LOG_E("hdl_flash_address_race error, ErrorCode:%d", rtn);
		return false;
	}

	rtn = hdl_flash_size_race(&da_report->flash_size);
	if (rtn != E_DONE)
	{
        LOG_E("hdl_flash_size_race error, ErrorCode:%d", rtn);
		return false;
	}

	rtn = hdl_flash_id_race(da_report->flash_id);
	if (rtn != E_DONE)
	{
		LOG_E("hdl_flash_id_race error, ErrorCode:%d", rtn);
		return false;
	}

	LOG_I("########## Flash address: 0x%X", da_report->flash_base_addr);
	LOG_I("########## Flash size: 0x%X", da_report->flash_size);
	LOG_I("########## Flash id: 0x%X%X%X", da_report->flash_id[0], da_report->flash_id[1], da_report->flash_id[2]);
 
    return true;
}


uint32_t format_get_block_count(uint32_t begin_address, uint32_t end_address)
{
	uint32_t packet_num = 0;

	const uint32_t length = end_address-begin_address;

	uint32_t address = begin_address;
	while (address < end_address)
	{
		uint32_t block_size = 0;
		if (length >= LEN_64K && (address%LEN_64K) == 0) 
		{
			block_size = LEN_64K;
		} 
		else 
		{
			block_size = LEN_4K;
		}
		address += block_size;

		packet_num++;
	}

	return packet_num;
}

void format_fill_block_info(format_block_info_t *buf, uint32_t begin_address, uint32_t end_address)
{
	uint32_t packet_num = 0;

	const uint32_t length = end_address-begin_address;

	uint32_t address = begin_address;
	while (address < end_address)
	{
		uint32_t block_size = 0;
		if (length >= LEN_64K && (address%LEN_64K) == 0) 
		{
			block_size = LEN_64K;
		} 
		else 
		{
			block_size = LEN_4K;
		}

		buf[packet_num].addr = address;
		buf[packet_num].size= block_size;

		address += block_size;

		packet_num++;
	}
}

bool format_send_single_block(uint32_t addr, uint32_t len)
{
	// send
	RACE_FM_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_) + sizeof(send.addr_) + sizeof(send.size_) + sizeof(send.crc_);
	send.id_ = RACE_DA_ERASE_BYTES;
	send.addr_ = addr;
	send.size_ = len;
	send.crc_ = 0;

	send.crc_ = CRC32((const uint8_t *)&send, sizeof(send) - sizeof(send.crc_));

	uint32_t j = 0;
	uint32_t resend_max_count = 3;

	for (j = 0; j < resend_max_count; j++)
	{
		if (!HDL_COM_PutByte_Buffer((uint8_t *)&send, sizeof(send), flow_ctrl_open))
		{
			if (j == resend_max_count - 1)
			{
				return false;
			}
		}
		else
		{
			return true;
		}
	}
	return false;
}

bool format_check_send_done(format_block_info_t *buf, uint32_t packet_num)
{
	uint32_t i = 0;
	for (i = 0; i < packet_num; i++)
	{
		if (buf[i].send_done == false)
		{
			LOG_I("########## Format send address(0x%X)", buf[i].addr);
		
			if (!format_send_single_block(buf[i].addr, buf[i].size))
			{
				return false;
			}

			buf[i].send_done = true;
			return false;
		}
	}
	return true;
}

bool format_check_recv_done(format_block_info_t *buf, uint32_t packet_num)
{
	uint32_t i = 0;
	for (i = 0; i < packet_num; i++)
	{
		if (buf[i].recv_done == false)
		{	
			return false;
		}
	}
	
	return true;
}

bool format_update_recv_status(format_block_info_t* buf, uint32_t packet_num, uint32_t res_addr)
{
	uint32_t i = 0;
	for (i = 0; i < packet_num; i++)
	{
		if (buf[i].send_done == true && buf[i].recv_done == false && res_addr == buf[i].addr)
		{
			buf[i].recv_done = true;
			return true;
		}
	}

	return false;
}

uint32_t format_reset_status(format_block_info_t *buf, uint32_t packet_num)
{
	uint32_t i = 0;
	uint32_t cnt = 0;
	for (i = 0; i < packet_num; i++)
	{
		if (!(buf[i].send_done == true && buf[i].recv_done == true))
		{	
			buf[i].send_done = false;	
			cnt++;		
		}
	}
	return cnt;
}

bool format_sliding_window_flow(format_block_info_t *buf, uint32_t packet_num)
{
	const uint32_t sliding_window_cout = (packet_num < SLIDING_WINDOW_COUNT) ? packet_num : SLIDING_WINDOW_COUNT;

	uint32_t i = 0;
	for (i = 0; i < sliding_window_cout; i++)
	{
		if (buf[i].send_done == false)
		{
			LOG_I("########## Format send address(0x%X)", buf[i].addr);
		
			if (!format_send_single_block(buf[i].addr, buf[i].size))
			{
				return false;
			}
			buf[i].send_done = true;
		}
	}

	pTime_t current = getSystemTickMs();
	RACE_FM_RES res={0};

	for (i = 0; i < packet_num; ++i)
	{
		if (((getSystemTickMs() - current) > DA_FORMAT_TIMEOUT)) //DA_READ_TIMEOUT*packet_num
		{
			LOG_E("format_sliding_window_flow TIMEOUT");
			return false;
		}

		uint32_t address = 0;
		int ret = get_response(RACE_DA_ERASE_BYTES, (uint8_t *)&address, sizeof(res));
		if (ret == E_DONE)
		{
			LOG_I("Format resp address(0x%X)", address);

			if (format_update_recv_status(buf, packet_num, address))
			{
				if (format_check_send_done(buf, packet_num) && format_check_recv_done(buf, packet_num))
				{
					return true;
				}
			}
		}
		else
		{
			if (format_check_send_done(buf, packet_num) && format_check_recv_done(buf, packet_num))
			{
				return true;
			}
		}
	}

	uint32_t resend_cnt = format_reset_status(buf, packet_num);
	if (resend_cnt > 0)
	{
		LOG_I("FORMAT Re-Send: count = %d", resend_cnt);
		format_check_send_done(buf, packet_num);

		for (i = 0; i < resend_cnt; ++i)
		{
			if (((getSystemTickMs() - current) > DA_FORMAT_TIMEOUT)) //DA_READ_TIMEOUT*packet_num
			{
				LOG_E("format_sliding_window_flow TIMEOUT");
				return false;
			}

			uint32_t address = 0;
			int ret = get_response(RACE_DA_ERASE_BYTES, (uint8_t *)&address, sizeof(res));
			if (ret == E_DONE)
			{
				LOG_I("Format resp address(0x%X)", address);

				if (format_update_recv_status(buf, packet_num, address))
				{
					if (format_check_send_done(buf, packet_num) && format_check_recv_done(buf, packet_num))
					{
						return true;
					}
				}
			}
		}
	}

	// while ((getSystemTickMs() - current) < DA_FORMAT_TIMEOUT)  //DA_READ_TIMEOUT*packet_num
    // {
	// 	uint32_t address = 0;
	// 	int ret = get_response(RACE_DA_ERASE_BYTES, (uint8_t *)&address,sizeof(res));
	// 	if (ret == E_DONE)
	// 	{
	// 		LOG_I("Format resp address(0x%X)", address);
		
	// 		if (format_update_recv_status(buf, packet_num, address))
	// 		{
	// 			if (format_check_send_done(buf, packet_num) && format_check_recv_done(buf, packet_num))
	// 			{
	// 				return true;
	// 			}
	// 		}
	// 	}
	// 	else if (ret == E_RACE_ID_ERROR || ret == E_DA_RES_ERROR)
	// 	{
	// 		LOG_E("Format Error: address(0x%X) ret(%d)", address, ret );
	// 		return false;
	// 	}
	// }

	LOG_E("FORMAT_TIMEOUT");
	return false;
}

bool hdl_da_format(const uint32_t format_flash_addr, const uint32_t format_len)
{
	const uint32_t format_begin_address = format_flash_addr;
	const uint32_t format_size = format_len;
	const uint32_t format_end_address = format_begin_address+format_size;

	uint32_t packet_num = format_get_block_count(format_begin_address, format_end_address);
	if (packet_num <= 0)
	{
		return false;
	}

	const uint32_t malloc_len = packet_num*sizeof(format_block_info_t);

	format_block_info_t *pData = (format_block_info_t *)malloc(malloc_len);
	if (pData == NULL)
	{
		LOG_E("da_format_flash malloc pData fail");
		return false;
	}
	memset(pData, 0, malloc_len);

	format_fill_block_info(pData, format_begin_address, format_end_address);

	if (!format_sliding_window_flow(pData, packet_num))
	{
		LOG_E("format_sliding_window_flow fail");
		if (pData != NULL)
		{
			free(pData);
			pData = NULL;
		}
		return false;
	}

	if (pData != NULL)
	{
		free(pData);
		pData = NULL;
	}

	return true;
}
 
bool hdl_readback_race(const char *host_file_path, uint32_t addr)
{
	uint8_t i = 0;
	// send
	RACE_RB_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_) + sizeof(send.addr_) + sizeof(send.size_) + sizeof(send.crc_);
	send.id_ = RACE_DA_READ_BYTES;
	send.addr_ = addr;
	send.size_ = DA_RECV_PACKET_LEN;
	send.crc_ = 0;

	send.crc_ = CRC32((const uint8_t *)&send, sizeof(send) - sizeof(send.crc_));

	for (i = 0; i < 3; i++)
	{

		HDL_COM_PutByte_Buffer((uint8_t *)&send, sizeof(send), flow_ctrl_open);

		pTime_t current = getSystemTickMs();
		while ((getSystemTickMs() - current) < DA_READ_TIMEOUT)
		{
			RACE_RB_RES res = {0};

			// uint32_t address = 0;
			// uint32_t crc = 0;
			int ret = get_readback_response(RACE_DA_READ_BYTES, &res.addr_, &res.crc_, res.buf_, sizeof(res));
			if (ret == E_DONE)
			{
				uint32_t crc = CRC32(res.buf_, DA_RECV_PACKET_LEN);
				if (res.crc_ != crc)
				{
					LOG_I("hdl_readback_race CRC fail: addr=0x%X, crc_cal=0x%X, crc_resp=0x%X", addr, crc, res.crc_);
					continue;
				}

				FILE *file = fopen(host_file_path, "ab");
				if (file == NULL)
				{
					LOG_E("can not open %s", host_file_path);
					return false;
				}
				size_t bytes_write= fwrite(res.buf_, 1, sizeof(res.buf_), file);
				if (bytes_write != sizeof(res.buf_))
				{
					if (fclose(file) == EOF)
					{
						LOG_E("hdl_readback_race The file(%s) could not closed.", host_file_path);
					}
					return false;
				}			
				if (fclose(file) == EOF)
				{
					LOG_E("hdl_readback_race The file(%s) could not closed.", host_file_path);
					return false;
				}

				LOG_I("hdl_readback_race res done: addr=0x%X", addr);
				return true;				
			}
		}
		LOG_I("hdl_readback_race res retry: addr=0x%X", addr);
	}

	return false;
}

bool hdl_read_otp_race(const char* host_file_path, uint32_t addr, uint32_t len)
{
	uint8_t i = 0;
	// send
	RACE_OTP_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_) + sizeof(send.addr_) + sizeof(send.size_) + sizeof(send.crc_);
	send.id_ = RACE_DA_READ_OTP;
	send.addr_ = addr;
	send.size_ = len;
	send.crc_ = 0;

	send.crc_ = CRC32((const uint8_t*)&send, sizeof(send) - sizeof(send.crc_));

	for (i = 0; i < 3; i++)
	{

		HDL_COM_PutByte_Buffer((uint8_t*)&send, sizeof(send), flow_ctrl_open);

		pTime_t current = getSystemTickMs();
		while ((getSystemTickMs() - current) < DA_READ_TIMEOUT)
		{
			RACE_RB_RES res;

			// uint32_t address = 0;
			// uint32_t crc = 0;
			int ret = get_readback_response(RACE_DA_READ_OTP, &res.addr_, &res.crc_, res.buf_, 0); //not support SPI
			if (ret == E_DONE)
			{
				uint32_t crc = CRC32(res.buf_, len);
				if (res.crc_ != crc)
				{
					LOG_I("hdl_readback_race CRC fail: addr=0x%X, crc_cal=0x%X, crc_resp=0x%X", addr, crc, res.crc_);
					continue;
				}

				FILE* file = fopen(host_file_path, "ab");
				if (file == NULL)
				{
					LOG_E("can not open %s", host_file_path);
					return false;
				}				
				size_t bytes_write= fwrite(res.buf_, 1, len, file);
				if (bytes_write != sizeof(res.buf_))
				{
					if (fclose(file) == EOF)
					{
						LOG_E("hdl_read_otp_race The file(%s) could not closed.", host_file_path);
					}
					return false;
				}							
				if (fclose(file) == EOF)
				{
					LOG_E("The file(%s) could not closed.", host_file_path);
					return false;
				}

				LOG_I("hdl_readback_race res done: addr=0x%X", addr);
				return true;
			}
		}
		LOG_I("hdl_readback_race res retry: addr=0x%X", addr);
	}

	return false;
}
bool download_send_single_block(uint32_t addr, const image_info_t *image_info)
{
	if (image_info == NULL)
	{
		return false;
	}

	if (addr < image_info->begin_address)
	{
		return false;
	}
	const uint32_t offset = addr - image_info->begin_address;

	size_t result;
	const char *path = image_info->image_name;
	FILE *file = fopen(path, "rb");
	if (file == NULL)
	{
		LOG_E("can not open %s", path);
		return false;
	}

	// send
	RACE_DL_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_) + sizeof(send.addr_) + sizeof(send.size_) + DA_SEND_PACKET_LEN + sizeof(send.crc_);
	send.id_ = RACE_DA_WRITE_BYTES;
	send.addr_ = addr;
	send.size_ = DA_SEND_PACKET_LEN;
	send.crc_ = 0;

	memset(send.buf_, 0xFF, DA_SEND_PACKET_LEN);
	if(fseek(file, offset, SEEK_SET) != 0)
	{
		if (fclose(file) == EOF)
		{
			LOG_E("The file(%s) could not closed.", path);
			return false;
		}
		return false;
	}

	result = fread(send.buf_, DA_SEND_PACKET_LEN, 1, file);
	if (result == 0)
	{
		LOG_I("Not-4K-Alignment ( %d )", (int)result);
		// fclose(file);
		// return false;
	}	
	if (fclose(file) == EOF)
	{
		LOG_E("download_send_single_block The file(%s) could not closed.", path);
		return false;
	}
	send.crc_ = CRC32((const uint8_t *)&send, sizeof(send) - sizeof(send.crc_));
	
	uint32_t j = 0;
	uint32_t resend_max_count = 3;

	for (j = 0; j < resend_max_count; j++)
	{
		if (!HDL_COM_PutByte_Buffer((uint8_t *)&send, sizeof(send), flow_ctrl_open))
		{
			if (j == resend_max_count - 1)
			{
				return false;
			}
		}
		else
		{
			return true;
		}
	}
	return false;
	//return HDL_COM_PutByte_Buffer((uint8_t *)&send, sizeof(send), flow_ctrl_open);
}

bool download_check_send_done(download_block_info_t *buf, const image_info_t *image_info, uint32_t packet_num)
{
	uint32_t i = 0;
	for (i = 0; i < packet_num; i++)
	{
		if (buf[i].send_done == false)
		{
			LOG_I("########## Download send address(0x%X)", buf[i].addr);
		
			if (!download_send_single_block(buf[i].addr, image_info))
			{
				return false;
			}

			buf[i].send_done = true;
			return false;
		}
	}
	return true;
}

bool download_check_recv_done(download_block_info_t *buf, uint32_t packet_num)
{
	uint32_t i = 0;
	for (i = 0; i < packet_num; i++)
	{
		if (buf[i].recv_done == false)
		{	
			return false;
		}
	}
	
	return true;
}

bool download_update_recv_status(download_block_info_t *buf, uint32_t packet_num, uint32_t res_addr)
{
	uint32_t i = 0;
	for (i = 0; i < packet_num; i++)
	{
		if (buf[i].send_done == true && buf[i].recv_done == false && res_addr == buf[i].addr)
		{	
			buf[i].recv_done = true;
			return true;
		}
	}
	
	return false;
}
uint32_t download_reset_status(download_block_info_t *buf, uint32_t packet_num)
{
	uint32_t i = 0;
	uint32_t cnt = 0;
	for (i = 0; i < packet_num; i++)
	{
		if (!(buf[i].send_done == true && buf[i].recv_done == true))
		{	
			buf[i].send_done = false;	
			cnt++;		
		}
	}
	return cnt;
}

bool download_sliding_window_flow(download_block_info_t *buf, const image_info_t *image_info, uint32_t packet_num)
{
	const uint32_t sliding_window_cout = (packet_num < SLIDING_WINDOW_COUNT) ? packet_num : SLIDING_WINDOW_COUNT;

	uint32_t i = 0;
	for (i = 0; i < sliding_window_cout; i++)
	{
		if (buf[i].send_done == false)
		{
			LOG_I("########## Download send address(0x%X)", buf[i].addr);

			if (!download_send_single_block(buf[i].addr, image_info))
			{
				LOG_E("########## Download send address(0x%X) fail", buf[i].addr);
				return false;
			}

			buf[i].send_done = true;
		}
	}

	pTime_t current = getSystemTickMs();

	RACE_DL_RES res ={0};

	for(i =0;i < packet_num ; ++i)
	{
		if(((getSystemTickMs() - current) > (pTime_t)DA_READ_TIMEOUT * (pTime_t)packet_num))
		{
			LOG_E("download_sliding_window_flow TIMEOUT");
			return false;
		}

		uint32_t address = 0;		
		int ret = get_response(RACE_DA_WRITE_BYTES, (uint8_t *)&address,sizeof(res));

		if (ret == E_DONE)
		{
			LOG_I("Download resp address(0x%X)", address);

			if (download_update_recv_status(buf, packet_num, address))
			{
				if (download_check_send_done(buf, image_info, packet_num) && download_check_recv_done(buf, packet_num))
				{
					return true;
				}
			}
		}
		else
		{
			if (download_check_send_done(buf, image_info, packet_num) && download_check_recv_done(buf, packet_num))
			{
				return true;
			}
		}
	}

	uint32_t resend_cnt = download_reset_status(buf, packet_num);
	if (resend_cnt > 0)
	{
		LOG_I("DOWNLOAD Re-Send: count = %d", resend_cnt);
		download_check_send_done(buf, image_info, packet_num);

		for (i = 0; i < resend_cnt; ++i)
		{
			if (((getSystemTickMs() - current) > (pTime_t)DA_READ_TIMEOUT * (pTime_t)packet_num))
			{
				LOG_E("download_sliding_window_flow TIMEOUT");
				return false;
			}

			uint32_t address = 0;
			int ret = get_response(RACE_DA_WRITE_BYTES, (uint8_t *)&address, sizeof(res));
			if (ret == E_DONE)
			{
				LOG_I("Download resp address(0x%X)", address);

				if (download_update_recv_status(buf, packet_num, address))
				{
					if (download_check_send_done(buf, image_info, packet_num) && download_check_recv_done(buf, packet_num))
					{
						return true;
					}
				}
			}
		}
	}

	// while ((getSystemTickMs() - current) < (pTime_t)DA_READ_TIMEOUT * (pTime_t)packet_num)
	// {
	// 	uint32_t address = 0;		
	// 	int ret = get_response(RACE_DA_WRITE_BYTES, (uint8_t *)&address,sizeof(res));
	// 	if (ret == E_DONE)
	// 	{
	// 		LOG_I("Download resp address(0x%X)", address);

	// 		if (download_update_recv_status(buf, packet_num, address))
	// 		{
	// 			if (download_check_send_done(buf, image_info, packet_num) && download_check_recv_done(buf, packet_num))
	// 			{
	// 				return true;
	// 			}
	// 		}
	// 	}
	// 	else if (ret == E_GET_RACE_TIMEOUT)
	// 	{
	// 		timeout_count++;
	// 		if(timeout_count >= max_timeout_count)
	// 		{
	// 			LOG_E("Download resp address(0x%X) E_GET_RACE_TIMEOUT", address);
	// 			return false;
	// 		}
	// 	}
	// 	else if (ret == E_RACE_ID_ERROR || ret == E_DA_RES_ERROR)
	// 	{
	// 		LOG_E("Download resp address(0x%X) RES_ERROR_CODE: %d", address, ret);
	// 		return false;
	// 	}		
	// }

	return false;
}

// bool download_sliding_window_flow(download_block_info_t *buf, const image_info_t *image_info, uint32_t packet_num)
// {
// 	const uint32_t sliding_window_cout = (packet_num < SLIDING_WINDOW_COUNT) ? packet_num : SLIDING_WINDOW_COUNT;


//#define BUFF_SIZE  1024

bool check_image_crc(const image_info_t *image_info)
{

	const uint32_t image_len = (uint32_t)getFileSize(image_info->image_name);
	const uint32_t image_crc = CRC32_file(image_info->image_name);
 
	// send
	RACE_CHECK_CRC_SEND send;
	send.head_ = 0x05;
	send.type_ = 0x5A;
	send.len_ = sizeof(send.id_) + sizeof(send.data_addr_) + sizeof(send.data_len_) + sizeof(send.data_crc_) + sizeof(send.crc_);
	send.id_ = RACE_DA_DATA_RANGE_CRC;
	send.data_addr_ = image_info->begin_address;
	send.data_len_ = image_len;
	send.data_crc_ = image_crc;
	send.crc_ = 0;

	send.crc_ = CRC32((const uint8_t *)&send, sizeof(send) - sizeof(send.crc_));
 
	HDL_COM_PutByte_Buffer((uint8_t *)&send, sizeof(send), flow_ctrl_open);
	RACE_CHECK_CRC_RES res = {0};
	uint32_t res_crc = 0;
	int rtn = get_response(send.id_, (uint8_t *)&res_crc, sizeof(res));
	if (rtn != E_DONE)
	{
        LOG_E("check_image_crc error, ErrorCode:%d, res_crc=  0x%08X", rtn, res_crc);
		return false;
	}
	else
	{
		LOG_I("check_image_crc done, image name:%s, response_crc=  0x%08X",image_info->image_name, res_crc);
		return true;
	}

}

bool download_image(const image_info_t *image_info) 
{
	const uint32_t image_len = (uint32_t)getFileSize(image_info->image_name);
	if(image_len <= 1)
	{
		LOG_E("download_image getFileSize fail, image length smaller than 1 byte.");
		return false;
	}

	const uint32_t packet_num = ((image_len - 1) / FW_PACKET_LEN) + 1;
	if(packet_num == 0)
	{
		LOG_E("download_image get packet_num fail, packet_num = 0.");
		return false;
	}

	const uint32_t malloc_len = packet_num*sizeof(download_block_info_t);
	if(malloc_len == 0)
	{
		LOG_E("download_image get malloc_len fail, malloc_len = 0.");
		return false;
	}
	
	download_block_info_t *pData = (download_block_info_t *)malloc(malloc_len);
	if (pData == NULL)
	{
		LOG_E("download_image malloc pData fail");
		return false;
	}
	memset(pData, 0, malloc_len);

	uint32_t address = image_info->begin_address;
	uint32_t i = 0;
	for (i = 0; i < packet_num; i++)
	{
		pData[i].addr = address;			
		address += FW_PACKET_LEN;
	}

	if (!download_sliding_window_flow(pData, image_info, packet_num))
	{
		LOG_E("download_sliding_window_flow fail");
		if (pData != NULL)
		{
			free(pData);
			pData = NULL;
		}
		return false;
	}

	if (pData != NULL)
	{
		free(pData);
		pData = NULL;
	}

	return true;
}

bool da_download_flash(image_info_t *list, uint32_t count) 
{	
    uint32_t i;
    for (i = 0; i < count; i++) 
	{
		image_info_t image_info = list[i];
        if (!download_image(&image_info))
		{
            LOG_E("download %s FAILED", image_info.image_name);
            return false;
        }
    }
    return true;
}

bool hdl_da_readback(const char *host_file_path, const uint32_t flash_addr, const uint32_t len)
{
	FILE *file;
	if ((file = fopen(host_file_path, "r")) != NULL)
	{
		// file exists
		if (fclose(file) == EOF)
		{
			LOG_E("The file(%s) could not closed.", host_file_path);
			return false;
		}
		if (remove(host_file_path) == 0)
		{
			LOG_I("The file(%s) is deleted successfully.", host_file_path);
		}
		else
		{
			LOG_E("The file(%s) is not deleted.", host_file_path);
			return false;
		}
	}

	const uint32_t rb_begin_addr = flash_addr;
	const uint32_t rb_size = len;
	const uint32_t rb_end_addr = rb_begin_addr + rb_size;

	uint32_t rb_addr = rb_begin_addr;
	while (rb_addr < rb_end_addr)
	{
		if (!hdl_readback_race(host_file_path, rb_addr))
		{
			LOG_E("hdl_readback_race addr=0x%08X fail!", rb_addr);
			return false;
		}

		rb_addr += DA_RECV_PACKET_LEN;
	}

	/*
	   You can add the data comparison function.
	    1. Get original data from your host chip or file system
	   	2. Compare with slave chip readback data 
	*/

	return true;
}
bool hdl_da_read_otp(const char* host_file_path, const uint32_t otp_addr, const uint32_t len)
{
	FILE* file;
	if ((file = fopen(host_file_path, "r")) != NULL)
	{
		// file exists		
		if (fclose(file) == EOF)
		{
			LOG_E("The file(%s) could not closed.", host_file_path);
			return false;
		}
		if (remove(host_file_path) == 0)
		{
			LOG_I("The file(%s) is deleted successfully.", host_file_path);
		}
		else
		{
			LOG_E("The file(%s) is not deleted.", host_file_path);
			return false;
		}
	}

	const uint32_t rb_begin_addr = otp_addr;
	const uint32_t rb_size = len;
	const uint32_t rb_end_addr = rb_begin_addr + rb_size;

	uint32_t rb_addr = rb_begin_addr;
	while (rb_addr < rb_end_addr)
	{
		if (!hdl_read_otp_race(host_file_path, rb_addr, len))
		{
			LOG_E("hdl_read_otp_race addr=0x%08X fail!", rb_addr);
			return false;
		}

		rb_addr += DA_SEND_PACKET_LEN;
	}

	/*
	   You can add the data comparison function.
		1. Get original data from your host chip or file system
		2. Compare with slave chip readback data
	*/

	return true;
}
