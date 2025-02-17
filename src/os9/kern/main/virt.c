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

/*

At initialization:

...
|XXX-USED-XXX|
+------------+  <<- gData.mpShared->LoadInfo.mKernArenaHigh - used and mapped above this
|            |
+------------+
|            |  (unused but mappable virtual page frames)
...
|            |
+------------+  <<- gData.Virt.mTopPt, pagetable map boundary (4MB)
|            |
|            |
...             (unused and unmappable virtual page frames)
|            |
|            |
+------------+  <<- gData.Virt.mBotPt, pagetable map boundary (4MB)
|            |
...
|            |  (unused but mappable virtual page frames)
+------------+
|            |
+------------+  <<- gData.mpShared->LoadInfo.mKernArenaLow - used and mapped below this
|XXX-USED-XXX|
...

*/

#define CHECK_LOGIC_FAULT   1

#if CHECK_LOGIC_FAULT
BOOL sgFault = FALSE;
#endif

#define HEAPNODES_PER_PAGE (K2_VA_MEMPAGE_BYTES / sizeof(K2OSKERN_VIRTHEAP_NODE))

K2HEAP_NODE * KernVirt_HeapAcqNode(K2HEAP_ANCHOR *apHeap);
void          KernVirt_HeapRelNode(K2HEAP_ANCHOR *apHeap, K2HEAP_NODE *apNode);

UINT32  KernVirt_RamHeapLock(K2RAMHEAP *apRamHeap);
void    KernVirt_RamHeapUnlock(K2RAMHEAP *apRamHeap, UINT32 aDisp);
K2STAT  KernVirt_RamHeapCreateRange(UINT32 aTotalPageCount, UINT32 aInitCommitPageCount, void **appRetRange, UINT32 *apRetStartAddr);
K2STAT  KernVirt_RamHeapCommitPages(void *aRange, UINT32 aAddr, UINT32 aPageCount);

static K2RAMHEAP_SUPP const sgRamHeapSupp =
{
    KernVirt_RamHeapLock,
    KernVirt_RamHeapUnlock,
    KernVirt_RamHeapCreateRange,
    KernVirt_RamHeapCommitPages,
    NULL,
    NULL
};

UINT32  KernVirt_Locked_Alloc(UINT32 aPagesCount, K2OSKERN_VIRTHEAP_NODE **apRetHeapNode);
BOOL    KernVirt_Locked_Release(UINT32 aPagesAddr);

void
KernVirt_Init(
    void
)
{
    UINT32  chk;
    UINT32 *pPTE;

    //
    // verify bottom-up remaining page range is really free (no pages mapped)
    //
    if (0 != (gData.mpShared->LoadInfo.mKernArenaLow & (K2_VA32_PAGETABLE_MAP_BYTES - 1)))
    {
        chk = gData.mpShared->LoadInfo.mKernArenaLow;
        pPTE = (UINT32 *)K2OS_KVA_TO_PTE_ADDR(chk);
        do
        {
            // verify this page is not mapped
            K2_ASSERT(0 == ((*pPTE) & K2OSKERN_PTE_PRESENT_BIT));
            pPTE++;
            chk += K2_VA_MEMPAGE_BYTES;
        } while (0 != (chk & (K2_VA32_PAGETABLE_MAP_BYTES - 1)));
        gData.Virt.mBotPt = chk;
    }
    else
        gData.Virt.mBotPt = gData.mpShared->LoadInfo.mKernArenaLow;

    //
    // verify top-down remaining page range is really free (no pages mapped)
    //
    if (0 != (gData.mpShared->LoadInfo.mKernArenaHigh & (K2_VA32_PAGETABLE_MAP_BYTES - 1)))
    {
        chk = gData.mpShared->LoadInfo.mKernArenaHigh;
        pPTE = (UINT32 *)K2OS_KVA_TO_PTE_ADDR(chk);
        do
        {
            // verify this page is not mapped
            chk -= K2_VA_MEMPAGE_BYTES;
            pPTE--;
            K2_ASSERT(0 == ((*pPTE) & K2OSKERN_PTE_PRESENT_BIT));
        } while (0 != (chk & (K2_VA32_PAGETABLE_MAP_BYTES - 1)));
        gData.Virt.mTopPt = chk;
    }
    else
        gData.Virt.mTopPt = gData.mpShared->LoadInfo.mKernArenaHigh;

    //
    // verify all pagetables between bottom and top pt are not mapped
    //
    if (gData.Virt.mBotPt != gData.Virt.mTopPt)
    {
        chk = gData.Virt.mBotPt;
        pPTE = (UINT32 *)K2OS_KVA_TO_PTE_ADDR(K2OS_KVA_TO_PT_ADDR(chk));
        do
        {
            K2_ASSERT(0 == ((*pPTE) & K2OSKERN_PTE_PRESENT_BIT));
            pPTE++;
            chk += K2_VA32_PAGETABLE_MAP_BYTES;
        } while (chk != gData.Virt.mTopPt);
    }

    //
    // this heap is the kernel's object heap. it uses the range heap
    //
    K2OSKERN_SeqInit(&gData.Virt.RamHeapSeqLock);
    K2RAMHEAP_Init(&gData.Virt.RamHeap, &sgRamHeapSupp);
    K2LIST_Init(&gData.Virt.PhysTrackList);

    //
    // this heap is the kernel's virtual range heap
    //
    K2OSKERN_SeqInit(&gData.Virt.HeapSeqLock);
    K2LIST_Init(&gData.Virt.HeapFreeNodeList);
    K2HEAP_Init(&gData.Virt.Heap, KernVirt_HeapAcqNode, KernVirt_HeapRelNode);

    //
    // set up the range heap with the ranges for calculated above.  this is why we
    // have stock nodes
    //
    if (gData.mpShared->LoadInfo.mKernArenaHigh != gData.Virt.mTopPt)
    {
        K2LIST_AddAtTail(&gData.Virt.HeapFreeNodeList, (K2LIST_LINK *)&gData.Virt.StockNode[0].HeapNode);
        K2HEAP_AddFreeSpaceNode(&gData.Virt.Heap, gData.Virt.mTopPt, gData.mpShared->LoadInfo.mKernArenaHigh - gData.Virt.mTopPt, NULL);
    }
    if (gData.mpShared->LoadInfo.mKernArenaLow != gData.Virt.mBotPt)
    {
        K2LIST_AddAtTail(&gData.Virt.HeapFreeNodeList, (K2LIST_LINK *)&gData.Virt.StockNode[1].HeapNode);
        K2HEAP_AddFreeSpaceNode(&gData.Virt.Heap, gData.mpShared->LoadInfo.mKernArenaLow, gData.Virt.mBotPt - gData.mpShared->LoadInfo.mKernArenaLow, NULL);
    }

    KernArch_VirtInit();
}

