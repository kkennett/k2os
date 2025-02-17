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
KernVirtMap_CreateUser(
    K2OSKERN_OBJ_PROCESS *      apProc,
    K2OSKERN_OBJ_PAGEARRAY *    apPageArray,
    UINT32                      aStartPageOffset,
    UINT32                      aProcVirtAddr,
    UINT32                      aPageCount,
    K2OS_VirtToPhys_MapType     aMapType,
    K2OSKERN_OBJREF *           apRetMapRef
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_VIRTMAP *  pMap;
    BOOL                    disp;
    K2HEAP_NODE *           pHeapNode;
    UINT32                  pagesLeft;
    UINT32 *                pPTE;
    UINT32                  ixPage;
    UINT32                  mapAttr;
    K2OSKERN_PROCHEAP_NODE *pProcHeapNode;
    UINT32                  physAddr;

    apRetMapRef->AsAny = NULL;

    if ((aStartPageOffset >= apPageArray->mPageCount) ||
        ((apPageArray->mPageCount - aStartPageOffset) < aPageCount) ||
        (aMapType == K2OS_MapType_Invalid) ||
        (aMapType >= K2OS_MapType_Count))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    aProcVirtAddr &= K2_VA_PAGEFRAME_MASK;

    switch (aMapType)
    {
    case K2OS_MapType_Text:
        mapAttr = K2OS_MEMPAGE_ATTR_EXEC | K2OS_MEMPAGE_ATTR_READABLE;
        break;
    case K2OS_MapType_Data_ReadOnly:
        mapAttr = K2OS_MEMPAGE_ATTR_READABLE;
        break;
    case K2OS_MapType_Data_ReadWrite:
    case K2OS_MapType_Thread_Stack:
        mapAttr = K2OS_MEMPAGE_ATTR_WRITEABLE | K2OS_MEMPAGE_ATTR_READABLE;
        break;
    case K2OS_MapType_Write_Thru:
        mapAttr = K2OS_MEMPAGE_ATTR_WRITEABLE | K2OS_MEMPAGE_ATTR_READABLE | K2OS_MEMPAGE_ATTR_WRITE_THRU;
        break;
    case K2OS_MapType_MemMappedIo_ReadOnly:
        mapAttr = K2OS_MEMPAGE_ATTR_DEVICEIO | K2OS_MEMPAGE_ATTR_READABLE;
        break;
    case K2OS_MapType_MemMappedIo_ReadWrite:
        mapAttr = K2OS_MEMPAGE_ATTR_DEVICEIO | K2OS_MEMPAGE_ATTR_READWRITE;
        break;
    default:
        K2OSKERN_Panic("KernVirtMap_CreateUser unknown map type (%d)\n", aMapType);
        break;
    }
    if ((apPageArray->mUserPermit & mapAttr) != mapAttr)
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    mapAttr |= K2OS_MEMPAGE_ATTR_USER;

    pMap = (K2OSKERN_OBJ_VIRTMAP *)KernObj_Alloc(KernObj_VirtMap);
    if (NULL == pMap)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    pMap->OwnerMapTreeNode.mUserVal = aProcVirtAddr;
    pMap->mPageCount = aPageCount;
    pMap->mVirtToPhysMapType = aMapType;

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
                    K2_ASSERT(pMap->OwnerMapTreeNode.mUserVal == aProcVirtAddr);
                    K2TREE_Insert(&apProc->Virt.Locked.MapTree, aProcVirtAddr, &pMap->OwnerMapTreeNode);
                    pMap->User.mpProcHeapNode = pProcHeapNode;

                    KernObj_CreateRef(&pMap->PageArrayRef, &apPageArray->Hdr);
                    pMap->mPageArrayStartPageIx = aStartPageOffset;

                    for (ixPage = 0; ixPage < aPageCount; ixPage++)
                    {
                        physAddr = KernPageArray_PagePhys(apPageArray, aStartPageOffset + ixPage);
                        KernPte_MakePageMap(apProc, aProcVirtAddr, physAddr, mapAttr);
//                        K2OSKERN_Debug("U %08X->%08X\n", aProcVirtAddr, physAddr);
                        aProcVirtAddr += K2_VA_MEMPAGE_BYTES;
                    }

                    KernObj_CreateRef(apRetMapRef, &pMap->Hdr);

//                    K2OSKERN_Debug("Proc %2d: Virtmap %08X(%08X) create\n", apProc->mId, pMap->OwnerMapTreeNode.mUserVal, pMap->mPageCount * K2_VA_MEMPAGE_BYTES);
                }
            }
        }
    }

    K2OSKERN_SeqUnlock(&apProc->Virt.SeqLock, disp);

    if (K2STAT_IS_ERROR(stat))
    {
        KernObj_Free(&pMap->Hdr);
    }

    return stat;
}

