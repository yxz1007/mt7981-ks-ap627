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

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hdl_config.h"
#include "time_utils.h"

#ifdef HDL_ON_LINUX
#include <unistd.h>
#else
#include <Windows.h>
#endif

void reverse(uint8_t *to_reverse, dSize_t length) 
{
    if (to_reverse == NULL || length == 0) 
	{
        return;
    }

	dSize_t i = 0;
	dSize_t j = length - 1;
	uint8_t tmp;
	while (i < j) 
	{
		tmp = to_reverse[i];
		to_reverse[i] = to_reverse[j];
		to_reverse[j] = tmp;
		i++;
		j--;
	}
}

	/**
	* If current running in software flow control
	* Replace the following chars:
	*         0x77, 0xEE -> 0x11
	*         0x77, 0xEC -> 0x13
	*         0x77, 0x88 -> 0x77
	* */
//uint8_t flowctrl_buf[FLOW_CTRL_BUF_SIZE] = {0};
uint32_t flowctrl_encode(const uint8_t *p_src, const uint32_t src_len, uint8_t *p_dst)
{
	if (p_src == NULL || src_len == 0 || p_dst == NULL)
	{
		return 0;
	}

	int i = 0;	  
	int j = 0;

	int dest_len = src_len;
	
	for (i = 0; i < src_len && j < FLOW_CTRL_BUF_SIZE; i++) 
	{	   
		if (p_src[i] == 0x77) 
		{			  
			if (FLOW_CTRL_BUF_SIZE - j >= 2) 
			{				 
				p_dst[j] = 0x77;				
				p_dst[j + 1] = 0x88;			   
				j += 2;
				dest_len++;
			}
			else
				break;		  
		} 
		else if (p_src[i] == 0x13) 
		{			
			if (FLOW_CTRL_BUF_SIZE - j >= 2) 
			{				 
				p_dst[j] = 0x77;				
				p_dst[j + 1] = 0xEC;			   
				j += 2;
				dest_len++;
			} 
			else
				break;		  
		} 
		else if (p_src[i] == 0x11) 
		{			
			if (FLOW_CTRL_BUF_SIZE - j >= 2) 
			{				 
				p_dst[j] = 0x77;				
				p_dst[j + 1] = 0xEE;			   
				j += 2;
				dest_len++;
			} 
			else				
				break;		  
		} 
		else 
		{			  
			p_dst[j] = p_src[i];			 
			j++;		 
		}	  
	}

	memset(&p_dst[dest_len], 0, FLOW_CTRL_BUF_SIZE - dest_len);
	
	return dest_len;
}

static bool mEscapeCharMark = false;
uint32_t flowctrl_decode(uint8_t *p_src, const uint32_t src_len)
{
	if (p_src == NULL || src_len == 0)
	{
		return 0;
	}

	int i = 0;
	int j = 0;

    int dest_len = src_len;
	int loop_cnt = src_len;

    for (i = 0; i < loop_cnt; i++) 
	{
		if (p_src[i] == 0x77) 
		{
			mEscapeCharMark = true;
			continue;
		}
		if (mEscapeCharMark) 
		{
			if (p_src[i] == 0x88) 
			{
				p_src[j] = 0x77;
				dest_len--;
			}
			else if (p_src[i] == 0xEE) 
			{
				p_src[j] = 0x11;
				dest_len--;
			}
			else if (p_src[i] == 0xEC) 
			{
				p_src[j] = 0x13;
				dest_len--;
			}
			else 
			{
				LOG_E("unrecognized char 0x%X", p_src[i]);
			}
			mEscapeCharMark = false;
		}
		else 
		{
			p_src[j] = p_src[i];
		}
		j++;
	}

    memset(&p_src[dest_len], 0, src_len - dest_len);

    return dest_len;
}

char* concat(const char* a, const char* b)
{
	int len_a = strlen(a);
	int len_b = strlen(b);
	char* con = malloc(len_a + len_b + 1);
	if (con == NULL)
	{
		LOG_E("concat malloc fail");
		return NULL;
	}
	memcpy(con, a, len_a);
	memcpy(con + len_a, b, len_b + 1);
	return con;

}

void print_byte_array(char* array, const char* name, uint32_t size)
{
	int i;
	printf("[I]%s:", name);
	for (i = 0; i < size; i++)
	{
		if (i > 0) printf(":");
		printf("%02X", array[i]& 0xFF);
	}
	
	printf("\n");
}