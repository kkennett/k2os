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
KernSched_Locked_NotifySysProc(
    K2OS_MSG const *apMsg
);

void    
KernSched_Locked_InsertTimerItem(
    K2OSKERN_TIMERITEM *    apTimerItem
)
{
    K2LIST_LINK *           pListLink;
    K2OSKERN_TIMERITEM *    pQueueTimerItem;

    pListLink = gData.Sched.Locked.TimerQueue.mpHead;
    do
    {
        if (NULL == pListLink)
        {
            K2LIST_AddAtTail(&gData.Sched.Locked.TimerQueue, &apTimerItem->GlobalQueueListLink);
            break;
        }

        pQueueTimerItem = K2_GET_CONTAINER(K2OSKERN_TIMERITEM, pListLink, GlobalQueueListLink);
        if (pQueueTimerItem->mHfTicks > apTimerItem->mHfTicks)
        {
            pQueueTimerItem->mHfTicks -= apTimerItem->mHfTicks;
            K2LIST_AddBefore(&gData.Sched.Locked.TimerQueue, &apTimerItem->GlobalQueueListLink, &pQueueTimerItem->GlobalQueueListLink);
            break;
        }

        apTimerItem->mHfTicks -= pQueueTimerItem->mHfTicks;

        pListLink = pListLink->mpNext;

    } while (1);

    if (NULL == apTimerItem->GlobalQueueListLink.mpPrev)
    {
        // inserted timer became first timer item on the queue
        K2_ASSERT(gData.Sched.Locked.TimerQueue.mpHead == &apTimerItem->GlobalQueueListLink);
        KernArch_SchedTimer_Arm(gData.Sched.mpSchedulingCore, &apTimerItem->mHfTicks);
    }
}

void    
KernSched_Locked_RemoveTimerItem(
    K2OSKERN_TIMERITEM *    apTimerItem
)
{
    K2LIST_LINK *           pListLink;
    K2OSKERN_TIMERITEM *    pQueueTimerItem;

    pListLink = apTimerItem->GlobalQueueListLink.mpNext;
    if (NULL != pListLink)
    {
        //
        // there is an item after the one being removed.  add ticks of item
        // being removed to the one after it
        //
        pQueueTimerItem = K2_GET_CONTAINER(K2OSKERN_TIMERITEM, pListLink, GlobalQueueListLink);
        pQueueTimerItem->mHfTicks += apTimerItem->mHfTicks;
    }
    else
    {
        pQueueTimerItem = NULL;
    }

    pListLink = apTimerItem->GlobalQueueListLink.mpPrev;

    K2LIST_Remove(&gData.Sched.Locked.TimerQueue, &apTimerItem->GlobalQueueListLink);

    if (NULL == pListLink)
    {
        //
        // we removed the first item on the timer queue
        //
        if (0 == gData.Sched.Locked.TimerQueue.mNodeCount)
        {
            // disable the scheduling timer
            KernArch_SchedTimer_Arm(gData.Sched.mpSchedulingCore, NULL);
        }
        else
        {
            K2_ASSERT(NULL != pQueueTimerItem);
            K2_ASSERT(gData.Sched.Locked.TimerQueue.mpHead == &pQueueTimerItem->GlobalQueueListLink);
            KernArch_SchedTimer_Arm(gData.Sched.mpSchedulingCore, &pQueueTimerItem->mHfTicks);
        }
    }
}

void
KernSched_Locked_MakeThreadRun(
    K2OSKERN_OBJ_THREAD *   apThread
)
{
    K2OSKERN_CPUCORE volatile * pTargetCore;
    UINT32                      affMask;
    UINT32                      ixCore;
    UINT32                      lowestAffinitizedCoreThreadCountCoreIx;
    UINT32                      lowestAffinitizedCoreThreadCount;
    UINT32                      totalThreadsRunningInSystem;
    UINT32                      coreThreadCount;
    UINT32                      lastRunThreadCount;
    UINT32                      diff;

    affMask = apThread->Config.mAffinityMask;
    K2_ASSERT(0 == (affMask & ~((1 << gData.mCpuCoreCount) - 1)));
    K2_ASSERT(0 != affMask);

    pTargetCore = apThread->mpLastRunCore;

    totalThreadsRunningInSystem = 0;
    lowestAffinitizedCoreThreadCount = (UINT32)-1;
    lowestAffinitizedCoreThreadCountCoreIx = (UINT32)-1;
    for (ixCore = 0; ixCore < gData.mCpuCoreCount; ixCore++)
    {
        coreThreadCount = gData.Sched.mCoreThreadCount[ixCore];
        if ((NULL != pTargetCore) && (ixCore == pTargetCore->mCoreIx))
           lastRunThreadCount = coreThreadCount;
        totalThreadsRunningInSystem += coreThreadCount;
        if (affMask & (1 << ixCore))
        {
            if (coreThreadCount < lowestAffinitizedCoreThreadCount)
            {
                lowestAffinitizedCoreThreadCountCoreIx = ixCore;
                lowestAffinitizedCoreThreadCount = coreThreadCount;
            }
        }
    }
    K2_ASSERT(gData.mCpuCoreCount > lowestAffinitizedCoreThreadCountCoreIx);

    if ((pTargetCore == NULL) || (totalThreadsRunningInSystem == 0))
    {
        //
        // there are no thread running in the system, or thread has never run before. 
        // just put on the core with least # of threads on it
        //
        pTargetCore = K2OSKERN_COREIX_TO_CPUCORE(lowestAffinitizedCoreThreadCountCoreIx);
    }
    else
    {
        //
        // thread has run before, and target is set to that. 
        // and there is at least one (other) thread running in the system.
        //
        if (0 == (affMask & (1 << pTargetCore->mCoreIx)))
        {
            //
            // thread is not permitted to run on last core any more. so just put it
            // on the affinitized core with the least # of threads on it
            //
            pTargetCore = K2OSKERN_COREIX_TO_CPUCORE(lowestAffinitizedCoreThreadCountCoreIx);
        }
        else
        {
            //
            // thread could run on last core it ran on. target is currently set to that
            //
            if (lastRunThreadCount != lowestAffinitizedCoreThreadCount)
            {
                K2_ASSERT(lastRunThreadCount > lowestAffinitizedCoreThreadCount);

                // 
                // figure out if last run core would be better than lowestAffinitized based on
                // thread counts relative to total # of threads in the system
                //
                diff = lastRunThreadCount - lowestAffinitizedCoreThreadCount;
                if (((diff * 100) / lastRunThreadCount) >= 10)
                {
                    //
                    // last run core has at least 10% more threads running on it
                    // so we put the thread onto the lowest core
                    //
                    pTargetCore = K2OSKERN_COREIX_TO_CPUCORE(lowestAffinitizedCoreThreadCountCoreIx);
                }
                else
                {
                    //
                    // last run core has less than 10% more threads running on it
                    // so we put the thread back onto that core
                    //
                }
            }
            else
            {
                //
                // last run core and lowest have same count of threads on them
                // so put it onto the last run core
                //
            }
        }
    }

    //
    // put this thread onto the selected core
    //
    KTRACE(gData.Sched.mpSchedulingCore, 4, KTRACE_THREAD_MIGRATE, apThread->mIsKernelThread ? 0 : apThread->User.ProcRef.AsProc->mId, apThread->mGlobalIx, pTargetCore->mCoreIx);
    KernCpu_MigrateThreadToCore(apThread, pTargetCore);

    //
    // add cpu's flag to ici send mask if its not the scheduling core
    //
    if (pTargetCore != gData.Sched.mpSchedulingCore)
        gData.Sched.Locked.mMigratedMask |= (1 << pTargetCore->mCoreIx);
}

