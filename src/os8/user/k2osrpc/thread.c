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
#include "ik2osrpc.h"

UINT_PTR
K2OSRPC_Thread(
    void *apArg
)
{
    K2OSRPC_THREAD *    pThis;
    UINT_PTR            check;

    pThis = (K2OSRPC_THREAD *)apArg;

    do {
        check = K2OS_Wait_One(pThis->mWorkNotifyToken, K2OS_TIMEOUT_INFINITE);
        K2_ASSERT(check == K2OS_THREAD_WAIT_SIGNALLED_0);

        check = pThis->mRefCount;
        K2_CpuReadBarrier();
        if (0 == check)
            break;

        pThis->mfDoWork(pThis);

        check = pThis->mRefCount;
        K2_CpuReadBarrier();

    } while (0 != check);

    K2OS_Token_Destroy(pThis->mWorkNotifyToken);

    if (NULL != pThis->mfAtExit)
    {
        pThis->mfAtExit(pThis);
    }

    return 0;
}

K2STAT
K2OSRPC_Thread_Create(
    K2OSRPC_THREAD *            apThread,
    K2OSRPC_pf_Thread_AtExit    afAtExit,
    K2OSRPC_pf_Thread_DoWork    afDoWork
)
{
    K2OS_THREAD_TOKEN   tokThread;

    K2MEM_Zero(apThread, sizeof(K2OSRPC_THREAD));
    
    apThread->mWorkNotifyToken = K2OS_Notify_Create(FALSE);
    if (NULL == apThread->mWorkNotifyToken)
    {
        return K2OS_Thread_GetLastStatus();
    }

    apThread->mRefCount = 1;
    apThread->mfAtExit = afAtExit;
    apThread->mfDoWork = afDoWork;

    tokThread = K2OS_Thread_Create(K2OSRPC_Thread, apThread, NULL, &apThread->mThreadId);
    if (NULL == tokThread)
    {
        K2OS_Token_Destroy(apThread->mWorkNotifyToken);

        return K2OS_Thread_GetLastStatus();
    }

    K2OS_Token_Destroy(tokThread);

    return K2STAT_NO_ERROR;
}

INT_PTR
K2OSRPC_Thread_AddRef(
    K2OSRPC_THREAD *    apThread
)
{
    INT_PTR result;

    result = K2ATOMIC_Inc(&apThread->mRefCount);
    K2_ASSERT(1 != result);
    return result;
}

INT_PTR
K2OSRPC_Thread_Release(
    K2OSRPC_THREAD *    apThread
)
{
    INT_PTR             result;
    K2OS_NOTIFY_TOKEN   tokNotify;

    result = apThread->mRefCount;
    K2_CpuReadBarrier();
    K2_ASSERT(0 != result);

    tokNotify = apThread->mWorkNotifyToken;

    result = K2ATOMIC_Dec(&apThread->mRefCount);
    if (0 == result)
    {
        K2OS_Notify_Signal(tokNotify);
    }

    return result;
}

void
K2OSRPC_Thread_WakeUp(
    K2OSRPC_THREAD *    apThread
)
{
    K2OS_Notify_Signal(apThread->mWorkNotifyToken);
}