UINT32
KernVirt_RamHeapLock(
    K2RAMHEAP * apRamHeap
)
{
    return K2OSKERN_SeqLock(&gData.Virt.RamHeapSeqLock);
}

void
KernVirt_RamHeapUnlock(
    K2RAMHEAP * apRamHeap,
    UINT32      aDisp
)
{
    K2OSKERN_SeqUnlock(&gData.Virt.RamHeapSeqLock, (BOOL)aDisp);
}

K2STAT
KernVirt_RamHeapCreateRange(
    UINT32      aTotalPageCount,
    UINT32      aInitCommitPageCount,
    void **     appRetRange,
    UINT32 *    apRetStartAddr
)
{
    //
    // both gData.Virt.RamHeapSeqLock and gData.Virt.HeapSeqLock are held
    //
    UINT32                  virtSpace;
    UINT32                  virtPageAddr;
    K2STAT                  stat;
    K2LIST_ANCHOR           trackList;
    K2LIST_LINK *           pListLink;
    K2OSKERN_PHYSTRACK *    pTrack;
    K2OSKERN_PHYSRES        res;
    K2OSKERN_VIRTHEAP_NODE *pVirtHeapNode;
    UINT32                  virtEnd;
    UINT32 *                pPTE;

    K2_ASSERT(aInitCommitPageCount <= aTotalPageCount);

    virtSpace = KernVirt_Locked_Alloc(aTotalPageCount, &pVirtHeapNode);
    if (0 == virtSpace)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    virtEnd = virtSpace + (K2_VA_MEMPAGE_BYTES * aTotalPageCount);

    K2TREE_NODE_SETUSERBIT(&pVirtHeapNode->HeapNode.SizeTreeNode);

    if (!KernPhys_Reserve_Init(&res, aInitCommitPageCount))
    {
        stat = K2STAT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        stat = KernPhys_AllocSparsePages(&res, aInitCommitPageCount, &trackList);
        if (!K2STAT_IS_ERROR(stat))
        {
            KernPhys_CutTrackListIntoPages(&trackList, FALSE, NULL, FALSE, 0);
            pListLink = trackList.mpHead;
            virtPageAddr = virtSpace;
            do
            {
                pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, pListLink, ListLink);
                pListLink = pListLink->mpNext;

                KernPte_MakePageMap(NULL, virtPageAddr, K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack), K2OS_MAPTYPE_KERN_DATA);

                virtPageAddr += K2_VA_MEMPAGE_BYTES;
            } while (pListLink != NULL);

            if (virtPageAddr != virtEnd)
            {
                pPTE = (UINT32 *)K2OS_KVA_TO_PTE_ADDR(virtPageAddr);
                do {
                    *pPTE = K2OSKERN_PTE_HEAPMAP_PENDING;
                    pPTE++;
                    virtPageAddr += K2_VA_MEMPAGE_BYTES;
                } while (virtPageAddr != virtEnd);
                K2_CpuWriteBarrier();
            }

            K2LIST_AppendToTail(&gData.Virt.PhysTrackList, &trackList);

            *appRetRange = (void *)virtSpace;
            *apRetStartAddr = virtSpace;
            stat = K2STAT_NO_ERROR;
        }
        else
        {
            KernPhys_Reserve_Release(&res);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        KernVirt_Locked_Release(virtSpace);
    }
    else if (gData.Virt.mKernHeapThreaded)
    {
        K2_ASSERT(NULL == gData.Virt.mpRamHeapHunk);
        gData.Virt.mpRamHeapHunk = pVirtHeapNode;
    }

    return stat;
}

