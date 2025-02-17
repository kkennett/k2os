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

K2OS_SEMAPHORE_TOKEN  
K2OS_Semaphore_Create(
    UINT32 aMaxCount,
    UINT32 aInitCount
)
{
    K2OSKERN_OBJREF         semRef;
    K2STAT                  stat;
    K2OS_TOKEN              tokSemUser;
    K2OSKERN_OBJ_THREAD *   pThisThread;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJREF         semUserRef;

    if ((0 == aMaxCount) ||
        (aMaxCount < aInitCount))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    semRef.AsAny = NULL;
    stat = KernSem_Create(aMaxCount, aInitCount, &semRef);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    tokSemUser = NULL;
    semUserRef.AsAny = NULL;
    stat = KernSemUser_Create(semRef.AsSem, aMaxCount - aInitCount, &semUserRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        stat = KernToken_Create(semUserRef.AsAny, &tokSemUser);
        if (K2STAT_IS_ERROR(stat))
        {
            tokSemUser = NULL;
        }
        KernObj_ReleaseRef(&semUserRef);
    }

    KernObj_ReleaseRef(&semRef);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
    }

    return (K2OS_SEMAPHORE_TOKEN)tokSemUser;
}

BOOL                  
K2OS_Semaphore_Inc(
    K2OS_SEMAPHORE_TOKEN    aTokSemaphore,
    UINT32                  aIncCount,
    UINT32 *                apRetNewCount
)
{
    K2STAT                      stat;
    K2OSKERN_SCHED_ITEM *       pSchedItem;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OS_THREAD_PAGE *          pThreadPage;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    K2OSKERN_OBJREF             semUserRef;
    BOOL                        disp;

    if ((NULL == aTokSemaphore) || (0 == aIncCount))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_KVA_THREADPAGES_BASE + (K2OS_Thread_GetId() * K2_VA_MEMPAGE_BYTES));
    pThisThread = (K2OSKERN_OBJ_THREAD *)pThreadPage->mContext;
    K2_ASSERT(pThisThread->mIsKernelThread);

    semUserRef.AsAny = NULL;
    stat = KernToken_Translate(aTokSemaphore, &semUserRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_SemUser == semUserRef.AsAny->mObjType)
        {
            if (semUserRef.AsSemUser->SemRef.AsSem->mMaxCount < aIncCount)
            {
                stat = K2STAT_ERROR_BAD_ARGUMENT;
            }
            else
            {
                //
                // this is done through scheduler call because
                // unblocking a thread may cause this thread to
                // never execute another instruction.  so this
                // thread has to not exec again until after the sem is processed
                //
                pSchedItem = &pThisThread->SchedItem;
                pSchedItem->mSchedItemType = KernSchedItem_KernThread_SemInc;
                pSchedItem->Args.Sem_Inc.mCount = aIncCount;
                KernObj_CreateRef(&pSchedItem->ObjRef, semUserRef.AsAny);

                disp = K2OSKERN_SetIntr(FALSE);
                K2_ASSERT(disp);

                pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;

                KernThread_CallScheduler(pThisCore);

                // interrupts will be back on again here
                KernObj_ReleaseRef(&pSchedItem->ObjRef);

                if (0 != pThisThread->Kern.mSchedCall_Result)
                {
                    stat = K2STAT_NO_ERROR;
                }
                else
                {
                    stat = pThreadPage->mLastStatus;
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                }
            }
        }
        else
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }

        KernObj_ReleaseRef(&semUserRef);
    }

    return (K2STAT_IS_ERROR(stat) ? FALSE : TRUE);
}
