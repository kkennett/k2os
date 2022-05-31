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

void    
KernWait_SysCall(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    UINT_PTR                tokCount;
    UINT_PTR                timeoutMs;
    UINT_PTR                ixEntry;
    UINT_PTR                ixScan;

    pProc = apCurThread->ProcRef.Ptr.AsProc;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    apCurThread->MacroWait.mpWaitingThread = apCurThread;

    apCurThread->MacroWait.mNumEntries = tokCount = apCurThread->mSysCall_Arg0;

    timeoutMs = pThreadPage->mSysCall_Arg2;

    if (tokCount > 0)
    {
        if (tokCount > 1)
        {
            apCurThread->MacroWait.mIsWaitAll = (pThreadPage->mSysCall_Arg1 != 0) ? TRUE : FALSE;
        }
        else
        {
            apCurThread->MacroWait.mIsWaitAll = FALSE;
        }
    }
    else
    {
        if (timeoutMs == K2OS_TIMEOUT_INFINITE)
        {
            // thread is trying to sleep forever.
            K2OSKERN_Debug("%d - Thread %d\n", __LINE__, apCurThread->mGlobalIx);
            apCurThread->mSysCall_Result = pThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
            return;
        }
        apCurThread->MacroWait.mIsWaitAll = FALSE;
    }

    apCurThread->MacroWait.mTimerActive = FALSE;
    apCurThread->MacroWait.TimerItem.mIsMacroWait = TRUE;
    if (timeoutMs != K2OS_TIMEOUT_INFINITE)
    {
        apCurThread->MacroWait.TimerItem.mHfTicks = timeoutMs;
        KernTimer_HfTicksFromMs(&apCurThread->MacroWait.TimerItem.mHfTicks, &apCurThread->MacroWait.TimerItem.mHfTicks);
    }
    else
    {
        apCurThread->MacroWait.TimerItem.mHfTicks = K2OS_HFTIMEOUT_INFINITE;
    }

    apCurThread->MacroWait.mWaitResult = K2STAT_ERROR_UNKNOWN;

    stat = K2STAT_NO_ERROR;

    for (ixEntry = 0; ixEntry < tokCount; ixEntry++)
    {
        apCurThread->MacroWait.WaitEntry[ixEntry].ObjRef.Ptr.AsHdr = NULL;
        stat = KernProc_TokenTranslate(
            pProc, 
            pThreadPage->mWaitToken[ixEntry], 
            &apCurThread->MacroWait.WaitEntry[ixEntry].ObjRef
        );
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("%d - Thread %d\n", __LINE__, apCurThread->mGlobalIx);
            K2_ASSERT(0);
            break;
        }

        //
        // check for duplicate objects
        //
        for (ixScan = 0; ixScan < ixEntry; ixScan++)
        {
            if (apCurThread->MacroWait.WaitEntry[ixEntry].ObjRef.Ptr.AsHdr ==
                apCurThread->MacroWait.WaitEntry[ixScan].ObjRef.Ptr.AsHdr)
            {
                KernObj_ReleaseRef(&apCurThread->MacroWait.WaitEntry[ixEntry].ObjRef);
                stat = K2STAT_ERROR_BAD_ARGUMENT;
                break;
            }
        }
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("%d - Thread %d\n", __LINE__, apCurThread->mGlobalIx);
            break;
        }

        //
        // check that object is waitable
        //
        switch (apCurThread->MacroWait.WaitEntry[ixEntry].ObjRef.Ptr.AsHdr->mObjType)
        {
        // these are waitable objects
        case KernObj_Process:
            if (apCurThread->ProcRef.Ptr.AsProc == apCurThread->MacroWait.WaitEntry[ixEntry].ObjRef.Ptr.AsProc)
            {
                //
                // cannot wait on your own process
                //
                KernObj_ReleaseRef(&apCurThread->MacroWait.WaitEntry[ixEntry].ObjRef);
                K2OSKERN_Debug("%d - Thread %d\n", __LINE__, apCurThread->mGlobalIx);
                stat = K2STAT_ERROR_BAD_TOKEN;
            }
            break;

        case KernObj_Notify:
        case KernObj_Gate:
        case KernObj_Alarm:
        case KernObj_SemUser:
        case KernObj_Interrupt:
            break;

        case KernObj_Thread:
            if (apCurThread == apCurThread->MacroWait.WaitEntry[ixEntry].ObjRef.Ptr.AsThread)
            {
                //
                // cannot wait on yourself
                //
                KernObj_ReleaseRef(&apCurThread->MacroWait.WaitEntry[ixEntry].ObjRef);
                K2OSKERN_Debug("%d - Thread %d\n", __LINE__, apCurThread->mGlobalIx);
                stat = K2STAT_ERROR_BAD_TOKEN;
            }
            break;

        case KernObj_Mailbox:
            if (ixEntry == (tokCount - 1))
            {
                //
                // impossible to be a wait all if there is a mailbox token
                //
                apCurThread->MacroWait.mIsWaitAll = FALSE;
                break;
            }
            // mailbox token is in an invalid slot - fall through
        
        default:
            // this is not a waitable object
            KernObj_ReleaseRef(&apCurThread->MacroWait.WaitEntry[ixEntry].ObjRef);
            K2OSKERN_Debug("%d - Thread %d\n", __LINE__, apCurThread->mGlobalIx);
            stat = K2STAT_ERROR_BAD_TOKEN;
            break;
        }
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("%d - Thread %d\n", __LINE__, apCurThread->mGlobalIx);
            break;
        }

        apCurThread->MacroWait.WaitEntry[ixEntry].mMacroIndex = ixEntry;
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(ixEntry < tokCount);
        apCurThread->mSysCall_Result = K2OS_THREAD_WAIT_FAILED_0 + ixEntry;
        if (ixEntry > 0)
        {
            do {
                --ixEntry;
                KernObj_ReleaseRef(&apCurThread->MacroWait.WaitEntry[ixEntry].ObjRef);
            } while (ixEntry > 0);
        }
        pThreadPage->mLastStatus = stat;
        return;
    }

    K2_ASSERT(ixEntry == tokCount);

    //
    // ready to go to scheduler to try the wait
    //
    apCurThread->mQuantumHfTicksRemaining = 0;
    apCurThread->SchedItem.mType = KernSchedItem_Thread_SysCall;
    KernTimer_GetHfTick(&apCurThread->SchedItem.mHfTick);
    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
    KernSched_QueueItem(&apCurThread->SchedItem);
}

