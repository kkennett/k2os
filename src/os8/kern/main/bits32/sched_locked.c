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

    affMask = apThread->UserConfig.mAffinityMask;
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
    KTRACE(gData.Sched.mpSchedulingCore, 4, KTRACE_THREAD_MIGRATE, apThread->ProcRef.Ptr.AsProc->mId, apThread->mGlobalIx, pTargetCore->mCoreIx);
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
    UINT_PTR                ixObj;

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

            pHdr = pWaitEntry->ObjRef.Ptr.AsHdr;

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

            case KernObj_Mailbox:
                pList = &((K2OSKERN_OBJ_MAILBOX *)pHdr)->Common.NotifyRef.Ptr.AsNotify->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Gate:
                pList = &((K2OSKERN_OBJ_GATE *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Alarm:
                pList = &((K2OSKERN_OBJ_ALARM *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_SemUser:
                pList = &((K2OSKERN_OBJ_SEMUSER *)pHdr)->SemRef.Ptr.AsSem->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Interrupt:
                pList = &((K2OSKERN_OBJ_INTERRUPT *)pHdr)->GateRef.Ptr.AsGate->SchedLocked.MacroWaitEntryList;
                break;

            default:
                K2_ASSERT(0);
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
KernSched_Locked_DismountWaitAndReleaseRefs(
    K2OSKERN_MACROWAIT *    apMacroWait
)
{
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_OBJ_HEADER *   pHdr;
    K2LIST_ANCHOR *         pList;
    UINT_PTR                ixObj;

    if (apMacroWait->mNumEntries > 0)
    {
        pWaitEntry = &apMacroWait->WaitEntry[0];
        for (ixObj = 0; ixObj < apMacroWait->mNumEntries; ixObj++)
        {
            K2_ASSERT(pWaitEntry->mMacroIndex == ixObj);
            K2_ASSERT(0 != pWaitEntry->mMounted);
            pHdr = pWaitEntry->ObjRef.Ptr.AsHdr;
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

            case KernObj_Mailbox:
                pList = &((K2OSKERN_OBJ_MAILBOX *)pHdr)->Common.NotifyRef.Ptr.AsNotify->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Gate:
                pList = &((K2OSKERN_OBJ_GATE *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Alarm:
                pList = &((K2OSKERN_OBJ_ALARM *)pHdr)->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_SemUser:
                pList = &((K2OSKERN_OBJ_SEMUSER *)pHdr)->SemRef.Ptr.AsSem->SchedLocked.MacroWaitEntryList;
                break;

            case KernObj_Interrupt:
                pList = &((K2OSKERN_OBJ_INTERRUPT *)pHdr)->GateRef.Ptr.AsGate->SchedLocked.MacroWaitEntryList;
                break;

            default:
                K2_ASSERT(0);
            }

            K2LIST_Remove(pList, &pWaitEntry->ObjWaitListLink);

            KernObj_ReleaseRef(&pWaitEntry->ObjRef);

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
    UINT_PTR                aSignallingIndex
)
{
    // if wait is completed return true
    K2_ASSERT(0);
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

    KernSched_Locked_DismountWaitAndReleaseRefs(apMacroWait);

    pWaitingThread->mState = KernThreadState_InScheduler;

    pWaitingThread->mSysCall_Result = K2STAT_ERROR_TIMEOUT;
    pWaitingThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_TIMEOUT;

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
                pWaitingThread->mSysCall_Result = K2OS_THREAD_WAIT_SIGNALLED_0 + pWaitEntry->mMacroIndex;

                K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

                KernSched_Locked_DismountWaitAndReleaseRefs(pMacroWait);

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
    K2OSKERN_OBJ_PROCESS *  apExitingProcess
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

        pWaitingThread->mSysCall_Result = K2OS_THREAD_WAIT_ABANDONED_0 + pWaitEntry->mMacroIndex;
        pWaitingThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_CLOSED;

        K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

        KernSched_Locked_DismountWaitAndReleaseRefs(pMacroWait);

        pWaitingThread->mState = KernThreadState_InScheduler;

        //
        // notify may being aborted due to mailbox close due to process exiting.
        // do not make the thread run if the process is in its exit code
        //
        if (KernProcState_Stopping > pWaitingThread->ProcRef.Ptr.AsProc->mState)
        {
            KernSched_Locked_MakeThreadRun(pWaitingThread);
        }
        else
        {
            K2_ASSERT(apExitingProcess == pWaitingThread->ProcRef.Ptr.AsProc);
            KernSched_Locked_ExitThread(pWaitingThread, pWaitingThread->ProcRef.Ptr.AsProc->mExitCode);
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
            pWaitingThread->mSysCall_Result = K2OS_THREAD_WAIT_SIGNALLED_0 + pWaitEntry->mMacroIndex;

            K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

            KernSched_Locked_DismountWaitAndReleaseRefs(pMacroWait);

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
            pWaitingThread->mSysCall_Result = K2OS_THREAD_WAIT_SIGNALLED_0 + pWaitEntry->mMacroIndex;

            K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

            KernSched_Locked_DismountWaitAndReleaseRefs(pMacroWait);

            pWaitingThread->mState = KernThreadState_InScheduler;

            KernSched_Locked_MakeThreadRun(pWaitingThread);
        }

    } while (NULL != pListLink);
}

K2STAT
KernSched_Locked_MailboxSendFromReserve(
    K2OSKERN_MBOX_COMMON *  apMailboxCommon,
    K2OS_MSG *              apMsg,
    K2OSKERN_OBJ_HEADER *   apReserveUser
)
{
    K2STAT  stat;
    BOOL    doSignal;

    doSignal = FALSE;
    stat = KernMailbox_DeliverInternal(
        apMailboxCommon,
        apMsg,
        FALSE, TRUE, apReserveUser,   // do not alloc, set reserved bit
        &doSignal);
    if (K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(stat == K2STAT_ERROR_CLOSED);
    }
    if (doSignal)
    {
        KernSched_Locked_SignalNotify(apMailboxCommon->NotifyRef.Ptr.AsNotify);
    }
    return stat;
}

BOOL
KernSched_Locked_DisconnectIpcEnd(
    K2OSKERN_OBJ_IPCEND *   apLocal
)
{
    BOOL                    disp;
    BOOL                    disconnected;
    K2OSKERN_OBJ_IPCEND *   pRemote;
    K2OS_MSG                disconnectMsg;

    K2MEM_Zero(&disconnectMsg, sizeof(disconnectMsg));
    disconnectMsg.mControl = K2OS_SYSTEM_MSG_IPC_DISCONNECTED;
    disp = K2OSKERN_SeqLock(&apLocal->Connection.SeqLock);
    if (apLocal->Connection.Locked.mState == KernIpcEndState_Connected)
    {
        disconnected = TRUE;
        pRemote = apLocal->Connection.Locked.Connected.PartnerIpcEndRef.Ptr.AsIpcEnd;
        K2OSKERN_SeqLock(&pRemote->Connection.SeqLock);
        
        apLocal->Connection.Locked.mState = KernIpcEndState_WaitDisAck;
        if (apLocal->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc->mState < KernProcState_Stopping)
        {
            disconnectMsg.mPayload[0] = (UINT32)apLocal->mpUserKey;
            KernSched_Locked_MailboxSendFromReserve(&apLocal->MailboxRef.Ptr.AsMailbox->Common, &disconnectMsg, &apLocal->Hdr);
        }

        pRemote->Connection.Locked.mState = KernIpcEndState_WaitDisAck;
        disconnectMsg.mPayload[0] = (UINT32)pRemote->mpUserKey;
        KernSched_Locked_MailboxSendFromReserve(&pRemote->MailboxRef.Ptr.AsMailbox->Common, &disconnectMsg, &pRemote->Hdr);

        K2OSKERN_SeqUnlock(&pRemote->Connection.SeqLock, FALSE);
    }
    else
        disconnected = FALSE;
    K2OSKERN_SeqUnlock(&apLocal->Connection.SeqLock, disp);

    return disconnected;
}

void
KernSched_Locked_CloseMailbox(
    K2OSKERN_MBOX_COMMON *  apMailboxCommon,
    K2OSKERN_OBJ_THREAD *   apCloserThread,
    K2OSKERN_OBJ_PROCESS *  apExitingProcess
)
{
    K2OSKERN_OBJ_MAILBOX *   pMailbox;

    apMailboxCommon->SchedOnlyCanSet.mIsClosed = TRUE;

    //
    // any waits on the mailbox's notify are aborted
    //
    KernSched_Locked_AbortNotify(apMailboxCommon->NotifyRef.Ptr.AsNotify, K2STAT_ERROR_CLOSED, apExitingProcess);

    //
    // queue dpc to destroy the mailbox's token
    //
    if (NULL != apCloserThread)
    {
        K2_ASSERT(KernThreadState_InScheduler_ResumeDeferred == apCloserThread->mState);
        KernObj_CreateRef(&apMailboxCommon->SchedOnlyCanSet.CloserThreadRef, &apCloserThread->Hdr);
    }

    //
    // post close dpc
    //
    pMailbox = K2_GET_CONTAINER(K2OSKERN_OBJ_MAILBOX, apMailboxCommon, Common);
    pMailbox->Hdr.ObjDpc.Func = KernMailbox_PostCloseDpc;
    KernCpu_QueueDpc(&pMailbox->Hdr.ObjDpc.Dpc, &pMailbox->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
}

void
KernSched_Locked_IfaceLocked_InterfaceChange(
    K2OSKERN_IFACE *        apIface,
    BOOL                    aArrival,
    K2OSKERN_OBJ_PROCESS *  apPublisherProc
)
{
    K2STAT                  stat;
    K2LIST_LINK *           pListLink;
    K2OSKERN_OBJ_IFSUBS *   pSubs;
    K2OS_MSG                msg;
    K2OSKERN_OBJ_MAILBOX *  pMailbox;
    K2OSKERN_OBJ_PROCESS *  pProc;
    BOOL                    doSignal;
    UINT_PTR                rem;
    BOOL                    didSend;

    pListLink = gData.Iface.Locked.SubsList.mpHead;
    if (NULL == pListLink)
        return;

    msg.mControl = aArrival ? K2OS_SYSTEM_MSG_IFINST_ARRIVE : K2OS_SYSTEM_MSG_IFINST_DEPART;
    msg.mPayload[1] = apIface->GlobalTreeNode.mUserVal;
    msg.mPayload[2] = apIface->mpProc->mId;

    do {
        pSubs = K2_GET_CONTAINER(K2OSKERN_OBJ_IFSUBS, pListLink, GlobalSubsListLink);
        pListLink = pListLink->mpNext;

        //
        // publishers do not get notified of their own publish
        //
        if ((pSubs->mProcSelfNotify) ||
            (pSubs->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc != apPublisherProc))
        {
            do {
                rem = pSubs->mBacklogRemaining;
                K2_CpuReadBarrier();
                if (rem == 0)
                    break;
            } while (rem != K2ATOMIC_CompareExchange((UINT32 volatile *)&pSubs->mBacklogRemaining, rem - 1, rem));

            if (rem != 0)
            {
                //
                // we got a reserve entry we can use
                //
                didSend = FALSE;

                //
                // does interface match the subscription ?
                //
                if ((pSubs->mClassCode == 0) || (pSubs->mClassCode == apIface->mClassCode))
                {
                    if ((!pSubs->mHasSpecific) ||
                        (0 == K2MEM_Compare(&pSubs->Specific, &apIface->SpecificGuid, sizeof(K2_GUID128))))
                    {
                        //
                        // send reserved message to mailbox
                        //
                        msg.mPayload[0] = pSubs->mUserContext;

                        pMailbox = pSubs->MailboxRef.Ptr.AsMailbox;
                        pProc = pMailbox->Common.ProcRef.Ptr.AsProc;
                        if (pProc->mState < KernProcState_Stopping)
                        {
                            stat = KernMailbox_DeliverInternal(
                                &pMailbox->Common,
                                &msg,
                                FALSE,
                                TRUE,
                                &pSubs->Hdr,
                                &doSignal);
                            if (!K2STAT_IS_ERROR(stat))
                            {
                                didSend = TRUE;
                                if (doSignal)
                                {
                                    KernSched_Locked_SignalNotify(pMailbox->Common.NotifyRef.Ptr.AsNotify);
                                }
                            }
                        }
                    }
                }
                if (!didSend)
                {
                    K2ATOMIC_Inc(&pSubs->mBacklogRemaining);
                }
            }
        }
    } while (NULL != pListLink);
}

void
KernSched_Locked_AllProcThreadsExitedAndDpcRan(
    K2OSKERN_OBJ_PROCESS *  apProc
)
{
    K2LIST_LINK *           pListLink;
    K2OSKERN_OBJ_MAILBOX *  pMailbox;
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_OBJ_IPCEND *   pIpcEnd;
    BOOL                    disp;
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_MACROWAIT *    pMacroWait;
    K2OSKERN_IFACE *		pIface;
    K2LIST_ANCHOR           delIfaceList;

    K2_ASSERT(apProc->mState == KernProcState_Stopping);
    K2_ASSERT(apProc->Thread.SchedLocked.ActiveList.mNodeCount == 0);
    K2_ASSERT(apProc->mStopDpcRan);
    apProc->mState = KernProcState_Stopped;
    K2_CpuWriteBarrier();
    KTRACE(gData.Sched.mpSchedulingCore, 2, KTRACE_PROC_STOPPED, apProc->mId);
    K2OSKERN_Debug("Process %d Stopped\n", apProc->mId);

    //
    // unpublish all interfaces this process has published
    //
    K2LIST_Init(&delIfaceList);
    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
    pListLink = apProc->Iface.GlobalLocked.List.mpHead;
    if (NULL != pListLink)
    {
        do {
            pIface = K2_GET_CONTAINER(K2OSKERN_IFACE, pListLink, ProcIfaceListLink);
            pListLink = pListLink->mpNext;
            if (pIface->mClassCode != 0)
            {
                KernSched_Locked_IfaceLocked_InterfaceChange(pIface, FALSE, NULL);
            }
            K2TREE_Remove(&gData.Iface.Locked.InstanceTree, &pIface->GlobalTreeNode);
        } while (NULL != pListLink);
        K2MEM_Copy(&delIfaceList, &apProc->Iface.GlobalLocked.List, sizeof(K2LIST_ANCHOR));
        K2MEM_Zero(&apProc->Iface.GlobalLocked.List, sizeof(K2LIST_ANCHOR));
    }
    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);
    pListLink = delIfaceList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pIface = K2_GET_CONTAINER(K2OSKERN_IFACE, pListLink, ProcIfaceListLink);
            pListLink = pListLink->mpNext;
            KernObj_ReleaseRef(&pIface->MailboxRef);
            K2MEM_Zero(pIface, sizeof(K2OSKERN_IFACE));
            KernHeap_Free(pIface);
        } while (NULL != pListLink);
    }

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
                pThread->mSysCall_Result = K2OS_THREAD_WAIT_SIGNALLED_0 + pWaitEntry->mMacroIndex;

                K2_ASSERT(KernThreadState_Waiting == pThread->mState);

                KernSched_Locked_DismountWaitAndReleaseRefs(pMacroWait);

                pThread->mState = KernThreadState_InScheduler;

                //
                // theads cannot wait on their own process
                //
                K2_ASSERT(pThread->ProcRef.Ptr.AsProc != apProc);
                KernSched_Locked_MakeThreadRun(pThread);
            }
        } while (pListLink != NULL);
    }

    //
    // disconnect all IPC endpoints
    //
    disp = K2OSKERN_SeqLock(&apProc->IpcEnd.SeqLock);
    pListLink = apProc->IpcEnd.Locked.List.mpHead;
    if (NULL != pListLink)
    {
        pIpcEnd = K2_GET_CONTAINER(K2OSKERN_OBJ_IPCEND, pListLink, ProcIpcEndListLocked.ListLink);
        pListLink = pListLink->mpNext;
        KernSched_Locked_DisconnectIpcEnd(pIpcEnd);
    }
    K2OSKERN_SeqUnlock(&apProc->IpcEnd.SeqLock, disp);

    //
    // close all mailboxes - this may cause threads to exit
    //
    disp = K2OSKERN_SeqLock(&apProc->Mail.SeqLock);
    pListLink = apProc->Mail.Locked.MailboxList.mpHead;
    if (NULL != pListLink)
    {
        do
        {
            pMailbox = K2_GET_CONTAINER(K2OSKERN_OBJ_MAILBOX, pListLink, Common.ProcMailboxListLink);
            pListLink = pListLink->mpNext;
            KernSched_Locked_CloseMailbox(&pMailbox->Common, NULL, apProc);
        } while (NULL != pListLink);
    }
    K2OSKERN_SeqUnlock(&apProc->Mail.SeqLock, disp);

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
            if (pThread->ProcRef.Ptr.AsProc == apProc)
            {
                K2LIST_Remove(&gData.Sched.Locked.DefferedResumeList, &pThread->SchedListLink);
                KernSched_Locked_ExitThread(pThread, aExitCode);
            }
        } while (NULL != pListLink);
    }

    // 
    // any active threads from this process waiting need to have those waits aborted
    // and the threads exited
    //
    pListLink = apProc->Thread.SchedLocked.ActiveList.mpHead;
    if (NULL != pListLink)
    {
        do
        {
            pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListLink, ProcThreadListLink);
            pListLink = pListLink->mpNext;

            if (pThread->mState == KernThreadState_Waiting)
            {
                //
                // abort the thread wait and remove macrowait from lists it is on
                //
                KernSched_Locked_DismountWaitAndReleaseRefs(&pThread->MacroWait);
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

    K2_ASSERT(KernThreadState_Exited != apThread->mState);

//    K2OSKERN_Debug("Sched: Proc %d Thread %d Exits (%08X(%d)) from state %d\n", 
//        apThread->ProcRef.Ptr.AsProc->mId, apThread->mGlobalIx, 
//        aExitCode, aExitCode, apThread->mState);
    apThread->mState = KernThreadState_Exited;
    apThread->mExitCode = aExitCode;
    K2_CpuWriteBarrier();
    KTRACE(gData.Sched.mpSchedulingCore, 4, KTRACE_THREAD_EXIT, apThread->ProcRef.Ptr.AsProc->mId, apThread->mGlobalIx, aExitCode);

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
                pWaitingThread->mSysCall_Result = K2OS_THREAD_WAIT_SIGNALLED_0 + pWaitEntry->mMacroIndex;

                K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

                KernSched_Locked_DismountWaitAndReleaseRefs(pMacroWait);

                pWaitingThread->mState = KernThreadState_InScheduler;

                //
                // theads cannot wait on themselves
                //
                K2_ASSERT(pWaitingThread != apThread);
                KernSched_Locked_MakeThreadRun(pWaitingThread);
            }
        } while (pListLink != NULL);
    }

    pProc = apThread->ProcRef.Ptr.AsProc;

    disp = K2OSKERN_SeqLock(&pProc->Thread.SeqLock);
    K2LIST_Remove(&pProc->Thread.SchedLocked.ActiveList, &apThread->ProcThreadListLink);
    K2LIST_AddAtTail(&pProc->Thread.Locked.DeadList, &apThread->ProcThreadListLink);
    K2OSKERN_SeqUnlock(&pProc->Thread.SeqLock, disp);

    KernObj_ReleaseRef(&apThread->Running_SelfRef);

    if (pProc->Thread.SchedLocked.ActiveList.mNodeCount > 0)
    {
        //
        // still more threads running
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

    pNotify = apCallerThread->SchedItem.ObjRef.Ptr.AsNotify;
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

    pGate = apCallerThread->SchedItem.ObjRef.Ptr.AsGate;
    K2_ASSERT(NULL != pGate);
    // not possible for this call to fail
    KernSched_Locked_GateChange(pGate, apCallerThread->SchedItem.Args.Gate_Change.mSetTo);
    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
}

