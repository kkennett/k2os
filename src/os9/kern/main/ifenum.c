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
KernIfEnum_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_IFENUM *       apIfEnum
)
{
    K2_ASSERT(0 != (apIfEnum->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO));

    if (NULL != apIfEnum->mpEntries)
    {
        KernHeap_Free(apIfEnum->mpEntries);
    }

    K2MEM_Zero(apIfEnum, sizeof(K2OSKERN_OBJ_IFENUM));

    KernHeap_Free(apIfEnum);
}

K2STAT
KernIfEnum_Create(
    BOOL                aOneProc,
    UINT32              aTargetProcessId,
    UINT32              aClassCode,
    K2_GUID128 const *  apGuid,
    K2OSKERN_OBJREF *   apRetRef
)
{
    K2STAT                  stat;
    UINT32 const *          pCheck;
    K2OSKERN_OBJ_IFENUM *   pEnum;
    BOOL                    disp;
    UINT32                  enumCount;
    UINT32                  enumGot;
    K2TREE_NODE *           pTreeNode;
    K2OS_IFINST_DETAIL *    pEntries;
    K2OS_IFINST_DETAIL *    pOut;
    K2OSKERN_OBJ_IFINST *   pIfInst;

    pCheck = (UINT32 const *)apGuid;
    if ((pCheck[0] == 0) &&
        (pCheck[1] == 0) &&
        (pCheck[2] == 0) &&
        (pCheck[3] == 0))
    {
        apGuid = NULL;
    }
    else
    {
        if (0 == aClassCode)
        {
            return K2STAT_ERROR_BAD_ARGUMENT;
        }
    }

    pEnum = (K2OSKERN_OBJ_IFENUM *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_IFENUM));
    if (NULL == pEnum)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

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

        if (0 < gData.Iface.IdTree.mNodeCount)
        {
            pTreeNode = K2TREE_FirstNode(&gData.Iface.IdTree);
            do {
                pIfInst = K2_GET_CONTAINER(K2OSKERN_OBJ_IFINST, pTreeNode, IdTreeNode);
                pTreeNode = K2TREE_NextNode(&gData.Iface.IdTree, pTreeNode);
                if ((pIfInst->mIsPublic) && (!pIfInst->mIsDeparting))
                {
                    if ((!aOneProc) ||
                        ((pIfInst->RefProc.AsAny == NULL) && (0 == aTargetProcessId)) ||
                        ((pIfInst->RefProc.AsAny != NULL) && (pIfInst->RefProc.AsProc->mId == aTargetProcessId)))
                    {
                        if ((0 == aClassCode) ||
                            (pIfInst->mClassCode == aClassCode))
                        {
                            if ((NULL == apGuid) ||
                                (0 == K2MEM_Compare(&pIfInst->SpecificGuid, apGuid, sizeof(K2_GUID128))))
                            {
                                enumCount++;
                            }
                        }
                    }
                }
            } while (NULL != pTreeNode);
        }

        K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

        if (0 == enumCount)
            break;

        pEntries = (K2OS_IFINST_DETAIL *)KernHeap_Alloc(enumCount * sizeof(K2OS_IFINST_DETAIL));
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

        if (0 < gData.Iface.IdTree.mNodeCount)
        {
            pTreeNode = K2TREE_FirstNode(&gData.Iface.IdTree);
            pOut = pEntries;
            do {
                pIfInst = K2_GET_CONTAINER(K2OSKERN_OBJ_IFINST, pTreeNode, IdTreeNode);
                pTreeNode = K2TREE_NextNode(&gData.Iface.IdTree, pTreeNode);
                if ((pIfInst->mIsPublic) && (!pIfInst->mIsDeparting))
                {
                    if ((!aOneProc) ||
                        ((pIfInst->RefProc.AsAny == NULL) && (0 == aTargetProcessId)) ||
                        ((pIfInst->RefProc.AsAny != NULL) && (pIfInst->RefProc.AsProc->mId == aTargetProcessId)))
                    {
                        if ((0 == aClassCode) ||
                            (pIfInst->mClassCode == aClassCode))
                        {
                            if ((NULL == apGuid) ||
                                (0 == K2MEM_Compare(&pIfInst->SpecificGuid, apGuid, sizeof(K2_GUID128))))
                            {
                                enumGot++;
                                if (enumGot > enumCount)
                                    break;
                                pOut->mClassCode = pIfInst->mClassCode;
                                K2MEM_Copy(&pOut->SpecificGuid, &pIfInst->SpecificGuid, sizeof(K2_GUID128));
                                pOut->mInstId = pIfInst->IdTreeNode.mUserVal;
                                pOut->mOwningProcessId = (pIfInst->RefProc.AsAny == NULL) ? 0 : pIfInst->RefProc.AsProc->mId;
                                pOut++;
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
        KernObj_CreateRef(apRetRef, &pEnum->Hdr);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        if (NULL != pEnum->mpEntries)
        {
            KernHeap_Free(pEnum->mpEntries);
        }
        KernHeap_Free(pEnum);
    }

    return stat;
}

void    
KernIfEnum_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJREF         enumRef;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    enumRef.AsAny = NULL;
    stat = KernIfEnum_Create(
        (apCurThread->User.mSysCall_Arg0 != 0) ? TRUE : FALSE,
        pThreadPage->mSysCall_Arg1,
        pThreadPage->mSysCall_Arg2,
        (K2_GUID128 const *)&pThreadPage->mSysCall_Arg3,
        &enumRef
    );

    if (!K2STAT_IS_ERROR(stat))
    {
        stat = KernProc_TokenCreate(apCurThread->User.ProcRef.AsProc, enumRef.AsAny, (K2OS_TOKEN *)&apCurThread->User.mSysCall_Result);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
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

    objRef.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &objRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (objRef.AsAny->mObjType != KernObj_IfEnum)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            disp = K2OSKERN_SeqLock(&objRef.AsIfEnum->SeqLock);
            objRef.AsIfEnum->Locked.mCurrentIx = 0;
            K2OSKERN_SeqUnlock(&objRef.AsIfEnum->SeqLock, disp);
            apCurThread->User.mSysCall_Result = (UINT32)TRUE;
        }
        KernObj_ReleaseRef(&objRef);
    }
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
}

K2STAT
KernIfEnum_CopyOut(
    K2OSKERN_OBJ_PROCESS *  apProc,
    K2OSKERN_OBJ_IFENUM *   apEnum,
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

    userBufferBytes = (*apIoUserCount) * sizeof(K2OS_IFINST_DETAIL);

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
            K2MEM_Copy((void *)aUserVirt, &apEnum->mpEntries[startIx], numCopy * sizeof(K2OS_IFINST_DETAIL));
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
    K2STAT              stat;
    K2OSKERN_OBJREF     objRef;
    UINT32              startIx;
    K2OS_THREAD_PAGE *  pThreadPage;
    UINT32              userVirt;
    UINT32              userCount;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    userVirt = pThreadPage->mSysCall_Arg1;
    userCount = pThreadPage->mSysCall_Arg2;

    if ((userCount == 0) ||
        (userVirt > (K2OS_KVA_KERN_BASE - (userCount * sizeof(K2OS_IFINST_DETAIL)))))
    {
        pThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        apCurThread->User.mSysCall_Result = 0;
        return;
    }

    objRef.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &objRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (objRef.AsAny->mObjType != KernObj_IfEnum)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            // snapshot - if at end already this saves us all the time to lock the buffer maps
            startIx = objRef.AsIfEnum->Locked.mCurrentIx;
            K2_CpuReadBarrier();
            if (startIx == objRef.AsIfEnum->mEntryCount)
            {
                stat = K2STAT_ERROR_NO_MORE_ITEMS;
            }
            else
            {
                stat = KernIfEnum_CopyOut(
                    apCurThread->User.ProcRef.AsProc,
                    objRef.AsIfEnum,
                    userVirt,
                    &userCount
                );
                if (!K2STAT_IS_ERROR(stat))
                {
                    pThreadPage->mSysCall_Arg7_Result0 = userCount;
                    apCurThread->User.mSysCall_Result = (UINT32)TRUE;
                }
            }
        }
        KernObj_ReleaseRef(&objRef);
    }
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
}