K2STAT
KernVirt_RamHeapCommitPages(
    void *  aRange,
    UINT32  aAddr,
    UINT32  aPageCount
)
{
    //
    // both gData.Virt.RamHeapSeqLock and gData.Virt.HeapSeqLock are held
    //
    UINT32                  virtPageAddr;
    K2STAT                  stat;
    K2LIST_ANCHOR           trackList;
    K2LIST_LINK *           pListLink;
    K2OSKERN_PHYSTRACK *    pTrack;
    K2OSKERN_PHYSRES        res;

    if (!KernPhys_Reserve_Init(&res, aPageCount))
        return K2STAT_ERROR_OUT_OF_MEMORY;

    stat = KernPhys_AllocSparsePages(&res, aPageCount, &trackList);
    if (K2STAT_IS_ERROR(stat))
    {
        KernPhys_Reserve_Release(&res);
        return stat;
    }

    KernPhys_CutTrackListIntoPages(&trackList, FALSE, NULL, FALSE, 0);
    pListLink = trackList.mpHead;
    virtPageAddr = aAddr;
    do
    {
        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, pListLink, ListLink);
        pListLink = pListLink->mpNext;

        KernPte_MakePageMap(NULL, virtPageAddr, K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack), K2OS_MAPTYPE_KERN_DATA);

        virtPageAddr += K2_VA_MEMPAGE_BYTES;
    } while (pListLink != NULL);
    K2_CpuWriteBarrier();

    K2LIST_AppendToTail(&gData.Virt.PhysTrackList, &trackList);

    return K2STAT_NO_ERROR;
}

void
KernHeap_NewHeapMap(
    K2OSKERN_VIRTHEAP_NODE * apVirtHeapNode
)
{
    K2OSKERN_OBJ_VIRTMAP *  pVirtMap;
    K2OSKERN_OBJREF         refPageArray;
    K2STAT                  stat;
    UINT32                  virtAddr;
    UINT32                  pageCount;
    BOOL                    disp;

    virtAddr = apVirtHeapNode->HeapNode.AddrTreeNode.mUserVal;
    pageCount = apVirtHeapNode->HeapNode.SizeTreeNode.mUserVal / K2_VA_MEMPAGE_BYTES;

    refPageArray.AsAny = NULL;
    stat = KernPageArray_CreatePreMap(virtAddr, pageCount, 0, &refPageArray);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    pVirtMap = K2OS_Heap_Alloc(sizeof(K2OSKERN_OBJ_VIRTMAP));
    K2_ASSERT(NULL != pVirtMap);

    K2MEM_Zero(pVirtMap, sizeof(K2OSKERN_OBJ_VIRTMAP));

    pVirtMap->Hdr.mObjType = KernObj_VirtMap;
    pVirtMap->Hdr.mObjFlags = K2OSKERN_OBJ_FLAG_PERMANENT;
    K2LIST_Init(&pVirtMap->Hdr.RefObjList);
    KernObj_CreateRef(&pVirtMap->PageArrayRef, refPageArray.AsAny);
    KernObj_ReleaseRef(&refPageArray);
    pVirtMap->mVirtToPhysMapType = K2OS_MapType_Data_ReadWrite;
    pVirtMap->mPageArrayStartPageIx = 0;
    pVirtMap->mPageCount = pageCount;
    pVirtMap->OwnerMapTreeNode.mUserVal = virtAddr;
    pVirtMap->Kern.mSizeBytes = pageCount * K2_VA_MEMPAGE_BYTES;
    pVirtMap->Kern.mSegType = KernSeg_PreMap;
    pVirtMap->mpPte = (UINT32 *)K2OS_KVA_TO_PTE_ADDR(virtAddr);
    
    disp = K2OSKERN_SeqLock(&gData.VirtMap.SeqLock);
    K2TREE_Insert(&gData.VirtMap.Tree, pVirtMap->OwnerMapTreeNode.mUserVal, &pVirtMap->OwnerMapTreeNode);
    K2OSKERN_SeqUnlock(&gData.VirtMap.SeqLock, disp);
}

void * 
KernHeap_Alloc(
    UINT32 aByteCount
)
{
    K2STAT                      stat;
    void *                      retPtr;
    BOOL                        disp;
    K2OSKERN_VIRTHEAP_NODE *    pVirtHeapNode;

#if CHECK_LOGIC_FAULT
    K2_ASSERT(!sgFault);
#endif

    disp = K2OSKERN_SeqLock(&gData.Virt.HeapSeqLock);

    stat = K2RAMHEAP_Alloc(&gData.Virt.RamHeap, aByteCount, TRUE, &retPtr);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    pVirtHeapNode = gData.Virt.mpRamHeapHunk;
    gData.Virt.mpRamHeapHunk = NULL;

    K2OSKERN_SeqUnlock(&gData.Virt.HeapSeqLock, disp);

    if (NULL != pVirtHeapNode)
    {
        //
        // now we need to create the pagearray and virtmap
        // for 'hunkVirtBase' and 'hunkPageCount'
        //
        KernHeap_NewHeapMap(pVirtHeapNode);
    }

    return retPtr;
}

BOOL   
KernHeap_Free(
    void *aPtr
)
{
    K2STAT  stat;
    BOOL    disp;

    disp = K2OSKERN_SeqLock(&gData.Virt.HeapSeqLock);

    stat = K2RAMHEAP_Free(&gData.Virt.RamHeap, aPtr);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    K2OSKERN_SeqUnlock(&gData.Virt.HeapSeqLock, disp);

    return (K2STAT_IS_ERROR(stat) ? FALSE : TRUE);
}

