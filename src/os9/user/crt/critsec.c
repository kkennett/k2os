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
#include "crtuser.h"

#define CRITSEC_SENTINEL    K2_MAKEID4('C','R','I','T')

typedef struct _IntCritSec IntCritSec;
struct _IntCritSec
{
    UINT32 volatile     mLockOwner;
    UINT32              mRecursionCount;
    UINT32              mSentinel;
    K2OS_SIGNAL_TOKEN   mNotifyToken;
};

BOOL 
K2OS_CritSec_Init(
    K2OS_CRITSEC *apSec
)
{
    IntCritSec *pSec;
    pSec = (IntCritSec *)((((UINT32)apSec) + (K2OS_CACHELINE_BYTES - 1)) & (~(K2OS_CACHELINE_BYTES - 1)));

    K2_ASSERT(pSec->mSentinel != CRITSEC_SENTINEL);

    pSec->mLockOwner = 0;
    pSec->mRecursionCount = 0;
    pSec->mNotifyToken = K2OS_Notify_Create(FALSE);
    if (NULL == pSec->mNotifyToken)
        return FALSE;

    pSec->mSentinel = CRITSEC_SENTINEL;
    K2_CpuWriteBarrier();

    return TRUE;
}

BOOL   
K2OS_CritSec_TryEnter(
    K2OS_CRITSEC *apSec
)
{
    UINT32                  threadIx;
    UINT32                  v;
    IntCritSec *            pSec;
    K2OS_THREAD_PAGE * pThreadPage;

    threadIx = CRT_GET_CURRENT_THREAD_INDEX;

    pSec = (IntCritSec *)((((UINT32)apSec) + (K2OS_CACHELINE_BYTES - 1)) & (~(K2OS_CACHELINE_BYTES - 1)));

    K2_ASSERT(CRITSEC_SENTINEL == pSec->mSentinel);

    v = pSec->mLockOwner;

    if (threadIx == (v >> 22))
    {
        pSec->mRecursionCount++;
        return TRUE;
    }

    if (v != 0)
        return FALSE;

    if (0 == K2ATOMIC_CompareExchange(&pSec->mLockOwner, threadIx << 22, 0))
    {
        //
        // we have the CS at recursion count 1
        //
        pSec->mRecursionCount = 1;
        pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (threadIx * K2_VA_MEMPAGE_BYTES));
        pThreadPage->mCsDepth++;
        return TRUE;
    }

    return FALSE;
}

