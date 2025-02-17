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

K2STAT  
KernSem_Create(
    UINT32            aMaxCount,
    UINT32            aInitCount,
    K2OSKERN_OBJREF * apRetSemRef
)
{
    K2OSKERN_OBJ_SEM *pSem;

    pSem = (K2OSKERN_OBJ_SEM *)KernObj_Alloc(KernObj_Sem);
    if (NULL == pSem)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }
    
    pSem->mMaxCount = aMaxCount;
    pSem->SchedLocked.mCount = aInitCount;
    K2LIST_Init(&pSem->SchedLocked.MacroWaitEntryList);

    KernObj_CreateRef(apRetSemRef, &pSem->Hdr);

    return K2STAT_NO_ERROR;
}

void
KernSem_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_SEM *          apSem
)
{
    K2_ASSERT(0 == apSem->SchedLocked.MacroWaitEntryList.mNodeCount);
    KernObj_Free(&apSem->Hdr);
}

K2STAT
KernSemUser_Create(
    K2OSKERN_OBJ_SEM *  apSem,
    UINT32            aInitHold,
    K2OSKERN_OBJREF *   apRetSemUserRef
)
{
    K2OSKERN_OBJ_SEMUSER *pSemUser;

    pSemUser = (K2OSKERN_OBJ_SEMUSER *)KernObj_Alloc(KernObj_SemUser);
    if (NULL == pSemUser)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    KernObj_CreateRef(&pSemUser->SemRef, &apSem->Hdr);

    pSemUser->SchedLocked.mHeldCount = aInitHold;

    KernObj_CreateRef(apRetSemUserRef, &pSemUser->Hdr);

    return K2STAT_NO_ERROR;
}

void
KernSemUser_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_SEMUSER *      apSemUser
)
{
    if (apSemUser->SchedLocked.mHeldCount == 0)
    {
        KernObj_ReleaseRef(&apSemUser->SemRef);
        KernObj_Free(&apSemUser->Hdr);
        return;
    }

    //
    // sem counts held when the user is destroyed.
    // need to increment sem through scheduler or held count is lost.  
    // release of sem will be through post-cleanup dpc
    //
    apSemUser->CleanupSchedItem.mSchedItemType = KernSchedItem_SemUser_Cleanup;
    KernArch_GetHfTimerTick(&apSemUser->CleanupSchedItem.mHfTick);
    KernSched_QueueItem(&apSemUser->CleanupSchedItem);
}

void    
KernSemUser_PostCleanupDpc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apArg
)
{
    K2OSKERN_OBJ_SEMUSER *  pSemUser;

    pSemUser = K2_GET_CONTAINER(K2OSKERN_OBJ_SEMUSER, apArg, Hdr.ObjDpc.Func);
    K2_ASSERT(pSemUser->SchedLocked.mHeldCount == 0);

    KTRACE(apThisCore, 1, KTRACE_SEM_POST_CLEANUP_DPC);

    KernObj_ReleaseRef(&pSemUser->SemRef);

    KernObj_Free(&pSemUser->Hdr);
}

void
KernSemUser_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT             stat;
    K2OSKERN_OBJREF    semRef;
    K2OSKERN_OBJREF    semUserRef;
    K2OS_THREAD_PAGE * pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    if ((0 == apCurThread->User.mSysCall_Arg0) ||
        (apCurThread->User.mSysCall_Arg0 < pThreadPage->mSysCall_Arg1))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        return;
    }

    semRef.AsAny = NULL;
    stat = KernSem_Create(apCurThread->User.mSysCall_Arg0, pThreadPage->mSysCall_Arg1, &semRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        semUserRef.AsAny = NULL;
        stat = KernSemUser_Create(semRef.AsSem, apCurThread->User.mSysCall_Arg0 - pThreadPage->mSysCall_Arg1, &semUserRef);
        if (!K2STAT_IS_ERROR(stat))
        {
            stat = KernProc_TokenCreate(apCurThread->RefProc.AsProc, semUserRef.AsAny, (K2OS_TOKEN *)&apCurThread->User.mSysCall_Result);
            KernObj_ReleaseRef(&semUserRef);
        }
        KernObj_ReleaseRef(&semRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void    
KernSemUser_SysCall_Inc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJREF         semUserRef;
    K2OSKERN_SCHED_ITEM *   pSchedItem;
    K2OS_THREAD_PAGE * pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    if (0 == pThreadPage->mSysCall_Arg1)
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        semUserRef.AsAny = NULL;
        stat = KernProc_TokenTranslate(
            apCurThread->RefProc.AsProc,
            (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0,
            &semUserRef
        );
        if (!K2STAT_IS_ERROR(stat))
        {
            if (KernObj_SemUser == semUserRef.AsAny->mObjType)
            {
                if (semUserRef.AsSemUser->SemRef.AsSem->mMaxCount < pThreadPage->mSysCall_Arg1)
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
                    pSchedItem = &apCurThread->SchedItem;
                    pSchedItem->mSchedItemType = KernSchedItem_Thread_SysCall;
                    KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
                    pSchedItem->Args.Sem_Inc.mCount = pThreadPage->mSysCall_Arg1;
                    KernObj_CreateRef(&pSchedItem->ObjRef, semUserRef.AsAny);
                    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
                    KernSched_QueueItem(pSchedItem);
                }
            }
            else
            {
                stat = K2STAT_ERROR_BAD_TOKEN;
            }
            KernObj_ReleaseRef(&semUserRef);
        }
    }
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

K2STAT  
KernSem_Share(
    K2OSKERN_OBJ_SEM *      apSem,
    K2OSKERN_OBJ_PROCESS *  apTargetProc,
    UINT32 *                apRetToken
)
{
    K2OSKERN_OBJREF semUserRef;
    K2STAT          stat;

    semUserRef.AsAny = NULL;

    stat = KernSemUser_Create(apSem, 0, &semUserRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (NULL == apTargetProc)
        {
            stat = KernToken_Create(semUserRef.AsAny, (K2OS_TOKEN *)apRetToken);
        }
        else
        {
            stat = KernProc_TokenCreate(apTargetProc, semUserRef.AsAny, (K2OS_TOKEN *)apRetToken);
        }

        KernObj_ReleaseRef(&semUserRef);
    }

    return stat;
}

