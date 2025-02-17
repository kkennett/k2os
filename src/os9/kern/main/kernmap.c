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
KernVirtMap_Create(
    K2OSKERN_OBJ_PAGEARRAY *apPageArray,
    UINT32                  aPageOffset,
    UINT32                  aPageCount,
    UINT32                  aVirtAddr,
    K2OS_VirtToPhys_MapType aMapType,
    K2OSKERN_OBJREF *       apRetRef
)
{
    K2OSKERN_VIRTHEAP_NODE *    pKernHeapNode;
    K2STAT                      stat;
    K2OSKERN_OBJ_VIRTMAP *      pVirtMap;
    UINT32                      mapAttr;
    UINT32                      pagesLeft;
    UINT32 *                    pPTE;
    BOOL                        disp;
    UINT32                      ixPage;
    UINT32                      physAddr;

    pKernHeapNode = NULL;
    if (!KernVirt_AddRefContaining(aVirtAddr, &pKernHeapNode))
    {
        return K2STAT_ERROR_NOT_FOUND;
    }
    K2_ASSERT(NULL != pKernHeapNode);

    pagesLeft = (K2HEAP_NodeSize(&pKernHeapNode->HeapNode) - (aVirtAddr - K2HEAP_NodeAddr(&pKernHeapNode->HeapNode))) / K2_VA_MEMPAGE_BYTES;
    if (pagesLeft < aPageCount)
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        pVirtMap = (K2OSKERN_OBJ_VIRTMAP *)KernObj_Alloc(KernObj_VirtMap);
        if (NULL == pVirtMap)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
        }
        else
        {
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
                K2OSKERN_Panic("KernVirtMap_Create unknown map type (%d)\n", aMapType);
                break;
            }

            pVirtMap->mVirtToPhysMapType = aMapType;
            pVirtMap->mPageArrayStartPageIx = aPageOffset;
            pVirtMap->mPageCount = aPageCount;
            pVirtMap->Kern.mSizeBytes = aPageCount * K2_VA_MEMPAGE_BYTES;
            pVirtMap->Kern.mSegType = KernSeg_Dyn;

            pPTE = pVirtMap->mpPte = (UINT32 *)K2OS_KVA_TO_PTE_ADDR(aVirtAddr);

            disp = K2OSKERN_SeqLock(&gData.Virt.HeapSeqLock);

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
            }

            K2OSKERN_SeqUnlock(&gData.Virt.HeapSeqLock, disp);

            if (ixPage == aPageCount)
            {
                stat = K2STAT_NO_ERROR;

                KernObj_CreateRef(&pVirtMap->PageArrayRef, &apPageArray->Hdr);

                pVirtMap->OwnerMapTreeNode.mUserVal = aVirtAddr;

                for (ixPage = 0; ixPage < aPageCount; ixPage++)
                {
                    physAddr = KernPageArray_PagePhys(apPageArray, aPageOffset + ixPage);
                    KernPte_MakePageMap(NULL, aVirtAddr, physAddr, mapAttr);
//                    K2OSKERN_Debug("K %08X -> %08X\n", aVirtAddr, physAddr);
                    aVirtAddr += K2_VA_MEMPAGE_BYTES;
                }

                //
                // all done. as soon as this is inserted it may be used
                //
//                K2OSKERN_Debug("Kern   : Virtmap %08X(%08X) create\n", pVirtMap->OwnerMapTreeNode.mUserVal, pVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES);
                disp = K2OSKERN_SeqLock(&gData.VirtMap.SeqLock);
                K2TREE_Insert(&gData.VirtMap.Tree, pVirtMap->OwnerMapTreeNode.mUserVal, &pVirtMap->OwnerMapTreeNode);
                K2OSKERN_SeqUnlock(&gData.VirtMap.SeqLock, disp);
            }
            else
            {
                stat = K2STAT_ERROR_BAD_ARGUMENT;
                KernObj_Free(&pVirtMap->Hdr);
            }
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        // release ref taken on virt heap node (addrefcontaining above)
        KernVirt_Release(K2HEAP_NodeAddr(&pKernHeapNode->HeapNode));
    }
    else
    {
        K2_ASSERT(NULL != pVirtMap);
        KernObj_CreateRef(apRetRef, &pVirtMap->Hdr);
    }

    return stat;
}

