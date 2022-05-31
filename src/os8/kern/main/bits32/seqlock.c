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

#include "kern.h"

K2_PACKED_PUSH
typedef struct _K2OSKERN_SEQLOCK_INTERNAL32 K2OSKERN_SEQLOCK_INTERNAL32;
struct _K2OSKERN_SEQLOCK_INTERNAL32
{
    //
    // this structure lives somewhere within mOpaque, aligned on K2OS_CACHELINE_BYTES
    // which puts mSeqIn and mSeqOut on their own individual (sequential) cache lines
    //
    union {
        UINT32 volatile mSeqIn;
        UINT8           mAlign[K2OS_CACHELINE_BYTES];
    };
    UINT32 volatile     mSeqOut;
    K2OSKERN_SEQLOCK_INTERNAL32 *  mpStackNext;
#if DEBUG_LOCK
    char const *mpFile;
    int mLine;
#endif
} K2_PACKED_ATTRIB;
K2_PACKED_POP

void 
K2_CALLCONV_REGS
K2OSKERN_SeqInit(
    K2OSKERN_SEQLOCK *  apLock
)
{
    K2OSKERN_SEQLOCK_INTERNAL32 *pLock32;

    pLock32 = (K2OSKERN_SEQLOCK_INTERNAL32 *)
        ((((UINT_PTR)apLock) + (K2OS_CACHELINE_BYTES - 1)) & ~(K2OS_CACHELINE_BYTES - 1));

    pLock32->mSeqIn = 0;
    pLock32->mSeqOut = 0;
    pLock32->mpStackNext = NULL;
#if DEBUG_LOCK
    pLock32->mpFile = NULL;
    pLock32->mLine = 0;
#endif
    K2_CpuWriteBarrier();
}

#if DEBUG_LOCK

#undef K2OSKERN_SeqLock
BOOL
K2_CALLCONV_REGS
K2OSKERN_SeqLock(
    K2OSKERN_SEQLOCK *  apLock
)
{
    return K2OSKERN_DebugSeqLock(apLock, NULL, 0);
}

BOOL 
K2_CALLCONV_REGS 
K2OSKERN_DebugSeqLock(
    K2OSKERN_SEQLOCK * apLock,
    char const *apFile,
    int aLine
)
#else
BOOL 
K2_CALLCONV_REGS
K2OSKERN_SeqLock(
    K2OSKERN_SEQLOCK *  apLock
)
#endif
{
    BOOL                            enabled;
    UINT32                          mySeq;
    K2OSKERN_SEQLOCK_INTERNAL32 *   pLock32;
    K2OSKERN_CPUCORE volatile *     pThisCore;

    pLock32 = (K2OSKERN_SEQLOCK_INTERNAL32 *)
        ((((UINT_PTR)apLock) + (K2OS_CACHELINE_BYTES - 1)) & ~(K2OS_CACHELINE_BYTES - 1));

    enabled = K2OSKERN_SetIntr(FALSE);

    if (gData.mCpuCoreCount > 1)
    {
        do {
            mySeq = pLock32->mSeqIn;
            if (mySeq == K2ATOMIC_CompareExchange(&pLock32->mSeqIn, mySeq + 1, mySeq))
                break;
            if (enabled)
            {
                K2OSKERN_SetIntr(TRUE);
                K2OSKERN_MicroStall(10);
                K2OSKERN_SetIntr(FALSE);
            }
        } while (1);

        do {
            if (pLock32->mSeqOut == mySeq)
                break;
            K2OSKERN_MicroStall(10);
        } while (1);
    }

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    pLock32->mpStackNext = (K2OSKERN_SEQLOCK_INTERNAL32 *)pThisCore->mLockStack;
    pThisCore->mLockStack = (UINT32)pLock32;
#if DEBUG_LOCK
    pLock32->mpFile = apFile;
    pLock32->mLine = aLine;
#endif

    return enabled;
}


void 
K2_CALLCONV_REGS
K2OSKERN_SeqUnlock(
    K2OSKERN_SEQLOCK *  apLock,
    BOOL                aDisp
)
{
    K2OSKERN_SEQLOCK_INTERNAL32 * pLock32;
    K2OSKERN_CPUCORE volatile *   pThisCore;

    pLock32 = (K2OSKERN_SEQLOCK_INTERNAL32 *)
        ((((UINT_PTR)apLock) + (K2OS_CACHELINE_BYTES - 1)) & ~(K2OS_CACHELINE_BYTES - 1));

    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    K2_ASSERT(pThisCore->mLockStack == (UINT_PTR)pLock32);

#if DEBUG_LOCK
    pLock32->mpFile = NULL;
    pLock32->mLine = 0;
#endif

    pThisCore->mLockStack = (UINT32)pLock32->mpStackNext;
    pLock32->mpStackNext = NULL;
    if (gData.mCpuCoreCount > 1)
    {
        pLock32->mSeqOut = pLock32->mSeqOut + 1;
        K2_CpuWriteBarrier();
    }

    if (aDisp)
        K2OSKERN_SetIntr(TRUE);
}

UINT32
KernDbg_RawEmitf(
    char const *apFormat,
    ...
)
{
    VALIST  vList;
    UINT32  result;

    K2_VASTART(vList, apFormat);

    result = K2ASC_Emitf(KernDbg_Emitter, NULL, (UINT32)-1, apFormat, vList);

    K2_VAEND(vList);

    return result;
}

void
KernDbg_RawDumpLockStack(
    K2OSKERN_CPUCORE volatile *apThisCore
)
{
    K2OSKERN_SEQLOCK_INTERNAL32 * pLock32;
#if DEBUG_LOCK
    K2OSKERN_SEQLOCK_INTERNAL32 * pAbandon;
#endif

    pLock32 = (K2OSKERN_SEQLOCK_INTERNAL32 * )apThisCore->mLockStack;
    if (NULL == pLock32)
    {
        KernDbg_RawEmitf("--NO LOCKS--\n");
        return;
    }
    KernDbg_RawEmitf("LOCK STACK:\n");
    do {
#if DEBUG_LOCK
        KernDbg_RawEmitf("  ABANDON %08X %s:%d\n", pLock32, (NULL != pLock32->mpFile) ? pLock32->mpFile : "NOFILE", pLock32->mLine);
        pAbandon = pLock32;
#else
        KernDbg_RawEmitf("  %08X", pLock32);
#endif
        pLock32 = pLock32->mpStackNext;
#if DEBUG_LOCK
        if (gData.mCpuCoreCount > 1)
        {
            pAbandon->mSeqOut = pAbandon->mSeqOut + 1;
            K2_CpuWriteBarrier();
        }
#endif

    } while (NULL != pLock32);
}
