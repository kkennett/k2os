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

K2OS_IFINST_TOKEN
K2OS_IfInst_Create(
    UINT32          aContext,
    K2OS_IFINST_ID *apRetId
)
{
    K2OSKERN_OBJ_IFINST *   pIfInst;
    K2STAT                  stat;
    BOOL                    disp;
    K2OS_IFINST_TOKEN       tokResult;

    pIfInst = (K2OSKERN_OBJ_IFINST *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_IFINST));
    if (NULL == pIfInst)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    K2MEM_Zero(pIfInst, sizeof(K2OSKERN_OBJ_IFINST));
    pIfInst->Hdr.mObjType = KernObj_IfInst;
    K2LIST_Init(&pIfInst->Hdr.RefObjList);

    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

    pIfInst->IdTreeNode.mUserVal = ++gData.Iface.mLastId;
    K2_CpuWriteBarrier();

    //
    // need to do this while locked so that nobody gets
    // an enum or lookup containing an ifinst that is
    // not being returned to the owner  (race)
    //
    tokResult = NULL;
    stat = KernToken_Create(&pIfInst->Hdr, &tokResult);
    if (!K2STAT_IS_ERROR(stat))
    {
        K2TREE_Insert(&gData.Iface.IdTree, pIfInst->IdTreeNode.mUserVal, &pIfInst->IdTreeNode);
        if (NULL != apRetId)
        {
            *apRetId = pIfInst->IdTreeNode.mUserVal;
        }
    }

    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    if (NULL == tokResult)
    {
        KernHeap_Free(pIfInst);
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    return tokResult;
}

K2OS_IFINST_ID
K2OS_IfInst_GetId(
    K2OS_IFINST_TOKEN aTokIfInst
)
{
    K2OSKERN_OBJREF refObj;
    K2STAT          stat;
    UINT32          result;

    if (NULL == aTokIfInst)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    refObj.AsAny = NULL;
    stat = KernToken_Translate(aTokIfInst, &refObj);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return 0;
    }

    if (refObj.AsAny->mObjType == KernObj_IfInst)
    {
        result = refObj.AsIfInst->IdTreeNode.mUserVal;
    }
    else
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        result = 0;
    }

    KernObj_ReleaseRef(&refObj);

    return result;
}

BOOL
K2OS_IfInst_SetMailbox(
    K2OS_IFINST_TOKEN   aTokIfInst,
    K2OS_MAILBOX_TOKEN  aTokMailbox
)
{
    K2OSKERN_OBJREF refObj;
    K2OSKERN_OBJREF refMailboxOwner;
    K2STAT          stat;
    BOOL            result;

    if (NULL == aTokIfInst)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    refObj.AsAny = NULL;
    stat = KernToken_Translate(aTokIfInst, &refObj);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return FALSE;
    }

    result = FALSE;

    if (refObj.AsAny->mObjType != KernObj_IfInst)
    {
        stat = K2STAT_ERROR_BAD_TOKEN;
    }
    else
    {
        refMailboxOwner.AsAny = NULL;
        if (NULL != aTokMailbox)
        {
            stat = KernToken_Translate(aTokMailbox, &refMailboxOwner);
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
            result = TRUE;

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

    if (!result)
    {
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OS_Thread_SetLastStatus(stat);
    }

    return result;
}

BOOL
K2OS_IfInst_GetContext(
    K2OS_IFINST_TOKEN   aTokIfInst,
    UINT32 *            apRetContext
)
{
    K2OSKERN_OBJREF refObj;
    K2STAT          stat;
    UINT32          result;

    if (NULL == aTokIfInst)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    refObj.AsAny = NULL;
    stat = KernToken_Translate(aTokIfInst, &refObj);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return 0;
    }

    if (refObj.AsAny->mObjType == KernObj_IfInst)
    {
        result = TRUE;
        if (NULL != apRetContext)
        {
            *apRetContext = refObj.AsIfInst->mContext;
            K2_CpuReadBarrier();
        }
    }
    else
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        result = 0;
    }

    KernObj_ReleaseRef(&refObj);

    return result;
}

BOOL
K2OS_IfInst_SetContext(
    K2OS_IFINST_TOKEN   aTokIfInst,
    UINT32              aContext
)
{
    K2OSKERN_OBJREF refObj;
    K2STAT          stat;
    UINT32          result;

    if (NULL == aTokIfInst)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    refObj.AsAny = NULL;
    stat = KernToken_Translate(aTokIfInst, &refObj);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return 0;
    }

    if (refObj.AsAny->mObjType == KernObj_IfInst)
    {
        result = TRUE;
        K2ATOMIC_Exchange(&refObj.AsIfInst->mContext, aContext);
    }
    else
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        result = 0;
    }

    KernObj_ReleaseRef(&refObj);

    return result;
}

BOOL
K2OS_IfInst_Publish(
    K2OS_IFINST_TOKEN   aTokIfInst,
    UINT32              aClassCode,
    K2_GUID128 const *  apSpecific
)
{
    K2OSKERN_OBJREF             refObj;
    K2STAT                      stat;
    K2OSKERN_SCHED_ITEM *       pSchedItem;
    BOOL                        disp;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD *       pThisThread;

    refObj.AsAny = NULL;

    if ((0 == aClassCode) ||
        (K2MEM_VerifyZero(apSpecific, sizeof(K2_GUID128))))
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        stat = KernToken_Translate(aTokIfInst, &refObj);
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
                    disp = K2OSKERN_SetIntr(FALSE);
                    K2_ASSERT(disp);

                    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
                    pThisThread = pThisCore->mpActiveThread;
                    K2_ASSERT(pThisThread->mIsKernelThread);

                    pSchedItem = &pThisThread->SchedItem;
                    pSchedItem->mType = KernSchedItem_KernThread_IfInstPublish;
                    pSchedItem->Args.IfInst_Publish.mClassCode = aClassCode;
                    K2MEM_Copy(&pThisThread->mpKernRwViewOfThreadPage->mMiscBuffer, apSpecific, sizeof(K2_GUID128));

                    KernObj_CreateRef(&pSchedItem->ObjRef, refObj.AsAny);

                    KernThread_CallScheduler(pThisCore);

                    // interrupts will be back on again here
                    KernObj_ReleaseRef(&pSchedItem->ObjRef);

                    if (0 == pThisThread->Kern.mSchedCall_Result)
                    {
                        stat = pThisThread->mpKernRwViewOfThreadPage->mLastStatus;
                    }
                    else
                    {
                        stat = K2STAT_NO_ERROR;
                    }
                }
            }

            KernObj_ReleaseRef(&refObj);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL
K2OS_IfInstId_GetDetail(
    K2OS_IFINST_ID      aIfInstId,
    K2OS_IFINST_DETAIL *apDetail
)
{
    K2STAT stat;

    stat = KernIfInstId_GetDetail(aIfInstId, apDetail);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}