void 
KernSched_Locked_Thread_SysCall_MailboxClose(
    K2OSKERN_OBJ_THREAD *   apCallerThread,
    K2OSKERN_MBOX_COMMON *  apMailboxCommon
)
{
    K2_ASSERT(NULL != apMailboxCommon);
    if (!apMailboxCommon->SchedOnlyCanSet.mIsClosed)
    {
        K2LIST_AddAtTail(&gData.Sched.Locked.DefferedResumeList, &apCallerThread->SchedListLink);
        apCallerThread->mState = KernThreadState_InScheduler_ResumeDeferred;
        KernSched_Locked_CloseMailbox(apMailboxCommon, apCallerThread, NULL);
        apCallerThread->mSysCall_Result = TRUE;
        KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
        return;
    }

    apCallerThread->mSysCall_Result = FALSE;
    apCallerThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_CLOSED;
    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
    KernSched_Locked_MakeThreadRun(apCallerThread);
}

void 
KernSched_Locked_Thread_SysCall_ProcessExit(
    K2OSKERN_OBJ_THREAD *apCallerThread
)
{
    KernSched_Locked_ExitThread(apCallerThread, apCallerThread->mSysCall_Arg0);

    KernSched_Locked_StopProcess(apCallerThread->ProcRef.Ptr.AsProc, apCallerThread->mSysCall_Arg0);
}

