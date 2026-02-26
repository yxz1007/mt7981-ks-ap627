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

#include "hdl_config.h"
#include <memory.h>
#include <stdlib.h>
#include <time.h>

#include "config_parser.h"
#include "time_utils.h"
#include "utils.h"
#include "hdl_physical.h"

/*** BootROM HDL Doc Mapping Chapter 2 ***/
#include "hdl_brom_base.h"
/*** BootROM HDL Doc Mapping Chapter 3 ***/
#include "hdl_da_cmd.h"

HDL_CONFIG g_hdl_config = {
    .chip = CHIP_AG3335,
    .dl_slave_phy = SLAVE_UART,
    .dl_da_support_921600 = true,
    .dl_flash_baudrate = 921600,
    .clock_frequency = 1000000,
};

void hdl_config_slave_chip()
{
    #if defined(LOCATION_CHIP_AG3352)
    g_hdl_config.chip = CHIP_AG3352;
    #else
    g_hdl_config.chip = CHIP_AG3335;
    #endif /* defined(LOCATION_CHIP_AG3335) */

    #if defined(LOCATION_DOWNLOAD_DA_SUPPORT_921600)
    g_hdl_config.dl_da_support_921600 = true;
    #else
    g_hdl_config.dl_da_support_921600 = false;
    #endif /* defined(LOCATION_DOWNLOAD_DA_SUPPORT_921600) */

    g_hdl_config.dl_flash_baudrate = LOCATION_DOWNLOAD_FLASH_BAUDRATE;

    #if defined(LOCATION_SERIAL_PORT_TYPE_SPI)
    g_hdl_config.dl_slave_phy = SLAVE_SPI;
    location_config_spi_struct_t spi_info;
    location_config_get_spi_info(&spi_info);
    g_hdl_config.clock_frequency = spi_info.frequencey;
    memset(g_hdl_config.io_path, 0, sizeof(g_hdl_config.io_path));
    if (snprintf(g_hdl_config.io_path, sizeof(g_hdl_config.io_path), "%s", spi_info.path) < 0) {
        LOG_E("hdl_config_slave_chip, get io path fail");
    }
    #else
    g_hdl_config.dl_slave_phy = SLAVE_UART;
    memset(g_hdl_config.io_path, 0, sizeof(g_hdl_config.io_path));
    location_config_get_uart_port_path(g_hdl_config.io_path);
    #endif /* defined(LOCATION_SERIAL_PORT_TYPE_SPI) */

    memset(g_hdl_config.da_path, 0, sizeof(g_hdl_config.da_path));
    location_config_get_da_path(g_hdl_config.da_path);

    memset(g_hdl_config.image_path, 0, sizeof(g_hdl_config.image_path));
    location_config_get_image_path(g_hdl_config.image_path);
}

