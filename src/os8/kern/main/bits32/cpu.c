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
KernCpu_AtXdlEntry(
    void
)
{
    UINT32                      coreIx;
    K2OSKERN_CPUCORE volatile * pCore;

    K2_ASSERT(gData.mCpuCoreCount > 0);
    K2_ASSERT(gData.mCpuCoreCount <= K2OS_MAX_CPU_COUNT);

    for (coreIx = 0; coreIx < gData.mCpuCoreCount; coreIx++)
    {
        pCore = K2OSKERN_COREIX_TO_CPUCORE(coreIx);
        
        K2MEM_Zero((void *)pCore, sizeof(K2OSKERN_CPUCORE));
        
        pCore->mCoreIx = coreIx;

        K2LIST_Init((K2LIST_ANCHOR *)&pCore->PendingEventList);

        K2LIST_Init((K2LIST_ANCHOR *)&pCore->DpcHi);
        K2LIST_Init((K2LIST_ANCHOR *)&pCore->DpcMed);
        K2LIST_Init((K2LIST_ANCHOR *)&pCore->DpcLo);

        K2LIST_Init((K2LIST_ANCHOR *)&pCore->MigratedList);
        K2LIST_Init((K2LIST_ANCHOR *)&pCore->RunList);
        K2LIST_Init((K2LIST_ANCHOR *)&pCore->RanList);
    }

    K2_CpuWriteBarrier();
}

void
KernCpu_ProcessOneEvent(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_HEADER *       apEventObj,
    KernCpuCoreEventType        aEventType,
    UINT32                      aSrcCoreIx,
    UINT64 *                    apEventTick
)
{
    switch (aEventType)
    {
    case KernCpuCoreEvent_Thread_SysCall:
        KernThread_CpuEvent_SysCall(apThisCore, (K2OSKERN_OBJ_THREAD *)apEventObj);
        break;
    case KernCpuCoreEvent_Thread_Exception:
        KernThread_CpuEvent_Exception(apThisCore, (K2OSKERN_OBJ_THREAD *)apEventObj);
        break;
    case KernCpuCoreEvent_SchedTimer_Fired:
        KernSched_CpuEvent_TimerFired(apThisCore, apEventTick);
        break;
    case KernCpuCoreEvent_Ici:
        KernCpu_CpuEvent_RecvIci(apThisCore, apEventTick, aSrcCoreIx);
        break;
    case KernCpuCoreEvent_Interrupt:
        KernIntr_CpuEvent_Fired(apThisCore, (K2OSKERN_OBJ_INTERRUPT *)apEventObj, apEventTick);
        break;
    default: 
        K2OSKERN_Panic("Unknown CpuCore Event(%d)\n", aEventType);
        break;
    }
}

void    
KernCpu_DrainEvents(
    K2OSKERN_CPUCORE volatile *apThisCore
)
{
    K2OSKERN_CPUCORE_EVENT * pEvent;

    KernCpuCoreEventType    eventType;
    UINT32                  srcCoreIx;
    UINT64                  evtTick;
    K2OSKERN_OBJ_HEADER *   pEventObj;
    BOOL                    disp;
    K2LIST_ANCHOR *         pList;

    pList = (K2LIST_ANCHOR *)&apThisCore->PendingEventList;
    if (*((UINT32 volatile *)(&pList->mNodeCount)) == 0)
        return;

    disp = K2OSKERN_SetIntr(FALSE);
    do
    {
        pEvent = K2_GET_CONTAINER(K2OSKERN_CPUCORE_EVENT, pList->mpHead, ListLink);
        K2LIST_Remove(pList, &pEvent->ListLink);
        K2OSKERN_SetIntr(disp);

        eventType = pEvent->mEventType;
        if ((eventType >= KernCpuCoreEvent_Thread_SysCall) &&
            (eventType <= KernCpuCoreEvent_Thread_Exception))
            pEventObj = &(K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pEvent, CpuCoreEvent))->Hdr;
        else if (eventType == KernCpuCoreEvent_Interrupt)
            pEventObj = &(K2_GET_CONTAINER(K2OSKERN_OBJ_INTERRUPT, pEvent, Event))->Hdr;
        else
            pEventObj = NULL;
        srcCoreIx = pEvent->mSrcCoreIx;
        evtTick = pEvent->mEventHfTick;

        //
        // clear the event at its source
        //
        pEvent->mEventType = KernCpuCoreEvent_None;
        K2_CpuWriteBarrier();

        KernCpu_ProcessOneEvent(apThisCore, pEventObj, eventType, srcCoreIx, &evtTick);

        disp = K2OSKERN_SetIntr(FALSE);

    } while (pList->mNodeCount != 0);

    K2OSKERN_SetIntr(disp);
}
    
