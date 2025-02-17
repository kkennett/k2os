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

static UINT8 const sgTrapRestoreCode[]= 
{
/*  0: */  0x89, 0x51, 0x04,                        // mov    DWORD PTR[ecx + 0x4],edx      put trap result into correct place in trap structure
/*  3: */  0x8b, 0x11,                              // mov    edx,DWORD PTR[ecx]            put the next trap address into edx
/*  5: */  0x64, 0xa1, 0x00, 0x00, 0x00, 0x00,      // mov    eax,fs:0x0                    get the thread index
/*  b: */  0xc1, 0xe0, 0x0c,                        // shl    eax,0xc                       convert to page address
/*  e: */  0x89, 0x90, 0x28, 0x01, 0x00, 0x00,      // mov    DWORD PTR[eax + 0x128],edx    store the next trap address in the thread page
/* 14: */  0x89, 0xcc,                              // mov    esp,ecx                       set stack pointer to trap address
/* 16: */  0x83, 0xc4, 0x08,                        // add    esp,0x8                       skip next trap and trap result fields
/* 19: */  0x9d,                                    // popfd                                flags were pushed second
/* 1a: */  0x61,                                    // popad                                registers were pushed firat
/* 1b: */  0x58,                                    // pop    eax                           original return address to trap mount (last thing in trap saved context)
/* 1c: */  0x8b, 0x64, 0x24, 0xe8,                  // mov    esp,DWORD PTR[esp - 0x18]     restore original esp that was saved (and tweaked by trap save). never zero
/* 20: */  0xff, 0xe0                               // jmp    eax                           jump to return address from trap mount with eax nonzero
};

void
KernArch_UserInit(
    void
)
{
    //
    // copy in the public api codes to the public api page
    //
    K2MEM_Copy((void *)K2OS_KVA_PUBLICAPI_SYSCALL, sgPublicApiCode, sizeof(sgPublicApiCode));
    K2MEM_Copy((void *)K2OS_KVA_PUBLICAPI_TRAP_RESTORE, sgTrapRestoreCode, sizeof(sgTrapRestoreCode));
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

    apInitThread->User.ArchExecContext.REGS.ECX = K2OS_UVA_TIMER_IOPAGE_BASE - (gData.BuiltIn.RefRofsVirtMap.AsVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES);
    apInitThread->User.ArchExecContext.REGS.EDX = apProc->mId; // second arg to entry point
    apInitThread->User.ArchExecContext.EIP = gData.User.mEntrypoint;
    apInitThread->User.ArchExecContext.CS = (X32_SEGMENT_SELECTOR_USER_CODE | X32_SELECTOR_RPL_USER);
    apInitThread->User.ArchExecContext.EFLAGS = X32_EFLAGS_INTENABLE | X32_EFLAGS_SBO | X32_EFLAGS_ZERO;

    //
    // put first thread initial stack at end of TLS page for that thread
    // process is not mapped, so can't write into threadpage at its proc address
    //
    stackPtr = K2OS_KVA_THREADPAGES_BASE + (apInitThread->mGlobalIx * K2_VA_MEMPAGE_BYTES) + 0xFFC;
    *((UINT32 *)stackPtr) = 0;
    stackPtr -= sizeof(UINT32);
    *((UINT32 *)stackPtr) = 0;
    apInitThread->User.ArchExecContext.ESP = stackPtr - K2OS_KVA_THREADPAGES_BASE;   // offset threadpage from zero
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
    apThread->User.ArchExecContext.REGS.ESP_Before_PushA = (UINT32)&apThread->User.ArchExecContext.Exception_Vector;
    apThread->User.ArchExecContext.EIP = aEntryPoint;
    apThread->User.ArchExecContext.CS = (X32_SEGMENT_SELECTOR_USER_CODE | X32_SELECTOR_RPL_USER);
    apThread->User.ArchExecContext.EFLAGS = X32_EFLAGS_INTENABLE | X32_EFLAGS_SBO | X32_EFLAGS_ZERO;

    apThread->User.ArchExecContext.ESP = aStackPtr;
    apThread->User.ArchExecContext.SS = (X32_SEGMENT_SELECTOR_USER_DATA | X32_SELECTOR_RPL_USER);

    apThread->mState = KernThreadState_Created;
}