K2HEAP_NODE *
KernVirt_HeapAcqNode(
    K2HEAP_ANCHOR *apHeap
)
{
    K2LIST_LINK * pListLink;

    K2_ASSERT(apHeap == &gData.Virt.Heap);

    pListLink = gData.Virt.HeapFreeNodeList.mpHead;
    if (NULL != pListLink)
    {
        K2LIST_Remove(&gData.Virt.HeapFreeNodeList, pListLink);
    }

    return (K2HEAP_NODE *)pListLink;
}
 
void
KernVirt_HeapRelNode(
    K2HEAP_ANCHOR * apHeap,
    K2HEAP_NODE *   apNode
)
{
    K2_ASSERT(apHeap == &gData.Virt.Heap);
    K2LIST_AddAtTail(&gData.Virt.HeapFreeNodeList, (K2LIST_LINK *)apNode);
}

BOOL
KernVirt_ReplenishTracking(
    void
)
{
    UINT32                  physPageAddr;
    UINT32                  ix;
    K2OSKERN_VIRTHEAP_NODE *pVirtHeapNode;
    K2STAT                  stat;
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_PHYSTRACK *    pTrack;
    K2LIST_ANCHOR           trackList;
    K2OSKERN_PHYSRES        res;
    BOOL                    ok;

    //    K2OSKERN_Debug("VirtHeap SizeTree nodecount = %d\n", gData.Virt.Heap.SizeTree.mNodeCount);
    if (gData.Virt.Heap.SizeTree.mNodeCount == 0)
    {
        //
        // need to add some free space to the heap
        //
//        K2OSKERN_Debug("toppt = %08X\n", gData.Virt.mTopPt);
//        K2OSKERN_Debug("botpt = %08X\n", gData.Virt.mBotPt);
        if (gData.Virt.mTopPt == gData.Virt.mBotPt)
        {
            //
            // virtual memory exhausted
            //
            K2_ASSERT(0);   // if hit in debug we want to know
            return FALSE;
        }

        if (!KernPhys_Reserve_Init(&res, 2))
            return FALSE;
        stat = KernPhys_AllocSparsePages(&res, 2, &trackList);
        if (K2STAT_IS_ERROR(stat))
        {
            KernPhys_Reserve_Release(&res);
            return FALSE;
        }

        KernPhys_CutTrackListIntoPages(&trackList, FALSE, NULL, FALSE, 0);
        K2_ASSERT(trackList.mNodeCount == 2);

        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, trackList.mpHead, ListLink);
        K2LIST_Remove(&trackList, &pTrack->ListLink);
        K2LIST_AddAtTail(&gData.Virt.PhysTrackList, &pTrack->ListLink);
        physPageAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);
        //        K2OSKERN_Debug("Install PT for va %08X\n", gData.Virt.mTopPt - K2_VA32_PAGETABLE_MAP_BYTES);
        KernArch_InstallPageTable(NULL, gData.Virt.mTopPt - K2_VA32_PAGETABLE_MAP_BYTES, physPageAddr);

        pVirtHeapNode = (K2OSKERN_VIRTHEAP_NODE *)(gData.Virt.mTopPt - K2_VA_MEMPAGE_BYTES);
        //        K2OSKERN_Debug("pHeapNode = %08X\n", pHeapNode);

        gData.Virt.mTopPt -= K2_VA32_PAGETABLE_MAP_BYTES;

        //
        // cant install this virtual space into the heap without tracking structures, so we need to
        // add a page of tracking, then add the free space, then alloc the tracking from the heap
        //

        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, trackList.mpHead, ListLink);
        K2LIST_Remove(&trackList, &pTrack->ListLink);
        K2LIST_AddAtTail(&gData.Virt.PhysTrackList, &pTrack->ListLink);
        physPageAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);

        //        K2OSKERN_Debug("Map v%08X -> p%08X\n", (UINT32)pHeapNode, physPageAddr);
        KernPte_MakePageMap(NULL, (UINT32)pVirtHeapNode, physPageAddr, K2OS_MAPTYPE_KERN_DATA);
        K2MEM_Zero(pVirtHeapNode, K2_VA_MEMPAGE_BYTES);
        for (ix = 0; ix < HEAPNODES_PER_PAGE; ix++)
        {
            K2LIST_AddAtTail(&gData.Virt.HeapFreeNodeList, (K2LIST_LINK *)&pVirtHeapNode[ix]);
        }

        //        K2OSKERN_Debug("AddFreeSpace %08X (%08X)\n", gData.Virt.mTopPt, K2_VA32_PAGETABLE_MAP_BYTES);
        stat = K2HEAP_AddFreeSpaceNode(&gData.Virt.Heap, gData.Virt.mTopPt, K2_VA32_PAGETABLE_MAP_BYTES, NULL);
        K2_ASSERT(!K2STAT_IS_ERROR(stat));

        //        K2OSKERN_Debug("AllocAt(%08X,%08X)\n", (UINT32)pHeapNode, K2_VA_MEMPAGE_BYTES);
        stat = K2HEAP_AllocAt(&gData.Virt.Heap, (UINT32)pVirtHeapNode, K2_VA_MEMPAGE_BYTES);
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
    }
    else
    {
        ok = KernPhys_Reserve_Init(&res, 1);
        if (!ok)
            return K2STAT_ERROR_OUT_OF_MEMORY;
        stat = KernPhys_AllocPow2Bytes(&res, K2_VA_MEMPAGE_BYTES, &pTrack);
        if (K2STAT_IS_ERROR(stat))
        {
            KernPhys_Reserve_Release(&res);
            return stat;
        }
        K2LIST_AddAtTail(&gData.Virt.PhysTrackList, &pTrack->ListLink);
        physPageAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);

        //
        // there is space in the heap to alloc a page - take it from the start of the smallest size chunk
        //
        pTreeNode = K2TREE_FirstNode(&gData.Virt.Heap.SizeTree);
        pVirtHeapNode = K2_GET_CONTAINER(K2OSKERN_VIRTHEAP_NODE, pTreeNode, HeapNode.SizeTreeNode);
        //        K2OSKERN_Debug("Smallest node in size tree = addr %08X size %08X\n", pHeapNode->AddrTreeNode.mUserVal, pHeapNode->SizeTreeNode.mUserVal);

        pVirtHeapNode = (K2OSKERN_VIRTHEAP_NODE *)pVirtHeapNode->HeapNode.AddrTreeNode.mUserVal;
        //        K2OSKERN_Debug("pHeapNode = %08X\n", pHeapNode);

        //        K2OSKERN_Debug("Map v%08X -> p%08X\n", (UINT32)pHeapNode, physPageAddr);
        KernPte_MakePageMap(NULL, (UINT32)pVirtHeapNode, physPageAddr, K2OS_MAPTYPE_KERN_DATA);
        K2MEM_Zero(pVirtHeapNode, K2_VA_MEMPAGE_BYTES);
        for (ix = 0; ix < HEAPNODES_PER_PAGE; ix++)
        {
            K2LIST_AddAtTail(&gData.Virt.HeapFreeNodeList, (K2LIST_LINK *)&pVirtHeapNode[ix]);
        }

        //        K2OSKERN_Debug("AllocAt(%08X,%08X)\n", (UINT32)pHeapNode, K2_VA_MEMPAGE_BYTES);
        stat = K2HEAP_AllocAt(&gData.Virt.Heap, (UINT32)pVirtHeapNode, K2_VA_MEMPAGE_BYTES);
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
    }
    //
    // should be lots of tracking structures
    //
