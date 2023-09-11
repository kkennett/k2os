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

static UINT8 const sgPublicApiCode[] =
{
/* 00 */    0x89, 0xE0, // mov eax, esp
/* 02 */    0x0F, 0x34, // sysenter : eax=return esp, ecx, edx = call arguments
/* 04 */    0xC3        // ret instruction must be at this address (SYSCALL_API + 4)
};

void
KernArch_UserInit(
    void
)
{
    //
    // copy in the public api code to the public api page
    //
    K2MEM_Copy((void *)K2OS_KVA_PUBLICAPI_SYSCALL, sgPublicApiCode, sizeof(sgPublicApiCode));
    X32_CacheFlushAll(); // wbinvd

    //
    // we need a single page for the page directory for the process
    //
    gData.User.mInitPageCount++;
}

BOOL
KernArch_UserProcPrep(
    K2OSKERN_OBJ_PROCESS *apProc
)
{
    apProc->mVirtTransBase = KernVirt_Reserve(1);
    if (0 == apProc->mVirtTransBase)
        return FALSE;

    //
    // not possible to have simultaneous use here, so no lock taken on pagelists
    //
    K2_ASSERT(NULL != apProc->Virt.mpPageDirTrack);

    apProc->mPhysTransBase = K2OS_PHYSTRACK_TO_PHYS32((UINT32)apProc->Virt.mpPageDirTrack);

    KernPte_MakePageMap(NULL, apProc->mVirtTransBase, apProc->mPhysTransBase, K2OS_MAPTYPE_KERN_PAGEDIR);

    K2MEM_Zero((void *)apProc->mVirtTransBase, K2_VA_MEMPAGE_BYTES);

    return TRUE;
}

void    
KernArch_UserCrtPrep(
    K2OSKERN_OBJ_PROCESS *  apProc, 
    K2OSKERN_OBJ_THREAD *   apInitThread
)
{
    UINT32  stackPtr;

    K2_ASSERT(apInitThread->mState == KernThreadState_InitNoPrep);

    K2_ASSERT(*(((UINT32 *)K2OS_KVA_THREADPTRS_BASE) + apInitThread->mGlobalIx) == (UINT32)apInitThread);

    apInitThread->User.ArchExecContext.DS = (X32_SEGMENT_SELECTOR_USER_DATA | X32_SELECTOR_RPL_USER);

    apInitThread->User.ArchExecContext.REGS.ECX = K2OS_UVA_TIMER_IOPAGE_BASE - (gData.FileSys.RefRofsVirtMap.AsVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES);
    apInitThread->User.ArchExecContext.REGS.EDX = apProc->mId; // second arg to entry point
    apInitThread->User.ArchExecContext.EIP = gData.User.mEntrypoint;
    apInitThread->User.ArchExecContext.CS = (X32_SEGMENT_SELECTOR_USER_CODE | X32_SELECTOR_RPL_USER);
    apInitThread->User.ArchExecContext.EFLAGS = X32_EFLAGS_INTENABLE | X32_EFLAGS_SBO | X32_EFLAGS_ZERO;

    //
    // put first thread initial stack at end of TLS page for that thread
    // process is not mapped, so can't write into tls page at its proc address
    //
    stackPtr = K2OS_KVA_TLSAREA_BASE + (apInitThread->mGlobalIx * K2_VA_MEMPAGE_BYTES) + 0xFFC;
    *((UINT32 *)stackPtr) = 0;
    stackPtr -= sizeof(UINT32);
    *((UINT32 *)stackPtr) = 0;
    apInitThread->User.ArchExecContext.ESP = stackPtr - K2OS_KVA_TLSAREA_BASE;   // offset tls page from zero
    apInitThread->User.ArchExecContext.SS = (X32_SEGMENT_SELECTOR_USER_DATA | X32_SELECTOR_RPL_USER);

    apInitThread->mState = KernThreadState_Created;
}

void
KernArch_UserThreadPrep(
    K2OSKERN_OBJ_THREAD *   apThread,
    UINT32                  aEntryPoint,
    UINT32                  aStackPtr,
    UINT32                  aArg0,
    UINT32                  aArg1
)
{
    K2_ASSERT(apThread->mState == KernThreadState_InitNoPrep);

    K2_ASSERT(*(((UINT32 *)K2OS_KVA_THREADPTRS_BASE) + apThread->mGlobalIx) == (UINT32)apThread);

    apThread->User.ArchExecContext.DS = (X32_SEGMENT_SELECTOR_USER_DATA | X32_SELECTOR_RPL_USER);

    apThread->User.ArchExecContext.REGS.ECX = aArg0;
    apThread->User.ArchExecContext.REGS.EDX = aArg1;
    apThread->User.ArchExecContext.EIP = aEntryPoint;
    apThread->User.ArchExecContext.CS = (X32_SEGMENT_SELECTOR_USER_CODE | X32_SELECTOR_RPL_USER);
    apThread->User.ArchExecContext.EFLAGS = X32_EFLAGS_INTENABLE | X32_EFLAGS_SBO | X32_EFLAGS_ZERO;

    apThread->User.ArchExecContext.ESP = aStackPtr;
    apThread->User.ArchExecContext.SS = (X32_SEGMENT_SELECTOR_USER_DATA | X32_SELECTOR_RPL_USER);

    apThread->mState = KernThreadState_Created;
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
    UINT32                          initialStackPtr;

    K2_ASSERT(apThread->mState == KernThreadState_InitNoPrep);

    K2_ASSERT(*(((UINT32 *)K2OS_KVA_THREADPTRS_BASE) + apThread->mGlobalIx) == (UINT32)apThread);

    initialStackPtr = *apStackPtr;
    (*apStackPtr) -= (sizeof(K2OSKERN_ARCH_EXEC_CONTEXT) - (2 * sizeof(UINT32)));
    pCtx = (K2OSKERN_ARCH_EXEC_CONTEXT *)(*apStackPtr);
    K2MEM_Zero(pCtx, sizeof(K2OSKERN_ARCH_EXEC_CONTEXT) - 8);

    pCtx->DS = (X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_KERNEL);

    pCtx->REGS.ECX = aArg0;
    pCtx->REGS.EDX = aArg1;
    pCtx->EIP = aEntryPoint;
    pCtx->CS = (X32_SEGMENT_SELECTOR_KERNEL_CODE | X32_SELECTOR_RPL_KERNEL);
    pCtx->EFLAGS = X32_EFLAGS_INTENABLE | X32_EFLAGS_SBO | X32_EFLAGS_ZERO;

    pCtx->ESP = initialStackPtr;
    pCtx->SS = (X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_KERNEL);

    apThread->mIsKernelThread = TRUE;

    apThread->mState = KernThreadState_Created;
}
