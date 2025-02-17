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

UINT_PTR 
K2OSKERN_RWLOCK_StartAtomic(
    K2RWLOCK *apRwLock
)
{
    return K2OSKERN_SeqLock(&((K2OSKERN_THREADED_RWLOCK *)apRwLock)->SeqLock);
}

void 
K2OSKERN_RWLOCK_EndAtomic(
    K2RWLOCK *  apRwLock,
    UINT_PTR    aStartResult
)
{
    K2OSKERN_SeqUnlock(&((K2OSKERN_THREADED_RWLOCK *)apRwLock)->SeqLock, aStartResult);
}

K2RWLOCK_WAIT * 
K2OSKERN_RWLOCK_GetWait(
    K2RWLOCK *apRwLock
)
{
    K2OSKERN_THREADED_RWLOCK *  pLock;
    UINT_PTR                    ix;
    UINT_PTR                    addr;
    UINT_PTR                    v;
    K2RWLOCK_WAIT *             pWait;

    pLock = (K2OSKERN_THREADED_RWLOCK *)apRwLock;
    addr = ((((UINT_PTR)pLock->mSlotsMem) + (K2OS_CACHELINE_BYTES - 1)) / K2OS_CACHELINE_BYTES) * K2OS_CACHELINE_BYTES;
    for (ix = 0; ix < K2OSKERN_THREADED_RWLOCK_SLOT_COUNT; ix++)
    {
        v = *((UINT_PTR volatile *)addr);
        if (0 != v)
        {
            if (v == K2ATOMIC_CompareExchange((UINT_PTR volatile *)addr, 0, v))
                break;
        }
        addr += K2OS_CACHELINE_BYTES;
    }

    if (K2OSKERN_THREADED_RWLOCK_SLOT_COUNT != ix)
    {
        //
        // we captured an existing wait from a slot
        //
        return (K2RWLOCK_WAIT *)v;
    }

    //
    // we were unable to grab a wait from one of the slots
    // so we make a new one through a slow alloc and gate create
    //

    pWait = K2OS_Heap_Alloc(sizeof(K2RWLOCK_WAIT));
    if (NULL == pWait)
        return NULL;

    K2MEM_Zero(pWait, sizeof(K2RWLOCK_WAIT));
    pWait->mGateOpaque = (UINT_PTR)K2OS_Gate_Create(FALSE);
    if (0 == pWait->mGateOpaque)
    {
        K2OS_Heap_Free(pWait);
        return NULL;
    }

    return pWait;
}

void 
K2OSKERN_RWLOCK_PutWait(
    K2RWLOCK *      apRwLock,
    K2RWLOCK_WAIT * apWait
)
{
    K2OSKERN_THREADED_RWLOCK *  pLock;
    UINT_PTR                    ix;
    UINT_PTR                    addr;

    pLock = (K2OSKERN_THREADED_RWLOCK *)apRwLock;
    addr = ((((UINT_PTR)pLock->mSlotsMem) + (K2OS_CACHELINE_BYTES - 1)) / K2OS_CACHELINE_BYTES) * K2OS_CACHELINE_BYTES;
    for (ix = 0; ix < K2OSKERN_THREADED_RWLOCK_SLOT_COUNT; ix++)
    {
        if (0 == *((UINT_PTR volatile *)addr))
        {
            if (0 == K2ATOMIC_CompareExchange((UINT_PTR volatile *)addr, (UINT_PTR)apWait, 0))
            {
                // 
                // we shoved the wait into an empty slot
                //
                return;
            }
        }
        addr += K2OS_CACHELINE_BYTES;
    }

    //
    // we could not put the wait into a slot, so we need to get rid of it
    // through a slow destroy and deallocate
    //
    K2OS_Token_Destroy((K2OS_TOKEN)apWait->mGateOpaque);

    K2OS_Heap_Free(apWait);
}

void 
K2OSKERN_RWLOCK_WaitForGate(
    UINT_PTR aGateOpaque
)
{
    K2OS_WaitResult waitResult;
    K2OS_Thread_WaitOne(&waitResult, (K2OS_WAITABLE_TOKEN)aGateOpaque, K2OS_TIMEOUT_INFINITE);
}

void 
K2OSKERN_RWLOCK_SetGate(
    UINT_PTR    aGateOpaque,
    BOOL        aSetOpen
)
{
    if (aSetOpen)
    {
        K2OS_Gate_Open((K2OS_SIGNAL_TOKEN)aGateOpaque);
    }
    else
    {
        K2OS_Gate_Close((K2OS_SIGNAL_TOKEN)aGateOpaque);
    }
}

static K2RWLOCK_OPS const sgRwLockOps =
{
    K2OSKERN_RWLOCK_StartAtomic,
    K2OSKERN_RWLOCK_EndAtomic,
    K2OSKERN_RWLOCK_GetWait,
    K2OSKERN_RWLOCK_PutWait,
    K2OSKERN_RWLOCK_WaitForGate,
    K2OSKERN_RWLOCK_SetGate,
};

void 
K2OSKERN_RwLockInit(
    K2OSKERN_THREADED_RWLOCK *apLock
)
{
    K2MEM_Zero(apLock, sizeof(K2OSKERN_THREADED_RWLOCK));
    K2OSKERN_SeqInit(&apLock->SeqLock);
    K2RWLOCK_Init(&apLock->RwLock, &sgRwLockOps);
}

void 
K2OSKERN_RwLockDone(
    K2OSKERN_THREADED_RWLOCK *apLock
)
{
    UINT_PTR        ix;
    UINT_PTR        addr;
    K2RWLOCK_WAIT * pWait;

    K2RWLOCK_Done(&apLock->RwLock);

    addr = ((((UINT_PTR)apLock->mSlotsMem) + (K2OS_CACHELINE_BYTES - 1)) / K2OS_CACHELINE_BYTES) * K2OS_CACHELINE_BYTES;
    for (ix = 0; ix < K2OSKERN_THREADED_RWLOCK_SLOT_COUNT; ix++)
    {
        pWait = (K2RWLOCK_WAIT *)(*((UINT_PTR volatile *)addr));
        if (NULL != pWait)
        {
            K2OS_Token_Destroy((K2OS_TOKEN)pWait->mGateOpaque);
            K2OS_Heap_Free(pWait);
        }
    }
}

void 
K2OSKERN_RwLockReadLock(
    K2OSKERN_THREADED_RWLOCK *apLock
)
{
    K2RWLOCK_ReadLock(&apLock->RwLock);
}

void 
K2OSKERN_RwLockReadUnlock(
    K2OSKERN_THREADED_RWLOCK *apLock
)
{
    K2RWLOCK_ReadUnlock(&apLock->RwLock);
}

void 
K2OSKERN_RwLockUpgradeReadLockToWriteLock(
    K2OSKERN_THREADED_RWLOCK *apLock
)
{
    K2RWLOCK_UpgradeReadToWrite(&apLock->RwLock); 
}

void 
K2OSKERN_RwLockWriteLock(
    K2OSKERN_THREADED_RWLOCK *apLock
)
{
    K2RWLOCK_WriteLock(&apLock->RwLock);
}

void 
K2OSKERN_RwLockWriteUnlock(
    K2OSKERN_THREADED_RWLOCK *apLock
)
{
    K2RWLOCK_WriteUnlock(&apLock->RwLock);
}
