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

void 
KernIntr_Init(
    void
)
{
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
    K2OSKERN_TLBSHOOT volatile *    pTlbShoot;
    UINT32                          pagesLeft;
    UINT32                          virtAddr;

    //
    // this is not a cpucore event (yet)
    // either promote it to that by adding the ici's event as an event to this core
    // or clear the ici code so another ici can be sent
    //
    pIci = &apThisCore->IciFromOtherCore[aSrcCoreIx];

//    K2OSKERN_Debug("Core %d Recv ICI %d from core %d\n", apThisCore->mCoreIx, pIci->mIciType, aSrcCoreIx);

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
            if ((NULL == apCurThread) || (pTargetProc != apCurThread->User.ProcRef.AsProc))
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

    //
    // promote this to a cpucore event, which will cause the monitor to be 
    // entered if we're not already in the monitor
    //
    pEvent = (K2OSKERN_CPUCORE_EVENT *)&pIci->CpuCoreEvent;
    pEvent->mEventType = KernCpuCoreEvent_Ici;
    pEvent->mSrcCoreIx = aSrcCoreIx;
    KernArch_GetHfTimerTick((UINT64 *)&pEvent->mEventHfTick);
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

    //
    // called straight from arch interrupt handler 
    // 
    KernArch_GetHfTimerTick(&tick);
    
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
    callId = apCallingThread->User.mSysCall_Id;

    switch (callId)
    {
    case K2OS_SYSCALL_ID_CRT_INITXDL:
        apCallingThread->User.ProcRef.AsProc->mpUserXdlList = (K2LIST_ANCHOR *)apCallingThread->User.mSysCall_Arg0;
        *apRetFastResult = 1;
        return;

    case K2OS_SYSCALL_ID_TLS_ALLOC:
        *apRetFastResult = KernProc_SysCall_Fast_TlsAlloc(apThisCore, apCallingThread);
        return;

    case K2OS_SYSCALL_ID_TLS_FREE:
        *apRetFastResult = KernProc_SysCall_Fast_TlsFree(apThisCore, apCallingThread);
        return;

    case K2OS_SYSCALL_ID_MAILBOX_SENTFIRST:
        // return or break depending on whether consumerix == arg
        if (KernMailbox_InIntr_Fast_Check_SentFirst(apThisCore, apCallingThread))
        {
            *apRetFastResult = apCallingThread->User.mSysCall_Result;
            return;
        }
        // devolve into regular syscall
        break;

    case K2OS_SYSCALL_ID_IFINST_GETID:
        *apRetFastResult = KernIfInst_SysCall_Fast_GetId(apThisCore, apCallingThread);
        return;

    case K2OS_SYSCALL_ID_IFINST_CONTEXTOP:
        *apRetFastResult = KernIfInst_SysCall_Fast_ContextOp(apThisCore, apCallingThread);
        return;

    case K2OS_SYSCALL_ID_IPCEND_LOAD:
        *apRetFastResult = KernIpcEnd_InIntr_SysCall_Load(
            apCallingThread,
            (K2OS_IPCEND_TOKEN)apCallingThread->User.mSysCall_Arg0);
        return;

    default:
        break;
    }

    apCallingThread->User.mIsInSysCall = TRUE;

    pEvent = &apCallingThread->CpuCoreEvent;
    pEvent->mEventType = KernCpuCoreEvent_Thread_SysCall;
    pEvent->mSrcCoreIx = apThisCore->mCoreIx;
    KernArch_GetHfTimerTick((UINT64 *)&pEvent->mEventHfTick);
    KernCpu_QueueEvent(pEvent);

    return;
}

void    
KernIntr_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_INTERRUPT *    apInterrupt
)
{
    K2_ASSERT(0);
}

void    
KernIntr_CpuEvent_Fired(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_INTERRUPT *    apInterrupt,
    UINT64 const *              apHfTick
)
{
    //
    // must not already be in service
    //
    K2_ASSERT(!apInterrupt->mInService);

    //
    // Scheduler will put interrupt into service when
    // it pulses the interrupt gate
    //
    apInterrupt->SchedItem.mType = KernSchedItem_Interrupt;
    K2_ASSERT(apInterrupt->SchedItem.ObjRef.AsInterrupt == apInterrupt);
    apInterrupt->SchedItem.mHfTick = *apHfTick;
    KernSched_QueueItem(&apInterrupt->SchedItem);
}
