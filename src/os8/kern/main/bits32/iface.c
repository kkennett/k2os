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

void
KernIface_Init(
    void
)
{
    K2OSKERN_SeqInit(&gData.Iface.SeqLock);

    gData.Iface.Locked.mNextInstanceId = 1;
    K2TREE_Init(&gData.Iface.Locked.InstanceTree, NULL);

    gData.Iface.Locked.mNextRequestId = 1;
    K2TREE_Init(&gData.Iface.Locked.RequestTree, NULL);

    K2LIST_Init(&gData.Iface.Locked.SubsList);
}

void    
KernIfEnum_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_IFENUM *      apEnum
)
{
    K2_ASSERT(0 != (apEnum->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO));

    if (NULL != apEnum->mpEntries)
    {
        KernHeap_Free(apEnum->mpEntries);
    }

    K2MEM_Zero(apEnum, sizeof(K2OSKERN_OBJ_IFENUM));

    KernHeap_Free(apEnum);
}

void    
KernIfEnum_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    UINT32                  classCode;
    K2_GUID128 const *      pGuid;
    UINT_PTR                targetProcessId;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    UINT32 const *          pCheck;
    K2OSKERN_OBJ_IFENUM *   pEnum;
    BOOL                    disp;
    UINT32                  enumCount;
    UINT32                  enumGot;
    K2TREE_NODE *           pTreeNode;
    K2OS_IFENUM_ENTRY *     pEntries;
    K2OS_IFENUM_ENTRY *     pOut;
    K2OSKERN_IFACE *        pIface;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    targetProcessId = apCurThread->mSysCall_Arg0;
    classCode = pThreadPage->mSysCall_Arg1;
    pGuid = (K2_GUID128 const *)&pThreadPage->mSysCall_Arg2;
    pCheck = (UINT32 *)pGuid;
    if ((pCheck[0] == 0) &&
        (pCheck[1] == 0) &&
        (pCheck[2] == 0) &&
        (pCheck[3] == 0))
    {
        pGuid = NULL;
    }
    else
    {
        if (0 == classCode)
        {
            //
            // no class code specified but guid specified
            //
            apCurThread->mSysCall_Result = 0;
            pThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
            return;
        }
    }

    pEnum = (K2OSKERN_OBJ_IFENUM *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_IFENUM));
    if (NULL == pEnum)
    {
        stat = K2STAT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        stat = K2STAT_ERROR_NOT_FOUND;

        K2MEM_Zero(pEnum, sizeof(K2OSKERN_OBJ_IFENUM));

        pEnum->Hdr.mObjType = KernObj_IfEnum;
        K2LIST_Init(&pEnum->Hdr.RefObjList);

        K2OSKERN_SeqInit(&pEnum->SeqLock);

        do {
            //
            // count pass
            //
            enumCount = 0;

            disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

            if (0 < gData.Iface.Locked.InstanceTree.mNodeCount)
            {
                pTreeNode = K2TREE_FirstNode(&gData.Iface.Locked.InstanceTree);
                do {
                    pIface = K2_GET_CONTAINER(K2OSKERN_IFACE, pTreeNode, GlobalTreeNode);
                    pTreeNode = K2TREE_NextNode(&gData.Iface.Locked.InstanceTree, pTreeNode);
                    if ((0 == targetProcessId) ||
                        (pIface->mpProc->mId == targetProcessId))
                    {
                        if (0 != pIface->mClassCode)
                        {
                            if ((pIface->mRequestorProcId == 0) ||
                                (pIface->mRequestorProcId == apCurThread->ProcRef.Ptr.AsProc->mId))
                            {
                                if ((0 == classCode) ||
                                    (pIface->mClassCode == classCode))
                                {
                                    if ((NULL == pGuid) ||
                                        (0 == K2MEM_Compare(&pIface->SpecificGuid, pGuid, sizeof(K2_GUID128))))
                                    {
                                        enumCount++;
                                    }
                                }
                            }
                        }
                    }
                } while (NULL != pTreeNode);
            }

            K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

            if (0 == enumCount)
                break;

            pEntries = KernHeap_Alloc(enumCount * sizeof(K2OS_IFENUM_ENTRY));
            if (NULL == pEntries)
            {
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
                break;
            }

            //
            // gather pass
            //
            enumGot = 0;

            disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

            if (0 < gData.Iface.Locked.InstanceTree.mNodeCount)
            {
                pTreeNode = K2TREE_FirstNode(&gData.Iface.Locked.InstanceTree);
                pOut = pEntries;
                do {
                    pIface = K2_GET_CONTAINER(K2OSKERN_IFACE, pTreeNode, GlobalTreeNode);
                    pTreeNode = K2TREE_NextNode(&gData.Iface.Locked.InstanceTree, pTreeNode);
                    if ((0 == targetProcessId) ||
                        (pIface->mpProc->mId == targetProcessId))
                    {
                        if (0 != pIface->mClassCode)
                        {
                            if ((pIface->mRequestorProcId == 0) ||
                                (pIface->mRequestorProcId == apCurThread->ProcRef.Ptr.AsProc->mId))
                            {
                                if ((0 == classCode) ||
                                    (pIface->mClassCode == classCode))
                                {
                                    if ((NULL == pGuid) ||
                                        (0 == K2MEM_Compare(&pIface->SpecificGuid, pGuid, sizeof(K2_GUID128))))
                                    {
                                        enumGot++;
                                        if (enumGot > enumCount)
                                            break;
                                        pOut->mClassCode = pIface->mClassCode;
                                        K2MEM_Copy(&pOut->SpecificGuid, &pIface->SpecificGuid, sizeof(K2_GUID128));
                                        pOut->mGlobalInstanceId = pIface->GlobalTreeNode.mUserVal;
                                        pOut->mProcessId = pIface->mpProc->mId;
                                        pOut++;
                                    }
                                }
                            }
                        }
                    }
                } while (NULL != pTreeNode);
            }

            K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

            if ((enumGot > 0) && (enumGot <= enumCount))
            {
                stat = K2STAT_NO_ERROR;
                break;
            }

        } while (1);

        if (!K2STAT_IS_ERROR(stat))
        {
            pEnum->mEntryCount = enumGot;
            pEnum->mpEntries = pEntries;
            stat = KernProc_TokenCreate(apCurThread->ProcRef.Ptr.AsProc, &pEnum->Hdr, (K2OS_TOKEN *)&apCurThread->mSysCall_Result);
        }

        if (K2STAT_IS_ERROR(stat))
        {
            if (NULL != pEnum->mpEntries)
            {
                KernHeap_Free(pEnum->mpEntries);
            }
            KernHeap_Free(pEnum);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void    
KernIfEnum_SysCall_Reset(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT          stat;
    K2OSKERN_OBJREF objRef;
    BOOL            disp;

    objRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(apCurThread->ProcRef.Ptr.AsProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &objRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (objRef.Ptr.AsHdr->mObjType != KernObj_IfEnum)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            disp = K2OSKERN_SeqLock(&objRef.Ptr.AsIfEnum->SeqLock);
            objRef.Ptr.AsIfEnum->Locked.mCurrentIx = 0;
            K2OSKERN_SeqUnlock(&objRef.Ptr.AsIfEnum->SeqLock, disp);
            apCurThread->mSysCall_Result = (UINT32)TRUE;
        }
        KernObj_ReleaseRef(&objRef);
    }
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfUserThreadPage->mLastStatus = stat;
    }
}

