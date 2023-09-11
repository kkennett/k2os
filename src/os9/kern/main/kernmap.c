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
        pVirtMap = (K2OSKERN_OBJ_VIRTMAP *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_VIRTMAP));
        if (NULL == pVirtMap)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            K2MEM_Zero(pVirtMap, sizeof(K2OSKERN_OBJ_VIRTMAP));
            pVirtMap->Hdr.mObjType = KernObj_VirtMap;
            K2LIST_Init(&pVirtMap->Hdr.RefObjList);

            switch (aMapType)
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

            pVirtMap->mMapType = aMapType;
            pVirtMap->mPageArrayStartPageIx = aPageOffset;
            pVirtMap->mPageCount = aPageCount;
            pVirtMap->Kern.mpVirtHeapNode = pKernHeapNode;
            pVirtMap->Kern.mSizeBytes = aPageCount * K2_VA_MEMPAGE_BYTES;
            pVirtMap->Kern.mType = KernMap_Dyn;

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
                    KernPte_MakePageMap(NULL, aVirtAddr, KernPageArray_PagePhys(apPageArray, aPageOffset + ixPage), mapAttr);
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

                KernHeap_Free(pVirtMap);
            }
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
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

    pVirtMap = (K2OSKERN_OBJ_VIRTMAP *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_VIRTMAP));
    if (NULL == pVirtMap)
    {
        K2OSKERN_Panic("*** Out of memory creating premapped virtmap\n");
    }

    K2MEM_Zero(pVirtMap, sizeof(K2OSKERN_OBJ_VIRTMAP));
    pVirtMap->Hdr.mObjType = KernObj_VirtMap;
    pVirtMap->Hdr.mObjFlags = K2OSKERN_OBJ_FLAG_PERMANENT;
    K2LIST_Init(&pVirtMap->Hdr.RefObjList);

    KernObj_CreateRef(&pVirtMap->PageArrayRef, pageArrayRef.AsAny);
    pVirtMap->mPageArrayStartPageIx = 0;
    pVirtMap->mPageCount = aPageCount;
    pVirtMap->mMapType = aMapType;
    pVirtMap->Kern.mType = KernMap_PreMap;
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
