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

static char sgSymDump[A32_SYM_NAME_MAX_LEN * K2OS_MAX_CPU_COUNT];

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
            forceEnterMonitor = A32Kern_CoreTimerInterrupt(pThisCore);
        }
        else
        {
            //
            // if this is a valid irq this will go through plat logic to deal with it
            //
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

static void
sEmitSymbolName(
    K2OSKERN_OBJ_PROCESS *  apProc,
    UINT32                  aAddr,
    char *                  pBuffer
)
{
    *pBuffer = 0;
    KernXdl_FindClosestSymbol(apProc, aAddr, pBuffer, A32_SYM_NAME_MAX_LEN);
    if (*pBuffer == 0)
    {
        K2OSKERN_Debug("?(%08X)", aAddr);
        return;
    }
    K2OSKERN_Debug("%s", pBuffer);
}

void A32Kern_DumpStackTrace(K2OSKERN_OBJ_PROCESS *apProc, UINT32 aPC, UINT32 aSP, UINT32* apBackPtr, UINT32 aLR, char *apBuffer)
{
    K2OSKERN_Debug("StackTrace:\n");
    K2OSKERN_Debug("------------------\n");
    K2OSKERN_Debug("SP       %08X\n", aSP);

    K2OSKERN_Debug("PC       ");
    sEmitSymbolName(apProc, aPC, apBuffer);
    K2OSKERN_Debug("\n");

    K2OSKERN_Debug("LR       %08X ", aLR);
    sEmitSymbolName(apProc, aLR, apBuffer);
    K2OSKERN_Debug("\n");

#if 0

each frame:

pc  <-- fp points here : contains pointer to 8 bytes after the push instruction at the start of the function that is saving these 4 registers
lr  -4  (saved Link register which holds the return address this routine will return to)
sp  -8  (saved stack pointer in function that is returned at the point of the call)
fp  -12 (next frame pointer in stack)

#endif

    K2OSKERN_Debug("%08X ", apBackPtr);
    if (apBackPtr == NULL)
    {
        K2OSKERN_Debug("Stack Ends\n");
        return;
    }
    K2OSKERN_Debug("%08X ", apBackPtr[-1]);
    sEmitSymbolName(apProc, apBackPtr[-1], apBuffer);
    K2OSKERN_Debug("\n");
    // saved stack pointer is at -2;  don't really care what that is
    do {
        apBackPtr = (UINT32*)apBackPtr[-3];
        K2OSKERN_Debug("%08X ", apBackPtr);
        if (apBackPtr == NULL)
        {
            K2OSKERN_Debug("\n");
            return;
        }
        if (apBackPtr[0] == 0)
        {
            K2OSKERN_Debug("00000000 Stack Ends\n");
            return;
        }
        K2OSKERN_Debug("%08X ", apBackPtr[-1]);
        sEmitSymbolName(apProc, apBackPtr[-1], apBuffer);
        K2OSKERN_Debug("\n");
    } while (1);
}

void
A32Kern_DumpExecContext(
    K2OSKERN_ARCH_EXEC_CONTEXT * apExContext
)
{
    K2OSKERN_Debug("SPSR      %08X\n", apExContext->PSR);
    K2OSKERN_Debug("  r0=%08X   r8=%08X\n", apExContext->R[0], apExContext->R[8]);
    K2OSKERN_Debug("  r1=%08X   r9=%08X\n", apExContext->R[1], apExContext->R[9]);
    K2OSKERN_Debug("  r2=%08X  r10=%08X\n", apExContext->R[2], apExContext->R[10]);
    K2OSKERN_Debug("  r3=%08X  r11=%08X\n", apExContext->R[3], apExContext->R[11]);
    K2OSKERN_Debug("  r4=%08X  r12=%08X\n", apExContext->R[4], apExContext->R[12]);
    K2OSKERN_Debug("  r5=%08X   sp=%08X\n", apExContext->R[5], apExContext->R[13]);
    K2OSKERN_Debug("  r6=%08X   lr=%08X\n", apExContext->R[6], apExContext->R[14]);
    K2OSKERN_Debug("  r7=%08X   pc=%08X\n", apExContext->R[7], apExContext->R[15]);
}

void A32Kern_DumpExceptionContext(K2OSKERN_CPUCORE volatile* apCore, UINT32 aReason, K2OSKERN_ARCH_EXEC_CONTEXT* pEx)
{
    K2OSKERN_Debug("Exception %d (", aReason);
    switch (aReason)
    {
    case A32KERN_EXCEPTION_REASON_UNDEFINED_INSTRUCTION:
        K2OSKERN_Debug("Undefined Instruction");
        break;
    case A32KERN_EXCEPTION_REASON_SYSTEM_CALL:
        K2OSKERN_Debug("System Call");
        break;
    case A32KERN_EXCEPTION_REASON_PREFETCH_ABORT:
        K2OSKERN_Debug("Prefetch Abort");
        break;
    case A32KERN_EXCEPTION_REASON_DATA_ABORT:
        K2OSKERN_Debug("Data Abort");
        break;
    case A32KERN_EXCEPTION_REASON_IRQ:
        K2OSKERN_Debug("IRQ");
        break;
    case A32KERN_EXCEPTION_REASON_RAISE_EXCEPTION:
        K2OSKERN_Debug("RaiseException");
        break;
    default:
        K2OSKERN_Debug("Unknown");
        break;
    }
    K2OSKERN_Debug(") in Arm32.");
    switch (pEx->PSR & A32_PSR_MODE_MASK)
    {
    case A32_PSR_MODE_USR:
        K2OSKERN_Debug("USR");
        break;
    case A32_PSR_MODE_SYS:
        K2OSKERN_Debug("SYS");
        break;
    case A32_PSR_MODE_SVC:
        K2OSKERN_Debug("SVC");
        break;
    case A32_PSR_MODE_UNDEF:
        K2OSKERN_Debug("UND");
        break;
    case A32_PSR_MODE_ABT:
        K2OSKERN_Debug("ABT");
        break;
    case A32_PSR_MODE_IRQ:
        K2OSKERN_Debug("IRQ");
        break;
    case A32_PSR_MODE_FIQ:
        K2OSKERN_Debug("FIQ");
        break;
    default:
        K2OSKERN_Debug("???");
        break;
    }
    K2OSKERN_Debug(" mode. ExContext @ 0x%08X\n", pEx);
    K2OSKERN_Debug("------------------\n");
    K2OSKERN_Debug("Core      %d\n", apCore->mCoreIx);
    K2OSKERN_Debug("DFAR      %08X\n", A32_ReadDFAR());
    K2OSKERN_Debug("DFSR      %08X\n", A32_ReadDFSR());
    K2OSKERN_Debug("IFAR      %08X\n", A32_ReadIFAR());
    K2OSKERN_Debug("IFSR      %08X\n", A32_ReadIFSR());
    K2OSKERN_Debug("STACK     %08X\n", A32_ReadStackPointer());
    K2OSKERN_Debug("SCTLR     %08X\n", A32_ReadSCTRL());
    K2OSKERN_Debug("ACTLR     %08X\n", A32_ReadAUXCTRL());
    K2OSKERN_Debug("SCU       %08X\n", *((UINT32*)K2OS_KVA_A32_PERIPHBASE));
    //    K2OSKERN_Debug("SCR       %08X\n", A32_ReadSCR());    // will fault in NS mode
    K2OSKERN_Debug("DACR      %08X\n", A32_ReadDACR());
    K2OSKERN_Debug("TTBCR     %08X\n", A32_ReadTTBCR());
    K2OSKERN_Debug("TTBR0     %08X\n", A32_ReadTTBR0());
    K2OSKERN_Debug("TTBR1     %08X\n", A32_ReadTTBR1());
    K2OSKERN_Debug("CPSR      %08X\n", A32_ReadCPSR());
    K2OSKERN_Debug("------------------\n");
    A32Kern_DumpExecContext(pEx);
}

void
A32Kern_OnException(
    K2OSKERN_CPUCORE volatile*  apThisCore,
    UINT32                      aReason,
    K2OSKERN_ARCH_EXEC_CONTEXT* apEx
)
{
    K2OSKERN_OBJ_THREAD *       pCurThread;
    K2OSKERN_CPUCORE_EVENT *    pEvent;
    K2OSKERN_THREAD_EX  *       pThreadEx;
    UINT32                      fsr;

    if (A32_PSR_MODE_USR != (apEx->PSR & A32_PSR_MODE_MASK))
    {
        // 
        // kernel mode exception = panic
        //
//        K2OSKERN_Panic("Exception while not in user mode\n");
        K2OSKERN_Debug("Exception while not in user mode\n");
        A32Kern_DumpExceptionContext(apThisCore, aReason, apEx);
        A32Kern_DumpStackTrace(NULL, apEx->R[15], apEx->R[13], (UINT32 *)apEx->R[11], apEx->R[14], &sgSymDump[apThisCore->mCoreIx * A32_SYM_NAME_MAX_LEN]);
        while (1);
    }

    pCurThread = apThisCore->mpActiveThread;
    K2_ASSERT(NULL != pCurThread);

    pThreadEx = &pCurThread->User.LastEx;
    if ((aReason != A32KERN_EXCEPTION_REASON_UNDEFINED_INSTRUCTION) &&
        (aReason != A32KERN_EXCEPTION_REASON_PREFETCH_ABORT) &&
        (aReason != A32KERN_EXCEPTION_REASON_DATA_ABORT))
    {
        K2OSKERN_Debug("Unhandled exception reason (%d)\n", aReason);
        A32Kern_DumpExceptionContext(apThisCore, aReason, apEx);
        A32Kern_DumpStackTrace(NULL, apEx->R[15], apEx->R[13], (UINT32 *)apEx->R[11], apEx->R[14], &sgSymDump[apThisCore->mCoreIx * A32_SYM_NAME_MAX_LEN]);
        while (1);
    }

    if (aReason != A32KERN_EXCEPTION_REASON_DATA_ABORT)
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
        K2OSKERN_Debug("Core %d Exception, reason %d, fsr = %02X\n", apThisCore->mCoreIx, aReason, fsr);
        pThreadEx->mExCode = K2STAT_EX_UNKNOWN;
        pThreadEx->mPageWasPresent = FALSE;
    }

    pEvent = &pCurThread->CpuCoreEvent;
    pEvent->mEventType = KernCpuCoreEvent_Thread_Exception;
    pEvent->mSrcCoreIx = apThisCore->mCoreIx;
    KernArch_GetHfTimerTick((UINT64 *)&pEvent->mEventHfTick);
    KernCpu_QueueEvent(pEvent);
}

