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

void KernSched_Locked_TimePassedUntil(UINT64 *apTick);
void KernSched_Locked_ExecOneItem(K2OSKERN_SCHED_ITEM *apItem);

void 
KernSched_Init(
    void
)
{
    K2OSKERN_SeqInit(&gData.Sched.SeqLock);
    K2LIST_Init(&gData.Sched.Locked.TimerQueue);
    K2LIST_Init(&gData.Sched.Locked.DefferedResumeList);
}

void
KernSched_QueueItem(
    K2OSKERN_SCHED_ITEM *apItem
)
{
    K2OSKERN_SCHED_ITEM * pHead;
    K2OSKERN_SCHED_ITEM * pOld;

    //
    // lockless add to sched item list
    //
    do {
        pHead = gData.Sched.mpPendingItemListHead;
        apItem->mpNextItem = pHead;
        pOld = (K2OSKERN_SCHED_ITEM *)K2ATOMIC_CompareExchange((UINT32 volatile *)&gData.Sched.mpPendingItemListHead, (UINT32)apItem, (UINT32)pHead);
    } while (pOld != pHead);

    K2ATOMIC_Inc((INT32 volatile *)&gData.Sched.mReq);
}

void
KernSched_CheckAndDequeue(
    K2LIST_ANCHOR * pWorkList
)
{
    K2OSKERN_SCHED_ITEM *   pPendNew;
    K2OSKERN_SCHED_ITEM *   pWork;
    K2LIST_LINK *           pListLink;
    K2OSKERN_SCHED_ITEM *   pOtherItem;

    //
    // dequeue any new items
    //
    pPendNew = (K2OSKERN_SCHED_ITEM *)K2ATOMIC_Exchange((volatile UINT32 *)&gData.Sched.mpPendingItemListHead, 0);
    if (pPendNew == NULL)
        return;

    do
    {
        pWork = pPendNew;
        pPendNew = pPendNew->mpNextItem;

        //
        // insert to end of event list, IN TICK ORDER
        //
        if (pWorkList->mNodeCount == 0)
        {
            K2LIST_AddAtTail(pWorkList, &pWork->ListLink);
        }
        else
        {
            pListLink = pWorkList->mpTail;
            do
            {
                pOtherItem = K2_GET_CONTAINER(K2OSKERN_SCHED_ITEM, pListLink, ListLink);
                if (pOtherItem->mHfTick <= pWork->mHfTick)
                {
                    K2LIST_AddAfter(pWorkList, &pWork->ListLink, &pOtherItem->ListLink);
                    break;
                }
                pListLink = pListLink->mpPrev;
            } while (NULL != pListLink);
            if (NULL == pListLink)
            {
                K2LIST_AddAtHead(pWorkList, &pWork->ListLink);
            }
        }
    } while (NULL != pPendNew);
}

void
KernSched_Loop(
    void
)
{
    K2OSKERN_SCHED_ITEM *       pWork;
    K2LIST_ANCHOR               workList;
    BOOL                        ranDpc;
    BOOL                        disp;
    UINT64                      timeNow;

    K2LIST_Init(&workList);

    do
    {
        //
        // always exec hi-priority dpcs on this core prior to 
        // executing any scheduler items
        //
        ranDpc = KernCpu_ExecOneDpc(gData.Sched.mpSchedulingCore, KernDpcPrio_Hi);

        KernSched_CheckAndDequeue(&workList);

        if ((!ranDpc) && (0 == workList.mNodeCount))
        {
            KernTimer_GetHfTick(&timeNow);
            disp = K2OSKERN_SeqLock(&gData.Sched.SeqLock);
            KernSched_Locked_TimePassedUntil(&timeNow);
            K2OSKERN_SeqUnlock(&gData.Sched.SeqLock, disp);
            KernSched_CheckAndDequeue(&workList);
            if (0 == workList.mNodeCount)
                break;
        }

        if (0 != workList.mNodeCount)
        {
            pWork = K2_GET_CONTAINER(K2OSKERN_SCHED_ITEM, workList.mpHead, ListLink);
            K2LIST_Remove(&workList, &pWork->ListLink);
            K2ATOMIC_Dec((INT32 volatile *)&gData.Sched.mReq);

            disp = K2OSKERN_SeqLock(&gData.Sched.SeqLock);
            KernSched_Locked_ExecOneItem(pWork);
            K2OSKERN_SeqUnlock(&gData.Sched.SeqLock, disp);
        }
        else
        {
            K2_ASSERT(ranDpc);
            KernTimer_GetHfTick(&timeNow);
            disp = K2OSKERN_SeqLock(&gData.Sched.SeqLock);
            KernSched_Locked_TimePassedUntil(&timeNow);
            K2OSKERN_SeqUnlock(&gData.Sched.SeqLock, disp);
        }

    } while (1);
}

