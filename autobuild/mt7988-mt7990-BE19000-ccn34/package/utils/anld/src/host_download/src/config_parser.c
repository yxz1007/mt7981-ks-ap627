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

#include "config_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "hdl_config.h"
#include "time_utils.h"
#include "utils.h"

#define MAX_LEN 256

extern HDL_CONFIG g_hdl_config;

size_t getFileSize(const char* filename)
{
    if (filename == NULL)
    {
        return 0;
    }

    FILE* f = fopen(filename, "rb");
    if (f == NULL) {
        return 0;
    }
    if (fseek(f, 0, SEEK_END) != 0)
    {
        if (fclose(f) == EOF)
		{
			LOG_E("The file(%s) could not closed.", filename);
			return 0;
		}
        return 0;
    }

    long lsize = ftell(f);
    size_t size = 0;
    if (lsize < 0)
        size = 0;
    else
        size = lsize;    
    if (fclose(f) == EOF)
    {
        LOG_E("The file(%s) could not closed.", filename);
        return 0;
    }
    return size;
}

void removeLeading(char* str, char* str1)
{
    int idx = 0, j, k = 0;

    // Iterate String until last
    // leading space character
    while (str[idx] == ' ' || str[idx] == '\t'  || str[idx] == '\r' || str[idx] == '\n')
    {
        idx++;
    }

    // Run a for loop from index until the original
    // string ends and copy the content of str to str1
    for (j = idx; str[j] != '\0'; j++)
    {
        if (str[j] == ' ') continue;
        str1[k] = str[j];
        k++;
    }

    // Insert a string terminating character
    // at the end of new string
    str1[k] = '\0';

}

image_info_t* read_cfg_file(FILE* fp, int img_count)
{
    image_info_t* cfg_list = (image_info_t*)malloc(img_count * (sizeof(image_info_t)));
    if (cfg_list == NULL)
	{
		LOG_E("read_cfg_file malloc cfg_list fail");
		return NULL;
	}
    memset(cfg_list, 0, img_count * sizeof(image_info_t));

    char buffer[MAX_LEN];
    char buffer2[MAX_LEN];
    char buffer3[MAX_LEN];

    char buf[MAX_LEN];
    
    realpath(g_hdl_config.image_path, buf);
    char *dir = dirname(buf);
    printf("The FW load directory is: %s\n", dir);


    int index = 0;
    while (fgets(buffer, MAX_LEN, fp) != NULL)
    {
        // Remove trailing newline        
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strstr(buffer, "- rom:"))
        {

            int count = 3;
            while (count > 0)
            {
                memset(buffer2, 0x00, MAX_LEN);
                memset(buffer3, 0x00, MAX_LEN);

                if(fgets(buffer, MAX_LEN, fp)==NULL) //read file
                {
                    break;
                }
                buffer[strcspn(buffer, "\r\n")] = 0; //add end

                removeLeading(buffer, buffer2);

                if (buffer2[0] == '#')
                {
                    LOG_I("skip [%s]:\n", buffer2);
                    break;
                }

                if (img_count == index)
                {
                    LOG_E("Out of index(0~)=%d. Total image count= %d.", index, img_count);
                    free(cfg_list);
                    cfg_list = NULL;
                    return NULL;
                }

                if (strstr(buffer2, "file:"))
                {
                    strncpy(buffer3, buffer2 + 5, MAX_LEN - 5);
                    //sprintf((char*)cfg_list[index].image_name, "load/%s", buffer3);
                    char *full_path1 = concat(dir,"/");
                    if (full_path1 == NULL)
                    {
                        LOG_E("read_cfg_file concat ERROR\n");
                        free(cfg_list);
                        cfg_list = NULL;
                        return NULL;
                    }
                    char *full_path = concat(full_path1,buffer3);
                    if (full_path == NULL)
                    {
                        LOG_E("read_cfg_file concat ERROR\n");
                        free(full_path1);
                        free(cfg_list);
                        cfg_list = NULL;
                        return NULL;
                    }
                    // if(sprintf((char*)cfg_list[index].image_name, "%s", full_path)<0)
                    // {
                    //     LOG_E("read_cfg_file sprintf ERROR\n");
                    // }
                    int rtn_pf = snprintf((char*)cfg_list[index].image_name, sizeof(cfg_list[index].image_name), "%s", full_path);
                    if (rtn_pf <= 0)
                    {
                        LOG_E("read_cfg_file snprintf ERROR\n");
                    }
                    LOG_I("file name=%s\n", cfg_list[index].image_name);
                    free(full_path1);
                    free(full_path);
                }
                else if (strstr(buffer2, "name:"))
                {
                    //strncpy(buffer3, buffer2+5, MAX_LEN-5);
                    //LOG_I("%s\n", buffer3);
                }
                else if (strstr(buffer2, "begin_address:"))
                {
                    strncpy(buffer3, buffer2 + 14, MAX_LEN - 14);

                    cfg_list[index].begin_address = strtol(buffer3, NULL, 0);

                    LOG_I("begin_address:[%s]=> 0x%x\n", buffer3, cfg_list[index].begin_address);
                }
                else
                {
                    LOG_E("UnExpected line=%s", buffer);
                    free(cfg_list);
                    return NULL;
                }
                --count;

            }

            if (count == 0)
            {
                ++index;
            }
        }
    }

    return cfg_list;
}

image_info_t* get_image_info(int *img_count)
{
    FILE* fp;
    fp = fopen(g_hdl_config.image_path, "r");

    if (fp == NULL) {
        LOG_E("open file Failed:");
        return NULL;
    }

    char buffer[MAX_LEN];
    char buffer2[MAX_LEN];
    while (fgets(buffer, MAX_LEN, fp) != NULL)
    {
        buffer[strcspn(buffer, "\r\n")] = 0;
        memset(buffer2, 0x00, MAX_LEN);
        removeLeading(buffer, buffer2);

        if (buffer2[0] == '#')
        {
            continue;
        }

        if (strstr(buffer, "- rom:"))
        {
            (*img_count)++;
        }
    }

    LOG_I("image count=%d", (*img_count));

    if ((*img_count) == 0)
    {
        LOG_E("Parsing config image count failed");
        if (fclose(fp) == EOF)
        {
            LOG_E("The file(%s) could not closed.", g_hdl_config.image_path);
        }
        fp = NULL;
        return NULL;
    }
    rewind(fp); //set to start

    image_info_t* cfg_list = read_cfg_file(fp, (*img_count));    
    if (fclose(fp) == EOF)
    {
        LOG_E("The file(%s) could not closed.", g_hdl_config.image_path);
        return 0;
    }

    return cfg_list;
}
