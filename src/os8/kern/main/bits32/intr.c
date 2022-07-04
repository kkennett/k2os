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
KernIntr_Init(
    void
)
{
    K2OSKERN_SeqInit(&gData.Intr.SeqLock);
    K2LIST_Init(&gData.Intr.Locked.List);
}

void    
KernIntr_OnIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    UINT32                      aSrcCoreIx
)
{
    K2OSKERN_CPUCORE_ICI volatile * pIci;
    K2OSKERN_CPUCORE_EVENT *        pEvent;
    K2OSKERN_OBJ_PROCESS *          pTargetProc;
    K2OSKERN_OBJ_THREAD *           pThread;
    K2OSKERN_TLBSHOOT volatile *    pTlbShoot;
    UINT32                          pagesLeft;
    UINT32                          virtAddr;

    //
    // this is not a cpucore event (yet)
    // either promote it to that by adding the ici's event as an event to this core
    // or clear the ici code so another ici can be sent
    //
    pIci = &apThisCore->IciFromOtherCore[aSrcCoreIx];

    if (pIci->mIciType == KernIci_Panic)
    {
        KernCpu_PanicSpin(apThisCore);
        while (1);
    }

    if (pIci->mIciType == KernIci_TlbInv)
    {
        pTlbShoot = (K2OSKERN_TLBSHOOT volatile *)pIci->mpArg;
        pTargetProc = pTlbShoot->mpProc;
        if (pTargetProc != NULL)
        {
            // 
            // tlb shootdown is for a process, not for the kernel
            //
            if ((NULL == apCurThread) || (pTargetProc != apCurThread->ProcRef.Ptr.AsProc))
            {
                //
                // tlb shootdown is for a different process than 
                // what is mapped on this core.  so resolve the ici
                // and ignore it
                //
                K2ATOMIC_And(&pTlbShoot->mCoresRemaining, ~(1 << apThisCore->mCoreIx));
                pIci->mIciType = KernIci_None;
                KTRACE(apThisCore, 1, KTRACE_PROC_TLBSHOOT_ICI_IGNORED);
                return;
            }
        }

        if (pTlbShoot->mPageCount <= 16)
        {
            if (pTargetProc != NULL)
            {
                KTRACE(apThisCore, 2, KTRACE_PROC_TLBSHOOT_ICI, pTargetProc->mId);
            }
            else
            {
                KTRACE(apThisCore, 1, KTRACE_KERN_TLBSHOOT_ICI);
            }
            //
            // small tlb invalidate do here without entering monitor
            //
            virtAddr = pTlbShoot->mVirtBase;
            pagesLeft = pTlbShoot->mPageCount;
            do
            {
                KernArch_InvalidateTlbPageOnCurrentCore(virtAddr);
                virtAddr += K2_VA_MEMPAGE_BYTES;
            } while (--pagesLeft);
            pIci->mIciType = KernIci_None;
            K2ATOMIC_And(&pTlbShoot->mCoresRemaining, ~(1 << apThisCore->mCoreIx));
            return;
        }
    }

    if (pIci->mIciType == KernIci_IoPermitUpdate)
    {
        pThread = (K2OSKERN_OBJ_THREAD *)pIci->mpArg;
        K2_ASSERT(NULL != pThread);
        K2_ASSERT(NULL != pThread->IoPermitProcRef.Ptr.AsHdr);
        if ((apThisCore->MappedProcRef.Ptr.AsProc == NULL) ||
            (apThisCore->MappedProcRef.Ptr.AsProc != pThread->IoPermitProcRef.Ptr.AsProc))
        {
            //
            // this core is not running the specified process
            //
            pIci->mIciType = KernIci_None;
            K2ATOMIC_And(&pThread->TlbShoot.mCoresRemaining, ~(1 << apThisCore->mCoreIx));
            KTRACE(apThisCore, 1, KTRACE_PROC_IOPERMIT_ICI_IGNORED);
            return;
        }
        //
        // this core is running the specified process, so we need to queue
        // a cpu event
        //
    }

    //
    // promote this to a cpucore event, which will cause the monitor to be 
    // entered if we're not already in the monitor
    //
    pEvent = (K2OSKERN_CPUCORE_EVENT *)&pIci->CpuCoreEvent;
    pEvent->mEventType = KernCpuCoreEvent_Ici;
    pEvent->mSrcCoreIx = aSrcCoreIx;
    KernTimer_GetHfTick((UINT64 *)&pEvent->mEventHfTick);
    KernCpu_QueueEvent(pEvent);
}

