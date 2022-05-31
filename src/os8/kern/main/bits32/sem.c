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

K2STAT  
KernSem_Create(
    UINT_PTR            aMaxCount,
    UINT_PTR            aInitCount,
    K2OSKERN_OBJREF *   apRetsemRef
)
{
    K2OSKERN_OBJ_SEM *pSem;

    pSem = (K2OSKERN_OBJ_SEM *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_SEM));
    if (NULL == pSem)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pSem, sizeof(K2OSKERN_OBJ_SEM));

    pSem->Hdr.mObjType = KernObj_Sem;
    K2LIST_Init(&pSem->Hdr.RefObjList);
    
    pSem->mMaxCount = aMaxCount;
    pSem->SchedLocked.mCount = aInitCount;
    K2LIST_Init(&pSem->SchedLocked.MacroWaitEntryList);

    KernObj_CreateRef(apRetsemRef, &pSem->Hdr);

    return K2STAT_NO_ERROR;
}

void
KernSem_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_SEM *          apSem
)
{
    K2_ASSERT(0 == apSem->Hdr.RefObjList.mNodeCount);
    K2_ASSERT(0 == apSem->SchedLocked.MacroWaitEntryList.mNodeCount);
    K2MEM_Zero(apSem, sizeof(K2OSKERN_OBJ_SEM));
    KernHeap_Free(apSem);
}

K2STAT
KernSemUser_Create(
    K2OSKERN_OBJ_SEM *  apSem,
    UINT_PTR            aInitHold,
    K2OSKERN_OBJREF *   apRetSemUserRef
)
{
    K2OSKERN_OBJ_SEMUSER *pSemUser;

    pSemUser = (K2OSKERN_OBJ_SEMUSER *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_SEMUSER));
    if (NULL == pSemUser)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pSemUser, sizeof(K2OSKERN_OBJ_SEMUSER));

    pSemUser->Hdr.mObjType = KernObj_SemUser;
    K2LIST_Init(&pSemUser->Hdr.RefObjList);

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
    K2_ASSERT(0 == apSemUser->Hdr.RefObjList.mNodeCount);

    if (apSemUser->SchedLocked.mHeldCount == 0)
    {
        KernObj_ReleaseRef(&apSemUser->SemRef);
        K2MEM_Zero(apSemUser, sizeof(K2OSKERN_OBJ_SEMUSER));
        KernHeap_Free(apSemUser);
        return;
    }

    //
    // sem counts held when the user is destroyed.
    // need to increment sem through scheduler or held count is lost.  
    // release of sem will be through post-cleanup dpc
    //
    apSemUser->CleanupSchedItem.mType = KernSchedItem_SemUser_Cleanup;
    KernTimer_GetHfTick(&apSemUser->CleanupSchedItem.mHfTick);
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

    K2MEM_Zero(pSemUser, sizeof(K2OSKERN_OBJ_SEMUSER));
    KernHeap_Free(pSemUser);
}

void
KernSemUser_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJREF         semRef;
    K2OSKERN_OBJREF         semUserRef;
    K2OS_USER_THREAD_PAGE * pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    if ((0 == apCurThread->mSysCall_Arg0) ||
        (apCurThread->mSysCall_Arg0 < pThreadPage->mSysCall_Arg1))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        return;
    }

    semRef.Ptr.AsHdr = NULL;
    stat = KernSem_Create(apCurThread->mSysCall_Arg0, pThreadPage->mSysCall_Arg1, &semRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        semUserRef.Ptr.AsHdr = NULL;
        stat = KernSemUser_Create(semRef.Ptr.AsSem, apCurThread->mSysCall_Arg0 - pThreadPage->mSysCall_Arg1, &semUserRef);
        if (!K2STAT_IS_ERROR(stat))
        {
            stat = KernProc_TokenCreate(apCurThread->ProcRef.Ptr.AsProc, semUserRef.Ptr.AsHdr, (K2OS_TOKEN *)&apCurThread->mSysCall_Result);
            KernObj_ReleaseRef(&semUserRef);
        }
        KernObj_ReleaseRef(&semRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
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
    K2OS_USER_THREAD_PAGE * pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    if (0 == pThreadPage->mSysCall_Arg1)
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        semUserRef.Ptr.AsHdr = NULL;
        stat = KernProc_TokenTranslate(
            apCurThread->ProcRef.Ptr.AsProc,
            (K2OS_TOKEN)apCurThread->mSysCall_Arg0,
            &semUserRef
        );
        if (!K2STAT_IS_ERROR(stat))
        {
            if (KernObj_SemUser == semUserRef.Ptr.AsHdr->mObjType)
            {
                if (semUserRef.Ptr.AsSemUser->SemRef.Ptr.AsSem->mMaxCount < pThreadPage->mSysCall_Arg1)
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
                    pSchedItem->mType = KernSchedItem_Thread_SysCall;
                    KernTimer_GetHfTick(&pSchedItem->mHfTick);
                    pSchedItem->Args.Sem_Inc.mCount = pThreadPage->mSysCall_Arg1;
                    KernObj_CreateRef(&pSchedItem->ObjRef, semUserRef.Ptr.AsHdr);
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
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