void 
KernSched_Locked_Thread_SysCall_ThreadExit(
    K2OSKERN_OBJ_THREAD *apCallerThread
)
{
    KernSched_Locked_ExitThread(apCallerThread, apCallerThread->mSysCall_Arg0);
}

void 
KernSched_Locked_Thread_SysCall_ThreadCreate(
    K2OSKERN_OBJ_THREAD *apCallerThread
)
{
    K2OSKERN_OBJ_PROCESS *  pCallerProc;
    K2OSKERN_OBJ_THREAD *   pNewThread;
    BOOL                    disp;

    pNewThread = apCallerThread->SchedItem.ObjRef.Ptr.AsThread;
    K2_ASSERT(NULL != pNewThread);
    K2_ASSERT(pNewThread->mState == KernThreadState_Created);
    
    pCallerProc = apCallerThread->ProcRef.Ptr.AsProc;

    disp = K2OSKERN_SeqLock(&pCallerProc->Thread.SeqLock);
    K2LIST_Remove(&pCallerProc->Thread.Locked.CreateList, &pNewThread->ProcThreadListLink);
    K2LIST_AddAtTail(&pCallerProc->Thread.SchedLocked.ActiveList, &pNewThread->ProcThreadListLink);
    K2OSKERN_SeqUnlock(&pCallerProc->Thread.SeqLock, disp);

    //
    // first resume caller thread so it gets first crack at the core it just
    // was running on
    //
    apCallerThread->mSysCall_Result = apCallerThread->SchedItem.Args.Thread_Create.mThreadToken;
    apCallerThread->mpKernRwViewOfUserThreadPage->mSysCall_Arg7_Result0 = pNewThread->mGlobalIx;
    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
    KernSched_Locked_MakeThreadRun(apCallerThread);

    //
    // now resume new thread
    //
    KTRACE(gData.Sched.mpSchedulingCore, 3, KTRACE_THREAD_START, pCallerProc->mId, pNewThread->mGlobalIx);
    pNewThread->mState = KernThreadState_InScheduler;
    KernObj_CreateRef(&pNewThread->Running_SelfRef, &pNewThread->Hdr);
    KernSched_Locked_MakeThreadRun(pNewThread);
}

