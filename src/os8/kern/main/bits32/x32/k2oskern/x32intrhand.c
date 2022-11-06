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

#include "x32kern.h"

static BOOL sgInIntr[K2OS_MAX_CPU_COUNT] = { 0, };

static K2OSKERN_IRQ * sgpIrqObjByIrqIx[X32_NUM_IDT_ENTRIES] = { 0, };

void
X32Kern_OnException(
    K2OSKERN_CPUCORE volatile *     apThisCore,
    K2OSKERN_ARCH_EXEC_CONTEXT *    apContext
)
{
    K2OSKERN_OBJ_THREAD *       pCurThread;
    K2OSKERN_CPUCORE_EVENT *    pEvent;
    K2OSKERN_THREAD_EX  *       pEx;

    if ((X32_SEGMENT_SELECTOR_USER_CODE | X32_SELECTOR_RPL_USER) == apContext->CS)
    {
        pCurThread = apThisCore->mpActiveThread;
        K2_ASSERT(NULL != pCurThread);
        pEx = &pCurThread->LastEx;
        pEx->mFaultAddr = X32_ReadCR2();
        pEx->mPageWasPresent = (apContext->Exception_ErrorCode & X32_EX_PAGE_FAULT_PRESENT) ? TRUE : FALSE;
        pEx->mWasWrite = (apContext->Exception_ErrorCode & X32_EX_PAGE_FAULT_ON_WRITE) ? TRUE : FALSE;

        if (apContext->Exception_Vector == X32_EX_PAGE_FAULT)
        {
            pEx->mExCode = K2STAT_EX_ACCESS;
        }
        else if (apContext->Exception_Vector == X32_EX_DIVIDE_BY_ZERO)
        {
            pEx->mExCode = K2STAT_EX_ZERODIVIDE;
        }
        else if (apContext->Exception_Vector == X32_EX_GENERAL)
        {
            pEx->mExCode = K2STAT_EX_PRIVILIGE;
        }
        else if (apContext->Exception_Vector == X32_EX_STACK_FAULT)
        {
            pEx->mExCode = K2STAT_EX_STACK;
        }
        else
        {
            pEx->mExCode = K2STAT_EX_UNKNOWN;
        }

        pEvent = &pCurThread->CpuCoreEvent;
        pEvent->mEventType = KernCpuCoreEvent_Thread_Exception;
        pEvent->mSrcCoreIx = apThisCore->mCoreIx;
        KernTimer_GetHfTick((UINT64 *)&pEvent->mEventHfTick);
        KernCpu_QueueEvent(pEvent);
        return;
    }

    // 
    // kernel mode exception = panic
    //
    K2OSKERN_Debug("Core %d, Exception in kernel mode. Context @ %08X\n", apThisCore->mCoreIx, apContext);
    K2OSKERN_Debug("Exception %d\n", apContext->Exception_Vector);
    K2OSKERN_Debug("CR2 = %08X\n", X32_ReadCR2());

    X32Kern_DumpKernelModeExceptionContext(apContext);
    X32Kern_DumpStackTrace(
        NULL,
        apContext->EIP,
        apContext->REGS.EBP,
        ((UINT32)apContext) + X32_SIZEOF_KERN_EXCEPTION_CONTEXT,
        &gX32Kern_SymDump[apThisCore->mCoreIx * X32_SYM_NAME_MAX_LEN]
    );
    K2OSKERN_Panic(NULL);
}

