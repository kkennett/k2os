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

#include <lib/k2rwlock.h>
#include <lib/k2atomic.h>
#include <lib/k2mem.h>
#include <lib/k2list.h>

void 
K2RWLOCK_Init(
    K2RWLOCK *          apRwLock, 
    K2RWLOCK_OPS const *apOps
)
{
    K2MEM_Zero(apRwLock, sizeof(K2RWLOCK));
    K2MEM_Copy(&apRwLock->Ops, apOps, sizeof(K2RWLOCK_OPS));
    apRwLock->mHolder = K2RWLock_Holder_Nobody;
    K2LIST_Init(&apRwLock->WaitList);
}

void 
K2RWLOCK_Done(
    K2RWLOCK *apRwLock
)
{
    K2_ASSERT(K2RWLock_Holder_Nobody == apRwLock->mHolder);
    K2_ASSERT(0 == apRwLock->mHeldCount);
    K2_ASSERT(0 == apRwLock->WaitList.mNodeCount);
}

static
K2RWLOCK_WAIT * 
K2RWLOCK_WAIT_Acquire(
    K2RWLOCK *apRwLock
)
{
    K2RWLOCK_WAIT *pRet;

    pRet = apRwLock->Ops.GetWait(apRwLock);
    if (NULL == pRet)
        return NULL;

    K2_ASSERT(0 != pRet->mGateOpaque);

    pRet->mWaitingThreadCount = 1;

    return pRet;
}

static
void 
K2RWLOCK_WAIT_Release(
    K2RWLOCK *      apRwLock, 
    K2RWLOCK_WAIT * apWait, 
    BOOL            aWaited
)
{
    UINT_PTR left;

    left = K2ATOMIC_Dec((INT_PTR volatile *)&apWait->mWaitingThreadCount);
    if (0 != left)
        return;

    if (aWaited)
    {
        apRwLock->Ops.SetGate(apWait->mGateOpaque, FALSE);
    }

    apRwLock->Ops.PutWait(apRwLock, apWait);
}

void 
K2RWLOCK_ReadLock(
    K2RWLOCK *apLock
)
{
    K2RWLOCK_WAIT * pThisWait;
    K2RWLOCK_WAIT * pWait;
    K2LIST_LINK *   pListLink;
    UINT_PTR        atomicDisp;
    K2RWLOCK_WAIT * pDumpWait;

    pThisWait = K2RWLOCK_WAIT_Acquire(apLock);
    K2_ASSERT(NULL != pThisWait);
    pThisWait->mIsWriter = FALSE;
    pDumpWait = NULL;

    atomicDisp = apLock->Ops.StartAtomic(apLock);

    if (K2RWLock_Holder_Nobody == apLock->mHolder)
    {
        K2_ASSERT(0 == apLock->WaitList.mNodeCount);
        apLock->mHolder = K2RWLock_Holder_Reader;
        apLock->mHeldCount = 1;
        apLock->Ops.EndAtomic(apLock, atomicDisp);
        K2RWLOCK_WAIT_Release(apLock, pThisWait, FALSE);
        return;
    }

    //
    // currently held by a reader or writer
    //
    pListLink = apLock->WaitList.mpTail;
    if (NULL == pListLink)
    {
        //
        // nothing is waiting after the current holder
        //
        if (K2RWLock_Holder_Reader == apLock->mHolder)
        {
            apLock->mHeldCount++;
            apLock->Ops.EndAtomic(apLock, atomicDisp);
            K2RWLOCK_WAIT_Release(apLock, pThisWait, FALSE);
            return;
        }

        //
        // nothing is waiting and lock is held by a writer
        // so we are going to wait
        //
        K2LIST_AddAtTail(&apLock->WaitList, &pThisWait->ListLink);
    }
    else
    {
        //
        // threads are waiting after the current holder
        // the first thing waiting must be a writer, so we look
        // to see if the last thing on the chain is a group of readers
        //
        pWait = K2_GET_CONTAINER(K2RWLOCK_WAIT, pListLink, ListLink);
        if (!pWait->mIsWriter)
        {
            //
            // go with the trailing group of readers
            //
            K2ATOMIC_Inc((INT_PTR *)&pWait->mWaitingThreadCount);
            pDumpWait = pThisWait;
            pThisWait = pWait;
        }
        else
        {
            //
            // latch us as a new group of readers at the end of the chain
            //
            K2LIST_AddAtTail(&apLock->WaitList, &pThisWait->ListLink);
        }
    }

    apLock->Ops.EndAtomic(apLock, atomicDisp);

    if (NULL != pDumpWait)
    {
        K2RWLOCK_WAIT_Release(apLock, pDumpWait, FALSE);
    }

    apLock->Ops.WaitForGate(pThisWait->mGateOpaque);

    K2RWLOCK_WAIT_Release(apLock, pThisWait, TRUE);
}

