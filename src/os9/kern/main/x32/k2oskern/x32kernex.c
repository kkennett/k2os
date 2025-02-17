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

void KernArch_PopKernelTrap(K2OSKERN_OBJ_THREAD *apThread)
{
    K2_EXCEPTION_TRAP *         pTrap;
    K2OS_THREAD_PAGE *          pThreadPage;
    K2OSKERN_ARCH_EXEC_CONTEXT *pExContext;

    K2_ASSERT(apThread->mIsKernelThread);

    pThreadPage = apThread->mpKernRwViewOfThreadPage;

    pTrap = (K2_EXCEPTION_TRAP *)pThreadPage->mTrapStackTop;
    pThreadPage->mTrapStackTop = (UINT32)pTrap->mpNextTrap;
    pTrap->mTrapResult = apThread->LastEx.mExCode;

    apThread->Kern.mStackPtr = (pTrap->SavedContext.ESP + 4) - X32_SIZEOF_KERN_EXCEPTION_CONTEXT;
    pExContext = (K2OSKERN_ARCH_EXEC_CONTEXT *)apThread->Kern.mStackPtr;
    K2MEM_Copy(&pExContext->REGS.EDI, &pTrap->SavedContext.EDI, sizeof(X32_PUSHA));
    pExContext->REGS.EAX = 0xFFFFFFFF;
    pExContext->DS = X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_KERNEL;
    pExContext->EIP = pTrap->SavedContext.EIP;
    pExContext->EFLAGS = pTrap->SavedContext.EFLAGS;
    pExContext->CS = X32_SEGMENT_SELECTOR_KERNEL_CODE | X32_SELECTOR_RPL_KERNEL;

    K2_ASSERT(apThread->LastEx.VirtMapRef.AsAny == NULL);
    K2MEM_Zero(&apThread->LastEx, sizeof(K2OSKERN_THREAD_EX));
}

void
KernArch_PopUserTrap(
    K2OSKERN_OBJ_THREAD *apThread
)
{
    K2OS_THREAD_PAGE *          pThreadPage;
    K2OSKERN_ARCH_EXEC_CONTEXT *pExContext;

    K2_ASSERT(!apThread->mIsKernelThread);

    pThreadPage = apThread->mpKernRwViewOfThreadPage;

    pExContext = &apThread->User.ArchExecContext;

    pExContext->EIP = K2OS_UVA_PUBLICAPI_TRAP_RESTORE;
    pExContext->REGS.ECX = pThreadPage->mTrapStackTop;
    pExContext->REGS.EDX = apThread->LastEx.mExCode;

    pThreadPage->mTrapStackTop = 0; // restored to proper next-trap by trap resume code, unless that code faults

    K2_ASSERT(apThread->LastEx.VirtMapRef.AsAny == NULL);
    K2MEM_Zero(&apThread->LastEx, sizeof(K2OSKERN_THREAD_EX));
}

void X32Kern_MountExceptionTrap(X32_CONTEXT aThreadContext)
{
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJ_THREAD *   pThisThread;
    K2_EXCEPTION_TRAP *     pTrap;

    //
    // interrupts are off
    //
    K2_ASSERT(!K2OSKERN_GetIntr());

    // get the trap and save goop to it
    pTrap = (K2_EXCEPTION_TRAP *)aThreadContext.ECX;
    pTrap->mTrapResult = K2STAT_ERROR_UNKNOWN;
    K2MEM_Copy(&pTrap->SavedContext, &aThreadContext, sizeof(X32_CONTEXT));

    // push this trap onto the trap stack
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (KernThread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    pTrap->mpNextTrap = (K2_EXCEPTION_TRAP *)pThreadPage->mTrapStackTop;
    pThreadPage->mTrapStackTop = (UINT32)pTrap;
}

