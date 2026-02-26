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

#include "anld_ctrl.h"

static int g_display_nmea = 1;

void message_callback(anld_ctrl_response_message_struct_t *message)
{
    // TODO: Customization
    switch (message->type) {
        case ANLD_RESPONSE_MSG_TYPE_START: {
            
            break;
        }
        case ANLD_RESPONSE_MSG_TYPE_STOP: {
            
            break;
        }
        case ANLD_RESPONSE_MSG_TYPE_UPGRADE: {
            
            break;
        }
        case ANLD_RESPONSE_MSG_TYPE_SAVE_LOCATION: {
            
            break;
        }
        case ANLD_RESPONSE_MSG_TYPE_DEBUG_CONFIG: {
            
            break;
        }
        case ANLD_RESPONSE_MSG_TYPE_PAIR_COMMAND: {
            
            break;
        }
        case ANLD_RESPONSE_MSG_TYPE_NMEA: {
            
            break;
        }
        case ANLD_RESPONSE_MSG_TYPE_TIME_VALID: {
            
            break;
        }
        case ANLD_RESPONSE_MSG_TYPE_EPO_DOWNLOAD: {
            
            break;
        }
        case ANLD_RESPONSE_MSG_TYPE_LOCATION: {
            anld_ctrl_location_t location;
            char buf[256];
            int ret;
            memset(buf, 0, sizeof(buf));
            memcpy(&location, message->buf, sizeof(message->buf) > sizeof(location) - 1 ? sizeof(location) - 1 : sizeof(message->buf));
            location.utc[sizeof(location.utc) - 1] = '\0';
            location.longitude[sizeof(location.longitude) - 1] = '\0';
            location.latitude[sizeof(location.latitude) - 1] = '\0';
            location.height[sizeof(location.height) - 1] = '\0';
            location.majorAxis[sizeof(location.majorAxis) - 1] = '\0';
            location.minorAxis[sizeof(location.minorAxis) - 1] = '\0';
            location.orientation[sizeof(location.orientation) - 1] = '\0';
            location.vacc[sizeof(location.vacc) - 1] = '\0';
            ret = snprintf(buf, sizeof(buf), "%s,%s,%s,%s,%s,%s,%s,%s",
                    location.utc,
                    location.longitude,
                    location.latitude,
                    location.height,
                    location.majorAxis,
                    location.minorAxis,
                    location.orientation,
                    location.vacc
                    );
            if (ret > 0 && (g_display_nmea == 1)) {
                printf("message_callback, type:%d,state:%d,%s\n", message->type, message->state, buf);
            }
            break;
        }
        default: {
            break;
        }
    }
    if (g_display_nmea == 1) {
        if (message->type != ANLD_RESPONSE_MSG_TYPE_LOCATION) {
            printf("message_callback, type:%d,state:%d,%s\n", message->type, message->state, message->buf);
        }
    }
}

int main(int argc, const char *argv[])
{
    char type;
    int ret;
    if (anld_ctrl_init(message_callback) != 0) {
        return -1;
    }
    while (1) {
        printf("put number: 1-start, 2-stop, 3-upgrade, 4-save location, 5-debug disable, 6-debug save to file, 7-debug send by port, 8-pair command, 9-time valid, a-change nmea display\n");
        type = -1;
        if (scanf("%c", &type) == EOF) {
            printf("input error\n");
            continue;
        }
        if (fflush(stdin) != 0) {
            printf("fflush error\n");
        }
        switch (type) {
            case '1': {
                ret = anld_ctrl_start();
                printf("anld_ctrl_start, %d\n", ret);
                break;
            }
            case '2': {
                ret = anld_ctrl_stop();
                printf("anld_ctrl_stop, %d\n", ret);
                break;
            }
            case '3': {
                ret = anld_ctrl_upgrade();
                printf("anld_ctrl_upgrade, %d\n", ret);
                break;
            }
            case '4': {
                ret = anld_ctrl_save_location();
                printf("anld_ctrl_save_location, %d\n", ret);
                break;
            }
            case '5': {
                ret = anld_ctrl_debug_config(ANLD_CTRL_DEBUG_CONFIG_DISABLE);
                printf("anld_ctrl_debug_config, %d\n", ret);
                break;
            }
            case '6': {
                ret = anld_ctrl_debug_config(ANLD_CTRL_DEBUG_CONFIG_SAVE_TO_FILE);
                printf("anld_ctrl_debug_config, %d\n", ret);
                break;
            }
            case '7': {
                ret = anld_ctrl_debug_config(ANLD_CTRL_DEBUG_CONFIG_SEND_BY_PORT);
                printf("anld_ctrl_debug_config, %d\n", ret);
                break;
            }
            case '8': {
                char buf[ANLD_BUF_LEN];
                printf("put command:\n");
                if (fgets(buf, sizeof(buf), stdin) == NULL) {
                    printf("input error\n");
                    break;
                }
                if (fflush(stdin) != 0) {
                    printf("fflush error\n");
                }
                if (strlen(buf) > 0) {
                    buf[strlen(buf) - 1] = '\0';
                    ret = anld_ctrl_send_pair_command(buf);
                    printf("anld_ctrl_send_pair_command, %d\n", ret);
                }
                break;
            }
            case '9': {
                ret = anld_ctrl_send_time_valid();
                printf("anld_ctrl_send_time_valid, %d\n", ret);
                break;
            }
            case 'a': {
                g_display_nmea = (g_display_nmea == 0) ? 1 : 0;
            }
            default:
                break;
        }
    }
    anld_ctrl_deinit();
    return 0;
}