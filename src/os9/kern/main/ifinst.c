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

void    
KernIfInst_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_IFINST *       apIfInst
)
{
    BOOL disp;

    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

    K2TREE_Remove(&gData.Iface.IdTree, &apIfInst->IdTreeNode);

    // if it was public it would already have been removed from the
    // class code tree during the scheduler's ifinst departure processing
    K2_ASSERT(!apIfInst->mIsPublic);

    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    if (NULL != apIfInst->RefProc.AsAny)
    {
        KernObj_ReleaseRef(&apIfInst->RefProc);
    }

    if (NULL != apIfInst->RefMailboxOwner.AsAny)
    {
        KernObj_ReleaseRef(&apIfInst->RefMailboxOwner);
    }

    K2_ASSERT(0 == apIfInst->Hdr.RefObjList.mNodeCount);

    K2MEM_Zero(apIfInst, sizeof(K2OSKERN_OBJ_IFINST));

    KernHeap_Free(apIfInst);
}

void    
KernIfInst_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJ_IFINST *   pIfInst;
    K2STAT                  stat;
    BOOL                    disp;
    K2OS_THREAD_PAGE *      pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    pIfInst = (K2OSKERN_OBJ_IFINST *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_IFINST));
    if (NULL == pIfInst)
    {
        stat = K2STAT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        K2MEM_Zero(pIfInst, sizeof(K2OSKERN_OBJ_IFINST));
        pIfInst->Hdr.mObjType = KernObj_IfInst;
        K2LIST_Init(&pIfInst->Hdr.RefObjList);

        KernObj_CreateRef(&pIfInst->RefProc, apCurThread->User.ProcRef.AsAny);

        disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

        pIfInst->IdTreeNode.mUserVal = ++gData.Iface.mLastId;
        K2_CpuWriteBarrier();

        //
        // need to do this while locked so that nobody gets
        // an enum or lookup containing an ifinst that is
        // not being returned to the owner  (race)
        //
        stat = KernProc_TokenCreate(apCurThread->User.ProcRef.AsProc, &pIfInst->Hdr, (K2OS_TOKEN *)&apCurThread->User.mSysCall_Result);

        if (!K2STAT_IS_ERROR(stat))
        {
            K2TREE_Insert(&gData.Iface.IdTree, pIfInst->IdTreeNode.mUserVal, &pIfInst->IdTreeNode);
        }

        K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

        if (K2STAT_IS_ERROR(stat))
        {
            KernObj_ReleaseRef(&pIfInst->RefProc);
            KernHeap_Free(pIfInst);
        }
        else
        {
            pThreadPage->mSysCall_Arg7_Result0 = pIfInst->IdTreeNode.mUserVal;
            K2_ASSERT(0 != apCurThread->User.mSysCall_Result);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        pThreadPage->mLastStatus = stat;
        apCurThread->User.mSysCall_Result = 0;
    }
}

UINT32  
KernIfInst_SysCall_Fast_GetId(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF     refObj;
    K2STAT              stat;
    UINT32              result;

    refObj.AsAny = NULL;

    stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refObj);

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_BAD_TOKEN;
        return 0;
    }

    if (refObj.AsAny->mObjType == KernObj_IfInst)
    {
        result = refObj.AsIfInst->IdTreeNode.mUserVal;
    }
    else
    {
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_BAD_TOKEN;
        result = 0;
    }

    KernObj_ReleaseRef(&refObj);

    return result;
}

void    
KernIfInst_SysCall_SetMailbox(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF         refObj;
    K2OSKERN_OBJREF         refMailboxOwner;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2STAT                  stat;

    refObj.AsAny = NULL;

    pProc = apCurThread->User.ProcRef.AsProc;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refObj);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (refObj.AsAny->mObjType != KernObj_IfInst)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            refMailboxOwner.AsAny = NULL;
            if (0 != pThreadPage->mSysCall_Arg1)
            {
                stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)pThreadPage->mSysCall_Arg1, &refMailboxOwner);
                if (!K2STAT_IS_ERROR(stat))
                {
                    if (refMailboxOwner.AsAny->mObjType != KernObj_MailboxOwner)
                    {
                        stat = K2STAT_ERROR_BAD_TOKEN;
                        KernObj_ReleaseRef(&refMailboxOwner);
                    }
                }
            }
            else
            {
                stat = K2STAT_NO_ERROR;
            }

            if (!K2STAT_IS_ERROR(stat))
            {
                if (NULL != refObj.AsIfInst->RefMailboxOwner.AsAny)
                {
                    KernObj_ReleaseRef(&refObj.AsIfInst->RefMailboxOwner);
                }

                if (NULL != refMailboxOwner.AsAny)
                {
                    KernObj_CreateRef(&refObj.AsIfInst->RefMailboxOwner, refMailboxOwner.AsAny);
                    KernObj_ReleaseRef(&refMailboxOwner);
                }
            }

            K2_ASSERT(NULL == refMailboxOwner.AsAny);
        }

        KernObj_ReleaseRef(&refObj);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        pThreadPage->mLastStatus = stat;
        apCurThread->User.mSysCall_Result = 0;
    }
}

