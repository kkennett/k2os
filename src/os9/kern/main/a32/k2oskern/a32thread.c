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
    K2OSKERN_OBJ_THREAD *       pCurThread;
    UINT32                      scratchPtr;
    UINT64                      coreTicks;
    K2OSKERN_COREMEMORY *       pCoreMem;
    K2OSKERN_ARCH_EXEC_CONTEXT *pExec;
    UINT32                      restoreFrom;

    //
    // interrupts should be off when we get here
    //
    K2_ASSERT(!K2OSKERN_GetIntr());

    //
    // must be in SYS mode.  we will be on core stack
    //
    K2_ASSERT(A32_PSR_MODE_SYS == (A32_ReadCPSR() & A32_PSR_MODE_MASK));

    pCurThread = apThisCore->mpActiveThread;
    K2_ASSERT(NULL != pCurThread);

    A32_WriteTPIDRPRW((UINT32)pCurThread->mGlobalIx);
    A32_WriteTPIDRURO((UINT32)pCurThread->mGlobalIx);

    if (NULL != pCurThread->RefProc.AsAny)
    {
        K2_ASSERT(apThisCore->MappedProcRef.AsProc == pCurThread->RefProc.AsProc);
    }

    if (pCurThread->mIsKernelThread)
    {
        restoreFrom = (UINT32)&pCurThread->Kern.ArchExecContext;
        pExec = (K2OSKERN_ARCH_EXEC_CONTEXT *)restoreFrom;
    }
    else
    {
        restoreFrom = (UINT32)&pCurThread->User.ArchExecContext;
        pExec = (K2OSKERN_ARCH_EXEC_CONTEXT *)restoreFrom;
        if (pCurThread->User.mIsInSysCall)
        {
            pExec->R[0] = pCurThread->User.mSysCall_Result;
            pCurThread->User.mIsInSysCall = FALSE;
        }
    }

    pCurThread->mpLastRunCore = apThisCore;

    //
    // interrupts must be unmasked in the context we are returning to, or something is wrong.
    //
    K2_ASSERT(0 == (pExec->PSR & A32_PSR_I_BIT));

    //
    // translate from global timer ticks to core ticks for quanta remaining
    //
    coreTicks = pCurThread->mQuantumHfTicksRemaining;
    if (coreTicks > 0)
    {
        // private timer rate and global timer rate are the same so we don't need to convert
        A32Kern_SetCoreTimer(apThisCore, (UINT32)(coreTicks & 0xFFFFFFFFull));
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
    A32KernAsm_ResumeThread(restoreFrom, scratchPtr);

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
    K2OSKERN_ARCH_EXEC_CONTEXT *    pCtx;

    K2_ASSERT(apThread->mState == KernThreadState_InitNoPrep);
    K2_ASSERT(apThread->mIsKernelThread);

    K2_ASSERT(*(((UINT32 *)K2OS_KVA_THREADPTRS_BASE) + apThread->mGlobalIx) == (UINT32)apThread);

    pCtx = (K2OSKERN_ARCH_EXEC_CONTEXT *)&apThread->Kern.ArchExecContext;
    K2MEM_Zero(pCtx, sizeof(K2OSKERN_ARCH_EXEC_CONTEXT));

    pCtx->PSR = A32_PSR_MODE_SYS;
    pCtx->R[0] = aArg0;
    pCtx->R[1] = aArg1;
    pCtx->R[13] = *apStackPtr;
    pCtx->R[14] = 0;
    pCtx->R[15] = aEntryPoint;

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

    A32Kern_StopCoreTimer(apThisCore);

    pCoreMem = (K2OSKERN_COREMEMORY volatile *)(K2OS_KVA_COREMEMORY_BASE + ((apThisCore->mCoreIx) * (4 * K2_VA_MEMPAGE_BYTES)));
    newStackPtr = ((UINT32)pCoreMem) + ((K2_VA_MEMPAGE_BYTES * 4) - 4);
    *((UINT32 *)newStackPtr) = 0;
    newStackPtr -= sizeof(UINT32);
    *((UINT32 *)newStackPtr) = 0;

    A32KernAsm_SaveKernelThreadStateAndEnterMonitor(newStackPtr, &apThread->Kern.ArchExecContext);
}

