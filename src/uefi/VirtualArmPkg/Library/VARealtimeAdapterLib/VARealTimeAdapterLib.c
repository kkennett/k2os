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

#include <Library/VARealTimeAdapterLib.h>

void VIRTARMTIME_Init(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter)
{
    if (!apAdapter)
        return;

    /* mask and clear pending status of all interrupts */
    apAdapter->mIntMask = VIRTARM_REALTIMEADAPTER_INT_ALL;
    apAdapter->mIntPend = VIRTARM_REALTIMEADAPTER_INT_ALL;
}

void VIRTARMTIME_GetRealTime(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter, VASYSTEMTIME *apRetTime)
{
    UINT32 regVal;

    if (!apRetTime)
        return;
    ZeroMemory(apRetTime,sizeof(VASYSTEMTIME));
    if (!apAdapter)
        return;

    apAdapter->mRealTime_Command = VIRTARM_REALTIMEADAPTER_COMMAND_GET;
    
    regVal = apAdapter->mRealTime_YearMonth;
    apRetTime->wYear = regVal >> 16;
    apRetTime->wMonth = regVal & 0xFFFF;

    regVal = apAdapter->mRealTime_DayHour;
    apRetTime->wDay = regVal >> 16;
    apRetTime->wHour = regVal & 0xFFFF;

    regVal = apAdapter->mRealTime_MinSec;
    apRetTime->wMinute = regVal >> 16;
    apRetTime->wSecond = regVal & 0xFFFF;
}

void VIRTARMTIME_SetRealTime(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter, VASYSTEMTIME const *apTimeToSet)
{
    UINT32 regVal;

    if ((!apAdapter) || (!apTimeToSet))
        return;

    regVal = (((UINT32)apTimeToSet->wMinute) << 16) | ((UINT32)apTimeToSet->wSecond);
    apAdapter->mRealTime_MinSec = regVal;

    regVal = (((UINT32)apTimeToSet->wDay) << 16) | ((UINT32)apTimeToSet->wHour);
    apAdapter->mRealTime_DayHour = regVal;

    regVal = (((UINT32)apTimeToSet->wYear) << 16) | ((UINT32)apTimeToSet->wMonth);
    apAdapter->mRealTime_YearMonth = regVal;

    apAdapter->mRealTime_Command = VIRTARM_REALTIMEADAPTER_COMMAND_SET;
}

void VIRTARMTIME_GetAlarmTime(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter, VASYSTEMTIME *apRetAlarmTime)
{
    UINT32 regVal;

    if (!apRetAlarmTime)
        return;
    ZeroMemory(apRetAlarmTime,sizeof(VASYSTEMTIME));
    if (!apAdapter)
        return;

    apAdapter->mRealTime_Command = VIRTARM_REALTIMEADAPTER_COMMAND_GETALARM;
    
    regVal = apAdapter->mRealTime_YearMonth;
    apRetAlarmTime->wYear = regVal >> 16;
    apRetAlarmTime->wMonth = regVal & 0xFFFF;

    regVal = apAdapter->mRealTime_DayHour;
    apRetAlarmTime->wDay = regVal >> 16;
    apRetAlarmTime->wHour = regVal & 0xFFFF;

    regVal = apAdapter->mRealTime_MinSec;
    apRetAlarmTime->wMinute = regVal >> 16;
    apRetAlarmTime->wSecond = regVal & 0xFFFF;
}

void VIRTARMTIME_SetAlarmTime(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter, VASYSTEMTIME const *apAlarmTimeToSet)
{
    UINT32 regVal;

    if ((!apAdapter) || (!apAlarmTimeToSet))
        return;

    regVal = (((UINT32)apAlarmTimeToSet->wMinute) << 16) | ((UINT32)apAlarmTimeToSet->wSecond);
    apAdapter->mRealTime_MinSec = regVal;

    regVal = (((UINT32)apAlarmTimeToSet->wDay) << 16) | ((UINT32)apAlarmTimeToSet->wHour);
    apAdapter->mRealTime_DayHour = regVal;

    regVal = (((UINT32)apAlarmTimeToSet->wYear) << 16) | ((UINT32)apAlarmTimeToSet->wMonth);
    apAdapter->mRealTime_YearMonth = regVal;

    apAdapter->mRealTime_Command = VIRTARM_REALTIMEADAPTER_COMMAND_SETALARM;
}

void VIRTARMTIME_GetSystemTicks64(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter, UINT64 *apRetTicks)
{
    UINT32 val0;
    UINT32 val1;

    if (!apRetTicks)
        return;
    *apRetTicks = 0;
    if (!apAdapter)
        return;

    do {
        val1 = apAdapter->mSystemTicks64_High;
        val0 = apAdapter->mSystemTicks64_Low;
    } while (apAdapter->mSystemTicks64_High != val1);

    *apRetTicks = (((UINT64)val1)<<32) | ((UINT64)val0);
}

void VIRTARMTIME_ArmTimerDelta(volatile VIRTARM_REALTIMEADAPTER_REGS *apAdapter, UINTN aTimerNum, UINTN aDeltaMs)
{
    volatile UINT32 *pMatchTimer;

    if (aTimerNum>2)
        return;

    // mask
    apAdapter->mIntMask = (1<<aTimerNum);

    pMatchTimer = &apAdapter->mSetDeltaMatch0;
    pMatchTimer += aTimerNum;

    if (aDeltaMs<5)
        aDeltaMs = 5;

    // set delta to new target
    *pMatchTimer = aDeltaMs;
    
    // clear pending and unmask
    apAdapter->mIntPend = (1<<aTimerNum);
    apAdapter->mIntUnmask = (1<<aTimerNum);
}
