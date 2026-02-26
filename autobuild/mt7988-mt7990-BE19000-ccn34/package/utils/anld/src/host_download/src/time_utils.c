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

#include "time_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hdl_config.h"
#include "utils.h"

#ifdef HDL_ON_LINUX
#include <unistd.h>
#include <sys/time.h>
#else
#include <Windows.h>
#endif

#ifdef HDL_ON_WINDOWS
static int64_t freq_QuadPart = 0;
#endif

static int64_t currentTimeBase = -1;
static char timeAscii[20] = { 0 };
static char timestamp[24] = { 0 };


void initTimebase()
{
#ifdef HDL_ON_LINUX
    struct timeval start;
    int64_t ms;
    gettimeofday(&start, NULL);
    ms = 1000 * (start.tv_sec) + start.tv_usec / 1000;

    if (currentTimeBase == -1)
    {
        currentTimeBase = ms;
}
#else
    //currentTimeBase = getSystemTickMs();

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    //freq.QuadPart /= 1000000; // us

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    freq_QuadPart = freq.QuadPart;
    currentTimeBase = now.QuadPart;

    LOG_I("freq_QuadPart=%I64d", freq_QuadPart);
    LOG_I("currentTimeBase=%I64d", currentTimeBase);
#endif

    // int64_t ns = clock();
    // if (currentTimeBase == -1) 
    // {
    //     currentTimeBase = ns;
    // }
}

int64_t getSystemTickMs()
{
#ifdef HDL_ON_LINUX
    struct  timeval  ts;
    gettimeofday(&ts,NULL);
    return ts.tv_sec * 1000 + ts.tv_usec / 1000;
#else
    //return (int64_t)(clock() / CLOCKS_PER_SEC * 1000);

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    return (double)(now.QuadPart) * 1000 / (double)freq_QuadPart;

#endif

    //return clock();
}

const char *getSystemRunMs() 
{
#ifdef HDL_ON_LINUX
    struct  timeval  ts;
	gettimeofday(&ts,NULL);
    int64_t ms = ts.tv_sec * 1000 + ts.tv_usec / 1000;
	double delta = ms - currentTimeBase;
	int rtn = snprintf(timeAscii, sizeof(timeAscii), "[%8.3f]", delta / 1000);
    if(rtn <= 0)
    {
        LOG_E("getSystemRunMs ERROR!");
    }
	return timeAscii;
#else	
	int64_t ns = getSystemTickMs();
	double delta = ns - (currentTimeBase * 1000 / (double)freq_QuadPart);
	snprintf(timeAscii, sizeof(timeAscii), "[%8.3f]", (double)(delta/1000));
	return timeAscii;
#endif

 
    // int64_t ns = clock();
    // double delta = ns - currentTimeBase;
    // snprintf(timeAscii, sizeof(timeAscii), "[%8.3f]", delta / 1000);
    // return timeAscii;
}

const char* getTimestamp()
{
	time_t t = time(NULL);
    if(t <= 0)
    {
        return NULL;
    }
	struct tm *ltm = localtime(&t);
    if (ltm == NULL)
    {
        return NULL;
    }

	struct timespec timespec;
	int get_rtn = timespec_get(&timespec, TIME_UTC);
    if(get_rtn ==0)
    {
        LOG_E("timespec_get ERROR!");
    }
	int millis = ((int64_t)timespec.tv_nsec) / 1000000;

	//snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d.%03d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, millis);
    int rtn = snprintf(timestamp, sizeof(timestamp), "%04u-%02u-%02u %02u:%02u:%02u.%03u", 
    (unsigned)(ltm->tm_year + 1900)%10000u, (unsigned)(ltm->tm_mon + 1)%100u, 
    (unsigned)ltm->tm_mday%100u, (unsigned)ltm->tm_hour%100u, 
    (unsigned)ltm->tm_min%100u, (unsigned)ltm->tm_sec%100u, (unsigned)millis%1000u);
    if(rtn <= 0)
    {
        LOG_E("getTimestamp ERROR!");
    }
	return timestamp;
}

void portTaskDelayMs(pTime_t ms)
{
#ifdef HDL_ON_LINUX
    struct timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000 * 1000;

    int ret = nanosleep(&req, NULL);
    if (-1 == ret)
    {
        LOG_E("\t nanousleep %u not support\n", (unsigned int)ms);
    }
#else
    //usleep(ms * 1000);
    Sleep(ms);
#endif
}