//    K2OSKERN_Debug("VirtHeap freelist nodecount = %d\n", gData.Virt.HeapFreeNodeList.mNodeCount);
    return TRUE;
}

UINT32
KernVirt_Locked_Alloc(
    UINT32                      aPagesCount, 
    K2OSKERN_VIRTHEAP_NODE **   apRetHeapNode
)
{
    UINT32                  byteCount;
    UINT32                  topSpace;
    UINT32                  botSpace;
    K2HEAP_NODE *           pHeapNode;
    K2LIST_ANCHOR           trackList;
    K2STAT                  stat;
    K2OSKERN_PHYSTRACK *    pTrack;
    UINT32                  numPt;
    UINT32                  physPageAddr;
    K2OSKERN_PHYSRES        res;
    K2OSKERN_VIRTHEAP_NODE *pKernHeapNode;

    //    K2OSKERN_Debug("VirtHeap.Alloc freelist nodecount = %d\n", gData.Virt.HeapFreeNodeList.mNodeCount);
    if (gData.Virt.HeapFreeNodeList.mNodeCount < 4)
    {
        if (!KernVirt_ReplenishTracking())
        {
            return 0;
        }
    }

    byteCount = aPagesCount * K2_VA_MEMPAGE_BYTES;

    stat = K2HEAP_AllocNodeBest(&gData.Virt.Heap, byteCount, 0, &pHeapNode);
    if (!K2STAT_IS_ERROR(stat))
    {
        pKernHeapNode = K2_GET_CONTAINER(K2OSKERN_VIRTHEAP_NODE, pHeapNode, HeapNode);
        pKernHeapNode->mRefs = 1;
        K2TREE_NODE_CLRUSERBIT(&pKernHeapNode->HeapNode.SizeTreeNode);
        K2_CpuWriteBarrier();
        if (NULL != apRetHeapNode)
        {
            *apRetHeapNode = pKernHeapNode;
        }
        return K2HEAP_NodeAddr(pHeapNode);
    }

//    K2OSKERN_Debug("VirtAlloc(%08X) => FAIL\n", byteCount);
//    K2OSKERN_Debug("TopPT = %08X\n", gData.Virt.mTopPt);
//    K2OSKERN_Debug("BotPT = %08X\n", gData.Virt.mBotPt);

    pHeapNode = K2HEAP_FindNodeContainingAddr(&gData.Virt.Heap, gData.Virt.mBotPt - 1);
    K2_ASSERT(NULL != pHeapNode);
    botSpace = K2HEAP_NodeIsFree(pHeapNode) ? K2HEAP_NodeSize(pHeapNode) : 0;
//    K2OSKERN_Debug("botAddr = %08X, botSpace = %08X\n", K2HEAP_NodeAddr(pHeapNode), botSpace);

    pHeapNode = K2HEAP_FindNodeContainingAddr(&gData.Virt.Heap, gData.Virt.mTopPt);
    if (NULL != pHeapNode)
        topSpace = K2HEAP_NodeIsFree(pHeapNode) ? K2HEAP_NodeSize(pHeapNode) : 0;
    else
        topSpace = 0;
//    K2OSKERN_Debug("topAddr = %08X, topSpace = %08X\n", K2HEAP_NodeAddr(pHeapNode), topSpace);

    if ((topSpace + (gData.Virt.mTopPt - gData.Virt.mBotPt) + botSpace) < byteCount)
    {
        // insufficient space between bottom and top taking into account any free space available 
        K2OSKERN_Debug("-- Insufficient space (%08X) to meet alloc (%08X)\n",
            topSpace + (gData.Virt.mTopPt - gData.Virt.mBotPt) + botSpace,
            byteCount);
        return 0;
    }

    //
    // virtual space allocation will succeed as long as enough physical memory exists to
    // map the pagetables
    //

    if (((gData.Virt.mTopPt - gData.Virt.mBotPt) + botSpace) < byteCount)
    {
        //
        // allocation will only succeed if we include top space available
        // so we need to map pagetables from bot to top
        //
        numPt = (gData.Virt.mTopPt - gData.Virt.mBotPt) / K2_VA32_PAGETABLE_MAP_BYTES;
    }
    else
    {
        numPt = 0;
        while (botSpace < byteCount)
        {
            numPt++;
            botSpace += K2_VA32_PAGETABLE_MAP_BYTES;
        }
    }
//    K2OSKERN_Debug("-- Need %d pagetables\n", numPt);

    //
    // we know the # of pagetables we need to allocate to add at the botPt marker to
    // make the allocation
    //
    if (!KernPhys_Reserve_Init(&res, numPt))
    {
        K2OSKERN_Debug("Phys reserve for pagetables failed\n");
        return 0;
    }
    stat = KernPhys_AllocSparsePages(&res, numPt, &trackList);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("Phys alloc for pagetables failed\n");
        KernPhys_Reserve_Release(&res);
        return 0;
    }

    KernPhys_CutTrackListIntoPages(&trackList, FALSE, NULL, FALSE, 0);
    K2_ASSERT(trackList.mNodeCount == numPt);
    do
    {
        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, trackList.mpHead, ListLink);
        K2_ASSERT(pTrack->Flags.Field.BlockSize == K2_VA_MEMPAGE_BYTES_POW2);
        K2LIST_Remove(&trackList, &pTrack->ListLink);
        K2LIST_AddAtTail(&gData.Virt.PhysTrackList, &pTrack->ListLink);

        physPageAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);
        KernArch_InstallPageTable(NULL, gData.Virt.mBotPt, physPageAddr);

        K2HEAP_AddFreeSpaceNode(&gData.Virt.Heap, gData.Virt.mBotPt, K2_VA32_PAGETABLE_MAP_BYTES, NULL);

        gData.Virt.mBotPt += K2_VA32_PAGETABLE_MAP_BYTES;

    } while (--numPt);

    stat = K2HEAP_AllocNodeBest(&gData.Virt.Heap, byteCount, 0, &pHeapNode);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    pKernHeapNode = K2_GET_CONTAINER(K2OSKERN_VIRTHEAP_NODE, pHeapNode, HeapNode);
    pKernHeapNode->mRefs = 1;
    K2TREE_NODE_CLRUSERBIT(&pKernHeapNode->HeapNode.SizeTreeNode);
    K2_CpuWriteBarrier();
    if (NULL != apRetHeapNode)
    {
        *apRetHeapNode = pKernHeapNode;
    }
    return K2HEAP_NodeAddr(pHeapNode);
}

