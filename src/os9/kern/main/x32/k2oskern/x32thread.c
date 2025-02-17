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

#include "x32kern.h"

void
KernArch_ResumeThread(
    K2OSKERN_CPUCORE volatile * apThisCore
)
{
    K2OSKERN_OBJ_THREAD *       pCurThread;
    UINT32                      stackPtr;
    UINT64                      coreTicks;
    K2OSKERN_ARCH_EXEC_CONTEXT *pExec;

    //
    // interrupts should be off when we get here
    //
    K2_ASSERT(FALSE == K2OSKERN_GetIntr());

    pCurThread = apThisCore->mpActiveThread;
    K2_ASSERT(NULL != pCurThread);

    K2_ASSERT(pCurThread->mIsKernelThread || (apThisCore->MappedProcRef.AsProc == pCurThread->RefProc.AsProc));

    if (pCurThread->mIsKernelThread)
    {
        stackPtr = pCurThread->Kern.mStackPtr;
        K2_ASSERT(0 != stackPtr);
        pExec = (K2OSKERN_ARCH_EXEC_CONTEXT *)stackPtr;

        K2_ASSERT(pExec->CS == (X32_SEGMENT_SELECTOR_KERNEL_CODE | X32_SELECTOR_RPL_KERNEL));
        K2_ASSERT(pExec->DS == (X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_KERNEL));
    }
    else
    {
        gpX32Kern_PerCoreFS[apThisCore->mCoreIx] = pCurThread->mGlobalIx;
        K2_CpuWriteBarrier();

        stackPtr = (UINT32)&pCurThread->User.ArchExecContext;
        pExec = (K2OSKERN_ARCH_EXEC_CONTEXT *)stackPtr;

        if (pCurThread->User.mIsInSysCall)
        {
            ((K2OSKERN_ARCH_EXEC_CONTEXT *)stackPtr)->REGS.EAX = pCurThread->User.mSysCall_Result;
            pCurThread->User.mIsInSysCall = FALSE;
        }
        K2_ASSERT(pExec->CS == (X32_SEGMENT_SELECTOR_USER_CODE | X32_SELECTOR_RPL_USER));
        K2_ASSERT(pExec->DS == (X32_SEGMENT_SELECTOR_USER_DATA | X32_SELECTOR_RPL_USER));
    }

    pCurThread->mpLastRunCore = apThisCore;

    //
    // interrupts must be enabled in the context we are returning to, or something is wrong.
    //
    K2_ASSERT(0 != (pExec->EFLAGS & X32_EFLAGS_INTENABLE));

    //
    // translate from global timer ticks to core ticks for quanta remaining
    //
    coreTicks = pCurThread->mQuantumHfTicksRemaining;
    if (coreTicks > 0)
    {
        coreTicks = (coreTicks * gX32Kern_BusClockRate) / gData.Timer.mFreq;
        X32Kern_SetCoreTimer(apThisCore, (UINT32)coreTicks);
    }

    if (!pCurThread->mIsKernelThread)
    {
        X32_SetFS((apThisCore->mCoreIx * X32_SIZEOF_GDTENTRY) | X32_SELECTOR_TI_LDT | X32_SELECTOR_RPL_USER);
    }

    //
    // return to the thread
    //
    X32KernAsm_InterruptReturn(stackPtr);

    K2OSKERN_Panic("Switch to Thread returned!\n");
}

void
KernArch_KernThreadPrep(
    K2OSKERN_OBJ_THREAD *   apThread,
    UINT32                  aEntryPoint,
    UINT32 *                apStackPtr,
    UINT32                  aArg0,
    UINT32                  aArg1
)
{
    K2OSKERN_ARCH_EXEC_CONTEXT * pCtx;

    K2_ASSERT(apThread->mState == KernThreadState_InitNoPrep);
    K2_ASSERT(apThread->mIsKernelThread);

    K2_ASSERT(*(((UINT32 *)K2OS_KVA_THREADPTRS_BASE) + apThread->mGlobalIx) == (UINT32)apThread);

    (*apStackPtr) -= (sizeof(K2OSKERN_ARCH_EXEC_CONTEXT) - (2 * sizeof(UINT32)));
    pCtx = (K2OSKERN_ARCH_EXEC_CONTEXT *)(*apStackPtr);
    K2MEM_Zero(pCtx, sizeof(K2OSKERN_ARCH_EXEC_CONTEXT) - 8);

    pCtx->DS = (X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_KERNEL);

    pCtx->REGS.ECX = aArg0;
    pCtx->REGS.EDX = aArg1;
    pCtx->REGS.ESP_Before_PushA = (UINT32)&pCtx->Exception_Vector;
    pCtx->EIP = aEntryPoint;
    pCtx->CS = (X32_SEGMENT_SELECTOR_KERNEL_CODE | X32_SELECTOR_RPL_KERNEL);
    pCtx->EFLAGS = X32_EFLAGS_INTENABLE | X32_EFLAGS_SBO | X32_EFLAGS_ZERO;

    apThread->mState = KernThreadState_Created;
}

void
KernArch_IntsOff_SaveKernelThreadStateAndEnterMonitor(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apThread
)
{
    K2OSKERN_COREMEMORY volatile *  pCoreMem;
    UINT32                          newStackPtr;

    apThisCore->mIsInMonitor = TRUE;

    //
    // interrupts are off. reset to base of core stack, then head to save state and switch to new stack
    //
    X32Kern_StopCoreTimer(apThisCore);

    pCoreMem = (K2OSKERN_COREMEMORY volatile *)(K2OS_KVA_COREMEMORY_BASE + ((apThisCore->mCoreIx) * (4 * K2_VA_MEMPAGE_BYTES)));
    newStackPtr = ((UINT32)pCoreMem) + ((K2_VA_MEMPAGE_BYTES * 4) - 4);
    *((UINT32 *)newStackPtr) = 0;
    newStackPtr -= sizeof(UINT32);
    *((UINT32 *)newStackPtr) = 0;

    X32KernAsm_SaveKernelThreadStateAndEnterMonitor(newStackPtr, &apThread->Kern.mStackPtr);
}