void    
KernIntr_OnIrq(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_IRQ *              apIrq
)
{
    BOOL    disp;
    UINT64  tick;

    KernTimer_GetHfTick(&tick);
    
    disp = K2OSKERN_SeqLock(&gData.Intr.SeqLock);

    KernPlat_IntrLocked_OnIrq(apThisCore, &tick, apIrq);

    K2OSKERN_SeqUnlock(&gData.Intr.SeqLock, disp);
}

void
KernIntr_OnSystemCall(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCallingThread,
    UINT32 *                    apRetFastResult
)
{
    K2OSKERN_CPUCORE_EVENT *    pEvent;
    UINT32                      callId;

    //
    // thread state NOT saved to its context (yet)
    // this is an opportunity to do a fast return/resume if the system call
    // operation is something super simple we don't need to save context for
    //
    callId = apCallingThread->mSysCall_Id;

    switch (callId)
    {
    case K2OS_SYSCALL_ID_CRT_INITXDL:
        apCallingThread->ProcRef.Ptr.AsProc->mpUserXdlList = (K2LIST_ANCHOR *)apCallingThread->mSysCall_Arg0;
        *apRetFastResult = 1;
        return;

    case K2OS_SYSCALL_ID_TLS_ALLOC:
        *apRetFastResult = KernProc_SysCall_Fast_TlsAlloc(apThisCore, apCallingThread);
        return;

    case K2OS_SYSCALL_ID_TLS_FREE:
        *apRetFastResult = KernProc_SysCall_Fast_TlsFree(apThisCore, apCallingThread);
        return;

    case K2OS_SYSCALL_ID_IPCEND_LOAD:
        *apRetFastResult = KernIpcEnd_InIntr_SysCall_Load(
            apCallingThread, 
            (K2OS_IPCEND_TOKEN)apCallingThread->mSysCall_Arg0);
        return;

    case K2OS_SYSCALL_ID_MAILBOX_RECV_RES:
        //
        // this may be able to be done quickly without going into the monitor
        //
        if (KernMailbox_InIntr_SysCall_RecvRes(apCallingThread, (K2OS_TOKEN)apCallingThread->mSysCall_Arg0, apCallingThread->mpKernRwViewOfUserThreadPage->mSysCall_Arg1))
        {
            *apRetFastResult = 1;
            return;
        }
        break;

    default:
        break;
    }

    apCallingThread->mIsInSysCall = TRUE;

    pEvent = &apCallingThread->CpuCoreEvent;
    pEvent->mEventType = KernCpuCoreEvent_Thread_SysCall;
    pEvent->mSrcCoreIx = apThisCore->mCoreIx;
    KernTimer_GetHfTick((UINT64 *)&pEvent->mEventHfTick);
    KernCpu_QueueEvent(pEvent);

    return;
}

