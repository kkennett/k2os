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

#include "a32kern.h"

static BOOL sgInIntr[K2OS_MAX_CPU_COUNT] = { 0, };

static K2OSKERN_IRQ * sgpIrqObjByIrqIx[A32KERN_MAX_IRQ] = { 0, };

BOOL
A32Kern_CheckSvcInterrupt(
    UINT32 aStackPtr
)
{
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD *       pCurThread;
    K2OSKERN_ARCH_EXEC_CONTEXT *pEx;

    //
    // this can only happen when a user thread makes a system call
    //

    //
    // interrupts are off. 
    // core is in SVC mode.  
    // r0-r3,r13(usr/sys),r15 are saved in in exception context pointed to by aStackPtr
    // user mode r12 saved in r14 spot in exception context. 
    //

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    K2_ASSERT(!pThisCore->mIsInMonitor);
    K2_ASSERT(!pThisCore->mIsIdle);
    
    pCurThread = pThisCore->mpActiveThread;
    K2_ASSERT(NULL != pCurThread);
    
    pEx = (K2OSKERN_ARCH_EXEC_CONTEXT*)aStackPtr;
    pCurThread->User.mSysCall_Id = pEx->R[0];
    pCurThread->User.mSysCall_Arg0 = pEx->R[1];

    KTRACE(pThisCore, 2, KTRACE_CORE_SYSCALL, pEx->R[0]);

    KernIntr_OnSystemCall(pThisCore, pCurThread, &pEx->R[0]);

    //
    // cpu core event will have been pushed if we are doing a full system call
    // otherwise the system call has been handled and the result is already in R[0]
    //

    return (pThisCore->PendingEventList.mNodeCount != 0) ? TRUE : FALSE;
}

BOOL
A32Kern_CheckIrqInterrupt(
    UINT32 aStackPtr
)
{
    UINT32                      ackVal;
    UINT32                      intrId;
    K2OSKERN_CPUCORE volatile * pThisCore;
    BOOL                        forceEnterMonitor;

    //
    // this can happen any time interrupts are on
    //

    //
    // interrupts are off. 
    // core is in IRQ mode.  
    // r0-r3,r13(usr/sys),r15 are saved in exception context pointed to by aStackPtr
    // source mode r12 saved in r14 spot in exception context. 
    //
    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;

    ackVal = A32Kern_IntrAck();
    intrId = (ackVal & A32_PERIF_GICC_ICCIAR_ACKINTID);
    if (intrId == 0x3FF)
    {
        K2OSKERN_Debug("Core %d: Spurious IRQ\n", pThisCore->mCoreIx);
        //
        // just return to what you were doing
        //
        return FALSE;
    }

    forceEnterMonitor = FALSE;

    if (intrId < A32_SGI_COUNT)
    {
        //
        // this will add a pending event to the CPU if we need to enter the monitor
        //

        //
        // cpu this ici is from is ((ackVal>>10) & 0x3)
        //
        KTRACE(pThisCore, 2, KTRACE_CORE_ICI, (ackVal>>10) & 3);
        KernIntr_OnIci(pThisCore, pThisCore->mpActiveThread, intrId);
    }
    else
    {
        if (intrId == A32_MP_GTIMER_IRQ)
        {
            //
            // this will add a pending event to the CPU if we need to enter the monitor
            //
            A32Kern_TimerInterrupt(pThisCore);
        }
        else if (intrId == A32_MP_PTIMERS_IRQ)
        {
            //
            // this will add a pending event to the CPU if we need to enter the monitor
            //
            KTRACE(pThisCore, 2, KTRACE_CORE_TIMER_FIRED, intrId);
            forceEnterMonitor = A32Kern_CoreTimerInterrupt(pThisCore);
        }
        else
        {
            //
            // if this is a valid irq this will go through plat logic to deal with it
            //
            KTRACE(pThisCore, 2, KTRACE_CORE_DEVICE_IRQ, intrId);
            K2OSKERN_Debug("Core %d recv IRQ %d\n", pThisCore->mCoreIx, intrId);
            pThisCore->mActiveIrq = intrId;
            KernIntr_OnIrq(pThisCore, sgpIrqObjByIrqIx[intrId]);
        }
    }

    A32Kern_IntrEoi(ackVal);

    if (pThisCore->mIsInMonitor)
    {
        //
        // must return to where we left off
        //
        return FALSE;
    }

    if (forceEnterMonitor)
        return TRUE;

    return (pThisCore->PendingEventList.mNodeCount != 0) ? TRUE : FALSE;
}