UINT32
A32Kern_InterruptHandler(
    UINT32 aReason,
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

    pEx = (K2OSKERN_ARCH_EXEC_CONTEXT*)aStackPtr;

    if (sgInIntr[pCoreMem->CpuCore.mCoreIx])
    {
        K2OSKERN_Panic("Recursive entry to interrupt handler\n");
        while (1);
    }
    sgInIntr[pCoreMem->CpuCore.mCoreIx] = TRUE;

    pCurThread = pCoreMem->CpuCore.mpActiveThread;
    if (NULL != pCurThread)
    {
        K2MEM_Copy(&pCurThread->User.ArchExecContext, (void *)aStackPtr, sizeof(K2OSKERN_ARCH_EXEC_CONTEXT));
    }

    if (aReason != A32KERN_EXCEPTION_REASON_IRQ)
    {
        if (aReason != A32KERN_EXCEPTION_REASON_SYSTEM_CALL)
        {
            A32Kern_OnException(&pCoreMem->CpuCore, aReason, pEx);
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

    //
    // returns stack pointer to use when jumping into monitor
    //
    return ((UINT32)&pCoreMem->mStacks[K2OSKERN_COREMEMORY_STACKS_BYTES]) - 8;
}

void
KernArch_Panic(
    K2OSKERN_CPUCORE volatile * apThisCore,
    BOOL                        aDumpStack
)
{
    if (aDumpStack)
    {
        A32Kern_DumpStackTrace(
            NULL,
            (UINT32)KernArch_Panic,
            A32_ReadStackPointer(),
            (UINT32 *)A32_ReadFramePointer(),
            (UINT32)K2_RETURN_ADDRESS,
            &sgSymDump[apThisCore->mCoreIx * A32_SYM_NAME_MAX_LEN]
        );
    }
    while (1);
}

void
KernArch_DumpThreadContext(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread
)
{
    K2OSKERN_Debug("Process %d Thread %d Context\n", apThread->User.ProcRef.AsProc->mId, apThread->mGlobalIx);
    A32Kern_DumpExecContext(&apThread->User.ArchExecContext);
    if (K2STAT_IS_ERROR(apThread->User.LastEx.mExCode))
    {
        K2OSKERN_Debug("Last Exception:\n");
        K2OSKERN_Debug("  Code    = %08X\n", apThread->User.LastEx.mExCode);
        K2OSKERN_Debug("  Address = %08X\n", apThread->User.LastEx.mFaultAddr);
        K2OSKERN_Debug("  Write   = %s\n", apThread->User.LastEx.mWasWrite ? "TRUE" : "FALSE");
        K2OSKERN_Debug("  Present = %s\n", apThread->User.LastEx.mPageWasPresent ? "TRUE" : "FALSE");
    }
    A32Kern_DumpStackTrace(
        apThread->User.ProcRef.AsProc,
        apThread->User.ArchExecContext.R[15],
        apThread->User.ArchExecContext.R[13],
        (UINT32 *)apThread->User.ArchExecContext.R[11],
        apThread->User.ArchExecContext.R[14],
        &sgSymDump[apThisCore->mCoreIx * A32_SYM_NAME_MAX_LEN]
        );
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