void    
KernIntr_SysCall_SetEnable(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                      stat;
    K2OS_USER_THREAD_PAGE *     pThreadPage;
    K2OSKERN_OBJ_PROCESS *      pProc;
    BOOL                        disp;
    K2OSKERN_OBJREF             intrRef;

    pProc = apCurThread->ProcRef.Ptr.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    intrRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &intrRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (intrRef.Ptr.AsHdr->mObjType != KernObj_Interrupt)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
       }
        else
        {
            disp = K2OSKERN_SeqLock(&gData.Intr.SeqLock);

            if (!KernPlat_IntrLocked_SetEnable(intrRef.Ptr.AsInterrupt, (pThreadPage->mSysCall_Arg1 != 0) ? TRUE : FALSE))
            {
                //
                // only reason this will fail is if the interrupt is in service 
                // and the user is trying to enable it
                //
                stat = K2STAT_ERROR_API_ORDER;
            }
            else
            {
                apCurThread->mSysCall_Result = TRUE;
            }

            K2OSKERN_SeqUnlock(&gData.Intr.SeqLock, disp);
        }
        KernObj_ReleaseRef(&intrRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void    
KernIntr_SysCall_End(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT          stat;
    BOOL            disp;
    K2OSKERN_OBJREF intrRef;

    intrRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(apCurThread->ProcRef.Ptr.AsProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &intrRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (intrRef.Ptr.AsHdr->mObjType != KernObj_Interrupt)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            disp = K2OSKERN_SeqLock(&gData.Intr.SeqLock);

            if (intrRef.Ptr.AsInterrupt->mInService)
            {
                KernPlat_IntrLocked_End(intrRef.Ptr.AsInterrupt);
                K2_ASSERT(!intrRef.Ptr.AsInterrupt->mInService);
            }

            K2OSKERN_SeqUnlock(&gData.Intr.SeqLock, disp);

            apCurThread->mSysCall_Result = TRUE;
        }
        KernObj_ReleaseRef(&intrRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfUserThreadPage->mLastStatus = stat;
    }
}

void    
KernIntr_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_INTERRUPT *    apIntr
)
{
    BOOL                disp;
    K2OSKERN_IRQ *      pIrq;
    K2OSKERN_PLATINTR * pPlatIntr;
    K2OSKERN_PLATINTR * pPrev;

    disp = K2OSKERN_SeqLock(&gData.Intr.SeqLock);

    //
    // TBD - in service? is that possible if we are in cleanup?
    //
    K2_ASSERT(!apIntr->mInService);

    K2OSKERN_SeqLock(&gData.Plat.SeqLock);

    pIrq = K2_GET_CONTAINER(K2OSKERN_IRQ, apIntr->PlatIntr.mpIrq, PlatIrq);
    K2_ASSERT(pIrq->PlatIrq.IntrList.mNodeCount > 0);

    K2LIST_Remove(&pIrq->PlatIrq.IntrList, &apIntr->PlatIntr.IrqIntrListLink);
    apIntr->PlatIntr.mpIrq = NULL;

    pPrev = NULL;
    pPlatIntr = apIntr->PlatIntr.mpNode->mpIntrList;
    do {
        if (pPlatIntr == &apIntr->PlatIntr)
            break;
        pPrev = pPlatIntr;
        pPlatIntr = pPlatIntr->mpNext;
    } while (NULL != pPlatIntr);
    K2_ASSERT(NULL != pPlatIntr);

    if (pPrev == NULL)
    {
        apIntr->PlatIntr.mpNode->mpIntrList = pPlatIntr->mpNext;
    }
    else
    {
        pPrev->mpNext = pPlatIntr->mpNext;
    }

    apIntr->PlatIntr.mpNode = NULL;

    K2OSKERN_SeqUnlock(&gData.Plat.SeqLock, FALSE);

    K2OSKERN_SeqUnlock(&gData.Intr.SeqLock, disp);

    KernObj_ReleaseRef(&apIntr->GateRef);

    K2MEM_Zero(apIntr, sizeof(K2OSKERN_OBJ_INTERRUPT));

    KernHeap_Free(apIntr);
}

void    
KernIntr_CpuEvent_Fired(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_INTERRUPT *    apInterrupt,
    UINT64 const *              apHfTick
)
{
    //
    // irq code must have disabled the interrupt
    //
    K2_ASSERT(!apInterrupt->mEnabled);

    //
    // must not already be in service
    //
    K2_ASSERT(!apInterrupt->mInService);

    //
    // Scheduler will put interrupt into service when
    // it pulses the interrupt gate
    //
    apInterrupt->SchedItem.mType = KernSchedItem_Interrupt;
    K2_ASSERT(apInterrupt->SchedItem.ObjRef.Ptr.AsInterrupt == apInterrupt);
    apInterrupt->SchedItem.mHfTick = *apHfTick;
    KernSched_QueueItem(&apInterrupt->SchedItem);
}