UINT32
KernVirt_Reserve(
    UINT32  aPagesCount
)
{
    UINT32  result;
    BOOL    disp;

    disp = K2OSKERN_SeqLock(&gData.Virt.HeapSeqLock);

    result = KernVirt_Locked_Alloc(aPagesCount, NULL);

    K2OSKERN_SeqUnlock(&gData.Virt.HeapSeqLock, disp);

    return result;
}

BOOL
KernVirt_Locked_Release(
    UINT32 aPagesAddr
)
{
    BOOL                        result;
    K2OSKERN_VIRTHEAP_NODE *    pVirtHeapNode;

    if (gData.Virt.HeapFreeNodeList.mNodeCount < 2)
    {
        if (!KernVirt_ReplenishTracking())
            return FALSE;
    }

    pVirtHeapNode = (K2OSKERN_VIRTHEAP_NODE *)K2HEAP_FindNodeContainingAddr(&gData.Virt.Heap, aPagesAddr);
    if (NULL == pVirtHeapNode)
    {
        return FALSE;
    }

    if (0 == pVirtHeapNode->mRefs)
    {
        K2OSKERN_Panic("Virtual rage has zero refcount at free!\n");
    }

    if (0 != K2ATOMIC_Dec(&pVirtHeapNode->mRefs))
    {
        return TRUE;
    }

    result = K2HEAP_Free(&gData.Virt.Heap, aPagesAddr);
    K2_ASSERT(result);

    return result;
}