void 
KernSched_Locked_Thread_SysCall_ProcessCreate(
    K2OSKERN_OBJ_THREAD *   apCallerThread,
    K2OSKERN_OBJ_PROCESS *  apNewProc,
    K2OSKERN_OBJ_THREAD *   apNewThread
)
{
    BOOL disp;

    K2_ASSERT(apNewThread->mState == KernThreadState_Created);
    K2_ASSERT(apNewProc->mState = KernProcState_Launching);

    apNewProc->mState = KernProcState_Starting;

    disp = K2OSKERN_SeqLock(&apNewProc->Thread.SeqLock);
    K2LIST_Remove(&apNewProc->Thread.Locked.CreateList, &apNewThread->ProcThreadListLink);
    K2LIST_AddAtTail(&apNewProc->Thread.SchedLocked.ActiveList, &apNewThread->ProcThreadListLink);
    K2OSKERN_SeqUnlock(&apNewProc->Thread.SeqLock, disp);

    //
    // first resume caller thread so it gets first crack at the core it just
    // was running on
    //
    apCallerThread->mSysCall_Result = apCallerThread->SchedItem.Args.Process_Create.mProcessToken;
    apCallerThread->mpKernRwViewOfUserThreadPage->mSysCall_Arg7_Result0 = apNewProc->mId;
    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
    KernSched_Locked_MakeThreadRun(apCallerThread);

    //
    // now resume new thread
    //
    KTRACE(gData.Sched.mpSchedulingCore, 3, KTRACE_PROC_START, apNewThread->ProcRef.Ptr.AsProc->mId, apNewThread->mGlobalIx);
    KernObj_CreateRef(&apNewThread->Running_SelfRef, &apNewThread->Hdr);
    apNewThread->mState = KernThreadState_InScheduler;
    KernSched_Locked_MakeThreadRun(apNewThread);
}

void
KernSched_Locked_Thread_SysCall_SetAffinity(
    K2OSKERN_OBJ_THREAD *   apCallerThread,
    UINT32                  aNewMask
)
{
    apCallerThread->UserConfig.mAffinityMask = (UINT8)(aNewMask & 0xFF);
}

void
KernSched_Locked_Release_Wait_References(
    K2OSKERN_MACROWAIT *    apMacroWait
)
{
    UINT_PTR                ixObj;
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
    K2OSKERN_WAITENTRY * apWaitEntry
)
{
    K2OSKERN_OBJ_HEADER *pHdr;

    pHdr = apWaitEntry->ObjRef.Ptr.AsHdr;

    switch (pHdr->mObjType)
    {
    case KernObj_Process:
    case KernObj_Thread:
    case KernObj_Gate:
        break;

    case KernObj_Notify:
        KernSched_Locked_NotifyReleasedThread((K2OSKERN_OBJ_NOTIFY *)pHdr);
        break;

    case KernObj_Mailbox:
        KernSched_Locked_NotifyReleasedThread(((K2OSKERN_OBJ_MAILBOX *)pHdr)->Common.NotifyRef.Ptr.AsNotify);
        break;

    case KernObj_SemUser:
        KernSched_Locked_SemReleasedThread(((K2OSKERN_OBJ_SEMUSER *)pHdr)->SemRef.Ptr.AsSem);
        ((K2OSKERN_OBJ_SEMUSER *)pHdr)->SchedLocked.mHeldCount++;
        break;

    default:
//        K2OSKERN_Debug("EarlySatisfyWaitEntry(type %d)\n", pHdr->mObjType);
        K2_ASSERT(0);
        break;
    }
}