void
KernVirtMap_CreatePreMap(
    UINT32                  aVirtAddr,
    UINT32                  aPageCount,
    K2OS_VirtToPhys_MapType aMapType,
    K2OSKERN_OBJREF *       apRetRef
)
{
    K2OSKERN_OBJREF         pageArrayRef;
    K2STAT                  stat;
    K2OSKERN_OBJ_VIRTMAP *  pVirtMap;
    BOOL                    disp;

    K2_ASSERT(0 == (aVirtAddr & K2_VA_MEMPAGE_OFFSET_MASK));
    K2_ASSERT(aPageCount > 0);

    pageArrayRef.AsAny = NULL;
    stat = KernPageArray_CreatePreMap(aVirtAddr, aPageCount, 0, &pageArrayRef);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Panic("*** Error %08X creating premapped page array\n", stat);
    }

    pVirtMap = (K2OSKERN_OBJ_VIRTMAP *)KernObj_Alloc(KernObj_VirtMap);
    if (NULL == pVirtMap)
    {
        K2OSKERN_Panic("*** Out of memory creating premapped virtmap\n");
    }

    pVirtMap->Hdr.mObjFlags = K2OSKERN_OBJ_FLAG_PERMANENT;

    KernObj_CreateRef(&pVirtMap->PageArrayRef, pageArrayRef.AsAny);
    pVirtMap->mPageArrayStartPageIx = 0;
    pVirtMap->mPageCount = aPageCount;
    pVirtMap->mVirtToPhysMapType = aMapType;
    pVirtMap->Kern.mSegType = KernSeg_PreMap;
    pVirtMap->Kern.mSizeBytes = aPageCount * K2_VA_MEMPAGE_BYTES;
    pVirtMap->mpPte = pageArrayRef.AsPageArray->Data.PreMap.mpPTEs;
    pVirtMap->OwnerMapTreeNode.mUserVal = aVirtAddr;

//    K2OSKERN_Debug("Kern   : Virtmap %08X(%08X) create\n", pVirtMap->OwnerMapTreeNode.mUserVal, pVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES);
    disp = K2OSKERN_SeqLock(&gData.VirtMap.SeqLock);
    K2TREE_Insert(&gData.VirtMap.Tree, pVirtMap->OwnerMapTreeNode.mUserVal, &pVirtMap->OwnerMapTreeNode);
    K2OSKERN_SeqUnlock(&gData.VirtMap.SeqLock, disp);

    KernObj_ReleaseRef(&pageArrayRef);

    KernObj_CreateRef(apRetRef, &pVirtMap->Hdr);
}