K2STAT
KernIfEnum_CopyOut(
    K2OSKERN_OBJ_PROCESS *  apProc,
    K2OSKERN_OBJ_IFENUM *  apEnum,
    UINT32                  aUserVirt,
    UINT32 *                apIoUserCount
)
{
    K2STAT              stat;
    UINT32              userBufferBytes;
    UINT32              lockMapCount;
    K2OSKERN_OBJREF *   pLockedMapRefs;
    UINT32              map0FirstPageIx;
    UINT32              numCopy;
    BOOL                disp;
    UINT32              startIx;

    userBufferBytes = (*apIoUserCount) * sizeof(K2OS_IFENUM_ENTRY);

    lockMapCount = (userBufferBytes + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES;

    pLockedMapRefs = (K2OSKERN_OBJREF *)KernHeap_Alloc(lockMapCount * sizeof(K2OSKERN_OBJREF));
    if (NULL == pLockedMapRefs)
    {
        stat = K2STAT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        K2MEM_Zero(pLockedMapRefs, sizeof(K2OSKERN_OBJREF) * lockMapCount);
        stat = KernProc_AcqMaps(
            apProc,
            aUserVirt,
            userBufferBytes,
            TRUE,
            &lockMapCount,
            pLockedMapRefs,
            &map0FirstPageIx
        );
    }

    if (!K2STAT_IS_ERROR(stat))
    {
        disp = K2OSKERN_SeqLock(&apEnum->SeqLock);

        startIx = apEnum->Locked.mCurrentIx;

        numCopy = apEnum->mEntryCount - startIx;
        if (numCopy == 0)
        {
            stat = K2STAT_ERROR_NO_MORE_ITEMS;
        }
        else
        {
            if (numCopy > (*apIoUserCount))
            {
                numCopy = *apIoUserCount;
            }
            K2MEM_Copy((void *)aUserVirt, &apEnum->mpEntries[startIx], numCopy * sizeof(K2OS_IFENUM_ENTRY));
            apEnum->Locked.mCurrentIx += numCopy;
            *apIoUserCount = numCopy;
        }
        K2OSKERN_SeqUnlock(&apEnum->SeqLock, disp);
    }

    if (NULL != pLockedMapRefs)
    {
        for (startIx = 0; startIx < lockMapCount; startIx++)
        {
            KernObj_ReleaseRef(&pLockedMapRefs[startIx]);
        }
        KernHeap_Free(pLockedMapRefs);
    }

    return stat;
}

void    
KernIfEnum_SysCall_Next(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJREF         objRef;
    UINT32                  startIx;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    UINT32                  userVirt;
    UINT32                  userCount;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    userVirt = pThreadPage->mSysCall_Arg1;
    userCount = pThreadPage->mSysCall_Arg2;

    if ((userCount == 0) ||
        (userVirt > (K2OS_KVA_KERN_BASE - (userCount * sizeof(K2OS_IFENUM_ENTRY)))))
    {
        pThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        apCurThread->mSysCall_Result = 0;
        return;
    }

    objRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(apCurThread->ProcRef.Ptr.AsProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &objRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (objRef.Ptr.AsHdr->mObjType != KernObj_IfEnum)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            // snapshot - if at end already this saves us all the time to lock the buffer maps
            startIx = objRef.Ptr.AsIfEnum->Locked.mCurrentIx;
            K2_CpuReadBarrier();
            if (startIx == objRef.Ptr.AsIfEnum->mEntryCount)
            {
                stat = K2STAT_ERROR_NO_MORE_ITEMS;
            }
            else
            {
                stat = KernIfEnum_CopyOut(
                    apCurThread->ProcRef.Ptr.AsProc,
                    objRef.Ptr.AsIfEnum,
                    userVirt,
                    &userCount
                );
                if (!K2STAT_IS_ERROR(stat))
                {
                    pThreadPage->mSysCall_Arg7_Result0 = userCount;
                    apCurThread->mSysCall_Result = (UINT32)TRUE;
                }
            }
        }
        KernObj_ReleaseRef(&objRef);
    }
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfUserThreadPage->mLastStatus = stat;
    }
}