BOOL
KernSched_Locked_Thread_SysCall_Wait(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OSKERN_MACROWAIT *    pMacroWait;
    K2OSKERN_WAITENTRY *    pWaitEntry;
    K2OSKERN_OBJ_HEADER *   pHdr;
    UINT_PTR                ixObj;
    UINT_PTR                countSatisfied;
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
            pHdr = pWaitEntry->ObjRef.Ptr.AsHdr;

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

            case KernObj_Mailbox:
                K2_ASSERT(ixObj == (pMacroWait->mNumEntries - 1));
                if (TRUE == ((K2OSKERN_OBJ_MAILBOX *)pHdr)->Common.SchedOnlyCanSet.mIsClosed)
                {
                    waitFailed = TRUE;
                    apCallerThread->mSysCall_Result = K2OS_THREAD_WAIT_FAILED_0 + ixObj;
                    apCallerThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_CLOSED;
                }
                else
                {
                    if (((K2OSKERN_OBJ_MAILBOX *)pHdr)->Common.NotifyRef.Ptr.AsNotify->SchedLocked.mSignalled)
                    {
                        if (!pMacroWait->mIsWaitAll)
                            waitSatisfied = TRUE;
                        countSatisfied++;
                    }
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
                if (((K2OSKERN_OBJ_SEMUSER *)pHdr)->SemRef.Ptr.AsSem->SchedLocked.mCount > 0)
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
                K2_ASSERT(0);
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
                    KernSched_Locked_EarlySatisfyWaitEntry(pWaitEntry);
                    apCallerThread->mSysCall_Result = K2OS_THREAD_WAIT_SIGNALLED_0 + pWaitEntry->mMacroIndex;
                }
                else
                {
                    // all entries together satisfy the wait
                    pWaitEntry = &pMacroWait->WaitEntry[0];
                    for (ixObj = 0; ixObj < pMacroWait->mNumEntries; ixObj++)
                    {
                        KernSched_Locked_EarlySatisfyWaitEntry(pWaitEntry);
                        pWaitEntry++;
                    }
                    apCallerThread->mSysCall_Result = K2OS_THREAD_WAIT_SIGNALLED_0;
                }
            }

            KernSched_Locked_Release_Wait_References(pMacroWait);

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
        apCallerThread->mSysCall_Result = K2STAT_ERROR_TIMEOUT;
        apCallerThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_TIMEOUT;

        KernSched_Locked_Release_Wait_References(pMacroWait);

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

    pAlarm = apCallerThread->SchedItem.ObjRef.Ptr.AsAlarm;

    K2_ASSERT(0 != pAlarm->SchedLocked.mHfTicks);

    pAlarm->SchedLocked.TimerItem.mIsMacroWait = FALSE;
    pAlarm->SchedLocked.TimerItem.mHfTicks = pAlarm->SchedLocked.mHfTicks;
    KernSched_Locked_InsertTimerItem(&pAlarm->SchedLocked.TimerItem);
    pAlarm->SchedLocked.mTimerActive = TRUE;

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);

    apCallerThread->mSysCall_Result = apCallerThread->SchedItem.Args.Alarm_Create.mAlarmToken;
}

void
KernSched_Locked_Sem_Inc(
    K2OSKERN_OBJ_SEM *  apSem,
    UINT_PTR            aRelCount
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
                K2_ASSERT(pWaitEntry->ObjRef.Ptr.AsHdr->mObjType == KernObj_SemUser);

                pWaitEntry->ObjRef.Ptr.AsSemUser->SchedLocked.mHeldCount++;

                pWaitingThread->mSysCall_Result = K2OS_THREAD_WAIT_SIGNALLED_0 + pWaitEntry->mMacroIndex;

                K2_ASSERT(KernThreadState_Waiting == pWaitingThread->mState);

                KernSched_Locked_DismountWaitAndReleaseRefs(pMacroWait);

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
    UINT_PTR                relCount;

    pSemUser = apCallerThread->SchedItem.ObjRef.Ptr.AsSemUser;

    relCount = apCallerThread->SchedItem.Args.Sem_Inc.mCount;
    K2_ASSERT(relCount > 0);

    pSem = pSemUser->SemRef.Ptr.AsSem;

    if ((relCount > pSem->mMaxCount) ||
        ((pSem->mMaxCount - pSem->SchedLocked.mCount) < relCount) ||
        (pSemUser->SchedLocked.mHeldCount < relCount))
    {
        apCallerThread->mSysCall_Result = FALSE;
        apCallerThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        apCallerThread->mSysCall_Result = TRUE;
        KernSched_Locked_Sem_Inc(
            pSemUser->SemRef.Ptr.AsSem,
            apCallerThread->SchedItem.Args.Sem_Inc.mCount
        );
    }

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
}

void
KernSched_Locked_Thread_SysCall_IpcAccept(
    K2OSKERN_OBJ_THREAD *apCallerThread
)
{
    K2OSKERN_OBJ_IPCEND *   pLocal;
    K2OSKERN_OBJ_IPCEND *   pRemote;
    BOOL                    disp;
    BOOL                    connected;
    K2OS_MSG                connectMsg;
    K2OSKERN_IPC_REQUEST *  pReqClean;

    pLocal = apCallerThread->SchedItem.ObjRef.Ptr.AsIpcEnd;
    pRemote = apCallerThread->SchedItem.Args.Ipc_Accept.RemoteEndRef.Ptr.AsIpcEnd;
    K2_ASSERT(pLocal != pRemote);

    connected = FALSE;
    pReqClean = NULL;

    if (pRemote->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc->mState < KernProcState_Stopping)
    {
        disp = K2OSKERN_SeqLock(&pLocal->Connection.SeqLock);
        if (pLocal->Connection.Locked.mState == KernIpcEndState_Disconnected)
        {
            K2OSKERN_SeqLock(&pRemote->Connection.SeqLock);
            if (pRemote->Connection.Locked.mState == KernIpcEndState_Disconnected)
            {
                K2OSKERN_SeqLock(&gData.Iface.SeqLock);
                if ((NULL == pRemote->Connection.Locked.mpOpenRequest) ||
                    (pRemote->Connection.Locked.mpOpenRequest->mPending != (UINT_PTR)pLocal))
                {
                    apCallerThread->mSysCall_Result = 0;
                    apCallerThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_ABANDONED;
                }
                else
                {
                    //
                    // connect them
                    //
                    connected = TRUE;
                    pLocal->Connection.Locked.mState = KernIpcEndState_Connected;
                    KernObj_CreateRef(&pLocal->Connection.Locked.Connected.PartnerIpcEndRef, &pRemote->Hdr);
                    KernObj_CreateRef(&pLocal->Connection.Locked.Connected.WriteMapRef, apCallerThread->SchedItem.Args.Ipc_Accept.LocalMapOfRemoteBufferRef.Ptr.AsHdr);

                    pRemote->Connection.Locked.mState = KernIpcEndState_Connected;
                    KernObj_CreateRef(&pRemote->Connection.Locked.Connected.PartnerIpcEndRef, &pLocal->Hdr);
                    KernObj_CreateRef(&pRemote->Connection.Locked.Connected.WriteMapRef, apCallerThread->SchedItem.Args.Ipc_Accept.RemoteMapOfLocalBufferRef.Ptr.AsHdr);
                    apCallerThread->mSysCall_Result = TRUE;

                    //
                    // eat the request
                    //
                    pReqClean = pRemote->Connection.Locked.mpOpenRequest;
                    pReqClean->mpRequestor = NULL;
                    K2TREE_Remove(&gData.Iface.Locked.RequestTree, &pReqClean->GlobalTreeNode);
                    pRemote->Connection.Locked.mpOpenRequest = NULL;
                }
                K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, FALSE);
            }
            else
            {
                apCallerThread->mSysCall_Result = 0;
                apCallerThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_IN_USE;
            }
            K2OSKERN_SeqUnlock(&pRemote->Connection.SeqLock, FALSE);
        }
        else
        {
            apCallerThread->mSysCall_Result = 0;
            apCallerThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_CONNECTED;
        }
        K2OSKERN_SeqUnlock(&pLocal->Connection.SeqLock, disp);
    }
    else
    {
        apCallerThread->mSysCall_Result = 0;
        apCallerThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_PROCESS_EXITED;
    }

    if (connected)
    {
        //
        // send connection messages to the two Mailboxs for the ipc endpoints
        //
        connectMsg.mControl = K2OS_SYSTEM_MSG_IPC_CONNECTED;
        connectMsg.mPayload[0] = (UINT32)pLocal->mpUserKey;
        connectMsg.mPayload[1] = (pRemote->mMaxMsgCount << 16) | pRemote->mMaxMsgBytes;
        connectMsg.mPayload[2] = (UINT32)apCallerThread->SchedItem.Args.Ipc_Accept.mTokLocalMapOfRemoteBuffer;
        KernSched_Locked_MailboxSendFromReserve(&pLocal->MailboxRef.Ptr.AsMailbox->Common, &connectMsg, NULL);

        connectMsg.mPayload[0] = (UINT32)pRemote->mpUserKey;
        connectMsg.mPayload[1] = (pLocal->mMaxMsgCount << 16) | pLocal->mMaxMsgBytes;
        connectMsg.mPayload[2] = (UINT32)apCallerThread->SchedItem.Args.Ipc_Accept.mTokRemoteMapOfLocalBuffer;
        KernSched_Locked_MailboxSendFromReserve(&pRemote->MailboxRef.Ptr.AsMailbox->Common, &connectMsg, NULL);

        K2_ASSERT(NULL != pReqClean);
        KernHeap_Free(pReqClean);
    }
    else
    {
        KernProc_TokenDestroy(
            pLocal->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc,
            apCallerThread->SchedItem.Args.Ipc_Accept.mTokLocalMapOfRemoteBuffer
        );

        KernProc_TokenDestroy(
            pRemote->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc, 
            apCallerThread->SchedItem.Args.Ipc_Accept.mTokRemoteMapOfLocalBuffer
        );
    }

    KernObj_ReleaseRef(&apCallerThread->SchedItem.Args.Ipc_Accept.RemoteMapOfLocalBufferRef);
    KernObj_ReleaseRef(&apCallerThread->SchedItem.Args.Ipc_Accept.LocalMapOfRemoteBufferRef);

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
    KernObj_ReleaseRef(&apCallerThread->SchedItem.Args.Ipc_Accept.RemoteEndRef);
}