BOOL
KernVirt_AddRefContaining(
    UINT32                      aPagesAddr,
    K2OSKERN_VIRTHEAP_NODE **   appRetVirtHeapNode
)
{
    BOOL                        disp;
    BOOL                        result;
    K2OSKERN_VIRTHEAP_NODE *    pVirtHeapNode;

    K2_ASSERT(0 == (aPagesAddr & K2_VA_MEMPAGE_OFFSET_MASK));

    disp = K2OSKERN_SeqLock(&gData.Virt.HeapSeqLock);

    pVirtHeapNode = (K2OSKERN_VIRTHEAP_NODE *)K2HEAP_FindNodeContainingAddr(&gData.Virt.Heap, aPagesAddr);
    result = (NULL == pVirtHeapNode) ? FALSE : TRUE;

    if (result)
    {
        K2ATOMIC_Inc(&pVirtHeapNode->mRefs);
        if (NULL != appRetVirtHeapNode)
        {
            *appRetVirtHeapNode = pVirtHeapNode;
        }
    }

    K2OSKERN_SeqUnlock(&gData.Virt.HeapSeqLock, disp);

    return result;
}

BOOL
KernVirt_Release(
    UINT32 aPagesAddr
)
{
    BOOL disp;
    BOOL result;

    K2_ASSERT(0 == (aPagesAddr & K2_VA_MEMPAGE_OFFSET_MASK));

    disp = K2OSKERN_SeqLock(&gData.Virt.HeapSeqLock);

    result = KernVirt_Locked_Release(aPagesAddr);

    K2OSKERN_SeqUnlock(&gData.Virt.HeapSeqLock, disp);

    return result;
}