K2STAT  
KernVirtMap_CreateThreadStack(
    K2OSKERN_OBJ_THREAD *apThread
)
{
    UINT32                  pageCount;
    K2OSKERN_OBJ_VIRTMAP *  pVirtMap;
    K2STAT                  stat;
    UINT32                  virtAddr;
    UINT32 *                pPTE;
    UINT32                  ixPage;
    BOOL                    disp;

    K2_ASSERT(apThread->mIsKernelThread);

    pageCount = apThread->Config.mStackPages;

    K2_ASSERT(pageCount > 0);

    pVirtMap = (K2OSKERN_OBJ_VIRTMAP *)KernObj_Alloc(KernObj_VirtMap);
    if (NULL == pVirtMap)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    stat = K2STAT_ERROR_UNKNOWN;

    do
    {
        stat = KernPageArray_CreateSparse(pageCount, 0, &pVirtMap->PageArrayRef);
        if (K2STAT_IS_ERROR(stat))
            break;

        do
        {
            virtAddr = KernVirt_Reserve(pageCount + 2);
            if (0 == virtAddr)
                break;

            // we have everything we need to make the stack map now
            pVirtMap->mVirtToPhysMapType = K2OS_MapType_Thread_Stack;
            pVirtMap->mPageCount = pageCount + 2;
            pVirtMap->Kern.mSizeBytes = (pageCount + 2) * K2_VA_MEMPAGE_BYTES;
            pVirtMap->Kern.mSegType = KernSeg_Thread_Stack;

            pPTE = pVirtMap->mpPte = (UINT32 *)K2OS_KVA_TO_PTE_ADDR(virtAddr);

            stat = K2STAT_ERROR_CORRUPTED;

            disp = K2OSKERN_SeqLock(&gData.Virt.HeapSeqLock);

            do
            {
                if (0 != (*pPTE))
                    break;
                *pPTE = K2OSKERN_PTE_STACK_GUARD;

                pPTE += (pageCount + 1);

                if (0 != (*pPTE))
                {
                    pPTE -= (pageCount + 1);
                    *pPTE = 0;
                    break;
                }
                *pPTE = K2OSKERN_PTE_STACK_GUARD;
                pPTE -= pageCount;

                // 
                // guard pages set, pPTE set at first stack data page
                //

                for (ixPage = 0; ixPage < pageCount; ixPage++)
                {
                    if (0 != (*pPTE))
                        break;
                    *pPTE = K2OSKERN_PTE_MAP_CREATE;
                    pPTE++;
                }
                if (ixPage < pageCount)
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
                    // pPTE points to first stack data page again
                    // kill the end guard page
                    pPTE += pageCount;
                    *pPTE = 0;
                    // kill the start guard page
                    pPTE -= (pageCount + 1);
                    K2_ASSERT(pPTE == pVirtMap->mpPte);
                    *pPTE = 0;
                }
                else
                {
                    stat = K2STAT_NO_ERROR;
                }

            } while (0);

            K2OSKERN_SeqUnlock(&gData.Virt.HeapSeqLock, disp);

            if (K2STAT_IS_ERROR(stat))
            {
                KernVirt_Release(virtAddr);
                break;
            }

            pVirtMap->OwnerMapTreeNode.mUserVal = virtAddr;
            virtAddr += K2_VA_MEMPAGE_BYTES;    // bypass guard page at start
            for(ixPage = 0; ixPage < pageCount; ixPage++)
            {
                KernPte_MakePageMap(NULL, virtAddr, KernPageArray_PagePhys(pVirtMap->PageArrayRef.AsPageArray, ixPage), K2OS_MAPTYPE_KERN_DATA);
                virtAddr += K2_VA_MEMPAGE_BYTES;
            }

            //
            // double terminate
            //
            virtAddr -= sizeof(UINT32); // points to end of stack memory range just below end guard page now
            *((UINT32 *)virtAddr) = 0;
            virtAddr -= sizeof(UINT32);
            *((UINT32 *)virtAddr) = 0;
            apThread->Kern.mStackPtr = virtAddr;

            // add to kernel map tree
            disp = K2OSKERN_SeqLock(&gData.VirtMap.SeqLock);
            K2TREE_Insert(&gData.VirtMap.Tree, pVirtMap->OwnerMapTreeNode.mUserVal, &pVirtMap->OwnerMapTreeNode);
            K2OSKERN_SeqUnlock(&gData.VirtMap.SeqLock, disp);

            stat = K2STAT_NO_ERROR;

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            KernObj_ReleaseRef(&pVirtMap->PageArrayRef);
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        KernObj_Free(&pVirtMap->Hdr);
    }
    else
    {
        KernObj_CreateRef(&apThread->StackMapRef, &pVirtMap->Hdr);
    }

    return stat;
}

K2STAT
KernVirtMap_FindMapAndCreateRef(
    UINT32                  aKernVirtAddr,
    K2OSKERN_OBJREF *       apFillMapRef,
    UINT32 *                apRetMapPageIx
)
{
    BOOL                    disp;
    K2STAT                  stat;
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_OBJ_VIRTMAP *  pMap;
    UINT32                  pageIx;

    if (aKernVirtAddr < K2OS_KVA_KERN_BASE)
        return K2STAT_ERROR_NOT_FOUND;

    stat = K2STAT_ERROR_NOT_FOUND;

    disp = K2OSKERN_SeqLock(&gData.VirtMap.SeqLock);

    pTreeNode = K2TREE_FindOrAfter(&gData.VirtMap.Tree, aKernVirtAddr);
    if (NULL == pTreeNode)
    {
        pTreeNode = K2TREE_LastNode(&gData.VirtMap.Tree);
        K2_ASSERT(NULL != pTreeNode);
    }
    else
    {
        if (pTreeNode->mUserVal != aKernVirtAddr)
            pTreeNode = K2TREE_PrevNode(&gData.VirtMap.Tree, pTreeNode);
    }
    if (NULL != pTreeNode)
    {
        pMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, pTreeNode, OwnerMapTreeNode);
        if (0 == (pMap->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO))
        {
            pageIx = (aKernVirtAddr - pTreeNode->mUserVal) / K2_VA_MEMPAGE_BYTES;
            if (pageIx < pMap->mPageCount)
            {
                stat = K2STAT_NO_ERROR;
                KernObj_CreateRef(apFillMapRef, &pMap->Hdr);
                *apRetMapPageIx = pageIx;
            }
        }
        else
        {
            stat = K2STAT_ERROR_NOT_READY;
        }
    }

    K2OSKERN_SeqUnlock(&gData.VirtMap.SeqLock, disp);

    return stat;
}

