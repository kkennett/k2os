//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED 'AS IS', WITH NO WARRANTIES OR INDEMNITIES.
//
// Code Originator:  Kurt Kennett
// Last Updated By:  Kurt Kennett
//

#ifndef __VIRTARM_REALTIMEADAPTER_H
#define __VIRTARM_REALTIMEADAPTER_H

#include <virtarm.h>
#include <Adapters\VARealTimeAdapterRegs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _VASYSTEMTIME VASYSTEMTIME;
struct _VASYSTEMTIME
{
    UINT16 wYear;
    UINT16 wMonth;
    UINT16 wDayOfWeek;
    UINT16 wDay;
    UINT16 wHour;
    UINT16 wMinute;
    UINT16 wSecond;
    UINT16 wMilliseconds;
} VASYSTEMTIME;

//------------------------------------------------------------------------------
void VIRTARMTIME_Init(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter);
    
void VIRTARMTIME_GetRealTime(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter, VASYSTEMTIME *apRetTime);
void VIRTARMTIME_SetRealTime(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter, VASYSTEMTIME const *apTimeToSet);

void VIRTARMTIME_GetAlarmTime(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter, VASYSTEMTIME *apRetAlarmTime);
void VIRTARMTIME_SetAlarmTime(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter, VASYSTEMTIME const *apAlarmTimeToSet);

void VIRTARMTIME_GetSystemTicks64(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter, UINT64 *apRetTicks);

void VIRTARMTIME_ArmTimerDelta(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter, UINTN aTimerNum, UINTN aDeltaMs);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif  // __VIRTARM_REALTIMEADAPTER_H