void 
K2RWLOCK_ReadUnlock(
    K2RWLOCK *apLock
)
{
    K2LIST_LINK *   pListLink;
    K2RWLOCK_WAIT * pWait;
    UINT_PTR        gateOpaque;
    UINT_PTR        atomicDisp;

    atomicDisp = apLock->Ops.StartAtomic(apLock);

    K2_ASSERT(K2RWLock_Holder_Reader == apLock->mHolder);

    if (0 == --apLock->mHeldCount)
    {
        pListLink = apLock->WaitList.mpHead;
        if (NULL != pListLink)
        {
            pWait = K2_GET_CONTAINER(K2RWLOCK_WAIT, pListLink, ListLink);
            K2_ASSERT(pWait->mIsWriter);
            apLock->mHeldCount = 1;
            apLock->mHolder = K2RWLock_Holder_Writer;
            gateOpaque = pWait->mGateOpaque;
            K2LIST_Remove(&apLock->WaitList, pListLink);
        }
        else
        {
            apLock->mHolder = K2RWLock_Holder_Nobody;
            gateOpaque = 0;
        }
    }
    else
    {
        gateOpaque = 0;
    }

    apLock->Ops.EndAtomic(apLock, atomicDisp);

    if (0 != gateOpaque)
    {
        //
        // waking up the next writer
        //
        apLock->Ops.SetGate(gateOpaque, TRUE);
    }
}

void 
K2RWLOCK_UpgradeReadToWrite(
    K2RWLOCK *apLock
)
{
    K2RWLOCK_WAIT * pThisWait;
    UINT_PTR        atomicDisp;
    K2RWLOCK_WAIT * pDumpWait;

    pThisWait = K2RWLOCK_WAIT_Acquire(apLock);
    K2_ASSERT(NULL != pThisWait);
    pThisWait->mIsWriter = TRUE;
    pDumpWait = NULL;

    atomicDisp = apLock->Ops.StartAtomic(apLock);

    K2_ASSERT(K2RWLock_Holder_Reader == apLock->mHolder);

    if (0 == --apLock->mHeldCount)
    {
        //
        // we are the last reader, we are going right into the write lock
        //
        pDumpWait = pThisWait;
        pThisWait = NULL;
        apLock->mHeldCount = 1;
        apLock->mHolder = K2RWLock_Holder_Writer;
    }
    else
    {
        //
        // latch the wait at the FRONT of the wait chain
        // and we will get the write as soon as the rest of the
        // readers leave
        //
        K2LIST_AddAtHead(&apLock->WaitList, &pThisWait->ListLink);
    }

    apLock->Ops.EndAtomic(apLock, atomicDisp);

    if (NULL != pDumpWait)
    {
        K2RWLOCK_WAIT_Release(apLock, pDumpWait, FALSE);
    }
    else
    {
        K2_ASSERT(NULL != pThisWait);

        apLock->Ops.WaitForGate(pThisWait->mGateOpaque);

        K2RWLOCK_WAIT_Release(apLock, pThisWait, TRUE);
    }
}

void 
K2RWLOCK_WriteLock(
    K2RWLOCK *apLock
)
{
    K2RWLOCK_WAIT * pThisWait;
    UINT_PTR        atomicDisp;

    pThisWait = K2RWLOCK_WAIT_Acquire(apLock);
    K2_ASSERT(NULL != pThisWait);
    pThisWait->mIsWriter = TRUE;

    atomicDisp = apLock->Ops.StartAtomic(apLock);

    if (K2RWLock_Holder_Nobody == apLock->mHolder)
    {
        K2_ASSERT(0 == apLock->WaitList.mNodeCount);
        apLock->mHolder = K2RWLock_Holder_Writer;
        apLock->mHeldCount = 1;
        apLock->Ops.EndAtomic(apLock, atomicDisp);
        K2RWLOCK_WAIT_Release(apLock, pThisWait, FALSE);
        return;
    }

    //
    // currently held by a reader or writer
    //
    K2LIST_AddAtTail(&apLock->WaitList, &pThisWait->ListLink);

    apLock->Ops.EndAtomic(apLock, atomicDisp);

    apLock->Ops.WaitForGate(pThisWait->mGateOpaque);

    K2RWLOCK_WAIT_Release(apLock, pThisWait, TRUE);
}

void 
K2RWLOCK_WriteUnlock(
    K2RWLOCK *apLock
)
{
    K2LIST_LINK *   pListLink;
    K2RWLOCK_WAIT * pWait;
    UINT_PTR        gateOpaque;
    UINT_PTR        atomicDisp;

    atomicDisp = apLock->Ops.StartAtomic(apLock);
    
    K2_ASSERT(K2RWLock_Holder_Writer == apLock->mHolder);
    K2_ASSERT(1 == apLock->mHeldCount);

    pListLink = apLock->WaitList.mpHead;
    if (NULL != pListLink)
    {
        pWait = K2_GET_CONTAINER(K2RWLOCK_WAIT, pListLink, ListLink);
        if (pWait->mIsWriter)
        {
            K2_ASSERT(1 == pWait->mWaitingThreadCount);
            apLock->mHeldCount = 1;
        }
        else
        {
            apLock->mHeldCount = pWait->mWaitingThreadCount;
            apLock->mHolder = K2RWLock_Holder_Reader;
        }
        gateOpaque = pWait->mGateOpaque;
        K2LIST_Remove(&apLock->WaitList, &pWait->ListLink);
    }
    else
    {
        apLock->mHeldCount = 0;
        apLock->mHolder = K2RWLock_Holder_Nobody;
        gateOpaque = 0;
    }

    apLock->Ops.EndAtomic(apLock, atomicDisp);

    if (0 != gateOpaque)
    {
        //
        // waking up the next writer or set of readers
        //
        apLock->Ops.SetGate(gateOpaque, TRUE);
    }
}