BOOL    
KernCpu_ExecOneDpc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    KernDpcPrioType             aPrio
)
{
    K2LIST_ANCHOR *     pList;
    BOOL                disp;
    K2OSKERN_DPC *      pDpc;
    K2OSKERN_pf_DPC *   pKey;

    K2_ASSERT(aPrio > KernDpcPrio_Invalid);
    K2_ASSERT(aPrio < KernDpcPrio_Count);

    switch (aPrio)
    {
    case KernDpcPrio_Lo:
        pList = (K2LIST_ANCHOR *)&apThisCore->DpcLo;
        break;
    case KernDpcPrio_Med:
        pList = (K2LIST_ANCHOR *)&apThisCore->DpcMed;
        break;
    case KernDpcPrio_Hi:
        pList = (K2LIST_ANCHOR *)&apThisCore->DpcHi;
        break;
    default:
        K2OSKERN_Panic("KernCpu_ExecOneDpc unknown Dpc Priority(%d)\n", aPrio);
        break;
    }

    //
    // return TRUE if we do anything
    //

    if (0 == pList->mNodeCount)
        return FALSE;

    disp = K2OSKERN_SetIntr(FALSE);
    pDpc = K2_GET_CONTAINER(K2OSKERN_DPC, pList->mpHead, DpcListLink);
    K2LIST_Remove(pList, &pDpc->DpcListLink);
    K2OSKERN_SetIntr(disp);

    pKey = pDpc->mpKey;
    pDpc->mpKey = NULL;
    K2_CpuWriteBarrier();

    (*pKey)(apThisCore, pKey);

    return TRUE;
}

void
KernCpu_QueueEvent(
    K2OSKERN_CPUCORE_EVENT * apEvent
)
{
    K2OSKERN_CPUCORE volatile *pThisCore;
    K2_ASSERT(FALSE == K2OSKERN_GetIntr());
    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    K2LIST_AddAtTail((K2LIST_ANCHOR *)&pThisCore->PendingEventList, &apEvent->ListLink);
}

void
KernCpu_QueueDpc(
    K2OSKERN_DPC *      apDpc,
    K2OSKERN_pf_DPC *   apKey,
    KernDpcPrioType     aPrio
)
{
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2LIST_ANCHOR *             pList;
    BOOL                        disp;

    K2_ASSERT(NULL != apKey);
    K2_ASSERT(NULL != (*apKey));
    K2_ASSERT(aPrio > KernDpcPrio_Invalid);
    K2_ASSERT(aPrio < KernDpcPrio_Count);

    //
    // can only ever queue to current core.  
    // that why cpu ptr is not an argument
    // that is why a lock is never needed
    //
    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;

    apDpc->mpKey = apKey;

    switch (aPrio)
    {
    case KernDpcPrio_Lo:
        pList = (K2LIST_ANCHOR *)&pThisCore->DpcLo;
        break;
    case KernDpcPrio_Med:
        pList = (K2LIST_ANCHOR *)&pThisCore->DpcMed;
        break;
    case KernDpcPrio_Hi:
        pList = (K2LIST_ANCHOR *)&pThisCore->DpcHi;
        break;
    default:
        K2OSKERN_Panic("KernCpu_QueueDpc unknown Dpc Priority(%d)\n", aPrio);
    }

    disp = K2OSKERN_SetIntr(FALSE);

    K2LIST_AddAtTail(pList, &apDpc->DpcListLink);

    K2OSKERN_SetIntr(disp);
}

