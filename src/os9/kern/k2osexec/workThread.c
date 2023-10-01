
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

#include "k2osexec.h"

typedef struct _WORKER_THREAD WORKER_THREAD;
struct _WORKER_THREAD
{
    UINT32                	    mThreadId;

    BOOL                        mIsDoingWork;
    K2OS_NOTIFY_TOKEN           mTokWorkNotify;

    UINT32                      mUserContext;

    WorkerThread_pf_WorkFunc    mfDoWork;

    WORKER_THREAD * volatile    mpPoolLink;
};

static WORKER_THREAD * volatile sgpThreadList = NULL;

void
WorkerThread_Put(
    WORKER_THREAD * apThread
)
{
    WORKER_THREAD * pHead;

    do {
        K2_ASSERT(!apThread->mIsDoingWork);
        pHead = sgpThreadList;
        apThread->mpPoolLink = pHead;
        K2_CpuFullBarrier();
    } while (pHead != (WORKER_THREAD *)K2ATOMIC_CompareExchange((UINT32 volatile *)&sgpThreadList, (UINT32)apThread, (UINT32)pHead));
}

UINT32
WorkerThread(
    void *apArg
)
{
    K2OS_WaitResult waitResult;
    WORKER_THREAD * pThis;

    pThis = (WORKER_THREAD *)apArg;

    do {
        if (!K2OS_Thread_WaitOne(&waitResult, pThis->mTokWorkNotify, K2OS_TIMEOUT_INFINITE))
        {
            break;
        }

        if (NULL != pThis->mfDoWork)
        {
            pThis->mIsDoingWork = TRUE;
            pThis->mfDoWork((void *)pThis->mUserContext);
            pThis->mIsDoingWork = FALSE;
        }

        WorkerThread_Put(pThis);

    } while (1);

    return 0;
}

WORKER_THREAD *
WorkerThread_Get(
    void
)
{
    WORKER_THREAD *     pHead;
    WORKER_THREAD *     pReadyNext;
    K2OS_THREAD_TOKEN   tokThread;

    do {
        pHead = sgpThreadList;
        K2_CpuReadBarrier();

        if (NULL == pHead)
        {
            pHead = (WORKER_THREAD *)K2OS_Heap_Alloc(sizeof(WORKER_THREAD));
            if (NULL == pHead)
            {
                return NULL;
            }

            K2MEM_Zero(pHead, sizeof(WORKER_THREAD));
            pHead->mTokWorkNotify = K2OS_Notify_Create(FALSE);
            if (NULL == pHead->mTokWorkNotify)
            {
                K2OS_Heap_Free(pHead);
                return NULL;
            }

            tokThread = K2OS_Thread_Create("WorkerThread", WorkerThread, pHead, NULL, &pHead->mThreadId);
            if (NULL == tokThread)
            {
                K2OS_Token_Destroy(pHead->mTokWorkNotify);
                K2OS_Heap_Free(pHead);
                return NULL;
            }

            K2OS_Token_Destroy(tokThread);

//            K2OSKERN_Debug("Created worker thread %d\n", pHead->mThreadId);

            break;
        }

        //
        // try to pull a thread from the thread list
        //
        pReadyNext = pHead->mpPoolLink;

    } while (pHead != (WORKER_THREAD *)K2ATOMIC_CompareExchange((UINT32 volatile *)&sgpThreadList, (UINT32)pReadyNext, (UINT32)pHead));

//    K2OSKERN_Debug("Returning worker thread %d\n", pHead->mThreadId);

    return pHead;
}

K2STAT 
WorkerThread_Exec(
    WorkerThread_pf_WorkFunc    aWorkFunc,
    void *                      apArg
)
{
    WORKER_THREAD * pWorkerThread;

    pWorkerThread = WorkerThread_Get();
    if (NULL == pWorkerThread)
    {
        return K2STAT_ERROR_OUT_OF_RESOURCES;
    }

    pWorkerThread->mfDoWork = aWorkFunc;
    pWorkerThread->mUserContext = (UINT32)apArg;

    if (!K2OS_Notify_Signal(pWorkerThread->mTokWorkNotify))
    {
        WorkerThread_Put(pWorkerThread);
    }

    return K2STAT_NO_ERROR;
}