void    
KernIfSubs_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_IFSUBS *   pSubs;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    BOOL                    disp;
    K2OSKERN_OBJREF         refMailbox;
    UINT_PTR                backlog;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    pSubs = (K2OSKERN_OBJ_IFSUBS *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_IFSUBS));
    if (pSubs == 0)
        stat = K2STAT_ERROR_OUT_OF_MEMORY;
    else
    {
        pProc = apCurThread->ProcRef.Ptr.AsProc;
        
        refMailbox.Ptr.AsHdr = NULL;
        stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &refMailbox);
        if (!K2STAT_IS_ERROR(stat))
        {
            if (refMailbox.Ptr.AsHdr->mObjType != KernObj_Mailbox)
            {
                stat = K2STAT_ERROR_BAD_TOKEN;
            }
            else
            {
                K2MEM_Zero(pSubs, sizeof(K2OSKERN_OBJ_IFSUBS));

                backlog = pThreadPage->mSysCall_Arg6_Result1 & 0x0000FFFF;

                if (!KernMailbox_Reserve(&refMailbox.Ptr.AsMailbox->Common, backlog))
                {
                    stat = K2STAT_ERROR_OUT_OF_RESOURCES;
                }
                else
                {
                    pSubs->Hdr.mObjType = KernObj_IfSubs;
                    K2LIST_Init(&pSubs->Hdr.RefObjList);

                    KernObj_CreateRef(&pSubs->MailboxRef, refMailbox.Ptr.AsHdr);

                    pSubs->mBacklogInit = pSubs->mBacklogRemaining = backlog;

                    pSubs->mProcSelfNotify = (pThreadPage->mSysCall_Arg6_Result1 & 0xFFFF0000) ? TRUE : FALSE;

                    pSubs->mClassCode = pThreadPage->mSysCall_Arg1;
                    if ((pThreadPage->mSysCall_Arg2 != 0) &&
                        (pThreadPage->mSysCall_Arg3 != 0) &&
                        (pThreadPage->mSysCall_Arg4_Result3 != 0) &&
                        (pThreadPage->mSysCall_Arg5_Result2 != 0))
                    {
                        pSubs->mHasSpecific = TRUE;
                        K2MEM_Copy(&pSubs->Specific, &pThreadPage->mSysCall_Arg2, sizeof(K2_GUID128));
                    }

                    pSubs->mUserContext = pThreadPage->mSysCall_Arg7_Result0;

                    stat = KernProc_TokenCreate(pProc, &pSubs->Hdr, (K2OS_TOKEN *)&apCurThread->mSysCall_Result);
                    if (K2STAT_IS_ERROR(stat))
                    {
                        KernMailbox_UndoReserve(&refMailbox.Ptr.AsMailbox->Common, pSubs->mBacklogRemaining);
                        KernObj_ReleaseRef(&pSubs->MailboxRef);
                    }
                    else
                    {
                        //
                        // add to global list is the last step
                        //
                        disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
                        K2LIST_AddAtTail(&gData.Iface.Locked.SubsList, &pSubs->GlobalSubsListLink);
                        K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);
                    }
                }
            }
            KernObj_ReleaseRef(&refMailbox);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        KernHeap_Free(pSubs);
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void    
KernIfSubs_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_IFSUBS *       apSubs
)
{
    BOOL disp;

    K2_ASSERT(0 != (apSubs->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO));

    //
    // remove from global list is the first step
    //
    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
    K2LIST_Remove(&gData.Iface.Locked.SubsList, &apSubs->GlobalSubsListLink);
    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    // sanity check - this must be zero or there are outstanding references
    K2_ASSERT(apSubs->mBacklogRemaining == apSubs->mBacklogInit);

    KernMailbox_UndoReserve(&apSubs->MailboxRef.Ptr.AsMailbox->Common, apSubs->mBacklogInit);

    KernObj_ReleaseRef(&apSubs->MailboxRef);

    K2MEM_Zero(apSubs, sizeof(K2OSKERN_OBJ_IFSUBS));

    KernHeap_Free(apSubs);
}

