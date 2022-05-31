//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2020, Kurt Kennett
//   All rights reserved.
//   
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions are met:
//   
//   1. Redistributions of source code must retain the above copyright notice, this
//      list of conditions and the following disclaimer.
//   
//   2. Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.
//   
//   3. Neither the name of the copyright holder nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//   
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "kern.h"

K2STAT  
KernAlarm_Create(
    UINT64 const *      apPeriodMs,
    BOOL                aIsPeriodic,
    K2OSKERN_OBJREF *   apRetAlarmRef
)
{
    K2OSKERN_OBJ_ALARM *pAlarm;

    pAlarm = (K2OSKERN_OBJ_ALARM *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_ALARM));
    if (NULL == pAlarm)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pAlarm, sizeof(K2OSKERN_OBJ_ALARM));

    pAlarm->Hdr.mObjType = KernObj_Alarm;
    K2LIST_Init(&pAlarm->Hdr.RefObjList);
    
    KernTimer_HfTicksFromMs(&pAlarm->SchedLocked.mHfTicks, apPeriodMs);
    pAlarm->SchedLocked.mIsPeriodic = aIsPeriodic;
    pAlarm->SchedLocked.TimerItem.mHfTicks = pAlarm->SchedLocked.mHfTicks;
    K2LIST_Init(&pAlarm->SchedLocked.MacroWaitEntryList);

    KernObj_CreateRef(apRetAlarmRef, &pAlarm->Hdr);

    return K2STAT_NO_ERROR;
}

void
KernAlarm_PostCleanupDpc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apArg
)
{
    K2OSKERN_OBJ_ALARM *    pAlarm;
    pAlarm = K2_GET_CONTAINER(K2OSKERN_OBJ_ALARM, apArg, Hdr.ObjDpc.Func);
    K2_ASSERT(pAlarm->Hdr.mObjType == KernObj_Alarm);
    K2_ASSERT(pAlarm->SchedLocked.mTimerActive == FALSE);
    K2_ASSERT(0 == pAlarm->SchedLocked.MacroWaitEntryList.mNodeCount);
    KTRACE(apThisCore, 1, KTRACE_ALARM_POST_CLEANUP_DPC);
    K2MEM_Zero(pAlarm, sizeof(K2OSKERN_OBJ_ALARM));
    KernHeap_Free(pAlarm);
}

void
KernAlarm_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_ALARM *        apAlarm
)
{
//    K2OSKERN_Debug("Alarm Cleanup\n");
    K2_ASSERT(0 == apAlarm->Hdr.RefObjList.mNodeCount);
    K2_ASSERT(0 == apAlarm->SchedLocked.MacroWaitEntryList.mNodeCount);
    apAlarm->CleanupSchedItem.mType = KernSchedItem_Alarm_Cleanup;
    KernTimer_GetHfTick(&apAlarm->CleanupSchedItem.mHfTick);
    KernSched_QueueItem(&apAlarm->CleanupSchedItem);
}

void    
KernAlarm_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJREF         alarmRef;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OS_ALARM_TOKEN        tokAlarm;
    UINT64                  msTick;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    if (0 == apCurThread->mSysCall_Arg0)
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        return;
    }

    alarmRef.Ptr.AsHdr = NULL;
    msTick = apCurThread->mSysCall_Arg0;
    stat = KernAlarm_Create(&msTick, (BOOL)pThreadPage->mSysCall_Arg1, &alarmRef);
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
        return;
    }

    stat = KernProc_TokenCreate(apCurThread->ProcRef.Ptr.AsProc, alarmRef.Ptr.AsHdr, &tokAlarm);
    if (!K2STAT_IS_ERROR(stat))
    {
        apCurThread->SchedItem.mType = KernSchedItem_Thread_SysCall;
        apCurThread->SchedItem.Args.Alarm_Create.mAlarmToken = (UINT32)tokAlarm;
        KernObj_CreateRef(&apCurThread->SchedItem.ObjRef, alarmRef.Ptr.AsHdr);
        KernTimer_GetHfTick(&apCurThread->SchedItem.mHfTick);
        KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
        KernSched_QueueItem(&apCurThread->SchedItem);
    }

    KernObj_ReleaseRef(&alarmRef);

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