void
KernCpu_TlbInvalidate(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_TLBSHOOT *         apShoot
)
{
    UINT32                  virtWork;
    UINT32                  pagesLeft;
    K2OSKERN_OBJ_PROCESS *  pProc;

    pProc = apShoot->mpProc;

    if (pProc != NULL)
    {
        if (pProc != apThisCore->MappedProcRef.Ptr.AsProc)
        {
            K2ATOMIC_And(&apShoot->mCoresRemaining, ~(1 << apThisCore->mCoreIx));
            return;
        }
        KTRACE(apThisCore, 2, KTRACE_PROC_TLBSHOOT_ICI, pProc->mId);
    }
    else
    {
        KTRACE(apThisCore, 1, KTRACE_KERN_TLBSHOOT_ICI);
    }

    pagesLeft = apShoot->mPageCount;
    virtWork = apShoot->mVirtBase;
    do
    {
        KernArch_InvalidateTlbPageOnCurrentCore(virtWork);
        virtWork += K2_VA_MEMPAGE_BYTES;
    } while (--pagesLeft);

    K2ATOMIC_And(&apShoot->mCoresRemaining, ~(1 << apThisCore->mCoreIx));
}

void
KernCpu_AbortListThreadsFromProc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_PROCESS *      apProc,
    K2LIST_ANCHOR *             apList
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_SCHED_ITEM *   pSchedItem;
    K2LIST_LINK *           pListLink;

    if (apList->mNodeCount == 0)
        return;

    pListLink = apList->mpHead;
    do
    {
        pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListLink, CpuCoreThreadListLink);
        pListLink = pListLink->mpNext;

        if (pThread->ProcRef.Ptr.AsProc == apProc)
        {
            //
            // terminate the thread
            //
            KTRACE(apThisCore, 3, KTRACE_THREAD_STOPPED, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);
            K2LIST_Remove(apList, &pThread->CpuCoreThreadListLink);
            K2ATOMIC_Dec(&gData.Sched.mCoreThreadCount[apThisCore->mCoreIx]);

            pSchedItem = &pThread->SchedItem;
            pSchedItem->mType = KernSchedItem_Aborted_Running_Thread;
            pThread->mState = KernThreadState_InScheduler;
            KernTimer_GetHfTick(&pSchedItem->mHfTick);
            KernSched_QueueItem(pSchedItem);
        }
    } while (pListLink != NULL);
}

void
KernCpu_StopProc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_PROCESS *      apProc
)
{
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OSKERN_SCHED_ITEM *   pSchedItem;

    KTRACE(apThisCore, 2, KTRACE_CORE_STOP_PROC, apProc->mId);

    //
    // get off the specified process
    //
    KernCpu_TakeInMigratingThreads(apThisCore);
    KernCpu_AbortListThreadsFromProc(apThisCore, apProc, (K2LIST_ANCHOR *)&apThisCore->MigratedList);
    KernCpu_AbortListThreadsFromProc(apThisCore, apProc, (K2LIST_ANCHOR *)&apThisCore->RunList);
    KernCpu_AbortListThreadsFromProc(apThisCore, apProc, (K2LIST_ANCHOR *)&apThisCore->RanList);

    if (apProc == apThisCore->MappedProcRef.Ptr.AsProc)
    {
        pThread = apThisCore->mpActiveThread;
        if (NULL != pThread)
        {
            if (pThread->ProcRef.Ptr.AsProc == apProc)
            {
                //
                // terminate the active thread
                //
                KTRACE(apThisCore, 3, KTRACE_CORE_ABORT_THREAD, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);
                pSchedItem = &pThread->SchedItem;
                pSchedItem->mType = KernSchedItem_Aborted_Running_Thread;
                KernTimer_GetHfTick(&pSchedItem->mHfTick);
                KernCpu_TakeCurThreadOffThisCore(apThisCore, pThread, KernThreadState_InScheduler);
                KernSched_QueueItem(pSchedItem);
            }
        }

        KTRACE(apThisCore, 1, KTRACE_CORE_SWITCH_NOPROC);
        KernArch_SetCoreToProcess(apThisCore, NULL);
    }

    K2ATOMIC_And(&apProc->mStopCoresRemaining, ~(1 << apThisCore->mCoreIx));
}