void
A32Kern_OnException(
    K2OSKERN_CPUCORE volatile*  apThisCore,
    UINT32                      aArmReason,
    K2OSKERN_ARCH_EXEC_CONTEXT* apEx
)
{
    K2OSKERN_OBJ_THREAD *               pCurThread;
    K2OSKERN_CPUCORE_EVENT volatile *   pEvent;
    K2OSKERN_THREAD_EX  *               pThreadEx;
    UINT32                              fsr;

    if (apThisCore->mIsInMonitor)
    {
        K2OSKERN_Debug("Core %d, Exception %d in monitor. Context @ %08X\n", apThisCore->mCoreIx, aArmReason, apEx);
        A32Kern_DumpExceptionContext(apThisCore, aArmReason, apEx);
        A32Kern_DumpStackTrace(apThisCore, NULL, apEx->R[15], apEx->R[13], (UINT32 *)apEx->R[11], apEx->R[14]);
        K2OSKERN_Panic(NULL);
    }

    pCurThread = apThisCore->mpActiveThread;
    K2_ASSERT(NULL != pCurThread);

    pThreadEx = &pCurThread->LastEx;

    if (aArmReason != A32KERN_EXCEPTION_REASON_DATA_ABORT)
    {
        // IFAR, IFSR
        pThreadEx->mFaultAddr = A32_ReadIFAR();
        pThreadEx->mWasWrite = FALSE;
        fsr = A32_ReadIFSR();
    }
    else
    {
        // DFAR, DFSR
        pThreadEx->mFaultAddr = A32_ReadDFAR();
        fsr = A32_ReadDFSR();
        pThreadEx->mWasWrite = (fsr & A32_DFSR_WnR) ? TRUE : FALSE;
    }

    fsr = (fsr & A32_FSR_STATUS_03_MASK) | ((fsr & A32_FSR_STATUS_4) >> (A32_FSR_STATUS_4_SHL - 4));

    if ((fsr == A32_FAULT_TYPE_DOMAIN_FAULT_FIRST_LEVEL) ||
        (fsr == A32_FAULT_TYPE_PERMISSION_FIRST_LEVEL))
    {
        //
        // first level permission or domain wrong --> priviledge violation
        //
        pThreadEx->mExCode = K2STAT_EX_PRIVILIGE;
        pThreadEx->mPageWasPresent = FALSE;
    }
    else if (fsr == A32_FAULT_TYPE_DOMAIN_FAULT_SECOND_LEVEL)
    {
        //
        // second level domain faulre - priviledge violation with page present
        // because if it wasn't present this would be a translation fault
        //
        pThreadEx->mExCode = K2STAT_EX_PRIVILIGE;
        pThreadEx->mPageWasPresent = TRUE;  // otherwise it is a translation fault
    }
    else if (fsr == A32_FAULT_TYPE_PERMISSION_SECOND_LEVEL)
    {
        //
        // second level pemission fault - write to read only page, etc.
        //
        pThreadEx->mExCode = K2STAT_EX_ACCESS;
        pThreadEx->mPageWasPresent = TRUE;
    }
    else if ((fsr == A32_FAULT_TYPE_TRANSLATION_FIRST_LEVEL) ||
             (fsr == A32_FAULT_TYPE_TRANSLATION_SECOND_LEVEL))
    {
        //
        // generic page fault
        //
        pThreadEx->mExCode = K2STAT_EX_ACCESS;
        pThreadEx->mPageWasPresent = FALSE;
    }
    else
    {
        //
        // no idea what to do with this
        //
        K2OSKERN_Debug("Core %d Exception, reason %d, fsr = %02X\n", apThisCore->mCoreIx, aArmReason, fsr);
        pThreadEx->mExCode = K2STAT_EX_UNKNOWN;
        pThreadEx->mPageWasPresent = FALSE;
    }

    pThreadEx->mArchSpec[0] = aArmReason;
    pThreadEx->mArchSpec[1] = fsr;

    if (A32_PSR_MODE_USR == (apEx->PSR & A32_PSR_MODE_MASK))
    {
        K2_ASSERT(!pCurThread->mIsKernelThread);
    }
    else
    {
        K2_ASSERT(pCurThread->mIsKernelThread);
    }

    pEvent = &pCurThread->CpuCoreEvent;
    pEvent->mEventType = KernCpuCoreEvent_Thread_Exception;
    pEvent->mSrcCoreIx = apThisCore->mCoreIx;
    KernArch_GetHfTimerTick((UINT64 *)&pEvent->mEventHfTick);
    KernCpu_QueueEvent(pEvent);
}