void
X32Kern_InterruptHandler(
    K2OSKERN_ARCH_EXEC_CONTEXT aContext
    )
{
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD *       pCurThread;
    UINT32                      devIrq;
    BOOL                        forceEnterMonitor;

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    forceEnterMonitor = FALSE;

    if (sgInIntr[pThisCore->mCoreIx])
    {
        K2OSKERN_Panic("%d: Recursive entry to interrupt handler\n", pThisCore->mCoreIx);
        while (1);
    }
    sgInIntr[pThisCore->mCoreIx] = TRUE;

    //
    // re-set the fs in case somebody in user land changed it
    //
    X32_SetFS((pThisCore->mCoreIx * X32_SIZEOF_GDTENTRY) | X32_SELECTOR_TI_LDT | X32_SELECTOR_RPL_USER);

    pCurThread = pThisCore->mpActiveThread;
    if (NULL != pCurThread)
    {
        K2_ASSERT(K2OSKERN_GetThisCoreCurrentThread() == pCurThread);
    }

    //
    // wha happon?
    //
    if (aContext.Exception_Vector < X32KERN_DEVVECTOR_BASE)
    {
        X32Kern_OnException(pThisCore, &aContext);
    }
    else if (aContext.Exception_Vector == 255)
    {
        K2_ASSERT(NULL != pCurThread);
        K2_ASSERT(!pThisCore->mIsInMonitor);
        aContext.EFLAGS |= X32_EFLAGS_INTENABLE;
        pCurThread->mSysCall_Id = aContext.REGS.ECX;
        pCurThread->mSysCall_Arg0 = aContext.REGS.EDX;
        KernIntr_OnSystemCall(pThisCore, pCurThread, &aContext.REGS.EAX);
    }
    else
    {
        if (aContext.Exception_Vector >= X32KERN_VECTOR_ICI_BASE)
        {
            K2_ASSERT(aContext.Exception_Vector <= X32KERN_VECTOR_ICI_LAST);
            KernIntr_OnIci(pThisCore, pCurThread, aContext.Exception_Vector - X32KERN_VECTOR_ICI_BASE);
        }
        else
        {
            devIrq = KernArch_VectorToDevIrq(aContext.Exception_Vector);
            K2_ASSERT(devIrq < X32_DEVIRQ_MAX_COUNT);
            if (devIrq == 0)
            {
                X32Kern_TimerInterrupt(pThisCore);
                forceEnterMonitor = TRUE;
            }
            else if (devIrq == X32_DEVIRQ_LVT_TIMER)
            {
                forceEnterMonitor = X32Kern_CoreTimerInterrupt(pThisCore);
            }
            else
            {
                KernIntr_OnIrq(pThisCore, sgpIrqObjByIrqIx[devIrq]);
            }
        }
        X32Kern_EOI(aContext.Exception_Vector);
    }

    if ((!pThisCore->mIsInMonitor) || (pThisCore->mIsIdle))
    {
        if ((forceEnterMonitor) ||
            (pThisCore->PendingEventList.mNodeCount != 0))
        {
            X32Kern_StopCoreTimer(pThisCore);

            if (pThisCore->mpActiveThread != NULL)
            {
                //
                // entering monitor from current thread
                // 
                K2MEM_Copy(&pCurThread->ArchExecContext, &aContext, sizeof(K2OSKERN_ARCH_EXEC_CONTEXT));
            }
            else
            {
                //
                // entering monitor from idle state
                //
            }

            pThisCore->mIsInMonitor = TRUE;
            sgInIntr[pThisCore->mCoreIx] = FALSE;
            X32Kern_EnterMonitor(gX32Kern_TSS[pThisCore->mCoreIx].TSS.mESP0);

            // should never return here
            K2OSKERN_Panic("X32Kern_EnterMonitor() returned\n");
        }
    }

    sgInIntr[pThisCore->mCoreIx] = FALSE;
}

void 
KernArch_InstallDevIrqHandler(
    K2OSKERN_IRQ *apIrq
)
{
    UINT32  irqIx;
    BOOL    disp;

    irqIx = apIrq->PlatIrq.Config.mSourceIrq;

    K2_ASSERT(irqIx < X32_DEVIRQ_MAX_COUNT);

    disp = K2OSKERN_SeqLock(&gX32Kern_IntrSeqLock);

    K2_ASSERT(sgpIrqObjByIrqIx[irqIx] == NULL);
    sgpIrqObjByIrqIx[irqIx] = apIrq;
    X32Kern_ConfigDevIrq(&apIrq->PlatIrq.Config);

    K2OSKERN_SeqUnlock(&gX32Kern_IntrSeqLock, disp);
}

void 
KernArch_SetDevIrqMask(
    K2OSKERN_IRQ *  apIrq, 
    BOOL            aMask
)
{
    UINT32  irqIx;
    BOOL    disp;

    irqIx = apIrq->PlatIrq.Config.mSourceIrq;

    K2_ASSERT(irqIx < X32_DEVIRQ_MAX_COUNT);

    disp = K2OSKERN_SeqLock(&gX32Kern_IntrSeqLock);

    K2_ASSERT(sgpIrqObjByIrqIx[irqIx] != NULL);
    if (aMask)
        X32Kern_MaskDevIrq(irqIx);
    else
        X32Kern_UnmaskDevIrq(irqIx);

    K2OSKERN_SeqUnlock(&gX32Kern_IntrSeqLock, disp);
}

void 
KernArch_RemoveDevIrqHandler(
    K2OSKERN_IRQ *apIrq
)
{
    UINT32  irqIx;
    BOOL    disp;

    irqIx = apIrq->PlatIrq.Config.mSourceIrq;

    K2_ASSERT(irqIx < X32_DEVIRQ_MAX_COUNT);

    disp = K2OSKERN_SeqLock(&gX32Kern_IntrSeqLock);

    K2_ASSERT(sgpIrqObjByIrqIx[irqIx] == apIrq);
    X32Kern_MaskDevIrq(irqIx);
    sgpIrqObjByIrqIx[irqIx] = NULL;

    K2OSKERN_SeqUnlock(&gX32Kern_IntrSeqLock, disp);
}