struct _K2OSKERN_MAPUSER_OPAQUE
{
    K2OSKERN_OBJREF     RefProc;
    UINT32              mMapRefCount;
    K2OSKERN_OBJREF *   mpUserMapRefs;
    K2OSKERN_OBJREF *   mpKernMapRefs;
    UINT32              mMap0FirstPageIx;
    UINT32              mKernVirtAddr;
};

K2OSKERN_MAPUSER 
KernMapUser_Create(
    UINT32              aProcessId,
    K2OS_BUFDESC const *apBufDesc,
    UINT32 *            apRetKernVirtAddr
)
{
    K2OSKERN_MAPUSER    pMapUser;
    UINT32              calc;
    UINT32              offset;
    UINT32              left;
    K2STAT              stat;
    UINT32              pagesToMap;

    K2_ASSERT(0 != aProcessId);

//    K2OSKERN_Debug("Map process %d buffer 0x%08X for 0x%08X %s\n", aProcessId, apBufDesc->mAddress, apBufDesc->mBytesLength,
//        (apBufDesc->mAttrib & K2OS_BUFDESC_ATTRIB_READONLY) ? "RO" : "RW");

    pMapUser = (K2OSKERN_MAPUSER)K2OS_Heap_Alloc(sizeof(K2OSKERN_MAPUSER_OPAQUE));
    if (NULL == pMapUser)
    {
        return NULL;
    }

    stat = K2STAT_NO_ERROR;

    do
    {
        K2MEM_Zero(pMapUser, sizeof(K2OSKERN_MAPUSER_OPAQUE));

        calc = apBufDesc->mAddress;
        left = apBufDesc->mBytesLength;
        offset = (calc & K2_VA_MEMPAGE_OFFSET_MASK);
        if (0 != offset)
        {
            left = K2_VA_MEMPAGE_BYTES - offset;
            pagesToMap = 1;
            calc += left;
            if (left >= apBufDesc->mBytesLength)
            {
                left = 0;
            }
            else
            {
                left = apBufDesc->mBytesLength - left;
            }
        }
        else
        {
            pagesToMap = 0;
        }
        if (0 != left)
        {
            K2_ASSERT(0 == (calc & K2_VA_MEMPAGE_OFFSET_MASK));
            if (0 != (left & K2_VA_MEMPAGE_OFFSET_MASK))
            {
                pagesToMap++;  // end page
            }
            pagesToMap += (left / K2_VA_MEMPAGE_BYTES);
        }

        pMapUser->mpUserMapRefs = (K2OSKERN_OBJREF *)K2OS_Heap_Alloc(2 * pagesToMap * sizeof(K2OSKERN_OBJREF));
        if (NULL == pMapUser->mpUserMapRefs)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            break;
        }

        do
        {
            if (!KernProc_FindAddRefById(aProcessId, &pMapUser->RefProc))
            {
                stat = K2STAT_ERROR_NOT_FOUND;
                break;
            }

            do
            {
                K2MEM_Zero(pMapUser->mpUserMapRefs, 2 * pagesToMap * sizeof(K2OSKERN_OBJREF));
                pMapUser->mpKernMapRefs = pMapUser->mpUserMapRefs + pagesToMap;
                pMapUser->mMapRefCount = pagesToMap;
                stat = KernProc_AcqMaps(
                    pMapUser->RefProc.AsProc,
                    apBufDesc->mAddress,
                    apBufDesc->mBytesLength,
                    (0 == (apBufDesc->mAttrib & K2OS_BUFDESC_ATTRIB_READONLY)) ? TRUE : FALSE,
                    &pMapUser->mMapRefCount,
                    pMapUser->mpUserMapRefs,
                    &pMapUser->mMap0FirstPageIx
                );

                if (K2STAT_IS_ERROR(stat))
                    break;

                do
                {
                    pMapUser->mKernVirtAddr = K2OS_Virt_Reserve(pagesToMap);
                    if (0 == pMapUser->mKernVirtAddr)
                        break;

                    do
                    {
                        // mapping from page start, so left includes area before buffer on the page
                        // that we are actually not using
                        left = apBufDesc->mBytesLength + (apBufDesc->mAddress & K2_VA_MEMPAGE_OFFSET_MASK);
                        offset = 0;
                        pagesToMap = pMapUser->mpUserMapRefs[offset].AsVirtMap->mPageCount - pMapUser->mMap0FirstPageIx;
                        if (left <= (pagesToMap * K2_VA_MEMPAGE_BYTES))
                        {
                            pagesToMap = (left + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES;
                        }

                        stat = KernVirtMap_Create(
                            pMapUser->mpUserMapRefs[offset].AsVirtMap->PageArrayRef.AsPageArray,
                            pMapUser->mpUserMapRefs[offset].AsVirtMap->mPageArrayStartPageIx + pMapUser->mMap0FirstPageIx,
                            pagesToMap,
                            pMapUser->mKernVirtAddr,
                            (0 == (apBufDesc->mAttrib & K2OS_BUFDESC_ATTRIB_READONLY)) ? K2OS_MapType_Data_ReadWrite : K2OS_MapType_Data_ReadOnly,
                            &pMapUser->mpKernMapRefs[offset]
                        );

                        if (K2STAT_IS_ERROR(stat))
                            break;

                        offset++;

                        if (left > (pagesToMap * K2_VA_MEMPAGE_BYTES))
                        {
                            //
                            // we exhausted the first map.  keep going
                            // to the next one and map from there
                            //
                            left -= (pagesToMap * K2_VA_MEMPAGE_BYTES);
                            calc = pMapUser->mKernVirtAddr + (pagesToMap * K2_VA_MEMPAGE_BYTES);
                            do
                            {
                                pagesToMap = pMapUser->mpUserMapRefs[offset].AsVirtMap->mPageCount;
                                if (left <= (pagesToMap * K2_VA_MEMPAGE_BYTES))
                                {
                                    pagesToMap = (left + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES;
                                }

                                stat = KernVirtMap_Create(
                                    pMapUser->mpUserMapRefs[offset].AsVirtMap->PageArrayRef.AsPageArray,
                                    pMapUser->mpUserMapRefs[offset].AsVirtMap->mPageArrayStartPageIx,
                                    pagesToMap,
                                    calc,
                                    (0 == (apBufDesc->mAttrib & K2OS_BUFDESC_ATTRIB_READONLY)) ? K2OS_MapType_Data_ReadWrite : K2OS_MapType_Data_ReadOnly,
                                    &pMapUser->mpKernMapRefs[offset]
                                );

                                if (K2STAT_IS_ERROR(stat))
                                    break;

                                offset++;

                                if (left <= (pagesToMap * K2_VA_MEMPAGE_BYTES))
                                    break;

                                left -= (pagesToMap * K2_VA_MEMPAGE_BYTES);
                                calc += (pagesToMap * K2_VA_MEMPAGE_BYTES);

                            } while (1);
                        }

                        if (K2STAT_IS_ERROR(stat))
                        {
                            for (calc = 0; calc < offset; calc++)
                            {
                                KernObj_ReleaseRef(&pMapUser->mpKernMapRefs[calc]);
                            }
                        }

                    } while (0);

                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OS_Virt_Release(pMapUser->mKernVirtAddr);
                    }

                } while (0);

                // user maps are no longer needed in any case
                // since we have mapped all the page arrays
                // into the kernel.  so we don't keep the references around
                for (calc = 0; calc < pMapUser->mMapRefCount; calc++)
                {
                    KernObj_ReleaseRef(&pMapUser->mpUserMapRefs[calc]);
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                KernObj_ReleaseRef(&pMapUser->RefProc);
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Heap_Free(pMapUser->mpUserMapRefs);
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Heap_Free(pMapUser);
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    *apRetKernVirtAddr = pMapUser->mKernVirtAddr + (apBufDesc->mAddress & K2_VA_MEMPAGE_OFFSET_MASK);
//    K2OSKERN_Debug("Proc %d User %08X --> Kern %08X, %d bytes\n", aProcessId, apBufDesc->mAddress, *apRetKernVirtAddr, apBufDesc->mBytesLength);

    return pMapUser;
}

void             
KernMapUser_Destroy(
    K2OSKERN_MAPUSER    aMapUser
)
{
    UINT32 ix;

//    K2OSKERN_Debug("MapUser_Destroy(0x%08X)\n", aMapUser->mKernVirtAddr);
    for (ix = 0; ix < aMapUser->mMapRefCount; ix++)
    {
        KernObj_ReleaseRef(&aMapUser->mpKernMapRefs[ix]);
        K2_ASSERT(NULL == aMapUser->mpUserMapRefs[ix].AsAny);
    }
    KernVirt_Release(aMapUser->mKernVirtAddr);
    K2OS_Heap_Free(aMapUser->mpUserMapRefs);
    KernObj_ReleaseRef(&aMapUser->RefProc);
    K2OS_Heap_Free(aMapUser);
}