void
KernCpu_IoPermitUpdate(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apInitiatorThread
)
{
#if K2_TARGET_ARCH_IS_INTEL
    K2_ASSERT(NULL != apInitiatorThread);
    K2_ASSERT(NULL != apInitiatorThread->IoPermitProcRef.Ptr.AsHdr);

    if ((apThisCore->MappedProcRef.Ptr.AsProc == NULL) ||
        (apThisCore->MappedProcRef.Ptr.AsProc != apInitiatorThread->IoPermitProcRef.Ptr.AsProc))
    {
        KTRACE(apThisCore, 1, KTRACE_PROC_IOPERMIT_ICI_IGNORED);
        return;
    }

    KTRACE(apThisCore, 2, KTRACE_PROC_IOPERMIT_ICI, apInitiatorThread->IoPermitProcRef.Ptr.AsProc->mId);

    //
    // update the io permit bitmap for the process
    //
    KernArch_IoPermitAdded(apThisCore, apInitiatorThread->IoPermitProcRef.Ptr.AsProc);

    K2ATOMIC_And(&apInitiatorThread->TlbShoot.mCoresRemaining, ~(1 << apThisCore->mCoreIx));
#else
    K2OSKERN_Panic("impossible\n");
#endif
}

void    
KernCpu_CpuEvent_RecvIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    UINT64 const *              apHfTick,
    UINT32                      aSrcCoreIx
)
{
    K2OSKERN_CPUCORE_ICI volatile * pIci;
    KernIciType                     iciType;
    void *                          pArg;

    //
    // Clear the ici code so another ici can be sent
    //
    pIci = &apThisCore->IciFromOtherCore[aSrcCoreIx];

    iciType = pIci->mIciType;
    pArg = pIci->mpArg;
    pIci->mpArg = NULL;
    pIci->mIciType = KernIci_None;
    K2_CpuWriteBarrier();

    KTRACE(apThisCore, 2, KTRACE_CORE_RECV_ICI, iciType);

    switch (iciType)
    {
    case KernIci_Wakeup:
        // the scheduling core woke us up
        break;
    case KernIci_TlbInv:
        KernCpu_TlbInvalidate(apThisCore, (K2OSKERN_TLBSHOOT *)pArg);
        break;
    case KernIci_StopProc:
        KernCpu_StopProc(apThisCore, (K2OSKERN_OBJ_PROCESS *)pArg);
        break;
    case KernIci_IoPermitUpdate:
        KernCpu_IoPermitUpdate(apThisCore, (K2OSKERN_OBJ_THREAD *)pArg);
        break;
    case KernIci_DebugCmd:
        KernDbg_RecvSlaveCommand(apThisCore, (K2OSKERN_DBG_COMMAND *)pArg);
        break;
    default:
        K2OSKERN_Panic("KernCpu_CpuEvent_RecvIci unknown Ici type (%d)\n", iciType);
        break;
    }
}

void    
KernCpu_TakeCurThreadOffThisCore(
    K2OSKERN_CPUCORE volatile *apThisCore,
    K2OSKERN_OBJ_THREAD *apCurThread,
    KernThreadStateType aNewState
)
{
    K2_ASSERT(apThisCore == K2OSKERN_GET_CURRENT_CPUCORE);
    K2_ASSERT(apThisCore->mpActiveThread == apCurThread);
    K2_ASSERT(apCurThread->mState == KernThreadState_Running);
    K2_ASSERT(aNewState != KernThreadState_Running);
    K2_ASSERT(apCurThread->mpLastRunCore == apThisCore);
    K2ATOMIC_Dec(&gData.Sched.mCoreThreadCount[apThisCore->mCoreIx]);
    KTRACE(apThisCore, 4, KTRACE_CORE_SUSPEND_THREAD, apCurThread->ProcRef.Ptr.AsProc->mId, apCurThread->mGlobalIx, aNewState);
    apCurThread->mState = aNewState;
    apThisCore->mpActiveThread = NULL;
}

void    
KernCpu_PanicSpin(
    K2OSKERN_CPUCORE volatile *apThisCore
)
{
    KTRACE(apThisCore, 1, KTRACE_CORE_PANIC_SPIN);
    K2ATOMIC_Inc(&gData.Debug.mCoresInPanicSpin);
    while (1);
}