void    
KernVirtMap_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJREF         mapRef;
    K2OS_TOKEN              virtMapToken;
    K2OSKERN_OBJREF         pageArrayRef;

    pProc = apCurThread->RefProc.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    pageArrayRef.AsAny = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &pageArrayRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_PageArray != pageArrayRef.AsAny->mObjType)
        {
//            K2OSKERN_Debug("MapCreate - token does not refer to page array\n");
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            stat = KernVirtMap_CreateUser(
                pProc,
                pageArrayRef.AsPageArray,
                pThreadPage->mSysCall_Arg1,
                pThreadPage->mSysCall_Arg3 & K2_VA_PAGEFRAME_MASK,
                pThreadPage->mSysCall_Arg2,
                pThreadPage->mSysCall_Arg4_Result3,
                &mapRef);
            if (!K2STAT_IS_ERROR(stat))
            {
                stat = KernProc_TokenCreate(pProc, mapRef.AsAny, (K2OS_TOKEN *)&virtMapToken);
                if (!K2STAT_IS_ERROR(stat))
                {
                    apCurThread->User.mSysCall_Result = (UINT32)virtMapToken;
                }
                KernObj_ReleaseRef(&mapRef);
            }
        }
        KernObj_ReleaseRef(&pageArrayRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void    
KernVirtMap_SysCall_GetInfo(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD * apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_OBJREF         mapRef;
    K2OS_THREAD_PAGE * pThreadPage;

    pProc = apCurThread->RefProc.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    mapRef.AsAny = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &mapRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_VirtMap != mapRef.AsAny->mObjType)
        {
//            K2OSKERN_Debug("MapAcqPageArray - token does not refer to a map\n");
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            apCurThread->User.mSysCall_Result = mapRef.AsVirtMap->OwnerMapTreeNode.mUserVal;
            pThreadPage->mSysCall_Arg7_Result0 = mapRef.AsVirtMap->mVirtToPhysMapType;
            pThreadPage->mSysCall_Arg6_Result1 = mapRef.AsVirtMap->mPageCount;
        }

        KernObj_ReleaseRef(&mapRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void    
KernVirtMap_SysCall_Acquire(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    UINT32                  procVirtAddr;
    UINT32                  pageIx;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJREF         mapRef;

    pProc = apCurThread->RefProc.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;
    procVirtAddr = apCurThread->User.mSysCall_Arg0;

    if ((procVirtAddr >= (K2OS_UVA_TIMER_IOPAGE_BASE)) ||
        (procVirtAddr < K2OS_UVA_LOW_BASE))
    {
        stat = K2STAT_ERROR_NOT_FOUND;
    }
    else
    {
        stat = KernProc_FindMapAndCreateRef(pProc, procVirtAddr, &mapRef, &pageIx);
        if (!K2STAT_IS_ERROR(stat))
        {
            // user is acquiring a map in their process they didn't create
            stat = KernProc_TokenCreate(pProc, mapRef.AsAny, (K2OS_TOKEN *)&apCurThread->User.mSysCall_Result);
            if (!K2STAT_IS_ERROR(stat))
            {
                pThreadPage->mSysCall_Arg7_Result0 = mapRef.AsVirtMap->mVirtToPhysMapType;
                pThreadPage->mSysCall_Arg6_Result1 = pageIx;
            }
            KernObj_ReleaseRef(&mapRef);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

