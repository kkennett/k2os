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

BOOL                
K2OS_Signal_Set(
    K2OS_SIGNAL_TOKEN aTokSignal
)
{
    K2STAT                  stat;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJ_THREAD *   pThisThread;
    K2OSKERN_OBJREF         objRef;

    if (NULL == aTokSignal)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    objRef.AsAny = NULL;
    stat = KernToken_Translate(aTokSignal, &objRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_Gate == objRef.AsAny->mObjType)
        {
            stat = KernGate_Threaded_KernelChange(pThisThread, objRef.AsGate, 1);
        }
        else if (KernObj_Notify == objRef.AsAny->mObjType)
        {
            stat = KernNotify_Threaded_KernelSignal(pThisThread, objRef.AsNotify);
        }
        else
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }

        KernObj_ReleaseRef(&objRef);
    }

    return (K2STAT_IS_ERROR(stat) ? FALSE : TRUE);
}

BOOL                
K2OS_Signal_Reset(
    K2OS_SIGNAL_TOKEN aTokSignal
)
{
    K2STAT                  stat;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJ_THREAD *   pThisThread;
    K2OSKERN_OBJREF         objRef;

    if (NULL == aTokSignal)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    objRef.AsAny = NULL;
    stat = KernToken_Translate(aTokSignal, &objRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_Gate == objRef.AsAny->mObjType)
        {
            stat = KernGate_Threaded_KernelChange(pThisThread, objRef.AsGate, 0);
        }
        else
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }

        KernObj_ReleaseRef(&objRef);
    }

    return (K2STAT_IS_ERROR(stat) ? FALSE : TRUE);
}

BOOL                
K2OS_Signal_Pulse(
    K2OS_SIGNAL_TOKEN aTokSignal
)
{
    K2STAT                  stat;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJ_THREAD *   pThisThread;
    K2OSKERN_OBJREF         objRef;

    if (NULL == aTokSignal)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    objRef.AsAny = NULL;
    stat = KernToken_Translate(aTokSignal, &objRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_Gate == objRef.AsAny->mObjType)
        {
            stat = KernGate_Threaded_KernelChange(pThisThread, objRef.AsGate, 2);
        }
        else
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }

        KernObj_ReleaseRef(&objRef);
    }

    return (K2STAT_IS_ERROR(stat) ? FALSE : TRUE);
}