void
KernCpu_TakeInMigratingThreads(
    K2OSKERN_CPUCORE volatile * apThisCore
)
{
    K2OSKERN_OBJ_THREAD *   pThreadList;
    K2OSKERN_OBJ_THREAD *   pThread;
    K2LIST_ANCHOR           newList;

    K2LIST_Init(&newList);

    do
    {
        pThreadList = (K2OSKERN_OBJ_THREAD *)K2ATOMIC_Exchange((UINT32 volatile *)&apThisCore->mpMigratingThreadList, 0);
        if (NULL == pThreadList)
            break;

        do
        {
            pThread = pThreadList;
            pThreadList = pThreadList->mpMigratingNext;

            K2_ASSERT(KernThreadState_Migrating == pThread->mState);

            K2LIST_AddAtHead(&newList, &pThread->CpuCoreThreadListLink);
            pThread->mState = KernThreadState_OnCpuLists;

        } while (NULL != pThreadList);

    } while (1);

    K2LIST_AppendToTail((K2LIST_ANCHOR *)&apThisCore->MigratedList, &newList);
}

void
KernCpu_ScheduleRunList(
    K2OSKERN_CPUCORE volatile * apThisCore
)
{
    UINT32                  nodeCount;
    UINT64                  quanta;
    K2LIST_LINK *           pListLink;
    K2OSKERN_OBJ_THREAD *   pThread;

    nodeCount = apThisCore->RunList.mNodeCount;
    if (nodeCount < 2)
        return;

    nodeCount = 100 / nodeCount;
    if (nodeCount < 10)
        nodeCount = 10;
    else if (nodeCount > 50)
        nodeCount = 50;

    quanta = nodeCount;
    KernTimer_HfTicksFromMs(&quanta, &quanta);

    pListLink = (K2LIST_LINK *)apThisCore->RunList.mpHead;
    do
    {
        pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pListLink, CpuCoreThreadListLink);
        pListLink = pListLink->mpNext;
        pThread->mQuantumHfTicksRemaining = quanta;
    } while (NULL != pListLink);
}

void    
KernCpu_Schedule(
    K2OSKERN_CPUCORE volatile *apThisCore
)
{
    K2OSKERN_OBJ_THREAD *   pThread;

    //
    // move asynchronously migrated threads onto our migrated list
    //
    KernCpu_TakeInMigratingThreads(apThisCore);

    pThread = apThisCore->mpActiveThread;
    if (NULL == pThread)
    {
        //
        // no active thread
        //
        if (0 != apThisCore->RunList.mNodeCount)
        {
            //
            // some threads left on run list in this iteration
            //
            K2LIST_AppendToTail((K2LIST_ANCHOR *)&apThisCore->RanList, (K2LIST_ANCHOR *)&apThisCore->MigratedList);
        }
        else
        {
            //
            // nothing left on run list this iteration
            //
            K2LIST_AppendToTail((K2LIST_ANCHOR *)&apThisCore->RunList, (K2LIST_ANCHOR *)&apThisCore->RanList);
            K2LIST_AppendToTail((K2LIST_ANCHOR *)&apThisCore->RunList, (K2LIST_ANCHOR *)&apThisCore->MigratedList);
            KernCpu_ScheduleRunList(apThisCore);
        }
        return;
    }

    //
    // there is a currently active thread
    //
    if (0 != apThisCore->RunList.mNodeCount)
    {
        //
        // there are threads left to run in this iteration
        //
        K2LIST_AppendToTail((K2LIST_ANCHOR *)&apThisCore->RanList, (K2LIST_ANCHOR *)&apThisCore->MigratedList);

        //
        // check to see if still some quantum left on current thread
        //
        if (0 == pThread->mQuantumHfTicksRemaining)
        {
            //
            // no quantum left of current thread - move to ran list
            // (after the threads that just migrated)
            //
            KTRACE(apThisCore, 3, KTRACE_THREAD_QUANTUM_EXPIRED, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);
            apThisCore->mpActiveThread = NULL;
            pThread->mState = KernThreadState_OnCpuLists;
            K2LIST_AddAtTail((K2LIST_ANCHOR *)&apThisCore->RanList, &pThread->CpuCoreThreadListLink);
        }

        return;
    }

    //
    // there is a current thread 
    // there are no threads on the run list
    // there may be threads on the ran list
    // there may be threads on the migrated list
    //
    if ((0 == apThisCore->RanList.mNodeCount) &&
        (0 == apThisCore->MigratedList.mNodeCount))
    {
        //
        // only thread is the active one. let it run
        //
        return;
    }

    if (pThread->mQuantumHfTicksRemaining != 0)
    {
        //
        // active thread has quantum left. move any migrated threads
        // to the ran list
        //
        K2LIST_AppendToTail((K2LIST_ANCHOR *)&apThisCore->RanList, (K2LIST_ANCHOR *)&apThisCore->MigratedList);
        return;
    }

    //
    // there are no threads on the run list, there is at 
    // least one thread on the ran list or migrated list,
    // and the active thread has exhausted its quantum
    //

    //
    // put ran threads onto the run list
    //
    K2LIST_AppendToTail((K2LIST_ANCHOR *)&apThisCore->RunList, (K2LIST_ANCHOR *)&apThisCore->RanList);

    //
    // put migrated threads onto the run list
    //
    K2LIST_AppendToTail((K2LIST_ANCHOR *)&apThisCore->RunList, (K2LIST_ANCHOR *)&apThisCore->MigratedList);

    //
    // put active thread onto the run list
    //
    KTRACE(apThisCore, 3, KTRACE_THREAD_QUANTUM_EXPIRED, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);
    apThisCore->mpActiveThread = NULL;
    pThread->mState = KernThreadState_OnCpuLists;
    K2LIST_AddAtTail((K2LIST_ANCHOR *)&apThisCore->RunList, &pThread->CpuCoreThreadListLink);

    //
    // schedule the run list now to give all the threads there some quantum
    //
    KernCpu_ScheduleRunList(apThisCore);
}