BOOL 
KernSched_Exec(
    K2OSKERN_CPUCORE volatile *apThisCore
)
{
    UINT32  v;
    UINT32  old;
    UINT32  reqVal;

    do {
        //
        // check to see if somebody else is scheduling already 
        //
        reqVal = gData.Sched.mReq;
        K2_CpuReadBarrier();
        if ((reqVal & 0xF0000000) != 0)
        {
            //
            // some other core is scheduling 
            // we didn't do anything
            //
            return FALSE;
        }

        //
        // nobody is scheduling - check to see if there is work to do 
        //
        if (0 == (reqVal & 0x0FFFFFFF))
        {
            //
            // no work to do
            // we didn't do anything
            //
            return FALSE;
        }

        //
        // nobody is scheduling and there is work to do,
        // to try to install this core as the scheduling core
        //
        v = reqVal | ((apThisCore->mCoreIx + 1) << 28);

        old = K2ATOMIC_CompareExchange(&gData.Sched.mReq, v, reqVal);
    } while (old != reqVal);

    //
    // this core got into the scheduler
    //
    gData.Sched.mpSchedulingCore = apThisCore;
    KTRACE(apThisCore, 1, KTRACE_CORE_ENTER_SCHEDULER);

    v = (apThisCore->mCoreIx + 1) << 28;

    do {
        gData.Sched.Locked.mMigratedMask = 0;

        //
        // execute scheduler
        //
        KernSched_Loop();

        if (0 != gData.Sched.Locked.mMigratedMask)
        {
            //
            // we don't care if this fails to send because if it does it means
            // the target core(s) is/are already awake
            //
            KernArch_SendIci(apThisCore, gData.Sched.Locked.mMigratedMask, KernIci_Wakeup, NULL);
            gData.Sched.Locked.mMigratedMask = 0;
        }

        //
        // try to leave the scheduler
        //
        gData.Sched.mpSchedulingCore = NULL;
        K2_CpuWriteBarrier();
        old = K2ATOMIC_CompareExchange(&gData.Sched.mReq, 0, v);
        if (old == v)
            break;

        //
        // we were not able to get out of the scheduler
        // before more work came in for us to do
        //
        gData.Sched.mpSchedulingCore = apThisCore;
        K2_CpuWriteBarrier();
    } while (1);

    KTRACE(apThisCore, 1, KTRACE_CORE_LEAVE_SCHEDULER);

    //
    // we did something
    //
    return TRUE;
}
    
void    
KernSched_CpuEvent_TimerFired(
    K2OSKERN_CPUCORE volatile * apThisCore,
    UINT64 const *              apHfTick
)
{
    //
    // this just tells the scheduler to actually run 
    // (get entered by this core or some other core)
    // without this the core will get woken but have no reason
    // to enter the scheduler (no items queued)
    //
    KTRACE(apThisCore, 1, KTRACE_TIMER_FIRED);
    K2_ASSERT(gData.Sched.TimerSchedItem.mType == KernSchedItem_Invalid);
    gData.Sched.TimerSchedItem.mType = KernSchedItem_SchedTimer_Fired;
    gData.Sched.TimerSchedItem.mHfTick = *apHfTick;
    KernSched_QueueItem(&gData.Sched.TimerSchedItem);
}