void
KernVirt_Threaded_Init(
    void
)
{
    K2HEAP_NODE *           pHeapNode;
    K2TREE_NODE *           pTreeNode;
    UINT32                  virtAddr;
    UINT32                  pageCount;
    K2OSKERN_OBJREF         refMap;
    K2EFI_MEMORY_DESCRIPTOR desc;
    UINT32                  entCount;
    UINT8 const *           pWork;

    //
    // go through EFI map and register runtime virtual ranges
    // as maps so we can see offsets in case of exceptions
    //
#if CHECK_LOGIC_FAULT
    sgFault = TRUE;
#endif

    do
    {
        entCount = gData.mpShared->LoadInfo.mEfiMapSize / gData.mpShared->LoadInfo.mEfiMemDescSize;
        pWork = (UINT8 const *)K2OS_KVA_EFIMAP_BASE;
        do
        {
            K2MEM_Copy(&desc, pWork, sizeof(K2EFI_MEMORY_DESCRIPTOR));

            if (0 != (desc.Attribute & K2EFI_MEMORYFLAG_RUNTIME))
            {
                if ((desc.Type == K2EFI_MEMTYPE_Run_Code) || 
                    (desc.Type == K2EFI_MEMTYPE_Run_Data))
                {
                    virtAddr = (UINT32)desc.VirtualStart;
                    pTreeNode = K2TREE_Find(&gData.VirtMap.Tree, virtAddr);
                    if (NULL == pTreeNode)
                    {
                        pageCount = (UINT32)desc.NumberOfPages;
//                        K2OSKERN_Debug("%08X %08X UEFI_NEED_MAP\n", virtAddr, pageCount * K2_VA_MEMPAGE_BYTES);
                        refMap.AsAny = NULL;
                        KernVirtMap_CreatePreMap(virtAddr, pageCount, 
                            (desc.Type == K2EFI_MEMTYPE_Run_Code) ? K2OS_MapType_Text : K2OS_MapType_Data_ReadWrite,
                            &refMap);
                        refMap.AsVirtMap->Hdr.mObjFlags |= K2OSKERN_OBJ_FLAG_PERMANENT;
                        KernObj_ReleaseRef(&refMap);
                        break;
                    }
                }
            }
            pWork += gData.mpShared->LoadInfo.mEfiMemDescSize;
        } while (--entCount > 0);
    } while (entCount > 0);

#if 0
    UINT32 ptAddr;
    UINT32 ptPteAddr;
    UINT32 pte;
    UINT32 ptPte;
    UINT32 pteAddr;

    //
    // dump mappings from kernel base to arena low
    //
    virtAddr = K2OS_KVA_KERN_BASE;
    do
    {
        ptAddr = K2OS_KVA_TO_PT_ADDR(virtAddr);
        ptPteAddr = K2OS_KVA_TO_PTE_ADDR(ptAddr);
        ptPte = *((UINT32 *)ptPteAddr);
        if (0 == (ptPte & K2OSKERN_PTE_PRESENT_BIT))
        {
            K2OSKERN_Debug("%08X %08X UNMAPPPED PT\n", virtAddr, K2_VA32_PAGETABLE_MAP_BYTES);
            virtAddr += K2_VA32_PAGETABLE_MAP_BYTES;
        }
        else
        {
            pageCount = K2_VA32_ENTRIES_PER_PAGETABLE;
            pteAddr = ptAddr;
            do
            {
                pte = *((UINT32 *)pteAddr);
                if ((0 != (pte & K2OSKERN_PTE_PRESENT_BIT)) &&
                    ((pte & K2_VA_PAGEFRAME_MASK) != gData.mpShared->LoadInfo.mZeroPagePhys))
                {
                    K2OSKERN_Debug("  %08X %08X\n", virtAddr, pte);
                }
                virtAddr += K2_VA_MEMPAGE_BYTES;
                pteAddr += sizeof(pte);
            } while (--pageCount);
        }
    } while (virtAddr < gData.mpShared->LoadInfo.mKernArenaLow);
#endif
    //
    // make sure all virtual heap allocations that are not
    // tracking structures have virtuap map objects
    //
//    K2OSKERN_Debug("%08X ARENA_LOW\n", gData.mpShared->LoadInfo.mKernArenaLow);
    do
    {
//        K2OSKERN_Debug("\nITERATE\n");

        pHeapNode = K2HEAP_GetFirstNode(&gData.Virt.Heap);
        K2_ASSERT(NULL != pHeapNode);

        do
        {
            if (!K2HEAP_NodeIsFree(pHeapNode))
            {
                if (0 == K2HEAP_NODE_USERBIT(pHeapNode))
                {
                    // not a tracking alloc and not free
                    virtAddr = pHeapNode->AddrTreeNode.mUserVal;
                    pageCount = pHeapNode->SizeTreeNode.mUserVal / K2_VA_MEMPAGE_BYTES;
                    pTreeNode = K2TREE_Find(&gData.VirtMap.Tree, pHeapNode->AddrTreeNode.mUserVal);
                    if (NULL == pTreeNode)
                    {
//                        K2OSKERN_Debug("%08X %08X NEED_MAP\n", pHeapNode->AddrTreeNode.mUserVal, pHeapNode->SizeTreeNode.mUserVal);
                        refMap.AsAny = NULL;
                        KernVirtMap_CreatePreMap(virtAddr, pageCount, K2OS_MapType_Data_ReadWrite, &refMap);
                        refMap.AsVirtMap->Hdr.mObjFlags |= K2OSKERN_OBJ_FLAG_PERMANENT;
                        KernObj_ReleaseRef(&refMap);
                        break;
                    }
//                    else
//                    {
//                        K2OSKERN_Debug("%08X %08X IN_TREE\n", pHeapNode->AddrTreeNode.mUserVal, pHeapNode->SizeTreeNode.mUserVal);
//                    }
                }
                else
                {
//                    K2OSKERN_Debug("%08X %08X TRACKING\n", pHeapNode->AddrTreeNode.mUserVal, pHeapNode->SizeTreeNode.mUserVal);
                }
            }
            else
            {
//                K2OSKERN_Debug("%08X %08X FREE\n", pHeapNode->AddrTreeNode.mUserVal, pHeapNode->SizeTreeNode.mUserVal);
            }

            pHeapNode = K2HEAP_GetNextNode(&gData.Virt.Heap, pHeapNode);

        } while (NULL != pHeapNode);

    } while (NULL != pHeapNode);
//    K2OSKERN_Debug("%08X ARENA_HIGH\n", gData.mpShared->LoadInfo.mKernArenaHigh);

#if 0
    virtAddr = gData.mpShared->LoadInfo.mKernArenaHigh;
    pageCount = ((virtAddr + (K2_VA32_PAGETABLE_MAP_BYTES - 1)) / K2_VA32_PAGETABLE_MAP_BYTES) * K2_VA32_PAGETABLE_MAP_BYTES;
    pteAddr = K2OS_KVA_TO_PTE_ADDR(virtAddr);
    while (virtAddr < pageCount)
    {
        pte = *((UINT32 *)pteAddr);
        if ((0 != (pte & K2OSKERN_PTE_PRESENT_BIT)) &&
            ((pte & K2_VA_PAGEFRAME_MASK) != gData.mpShared->LoadInfo.mZeroPagePhys))
        {
            K2OSKERN_Debug("  %08X %08X\n", virtAddr, pte);
        }
        pteAddr += sizeof(pte);
        virtAddr += K2_VA_MEMPAGE_BYTES;
    }
    do
    {
        ptAddr = K2OS_KVA_TO_PT_ADDR(virtAddr);
        ptPteAddr = K2OS_KVA_TO_PTE_ADDR(ptAddr);
        ptPte = *((UINT32 *)ptPteAddr);
        if (0 == (ptPte & K2OSKERN_PTE_PRESENT_BIT))
        {
            K2OSKERN_Debug("%08X %08X UNMAPPPED PT\n", virtAddr, K2_VA32_PAGETABLE_MAP_BYTES);
            virtAddr += K2_VA32_PAGETABLE_MAP_BYTES;
        }
        else
        {
            pageCount = K2_VA32_ENTRIES_PER_PAGETABLE;
            pteAddr = ptAddr;
            do
            {
                pte = *((UINT32 *)pteAddr);
                if ((0 != (pte & K2OSKERN_PTE_PRESENT_BIT)) &&
                    ((pte & K2_VA_PAGEFRAME_MASK) != gData.mpShared->LoadInfo.mZeroPagePhys))
                {
                    K2OSKERN_Debug("  %08X %08X\n", virtAddr, pte);
                }
                virtAddr += K2_VA_MEMPAGE_BYTES;
                pteAddr += sizeof(pte);
            } while (--pageCount);
        }
    } while (virtAddr > 0);
#endif

#if CHECK_LOGIC_FAULT
    sgFault = FALSE;
#endif
    gData.Virt.mKernHeapThreaded = TRUE;
    K2_CpuWriteBarrier();
}

