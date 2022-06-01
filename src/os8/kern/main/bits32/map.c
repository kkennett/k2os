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
KernMap_Create(
    K2OSKERN_OBJ_PROCESS *      apProc,
    K2OSKERN_OBJ_PAGEARRAY *    apPageArray,
    UINT32                      aStartPageOffset,
    UINT32                      aProcVirtAddr,
    UINT32                      aPageCount,
    K2OS_User_MapType           aUserMapType,
    K2OSKERN_OBJREF *           apRetMapRef
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_MAP *      pMap;
    BOOL                    disp;
    K2HEAP_NODE *           pHeapNode;
    UINT32                  pagesLeft;
    UINT32 *                pPTE;
    UINT32                  ixPage;
    UINT32                  mapAttr;
    K2OSKERN_PROCHEAP_NODE *pProcHeapNode;

    apRetMapRef->Ptr.AsHdr = NULL;

    if ((aStartPageOffset >= apPageArray->mPageCount) ||
        ((apPageArray->mPageCount - aStartPageOffset) < aPageCount) ||
        (aUserMapType == K2OS_MapType_Invalid) ||
        (aUserMapType >= K2OS_MapType_Count))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    aProcVirtAddr &= K2_VA_PAGEFRAME_MASK;

    switch (aUserMapType)
    {
    case K2OS_MapType_Text:
        mapAttr = K2OS_MEMPAGE_ATTR_EXEC | K2OS_MEMPAGE_ATTR_READABLE;
        break;
    case K2OS_MapType_Data_ReadOnly:
    case K2OS_MapType_Data_CopyOnWrite:
        mapAttr = K2OS_MEMPAGE_ATTR_READABLE;
        break;
    case K2OS_MapType_Data_ReadWrite:
    case K2OS_MapType_Thread_Stack:
        mapAttr = K2OS_MEMPAGE_ATTR_WRITEABLE | K2OS_MEMPAGE_ATTR_READABLE;
        break;
    case K2OS_MapType_MemMappedIo_ReadOnly:
        mapAttr = K2OS_MEMPAGE_ATTR_DEVICEIO | K2OS_MEMPAGE_ATTR_READABLE;
        break;
    case K2OS_MapType_MemMappedIo_ReadWrite:
        mapAttr = K2OS_MEMPAGE_ATTR_DEVICEIO | K2OS_MEMPAGE_ATTR_READWRITE;
        break;
    default:
        K2OSKERN_Panic("KernMap_Create unknown map type (%d)\n", aUserMapType);
        break;
    }
    if ((apPageArray->mUserPermit & mapAttr) != mapAttr)
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    mapAttr |= K2OS_MEMPAGE_ATTR_USER;

    pMap = (K2OSKERN_OBJ_MAP *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_MAP));
    if (NULL == pMap)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pMap, sizeof(K2OSKERN_OBJ_MAP));

    pMap->Hdr.mObjType = KernObj_Map;
    K2LIST_Init(&pMap->Hdr.RefObjList);

    pMap->mProcVirtAddr = aProcVirtAddr;
    pMap->mPageCount = aPageCount;
    pMap->mUserMapType = aUserMapType;

    disp = K2OSKERN_SeqLock(&apProc->Virt.SeqLock);

    pHeapNode = K2HEAP_FindNodeContainingAddr(&apProc->Virt.Locked.HeapAnchor, aProcVirtAddr);
    if (NULL == pHeapNode)
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        K2_ASSERT(aProcVirtAddr >= pHeapNode->AddrTreeNode.mUserVal);

        pagesLeft = (pHeapNode->SizeTreeNode.mUserVal - (aProcVirtAddr - pHeapNode->AddrTreeNode.mUserVal)) / K2_VA_MEMPAGE_BYTES;
        if (pagesLeft < aPageCount)
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            pProcHeapNode = K2_GET_CONTAINER(K2OSKERN_PROCHEAP_NODE, pHeapNode, HeapNode);
            if (pProcHeapNode->mMapCount == 0xFFFF)
            {
                //
                // virtual space is in process of being freed and cannot map here
                //
                stat = K2STAT_ERROR_BAD_ARGUMENT;
            }
            else
            {
                pPTE = (UINT32 *)K2_VA32_TO_PTE_ADDR(apProc->mVirtMapKVA, aProcVirtAddr);
                pMap->mpPte = pPTE;
                for (ixPage = 0; ixPage < aPageCount; ixPage++)
                {
                    if (0 != (*pPTE))
                        break;
                    *pPTE = K2OSKERN_PTE_MAP_CREATE;
                    pPTE++;
                }
                if (ixPage < aPageCount)
                {
                    if (ixPage > 0)
                    {
                        do
                        {
                            --ixPage;
                            --pPTE;
                            *pPTE = 0;
                        } while (ixPage > 0);
                    }
                    stat = K2STAT_ERROR_BAD_ARGUMENT;
                }
                else
                {
                    stat = K2STAT_NO_ERROR;

                    pProcHeapNode->mMapCount++;

                    KernObj_CreateRef(&pMap->ProcRef, &apProc->Hdr);
                    pMap->ProcMapTreeNode.mUserVal = aProcVirtAddr;
                    K2TREE_Insert(&apProc->Virt.Locked.MapTree, aProcVirtAddr, &pMap->ProcMapTreeNode);
                    pMap->mpProcHeapNode = pProcHeapNode;

                    KernObj_CreateRef(&pMap->PageArrayRef, &apPageArray->Hdr);
                    pMap->mPageArrayStartPageIx = aStartPageOffset;

                    for (ixPage = 0; ixPage < aPageCount; ixPage++)
                    {
                        KernPte_MakePageMap(apProc, aProcVirtAddr, KernPageArray_PagePhys(apPageArray, aStartPageOffset + ixPage), mapAttr);
                        aProcVirtAddr += K2_VA_MEMPAGE_BYTES;
                    }

                    KernObj_CreateRef(apRetMapRef, &pMap->Hdr);

//                    K2OSKERN_Debug("Create Map %08X\n", pMap);
                }
            }
        }
    }

    K2OSKERN_SeqUnlock(&apProc->Virt.SeqLock, disp);

    if (K2STAT_IS_ERROR(stat))
    {
        KernHeap_Free(pMap);
    }

    return stat;
}