UINT32
A32Kern_InterruptHandler(
    UINT32 aArmReason,
    UINT32 aStackPtr,
    UINT32 aCPSR
)
{
    //
    // if we return from this function, it enters the monitor at its start
    // using the stack pointer value returned
    // 
    // We are in the mode detailed in the aCPSR. 
    // If we interrupted from user mode this will be set to SYS
    // 
    // The aStackPtr points to the stack for the mode detailed in aCPSR
    // which contains the FULL saved exception context (starting with SPSR)
    // 
    // *IF* we return 'naturally' from this function, we must return the core memory 
    // stack pointer base for our core which will be used as the stack pointer the 
    // monitor is entered with
    //
    K2OSKERN_COREMEMORY *       pCoreMem;
    K2OSKERN_OBJ_THREAD *       pCurThread;
    K2OSKERN_ARCH_EXEC_CONTEXT* pEx;

    pCoreMem = K2OSKERN_GET_CURRENT_COREMEMORY;

    pEx = (K2OSKERN_ARCH_EXEC_CONTEXT*)aStackPtr;   // in whatever mode

    if (sgInIntr[pCoreMem->CpuCore.mCoreIx])
    {
        K2OSKERN_Panic("Recursive entry to interrupt handler\n");
        while (1);
    }
    sgInIntr[pCoreMem->CpuCore.mCoreIx] = TRUE;

    pCurThread = pCoreMem->CpuCore.mpActiveThread;
    if (NULL != pCurThread)
    {
        if (pCurThread->mIsKernelThread)
        {
            // we will never be in sys mode here as that is our native mode
            // outside of interrupt
            K2_ASSERT((aCPSR & A32_PSR_MODE_MASK) != A32_PSR_MODE_SYS);
            // we should always be coming from SYS mode into here, otherwise
            // there was an error while we were in one of the other modes
            // which is not recoverable.
            K2_ASSERT((pEx->PSR & A32_PSR_MODE_MASK) == A32_PSR_MODE_SYS);
            K2MEM_Copy(&pCurThread->Kern.ArchExecContext, (void*)aStackPtr, sizeof(K2OSKERN_ARCH_EXEC_CONTEXT));
        }
        else
        {
            K2_ASSERT((pEx->PSR & A32_PSR_MODE_MASK) == A32_PSR_MODE_USR);
            K2MEM_Copy(&pCurThread->User.ArchExecContext, (void *)aStackPtr, sizeof(K2OSKERN_ARCH_EXEC_CONTEXT));
        }
    }

    if (aArmReason != A32KERN_EXCEPTION_REASON_IRQ)
    {
        if (aArmReason != A32KERN_EXCEPTION_REASON_SYSTEM_CALL)
        {
            KTRACE(&pCoreMem->CpuCore, 2, KTRACE_CORE_EXCEPTION, aArmReason);
            A32Kern_OnException(&pCoreMem->CpuCore, aArmReason, pEx);
        }
        else
        {
            //
            // system call a no-op here as it was checked for activity inside SVC interrupt context
            // before the full context save.  an event was pushed to the CPU so we will enter the monitor
            //
            K2_ASSERT(!pCoreMem->CpuCore.mIsInMonitor);
            K2_ASSERT(pCoreMem->CpuCore.PendingEventList.mNodeCount != 0);
        }
    }

    K2_ASSERT(!pCoreMem->CpuCore.mIsInMonitor);
    sgInIntr[pCoreMem->CpuCore.mCoreIx] = FALSE;
    pCoreMem->CpuCore.mIsInMonitor = TRUE;
    A32Kern_StopCoreTimer(&pCoreMem->CpuCore);

    //
    // returns stack pointer to use when jumping into monitor
    //
    return ((UINT32)&pCoreMem->mStacks[K2OSKERN_COREMEMORY_STACKS_BYTES]) - 8;
}

void
KernArch_InstallDevIrqHandler(
    K2OSKERN_IRQ *apIrq
)
{
    UINT32  irqIx;
    BOOL    disp;

    irqIx = apIrq->Config.mSourceIrq;

    K2_ASSERT(irqIx < A32KERN_MAX_IRQ);

    disp = K2OSKERN_SeqLock(&gA32Kern_IntrSeqLock);

    K2_ASSERT(sgpIrqObjByIrqIx[irqIx] == NULL);
    sgpIrqObjByIrqIx[irqIx] = apIrq;

    A32Kern_IntrConfig(irqIx, &apIrq->Config);

    K2OSKERN_SeqUnlock(&gA32Kern_IntrSeqLock, disp);
}

void
KernArch_SetDevIrqMask(
    K2OSKERN_IRQ *  apIrq,
    BOOL            aMask
)
{
    UINT32  irqIx;
    BOOL    disp;

    irqIx = apIrq->Config.mSourceIrq;

    K2_ASSERT(irqIx < A32KERN_MAX_IRQ);

    disp = K2OSKERN_SeqLock(&gA32Kern_IntrSeqLock);

    K2_ASSERT(sgpIrqObjByIrqIx[irqIx] != NULL);
    A32Kern_IntrSetEnable(irqIx, !aMask);

    K2OSKERN_SeqUnlock(&gA32Kern_IntrSeqLock, disp);
}

void
KernArch_RemoveDevIrqHandler(
    K2OSKERN_IRQ *apIrq
)
{
    UINT32  irqIx;
    BOOL    disp;

    irqIx = apIrq->Config.mSourceIrq;

    K2_ASSERT(irqIx < A32KERN_MAX_IRQ);

    disp = K2OSKERN_SeqLock(&gA32Kern_IntrSeqLock);

    K2_ASSERT(sgpIrqObjByIrqIx[irqIx] == apIrq);
    A32Kern_IntrSetEnable(irqIx, FALSE);

    sgpIrqObjByIrqIx[irqIx] = NULL;

    K2OSKERN_SeqUnlock(&gA32Kern_IntrSeqLock, disp);
}
