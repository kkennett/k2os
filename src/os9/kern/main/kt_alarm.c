//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2023, Kurt Kennett
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

K2OS_ALARM_TOKEN 
K2OS_Alarm_Create(
    UINT32  aPeriodMs,
    BOOL    aIsPeriodic
)
{
    K2STAT                      stat;
    K2OSKERN_OBJREF             alarmRef;
    UINT64                      periodMs;
    K2OS_TOKEN                  tokAlarm;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    K2OSKERN_SCHED_ITEM *       pSchedItem;

    periodMs = aPeriodMs;

    alarmRef.AsAny = NULL;
    stat = KernAlarm_Create(&periodMs, aIsPeriodic, &alarmRef);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    stat = KernToken_Create(alarmRef.AsAny, &tokAlarm);

    if (K2STAT_IS_ERROR(stat))
    {
        KernObj_ReleaseRef(&alarmRef);
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    K2OSKERN_SetIntr(FALSE);

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    pThisThread = pThisCore->mpActiveThread;
    K2_ASSERT(pThisThread->mIsKernelThread);

    pSchedItem = &pThisThread->SchedItem;
    pSchedItem->mType = KernSchedItem_KernThread_MountAlarm;
    KernObj_CreateRef(&pSchedItem->ObjRef, alarmRef.AsAny);
    KernObj_ReleaseRef(&alarmRef);

    KernThread_CallScheduler(pThisCore);

    KernObj_ReleaseRef(&pSchedItem->ObjRef);

    return tokAlarm;
}
