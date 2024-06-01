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

#include "kern.h"

#define CRITSEC_SENTINEL1    K2_MAKEID4('k','S','e','c')
#define CRITSEC_SENTINEL2    K2_MAKEID4('S','e','c','K')

typedef struct _KernIntCritSec KernIntCritSec;
struct _KernIntCritSec
{
    UINT32 volatile     mLockOwner;
    UINT32              mRecursionCount;
    UINT32              mSentinel1;
    K2OS_SIGNAL_TOKEN   mNotifyToken;
    K2LIST_LINK         OwnerThreadOwnedCsListLink;
    UINT32              mSentinel2;
};
K2_STATIC_ASSERT(sizeof(KernIntCritSec) <= K2OS_CACHELINE_BYTES);

void
KernCritSec_Threaded_InitDeferred(
    void
)
{
    K2LIST_LINK *       pListLink;
    KernIntCritSec *    pKernSec;

    K2_ASSERT(gData.mCore0MonitorStarted);

    pListLink = gData.Thread.DeferredCsInitList.mpHead;
    if (NULL == pListLink)
        return;
    do {
        pKernSec = K2_GET_CONTAINER(KernIntCritSec, pListLink, OwnerThreadOwnedCsListLink);
        pListLink = pListLink->mpNext;

        K2_ASSERT(pKernSec->mSentinel1 == CRITSEC_SENTINEL1);
        K2_ASSERT(NULL == pKernSec->mNotifyToken);
        K2_ASSERT(pKernSec->mSentinel2 != CRITSEC_SENTINEL2);

        pKernSec->mNotifyToken = K2OS_Notify_Create(FALSE);
        K2_ASSERT(NULL != pKernSec->mNotifyToken);
        pKernSec->mSentinel2 = CRITSEC_SENTINEL2;

    } while (NULL != pListLink);

    K2_CpuWriteBarrier();

    K2LIST_Init(&gData.Thread.DeferredCsInitList);
}

BOOL 
K2OS_CritSec_Init(
    K2OS_CRITSEC *apKernSec
)
{
    KernIntCritSec *pKernSec;

    K2MEM_Zero(apKernSec, sizeof(apKernSec));

    pKernSec = (KernIntCritSec *)((((UINT32)apKernSec) + (K2OS_CACHELINE_BYTES - 1)) & (~(K2OS_CACHELINE_BYTES - 1)));

    K2_ASSERT(pKernSec->mSentinel1 != CRITSEC_SENTINEL1);
    K2_ASSERT(pKernSec->mSentinel2 != CRITSEC_SENTINEL2);

    pKernSec->mLockOwner = 0;
    pKernSec->mRecursionCount = 0;
    pKernSec->mSentinel1 = CRITSEC_SENTINEL1;
    K2_CpuWriteBarrier();

    if (!gData.mCore0MonitorStarted)
    {
        K2LIST_AddAtTail(&gData.Thread.DeferredCsInitList, &pKernSec->OwnerThreadOwnedCsListLink);
        return TRUE;
    }

    pKernSec->mNotifyToken = K2OS_Notify_Create(FALSE);
    if (NULL == pKernSec->mNotifyToken)
        return FALSE;

    pKernSec->mSentinel2 = CRITSEC_SENTINEL2;
    K2_CpuWriteBarrier();

    return TRUE;
}

BOOL 
K2OS_CritSec_TryEnter(
    K2OS_CRITSEC *apKernSec
)
{
    UINT32                  threadIx;
    UINT32                  v;
    KernIntCritSec *        pKernSec;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJ_THREAD *   pThread;

    threadIx = K2OS_Thread_GetId();

    pKernSec = (KernIntCritSec *)((((UINT32)apKernSec) + (K2OS_CACHELINE_BYTES - 1)) & (~(K2OS_CACHELINE_BYTES - 1)));

    K2_ASSERT(CRITSEC_SENTINEL1 == pKernSec->mSentinel1);
    K2_ASSERT(CRITSEC_SENTINEL2 == pKernSec->mSentinel2);

    v = pKernSec->mLockOwner;

    if (threadIx == (v >> 22))
    {
        pKernSec->mRecursionCount++;
        return TRUE;
    }

    if (v != 0)
        return FALSE;

    if (0 == K2ATOMIC_CompareExchange(&pKernSec->mLockOwner, threadIx << 22, 0))
    {
        //
        // we have the CS at recursion count 1
        //
        pKernSec->mRecursionCount = 1;
        pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_TLSAREA_BASE + (threadIx * K2_VA_MEMPAGE_BYTES));
        pThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
        K2LIST_AddAtTail(&pThread->Kern.HeldCsList, &pKernSec->OwnerThreadOwnedCsListLink);
        return TRUE;
    }

    return FALSE;
}