void
KernSched_Locked_Thread_SysCall_IpcReject(
    K2OSKERN_OBJ_THREAD *apCallerThread
)
{
    K2OSKERN_OBJ_IPCEND *   pRemote;
    BOOL                    disp;
    BOOL                    rejected;
    K2OS_MSG                rejectMsg;
    K2OSKERN_IPC_REQUEST *  pReqClean;

    pRemote = apCallerThread->SchedItem.ObjRef.Ptr.AsIpcEnd;

    rejected = FALSE;
    pReqClean = NULL;

    if (pRemote->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc->mState < KernProcState_Stopping)
    {
        K2OSKERN_SeqLock(&pRemote->Connection.SeqLock);
        if (pRemote->Connection.Locked.mState == KernIpcEndState_Disconnected)
        {
            disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

            if ((NULL == pRemote->Connection.Locked.mpOpenRequest) ||
                (pRemote->Connection.Locked.mpOpenRequest->mPending != (UINT_PTR)0xFFFFFFFF))
            {
                apCallerThread->mSysCall_Result = 0;
                apCallerThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_ABANDONED;
            }
            else
            {
                //
                // 'successfully' reject the request
                // 
                rejected = TRUE;
                apCallerThread->mSysCall_Result = TRUE;

                //
                // eat the request
                //
                pReqClean = pRemote->Connection.Locked.mpOpenRequest;
                pReqClean->mpRequestor = NULL;
                K2TREE_Remove(&gData.Iface.Locked.RequestTree, &pReqClean->GlobalTreeNode);
                pRemote->Connection.Locked.mpOpenRequest = NULL;
            }

            K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);
        }
        else
        {
            apCallerThread->mSysCall_Result = 0;
            apCallerThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_IN_USE;
        }
        K2OSKERN_SeqUnlock(&pRemote->Connection.SeqLock, FALSE);
    }
    else
    {
        apCallerThread->mSysCall_Result = 0;
        apCallerThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_PROCESS_EXITED;
    }

    if (rejected)
    {
        //
        // send rejection message to the requestors mailbox.
        // sender never knows its own request id so don't bother sending it
        //
        rejectMsg.mControl = K2OS_SYSTEM_MSG_IPC_REJECTED;
        rejectMsg.mPayload[0] = (UINT32)pRemote->mpUserKey;
        rejectMsg.mPayload[1] = apCallerThread->SchedItem.Args.Ipc_Reject.mReasonCode;
        rejectMsg.mPayload[2] = 0;
        KernSched_Locked_MailboxSendFromReserve(&pRemote->MailboxRef.Ptr.AsMailbox->Common, &rejectMsg, NULL);

        K2_ASSERT(NULL != pReqClean);
        KernHeap_Free(pReqClean);
    }

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
}

void
KernSched_Locked_Thread_SysCall_IpcManualDisconnect(
    K2OSKERN_OBJ_THREAD *apCallerThread
)
{
    K2OSKERN_OBJ_IPCEND * pIpcEnd;

    pIpcEnd = apCallerThread->SchedItem.ObjRef.Ptr.AsIpcEnd;

    if (!KernSched_Locked_DisconnectIpcEnd(pIpcEnd))
    {
        //
        // already disconnected
        //
        if (apCallerThread->mSysCall_Id == K2OS_SYSCALL_ID_IPCEND_MANUAL_DISCONNECT)
        {
            apCallerThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_NOT_CONNECTED;
            apCallerThread->mSysCall_Result = FALSE;
        }
        else
        {
            //
            // its a close call. we're here so it succeeds
            //
            apCallerThread->mSysCall_Result = TRUE;
        }
    }
    else
    {
        apCallerThread->mSysCall_Result = TRUE;
    }

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);

    if (apCallerThread->SchedItem.Args.Ipc_Manual_Disconnect.mChainClose)
    {
        K2LIST_AddAtTail(&gData.Sched.Locked.DefferedResumeList, &apCallerThread->SchedListLink);
        apCallerThread->mState = KernThreadState_InScheduler_ResumeDeferred;
        KernObj_CreateRef(&pIpcEnd->CloserThreadRef, &apCallerThread->Hdr);
        pIpcEnd->mHoldTokenOnDpcClose = (K2OS_TOKEN)apCallerThread->mSysCall_Arg0;
        pIpcEnd->Hdr.ObjDpc.Func = KernIpcEnd_CloseDpc;
        KernCpu_QueueDpc(&pIpcEnd->Hdr.ObjDpc.Dpc, &pIpcEnd->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
        return;
    }

    KernSched_Locked_MakeThreadRun(apCallerThread);
}

