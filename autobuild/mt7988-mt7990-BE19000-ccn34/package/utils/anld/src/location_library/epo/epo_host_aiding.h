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

#ifndef EPO_HOST_AIDING_H
#define EPO_HOST_AIDING_H

#include <stdint.h>
#include <stdbool.h>

typedef enum{
    EPO_DEMO_TYPE_3D_START,
    EPO_DEMO_TYPE_GR_3D = EPO_DEMO_TYPE_3D_START,
    EPO_DEMO_TYPE_GPS_3D,
    EPO_DEMO_TYPE_BDS_3D,
    EPO_DEMO_TYPE_GA_3D,
    EPO_DEMO_TYPE_3D_END = EPO_DEMO_TYPE_GA_3D,

    EPO_DEMO_TYPE_QEPO_START,
    EPO_DEMO_TYPE_GR_Q = EPO_DEMO_TYPE_QEPO_START,
    EPO_DEMO_TYPE_GPS_Q,
    EPO_DEMO_TYPE_BD2_Q,
    EPO_DEMO_TYPE_GA_Q,
    EPO_DEMO_TYPE_QEPO_END = EPO_DEMO_TYPE_GA_Q,
    
    EPO_DEMO_TYPE_MAX
}epo_demo_type_t;

int epo_mutex_lock();
int epo_mutex_unlock();
void epo_mutex_init();
void epo_download_get_filename(epo_demo_type_t type, uint32_t index, char *file_name);
void epo_download_get_filepath(epo_demo_type_t type, uint32_t index, char *file_name);
bool epo_host_aiding(int8_t type, int32_t weekno, int32_t tow);
#endif /*EPO_HOST_AIDING_H*/