void 
K2OS_CritSec_Enter(
    K2OS_CRITSEC *apSec
)
{
    UINT32                  threadIx;
    UINT32                  v;
    IntCritSec *            pSec;
    UINT32                waitResult;
    K2OS_THREAD_PAGE * pThreadPage;

    threadIx = CRT_GET_CURRENT_THREAD_INDEX;

    pSec = (IntCritSec *)((((UINT32)apSec) + (K2OS_CACHELINE_BYTES - 1)) & (~(K2OS_CACHELINE_BYTES - 1)));

    K2_ASSERT(CRITSEC_SENTINEL == pSec->mSentinel);

    v = pSec->mLockOwner;

    if (threadIx == (v >> 22))
    {
        pSec->mRecursionCount++;
        return;
    }

    // 
    // cs NOT currently locked by this thread
    //

    do
    {
        if (v == 0)
        {
            //
            // nobody has cs locked - try to grab it
            //
            if (0 == K2ATOMIC_CompareExchange(&pSec->mLockOwner, threadIx << 22, 0))
            {
                break;
            }
            //
            // go around again
            //
        }
        else
        {
            //
            // somebody else has cs locked. try to latch that we are waiting for it
            //
            if (v == K2ATOMIC_CompareExchange(&pSec->mLockOwner, v + 1, v))
            {
                //
                // we latched our wait.  somebody will wake us up when it is our
                // turn to have the CS.  At that point the un-locker will have
                // changed the owning thread ix to 0x3FF
                //
                if (!K2OS_Thread_WaitOne(&waitResult, pSec->mNotifyToken, K2OS_TIMEOUT_INFINITE))
                {
                    K2OS_RaiseException(K2STAT_EX_LOGIC);
                }
                K2_ASSERT(waitResult == K2OS_Wait_Signalled_0);

                //
                // if we are running here we take over the cs, which must have 
                // its owner set as 0x3FF.  we set ourselves as owner and decrement
                // the thread's waiting thread count at the same time
                //
                do
                {
                    v = pSec->mLockOwner;
                    K2_ASSERT(0x3FF == (v >> 22));
                    K2_ASSERT(0 != (v & 0x3FFFFF));
                } while (v != K2ATOMIC_CompareExchange(&pSec->mLockOwner, (((v & 0x3FFFFF) - 1) | (threadIx << 22)), v));

                break;
            }
            //
            // go around again
            //
        }

        v = pSec->mLockOwner;

    } while (1);

    //
    // we have the CS at recursion count 1
    //
    pSec->mRecursionCount = 1;
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (threadIx * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mCsDepth++;
}

void 
K2OS_CritSec_Leave(
    K2OS_CRITSEC *apSec
)
{
    UINT32                  threadIx;
    UINT32                  v;
    IntCritSec *            pSec;
    K2OS_THREAD_PAGE * pThreadPage;

    threadIx = CRT_GET_CURRENT_THREAD_INDEX;

    pSec = (IntCritSec *)((((UINT32)apSec) + (K2OS_CACHELINE_BYTES - 1)) & (~(K2OS_CACHELINE_BYTES - 1)));

    K2_ASSERT(CRITSEC_SENTINEL == pSec->mSentinel);

    v = pSec->mLockOwner;

    K2_ASSERT(threadIx == (v >> 22));

    if (0 != --pSec->mRecursionCount)
    {
        return;
    }

    //
    // leaving CS
    //
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (threadIx * K2_VA_MEMPAGE_BYTES));
    K2_ASSERT(0 != pThreadPage->mCsDepth);
    pThreadPage->mCsDepth--;

    // 
    // leaving CS - check if it is uncontended
    //
    if (0 == (v & 0x3FFFFF))
    {
        //
        // nobody is waiting for the CS. try to set it to zero
        //
        if (v == K2ATOMIC_CompareExchange(&pSec->mLockOwner, 0, v))
            return;
    }

    //
    // CS is contended - get out by setting owner thread to 0x3FF
    //
    do
    {
        v = pSec->mLockOwner;
        K2_ASSERT(threadIx == (v >> 22));
    } while (v != K2ATOMIC_CompareExchange(&pSec->mLockOwner, v | 0xFFC00000, v));

    //
    // CS is marked as being left but threads are waiting so we release a thread
    //
    K2OS_Notify_Signal(pSec->mNotifyToken);
}

BOOL 
K2OS_CritSec_Done(
    K2OS_CRITSEC *apSec
)
{
    UINT32                  threadIx;
    K2STAT                  stat;
    IntCritSec *            pSec;
    UINT32                  csOwner;

    pSec = (IntCritSec *)((((UINT32)apSec) + (K2OS_CACHELINE_BYTES - 1)) & (~(K2OS_CACHELINE_BYTES - 1)));

    K2_ASSERT(CRITSEC_SENTINEL == pSec->mSentinel);

    threadIx = CRT_GET_CURRENT_THREAD_INDEX;

    csOwner = pSec->mLockOwner >> 22;
    if ((0 != csOwner) && (threadIx != csOwner))
    {
        CrtDbg_Printf("*** Proc %d Thread %d is destroying CritSec while it is held by thread %d!\n", gProcessId, threadIx, csOwner);
        K2OS_RaiseException(K2STAT_EX_LOGIC);
    }

    stat = K2OS_Token_Destroy(pSec->mNotifyToken);
    if (!K2STAT_IS_ERROR(stat))
    {
        K2MEM_Zero(apSec, sizeof(K2OS_CRITSEC));
        return TRUE;
    }
    
    return FALSE;
}