void
KernSched_Locked_Thread_SysCall_IfacePublish(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_IFACE *        pScan;
    K2TREE_NODE *           pTreeNode;
    BOOL                    disp;

    pThreadPage = apCallerThread->mpKernRwViewOfUserThreadPage;

    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
    pTreeNode = K2TREE_Find(&gData.Iface.Locked.InstanceTree, apCallerThread->mSysCall_Arg0);
    if (NULL != pTreeNode)
    {
        pScan = K2_GET_CONTAINER(K2OSKERN_IFACE, pTreeNode, GlobalTreeNode);
        if (pScan->mClassCode != 0)
        {
            //
            // already published
            //
            apCallerThread->mSysCall_Result = 0;
            pThreadPage->mLastStatus = K2STAT_ERROR_ALREADY_EXISTS;
        }
        else
        {
            apCallerThread->mSysCall_Result = 1;
            K2_ASSERT(0 != pThreadPage->mSysCall_Arg1);
            pScan->mClassCode = pThreadPage->mSysCall_Arg1;
            K2MEM_Copy(&pScan->SpecificGuid, &pThreadPage->mSysCall_Arg2, sizeof(K2_GUID128));
            KernSched_Locked_IfaceLocked_InterfaceChange(pScan, TRUE, apCallerThread->ProcRef.Ptr.AsProc);
        }
    }
    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    if (NULL == pTreeNode)
    {
        apCallerThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = K2STAT_ERROR_NOT_FOUND;
    }
}

void 
KernSched_Locked_Thread_SysCall_IfaceRemove(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_IFACE *        pIface;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    BOOL                    disp;
    K2TREE_NODE *           pTreeNode;

    pProc = apCallerThread->ProcRef.Ptr.AsProc;
    
    pThreadPage = apCallerThread->mpKernRwViewOfUserThreadPage;

    apCallerThread->mSysCall_Result = (UINT32)FALSE;

    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
    pTreeNode = K2TREE_Find(&gData.Iface.Locked.InstanceTree, apCallerThread->mSysCall_Arg0);
    if (NULL != pTreeNode)
    {
        pIface = K2_GET_CONTAINER(K2OSKERN_IFACE, pTreeNode, GlobalTreeNode);
        if (pIface->mpProc != pProc)
        {
            pIface = NULL;
            pThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            KernObj_ReleaseRef(&pIface->MailboxRef);
            K2TREE_Remove(&gData.Iface.Locked.InstanceTree, &pIface->GlobalTreeNode);
            K2LIST_Remove(&pProc->Iface.GlobalLocked.List, &pIface->ProcIfaceListLink);
            apCallerThread->mSysCall_Result = (UINT32)TRUE;
            if (0 != pIface->mClassCode)
            {
                KernSched_Locked_IfaceLocked_InterfaceChange(pIface, FALSE, NULL);
            }
        }
    }
    else
    {
        pIface = NULL;
        pThreadPage->mLastStatus = K2STAT_ERROR_NOT_FOUND;
    }
    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    if (NULL != pIface)
    {
        KernHeap_Free(pIface);
    }
}