void
KernSched_Locked_MountUnsatisfiedWait(
    K2OSKERN_MACROWAIT *    apMacroWait
)
{
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_OBJ_HEADER *   pHdr;
    K2LIST_ANCHOR *         pList;
    UINT32                ixObj;

    if (apMacroWait->mNumEntries > 0)
    {
        //
        // put entries onto the wait list of all the referenced objects
        //
        pWaitEntry = &apMacroWait->WaitEntry[0];
        for (ixObj = 0; ixObj < apMacroWait->mNumEntries; ixObj++)
        {
            K2_ASSERT(0 == pWaitEntry->mMounted);
            K2_ASSERT(pWaitEntry->mMacroIndex == ixObj);

            pHdr = pWaitEntry->ObjRef.AsAny;

            switch (pHdr->mObjType)
            {
            case KernObj_Process:
                pList = &((K2OSKERN_OBJ_PROCESS *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Thread:
                pList = &((K2OSKERN_OBJ_THREAD *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Notify:
                pList = &((K2OSKERN_OBJ_NOTIFY *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_MailboxOwner:
                pList = &((K2OSKERN_OBJ_MAILBOXOWNER *)pHdr)->RefMailbox.AsMailbox->RefGate.AsGate->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Gate:
                pList = &((K2OSKERN_OBJ_GATE *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Alarm:
                pList = &((K2OSKERN_OBJ_ALARM *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_SemUser:
                pList = &((K2OSKERN_OBJ_SEMUSER *)pHdr)->SemRef.AsSem->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Interrupt:
                pList = &((K2OSKERN_OBJ_INTERRUPT *)pHdr)->GateRef.AsGate->SchedLocked.MacroWaitEntryList;
                break;

            default:
                K2OSKERN_Panic("*** KernSched_Locked_MountUnsatisfiedWait - unknown object type (%d)\n", pHdr->mObjType);
                break;
            }

            K2LIST_AddAtTail(pList, &pWaitEntry->ObjWaitListLink);

            pWaitEntry->mMounted = 1;

            pWaitEntry++;
        }
    }

    if (apMacroWait->TimerItem.mHfTicks != K2OS_HFTIMEOUT_INFINITE)
    {
        apMacroWait->TimerItem.mIsMacroWait = TRUE;
        KernSched_Locked_InsertTimerItem(&apMacroWait->TimerItem);
        apMacroWait->mTimerActive = TRUE;
    }
}

void
KernSched_Locked_DismountWait(
    K2OSKERN_MACROWAIT *    apMacroWait,
    BOOL                    aReleaseRefs
)
{
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_OBJ_HEADER *   pHdr;
    K2LIST_ANCHOR *         pList;
    UINT32                ixObj;

    if (apMacroWait->mNumEntries > 0)
    {
        pWaitEntry = &apMacroWait->WaitEntry[0];
        for (ixObj = 0; ixObj < apMacroWait->mNumEntries; ixObj++)
        {
            K2_ASSERT(pWaitEntry->mMacroIndex == ixObj);
            K2_ASSERT(0 != pWaitEntry->mMounted);
            pHdr = pWaitEntry->ObjRef.AsAny;
            switch (pHdr->mObjType)
            {
            case KernObj_Process:
                pList = &((K2OSKERN_OBJ_PROCESS *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Thread:
                pList = &((K2OSKERN_OBJ_THREAD *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Notify:
                pList = &((K2OSKERN_OBJ_NOTIFY *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_MailboxOwner:
                pList = &((K2OSKERN_OBJ_MAILBOXOWNER *)pHdr)->RefMailbox.AsMailbox->RefGate.AsGate->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Gate:
                pList = &((K2OSKERN_OBJ_GATE *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Alarm:
                pList = &((K2OSKERN_OBJ_ALARM *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_SemUser:
                pList = &((K2OSKERN_OBJ_SEMUSER *)pHdr)->SemRef.AsSem->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Interrupt:
                pList = &((K2OSKERN_OBJ_INTERRUPT *)pHdr)->GateRef.AsGate->SchedLocked.MacroWaitEntryList;
                break;

            default:
                K2OSKERN_Panic("*** KernSched_Locked_DismountWait - unknown object type (%d)\n", pHdr->mObjType);
                break;
            }

            K2LIST_Remove(pList, &pWaitEntry->ObjWaitListLink);

            if (aReleaseRefs)
            {
                KernObj_ReleaseRef(&pWaitEntry->ObjRef);
            }

            pWaitEntry->mMounted = 0;

            pWaitEntry++;
        }
    }

    if (apMacroWait->mTimerActive)
    {
        KernSched_Locked_RemoveTimerItem(&apMacroWait->TimerItem);
        apMacroWait->mTimerActive = FALSE;
    }
}

BOOL
KernSched_Locked_TryCompleteWaitAll(
    K2OSKERN_MACROWAIT *    apMacroWait,
    UINT32                aSignallingIndex
)
{
    // if wait is completed return true
    K2OSKERN_Panic("*** KernSched_Locked_TryCompleteWaitAll - not implemented yet\n");
    return FALSE;
}

void
KernSched_Locked_MacroWaitTimeoutExpired(
    K2OSKERN_MACROWAIT *   apMacroWait
)
{
    K2OSKERN_OBJ_THREAD * pWaitingThread;

    pWaitingThread = apMacroWait->mpWaitingThread;

    K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

    KernSched_Locked_DismountWait(apMacroWait, !pWaitingThread->mIsKernelThread);

    pWaitingThread->mState = KernThreadState_InScheduler;

    if (!pWaitingThread->mIsKernelThread)
    {
        pWaitingThread->User.mSysCall_Result = FALSE;
    }
    else
    {
        pWaitingThread->Kern.mSchedCall_Result = FALSE;
    }
    pWaitingThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = K2OS_Wait_TimedOut;
    pWaitingThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_TIMEOUT;

    KernSched_Locked_MakeThreadRun(pWaitingThread);
}

void
KernSched_Locked_AlarmFired(
    K2OSKERN_OBJ_ALARM *apAlarm
)
{
    K2LIST_LINK *           pListLink;
    K2OSKERN_MACROWAIT *    pMacroWait;
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_OBJ_THREAD *   pWaitingThread;

    //
    // alarm was removed from queue
    //
    apAlarm->SchedLocked.mTimerActive = FALSE;

    //
    // signal any waiting threads
    //
    if (apAlarm->SchedLocked.MacroWaitEntryList.mNodeCount > 0)
    {
        pListLink = apAlarm->SchedLocked.MacroWaitEntryList.mpHead;
        do {
            pWaitEntry = K2_GET_CONTAINER(K2OSKERN_WAITENTRY, pListLink, ObjWaitListLink);
            pMacroWait = K2_GET_CONTAINER(K2OSKERN_MACROWAIT, pWaitEntry, WaitEntry[pWaitEntry->mMacroIndex]);
            pWaitingThread = pMacroWait->mpWaitingThread;
            pListLink = pListLink->mpNext;

            if (pMacroWait->mIsWaitAll)
            {
                KernSched_Locked_TryCompleteWaitAll(pMacroWait, pWaitEntry->mMacroIndex);
            }
            else
            {
                if (!pWaitingThread->mIsKernelThread)
                {
                    pWaitingThread->User.mSysCall_Result = TRUE;
                }
                else
                {
                    pWaitingThread->Kern.mSchedCall_Result = TRUE;
                }
                pWaitingThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = K2OS_Wait_Signalled_0 + pWaitEntry->mMacroIndex;

                K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

                KernSched_Locked_DismountWait(pMacroWait, !pWaitingThread->mIsKernelThread);

                pWaitingThread->mState = KernThreadState_InScheduler;

                KernSched_Locked_MakeThreadRun(pWaitingThread);
            }

        } while (NULL != pListLink);
    }

    if (apAlarm->SchedLocked.mIsPeriodic)
    {
        //
        // re-mount the alarm
        //
        apAlarm->SchedLocked.TimerItem.mIsMacroWait = FALSE;
        apAlarm->SchedLocked.TimerItem.mHfTicks = apAlarm->SchedLocked.mHfTicks;
        KernSched_Locked_InsertTimerItem(&apAlarm->SchedLocked.TimerItem);
        apAlarm->SchedLocked.mTimerActive = TRUE;
    }
}

void
KernSched_Locked_TimePassedUntil(
    UINT64 *apHfTick
)
{
    UINT64                  elapsed;
    K2OSKERN_TIMERITEM *    pTimerItem;
    K2OSKERN_MACROWAIT *    pMacroWait;

    //
    // only time this should happen is when two cores add a scheduler item at roughly the
    // same time, but the one of them that got the tick first ends up losing the atomic
    // war for the lockless queue during it's first iteration, which it may lose due to
    // yet another core adding at the same time
    //
    if ((*apHfTick) <= gData.Sched.Locked.mLastHfTick)
        return;

    if (0 != gData.Sched.Locked.TimerQueue.mNodeCount)
    {
        //
        // there are items in the timer queue
        //
        elapsed = (*apHfTick) - gData.Sched.Locked.mLastHfTick;

        do
        {
            pTimerItem = K2_GET_CONTAINER(K2OSKERN_TIMERITEM, gData.Sched.Locked.TimerQueue.mpHead, GlobalQueueListLink);
            if (pTimerItem->mHfTicks > elapsed)
            {
                pTimerItem->mHfTicks -= elapsed;
                break;
            }

            //
            // a timer item is firing
            //
            elapsed -= pTimerItem->mHfTicks;
            pTimerItem->mHfTicks = 0;
            KernSched_Locked_RemoveTimerItem(pTimerItem);
            if (pTimerItem->mIsMacroWait)
            {
                pMacroWait = K2_GET_CONTAINER(K2OSKERN_MACROWAIT, pTimerItem, TimerItem);
                K2_ASSERT(TRUE == pMacroWait->mTimerActive);
                pMacroWait->mTimerActive = FALSE;
                KernSched_Locked_MacroWaitTimeoutExpired(pMacroWait);
            }
            else
            {
                KernSched_Locked_AlarmFired(K2_GET_CONTAINER(K2OSKERN_OBJ_ALARM, pTimerItem, SchedLocked.TimerItem));
            }

        } while ((0 < elapsed) && (0 != gData.Sched.Locked.TimerQueue.mNodeCount));
    }

    gData.Sched.Locked.mLastHfTick = *apHfTick;
}

void
KernSched_Locked_ExitThread(
    K2OSKERN_OBJ_THREAD *   apThread,
    UINT32                  aExitCode
);

void
KernSched_Locked_AbortNotify(
    K2OSKERN_OBJ_NOTIFY *   apNotify,
    K2STAT                  aStatus,
    K2OSKERN_OBJ_PROCESS *  apProcess
)
{
    K2LIST_LINK *           pListLink;
    K2OSKERN_MACROWAIT *    pMacroWait;
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_OBJ_THREAD *   pWaitingThread;

    //
    // abandoning a notify will stop a wait containing that notify, even a wait-all
    //
    if (0 == apNotify->SchedLocked.MacroWaitEntryList.mNodeCount)
        return;

    pListLink = apNotify->SchedLocked.MacroWaitEntryList.mpHead;
    do {
        pWaitEntry = K2_GET_CONTAINER(K2OSKERN_WAITENTRY, pListLink, ObjWaitListLink);
        pMacroWait = K2_GET_CONTAINER(K2OSKERN_MACROWAIT, pWaitEntry, WaitEntry[pWaitEntry->mMacroIndex]);
        pWaitingThread = pMacroWait->mpWaitingThread;
        pListLink = pListLink->mpNext;

        if (!pWaitingThread->mIsKernelThread)
        {
            pWaitingThread->User.mSysCall_Result = FALSE;
        }
        else
        {
            pWaitingThread->Kern.mSchedCall_Result = FALSE;
        }
        pWaitingThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = K2OS_Wait_Abandoned_0 + pWaitEntry->mMacroIndex;
        pWaitingThread->mpKernRwViewOfThreadPage->mLastStatus = aStatus;

        K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

        KernSched_Locked_DismountWait(pMacroWait, !pWaitingThread->mIsKernelThread);

        pWaitingThread->mState = KernThreadState_InScheduler;

        if (!pWaitingThread->mIsKernelThread)
        {
            //
            // notify may being aborted due to process exiting.
            // do not make the thread run if the process is in its exit code
            //
            if (KernProcState_Stopping > pWaitingThread->User.ProcRef.AsProc->mState)
            {
                KernSched_Locked_MakeThreadRun(pWaitingThread);
            }
            else
            {
                K2_ASSERT(apProcess == pWaitingThread->User.ProcRef.AsProc);
                KernSched_Locked_ExitThread(pWaitingThread, pWaitingThread->User.ProcRef.AsProc->mExitCode);
            }
        }
        else
        {
            KernSched_Locked_MakeThreadRun(pWaitingThread);
        }
    } while (NULL != pListLink);
}

void
KernSched_Locked_SignalNotify(
    K2OSKERN_OBJ_NOTIFY *       apNotify
)
{
    K2LIST_LINK *           pListLink;
    K2OSKERN_MACROWAIT *    pMacroWait;
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_OBJ_THREAD *   pWaitingThread;

    if (apNotify->SchedLocked.mSignalled)
    {
        //
        // already signalled
        //
        return;
    }

    if (apNotify->SchedLocked.MacroWaitEntryList.mNodeCount == 0)
    {
        //
        // nothing to do
        //
        apNotify->SchedLocked.mSignalled = TRUE;
        return;
    }

    //
    // we *may* be releasing a thread or considering wait-all waiting threads
    // run the list and release the first releasable thread
    //
    pListLink = apNotify->SchedLocked.MacroWaitEntryList.mpHead;
    do {
        pWaitEntry = K2_GET_CONTAINER(K2OSKERN_WAITENTRY, pListLink, ObjWaitListLink);
        pMacroWait = K2_GET_CONTAINER(K2OSKERN_MACROWAIT, pWaitEntry, WaitEntry[pWaitEntry->mMacroIndex]);
        pWaitingThread = pMacroWait->mpWaitingThread;
        pListLink = pListLink->mpNext;

        if (pMacroWait->mIsWaitAll)
        {
            if (KernSched_Locked_TryCompleteWaitAll(pMacroWait, pWaitEntry->mMacroIndex))
                return;
        }
        else
        {
            if (!pWaitingThread->mIsKernelThread)
            {
                pWaitingThread->User.mSysCall_Result = TRUE;
            }
            else
            {
                pWaitingThread->Kern.mSchedCall_Result = TRUE;
            }
            pWaitingThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = K2OS_Wait_Signalled_0 + pWaitEntry->mMacroIndex;

            K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

            KernSched_Locked_DismountWait(pMacroWait, !pWaitingThread->mIsKernelThread);

            pWaitingThread->mState = KernThreadState_InScheduler;

            KernSched_Locked_MakeThreadRun(pWaitingThread);

            return;
        }

    } while (NULL != pListLink);

    //
    // if we get here we were not able to release a thread and the notify was still signalled
    //
    apNotify->SchedLocked.mSignalled = TRUE;
}

void
KernSched_Locked_AbortGate(
    K2OSKERN_OBJ_GATE *     apGate,
    K2STAT                  aStatus,
    K2OSKERN_OBJ_PROCESS *  apProcess
)
{
    K2LIST_LINK *           pListLink;
    K2OSKERN_MACROWAIT *    pMacroWait;
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_OBJ_THREAD *   pWaitingThread;

    //
    // abandoning a notify will stop a wait containing that notify, even a wait-all
    //
    if (0 == apGate->SchedLocked.MacroWaitEntryList.mNodeCount)
        return;

    pListLink = apGate->SchedLocked.MacroWaitEntryList.mpHead;
    do {
        pWaitEntry = K2_GET_CONTAINER(K2OSKERN_WAITENTRY, pListLink, ObjWaitListLink);
        pMacroWait = K2_GET_CONTAINER(K2OSKERN_MACROWAIT, pWaitEntry, WaitEntry[pWaitEntry->mMacroIndex]);
        pWaitingThread = pMacroWait->mpWaitingThread;
        pListLink = pListLink->mpNext;

        if (!pWaitingThread->mIsKernelThread)
        {
            pWaitingThread->User.mSysCall_Result = FALSE;
        }
        else
        {
            pWaitingThread->Kern.mSchedCall_Result = FALSE;
        }
        pWaitingThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = K2OS_Wait_Abandoned_0 + pWaitEntry->mMacroIndex;
        pWaitingThread->mpKernRwViewOfThreadPage->mLastStatus = aStatus;

        K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

        KernSched_Locked_DismountWait(pMacroWait, !pWaitingThread->mIsKernelThread);

        pWaitingThread->mState = KernThreadState_InScheduler;

        if (!pWaitingThread->mIsKernelThread)
        {
            //
            // notify may being aborted due to process exiting.
            // do not make the thread run if the process is in its exit code
            //
            if (KernProcState_Stopping > pWaitingThread->User.ProcRef.AsProc->mState)
            {
                KernSched_Locked_MakeThreadRun(pWaitingThread);
            }
            else
            {
                K2_ASSERT(apProcess == pWaitingThread->User.ProcRef.AsProc);
                KernSched_Locked_ExitThread(pWaitingThread, pWaitingThread->User.ProcRef.AsProc->mExitCode);
            }
        }
        else
        {
            KernSched_Locked_MakeThreadRun(pWaitingThread);
        }
    } while (NULL != pListLink);
}

void
KernSched_Locked_GateChange(
    K2OSKERN_OBJ_GATE *         apGate,
    BOOL                        aSetTo
)
{
    K2LIST_LINK *           pListLink;
    K2OSKERN_MACROWAIT *    pMacroWait;
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_OBJ_THREAD *   pWaitingThread;

    if (apGate->SchedLocked.mOpen == aSetTo)
    {
        //
        // already in requested state
        //
        return;
    }

    //
    // gate is changing
    //

    if ((apGate->SchedLocked.MacroWaitEntryList.mNodeCount == 0) ||
        (FALSE == aSetTo))
    {
        //
        // nobody is waiting on this gate, or it is being closed
        //
        apGate->SchedLocked.mOpen = aSetTo;
        return;
    }

    //
    // gate becomes open
    //
    apGate->SchedLocked.mOpen = TRUE;

    //
    // we *may* be releasing a thread or considering wait-all waiting threads
    // run the list and release any releasable thread
    //
    pListLink = apGate->SchedLocked.MacroWaitEntryList.mpHead;
    do {
        pWaitEntry = K2_GET_CONTAINER(K2OSKERN_WAITENTRY, pListLink, ObjWaitListLink);
        pMacroWait = K2_GET_CONTAINER(K2OSKERN_MACROWAIT, pWaitEntry, WaitEntry[pWaitEntry->mMacroIndex]);
        pWaitingThread = pMacroWait->mpWaitingThread;
        pListLink = pListLink->mpNext;

        if (pMacroWait->mIsWaitAll)
        {
            KernSched_Locked_TryCompleteWaitAll(pMacroWait, pWaitEntry->mMacroIndex);
        }
        else
        {
            if (!pWaitingThread->mIsKernelThread)
            {
                pWaitingThread->User.mSysCall_Result = TRUE;
            }
            else
            {
                pWaitingThread->Kern.mSchedCall_Result = TRUE;
            }
            pWaitingThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = K2OS_Wait_Signalled_0 + pWaitEntry->mMacroIndex;

            K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

            KernSched_Locked_DismountWait(pMacroWait, !pWaitingThread->mIsKernelThread);

            pWaitingThread->mState = KernThreadState_InScheduler;

            KernSched_Locked_MakeThreadRun(pWaitingThread);
        }

    } while (NULL != pListLink);
}

void
KernSched_Locked_Mailbox_SentFirst(
    K2OSKERN_OBJ_MAILBOX *  apMailbox
);

void
KernSched_Locked_Mailbox_DeliverFromReserve(
    K2OSKERN_OBJ_MAILBOX *  apMailbox,
    K2OS_MSG const *        apMsg,
    K2OSKERN_OBJ_HEADER *   apReserve
)
{
    BOOL doSignal;

    KernMailbox_Deliver(apMailbox, apMsg, FALSE, apReserve, &doSignal);
    if (doSignal)
    {
        KernSched_Locked_Mailbox_SentFirst(apMailbox);
    }
}

BOOL
KernSched_Locked_IpcEnd_Disconnect(
    K2OSKERN_OBJ_IPCEND *apLocal
)
{
    BOOL                    disp;
    BOOL                    disconnected;
    K2OSKERN_OBJ_IPCEND *   pRemote;
    K2OS_MSG                disconnectMsg;
    K2OSKERN_OBJ_PROCESS *  pProc;

    K2MEM_Zero(&disconnectMsg, sizeof(disconnectMsg));
    disconnectMsg.mType = K2OS_SYSTEM_MSGTYPE_IPCEND;
    disconnectMsg.mShort = K2OS_SYSTEM_MSG_IPCEND_SHORT_DISCONNECTED;

    disp = K2OSKERN_SeqLock(&apLocal->Connection.SeqLock);

    if (apLocal->Connection.Locked.mState == KernIpcEndState_Connected)
    {
        disconnected = TRUE;

        pRemote = apLocal->Connection.Locked.Connected.PartnerIpcEndRef.AsIpcEnd;

        K2OSKERN_SeqLock(&pRemote->Connection.SeqLock);

        apLocal->Connection.Locked.mState = KernIpcEndState_WaitDisAck;
        pProc = apLocal->MailboxOwnerRef.AsMailboxOwner->RefProc.AsProc;
        if ((NULL == pProc) || (pProc->mState < KernProcState_Stopping))
        {
            disconnectMsg.mPayload[0] = (UINT32)apLocal->mpUserKey;
            KernSched_Locked_Mailbox_DeliverFromReserve(
                apLocal->MailboxOwnerRef.AsMailboxOwner->RefMailbox.AsMailbox, 
                &disconnectMsg,
                &apLocal->Hdr);
        }
        pProc = pRemote->MailboxOwnerRef.AsMailboxOwner->RefProc.AsProc;
        if ((NULL == pProc) || (pProc->mState < KernProcState_Stopping))
        {
            pRemote->Connection.Locked.mState = KernIpcEndState_WaitDisAck;
            disconnectMsg.mPayload[0] = (UINT32)pRemote->mpUserKey;
            KernSched_Locked_Mailbox_DeliverFromReserve(
                pRemote->MailboxOwnerRef.AsMailboxOwner->RefMailbox.AsMailbox,
                &disconnectMsg,
                &pRemote->Hdr);
        }
        K2OSKERN_SeqUnlock(&pRemote->Connection.SeqLock, FALSE);
    }
    else
    {
        disconnected = FALSE;
    }

    K2OSKERN_SeqUnlock(&apLocal->Connection.SeqLock, disp);
    
    return disconnected;
}

void
KernSched_Locked_LastToken_Destroyed(
    K2OSKERN_OBJREF *       apObjRef,
    K2OSKERN_OBJ_PROCESS *  apProc
);

BOOL
KernSched_Locked_ProcTokenLocked_Token_Destroy(
    K2OSKERN_OBJ_PROCESS *  apProc, 
    K2OS_TOKEN              aToken
)
{
    K2STAT          stat;
    BOOL            hitZero;
    K2OSKERN_OBJREF objRef;

    hitZero = FALSE;
    objRef.AsAny = NULL;
    stat = KernProc_TokenLocked_Destroy(apProc, aToken, &hitZero, &objRef);
    if (K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(0);
        return FALSE;
    }

    if (hitZero)
    {
        K2_ASSERT(NULL != objRef.AsAny);
        if (objRef.AsAny->RefObjList.mNodeCount)
        {
            //
            // there are references left even though there are no tokens
            // so if there are threads not from this process that are waiting
            // on this object then those waits need to be cancelled
            // (this is the other-process-thread-waits-and-another-thread-from-that-process-async-deletes-token case)
            //
            switch (objRef.AsAny->mObjType)
            {
                // for this one its because we need to send out messages that the public ifinst departed
            case KernObj_IfInst:
                if (objRef.AsIfInst->mIsPublic)
                {
                    objRef.AsIfInst->mIsDeparting = TRUE;
                }
                else
                {
                    KernObj_ReleaseRef(&objRef);
                    break;
                }
                // fall through
            case KernObj_Notify:
            case KernObj_Gate:
            case KernObj_MailboxOwner:
                KernSched_Locked_LastToken_Destroyed(&objRef, apProc);
                break;

            default:
                // not a waitable object
                KernObj_ReleaseRef(&objRef);
                break;
            };
        }
        else
        {
            KernObj_ReleaseRef(&objRef);
        }
    }

    K2_ASSERT(NULL == objRef.AsAny);

    return TRUE;
}

void
KernSched_Locked_AllProcThreadsExitedAndDpcRan(
    K2OSKERN_OBJ_PROCESS *  apProc
)
{
    K2LIST_LINK *           pListLink;
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_MACROWAIT *    pMacroWait;
    BOOL                    disp;
    BOOL                    ok;
    UINT32                  ixPage;
    K2OSKERN_TOKEN_PAGE *   pTokenPage;
    K2OSKERN_TOKEN *        pToken;
    UINT32                  ixToken;
    UINT32                  tokValue;
    K2OSKERN_OBJ_IPCEND *   pIpcEnd;
    K2OSKERN_OBJREF         refIpcEnd;
    K2OS_MSG                sysProcMsg;

    K2_ASSERT(apProc->mState == KernProcState_Stopping);
    K2_ASSERT(apProc->Thread.SchedLocked.ActiveList.mNodeCount == 0);
    K2_ASSERT(apProc->mStopDpcRan);
    apProc->mState = KernProcState_Stopped;
    K2_CpuWriteBarrier();
    KTRACE(gData.Sched.mpSchedulingCore, 2, KTRACE_PROC_STOPPED, apProc->mId);

#if K2OSKERN_TRACE_THREAD_LIFE
    K2OSKERN_Debug("-- Proc %d exited with exit code %08X\n", apProc->mId, apProc->mExitCode);
#endif

    //
    // signal all waiting objects
    //
    if (apProc->SchedLocked.MacroWaitEntryList.mNodeCount > 0)
    {
        //
        // signal anybody waiting
        //
        pListLink = apProc->SchedLocked.MacroWaitEntryList.mpHead;
        do {
            pWaitEntry = K2_GET_CONTAINER(K2OSKERN_WAITENTRY, pListLink, ObjWaitListLink);
            pMacroWait = K2_GET_CONTAINER(K2OSKERN_MACROWAIT, pWaitEntry, WaitEntry[pWaitEntry->mMacroIndex]);
            pThread = pMacroWait->mpWaitingThread;
            pListLink = pListLink->mpNext;

            if (pMacroWait->mIsWaitAll)
            {
                KernSched_Locked_TryCompleteWaitAll(pMacroWait, pWaitEntry->mMacroIndex);
            }
            else
            {
                if (!pThread->mIsKernelThread)
                {
                    pThread->User.mSysCall_Result = TRUE;
                }
                pThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = K2OS_Wait_Signalled_0 + pWaitEntry->mMacroIndex;

                K2_ASSERT(KernThreadState_Waiting == pThread->mState);

                KernSched_Locked_DismountWait(pMacroWait, !pThread->mIsKernelThread);

                pThread->mState = KernThreadState_InScheduler;

                //
                // theads cannot wait on their own process
                //
                K2_ASSERT((pThread->mIsKernelThread) || (pThread->User.ProcRef.AsProc != apProc));

                KernSched_Locked_MakeThreadRun(pThread);
            }
        } while (pListLink != NULL);
    }

    sysProcMsg.mType = K2OS_SYSTEM_MSGTYPE_SYSPROC;
    sysProcMsg.mShort = K2OS_SYSTEM_MSG_SYSPROC_SHORT_PROCESS_STOPPED;
    sysProcMsg.mPayload[0] = apProc->mId;
    sysProcMsg.mPayload[1] = apProc->mExitCode;
    KernSched_Locked_NotifySysProc(&sysProcMsg);

    //
    // manually disconnect all ipc endpoints here
    // as some may be connected to the kernel and must happen
    // independently of token close because otherwise it
    // will remain connected even though one side is gone.
    //
    refIpcEnd.AsAny = NULL;

    do {
        disp = K2OSKERN_SeqLock(&apProc->IpcEnd.SeqLock);

        pListLink = apProc->IpcEnd.Locked.List.mpHead;

        if (NULL != pListLink)
        {
            do {
                pIpcEnd = K2_GET_CONTAINER(K2OSKERN_OBJ_IPCEND, pListLink, OwnerIpcEndListLocked.ListLink);

                K2OSKERN_SeqLock(&pIpcEnd->Connection.SeqLock);

                if (pIpcEnd->Connection.Locked.mState == KernIpcEndState_Connected)
                {
                    //
                    // create ref so that it doesnt disappear between here
                    // and when we try to disconnect it outside the locks
                    // below
                    //
                    KernObj_CreateRef(&refIpcEnd, &pIpcEnd->Hdr);
                }

                K2OSKERN_SeqUnlock(&pIpcEnd->Connection.SeqLock, FALSE);

                if (NULL != refIpcEnd.AsAny)
                    break;

                pListLink = pListLink->mpNext;

            } while (NULL != pListLink);
        }

        K2OSKERN_SeqUnlock(&apProc->IpcEnd.SeqLock, disp);

        if (NULL != refIpcEnd.AsAny)
        {
            KernSched_Locked_IpcEnd_Disconnect(refIpcEnd.AsIpcEnd);
            KernObj_ReleaseRef(&refIpcEnd);
        }

    } while (NULL != pListLink);

    //
    // destroy tokens here - doing so may cause waiting threads in other
    // processes to have their waits abandoned (mailbox case).  
    // token creation should fail as soon as we set the process' state
    // to stopping (done above).  so once we're inside this lock
    // anybody who was creating a token is out of it, and anybody else
    // trying to get in will see the state is stopped and fail.
    // then that's all the tokens there will ever be for the process
    // 
    // this takes care of closing mailboxes (owner token destroyed)
    // this takes care of published interfaces (last token destroyed)
    //
    disp = K2OSKERN_SeqLock(&apProc->Token.SeqLock);
    if (apProc->Token.Locked.mCount > 0)
    {
        for (ixPage = 0; ixPage < apProc->Token.Locked.mPageCount; ixPage++)
        {
            pTokenPage = apProc->Token.Locked.mppPages[ixPage];
            pToken = &pTokenPage->Tokens[0];
            pToken++;
            ixToken = 1;
            do {
                tokValue = pToken->InUse.mTokValue;
                if (tokValue & K2OSKERN_TOKEN_VALID_BIT)
                {
                    ok = KernSched_Locked_ProcTokenLocked_Token_Destroy(apProc, (K2OS_TOKEN)tokValue);
                    K2_ASSERT(ok);
                    if (0 == apProc->Token.Locked.mCount)
                        break;
                }
                pToken++;
            } while (++ixToken < K2OSKERN_TOKENS_PER_PAGE);
        }
        K2_ASSERT(0 == apProc->Token.Locked.mCount);
    }
    K2OSKERN_SeqUnlock(&apProc->Token.SeqLock, disp);

    apProc->Hdr.ObjDpc.Func = KernProc_ExitedDpc;
    KernCpu_QueueDpc(&apProc->Hdr.ObjDpc.Dpc, &apProc->Hdr.ObjDpc.Func, KernDpcPrio_Med);
}

void
KernSched_Locked_ProcStopped(
    K2OSKERN_SCHED_ITEM *   apItem
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;

    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apItem, StoppedSchedItem);
    
    K2_ASSERT(pProc->mState == KernProcState_Stopping);

    pProc->mStopDpcRan = TRUE;

    if (pProc->Thread.SchedLocked.ActiveList.mNodeCount > 0)
        return;

    KernSched_Locked_AllProcThreadsExitedAndDpcRan(pProc);
}

void
KernSched_Locked_StopProcess(
    K2OSKERN_OBJ_PROCESS *  apProc,
    UINT32                  aExitCode
)
{
    K2LIST_LINK *           pListLink;
    K2OSKERN_OBJ_THREAD *   pThread;

    if (apProc->mState >= KernProcState_Stopping)
    {
        return;
    }

    apProc->mExitCode = aExitCode;
    apProc->mState = KernProcState_Stopping;
    KernObj_CreateRef(&apProc->StoppingRef, &apProc->Hdr);    // released in ExitedDpc
    K2_CpuWriteBarrier();
    KTRACE(gData.Sched.mpSchedulingCore, 3, KTRACE_PROC_BEGIN_STOP, apProc->mId, aExitCode);
#if K2OSKERN_TRACE_THREAD_LIFE
    K2OSKERN_Debug("-- Proc %d begin exit\n", apProc->mId);
#endif

    //
    // any threads pending resume need to be exited
    //
    pListLink = gData.Sched.Locked.DefferedResumeList.mpHead;
    if (NULL != pListLink)
    {
        do
        {
            pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListLink, SchedListLink);
            K2_ASSERT(KernThreadState_InScheduler_ResumeDeferred == pThread->mState);
            pListLink = pListLink->mpNext;
            if ((!pThread->mIsKernelThread) && (pThread->User.ProcRef.AsProc == apProc))
            {
                K2LIST_Remove(&gData.Sched.Locked.DefferedResumeList, &pThread->SchedListLink);
                KernSched_Locked_ExitThread(pThread, aExitCode);
            }
        } while (NULL != pListLink);
    }

    // 
    // any active threads from this process waiting need to have those waits aborted
    // and the threads exited.  
    //
    pListLink = apProc->Thread.SchedLocked.ActiveList.mpHead;
    if (NULL != pListLink)
    {
        do
        {
            pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListLink, OwnerThreadListLink);
            pListLink = pListLink->mpNext;

            if (pThread->mState == KernThreadState_Waiting)
            {
                //
                // abort the thread wait and remove macrowait from lists it is on
                //
                KernSched_Locked_DismountWait(&pThread->MacroWait, TRUE);
                pThread->mState = KernThreadState_InScheduler;

                //
                // abort the thread
                //
                KernSched_Locked_ExitThread(pThread, aExitCode);
            }
        } while (NULL != pListLink);
    }

    //
    // now queue the stop dpc for the process.  this will tell any other cores to get
    // off the process and abort any threads that are running.
    // 
    // WE NEED TO DO THIS ONLY IF THERE WAS ONLY ONE THREAD IN THE PROCESS SINCE
    // WE STILL NEED TO GET THE PROCESS OFF THE CURRENT CORE
    //
    apProc->Hdr.ObjDpc.Func = KernProc_StopDpc;
    KernCpu_QueueDpc(&apProc->Hdr.ObjDpc.Dpc, &apProc->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
}

void
KernSched_Locked_ExitThread(
    K2OSKERN_OBJ_THREAD *   apThread,
    UINT32                  aExitCode
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    BOOL                    disp;
    K2LIST_LINK *           pListLink;
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_MACROWAIT *    pMacroWait;
    K2OSKERN_OBJ_THREAD *   pWaitingThread;
    K2OS_MSG                sysProcMsg;

    K2_ASSERT(KernThreadState_Exited != apThread->mState);

    apThread->mState = KernThreadState_Exited;
    apThread->mExitCode = aExitCode;
    K2_CpuWriteBarrier();
    KTRACE(gData.Sched.mpSchedulingCore, 4, KTRACE_THREAD_EXIT, apThread->mIsKernelThread ? 0 : apThread->User.ProcRef.AsProc->mId, apThread->mGlobalIx, aExitCode);
#if K2OSKERN_TRACE_THREAD_LIFE
    K2OSKERN_Debug("-- Proc %d Thread %d exited with code %08X\n", apThread->mIsKernelThread ? 0 : apThread->User.ProcRef.AsProc->mId, apThread->mGlobalIx, aExitCode);
#endif

    if (apThread->SchedLocked.MacroWaitEntryList.mNodeCount > 0)
    {
        //
        // signal anybody waiting on this thread
        //
        pListLink = apThread->SchedLocked.MacroWaitEntryList.mpHead;
        do {
            pWaitEntry = K2_GET_CONTAINER(K2OSKERN_WAITENTRY, pListLink, ObjWaitListLink);
            pMacroWait = K2_GET_CONTAINER(K2OSKERN_MACROWAIT, pWaitEntry, WaitEntry[pWaitEntry->mMacroIndex]);
            pWaitingThread = pMacroWait->mpWaitingThread;
            pListLink = pListLink->mpNext;

            if (pMacroWait->mIsWaitAll)
            {
                KernSched_Locked_TryCompleteWaitAll(pMacroWait, pWaitEntry->mMacroIndex);
            }
            else
            {
                if (!pWaitingThread->mIsKernelThread)
                {
                    pWaitingThread->User.mSysCall_Result = TRUE;
                }
                pWaitingThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = K2OS_Wait_Signalled_0 + pWaitEntry->mMacroIndex;

                K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

                KernSched_Locked_DismountWait(pMacroWait, !pWaitingThread->mIsKernelThread);

                pWaitingThread->mState = KernThreadState_InScheduler;

                //
                // theads cannot wait on themselves
                //
                K2_ASSERT(pWaitingThread != apThread);
                KernSched_Locked_MakeThreadRun(pWaitingThread);
            }
        } while (pListLink != NULL);
    }

    if (apThread->mIsKernelThread)
    {
        disp = K2OSKERN_SeqLock(&gData.Thread.ListSeqLock);
        K2LIST_Remove(&gData.Thread.ActiveList, &apThread->OwnerThreadListLink);
        K2LIST_AddAtTail(&gData.Thread.DeadList, &apThread->OwnerThreadListLink);
        K2OSKERN_SeqUnlock(&gData.Thread.ListSeqLock, disp);
    }
    else
    {
        pProc = apThread->User.ProcRef.AsProc;
        disp = K2OSKERN_SeqLock(&pProc->Thread.SeqLock);
        K2LIST_Remove(&pProc->Thread.SchedLocked.ActiveList, &apThread->OwnerThreadListLink);
        K2LIST_AddAtTail(&pProc->Thread.Locked.DeadList, &apThread->OwnerThreadListLink);
        K2OSKERN_SeqUnlock(&pProc->Thread.SeqLock, disp);
    }

    KernObj_ReleaseRef(&apThread->Running_SelfRef);

    K2ATOMIC_Dec(&gData.mSystemWideThreadCount);

    sysProcMsg.mType = K2OS_SYSTEM_MSGTYPE_SYSPROC;
    sysProcMsg.mShort = K2OS_SYSTEM_MSG_SYSPROC_SHORT_THREAD_EXITED;
    sysProcMsg.mPayload[0] = pProc->mId;
    sysProcMsg.mPayload[1] = apThread->mGlobalIx;
    sysProcMsg.mPayload[2] = apThread->mExitCode;
    KernSched_Locked_NotifySysProc(&sysProcMsg);

    if ((apThread->mIsKernelThread) || 
        (pProc->Thread.SchedLocked.ActiveList.mNodeCount > 0))
    {
        //
        // is a kernel thread, or there still more threads running in the thread's process
        //
        return;
    }

    //
    // last thread has exited
    //

    if (pProc->mStopDpcRan)
    {
        KernSched_Locked_AllProcThreadsExitedAndDpcRan(pProc);
        return;
    }

    KernSched_Locked_StopProcess(pProc, apThread->mExitCode);
}

void 
KernSched_Locked_Thread_SysCall_SignalNotify(
    K2OSKERN_OBJ_THREAD *apCallerThread
)
{
    K2OSKERN_OBJ_NOTIFY *   pNotify;

    pNotify = apCallerThread->SchedItem.ObjRef.AsNotify;
    K2_ASSERT(NULL != pNotify);
    // not possible for this call to fail
    KernSched_Locked_SignalNotify(pNotify);
    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
}

void 
KernSched_Locked_Thread_SysCall_ChangeGate(
    K2OSKERN_OBJ_THREAD *apCallerThread
)
{
    K2OSKERN_OBJ_GATE *   pGate;
    UINT32                newState;

    pGate = apCallerThread->SchedItem.ObjRef.AsGate;
    K2_ASSERT(NULL != pGate);
    // not possible for this call to fail
    newState = apCallerThread->SchedItem.Args.Gate_Change.mNewState;
    if (newState <= 2)
    {
        // 2 is pulse
        if (newState != 0)
        {
            KernSched_Locked_GateChange(pGate, TRUE);
        }
        if (newState != 1)
        {
            KernSched_Locked_GateChange(pGate, FALSE);
        }
    }
    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
}

void
KernSched_Locked_Thread_SysCall_ProcessExit(
    K2OSKERN_OBJ_THREAD *apCallerThread
)
{
    KernSched_Locked_ExitThread(apCallerThread, apCallerThread->User.mSysCall_Arg0);

    KernSched_Locked_StopProcess(apCallerThread->User.ProcRef.AsProc, apCallerThread->User.mSysCall_Arg0);
}

void 
KernSched_Locked_Thread_SysCall_ThreadExit(
    K2OSKERN_OBJ_THREAD *apCallerThread
)
{
    KernSched_Locked_ExitThread(apCallerThread, apCallerThread->User.mSysCall_Arg0);
}

void 
KernSched_Locked_Thread_SysCall_ThreadCreate(
    K2OSKERN_OBJ_THREAD *apCallerThread
)
{
    K2OSKERN_OBJ_PROCESS *  pCallerProc;
    K2OSKERN_OBJ_THREAD *   pNewThread;
    BOOL                    disp;
    K2OS_MSG                sysProcMsg;

    pNewThread = apCallerThread->SchedItem.ObjRef.AsThread;
    K2_ASSERT(NULL != pNewThread);
    K2_ASSERT(pNewThread->mState == KernThreadState_Created);
    
    pCallerProc = apCallerThread->User.ProcRef.AsProc;

    disp = K2OSKERN_SeqLock(&pCallerProc->Thread.SeqLock);
    K2LIST_Remove(&pCallerProc->Thread.Locked.CreateList, &pNewThread->OwnerThreadListLink);
    K2LIST_AddAtTail(&pCallerProc->Thread.SchedLocked.ActiveList, &pNewThread->OwnerThreadListLink);
    K2OSKERN_SeqUnlock(&pCallerProc->Thread.SeqLock, disp);

    //
    // first resume caller thread so it gets first crack at the core it just
    // was running on
    //
    apCallerThread->User.mSysCall_Result = apCallerThread->SchedItem.Args.Thread_Create.mThreadToken;
    apCallerThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = pNewThread->mGlobalIx;
    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
    KernSched_Locked_MakeThreadRun(apCallerThread);

    //
    // now resume new thread
    //
    KTRACE(gData.Sched.mpSchedulingCore, 3, KTRACE_THREAD_START, pCallerProc->mId, pNewThread->mGlobalIx);
#if K2OSKERN_TRACE_THREAD_LIFE
    K2OSKERN_Debug("-- Proc %d Thread %d Start\n", pCallerProc->mId, pNewThread->mGlobalIx);
#endif
    pNewThread->mState = KernThreadState_InScheduler;
    KernObj_CreateRef(&pNewThread->Running_SelfRef, &pNewThread->Hdr);
    
    K2ATOMIC_Inc(&gData.mSystemWideThreadCount);
    sysProcMsg.mType = K2OS_SYSTEM_MSGTYPE_SYSPROC;
    sysProcMsg.mShort = K2OS_SYSTEM_MSG_SYSPROC_SHORT_THREAD_CREATED;
    sysProcMsg.mPayload[0] = pCallerProc->mId;
    sysProcMsg.mPayload[1] = pNewThread->mGlobalIx;
    sysProcMsg.mPayload[2] = apCallerThread->mGlobalIx; // thread that started it
    KernSched_Locked_NotifySysProc(&sysProcMsg);

    KernSched_Locked_MakeThreadRun(pNewThread);
}

void 
KernSched_Locked_Thread_SysCall_ProcessCreate(
    K2OSKERN_OBJ_THREAD *   apCallerThread,
    K2OSKERN_OBJ_PROCESS *  apNewProc,
    K2OSKERN_OBJ_THREAD *   apNewThread
)
{
    BOOL        disp;
    K2OS_MSG    sysProcMsg;

    K2_ASSERT(apNewThread->mState == KernThreadState_Created);
    K2_ASSERT(apNewProc->mState = KernProcState_Launching);

    apNewProc->mState = KernProcState_Starting;

    disp = K2OSKERN_SeqLock(&apNewProc->Thread.SeqLock);
    K2LIST_Remove(&apNewProc->Thread.Locked.CreateList, &apNewThread->OwnerThreadListLink);
    K2LIST_AddAtTail(&apNewProc->Thread.SchedLocked.ActiveList, &apNewThread->OwnerThreadListLink);
    K2OSKERN_SeqUnlock(&apNewProc->Thread.SeqLock, disp);

    //
    // first resume caller thread so it gets first crack at the core it just
    // was running on
    //
    apCallerThread->User.mSysCall_Result = apCallerThread->SchedItem.Args.Process_Create.mProcessToken;
    apCallerThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = apNewProc->mId;
    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
    KernSched_Locked_MakeThreadRun(apCallerThread);

    //
    // now resume new thread
    //
    KTRACE(gData.Sched.mpSchedulingCore, 3, KTRACE_PROC_START, apNewThread->User.ProcRef.AsProc->mId, apNewThread->mGlobalIx);
#if K2OSKERN_TRACE_THREAD_LIFE
    K2OSKERN_Debug("-- Proc %d Starting, Thread %d\n", apNewThread->User.ProcRef.AsProc->mId, apNewThread->mGlobalIx);
#endif
    KernObj_CreateRef(&apNewThread->Running_SelfRef, &apNewThread->Hdr);
    apNewThread->mState = KernThreadState_InScheduler;

    K2ATOMIC_Inc(&gData.mSystemWideProcessCount);
    sysProcMsg.mType = K2OS_SYSTEM_MSGTYPE_SYSPROC;
    sysProcMsg.mShort = K2OS_SYSTEM_MSG_SYSPROC_SHORT_PROCESS_CREATED;
    sysProcMsg.mPayload[0] = apNewProc->mId;
    sysProcMsg.mPayload[1] = apCallerThread->User.ProcRef.AsProc->mId;  // process that started it
    sysProcMsg.mPayload[2] = apCallerThread->mGlobalIx;                 // thread that started it
    KernSched_Locked_NotifySysProc(&sysProcMsg);

    K2ATOMIC_Inc(&gData.mSystemWideThreadCount);
    sysProcMsg.mShort = K2OS_SYSTEM_MSG_SYSPROC_SHORT_THREAD_CREATED;
    sysProcMsg.mPayload[0] = apNewProc->mId;
    sysProcMsg.mPayload[1] = apNewThread->mGlobalIx;                    // process that started it
    sysProcMsg.mPayload[2] = apCallerThread->mGlobalIx;                 // thread that started it
    KernSched_Locked_NotifySysProc(&sysProcMsg);

    KernSched_Locked_MakeThreadRun(apNewThread);
}

void
KernSched_Locked_Thread_SysCall_SetAffinity(
    K2OSKERN_OBJ_THREAD *   apCallerThread,
    UINT32                  aNewMask
)
{
    apCallerThread->Config.mAffinityMask = (UINT8)(aNewMask & 0xFF);
}

void
KernSched_Locked_Release_Wait_References(
    K2OSKERN_MACROWAIT *    apMacroWait
)
{
    UINT32                ixObj;
    K2OSKERN_WAITENTRY *    pWaitEntry;

    pWaitEntry = &apMacroWait->WaitEntry[0];
    for (ixObj = 0; ixObj < apMacroWait->mNumEntries; ixObj++)
    {
        KernObj_ReleaseRef(&pWaitEntry->ObjRef);
        pWaitEntry++;
    }
}

void
KernSched_Locked_NotifyReleasedThread(
    K2OSKERN_OBJ_NOTIFY * apNotify
)
{
    K2_ASSERT(apNotify->SchedLocked.mSignalled);
    apNotify->SchedLocked.mSignalled = FALSE;
}

void
KernSched_Locked_SemReleasedThread(
    K2OSKERN_OBJ_SEM *  apSem
)
{
    K2_ASSERT(apSem->SchedLocked.mCount > 0);
    apSem->SchedLocked.mCount--;
}

void
KernSched_Locked_EarlySatisfyWaitEntry(
    K2OSKERN_OBJ_THREAD *   apThread,
    K2OSKERN_WAITENTRY *    apWaitEntry
)
{
    K2OSKERN_OBJ_HEADER *pHdr;

    pHdr = apWaitEntry->ObjRef.AsAny;

    switch (pHdr->mObjType)
    {
    case KernObj_Process:
    case KernObj_Thread:
    case KernObj_Gate:
    case KernObj_Interrupt:
    case KernObj_MailboxOwner:
        break;

    case KernObj_Notify:
        KernSched_Locked_NotifyReleasedThread((K2OSKERN_OBJ_NOTIFY *)pHdr);
        break;

    case KernObj_SemUser:
        KernSched_Locked_SemReleasedThread(((K2OSKERN_OBJ_SEMUSER *)pHdr)->SemRef.AsSem);
        ((K2OSKERN_OBJ_SEMUSER *)pHdr)->SchedLocked.mHeldCount++;
        break;

    default:
        K2OSKERN_Panic("*** KernSched_Locked_EarlySatisfyWaitEntry - unknown object type (%d)\n", pHdr->mObjType);
        break;
    }
}

BOOL
KernSched_Locked_Thread_Wait(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OSKERN_MACROWAIT *    pMacroWait;
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_OBJ_HEADER *   pHdr;
    UINT32                  ixObj;
    UINT32                  countSatisfied;
    BOOL                    waitSatisfied;
    BOOL                    waitFailed;

    pMacroWait = &apCallerThread->MacroWait;
    pMacroWait->mpWaitingThread = apCallerThread;

    waitSatisfied = FALSE;

    if (pMacroWait->mNumEntries > 0)
    {
        //
        // check referenced objects to see if already satisfied
        // 
        waitFailed = FALSE;
        countSatisfied = 0;
        pWaitEntry = &pMacroWait->WaitEntry[0];
        for (ixObj = 0; ixObj < pMacroWait->mNumEntries; ixObj++)
        {
            pHdr = pWaitEntry->ObjRef.AsAny;

            switch (pHdr->mObjType)
            {
            case KernObj_Process:
                if (KernProcState_Stopped == ((K2OSKERN_OBJ_PROCESS *)pHdr)->mState)
                {
                    if (!pMacroWait->mIsWaitAll)
                        waitSatisfied = TRUE;
                    countSatisfied++;
                }
                break;

            case KernObj_Thread:
                if (KernThreadState_Exited == ((K2OSKERN_OBJ_THREAD *)pHdr)->mState)
                {
                    if (!pMacroWait->mIsWaitAll)
                        waitSatisfied = TRUE;
                    countSatisfied++;
                }
                break;

            case KernObj_Notify:
                if (((K2OSKERN_OBJ_NOTIFY *)pHdr)->SchedLocked.mSignalled)
                {
                    if (!pMacroWait->mIsWaitAll)
                        waitSatisfied = TRUE;
                    countSatisfied++;
                }
                break;

            case KernObj_MailboxOwner:
                if (((K2OSKERN_OBJ_MAILBOXOWNER *)pHdr)->RefMailbox.AsMailbox->RefGate.AsGate->SchedLocked.mOpen)
                {
                    if (!pMacroWait->mIsWaitAll)
                        waitSatisfied = TRUE;
                    countSatisfied++;
                }
                break;

            case KernObj_Gate:
                if (((K2OSKERN_OBJ_GATE *)pHdr)->SchedLocked.mOpen)
                {
                    if (!pMacroWait->mIsWaitAll)
                        waitSatisfied = TRUE;
                    countSatisfied++;
                }
                break;

            case KernObj_Alarm:
                if (!((K2OSKERN_OBJ_ALARM *)pHdr)->SchedLocked.mTimerActive)
                {
                    if (!pMacroWait->mIsWaitAll)
                        waitSatisfied = TRUE;
                    countSatisfied++;
                }
                break;

            case KernObj_SemUser:
                if (((K2OSKERN_OBJ_SEMUSER *)pHdr)->SemRef.AsSem->SchedLocked.mCount > 0)
                {
                    if (!pMacroWait->mIsWaitAll)
                        waitSatisfied = TRUE;
                    countSatisfied++;
                }
                break;

            case KernObj_Interrupt:
                // interrupt is a gate pulse and so can never be satisfied at wait latch
                break;

            default:
                K2OSKERN_Panic("*** KernSched_Locked_Thread_Wait - unknown object type (%d)\n", pHdr->mObjType);
                break;
            }

            if ((waitSatisfied) || (waitFailed))
                break;

            pWaitEntry++;
        }

        if (!waitFailed)
        {
            if ((!waitSatisfied) &&
                (pMacroWait->mIsWaitAll) && 
                (countSatisfied == pMacroWait->mNumEntries))
            {
                waitSatisfied = TRUE;
            }
        }

        if ((waitFailed) || (waitSatisfied))
        {
            if (waitSatisfied)
            {
                if (!pMacroWait->mIsWaitAll)
                {
                    // pWaitEntry points to entry satisfying the wait
                    KernSched_Locked_EarlySatisfyWaitEntry(apCallerThread, pWaitEntry);
                    if (apCallerThread->mIsKernelThread)
                    {
                        apCallerThread->Kern.mSchedCall_Result = TRUE;
                    }
                    else
                    {
                        apCallerThread->User.mSysCall_Result = TRUE;
                    }
                    apCallerThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = K2OS_Wait_Signalled_0 + pWaitEntry->mMacroIndex;
                }
                else
                {
                    // all entries together satisfy the wait
                    pWaitEntry = &pMacroWait->WaitEntry[0];
                    for (ixObj = 0; ixObj < pMacroWait->mNumEntries; ixObj++)
                    {
                        KernSched_Locked_EarlySatisfyWaitEntry(apCallerThread, pWaitEntry);
                        pWaitEntry++;
                    }
                    if (apCallerThread->mIsKernelThread)
                    {
                        apCallerThread->Kern.mSchedCall_Result = TRUE;
                    }
                    else
                    {
                        apCallerThread->User.mSysCall_Result = TRUE;
                    }
                    apCallerThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = K2OS_Wait_Signalled_0;
                }
            }

            if (!apCallerThread->mIsKernelThread)
            {
                KernSched_Locked_Release_Wait_References(pMacroWait);
            }

            //
            // returning false will resume thread
            //
            return FALSE;
        }
    }

    //
    // wait is not satisfied yet, and at this point cannot fail
    //

    if (pMacroWait->TimerItem.mHfTicks == 0)
    {
        //
        // checked and wait not satisfied.  do not wait as timeout is zero
        //
        if (!apCallerThread->mIsKernelThread)
        {
            apCallerThread->User.mSysCall_Result = FALSE;
        }
        else
        {
            apCallerThread->Kern.mSchedCall_Result = FALSE;
        }
        apCallerThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = K2OS_Wait_TimedOut;
        apCallerThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_TIMEOUT;

        if (!apCallerThread->mIsKernelThread)
        {
            KernSched_Locked_Release_Wait_References(pMacroWait);
        }

        //
        // returning false will resume thread
        //
        return FALSE;
    }

    //
    // at this point, we know that the thread is going to wait
    //
    apCallerThread->mState = KernThreadState_Waiting;

    KernSched_Locked_MountUnsatisfiedWait(pMacroWait);

    return TRUE;
}

void
KernSched_Locked_Thread_SysCall_AlarmCreate(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OSKERN_OBJ_ALARM *    pAlarm;

    pAlarm = apCallerThread->SchedItem.ObjRef.AsAlarm;

    K2_ASSERT(0 != pAlarm->SchedLocked.mHfTicks);

    pAlarm->SchedLocked.TimerItem.mIsMacroWait = FALSE;
    pAlarm->SchedLocked.TimerItem.mHfTicks = pAlarm->SchedLocked.mHfTicks;
    KernSched_Locked_InsertTimerItem(&pAlarm->SchedLocked.TimerItem);
    pAlarm->SchedLocked.mTimerActive = TRUE;

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);

    apCallerThread->User.mSysCall_Result = apCallerThread->SchedItem.Args.Alarm_Create.mAlarmToken;
}

void
KernSched_Locked_Mailbox_SentFirst(
    K2OSKERN_OBJ_MAILBOX *  apMailbox
)
{
    K2OS_MAILBOX_CONSUMER_PAGE *pCons;
    UINT32                      ixCons;

    pCons = (K2OS_MAILBOX_CONSUMER_PAGE *)apMailbox->mKernVirtAddr;
    ixCons = pCons->IxConsumer.mVal;
    K2_CpuReadBarrier();
    if ((0xFFFFFFFF != ixCons) &&
        (0 != (ixCons & K2OS_MAILBOX_GATE_CLOSED_BIT)))
    {
        K2ATOMIC_And(&pCons->IxConsumer.mVal, ~K2OS_MAILBOX_GATE_CLOSED_BIT);
        KernSched_Locked_GateChange(apMailbox->RefGate.AsGate, TRUE);
    }
}

K2STAT
KernSched_Locked_MailboxRecvLast(
    K2OSKERN_OBJ_MAILBOX *  apMailbox,
    K2OS_MSG *              apRetMsg,
    UINT32 *                apRetSlotIx
)
{
    K2OS_MAILBOX_CONSUMER_PAGE *pCons;
    K2OS_MAILBOX_PRODUCER_PAGE *pProd;
    K2OS_MAILBOX_MSGDATA_PAGE * pData;
    UINT32                      ixCons;
    UINT32                      ixWord;
    UINT32                      ixBit;
    UINT32                      ixProd;
    UINT32                      bitsVal;
    UINT32                      nextSlotIx;
    BOOL                        messageWasRetrieved;

    //
    // whoever called us thinks that we are going to take the
    // last message out of the mailbox.  if we do we need to close
    // the gate.
    //
    // in error case, recv was not done and last status is:
    //
    //  gate closed already                             K2STAT_ERROR_TIMEOUT
    //  no message found with open gate (transitory)    K2STAT_ERROR_EMPTY
    //  someone else consumed the last message OOB      K2STAT_ERROR_CHANGED
    //  

    pCons = (K2OS_MAILBOX_CONSUMER_PAGE *)apMailbox->mKernVirtAddr;
    pProd = (K2OS_MAILBOX_PRODUCER_PAGE *)(apMailbox->mKernVirtAddr + K2_VA_MEMPAGE_BYTES);
    pData = (K2OS_MAILBOX_MSGDATA_PAGE *)(apMailbox->mKernVirtAddr + (2 * K2_VA_MEMPAGE_BYTES));

    ixCons = pCons->IxConsumer.mVal;
    K2_CpuReadBarrier();
    if (0 != (ixCons & K2OS_MAILBOX_GATE_CLOSED_BIT))
    {
        //
        // gate is closed already and so there is no message available
        //
        return K2STAT_ERROR_EMPTY;
    }

    //
    // gate is not closed. try to receive
    //
    ixWord = ixCons >> 5;
    ixBit = ixCons & 0x1F;

    //
    // this comparison will fail if the producer is in between
    // incrementing its producer count and setting the ownership bit
    //
    bitsVal = pProd->OwnerMask[ixWord].mVal;
    K2_CpuReadBarrier();
    if (0 == (bitsVal & (1 << ixBit)))
    {
        //
        // no data present in the slot
        //
        return K2STAT_ERROR_EMPTY;
    }

    //
    // data present in slot
    //
    nextSlotIx = (ixCons + 1) & K2OS_MAILBOX_MSG_IX_MASK;

    ixProd = pProd->IxProducer.mVal;
    K2_CpuReadBarrier();

    if (ixProd != nextSlotIx)
    {
        // we are not going to shut the gate because there is more stuff in the mailbox now
        if (ixCons == K2ATOMIC_CompareExchange(&pCons->IxConsumer.mVal, nextSlotIx, ixCons))
        {
            //
            // we captured the message and left the gate open
            //
            K2MEM_Copy(apRetMsg, &pData->Msg[ixCons], sizeof(K2OS_MSG));
            *apRetSlotIx = ixCons;
            return K2STAT_NO_ERROR;
        }
    }

    //
    // still the last thing in the mailbox.  try to specutatively shut the gate
    //

    KernSched_Locked_GateChange(apMailbox->RefGate.AsGate, FALSE);

    messageWasRetrieved = FALSE;

    if (ixCons == K2ATOMIC_CompareExchange(&pCons->IxConsumer.mVal, K2OS_MAILBOX_GATE_CLOSED_BIT | nextSlotIx, ixCons))
    {
        //
        // we captured the message and shut the gate
        //
        K2MEM_Copy(apRetMsg, &pData->Msg[ixCons], sizeof(K2OS_MSG));
        *apRetSlotIx = ixCons;
        messageWasRetrieved = TRUE;
    }

    ixProd = pProd->IxProducer.mVal;
    K2_CpuReadBarrier();

    if ((!messageWasRetrieved) || (nextSlotIx != ixProd))
    {
        //
        // either we failed, or something new got put into the mailbox after we shut the gate
        //
        if (messageWasRetrieved)
        {
            //
            // we got the message but something new came in.  
            //
            K2ATOMIC_And(&pCons->IxConsumer.mVal, ~K2OS_MAILBOX_GATE_CLOSED_BIT);
        }

        KernSched_Locked_GateChange(apMailbox->RefGate.AsGate, TRUE);
    }

    if (!messageWasRetrieved)
        return K2STAT_ERROR_CHANGED;

    return K2STAT_NO_ERROR;
}

void
KernSched_Locked_Thread_SysCall_MailboxSentFirst(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OSKERN_OBJ_MAILBOX *pMailbox;

    pMailbox = apCallerThread->SchedItem.ObjRef.AsMailbox;
    K2_ASSERT(pMailbox->Hdr.mObjType == KernObj_Mailbox);

    KernSched_Locked_Mailbox_SentFirst(pMailbox);

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
}

void
KernSched_Locked_Thread_SysCall_MailboxRecvLast(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OSKERN_OBJ_MAILBOX *  pMailbox;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2STAT                  stat;

    K2_ASSERT(apCallerThread->SchedItem.ObjRef.AsAny != NULL);
    pMailbox = apCallerThread->SchedItem.ObjRef.AsMailbox;
    K2_ASSERT(pMailbox->Hdr.mObjType == KernObj_Mailbox);

    pThreadPage = apCallerThread->mpKernRwViewOfThreadPage;

    stat = KernSched_Locked_MailboxRecvLast(
        pMailbox, 
        (K2OS_MSG *)pThreadPage->mMiscBuffer, 
        &pThreadPage->mSysCall_Arg7_Result0);

    //
    // if we did not receive for any reason, 
    //      set last status
    //      return FALSE
    // if we received, 
    //      mSysCall_Arg7_Result0 = slot we received into
    //      return TRUE
    //
    if (K2STAT_IS_ERROR(stat))
    {
        apCallerThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
    else
    {
        apCallerThread->User.mSysCall_Result = (UINT32)TRUE;
    }

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
}

void
KernSched_Locked_Sem_Inc(
    K2OSKERN_OBJ_SEM *  apSem,
    UINT32            aRelCount
)
{
    K2LIST_LINK *           pListLink;
    K2OSKERN_MACROWAIT *    pMacroWait;
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_OBJ_THREAD *   pWaitingThread;

    if (apSem->SchedLocked.MacroWaitEntryList.mNodeCount > 0)
    {
        pListLink = apSem->SchedLocked.MacroWaitEntryList.mpHead;

        do {
            pWaitEntry = K2_GET_CONTAINER(K2OSKERN_WAITENTRY, pListLink, ObjWaitListLink);
            pMacroWait = K2_GET_CONTAINER(K2OSKERN_MACROWAIT, pWaitEntry, WaitEntry[pWaitEntry->mMacroIndex]);
            pWaitingThread = pMacroWait->mpWaitingThread;
            pListLink = pListLink->mpNext;

            if (pMacroWait->mIsWaitAll)
            {
                if (KernSched_Locked_TryCompleteWaitAll(pMacroWait, pWaitEntry->mMacroIndex))
                {
                    aRelCount--;
                }
            }
            else
            {
                K2_ASSERT(pWaitEntry->ObjRef.AsAny->mObjType == KernObj_SemUser);

                pWaitEntry->ObjRef.AsSemUser->SchedLocked.mHeldCount++;

                if (!pWaitingThread->mIsKernelThread)
                {
                    pWaitingThread->User.mSysCall_Result = TRUE;
                }
                else
                {
                    pWaitingThread->Kern.mSchedCall_Result = TRUE;
                }
                pWaitingThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = K2OS_Wait_Signalled_0 + pWaitEntry->mMacroIndex;

                K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

                KernSched_Locked_DismountWait(pMacroWait, !pWaitingThread->mIsKernelThread);

                pWaitingThread->mState = KernThreadState_InScheduler;

                KernSched_Locked_MakeThreadRun(pWaitingThread);

                aRelCount--;
            }

        } while ((NULL != pListLink) && (0 != aRelCount));
    }

    //
    // any count left goes onto the sem count
    //
    apSem->SchedLocked.mCount += aRelCount;
}

void
KernSched_Locked_Thread_SysCall_SemUser_SemInc(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OSKERN_OBJ_SEMUSER *  pSemUser;
    K2OSKERN_OBJ_SEM *      pSem;
    UINT32                  relCount;

    pSemUser = apCallerThread->SchedItem.ObjRef.AsSemUser;

    relCount = apCallerThread->SchedItem.Args.Sem_Inc.mCount;
    K2_ASSERT(relCount > 0);

    pSem = pSemUser->SemRef.AsSem;

    if ((relCount > pSem->mMaxCount) ||
        ((pSem->mMaxCount - pSem->SchedLocked.mCount) < relCount) ||
        (pSemUser->SchedLocked.mHeldCount < relCount))
    {
        apCallerThread->User.mSysCall_Result = FALSE;
        apCallerThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        apCallerThread->User.mSysCall_Result = TRUE;
        KernSched_Locked_Sem_Inc(
            pSemUser->SemRef.AsSem,
            apCallerThread->SchedItem.Args.Sem_Inc.mCount
        );
    }

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
}

void
KernSched_Locked_KernThread_IncSem(
    K2OSKERN_SCHED_ITEM *   apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_SEMUSER *  pSemUser;
    K2OSKERN_OBJ_SEM *      pSem;
    UINT32                  relCount;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pThread->mIsKernelThread);

    pSemUser = pThread->SchedItem.ObjRef.AsSemUser;

    relCount = pThread->SchedItem.Args.Sem_Inc.mCount;
    K2_ASSERT(relCount > 0);

    pSem = pSemUser->SemRef.AsSem;

    if ((relCount > pSem->mMaxCount) ||
        ((pSem->mMaxCount - pSem->SchedLocked.mCount) < relCount) ||
        (pSemUser->SchedLocked.mHeldCount < relCount))
    {
        pThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        pThread->Kern.mSchedCall_Result = FALSE;
    }
    else
    {
        pThread->Kern.mSchedCall_Result = TRUE;
        KernSched_Locked_Sem_Inc(
            pSemUser->SemRef.AsSem,
            pThread->SchedItem.Args.Sem_Inc.mCount
        );
    }

    KernSched_Locked_MakeThreadRun(pThread);
}

K2STAT
KernSched_Locked_IpcRejectRequest(
    K2OSKERN_OBJ_IPCEND *   apIpcEnd,
    UINT32                  aRequestId,
    UINT32                  aReasonCode
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    BOOL                    disp;
    K2OS_MSG                rejectMsg;
    K2STAT                  stat;
    K2OSKERN_IPC_REQUEST *  pReq;
    BOOL                    doSignal;
    K2OSKERN_OBJ_MAILBOX *  pMailbox;

    pProc = apIpcEnd->MailboxOwnerRef.AsMailboxOwner->RefProc.AsProc;
    if (NULL != pProc)
    {
        if (pProc->mState >= KernProcState_Stopping)
        {
            return K2STAT_ERROR_PROCESS_EXITED;
        }
    }

    pReq = NULL;

    disp = K2OSKERN_SeqLock(&apIpcEnd->Connection.SeqLock);

    if (apIpcEnd->Connection.Locked.mState != KernIpcEndState_Disconnected)
    {
        stat = K2STAT_ERROR_IN_USE;
    }
    else
    {
        K2OSKERN_SeqLock(&gData.Iface.SeqLock);

        pReq = apIpcEnd->Connection.Locked.mpOpenRequest;

        if ((NULL == pReq) || (pReq->mPending != 0xFFFFFFFF))
        {
            stat = K2STAT_ERROR_ABANDONED;
            pReq = NULL;
        }
        else
        {
            stat = K2STAT_NO_ERROR;

            pReq->mpRequestor = NULL;
            K2TREE_Remove(&gData.Iface.RequestTree, &pReq->TreeNode);
            apIpcEnd->Connection.Locked.mpOpenRequest = NULL;
        }

        K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, FALSE);
    }

    K2OSKERN_SeqUnlock(&apIpcEnd->Connection.SeqLock, disp);

    if (!K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(NULL != pReq);

        //
        // send rejection message to the requestors mailbox.
        // sender never knows its own request id so don't bother sending it
        //
        rejectMsg.mType = K2OS_SYSTEM_MSGTYPE_IPCEND;
        rejectMsg.mShort = K2OS_SYSTEM_MSG_IPCEND_SHORT_REJECTED;
        rejectMsg.mPayload[0] = (UINT32)apIpcEnd->mpUserKey;
        rejectMsg.mPayload[1] = aReasonCode;
        rejectMsg.mPayload[2] = 0;

        pMailbox = apIpcEnd->MailboxOwnerRef.AsMailboxOwner->RefMailbox.AsMailbox;

        doSignal = FALSE;
        KernMailbox_Deliver(
            pMailbox,
            &rejectMsg,
            TRUE,
            NULL,
            &doSignal
        );
        if (doSignal)
        {
            KernSched_Locked_Mailbox_SentFirst(pMailbox);
        }
    }
    else
    {
        K2_ASSERT(NULL == pReq);
    }

    return stat;
}

void
KernSched_Locked_LastToken_Destroyed(
    K2OSKERN_OBJREF *       apObjRef,
    K2OSKERN_OBJ_PROCESS *  apProc
)
{
    K2OSKERN_OBJ_MAILBOX *          pMailbox;
    K2OS_MAILBOX_CONSUMER_PAGE *    pCons;
    K2OSKERN_OBJ_IFINST *           pIfInst;
    BOOL                            disp;
    K2LIST_LINK *                   pListLink;
    K2OSKERN_OBJ_IFSUBS *           pSubs;
    K2OS_MSG                        msg;
    UINT32                          left;
    BOOL                            doSignal;

    switch (apObjRef->AsAny->mObjType)
    {
    case KernObj_Notify:
        KernSched_Locked_AbortNotify(apObjRef->AsNotify, K2STAT_ERROR_ABANDONED, apProc);
        break;

    case KernObj_Gate:
        KernSched_Locked_AbortGate(apObjRef->AsGate, K2STAT_ERROR_ABANDONED, apProc);
        break;

    case KernObj_MailboxOwner:
        pMailbox = apObjRef->AsMailboxOwner->RefMailbox.AsMailbox;
        pMailbox->mOwnerDead = TRUE;
        pCons = (K2OS_MAILBOX_CONSUMER_PAGE *)pMailbox->mKernVirtAddr;
        K2ATOMIC_Exchange(&pCons->IxConsumer.mVal, 0xFFFFFFFF);
        K2_CpuWriteBarrier();
        KernSched_Locked_AbortGate(apObjRef->AsMailboxOwner->RefMailbox.AsMailbox->RefGate.AsGate, K2STAT_ERROR_ABANDONED, apProc);
        break;

    case KernObj_IfInst:
        pIfInst = apObjRef->AsIfInst;
        K2_ASSERT(pIfInst->mIsPublic);
        K2_ASSERT(0 != pIfInst->mClassCode);
        K2_ASSERT(pIfInst->mIsDeparting);
        // remove the interface instance from the global classcode tree
        disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

        // id tree removal only happens at object cleanup
        pIfInst->mIsPublic = FALSE;

        pListLink = gData.Iface.SubsList.mpHead;
        if (NULL != pListLink)
        {
            //
            // now for each matching subscriber try to send a notification that
            // the interface instance has departed
            //
            msg.mType = K2OS_SYSTEM_MSGTYPE_IFINST;
            msg.mShort = K2OS_SYSTEM_MSG_IFINST_SHORT_DEPART;
            msg.mPayload[1] = pIfInst->IdTreeNode.mUserVal;

            do {
                pSubs = K2_GET_CONTAINER(K2OSKERN_OBJ_IFSUBS, pListLink, ListLink);

                if (((0 == pSubs->mClassCode) || (pSubs->mClassCode == pIfInst->mClassCode)) &&
                    ((!pSubs->mHasSpecific) || (0 == K2MEM_Compare(&pSubs->Specific, &pIfInst->SpecificGuid, sizeof(K2_GUID128)))) &&
                    (!pSubs->MailboxRef.AsMailbox->mOwnerDead))
                {
                    //
                    // this subscription matches enough of the interface for the subscriber to get a message
                    //
                    do {
                        left = pSubs->mBacklogRemaining;
                        K2_CpuReadBarrier();
                        if (left == 0)
                            break;
                    } while (left != K2ATOMIC_CompareExchange((UINT32 volatile *)&pSubs->mBacklogRemaining, left - 1, left));
                    if (0 != left)
                    {
                        msg.mPayload[0] = pSubs->mUserContext;
                        doSignal = FALSE;
                        KernMailbox_Deliver(
                            pSubs->MailboxRef.AsMailbox,
                            &msg,
                            FALSE,          // we are sending from a reserve so do not alloc
                            &pSubs->Hdr,    // this is the object that needs notification when the message is received
                            &doSignal);
                        if (doSignal)
                        {
                            KernSched_Locked_Mailbox_SentFirst(pSubs->MailboxRef.AsMailbox);
                        }
                    }
                }
                pListLink = pListLink->mpNext;

            } while (NULL != pListLink);
        }

        K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);
        break;

    default:
        // do not care
        break;
    }

    KernObj_ReleaseRef(apObjRef);
}

K2STAT 
KernSched_Locked_IfInstPublish(
    K2OSKERN_OBJ_IFINST *   apIfInst,
    UINT32                  aClassCode,
    K2_GUID128 const *      apSpecific
)
{
    BOOL                    disp;
    K2STAT                  stat;
    K2LIST_LINK *           pListLink;
    K2OSKERN_OBJ_IFSUBS *   pSubs;
    K2OS_MSG                msg;
    UINT32                  left;
    BOOL                    doSignal;

    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

    if (apIfInst->mIsPublic)
    {
        stat = K2STAT_ERROR_ALREADY_EXISTS;
    }
    else
    {
        if (apIfInst->mIsDeparting)
        {
            stat = K2STAT_ERROR_NOT_FOUND;
        }
        else
        {
            K2_ASSERT(0 != aClassCode);
            apIfInst->mIsPublic = TRUE;
            apIfInst->mClassCode = aClassCode;
            K2MEM_Copy(&apIfInst->SpecificGuid, apSpecific, sizeof(K2_GUID128));

            //
            // now for each matching subscriber try to send a notification that
            // the interface instance has arrived
            //
            pListLink = gData.Iface.SubsList.mpHead;
            if (NULL != pListLink)
            {
                msg.mType = K2OS_SYSTEM_MSGTYPE_IFINST;
                msg.mShort = K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE;
                msg.mPayload[1] = apIfInst->IdTreeNode.mUserVal;

                do {
                    pSubs = K2_GET_CONTAINER(K2OSKERN_OBJ_IFSUBS, pListLink, ListLink);
                    if (((0 == pSubs->mClassCode) || (pSubs->mClassCode == apIfInst->mClassCode)) &&
                        ((!pSubs->mHasSpecific) || (0 == K2MEM_Compare(&pSubs->Specific, &apIfInst->SpecificGuid, sizeof(K2_GUID128)))) &&
                        (!pSubs->MailboxRef.AsMailbox->mOwnerDead))
                    {
                        //
                        // this subscription matches enough of the interface for the subscriber to get a message
                        //
                        do {
                            left = pSubs->mBacklogRemaining;
                            K2_CpuReadBarrier();
                            if (left == 0)
                                break;
                        } while (left != K2ATOMIC_CompareExchange((UINT32 volatile *)&pSubs->mBacklogRemaining, left - 1, left));
                        if (0 != left)
                        {
                            msg.mPayload[0] = pSubs->mUserContext;
                            doSignal = FALSE;
                            KernMailbox_Deliver(
                                pSubs->MailboxRef.AsMailbox,
                                &msg,
                                FALSE,          // do not alloc as this is sent from a reserve backlog
                                &pSubs->Hdr,    // this is the object notified when message has completed receive
                                &doSignal);
                            if (doSignal)
                            {
                                KernSched_Locked_Mailbox_SentFirst(pSubs->MailboxRef.AsMailbox);
                            }
                        }
                    }
                    pListLink = pListLink->mpNext;

                } while (NULL != pListLink);
            }

            stat = K2STAT_NO_ERROR;
        }
    }

    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    return stat;
}

void
KernSched_Locked_Thread_SysCall_IfInst_Publish(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OSKERN_OBJ_IFINST *   pIfInst;
    K2STAT                  stat;
    K2OS_THREAD_PAGE *      pThreadPage;

    K2_ASSERT(apCallerThread->SchedItem.ObjRef.AsAny != NULL);
    pIfInst = apCallerThread->SchedItem.ObjRef.AsIfInst;
    K2_ASSERT(pIfInst->Hdr.mObjType == KernObj_IfInst);

    pThreadPage = apCallerThread->mpKernRwViewOfThreadPage;

    stat = KernSched_Locked_IfInstPublish(pIfInst, apCallerThread->SchedItem.Args.IfInst_Publish.mClassCode, (K2_GUID128 const *)pThreadPage->mMiscBuffer);
    if (K2STAT_IS_ERROR(stat))
    {
        apCallerThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
    else
    {
        apCallerThread->User.mSysCall_Result = (UINT32)TRUE;
    }

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
}

void
KernSched_Locked_Thread_SysCall_IpcEndManualDisconnect(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OSKERN_OBJ_IPCEND *   pIpcEnd;

    K2_ASSERT(apCallerThread->SchedItem.ObjRef.AsAny != NULL);
    pIpcEnd = apCallerThread->SchedItem.ObjRef.AsIpcEnd;
    K2_ASSERT(pIpcEnd->Hdr.mObjType == KernObj_IpcEnd);

    if (!KernSched_Locked_IpcEnd_Disconnect(pIpcEnd))
    {
        apCallerThread->User.mSysCall_Result = 0;
        apCallerThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_NOT_CONNECTED;
    }
    else
    {
        apCallerThread->User.mSysCall_Result = (UINT32)TRUE;
    }

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
}

void
KernSched_Locked_Thread_SysCall_IpcRejectRequest(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OSKERN_OBJ_IPCEND *   pRemoteIpcEnd;
    K2STAT                  stat;

    K2_ASSERT(apCallerThread->SchedItem.ObjRef.AsAny != NULL);
    pRemoteIpcEnd = apCallerThread->SchedItem.ObjRef.AsIpcEnd;
    K2_ASSERT(pRemoteIpcEnd->Hdr.mObjType == KernObj_IpcEnd);

    stat = KernSched_Locked_IpcRejectRequest(pRemoteIpcEnd, apCallerThread->SchedItem.Args.Ipc_Reject.mRequestId, apCallerThread->SchedItem.Args.Ipc_Reject.mReasonCode);
    if (K2STAT_IS_ERROR(stat))
    {
        apCallerThread->User.mSysCall_Result = 0;
        apCallerThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
    else
    {
        apCallerThread->User.mSysCall_Result = (UINT32)TRUE;
    }

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
}

void
KernSched_Locked_KernToken_Destroy(
    K2OS_TOKEN aToken
)
{
    BOOL                    disp;
    K2OSKERN_KERN_TOKEN *   pKernToken;
    K2OSKERN_OBJREF         objRef;
    BOOL                    decToZero;
    BOOL                    doSchedCall;

    pKernToken = (K2OSKERN_KERN_TOKEN *)aToken;
    K2_ASSERT(KERN_TOKEN_SENTINEL1 == pKernToken->mSentinel1);
    K2_ASSERT(KERN_TOKEN_SENTINEL2 == pKernToken->mSentinel2);

    pKernToken->mSentinel1 = 0;
    pKernToken->mSentinel2 = 0;

    objRef.AsAny = NULL;

    disp = K2OSKERN_SeqLock(&gData.Token.SeqLock);

    KernObj_CreateRef(&objRef, pKernToken->Ref.AsAny);

    if (0 == K2ATOMIC_Dec((INT32 volatile *)&pKernToken->Ref.AsAny->mTokenCount))
    {
        decToZero = TRUE;
    }
    else
    {
        decToZero = FALSE;
    }

    KernObj_ReleaseRef(&pKernToken->Ref);

    K2LIST_AddAtTail(&gData.Token.FreeList, (K2LIST_LINK *)pKernToken);

    K2OSKERN_SeqUnlock(&gData.Token.SeqLock, disp);

    if (!decToZero)
    {
        KernObj_ReleaseRef(&objRef);
        return;
    }
    //
    // objRef just destroyed a token and that made the token count
    // go to zero.  If this is for a waitable object then anything
    // waiting on it needs to have the waits aborted. also need to
    // deal with case of interface instance leaving
    //
    doSchedCall = FALSE;

    switch (objRef.AsAny->mObjType)
    {
    case KernObj_Gate:
    case KernObj_Notify:
    case KernObj_MailboxOwner:
    case KernObj_IfInst:
        doSchedCall = TRUE;
        break;

    case KernObj_IpcEnd:
        // must already be disconnected 
        K2_ASSERT(objRef.AsIpcEnd->Connection.Locked.mState == KernIpcEndState_Disconnected);
        break;

    default:
        break;
    }

    if (doSchedCall)
    {
        if (objRef.AsAny->mObjType == KernObj_IfInst)
        {
            disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
            K2_ASSERT(disp);

            if (objRef.AsIfInst->mIsPublic)
            {
                objRef.AsIfInst->mIsDeparting = TRUE;
            }
            else
            {
                doSchedCall = FALSE;
            }

            K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);
        }

        if (doSchedCall)
        {
            KernSched_Locked_LastToken_Destroyed(&objRef, NULL);
        }
    }

    if (!doSchedCall)
    {
        K2_ASSERT(NULL != objRef.AsAny);
        KernObj_ReleaseRef(&objRef);
    }

    K2_ASSERT(NULL == objRef.AsAny);
}

BOOL
KernSched_Locked_ProcTokenLocked_Token_Destroy(
    K2OSKERN_OBJ_PROCESS *  apProc,
    K2OS_TOKEN              aToken
);

K2STAT
KernSched_Locked_IpcAccept(
    K2OSKERN_OBJ_THREAD *   apCallerThread,
    K2OSKERN_OBJ_IPCEND *   apLocalIpcEnd
)
{
    K2OSKERN_SCHED_ITEM *   pSchedItem;
    K2OSKERN_OBJ_IPCEND *   pRemoteIpcEnd;
    K2OSKERN_OBJ_PROCESS *  pRemoteProc;
    BOOL                    disp;
    K2OSKERN_IPC_REQUEST *  pReq;
    K2STAT                  stat;
    K2OS_MSG                connectMsg;

    pSchedItem = &apCallerThread->SchedItem;

    pRemoteIpcEnd = pSchedItem->Args.Ipc_Accept.RemoteEndRef.AsIpcEnd;
    pRemoteProc = pRemoteIpcEnd->MailboxOwnerRef.AsMailboxOwner->RefProc.AsProc;

    pReq = NULL;

    if ((NULL == pRemoteProc) || (pRemoteProc->mState < KernProcState_Stopping))
    {
        disp = K2OSKERN_SeqLock(&apLocalIpcEnd->Connection.SeqLock);
        if (apLocalIpcEnd->Connection.Locked.mState == KernIpcEndState_Disconnected)
        {
            K2OSKERN_SeqLock(&pRemoteIpcEnd->Connection.SeqLock);
            if (pRemoteIpcEnd->Connection.Locked.mState == KernIpcEndState_Disconnected)
            {
                K2OSKERN_SeqLock(&gData.Iface.SeqLock);
                if ((NULL != pRemoteIpcEnd->Connection.Locked.mpOpenRequest) &&
                    (pRemoteIpcEnd->Connection.Locked.mpOpenRequest->mPending == (UINT32)&apLocalIpcEnd->Hdr))
                {
                    //
                    // can connect if we can manufacture the tokens for each endpoint
                    //
                    stat = K2STAT_NO_ERROR;

                    apLocalIpcEnd->Connection.Locked.mState = KernIpcEndState_Connected;
                    KernObj_CreateRef(&apLocalIpcEnd->Connection.Locked.Connected.PartnerIpcEndRef, &pRemoteIpcEnd->Hdr);
                    KernObj_CreateRef(&apLocalIpcEnd->Connection.Locked.Connected.WriteMapRef, pSchedItem->Args.Ipc_Accept.LocalMapOfRemoteBufferRef.AsAny);

                    pRemoteIpcEnd->Connection.Locked.mState = KernIpcEndState_Connected;
                    KernObj_CreateRef(&pRemoteIpcEnd->Connection.Locked.Connected.PartnerIpcEndRef, &apLocalIpcEnd->Hdr);
                    KernObj_CreateRef(&pRemoteIpcEnd->Connection.Locked.Connected.WriteMapRef, pSchedItem->Args.Ipc_Accept.RemoteMapOfLocalBufferRef.AsAny);

                    //
                    // eat the request that got us here
                    //
                    pReq = pRemoteIpcEnd->Connection.Locked.mpOpenRequest;
                    K2_ASSERT(NULL != pReq);    // sanity
                    pReq->mpRequestor = NULL;
                    K2TREE_Remove(&gData.Iface.RequestTree, &pReq->TreeNode);
                    pRemoteIpcEnd->Connection.Locked.mpOpenRequest = NULL;
                }
                else
                {
                    stat = K2STAT_ERROR_ABANDONED;
                }
                K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, FALSE);
            }
            else
            {
                stat = K2STAT_ERROR_IN_USE;
            }
            K2OSKERN_SeqUnlock(&pRemoteIpcEnd->Connection.SeqLock, FALSE);
        }
        else
        {
            stat = K2STAT_ERROR_CONNECTED;
        }

        K2OSKERN_SeqUnlock(&apLocalIpcEnd->Connection.SeqLock, disp);
    }
    else
    {
        stat = K2STAT_ERROR_PROCESS_EXITED;
    }

    if (!K2STAT_IS_ERROR(stat))
    {
//        K2OSKERN_Debug("\nCONNECT\n\n");
        connectMsg.mType = K2OS_SYSTEM_MSGTYPE_IPCEND;
        connectMsg.mShort = K2OS_SYSTEM_MSG_IPCEND_SHORT_CONNECTED;
        connectMsg.mPayload[0] = (UINT32)apLocalIpcEnd->mpUserKey;
        connectMsg.mPayload[1] = (pRemoteIpcEnd->mMaxMsgCount << 16) | pRemoteIpcEnd->mMaxMsgBytes;
        connectMsg.mPayload[2] = (UINT32)pSchedItem->Args.Ipc_Accept.mTokLocalMapOfRemote;
        KernSched_Locked_Mailbox_DeliverFromReserve(
            apLocalIpcEnd->MailboxOwnerRef.AsMailboxOwner->RefMailbox.AsMailbox, 
            &connectMsg, 
            NULL
        );

        connectMsg.mPayload[0] = (UINT32)pRemoteIpcEnd->mpUserKey;
        connectMsg.mPayload[1] = (apLocalIpcEnd->mMaxMsgCount << 16) | apLocalIpcEnd->mMaxMsgBytes;
        connectMsg.mPayload[2] = (UINT32)pSchedItem->Args.Ipc_Accept.mTokRemoteMapOfLocal;
        KernSched_Locked_Mailbox_DeliverFromReserve(
            pRemoteIpcEnd->MailboxOwnerRef.AsMailboxOwner->RefMailbox.AsMailbox,
            &connectMsg,
            NULL
        );

        K2_ASSERT(NULL != pReq);
        KernHeap_Free(pReq);
    }
    else
    {
        K2_ASSERT(NULL == pReq);

        if (apCallerThread->mIsKernelThread)
        {
            // local is the kernel
            KernSched_Locked_KernToken_Destroy(pSchedItem->Args.Ipc_Accept.mTokLocalMapOfRemote);
        }
        else
        {
            // local is a process
            KernSched_Locked_ProcTokenLocked_Token_Destroy(apCallerThread->User.ProcRef.AsProc, pSchedItem->Args.Ipc_Accept.mTokLocalMapOfRemote);
        }

        if (NULL == pRemoteProc)
        {
            // remote is the kernel
            KernSched_Locked_KernToken_Destroy(pSchedItem->Args.Ipc_Accept.mTokRemoteMapOfLocal);
        }
        else
        {
            // remote is a process
            KernSched_Locked_ProcTokenLocked_Token_Destroy(pRemoteProc, pSchedItem->Args.Ipc_Accept.mTokRemoteMapOfLocal);
        }
    }

    KernObj_ReleaseRef(&apCallerThread->SchedItem.Args.Ipc_Accept.LocalMapOfRemoteBufferRef);
    KernObj_ReleaseRef(&apCallerThread->SchedItem.Args.Ipc_Accept.RemoteMapOfLocalBufferRef);
    KernObj_ReleaseRef(&apCallerThread->SchedItem.Args.Ipc_Accept.RemoteEndRef);

    return stat;
}

void
KernSched_Locked_Thread_SysCall_IpcAccept(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OSKERN_OBJ_IPCEND *   pIpcEnd;
    K2STAT                  stat;

    K2_ASSERT(apCallerThread->SchedItem.ObjRef.AsAny != NULL);
    pIpcEnd = apCallerThread->SchedItem.ObjRef.AsIpcEnd;
    K2_ASSERT(pIpcEnd->Hdr.mObjType == KernObj_IpcEnd);

    stat = KernSched_Locked_IpcAccept(apCallerThread, pIpcEnd);
    if (K2STAT_IS_ERROR(stat))
    {
        apCallerThread->User.mSysCall_Result = 0;
        apCallerThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
    else
    {
        apCallerThread->User.mSysCall_Result = (UINT32)TRUE;
    }

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
}

void
KernSched_Locked_Thread_SysCall_SysProcRegister(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2_ASSERT(NULL != gData.SysProc.mTokReady2);
    KernSched_Locked_GateChange(gData.SysProc.RefReady1.AsGate, TRUE);
}

void
KernSched_Locked_Thread_SysCall(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pCallerThread;
    K2OSKERN_OBJ_PROCESS *  pCallerProc;
    K2OSKERN_OBJ_THREAD *   pNewThread;
    BOOL                    procIsAlive;
    BOOL                    disp;

    pCallerThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pCallerThread->mState == KernThreadState_InScheduler);
    K2_ASSERT(apItem == &pCallerThread->SchedItem);

    pCallerProc = pCallerThread->User.ProcRef.AsProc;
    
    KTRACE(gData.Sched.mpSchedulingCore, 4, KTRACE_SCHED_EXEC_SYSCALL, pCallerProc->mId, pCallerThread->mGlobalIx, pCallerThread->User.mSysCall_Id);

    procIsAlive = (pCallerProc->mState < KernProcState_Stopping) ? TRUE : FALSE;

    switch (pCallerThread->User.mSysCall_Id)
    {
    // any of these signal a notify which may unblock a thread
    case K2OS_SYSCALL_ID_NOTIFY_SIGNAL:
        K2_ASSERT(apItem->ObjRef.AsAny != NULL);
        K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_Notify);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_SignalNotify(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_GATE_CHANGE:
        K2_ASSERT(apItem->ObjRef.AsAny != NULL);
        K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_Gate);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_ChangeGate(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_PROCESS_EXIT:
        K2_ASSERT(apItem->ObjRef.AsAny == NULL);
        if (!procIsAlive)
        {
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_ProcessExit(pCallerThread);
        // impossible to fail - return not break
        return;

    case K2OS_SYSCALL_ID_THREAD_EXIT:
        K2_ASSERT(apItem->ObjRef.AsAny == NULL);
        KernSched_Locked_Thread_SysCall_ThreadExit(pCallerThread);
        // impossible to fail - return not break
        return;

    case K2OS_SYSCALL_ID_THREAD_CREATE:
        K2_ASSERT(apItem->ObjRef.AsAny != NULL);
        K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_Thread);
        pNewThread = apItem->ObjRef.AsThread;
        if (!procIsAlive)
        {
            K2_ASSERT(pNewThread->mState == KernThreadState_Created);
            pNewThread->mState = KernThreadState_Exited;
            pNewThread->mExitCode = pCallerProc->mExitCode;
            disp = K2OSKERN_SeqLock(&pCallerProc->Thread.SeqLock);
            K2LIST_Remove(&pCallerProc->Thread.Locked.CreateList, &pNewThread->OwnerThreadListLink);
            K2LIST_AddAtTail(&pCallerProc->Thread.Locked.DeadList, &pNewThread->OwnerThreadListLink);
            K2OSKERN_SeqUnlock(&pCallerProc->Thread.SeqLock, disp);
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_ThreadCreate(pCallerThread);
        // thread scheduling order requires us to return here not break
        return;

    case K2OS_SYSCALL_ID_THREAD_WAIT:
        K2_ASSERT(apItem->ObjRef.AsAny == NULL);
        if (!procIsAlive)
        {
            //
            // process died in between thread taking references and it getting here
            // in scheduler.  So references in wait list are held
            //
            KernSched_Locked_Release_Wait_References(&pCallerThread->MacroWait);
            //
            // now we can exit the thread
            //
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        if (KernSched_Locked_Thread_Wait(pCallerThread))
        {
            // succeeded to wait
            return;
        }
        // failed to wait
        break;

    case K2OS_SYSCALL_ID_THREAD_SETAFF:
        K2_ASSERT(apItem->ObjRef.AsAny == NULL);
        if (!procIsAlive)
        {
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_SetAffinity(pCallerThread, apItem->Args.Thread_SetAff.mNewMask);
        break;

    case K2OS_SYSCALL_ID_ALARM_CREATE:
        K2_ASSERT(apItem->ObjRef.AsAny != NULL);
        K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_Alarm);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_AlarmCreate(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_SEM_INC:
        K2_ASSERT(apItem->ObjRef.AsAny != NULL);
        K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_SemUser);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_SemUser_SemInc(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_TOKEN_DESTROY:
        K2_ASSERT(apItem->ObjRef.AsAny != NULL);
        // this always happens even if the caller thread's process is exited
        KernSched_Locked_LastToken_Destroyed(&apItem->ObjRef, pCallerProc);
        if (!procIsAlive)
        {
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        break;

    case K2OS_SYSCALL_ID_MAILBOX_SENTFIRST:
    case K2OS_SYSCALL_ID_IPCEND_CREATE:
    case K2OS_SYSCALL_ID_IPCEND_SEND:
    case K2OS_SYSCALL_ID_IPCEND_REQUEST:
        K2_ASSERT(apItem->ObjRef.AsAny != NULL);
        K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_Mailbox);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_MailboxSentFirst(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_MAILBOXOWNER_RECVLAST:
        K2_ASSERT(apItem->ObjRef.AsAny != NULL);
        K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_Mailbox);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_MailboxRecvLast(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_IFINST_PUBLISH:
        K2_ASSERT(apItem->ObjRef.AsAny != NULL);
        K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_IfInst);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_IfInst_Publish(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_IPCEND_MANUAL_DISCONNECT:
        K2_ASSERT(apItem->ObjRef.AsAny != NULL);
        K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_IpcEnd);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_IpcEndManualDisconnect(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_IPCREQ_REJECT:
        K2_ASSERT(apItem->ObjRef.AsAny != NULL);
        K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_IpcEnd);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_IpcRejectRequest(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_IPCEND_ACCEPT:
        K2_ASSERT(apItem->ObjRef.AsAny != NULL);
        K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_IpcEnd);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->Args.Ipc_Accept.LocalMapOfRemoteBufferRef);
            KernObj_ReleaseRef(&apItem->Args.Ipc_Accept.RemoteMapOfLocalBufferRef);
            KernObj_ReleaseRef(&apItem->Args.Ipc_Accept.RemoteEndRef);
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_IpcAccept(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_SYSPROC_REGISTER:
        K2_ASSERT(apItem->ObjRef.AsAny == NULL);
        if (!procIsAlive)
        {
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_SysProcRegister(pCallerThread);
        break;

    default:
        K2OSKERN_Panic("KernSched_Locked_Thread_SysCall - hit unsupported call (%d)\n", pCallerThread->User.mSysCall_Id);
        break;
    }

    K2_ASSERT(apItem->ObjRef.AsAny == NULL);

    KernSched_Locked_MakeThreadRun(pCallerThread);
}

void 
KernSched_Locked_Aborted_Running_Thread(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pAbortThread;

    pAbortThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(KernThreadState_InScheduler == pAbortThread->mState);
    KernSched_Locked_ExitThread(pAbortThread, K2STAT_ERROR_ABORTED);
}

void
KernSched_Locked_Thread_Crash_Process(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pCrashThread;
    K2OSKERN_OBJ_PROCESS *  pCrashProc;

    pCrashThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pCrashThread->mState == KernThreadState_InScheduler);
    K2_ASSERT(apItem == &pCrashThread->SchedItem);

    pCrashProc = pCrashThread->User.ProcRef.AsProc;
    K2_ASSERT(apItem->ObjRef.AsAny == NULL);

//    K2OSKERN_Debug(
//        "Sched: Unhandled Thread %d exception crashes process %d\n", 
//        pCrashThread->mGlobalIx, 
//        pCrashProc->mId
//    );

    KernSched_Locked_ExitThread(pCrashThread, pCrashThread->User.LastEx.mExCode);

    KernSched_Locked_StopProcess(pCrashProc, pCrashThread->User.LastEx.mExCode);
}

void
KernSched_Locked_ResumeDeferral_Completed(
    K2OSKERN_SCHED_ITEM *   apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);

    K2_ASSERT(pThread->Hdr.mObjType == KernObj_Thread);
    K2_ASSERT(pThread->SchedItem.ObjRef.AsThread == pThread);

    if (pThread->mState != KernThreadState_Exited)
    {
        K2_ASSERT(pThread->mState == KernThreadState_InScheduler_ResumeDeferred);
        K2LIST_Remove(&gData.Sched.Locked.DefferedResumeList, &pThread->SchedListLink);

        pThread->mState = KernThreadState_InScheduler;
        KernSched_Locked_MakeThreadRun(pThread);
    }

    //
    // this is the thread's reference to itself while resume was deferred
    //
    KernObj_ReleaseRef(&pThread->SchedItem.ObjRef);
}

void
KernSched_Locked_Alarm_Cleanup(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_ALARM *    pAlarm;

    pAlarm = K2_GET_CONTAINER(K2OSKERN_OBJ_ALARM, apItem, CleanupSchedItem);

    K2_ASSERT(pAlarm->SchedLocked.MacroWaitEntryList.mNodeCount == 0);

    if (pAlarm->SchedLocked.mTimerActive)
    {
        KernSched_Locked_RemoveTimerItem(&pAlarm->SchedLocked.TimerItem);
        pAlarm->SchedLocked.mTimerActive = FALSE;
    }

    pAlarm->Hdr.ObjDpc.Func = KernAlarm_PostCleanupDpc;
    KernCpu_QueueDpc(&pAlarm->Hdr.ObjDpc.Dpc, &pAlarm->Hdr.ObjDpc.Func, KernDpcPrio_Med);
}

void
KernSched_Locked_SemUser_Cleanup(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_SEMUSER *  pSemUser;

    pSemUser = K2_GET_CONTAINER(K2OSKERN_OBJ_SEMUSER, apItem, CleanupSchedItem);

    KernSched_Locked_Sem_Inc(pSemUser->SemRef.AsSem, pSemUser->SchedLocked.mHeldCount);
    pSemUser->SchedLocked.mHeldCount = 0;

    pSemUser->Hdr.ObjDpc.Func = KernSemUser_PostCleanupDpc;
    KernCpu_QueueDpc(&pSemUser->Hdr.ObjDpc.Dpc, &pSemUser->Hdr.ObjDpc.Func, KernDpcPrio_Med);
}

void
KernSched_Locked_Interrupt(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_INTERRUPT * pIntr;

    pIntr = K2_GET_CONTAINER(K2OSKERN_OBJ_INTERRUPT, apItem, SchedItem);

    //
    // only place in the system an interrupt can go into service
    //
    K2_ASSERT(!pIntr->mInService);
    pIntr->mInService = TRUE;

    KernSched_Locked_GateChange(pIntr->GateRef.AsGate, TRUE);
    KernSched_Locked_GateChange(pIntr->GateRef.AsGate, FALSE);

    KernObj_ReleaseRef(&pIntr->SchedItem.ObjRef);
}

void
KernSched_Locked_KernThreadExit(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD * pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);

    K2_ASSERT(0 == pThread->Kern.HeldCsList.mNodeCount);

    KernSched_Locked_ExitThread(pThread, pThread->mExitCode);
}

void
KernSched_Locked_KernThread_StartProc(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_PROCESS *  pNewProc;
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_THREAD *   pNewThread;
    BOOL                    disp;
    K2OS_MSG                sysProcMsg;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pThread->mIsKernelThread);

    K2_ASSERT(NULL != pThread->Kern.ResultRef.AsAny);
    K2_ASSERT(KernObj_Process == pThread->Kern.ResultRef.AsAny->mObjType);
    pNewProc = pThread->Kern.ResultRef.AsProc;
    K2_ASSERT(NULL != pNewProc->Thread.Locked.CreateList.mpHead);

    pNewThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pNewProc->Thread.Locked.CreateList.mpHead, OwnerThreadListLink);
    K2_ASSERT(!pNewThread->mIsKernelThread);
    K2_ASSERT(pNewThread->mState == KernThreadState_Created);
    K2_ASSERT(pNewProc->mState = KernProcState_Launching);

    pNewProc->mState = KernProcState_Starting;

    disp = K2OSKERN_SeqLock(&pNewProc->Thread.SeqLock);
    K2LIST_Remove(&pNewProc->Thread.Locked.CreateList, &pNewThread->OwnerThreadListLink);
    K2LIST_AddAtTail(&pNewProc->Thread.SchedLocked.ActiveList, &pNewThread->OwnerThreadListLink);
    K2OSKERN_SeqUnlock(&pNewProc->Thread.SeqLock, disp);

    // token for process already created so this ref release will not destroy the process object
    // the new thread in the new process also has a ProcRef
    KernObj_ReleaseRef(&pThread->Kern.ResultRef);
    pThread->Kern.mSchedCall_Result = TRUE;
    KernSched_Locked_MakeThreadRun(pThread);

    //
    // now resume new thread, which actually has no references but still exists
    //
    KTRACE(gData.Sched.mpSchedulingCore, 3, KTRACE_PROC_START, 0, pNewThread->mGlobalIx);

#if K2OSKERN_TRACE_THREAD_LIFE
    K2OSKERN_Debug("-- Proc %d Starting, Thread %d\n", 0, pNewThread->mGlobalIx);
#endif
    KernObj_CreateRef(&pNewThread->Running_SelfRef, &pNewThread->Hdr);
    pNewThread->mState = KernThreadState_InScheduler;

    K2ATOMIC_Inc(&gData.mSystemWideProcessCount);
    sysProcMsg.mType = K2OS_SYSTEM_MSGTYPE_SYSPROC;
    sysProcMsg.mShort = K2OS_SYSTEM_MSG_SYSPROC_SHORT_PROCESS_CREATED;
    sysProcMsg.mPayload[0] = pNewProc->mId;
    sysProcMsg.mPayload[1] = 0;                                 // process that started it
    sysProcMsg.mPayload[2] = pThread->mGlobalIx;                // thread that started it
    KernSched_Locked_NotifySysProc(&sysProcMsg);

    K2ATOMIC_Inc(&gData.mSystemWideThreadCount);
    sysProcMsg.mShort = K2OS_SYSTEM_MSG_SYSPROC_SHORT_THREAD_CREATED;
    sysProcMsg.mPayload[0] = pNewProc->mId;
    sysProcMsg.mPayload[1] = pNewThread->mGlobalIx;             
    sysProcMsg.mPayload[2] = pThread->mGlobalIx;                // thread that started it
    KernSched_Locked_NotifySysProc(&sysProcMsg);

    KernSched_Locked_MakeThreadRun(pNewThread);
}

void
KernSched_Locked_KernThread_Wait(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pThread->mIsKernelThread);

    if (!KernSched_Locked_Thread_Wait(pThread))
    {
        KernSched_Locked_MakeThreadRun(pThread);
    }
}

void
KernSched_Locked_KernThread_SignalNotify(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pThread->mIsKernelThread);

    K2_ASSERT(apItem->ObjRef.AsAny != NULL);

    K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_Notify);

    KernSched_Locked_SignalNotify(apItem->ObjRef.AsNotify);

    pThread->Kern.mSchedCall_Result = (UINT32)TRUE;

    KernSched_Locked_MakeThreadRun(pThread);
}

void
KernSched_Locked_KernThread_SetAffinity(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pThread->mIsKernelThread);

    pThread->Config.mAffinityMask = (UINT8)(apItem->Args.Thread_SetAff.mNewMask & 0xFF);
    pThread->Kern.mSchedCall_Result = TRUE;

    KernSched_Locked_MakeThreadRun(pThread);
}

void
KernSched_Locked_KernThread_MountAlarm(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_ALARM *    pAlarm;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pThread->mIsKernelThread);

    K2_ASSERT(apItem->ObjRef.AsAny != NULL);
    K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_Alarm);
    pAlarm = apItem->ObjRef.AsAlarm;

    K2_ASSERT(0 != pAlarm->SchedLocked.mHfTicks);

    pAlarm->SchedLocked.TimerItem.mIsMacroWait = FALSE;
    pAlarm->SchedLocked.TimerItem.mHfTicks = pAlarm->SchedLocked.mHfTicks;
    KernSched_Locked_InsertTimerItem(&pAlarm->SchedLocked.TimerItem);
    pAlarm->SchedLocked.mTimerActive = TRUE;

    KernSched_Locked_MakeThreadRun(pThread);
}

void
KernSched_Locked_KernThread_GateChange(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_GATE *     pGate;
    UINT32                  newState;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pThread->mIsKernelThread);

    K2_ASSERT(apItem->ObjRef.AsAny != NULL);
    K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_Gate);
    pGate = apItem->ObjRef.AsGate;
    newState = apItem->Args.Gate_Change.mNewState;

    if (newState <= 2)
    {
        if (newState != 0)
        {
            KernSched_Locked_GateChange(pGate, TRUE);
        }
        if (newState != 1)
        {
            KernSched_Locked_GateChange(pGate, FALSE);
        }
        pThread->Kern.mSchedCall_Result = TRUE;
    }
    else
    {
        pThread->Kern.mSchedCall_Result = FALSE;
    }

    KernSched_Locked_MakeThreadRun(pThread);
}

void
KernSched_Locked_KernThread_StartThread(
    K2OSKERN_SCHED_ITEM * apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_THREAD *   pNewThread;
    BOOL                    disp;
    K2OS_MSG                sysProcMsg;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pThread->mIsKernelThread);
    K2_ASSERT(apItem->ObjRef.AsAny != NULL);
    K2_ASSERT(apItem->ObjRef.AsAny->mObjType == KernObj_Thread);
    pNewThread = apItem->ObjRef.AsThread;
    K2_ASSERT(pNewThread->mState == KernThreadState_Created);

    disp = K2OSKERN_SeqLock(&gData.Thread.ListSeqLock);
    K2LIST_Remove(&gData.Thread.CreateList, &pNewThread->OwnerThreadListLink);
    K2LIST_AddAtTail(&gData.Thread.ActiveList, &pNewThread->OwnerThreadListLink);
    K2OSKERN_SeqUnlock(&gData.Thread.ListSeqLock, disp);

    //
    // first resume caller thread so it gets first crack at the core it just
    // was running on
    //
    KernSched_Locked_MakeThreadRun(pThread);

    //
    // now resume new thread
    //
    KTRACE(gData.Sched.mpSchedulingCore, 3, KTRACE_THREAD_START, 0, pNewThread->mGlobalIx);
#if K2OSKERN_TRACE_THREAD_LIFE
    K2OSKERN_Debug("-- Kernel Thread %d Start\n", pNewThread->mGlobalIx);
#endif
    pNewThread->mState = KernThreadState_InScheduler;
    KernObj_CreateRef(&pNewThread->Running_SelfRef, &pNewThread->Hdr);

    K2ATOMIC_Inc(&gData.mSystemWideThreadCount);
    sysProcMsg.mType = K2OS_SYSTEM_MSGTYPE_SYSPROC;
    sysProcMsg.mShort = K2OS_SYSTEM_MSG_SYSPROC_SHORT_THREAD_CREATED;
    sysProcMsg.mPayload[0] = 0;
    sysProcMsg.mPayload[1] = pNewThread->mGlobalIx;             // process that started it
    sysProcMsg.mPayload[2] = pThread->mGlobalIx;                // thread that started it
    KernSched_Locked_NotifySysProc(&sysProcMsg);

    KernSched_Locked_MakeThreadRun(pNewThread);
}

void
KernSched_Locked_KernThread_LastTokenDestroyed(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pThread->mIsKernelThread);
    K2_ASSERT(apItem->ObjRef.AsAny != NULL);

    KernSched_Locked_LastToken_Destroyed(&apItem->ObjRef, NULL);

    KernSched_Locked_MakeThreadRun(pThread);
}

void
KernSched_Locked_KernThread_MailboxSentFirst(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_MAILBOX *  pMailbox;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pThread->mIsKernelThread);
    K2_ASSERT(apItem->ObjRef.AsAny != NULL);
    pMailbox = apItem->ObjRef.AsMailbox;
    K2_ASSERT(pMailbox->Hdr.mObjType == KernObj_Mailbox);

    KernSched_Locked_Mailbox_SentFirst(pMailbox);

    KernSched_Locked_MakeThreadRun(pThread);
}

void
KernSched_Locked_KernThread_MailboxRecvLast(
    K2OSKERN_SCHED_ITEM *   apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_MAILBOX *  pMailbox;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2STAT                  stat;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);
    K2_ASSERT(pThread->mIsKernelThread);
    K2_ASSERT(apItem->ObjRef.AsAny != NULL);
    pMailbox = apItem->ObjRef.AsMailbox;
    K2_ASSERT(pMailbox->Hdr.mObjType == KernObj_Mailbox);

    pThreadPage = pThread->mpKernRwViewOfThreadPage;

    //
    // if we did not receive for any reason, 
    //      set last status
    //      return FALSE
    // if we received, 
    //      mSysCall_Arg7_Result0 = slot we received into
    //      return TRUE
    //
    stat = KernSched_Locked_MailboxRecvLast(
        pMailbox, 
        (K2OS_MSG *)pThreadPage->mMiscBuffer,
        &pThreadPage->mSysCall_Arg7_Result0
    );

    if (K2STAT_IS_ERROR(stat))
    {
        pThread->Kern.mSchedCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
    else
    {
        pThread->Kern.mSchedCall_Result = (UINT32)TRUE;

    }

    KernSched_Locked_MakeThreadRun(pThread);
}

void
KernSched_Locked_KernThread_IfInstPublish(
    K2OSKERN_SCHED_ITEM *   apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_IFINST *   pIfInst;
    K2STAT                  stat;
    K2OS_THREAD_PAGE *      pThreadPage;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);

    K2_ASSERT(pThread->mIsKernelThread);
    K2_ASSERT(apItem->ObjRef.AsAny != NULL);
    pIfInst = apItem->ObjRef.AsIfInst;
    K2_ASSERT(pIfInst->Hdr.mObjType == KernObj_IfInst);

    pThreadPage = pThread->mpKernRwViewOfThreadPage;

    stat = KernSched_Locked_IfInstPublish(pIfInst, apItem->Args.IfInst_Publish.mClassCode, (K2_GUID128 const *)pThreadPage->mMiscBuffer);
    if (K2STAT_IS_ERROR(stat))
    {
        pThread->Kern.mSchedCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
    else
    {
        pThread->Kern.mSchedCall_Result = (UINT32)TRUE;
    }

    KernSched_Locked_MakeThreadRun(pThread);
}

void
KernSched_Locked_KernThread_IpcEndManualDisconnect(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_IPCEND *   pIpcEnd;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);

    K2_ASSERT(pThread->mIsKernelThread);
    K2_ASSERT(apItem->ObjRef.AsAny != NULL);
    pIpcEnd = apItem->ObjRef.AsIpcEnd;
    K2_ASSERT(pIpcEnd->Hdr.mObjType == KernObj_IfInst);

    if (!KernSched_Locked_IpcEnd_Disconnect(pIpcEnd))
    {
        pThread->Kern.mSchedCall_Result = 0;
        pThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_NOT_CONNECTED;;
    }
    else
    {
        pThread->Kern.mSchedCall_Result = (UINT32)TRUE;
    }

    KernSched_Locked_MakeThreadRun(pThread);
}

void
KernSched_Locked_KernThread_IpcRejectRequest(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_IPCEND *   pRemoteIpcEnd;
    K2STAT                  stat;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);

    K2_ASSERT(pThread->mIsKernelThread);
    K2_ASSERT(apItem->ObjRef.AsAny != NULL);
    pRemoteIpcEnd = apItem->ObjRef.AsIpcEnd;
    K2_ASSERT(pRemoteIpcEnd->Hdr.mObjType == KernObj_IpcEnd);

    stat = KernSched_Locked_IpcRejectRequest(pRemoteIpcEnd, apItem->Args.Ipc_Reject.mRequestId, apItem->Args.Ipc_Reject.mReasonCode);
    if (K2STAT_IS_ERROR(stat))
    {
        pThread->Kern.mSchedCall_Result = 0;
        pThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
    else
    {
        pThread->Kern.mSchedCall_Result = (UINT32)TRUE;
    }

    KernSched_Locked_MakeThreadRun(pThread);
}

void
KernSched_Locked_KernThread_IpcAccept(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_IPCEND *   pIpcEnd;
    K2STAT                  stat;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);

    K2_ASSERT(pThread->mIsKernelThread);
    K2_ASSERT(apItem->ObjRef.AsAny != NULL);
    pIpcEnd = apItem->ObjRef.AsIpcEnd;
    K2_ASSERT(pIpcEnd->Hdr.mObjType == KernObj_IpcEnd);

    stat = KernSched_Locked_IpcAccept(pThread, pIpcEnd);
    if (K2STAT_IS_ERROR(stat))
    {
        pThread->Kern.mSchedCall_Result = 0;
        pThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
    else
    {
        pThread->Kern.mSchedCall_Result = (UINT32)TRUE;
    }

    KernSched_Locked_MakeThreadRun(pThread);
}

void
KernSched_Locked_NotifyProxy(
    K2OSKERN_SCHED_ITEM *   apItem
)
{
    K2OSKERN_OBJ_NOTIFYPROXY *  pNotifyProxy;

    pNotifyProxy = K2_GET_CONTAINER(K2OSKERN_OBJ_NOTIFYPROXY, apItem, SchedItem);
    
    KernSched_Locked_SignalNotify(pNotifyProxy->RefNotify.AsNotify);

    KernObj_ReleaseRef(&pNotifyProxy->RefSelf);
}

void 
KernSched_Locked_ExecOneItem(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    KernSchedItemType itemType;

    KernSched_Locked_TimePassedUntil(&apItem->mHfTick);

    itemType = apItem->mType;
    apItem->mType = KernSchedItem_Invalid;

    if (itemType != KernSchedItem_Thread_SysCall)
    {
        KTRACE(gData.Sched.mpSchedulingCore, 2, KTRACE_SCHED_EXEC_ITEM, itemType);
    }

    switch (itemType)
    {
    case KernSchedItem_Thread_SysCall:
        KernSched_Locked_Thread_SysCall(apItem);
        break;

    case KernSchedItem_Aborted_Running_Thread:
        KernSched_Locked_Aborted_Running_Thread(apItem);
        break;

    case KernSchedItem_SchedTimer_Fired:
        // no-op.  this exists to get into the "TimePassedUntil" above.
        break;

    case KernSchedItem_Thread_Crash_Process:
        KernSched_Locked_Thread_Crash_Process(apItem);
        break;

    case KernSchedItem_Thread_ResumeDeferral_Completed:
        KernSched_Locked_ResumeDeferral_Completed(apItem);
        break;

    case KernSchedItem_Alarm_Cleanup:
        KernSched_Locked_Alarm_Cleanup(apItem);
        break;

    case KernSchedItem_SemUser_Cleanup:
        KernSched_Locked_SemUser_Cleanup(apItem);
        break;

    case KernSchedItem_Interrupt:
        KernSched_Locked_Interrupt(apItem);
        break;

    case KernSchedItem_NotifyProxy:
        KernSched_Locked_NotifyProxy(apItem);
        break;

    case KernSchedItem_ProcStopped:
        KernSched_Locked_ProcStopped(apItem);
        break;

    case KernSchedItem_KernThread_Exit:
        KernSched_Locked_KernThreadExit(apItem);
        break;

    case KernSchedItem_KernThread_SemInc:
        KernSched_Locked_KernThread_IncSem(apItem);
        break;

    case KernSchedItem_KernThread_StartProc:
        KernSched_Locked_KernThread_StartProc(apItem);
        break;

    case KernSchedItem_KernThread_Wait:
        KernSched_Locked_KernThread_Wait(apItem);
        break;

    case KernSchedItem_KernThread_SignalNotify:
        KernSched_Locked_KernThread_SignalNotify(apItem);
        break;

    case KernSchedItem_KernThread_SetAffinity:
        KernSched_Locked_KernThread_SetAffinity(apItem);
        break;

    case KernSchedItem_KernThread_MountAlarm:
        KernSched_Locked_KernThread_MountAlarm(apItem);
        break;

    case KernSchedItem_KernThread_ChangeGate:
        KernSched_Locked_KernThread_GateChange(apItem);
        break;

    case KernSchedItem_KernThread_StartThread:
        KernSched_Locked_KernThread_StartThread(apItem);
        break;

    case KernSchedItem_KernThread_LastTokenDestroyed:
        KernSched_Locked_KernThread_LastTokenDestroyed(apItem);
        break;

    case KernSchedItem_KernThread_MailboxSentFirst:
        KernSched_Locked_KernThread_MailboxSentFirst(apItem);
        break;

    case KernSchedItem_KernThread_MailboxRecvLast:
        KernSched_Locked_KernThread_MailboxRecvLast(apItem);
        break;

    case KernSchedItem_KernThread_IfInstPublish:
        KernSched_Locked_KernThread_IfInstPublish(apItem);
        break;

    case KernSchedItem_KernThread_IpcEndManualDisconnect:
        KernSched_Locked_KernThread_IpcEndManualDisconnect(apItem);
        break;

    case KernSchedItem_KernThread_IpcRejectRequest:
        KernSched_Locked_KernThread_IpcRejectRequest(apItem);
        break;

    case KernSchedItem_KernThread_IpcAccept:
        KernSched_Locked_KernThread_IpcAccept(apItem);
        break;

    default:
        K2OSKERN_Panic("KernSched_Locked_ExecOneItem - unknown item type (%d)\n", itemType);
        break;
    }
}

BOOL
KernSched_Locked_NotifySysProc(
    K2OS_MSG const *apMsg
)
{
    BOOL    doSignal;
    K2STAT  stat;

    doSignal = FALSE;
    stat = KernSysProc_Notify(FALSE, apMsg, &doSignal);
    if (K2STAT_IS_ERROR(stat))
    {
        return FALSE;
    }

    if (doSignal)
    {
        KernSched_Locked_SignalNotify(gData.SysProc.RefNotify.AsNotify);
    }

    return TRUE;
}