void    
KernCpu_MigrateThreadToCore(
    K2OSKERN_OBJ_THREAD *       apThread,
    K2OSKERN_CPUCORE volatile * apTargetCore
)
{
    UINT32 v;

    K2_ASSERT(KernProcState_Stopping > apThread->ProcRef.Ptr.AsProc->mState);
    K2_ASSERT(KernThreadState_InScheduler == apThread->mState);
    apThread->mState = KernThreadState_Migrating;
    do
    {
        v = (UINT32)apTargetCore->mpMigratingThreadList;
        apThread->mpMigratingNext = (K2OSKERN_OBJ_THREAD *)v;
    } while (v != K2ATOMIC_CompareExchange((UINT32 volatile *)&apTargetCore->mpMigratingThreadList, (UINT32)apThread, v));
    K2ATOMIC_Inc(&gData.Sched.mCoreThreadCount[apTargetCore->mCoreIx]);
}

void 
KernCpu_RunMonitor(
    void
)
{
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD *       pThread;

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    K2_ASSERT(pThisCore->mIsExecuting);
    K2_ASSERT(pThisCore->mIsInMonitor);
    if (pThisCore->mIsIdle)
    {
        KTRACE(pThisCore, 1, KTRACE_CORE_LEAVE_IDLE);
        pThisCore->mIsIdle = FALSE;
    }
    KernCpu_SetTickMode(pThisCore, KernTickMode_Kern);

    /* interrupts MUST BE ON running here */
#ifdef K2_DEBUG
    if (FALSE != K2OSKERN_GetIntr())
        K2OSKERN_Panic("Interrupts enabled on entry to monitor!\n");
#endif

    K2OSKERN_SetIntr(TRUE);

    do
    {
        KernCpu_DrainEvents(pThisCore);

        do
        {
            KernDbg_IoCheck(pThisCore);

            if (KernCpu_ExecOneDpc(pThisCore, KernDpcPrio_Hi))
                break;

            if (KernSched_Exec(pThisCore))
                break;

            if (KernCpu_ExecOneDpc(pThisCore, KernDpcPrio_Med))
                break;

            KernCpu_Schedule(pThisCore);

            //
            // try returning to current thread, 
            // or exec low prio dpc if no thread,
            // or go idle if no low prio dpc
            //
            K2OSKERN_SetIntr(FALSE);

            if (0 == pThisCore->PendingEventList.mNodeCount)
            {
                if (!pThisCore->mInDebugMode)
                {
                    pThread = pThisCore->mpActiveThread;

                    if (pThread == NULL)
                    {
                        //
                        // no active thread - is there anything we can run?
                        //
                        if (pThisCore->RunList.mNodeCount > 0)
                        {
                            pThread = K2_GET_CONTAINER(K2OSKERN_OBJ_THREAD, pThisCore->RunList.mpHead, CpuCoreThreadListLink);
                            K2_ASSERT(KernThreadState_OnCpuLists == pThread->mState);
                            K2LIST_Remove((K2LIST_ANCHOR *)&pThisCore->RunList, &pThread->CpuCoreThreadListLink);
                            KTRACE(pThisCore, 3, KTRACE_THREAD_RUN, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);
                            pThread->mState = KernThreadState_Running;
                            KernArch_SetCoreToProcess(pThisCore, pThread->ProcRef.Ptr.AsProc);
                            pThisCore->mpActiveThread = pThread;
                            K2_CpuWriteBarrier();
                        }
                    }

                    if (pThread != NULL)
                    {
                        //
                        // we have an active thread to run, so return to it
                        //
                        K2_ASSERT((pThisCore->RunList.mNodeCount == 0) || (0 != pThread->mQuantumHfTicksRemaining));
                        KTRACE(pThisCore, 3, KTRACE_CORE_RESUME_THREAD, pThread->ProcRef.Ptr.AsProc->mId, pThread->mGlobalIx);
                        pThisCore->mIsInMonitor = FALSE;
                        KernCpu_SetTickMode(pThisCore, KernTickMode_Thread);

                        KernArch_ResumeThread(pThisCore);

                        //
                        // will never return here
                        //
                        K2OSKERN_Panic("Switching from monitor to thread returned!");
                    }
                }

                //
                // no thread to run
                //
                if (pThisCore->DpcLo.mNodeCount == 0)
                {
                    if ((!pThisCore->mInDebugMode) ||
                        (gData.Debug.mLeadCoreIx != pThisCore->mCoreIx))
                    {
                        //
                        // interrupts are off. exiting from this function
                        // returns to the caller, which knows this and will
                        // go into the idle loop waiting for an interrupt at
                        // which point it will re-enter this function again
                        // 
                        KTRACE(pThisCore, 1, KTRACE_CORE_ENTER_IDLE);
                        pThisCore->mIsIdle = TRUE;
                        KernCpu_SetTickMode(pThisCore, KernTickMode_Idle);
                        return;
                    }
                }

                K2OSKERN_SetIntr(TRUE);

                KernCpu_ExecOneDpc(pThisCore, KernDpcPrio_Lo);
            }
            else
            {
                //
                // event came in
                //
                K2OSKERN_SetIntr(TRUE);
            }

        } while (0);

    } while (1);

    //
    // should never get here
    //
    K2OSKERN_Panic("KernCpu_RunMonitor ended!\n");
}