UINT32  
KernIfInst_SysCall_Fast_ContextOp(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF     refObj;
    K2OS_THREAD_PAGE *  pThreadPage;
    K2STAT              stat;
    UINT32              result;

    refObj.AsAny = NULL;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refObj);

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_BAD_TOKEN;
        return (UINT32)FALSE;
    }

    if (refObj.AsAny->mObjType == KernObj_IfInst)
    {
        result = (UINT32)TRUE;
        if (0 != pThreadPage->mSysCall_Arg1)
        {
            // set
            K2ATOMIC_Exchange(&refObj.AsIfInst->mContext, pThreadPage->mSysCall_Arg2);
        }
        else 
        {
            // get
            pThreadPage->mSysCall_Arg7_Result0 = refObj.AsIfInst->mContext;
            K2_CpuReadBarrier();
        }
    }
    else
    {
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_BAD_TOKEN;
        result = 0;
    }

    KernObj_ReleaseRef(&refObj);

    return result;
}

void    
KernIfInst_SysCall_Publish(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF         refObj;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2STAT                  stat;
    K2OSKERN_SCHED_ITEM *   pSchedItem;

    refObj.AsAny = NULL;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    if ((0 == pThreadPage->mSysCall_Arg1) ||
        (K2MEM_VerifyZero(pThreadPage->mMiscBuffer, sizeof(K2_GUID128))))
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refObj);
        if (!K2STAT_IS_ERROR(stat))
        {
            if (refObj.AsAny->mObjType != KernObj_IfInst)
            {
                stat = K2STAT_ERROR_BAD_TOKEN;
            }
            else
            {
                if (refObj.AsIfInst->mIsPublic)
                {
                    stat = K2STAT_ERROR_ALREADY_EXISTS;
                }
                else if (refObj.AsIfInst->mIsDeparting)
                {
                    stat = K2STAT_ERROR_NOT_FOUND;
                }
                else
                {
                    stat = K2STAT_NO_ERROR;
                    pSchedItem = &apCurThread->SchedItem;
                    pSchedItem->mType = KernSchedItem_Thread_SysCall;
                    pSchedItem->Args.IfInst_Publish.mClassCode = pThreadPage->mSysCall_Arg1;
                    KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
                    KernObj_CreateRef(&pSchedItem->ObjRef, refObj.AsAny);
                    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
                    KernSched_QueueItem(pSchedItem);
                }
            }

            KernObj_ReleaseRef(&refObj);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        pThreadPage->mLastStatus = stat;
        apCurThread->User.mSysCall_Result = 0;
    }
}

K2STAT
KernIfInstId_GetDetail(
    K2OS_IFINST_ID          aIfInstId,
    K2OS_IFINST_DETAIL *    apRetDetail
)
{
    BOOL                    disp;
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_OBJ_IFINST *   pIfInst;
    K2STAT                  stat;

    K2_ASSERT(NULL != apRetDetail);

    if (0 == aIfInstId)
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        stat = K2STAT_ERROR_NOT_FOUND;

        disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

        pTreeNode = K2TREE_Find(&gData.Iface.IdTree, aIfInstId);
        if (NULL != pTreeNode)
        {
            pIfInst = K2_GET_CONTAINER(K2OSKERN_OBJ_IFINST, pTreeNode, IdTreeNode);
            if ((pIfInst->mIsPublic) && (!pIfInst->mIsDeparting))
            {
                apRetDetail->mClassCode = pIfInst->mClassCode;
                apRetDetail->mInstId = pIfInst->IdTreeNode.mUserVal;
                apRetDetail->mOwningProcessId = (pIfInst->RefProc.AsAny == NULL) ? 0 : pIfInst->RefProc.AsProc->mId;
                K2MEM_Copy(&apRetDetail->SpecificGuid, &pIfInst->SpecificGuid, sizeof(K2_GUID128));
                stat = K2STAT_NO_ERROR;
            }
        }

        K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);
    }

    return stat;
}

void    
KernIfInstId_SysCall_GetDetail(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OS_THREAD_PAGE *  pThreadPage;
    K2STAT              stat;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    if (0 == apCurThread->User.mSysCall_Arg0)
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        stat = KernIfInstId_GetDetail(apCurThread->User.mSysCall_Arg0, (K2OS_IFINST_DETAIL *)pThreadPage->mMiscBuffer);
        if (!K2STAT_IS_ERROR(stat))
        {
            apCurThread->User.mSysCall_Result = (UINT32)TRUE;
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        pThreadPage->mLastStatus = stat;
        apCurThread->User.mSysCall_Result = 0;
    }
}