void
KernIfInst_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_OBJREF         objRef;
    K2OSKERN_IFACE *        pIface;
    BOOL                    disp;
    K2OS_USER_THREAD_PAGE * pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    pProc = apCurThread->ProcRef.Ptr.AsProc;

    objRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &objRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if ((objRef.Ptr.AsHdr->mObjType != KernObj_Mailbox) ||
            (objRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc != pProc) ||
            (objRef.Ptr.AsMailbox->Common.SchedOnlyCanSet.mIsClosed))
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            pIface = KernHeap_Alloc(sizeof(K2OSKERN_IFACE));
            if (pIface == NULL)
            {
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
            }
            else
            {
                K2MEM_Zero(pIface, sizeof(K2OSKERN_IFACE));

                pIface->mpProc = pProc;
                KernObj_CreateRef(&pIface->MailboxRef, objRef.Ptr.AsHdr);
                pIface->mRequestorProcId = pThreadPage->mSysCall_Arg1;
                pIface->mContext = pThreadPage->mSysCall_Arg2;

                disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
                apCurThread->mSysCall_Result = pIface->GlobalTreeNode.mUserVal = gData.Iface.Locked.mNextInstanceId;
                gData.Iface.Locked.mNextInstanceId++;
                K2TREE_Insert(&gData.Iface.Locked.InstanceTree, pIface->GlobalTreeNode.mUserVal, &pIface->GlobalTreeNode);
                K2LIST_AddAtTail(&pProc->Iface.GlobalLocked.List, &pIface->ProcIfaceListLink);
                K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);
            }
        }
        KernObj_ReleaseRef(&objRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfUserThreadPage->mLastStatus = stat;
    }
}

void
KernIfInst_SysCall_Publish(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_SCHED_ITEM *   pSchedItem;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    UINT32 *                pCheck;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    pCheck = (UINT32 *)&pThreadPage->mSysCall_Arg2;
    if ((apCurThread->mSysCall_Arg0 == 0) ||
        (pThreadPage->mSysCall_Arg1 == 0) ||
        ((0 == pCheck[0]) &&
         (0 == pCheck[1]) &&
         (0 == pCheck[2]) &&
         (0 == pCheck[3])))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        return;
    }

    pSchedItem = &apCurThread->SchedItem;
    pSchedItem->mType = KernSchedItem_Thread_SysCall;
    KernTimer_GetHfTick(&pSchedItem->mHfTick);
    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
    KernSched_QueueItem(pSchedItem);
}

void
KernIfInst_SysCall_Remove(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_SCHED_ITEM * pSchedItem;

    if (apCurThread->mSysCall_Arg0 == 0)
    {
        apCurThread->mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfUserThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        return;
    }

    pSchedItem = &apCurThread->SchedItem;
    pSchedItem->mType = KernSchedItem_Thread_SysCall;
    KernTimer_GetHfTick(&pSchedItem->mHfTick);
    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
    KernSched_QueueItem(pSchedItem);
}
