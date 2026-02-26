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

#ifndef ANLD_CTRL_H
#define ANLD_CTRL_H

#define ANLD_BUF_LEN    (512)

/** @brief This enum defines ANLD response message type. */
typedef enum {
    ANLD_RESPONSE_MSG_TYPE_START,
    ANLD_RESPONSE_MSG_TYPE_STOP,
    ANLD_RESPONSE_MSG_TYPE_UPGRADE,
    ANLD_RESPONSE_MSG_TYPE_SAVE_LOCATION,
    ANLD_RESPONSE_MSG_TYPE_DEBUG_CONFIG,
    ANLD_RESPONSE_MSG_TYPE_PAIR_COMMAND,
    ANLD_RESPONSE_MSG_TYPE_NMEA,
    ANLD_RESPONSE_MSG_TYPE_TIME_VALID,
    ANLD_RESPONSE_MSG_TYPE_EPO_DOWNLOAD,
    ANLD_RESPONSE_MSG_TYPE_LOCATION,
} anld_response_msg_type_t;

/** @brief This enum defines the configuration type of debug log. */
typedef enum {
    ANLD_CTRL_DEBUG_CONFIG_DISABLE,
    ANLD_CTRL_DEBUG_CONFIG_SAVE_TO_FILE,
    ANLD_CTRL_DEBUG_CONFIG_SEND_BY_PORT,
} anld_ctrl_debug_config_t;

/** @brief This struct defines ANLD response message parameters. */
typedef struct {
    anld_response_msg_type_t type;
    int state;                     /* 0:success  -1:fail */
    char buf[ANLD_BUF_LEN];
} anld_ctrl_response_message_struct_t;

/** @brief This struct defines location information parameters receive from ANLD*/
typedef struct {
    char utc[12];          /* hhmmss.sss, UTC of this position report*/
    char latitude[15];     /* latitude, degrees */
    char longitude[15];    /* longitude, degrees */
    char height[10];       /* AGL height, meters */
    char majorAxis[10];    /* standard deviation of semi-major axis of error ellipse, meters */
    char minorAxis[10];    /* standard deviation of semi-minor axis of error ellipse, meters */
    char orientation[10];  /* orientation of semi-major axis of error ellipse, degrees from true north */
    char vacc[12];         /* position vertical acuracy, meters */
} anld_ctrl_location_t;

/** @brief This typedef defines user's callback function prototype.
 *             This callback function will be called when tclient receives ANLD messages.
 *             parameter "message" : ANLD response message structure
 */
typedef void (*anld_ctrl_callback_t)(anld_ctrl_response_message_struct_t *message);

/**
 * @brief     start GNSS
 * @return
 *                -1: send message to ANLD error
 *                others: the bytes send to ANLD
 * @note
 *            ANLD will send NMEA to tclient after power on.
 */
int anld_ctrl_start();

/**
 * @brief     stop GNSS
 * @return
 *                -1: send message to ANLD error
 *                others: the bytes send to ANLD
 */
int anld_ctrl_stop();

/**
 * @brief     download firmware to GNSS chip
 * @return
 *                -1: send message to ANLD error
 *                others: the bytes send to ANLD
 * @note
 *            must copy the DA bin to the DA path and copy the firmware 
 *            to the image path before upgrading, the path can be configured
 *            throught configuration file
 */
int anld_ctrl_upgrade();

/**
 * @brief     save the location information to location.dat
 * @return
 *                -1: send message to ANLD error
 *                others: the bytes send to ANLD
 * @note
 *            It is necessary to send this message to update location information,
 *            so location aiding can be used to accelerate the next positioning
 */
int anld_ctrl_save_location();

/**
 * @brief     debug configuration
 * @return
 *                -1: send message to ANLD error
 *                others: the bytes send to ANLD
 * @param[in] type  config type. 
 *                  #ANLD_CTRL_DEBUG_CONFIG_DISABLE ANLD do not handle debug log
 *                  #ANLD_CTRL_DEBUG_CONFIG_SAVE_TO_FILE save GNSS debug log to log.dat
  *                 #ANLD_CTRL_DEBUG_CONFIG_SEND_BY_PORT send GNSS debug log by configured port
 * @note
 *             execute this function before GNSS start
 */
int anld_ctrl_debug_config(anld_ctrl_debug_config_t type);

/**
 * @brief     send PAIR command to GNSS
 * @return
 *                -1: send message to ANLD error
 *                others: the bytes send to ANLD
 * @param[in] buf  the command to send
 * @note
 *             start GNSS before executing this function
 *             example: anld_ctrl_send_pair_command("382,1")
 */
int anld_ctrl_send_pair_command(char *buf);

/**
 * @brief     notify ANLD that the time of the platform is valid
 * @return
 *                -1: send message to ANLD error
 *                others: the bytes send to ANLD
 * @note
 *             it is necessary to execute this function when the time is valid.
 *             ANLD will use the time to download EPO, and time aiding to 
 *             accelerate positioning
 */
int anld_ctrl_send_time_valid();

/**
 * @brief     This function create communication with ANLD 
 *                and register callback function to handle messages from ANLD
 * @return
 *                -1: initialization failed
 *                0: success
 * @note
 *             it is necessary to execute this function before use other functions
 */
int anld_ctrl_init(anld_ctrl_callback_t callback_function);

/**
 * @brief     This function close communication with ANLD 
 */
void anld_ctrl_deinit();
#endif /* ANLD_CTRL_H */