void    
KernCpu_SetTickMode(
    K2OSKERN_CPUCORE volatile * apThisCore,
    KernTickModeType            aNewMode
)
{
    UINT64                  tickWork;
    K2OSKERN_OBJ_THREAD *   pCurThread;

    //
    // get delta from last
    //
    tickWork = apThisCore->mModeStartHfTick;
    KernTimer_GetHfTick((UINT64 *)&apThisCore->mModeStartHfTick);
    K2_ASSERT(apThisCore->mModeStartHfTick >= tickWork);
    tickWork = apThisCore->mModeStartHfTick - tickWork;

    switch (apThisCore->mTickMode)
    {
    case KernTickMode_Kern:
        apThisCore->mKernHfTicks += tickWork;
        break;

    case KernTickMode_Idle:
        apThisCore->mIdleHfTicks += tickWork;
        break;

    case KernTickMode_Thread:
        pCurThread = apThisCore->mpActiveThread;
        K2_ASSERT(NULL != pCurThread);
        pCurThread->mUserHfTicks += tickWork;
        if (pCurThread->mQuantumHfTicksRemaining <= tickWork)
        {
            if (0 != pCurThread->mQuantumHfTicksRemaining)
            {
                // thread exhausted its quantum while running
            }
            pCurThread->mQuantumHfTicksRemaining = 0;
        }
        else
        {
            pCurThread->mQuantumHfTicksRemaining -= tickWork;
        }
        break;

    default:
        // ignore transitions from invalid to valid
        break;
    }
    
    apThisCore->mTickMode = aNewMode;
}