void
KernSched_Locked_Thread_SysCall_Root_StopProcess(
    K2OSKERN_OBJ_THREAD *   apCallerThread
)
{
    K2OSKERN_OBJ_PROCESS *  pProcessToStop;
    K2OS_USER_THREAD_PAGE * pThreadPage;

    //
    // only root can do this
    //
    K2_ASSERT(1 == apCallerThread->ProcRef.Ptr.AsProc->mId);

    pThreadPage = apCallerThread->mpKernRwViewOfUserThreadPage;
    pProcessToStop = apCallerThread->SchedItem.ObjRef.Ptr.AsProc;

    KernSched_Locked_StopProcess(pProcessToStop, pThreadPage->mSysCall_Arg1);

    KernObj_ReleaseRef(&apCallerThread->SchedItem.ObjRef);
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

    pCallerProc = pCallerThread->ProcRef.Ptr.AsProc;
    
    KTRACE(gData.Sched.mpSchedulingCore, 4, KTRACE_SCHED_EXEC_SYSCALL, pCallerProc->mId, pCallerThread->mGlobalIx, pCallerThread->mSysCall_Id);

    procIsAlive = (pCallerProc->mState < KernProcState_Stopping) ? TRUE : FALSE;

    switch (pCallerThread->mSysCall_Id)
    {
    // any of these signal a notify which may unblock a thread
    case K2OS_SYSCALL_ID_NOTIFY_SIGNAL:
    case K2OS_SYSCALL_ID_MAILSLOT_SEND:
    case K2OS_SYSCALL_ID_IPCEND_CREATE:
    case K2OS_SYSCALL_ID_IPCEND_SEND:
    case K2OS_SYSCALL_ID_IPCEND_REQUEST:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr != NULL);
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr->mObjType == KernObj_Notify);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_SignalNotify(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_GATE_CHANGE:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr != NULL);
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr->mObjType == KernObj_Gate);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_ChangeGate(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_MAILBOX_CLOSE:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr != NULL);
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr->mObjType == KernObj_Mailbox);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_MailboxClose(pCallerThread, &pCallerThread->SchedItem.ObjRef.Ptr.AsMailbox->Common);
        // thread scheduling deferral requires us to return here not break
        return;

    case K2OS_SYSCALL_ID_PROCESS_EXIT:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr == NULL);
        if (!procIsAlive)
        {
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_ProcessExit(pCallerThread);
        // impossible to fail - return not break
        return;

    case K2OS_SYSCALL_ID_THREAD_EXIT:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr == NULL);
        KernSched_Locked_Thread_SysCall_ThreadExit(pCallerThread);
        // impossible to fail - return not break
        return;

    case K2OS_SYSCALL_ID_THREAD_CREATE:
    case K2OS_SYSCALL_ID_ROOT_PROCESS_LAUNCH:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr != NULL);
        if (pCallerThread->mSysCall_Id == K2OS_SYSCALL_ID_ROOT_PROCESS_LAUNCH)
        {
            K2_ASSERT(apItem->ObjRef.Ptr.AsHdr->mObjType == KernObj_Process);
            K2_ASSERT(NULL != apItem->ObjRef.Ptr.AsProc->Thread.Locked.CreateList.mpHead);
            pNewThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem->ObjRef.Ptr.AsProc->Thread.Locked.CreateList.mpHead, ProcThreadListLink);
        }
        else
        {
            K2_ASSERT(apItem->ObjRef.Ptr.AsHdr->mObjType == KernObj_Thread);
            pNewThread = apItem->ObjRef.Ptr.AsThread;
        }
        if (!procIsAlive)
        {
            K2_ASSERT(pNewThread->mState == KernThreadState_Created);
            pNewThread->mState = KernThreadState_Exited;
            pNewThread->mExitCode = pCallerProc->mExitCode;
            disp = K2OSKERN_SeqLock(&pCallerProc->Thread.SeqLock);
            K2LIST_Remove(&pCallerProc->Thread.Locked.CreateList, &pNewThread->ProcThreadListLink);
            K2LIST_AddAtTail(&pCallerProc->Thread.Locked.DeadList, &pNewThread->ProcThreadListLink);
            K2OSKERN_SeqUnlock(&pCallerProc->Thread.SeqLock, disp);
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        if (pCallerThread->mSysCall_Id == K2OS_SYSCALL_ID_ROOT_PROCESS_LAUNCH)
            KernSched_Locked_Thread_SysCall_ProcessCreate(pCallerThread, apItem->ObjRef.Ptr.AsProc, pNewThread);
        else
            KernSched_Locked_Thread_SysCall_ThreadCreate(pCallerThread);
        // thread scheduling order requires us to return here not break
        return;

    case K2OS_SYSCALL_ID_THREAD_WAIT:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr == NULL);
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
        if (KernSched_Locked_Thread_SysCall_Wait(pCallerThread))
        {
            // succeeded to wait
            return;
        }
        // failed to wait
        break;

    case K2OS_SYSCALL_ID_THREAD_SETAFF:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr == NULL);
        if (!procIsAlive)
        {
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_SetAffinity(pCallerThread, apItem->Args.Thread_SetAff.mNewMask);
        break;

    case K2OS_SYSCALL_ID_IPCEND_ACCEPT:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr != NULL);
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr->mObjType == KernObj_IpcEnd);
        K2_ASSERT(apItem->Args.Ipc_Accept.RemoteEndRef.Ptr.AsHdr != NULL);
        K2_ASSERT(apItem->Args.Ipc_Accept.RemoteEndRef.Ptr.AsHdr->mObjType == KernObj_IpcEnd);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernObj_ReleaseRef(&apItem->Args.Ipc_Accept.RemoteEndRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_IpcAccept(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_IPCREQ_REJECT:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr != NULL);
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr->mObjType == KernObj_IpcEnd);
        K2_ASSERT(apItem->Args.Ipc_Reject.mRequestId != 0);

        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_IpcReject(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_IPCEND_CLOSE:
    case K2OS_SYSCALL_ID_IPCEND_MANUAL_DISCONNECT:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr != NULL);
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr->mObjType == KernObj_IpcEnd);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
        }
        else
        {
            KernSched_Locked_Thread_SysCall_IpcManualDisconnect(pCallerThread);
        }
        return;

    case K2OS_SYSCALL_ID_ALARM_CREATE:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr != NULL);
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr->mObjType == KernObj_Alarm);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_AlarmCreate(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_SEM_INC:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr != NULL);
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr->mObjType == KernObj_SemUser);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_SemUser_SemInc(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_IFINST_PUBLISH:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr == NULL);
        if (!procIsAlive)
        {
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_IfacePublish(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_IFINST_REMOVE:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr == NULL);
        if (!procIsAlive)
        {
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_IfaceRemove(pCallerThread);
        break;

    case K2OS_SYSCALL_ID_ROOT_PROCESS_STOP:
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr != NULL);
        K2_ASSERT(apItem->ObjRef.Ptr.AsHdr->mObjType == KernObj_Process);
        if (!procIsAlive)
        {
            KernObj_ReleaseRef(&apItem->ObjRef);
            KernSched_Locked_ExitThread(pCallerThread, pCallerProc->mExitCode);
            return;
        }
        KernSched_Locked_Thread_SysCall_Root_StopProcess(pCallerThread);
        break;

    default:
        K2OSKERN_Panic("KernSched_Locked_Thread_SysCall - hit unsupMailboxed call (%d)\n", pCallerThread->mSysCall_Id);
        break;
    }

    K2_ASSERT(apItem->ObjRef.Ptr.AsHdr == NULL);

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
//    K2OSKERN_Debug("Sched: Proc %d Thread %d ABORT\n", pAbortThread->ProcRef.Ptr.AsProc->mId, pAbortThread->mGlobalIx);
    KernSched_Locked_ExitThread(pAbortThread, K2STAT_ERROR_ABORTED);
}

void
KernSched_Locked_SignalProxy(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_OBJ_SIGNALPROXY *pProxy;

    pProxy = K2_GET_CONTAINER(K2OSKERN_OBJ_SIGNALPROXY, apItem, SchedItem);
    K2_ASSERT(pProxy->InFlightSelfRef.Ptr.AsSignalProxy == pProxy);

    K2_ASSERT(apItem->ObjRef.Ptr.AsHdr != NULL);
    K2_ASSERT(apItem->ObjRef.Ptr.AsHdr->mObjType == KernObj_Notify);

    KernSched_Locked_SignalNotify(apItem->ObjRef.Ptr.AsNotify);

    KernObj_ReleaseRef(&apItem->ObjRef);

    KernObj_ReleaseRef(&pProxy->InFlightSelfRef);
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

    pCrashProc = pCrashThread->ProcRef.Ptr.AsProc;
    K2_ASSERT(apItem->ObjRef.Ptr.AsHdr == NULL);

//    K2OSKERN_Debug(
//        "Sched: Unhandled Thread %d exception crashes process %d\n", 
//        pCrashThread->mGlobalIx, 
//        pCrashProc->mId
//    );

    KernSched_Locked_ExitThread(pCrashThread, pCrashThread->LastEx.mExCode);

    KernSched_Locked_StopProcess(pCrashProc, pCrashThread->LastEx.mExCode);
}

void
KernSched_Locked_ResumeDeferral_Completed(
    K2OSKERN_SCHED_ITEM *   apItem
)
{
    K2OSKERN_OBJ_THREAD *   pThread;

    pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, apItem, SchedItem);

    K2_ASSERT(pThread->Hdr.mObjType == KernObj_Thread);
    K2_ASSERT(pThread->SchedItem.ObjRef.Ptr.AsThread == pThread);

    if (pThread->mState == KernThreadState_Exited)
    {
        //
        // thread died during resume deferral (proc exit/crash)
        // shouldn't need to do anything here
        //
//        K2_ASSERT(0);
    }
    else
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

    KernSched_Locked_Sem_Inc(pSemUser->SemRef.Ptr.AsSem, pSemUser->SchedLocked.mHeldCount);
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

    KernSched_Locked_GateChange(pIntr->GateRef.Ptr.AsGate, TRUE);
    KernSched_Locked_GateChange(pIntr->GateRef.Ptr.AsGate, FALSE);

    KernObj_ReleaseRef(&pIntr->SchedItem.ObjRef);
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

    case KernSchedItem_SignalProxy:
        KernSched_Locked_SignalProxy(apItem);
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

    case KernSchedItem_ProcStopped:
        KernSched_Locked_ProcStopped(apItem);
        break;

    default:
        K2OSKERN_Panic("KernSched_Locked_ExecOneItem - unknown item type (%d)\n", itemType);
        break;
    }
}