bool hdl_host_dl_demo()
{
    bool success = false;

    hdl_da_report_t report;

    int img_count = 0;

    // Read file
    image_info_t *cfg_list = get_image_info(&img_count);
    if (cfg_list == NULL)
    {
        LOG_E("Parsing config content failed");
        return false;
    }

    // Connect device
    success = HDL_COM_Init();
    if (!success)
    {
        HDL_COM_Deinit();
        return false;
    }

    /*** BootROM HDL Doc Mapping ***/
    /*** Chapter 2.2 Handshake Protocol ***/
    // HandShake with Chip BROM via Brom_StartCMD
    success = hdl_brom_start();
    if (!success)
    {
        LOG_E("========== Handshake fail ==========");
        HDL_COM_Deinit();
        return false;
    }
    LOG_I("========== Handshake pass ==========");

    if (g_hdl_config.dl_slave_phy == SLAVE_UART)
    {
        if (g_hdl_config.chip != CHIP_AG3352)
        {
            uint8_t uuid[16] ={0};

            if (!hdl_brom_read_uid((uint8_t *)&uuid))
            {
                HDL_COM_Deinit();
                return false;
            }

            LOG_I("========== Read UID ==========");
            print_byte_array((char *)uuid, "UID", 16);
        }

        //Set BTROM baudrate
        if (g_hdl_config.dl_da_support_921600)
        {
            success = hdl_brom_set_baudrate(921600);
            if (!success)
            {
                HDL_COM_Deinit();
                return false;
            }
        }
        else
        {
            success = hdl_brom_set_baudrate(115200);
            if (!success)
            {
                HDL_COM_Deinit();
                return false;
            }
        }

        LOG_I("========== Set BootROM baudrate pass ==========");
    }

    /*** BootROM HDL Doc Mapping ***/
    /*** Chapter 2.3 Disable Slave WDT ***/
    success = hdl_brom_disable_wdt();
    if (!success)
    {
        HDL_COM_Deinit();
        return false;
    }

    /*** BootROM HDL Doc Mapping ***/
    /*** Chapter 2.4 Send DA ***/
    success = hdl_brom_send_da();
    if (!success)
    {
        HDL_COM_Deinit();
        return false;
    }

    /*** BootROM HDL Doc Mapping ***/
    /*** Chapter 2.5 Jump to DA ***/
    success = hdl_brom_jump_da();
    if (!success)
    {
        HDL_COM_Deinit();
        return false;
    }

    /*** BootROM HDL Doc Mapping ***/
    /*** Chapter 3.1 Sync with DA ***/
    success = hdl_sync_with_da(&report);
    if (!success)
    {
        HDL_COM_Deinit();
        return false;
    }

#if 1
    // Total Format (optional)
    /*** BootROM HDL Doc Mapping ***/
    /*** Chapter 3.2 Format ***/
    pTime_t current_fm = getSystemTickMs();
    success = hdl_da_format(report.flash_base_addr, report.flash_size);
    if (!success)
    {
        return false;
    }
    pTime_t duration_fm_ms = getSystemTickMs() - current_fm;
    LOG_I("Format time: %d ms", (int32_t)duration_fm_ms);
#endif

#if 1
    // Download
    /*** BootROM HDL Doc Mapping ***/
    /*** Chapter 3.3 Download ***/
    pTime_t current_dl = getSystemTickMs();

    uint32_t i;
    for (i = 0; i < img_count; i++)
    {
        image_info_t image_info = cfg_list[i];
        if (!download_image(&image_info))
        {
            LOG_E("download %s FAILED", image_info.image_name);
            return false;
        }
        LOG_I("download %s PASS", image_info.image_name);
    }

    pTime_t duration_dl_ms = getSystemTickMs() - current_dl;
    LOG_I("Download time: %d ms", (int32_t)duration_dl_ms);
#endif

#if 1
    // Check image crc
    pTime_t current_crc = getSystemTickMs();

    uint32_t j;
    for (j = 0; j < img_count; j++)
    {
        image_info_t image_info = cfg_list[j];
        if (!check_image_crc(&image_info))
        {
            LOG_E("check_image_crc %s FAILED", image_info.image_name);
            return false;
        }
        LOG_I("check_image_crc %s PASS", image_info.image_name);
    }

    pTime_t duration_crc_ms = getSystemTickMs() - current_crc;
    LOG_I("Check image crc time: %d ms", (int32_t)duration_crc_ms);
#endif

    // Disconnect Device
    /*** BootROM HDL Doc Mapping ***/
    /*** Chapter 3.6 Disconnect ***/
    success = hdl_finish_race(false);

    HDL_COM_Deinit();

    if (!success)
    {
        return false;
    }

    if (cfg_list != NULL)
    {
        free(cfg_list);
    }

    return success;
}

int firmware_download()
{
    hdl_config_slave_chip();
    initTimebase();
    LOG_I("========== Download start ========");
    int ret = 0;
    if (!hdl_host_dl_demo())
    {
        ret = -1;
        LOG_I("========== Download Fail ========");
    }
    else
    {
        LOG_I("========== Download Finish ========");
    }
    return ret;
}