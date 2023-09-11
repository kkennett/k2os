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

void    
KernArch_ResumeThread(
    K2OSKERN_CPUCORE volatile *apThisCore
) 
{
    K2OSKERN_OBJ_THREAD *   pCurThread;
    UINT32                  stackPtr;
    UINT32                  scratchPtr;
    UINT64                  coreTicks;
    K2OSKERN_COREMEMORY *   pCoreMem;

    //
    // interrupts should be off when we get here
    //
    K2_ASSERT(FALSE == K2OSKERN_GetIntr());

    pCurThread = apThisCore->mpActiveThread;
    K2_ASSERT(NULL != pCurThread);

//    K2OSKERN_Debug("Core %d Resume thread %d at PC %08X\n", apThisCore->mCoreIx,pCurThread->mGlobalIx, pCurThread->User.ArchExecContext.R[15]);

    K2_ASSERT(apThisCore->MappedProcRef.AsProc == pCurThread->User.ProcRef.AsProc);

    A32_WriteTPIDRPRW((UINT32)pCurThread->mGlobalIx);
    A32_WriteTPIDRURO((UINT32)pCurThread->mGlobalIx);

    stackPtr = (UINT32)&pCurThread->User.ArchExecContext;

    pCurThread->mpLastRunCore = apThisCore;

    if (pCurThread->User.mIsInSysCall)
    {
        ((K2OSKERN_ARCH_EXEC_CONTEXT *)stackPtr)->R[0] = pCurThread->User.mSysCall_Result;
        pCurThread->User.mIsInSysCall = FALSE;
    }

    //
    // interrupts must be unmasked in the context we are returning to, or something is wrong.
    //
    K2_ASSERT(0 == (((K2OSKERN_ARCH_EXEC_CONTEXT *)stackPtr)->PSR & (A32_PSR_F_BIT | A32_PSR_I_BIT)));

    //
    // translate from global timer ticks to core ticks for quanta remaining
    //
    coreTicks = pCurThread->mQuantumHfTicksRemaining;
    if (coreTicks > 0)
    {
        // translate to core timer rate and set timer
        K2_ASSERT(0);   // TBD
//        coreTicks = (coreTicks * gX32Kern_BusClockRate) / gData.Timer.mFreq;
//        X32Kern_SetCoreTimer(apThisCore, (UINT32)coreTicks);
    }

    //
    // use the SVC stack as temp memory for the resume
    //
    pCoreMem = K2_GET_CONTAINER(K2OSKERN_COREMEMORY, apThisCore, CpuCore);
    scratchPtr = (UINT32)&pCoreMem->mStacks;
    scratchPtr += A32KERN_CORE_IRQ_STACK_BYTES +
        A32KERN_CORE_ABT_STACK_BYTES +
        A32KERN_CORE_UND_STACK_BYTES +
        A32KERN_CORE_SVC_STACK_BYTES - 4;

    //
    // return to the thread
    //
    A32KernAsm_ResumeThread(stackPtr, scratchPtr);

    K2OSKERN_Panic("Switch to Thread returned!\n");
}

void
KernArch_TrapToContext(
    K2_TRAP_CONTEXT *           apTrapContext,
    K2OSKERN_ARCH_EXEC_CONTEXT *apArchContext,
    UINT32                      aOverwriteResult
)
{
    K2MEM_Copy(apArchContext, apTrapContext, sizeof(K2OSKERN_ARCH_EXEC_CONTEXT));
    apArchContext->R[0] = aOverwriteResult;
}