void 
K2OS_CritSec_Enter(
    K2OS_CRITSEC *apKernSec
)
{
    UINT32                  threadIx;
    UINT32                  v;
    KernIntCritSec *        pKernSec;
    K2OS_WaitResult         waitResult;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJ_THREAD *   pThread;

    threadIx = K2OS_Thread_GetId();

    pKernSec = (KernIntCritSec *)((((UINT32)apKernSec) + (K2OS_CACHELINE_BYTES - 1)) & (~(K2OS_CACHELINE_BYTES - 1)));

    K2_ASSERT(CRITSEC_SENTINEL1 == pKernSec->mSentinel1);
    K2_ASSERT(CRITSEC_SENTINEL2 == pKernSec->mSentinel2);

    v = pKernSec->mLockOwner;

    if (threadIx == (v >> 22))
    {
        pKernSec->mRecursionCount++;
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
            if (0 == K2ATOMIC_CompareExchange(&pKernSec->mLockOwner, threadIx << 22, 0))
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
            if (v == K2ATOMIC_CompareExchange(&pKernSec->mLockOwner, v + 1, v))
            {
                //
                // we latched our wait.  somebody will wake us up when it is our
                // turn to have the CS.  At that point the un-locker will have
                // changed the owning thread ix to 0x3FF
                //
                if (!K2OS_Thread_WaitOne(&waitResult, pKernSec->mNotifyToken, K2OS_TIMEOUT_INFINITE))
                {
                    K2OSKERN_Panic("***CS notify wait failed!\n");
                }
                K2_ASSERT(waitResult == K2OS_Wait_Signalled_0);

                //
                // if we are running here we take over the cs, which must have 
                // its owner set as 0x3FF.  we set ourselves as owner and decrement
                // the thread's waiting thread count at the same time
                //
                do
                {
                    v = pKernSec->mLockOwner;
                    K2_ASSERT(0x3FF == (v >> 22));
                    K2_ASSERT(0 != (v & 0x3FFFFF));
                } while (v != K2ATOMIC_CompareExchange(&pKernSec->mLockOwner, (((v & 0x3FFFFF) - 1) | (threadIx << 22)), v));

                break;
            }
            //
            // go around again
            //
        }

        v = pKernSec->mLockOwner;

    } while (1);

    //
    // we have the CS at recursion count 1
    //
    pKernSec->mRecursionCount = 1;
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_TLSAREA_BASE + (threadIx * K2_VA_MEMPAGE_BYTES));
    pThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2LIST_AddAtTail(&pThread->Kern.HeldCsList, &pKernSec->OwnerThreadOwnedCsListLink);
}

void 
K2OS_CritSec_Leave(
    K2OS_CRITSEC *apKernSec
)
{
    UINT32                  threadIx;
    UINT32                  v;
    KernIntCritSec *        pKernSec;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJ_THREAD *   pThread;

    threadIx = K2OS_Thread_GetId();

    pKernSec = (KernIntCritSec *)((((UINT32)apKernSec) + (K2OS_CACHELINE_BYTES - 1)) & (~(K2OS_CACHELINE_BYTES - 1)));

    K2_ASSERT(CRITSEC_SENTINEL1 == pKernSec->mSentinel1);
    K2_ASSERT(CRITSEC_SENTINEL2 == pKernSec->mSentinel2);

    v = pKernSec->mLockOwner;

    K2_ASSERT(threadIx == (v >> 22));

    if (0 != --pKernSec->mRecursionCount)
    {
        return;
    }

    //
    // leaving CS
    //
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_TLSAREA_BASE + (threadIx * K2_VA_MEMPAGE_BYTES));
    pThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2LIST_Remove(&pThread->Kern.HeldCsList, &pKernSec->OwnerThreadOwnedCsListLink);

    // 
    // leaving CS - check if it is uncontended
    //
    if (0 == (v & 0x3FFFFF))
    {
        //
        // nobody is waiting for the CS. try to set it to zero
        //
        if (v == K2ATOMIC_CompareExchange(&pKernSec->mLockOwner, 0, v))
            return;
    }

    //
    // CS is contended - get out by setting owner thread to 0x3FF
    //
    do
    {
        v = pKernSec->mLockOwner;
        K2_ASSERT(threadIx == (v >> 22));
    } while (v != K2ATOMIC_CompareExchange(&pKernSec->mLockOwner, v | 0xFFC00000, v));

    //
    // CS is marked as being left but threads are waiting so we release a thread
    //
    K2OS_Notify_Signal(pKernSec->mNotifyToken);
}

BOOL 
K2OS_CritSec_Done(
    K2OS_CRITSEC *apKernSec
)
{
    UINT32                  threadIx;
    KernIntCritSec *        pKernSec;
    K2STAT                  stat;
    UINT32                  csOwner;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJ_THREAD *   pThread;

    threadIx = K2OS_Thread_GetId();

    pKernSec = (KernIntCritSec *)((((UINT32)apKernSec) + (K2OS_CACHELINE_BYTES - 1)) & (~(K2OS_CACHELINE_BYTES - 1)));

    K2_ASSERT(CRITSEC_SENTINEL1 == pKernSec->mSentinel1);
    K2_ASSERT(CRITSEC_SENTINEL2 == pKernSec->mSentinel2);

    csOwner = pKernSec->mLockOwner >> 22;

    if (threadIx == csOwner)
    {
        // thread is destroying a CS it is currently inside
        pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_TLSAREA_BASE + (threadIx * K2_VA_MEMPAGE_BYTES));
        pThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
        K2LIST_Remove(&pThread->Kern.HeldCsList, &pKernSec->OwnerThreadOwnedCsListLink);
    }
    else if (0 != csOwner)
    {
        K2OSKERN_Panic("*** Kernel thread %d is destroying CritSec while it is held by thread %d!\n", threadIx, csOwner);
        return FALSE;
    }

    stat = K2OS_Token_Destroy(pKernSec->mNotifyToken);
    if (!K2STAT_IS_ERROR(stat))
    {
        K2MEM_Zero(apKernSec, sizeof(K2OS_CRITSEC));
        return TRUE;
    }

    return FALSE;
}

