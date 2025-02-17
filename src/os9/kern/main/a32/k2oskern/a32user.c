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

static UINT32 const sgPublicApiCode[] =
{
    /* 00 */    0xE1A0C00D, // mov r12, sp
    /* 04 */    0xEF000000, // svc #0
    /* 08 */    0xE12FFF1E, // bx lr
};

static UINT32 const sgTrapRestoreCode[] =
{
    /*  0: */   0xe5801004, // str     r1, [r0, #4]
    /*  4: */   0xe5901000, // ldr     r1, [r0]
    /*  8: */   0xee1dcf70, // mrc     15, 0, ip, cr13, cr0, {3}
    /*  c: */   0xe1a0c60c, // lsl     ip, ip, #12
    /* 10: */   0xe28ccf4a, // add     ip, ip, #296    ; 0x128
    /* 14: */   0xe58c1000, // str     r1, [ip]
    /* 18: */   0xe2800008, // add     r0, r0, #8
    /* 1c: */   0xe590c000, // ldr     ip, [r0]
    /* 20: */   0xe129f00c, // msr     CPSR_fc, ip
    /* 24: */   0xe2800008, // add     r0, r0, #8
    /* 28: */   0xe8b07ffe, // ldm     r0!, {r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, sp, lr}
    /* 2c: */   0xe12fff1e, // bx      lr
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
    K2MEM_Copy((void *)K2OS_KVA_PUBLICAPI_TRAP_RESTORE, sgTrapRestoreCode, sizeof(sgTrapRestoreCode));
    K2OS_CacheOperation(K2OS_CACHEOP_FlushDataAll, 0, 0);

    //
    // we need a two pages for the lower half of a translation table for the process
    //
    gData.User.mInitPageCount += 2;
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

    apInitThread->User.ArchExecContext.PSR = A32_PSR_MODE_USR;
    apInitThread->User.ArchExecContext.R[0] = K2OS_UVA_TIMER_IOPAGE_BASE - (gData.BuiltIn.RefRofsVirtMap.AsVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES);
    apInitThread->User.ArchExecContext.R[1] = apProc->mId;
    apInitThread->User.ArchExecContext.R[14] = 0;
    apInitThread->User.ArchExecContext.R[15] = gData.User.mEntrypoint;

    //
    // put first thread initial stack at end of threadpage for that thread
    // process is not mapped, so can't write into threadpage at its proc address
    //
    stackPtr = K2OS_KVA_THREADPAGES_BASE + (apInitThread->mGlobalIx * K2_VA_MEMPAGE_BYTES) + 0xFFC;
    *((UINT32 *)stackPtr) = 0;
    stackPtr -= sizeof(UINT32);
    *((UINT32 *)stackPtr) = 0;
    apInitThread->User.ArchExecContext.R[13] = stackPtr - K2OS_KVA_THREADPAGES_BASE;   // offset threadpage from zero

    apInitThread->mState = KernThreadState_Created;
}
    
BOOL    
KernArch_UserProcPrep(
    K2OSKERN_OBJ_PROCESS *apProc
) 
{ 
    apProc->mVirtTransBase = KernVirt_Reserve(2);
    if (0 == apProc->mVirtTransBase)
        return FALSE;

    //
    // not possible to have simultaneous use here, so no lock taken on pagelists
    //
    K2_ASSERT(NULL != apProc->Virt.mpPageDirTrack);

    // this will be an 8KB physical block that is 8KB aligned
    apProc->mPhysTransBase = K2OS_PHYSTRACK_TO_PHYS32((UINT32)apProc->Virt.mpPageDirTrack);

    KernPte_MakePageMap(NULL, apProc->mVirtTransBase, apProc->mPhysTransBase, K2OS_MAPTYPE_KERN_PAGEDIR);
    KernPte_MakePageMap(NULL, apProc->mVirtTransBase + K2_VA_MEMPAGE_BYTES, apProc->mPhysTransBase + K2_VA_MEMPAGE_BYTES, K2OS_MAPTYPE_KERN_PAGEDIR);

    K2MEM_Zero((void *)apProc->mVirtTransBase, 2 * K2_VA_MEMPAGE_BYTES);

    return TRUE;
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

    apThread->User.ArchExecContext.PSR = A32_PSR_MODE_USR;
    apThread->User.ArchExecContext.R[0] = aArg0;
    apThread->User.ArchExecContext.R[1] = aArg1;
    apThread->User.ArchExecContext.R[13] = aStackPtr;
    apThread->User.ArchExecContext.R[14] = 0;
    apThread->User.ArchExecContext.R[15] = aEntryPoint;

    apThread->mState = KernThreadState_Created;
}

