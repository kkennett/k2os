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

BOOL
K2OS_Thread_WaitMany(
    K2OS_WaitResult *           apRetResult,
    UINT32                      aCount,
    K2OS_WAITABLE_TOKEN const * apWaitableTokens,
    BOOL                        aWaitAll,
    UINT32                      aTimeoutMs
)
{
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OS_THREAD_PAGE *          pThreadPage;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    K2OSKERN_SCHED_ITEM *       pSchedItem;
    K2STAT                      stat;
    BOOL                        disp;
    UINT32                      ixEntry;
    UINT32                      ixScan;
    BOOL                        result;

    if (NULL == apRetResult)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_TLSAREA_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    pThisThread->MacroWait.mpWaitingThread = pThisThread;
    pThisThread->MacroWait.mNumEntries = aCount;
    pThisThread->MacroWait.mIsWaitAll = FALSE;

    if (aCount > 0)
    {
        if (aCount > 1)
        {
            pThisThread->MacroWait.mIsWaitAll = aWaitAll;
        }
        else
        {
            pThisThread->MacroWait.mIsWaitAll = FALSE;
        }
    }
    else
    {
        if (aTimeoutMs == K2OS_TIMEOUT_INFINITE)
        {
            // thread is trying to sleep forever.
            *apRetResult = K2OS_Wait_Failed_SleepForever;
            pThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
            return FALSE;
        }

        if (0 == aTimeoutMs)
        {
            *apRetResult = K2OS_Wait_TimedOut;
            return FALSE;
        }

        pThisThread->MacroWait.mIsWaitAll = FALSE;
    }

    pThisThread->MacroWait.mTimerActive = FALSE;
    pThisThread->MacroWait.TimerItem.mIsMacroWait = TRUE;
    if (aTimeoutMs != K2OS_TIMEOUT_INFINITE)
    {
        pThisThread->MacroWait.TimerItem.mHfTicks = aTimeoutMs;
        K2OS_System_HfTickFromMsTick(&pThisThread->MacroWait.TimerItem.mHfTicks, &pThisThread->MacroWait.TimerItem.mHfTicks);
    }
    else
    {
        pThisThread->MacroWait.TimerItem.mHfTicks = K2OS_HFTIMEOUT_INFINITE;
    }

    pThisThread->MacroWait.mWaitResult = K2STAT_ERROR_UNKNOWN;

    stat = K2STAT_NO_ERROR;

    for (ixEntry = 0; ixEntry < aCount; ixEntry++)
    {
        pThisThread->MacroWait.WaitEntry[ixEntry].ObjRef.AsAny = NULL;

        stat = KernToken_Translate(apWaitableTokens[ixEntry], &pThisThread->MacroWait.WaitEntry[ixEntry].ObjRef);
        if (K2STAT_IS_ERROR(stat))
            break;

        //
        // check for duplicate objects
        //
        for (ixScan = 0; ixScan < ixEntry; ixScan++)
        {
            if (pThisThread->MacroWait.WaitEntry[ixEntry].ObjRef.AsAny ==
                pThisThread->MacroWait.WaitEntry[ixScan].ObjRef.AsAny)
            {
                KernObj_ReleaseRef(&pThisThread->MacroWait.WaitEntry[ixEntry].ObjRef);
                stat = K2STAT_ERROR_BAD_ARGUMENT;
                break;
            }
        }
        if (K2STAT_IS_ERROR(stat))
            break;

        //
        // check that object is waitable
        //
        switch (pThisThread->MacroWait.WaitEntry[ixEntry].ObjRef.AsAny->mObjType)
        {
            // these are waitable objects
        case KernObj_Process:
        case KernObj_Notify:
        case KernObj_Gate:
        case KernObj_Alarm:
        case KernObj_SemUser:
        case KernObj_Interrupt:
		case KernObj_MailboxOwner:
            break;

        case KernObj_Thread:
            if (pThisThread == pThisThread->MacroWait.WaitEntry[ixEntry].ObjRef.AsThread)
            {
                //
                // cannot wait on yourself
                //
                KernObj_ReleaseRef(&pThisThread->MacroWait.WaitEntry[ixEntry].ObjRef);
                stat = K2STAT_ERROR_BAD_TOKEN;
            }
            break;

        default:
            // this is not a waitable object
            KernObj_ReleaseRef(&pThisThread->MacroWait.WaitEntry[ixEntry].ObjRef);
            stat = K2STAT_ERROR_BAD_TOKEN;
            break;
        }
        if (K2STAT_IS_ERROR(stat))
        {
            break;
        }

        pThisThread->MacroWait.WaitEntry[ixEntry].mMacroIndex = ixEntry;
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(ixEntry < aCount);
        K2_ASSERT(0 != aCount);
        *apRetResult = K2OS_Wait_Failed_0 + ixEntry;
        if (ixEntry > 0)
        {
            do {
                --ixEntry;
                KernObj_ReleaseRef(&pThisThread->MacroWait.WaitEntry[ixEntry].ObjRef);
            } while (ixEntry > 0);
        }
        pThreadPage->mLastStatus = stat;
        return FALSE;
    }

    K2_ASSERT(ixEntry == aCount);

    pSchedItem = &pThisThread->SchedItem;
    pSchedItem->mType = KernSchedItem_KernThread_Wait;

    disp = K2OSKERN_SetIntr(FALSE);
    K2_ASSERT(disp);

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;

    // any wait forfeits the remainder of the threads quantum
    pThisThread->mQuantumHfTicksRemaining = 0;

    KernThread_CallScheduler(pThisCore);

    // interrupts will be back on again here

    // need to release all references here as the references were taken outside the monitor
    for (ixEntry = 0; ixEntry < aCount; ixEntry++)
    {
        KernObj_ReleaseRef(&pThisThread->MacroWait.WaitEntry[ixEntry].ObjRef);
    }
     
    result = pThisThread->Kern.mSchedCall_Result;

    if (0 == aCount)
    {
        K2_ASSERT(K2STAT_IS_ERROR(pThreadPage->mLastStatus));
        *apRetResult = K2OS_Wait_TimedOut;
    }
    else
    {
        *apRetResult = (K2OS_WaitResult)pThreadPage->mSysCall_Arg7_Result0;
    }

    return result;
}

BOOL 
K2OS_Thread_WaitOne(
    K2OS_WaitResult *   apRetResult, 
    K2OS_WAITABLE_TOKEN aToken, 
    UINT32              aTimeoutMs
)
{
    return K2OS_Thread_WaitMany(apRetResult, (NULL == aToken) ? 0 : 1, (NULL == aToken) ? NULL : &aToken, FALSE, aTimeoutMs);
}