void
KernMap_Purge(
    K2OSKERN_OBJ_MAP *apMap
)
{
    KernObj_ReleaseRef(&apMap->ProcRef);
    KernObj_ReleaseRef(&apMap->PageArrayRef);
    K2MEM_Zero(apMap, sizeof(K2OSKERN_OBJ_MAP));
    KernHeap_Free(apMap);
}

void    
KernMap_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJREF         mapRef;
    K2OS_MAP_TOKEN          tokMap;
    K2OSKERN_OBJREF         pageArrayRef;

    pProc = apCurThread->ProcRef.Ptr.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    pageArrayRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &pageArrayRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_PageArray != pageArrayRef.Ptr.AsHdr->mObjType)
        {
//            K2OSKERN_Debug("MapCreate - token does not refer to page array\n");
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            stat = KernMap_Create(
                pProc,
                pageArrayRef.Ptr.AsPageArray,
                pThreadPage->mSysCall_Arg1,
                pThreadPage->mSysCall_Arg3 & K2_VA_PAGEFRAME_MASK,
                pThreadPage->mSysCall_Arg2,
                pThreadPage->mSysCall_Arg4_Result3,
                &mapRef);
            if (!K2STAT_IS_ERROR(stat))
            {
                stat = KernProc_TokenCreate(pProc, mapRef.Ptr.AsHdr, &tokMap);
                if (!K2STAT_IS_ERROR(stat))
                {
                    apCurThread->mSysCall_Result = (UINT32)tokMap;
                }
                KernObj_ReleaseRef(&mapRef);
            }
        }
        KernObj_ReleaseRef(&pageArrayRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void    
KernMap_SysCall_AcqPageArray(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD * apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_OBJREF         mapRef;
    K2OS_PAGEARRAY_TOKEN    tokPageArray;
    K2OS_USER_THREAD_PAGE * pThreadPage;

    pProc = apCurThread->ProcRef.Ptr.AsProc;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    mapRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &mapRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_Map != mapRef.Ptr.AsHdr->mObjType)
        {
//            K2OSKERN_Debug("MapAcqPageArray - token does not refer to a map\n");
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            stat = KernProc_TokenCreate(pProc, mapRef.Ptr.AsMap->PageArrayRef.Ptr.AsHdr, &tokPageArray);
            if (!K2STAT_IS_ERROR(stat))
            {
                apCurThread->mSysCall_Result = (UINT32)tokPageArray;
                pThreadPage->mSysCall_Arg7_Result0 = mapRef.Ptr.AsMap->mPageArrayStartPageIx;
            }
        }
        KernObj_ReleaseRef(&mapRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void    
KernMap_SysCall_GetInfo(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD * apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_OBJREF         mapRef;
    K2OS_USER_THREAD_PAGE * pThreadPage;

    pProc = apCurThread->ProcRef.Ptr.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    mapRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &mapRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_Map != mapRef.Ptr.AsHdr->mObjType)
        {
//            K2OSKERN_Debug("MapAcqPageArray - token does not refer to a map\n");
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            apCurThread->mSysCall_Result = mapRef.Ptr.AsMap->mProcVirtAddr;
            pThreadPage->mSysCall_Arg7_Result0 = mapRef.Ptr.AsMap->mUserMapType;
            pThreadPage->mSysCall_Arg6_Result1 = mapRef.Ptr.AsMap->mPageCount;
        }

        KernObj_ReleaseRef(&mapRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void    
KernMap_SysCall_Acquire(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    UINT32                  procVirtAddr;
    UINT32                  pageIx;
    K2OS_MAP_TOKEN          tokMap;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJREF         mapRef;

    pProc = apCurThread->ProcRef.Ptr.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;
    procVirtAddr = apCurThread->mSysCall_Arg0;

    if ((procVirtAddr >= (K2OS_UVA_TIMER_IOPAGE_BASE)) ||
        (procVirtAddr < K2OS_UVA_LOW_BASE))
    {
        stat = K2STAT_ERROR_NOT_FOUND;
    }
    else
    {
        stat = KernProc_FindCreateMapRef(pProc, procVirtAddr, &mapRef, &pageIx);
        if (!K2STAT_IS_ERROR(stat))
        {
            stat = KernProc_TokenCreate(pProc, mapRef.Ptr.AsHdr, &tokMap);
            if (!K2STAT_IS_ERROR(stat))
            {
                apCurThread->mSysCall_Result = (UINT32)tokMap;
                pThreadPage->mSysCall_Arg7_Result0 = mapRef.Ptr.AsMap->mUserMapType;
                pThreadPage->mSysCall_Arg6_Result1 = pageIx;
            }

            KernObj_ReleaseRef(&mapRef);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfUserThreadPage->mLastStatus = stat;
    }
}

void
KernMap_Cleanup_VirtNotLocked_FreeVirt(
    K2OSKERN_OBJ_MAP *  apMap
)
{
    KernProc_UserVirtHeapFree(apMap->ProcRef.Ptr.AsProc, K2HEAP_NodeAddr(&apMap->mpProcHeapNode->HeapNode));
    apMap->mpProcHeapNode = NULL;
}

BOOL
KernMap_Cleanup_VirtLocked_Done(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_MAP *          apMap
)
{
    K2OSKERN_PROCHEAP_NODE * pProcHeapNode;

    //
    // return TRUE if virtual memory should be freed
    //
    pProcHeapNode = apMap->mpProcHeapNode;

    K2_ASSERT(0 != pProcHeapNode->mMapCount);

    --pProcHeapNode->mMapCount;

    if ((0 == pProcHeapNode->mMapCount) &&
        (0 == pProcHeapNode->mUserOwned))
    {
        //
        // non-user-controlled allocation hit zero mappings so
        // we are going to free it
        //
        pProcHeapNode->mMapCount = 0xFFFF;
        return TRUE;
    }

    return FALSE;
}

void
KernMap_Cleanup_CheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_MAP *      pMap;
    K2OSKERN_OBJ_PROCESS *  pProc;
    BOOL                    disp;
    BOOL                    doFree;

    pMap = K2_GET_CONTAINER(K2OSKERN_OBJ_MAP, apKey, Hdr.ObjDpc.Func);

    KTRACE(apThisCore, 1, KTRACE_MAP_CLEAN_CHECK_DPC);

    if (0 == pMap->MapFreedTlbShoot.mCoresRemaining)
    {
        pProc = pMap->ProcRef.Ptr.AsProc;
        disp = K2OSKERN_SeqLock(&pProc->Virt.SeqLock);
        doFree = KernMap_Cleanup_VirtLocked_Done(apThisCore, pMap);
        K2OSKERN_SeqUnlock(&pProc->Virt.SeqLock, disp);
        if (doFree)
        {
            KernMap_Cleanup_VirtNotLocked_FreeVirt(pMap);
        }
        KernMap_Purge(pMap);
        KTRACE(apThisCore, 1, KTRACE_MAP_CLEAN_DONE);
    }
    else
    {
        pMap->Hdr.ObjDpc.Func = KernMap_Cleanup_CheckComplete;
        KernCpu_QueueDpc(&pMap->Hdr.ObjDpc.Dpc, &pMap->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
    }
}

void
KernMap_Cleanup_SendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_MAP *  pMap;
    UINT32              sentMask;

    pMap = K2_GET_CONTAINER(K2OSKERN_OBJ_MAP, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pMap->mIciSendMask);
    
    KTRACE(apThisCore, 1, KTRACE_MAP_CLEAN_SENDICI_DPC);

    sentMask = KernArch_SendIci(
        apThisCore,
        pMap->mIciSendMask,
        KernIci_TlbInv,
        &pMap->MapFreedTlbShoot
    );

    pMap->mIciSendMask &= ~sentMask;

    if (0 == pMap->mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        pMap->Hdr.ObjDpc.Func = KernMap_Cleanup_CheckComplete;
    }
    else
    {
        pMap->Hdr.ObjDpc.Func = KernMap_Cleanup_SendIci;
    }
    KernCpu_QueueDpc(&pMap->Hdr.ObjDpc.Dpc, &pMap->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
}

BOOL
KernMap_Cleanup_VirtLocked_StartShootDown(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_MAP *          apMap
)
{
    UINT32 ix;
    UINT32 workAddr;

    if (gData.mCpuCoreCount > 1)
    {
        apMap->MapFreedTlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
        apMap->MapFreedTlbShoot.mpProc = apMap->ProcRef.Ptr.AsProc;
        apMap->MapFreedTlbShoot.mVirtBase = apMap->mProcVirtAddr;
        apMap->MapFreedTlbShoot.mPageCount = apMap->mPageCount;
        apMap->mIciSendMask = apMap->MapFreedTlbShoot.mCoresRemaining;
        KernMap_Cleanup_SendIci(apThisCore, &apMap->Hdr.ObjDpc.Func);
    }

    workAddr = apMap->mProcVirtAddr;
    for (ix = 0; ix < apMap->mPageCount; ix++)
    {
        KernArch_InvalidateTlbPageOnCurrentCore(workAddr);
        workAddr += K2_VA_MEMPAGE_BYTES;
    }

    if (gData.mCpuCoreCount == 1)
    {
        return KernMap_Cleanup_VirtLocked_Done(apThisCore, apMap);
    }

    return FALSE;
}

void    
KernMap_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_MAP *          apMap
)
{
    K2OSKERN_OBJ_PROCESS *      pProc;
    K2OSKERN_OBJ_PAGEARRAY *    pPageArray;
    BOOL                        disp;
    BOOL                        procStopped;
    UINT32                      ixPage;
    UINT32                      pagePhys;
    UINT32                      userVirtAddr;
    BOOL                        doFree;

    K2_ASSERT(0 != (apMap->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO));

    pProc = apMap->ProcRef.Ptr.AsProc;

    procStopped = (pProc->mState == KernProcState_Stopped) ? TRUE : FALSE;

    pPageArray = apMap->PageArrayRef.Ptr.AsPageArray;

    disp = K2OSKERN_SeqLock(&pProc->Virt.SeqLock);

    if (!procStopped)
    {
        //
        // remove the pte mappings from the process pagetables
        //
        userVirtAddr = apMap->mProcVirtAddr;
        for (ixPage = 0; ixPage < apMap->mPageCount; ixPage++)
        {
            pagePhys = KernPte_BreakPageMap(pProc, userVirtAddr, 0);
            K2_ASSERT(pagePhys == KernPageArray_PagePhys(pPageArray, apMap->mPageArrayStartPageIx + ixPage));
            userVirtAddr += K2_VA_MEMPAGE_BYTES;
        }

        //
        // begin tlb shootdown dpc chain
        //
        doFree = KernMap_Cleanup_VirtLocked_StartShootDown(apThisCore, apMap);
    }
    else
    {
        doFree = FALSE;
    }

    //
    // always remove from the mapping tree
    //
    K2TREE_Remove(&pProc->Virt.Locked.MapTree, &apMap->ProcMapTreeNode);

    K2OSKERN_SeqUnlock(&pProc->Virt.SeqLock, disp);

    if (procStopped)
    {
        //
        // process exited case.  no tlbshootdown or virtual free done
        //
        KernMap_Purge(apMap);
        return;
    }

    if (doFree)
    {
        //
        // single core case
        //
        KernMap_Cleanup_VirtNotLocked_FreeVirt(apMap);
        KernMap_Purge(apMap);
    }

    //
    // else DPC queued (for sending icis or for checking if they are all complete)
    //
}
