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
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"Token
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

K2HEAP_NODE * 
KernProc_UserHeapNodeAcquire(
    K2HEAP_ANCHOR *apHeap
)
{
    K2OSKERN_PROCVIRT*              pUserVirt;
    K2OSKERN_PROCHEAP_NODE*         pNode;
    K2OSKERN_PROCHEAP_NODEBLOCK*    pBlock;

    pUserVirt = K2_GET_CONTAINER(K2OSKERN_PROCVIRT, apHeap, Locked.HeapAnchor);

    if (pUserVirt->Locked.FreeNodeList.mNodeCount == 0)
    {
        K2_ASSERT(0);   // if hit in debug we want to know
        return NULL;
    }

    pNode = (K2OSKERN_PROCHEAP_NODE *)pUserVirt->Locked.FreeNodeList.mpHead;

    K2LIST_Remove(&pUserVirt->Locked.FreeNodeList, pUserVirt->Locked.FreeNodeList.mpHead);

    pBlock = K2_GET_CONTAINER(K2OSKERN_PROCHEAP_NODEBLOCK, pNode, Node[pNode->mIxInBlock]);
    
    pBlock->mUseMask |= (1ull << pNode->mIxInBlock);

    return &pNode->HeapNode;
}

void          
KernProc_UserHeapNodeRelease(
    K2HEAP_ANCHOR * apHeap,
    K2HEAP_NODE *   apNode
)
{
    K2OSKERN_PROCVIRT*              pUserVirt;
    K2OSKERN_PROCHEAP_NODE*         pNode;
    K2OSKERN_PROCHEAP_NODEBLOCK*    pBlock;
    UINT32                          ix;

    pUserVirt = K2_GET_CONTAINER(K2OSKERN_PROCVIRT, apHeap, Locked.HeapAnchor);
    pNode = K2_GET_CONTAINER(K2OSKERN_PROCHEAP_NODE, apNode, HeapNode);
    K2_ASSERT(0 == pNode->mMapCount);
    pBlock = K2_GET_CONTAINER(K2OSKERN_PROCHEAP_NODEBLOCK, pNode, Node[pNode->mIxInBlock]);

    K2_ASSERT(0 != (pBlock->mUseMask & (1ull << pNode->mIxInBlock)));
    K2LIST_AddAtTail(&pUserVirt->Locked.FreeNodeList, (K2LIST_LINK *)pNode);
    pBlock->mUseMask &= ~(1ull << pNode->mIxInBlock);

    if (0 != pBlock->mUseMask)
        return;

    //
    // block is empty - remove all entries from free list and block from block list
    //
    for (ix = 0; ix < 64; ix++)
    {
        K2LIST_Remove(&pUserVirt->Locked.FreeNodeList, (K2LIST_LINK *)&pBlock->Node[ix]);
    }
    K2LIST_Remove(&pUserVirt->Locked.BlockList, &pBlock->BlockListLink);

    //
    // get heap wrapper to free this block when its done
    //
    pUserVirt->Locked.mpBlockToFree = pBlock;
}

K2STAT
KernProc_CreateRawProc(
    K2OSKERN_OBJ_PROCESS ** appRetProc
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_PHYSRES        res;
    UINT32                  virtMapKVA;
    BOOL                    disp;
    UINT32                  pagesLeft;
    K2OSKERN_OBJREF         xdlTrackPageArrayRef;

    if (!KernPhys_Reserve_Init(&res, gData.User.mInitPageCount))
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    do
    {
        stat = K2STAT_NO_ERROR;

        xdlTrackPageArrayRef.AsAny = NULL;
        stat = KernPageArray_CreateSparse(2, K2OS_MEMPAGE_ATTR_READWRITE, &xdlTrackPageArrayRef);
        if (K2STAT_IS_ERROR(stat))
            break;

        do {

            virtMapKVA = KernVirt_Reserve((K2_VA32_PAGEFRAMES_FOR_2G * sizeof(UINT32)) / K2_VA_MEMPAGE_BYTES);
            if (0 == virtMapKVA)
            {
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
                break;
            }

            do
            {
                pProc = (K2OSKERN_OBJ_PROCESS *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_PROCESS));
                if (NULL == pProc)
                {
                    stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    break;
                }

                K2MEM_Zero(pProc, sizeof(K2OSKERN_OBJ_PROCESS));

                pProc->Hdr.mObjType = KernObj_Process;
                K2LIST_Init(&pProc->Hdr.RefObjList);

                pProc->mState = KernProcState_InRawCreate;

                K2OSKERN_SeqInit(&pProc->Virt.SeqLock);
                K2HEAP_Init(&pProc->Virt.Locked.HeapAnchor, KernProc_UserHeapNodeAcquire, KernProc_UserHeapNodeRelease);
                K2LIST_Init(&pProc->Virt.Locked.FreeNodeList);
                K2LIST_Init(&pProc->Virt.Locked.BlockList);
                K2TREE_Init(&pProc->Virt.Locked.MapTree, NULL);

                K2OSKERN_SeqInit(&pProc->PageList.SeqLock);
                K2LIST_Init(&pProc->PageList.Locked.Reserved_Dirty);
                K2LIST_Init(&pProc->PageList.Locked.Reserved_Clean);
                K2LIST_Init(&pProc->PageList.Locked.PageTables);
                K2LIST_Init(&pProc->PageList.Locked.ThreadTls);
                K2LIST_Init(&pProc->PageList.Locked.Tokens);

                K2OSKERN_SeqInit(&pProc->Token.SeqLock);
                K2LIST_Init(&pProc->Token.Locked.FreeList);
                pProc->Token.Locked.mSalt = K2OSKERN_TOKEN_SALT_MASK;

                K2OSKERN_SeqInit(&pProc->Thread.SeqLock);
                K2LIST_Init(&pProc->Thread.Locked.CreateList);
                K2LIST_Init(&pProc->Thread.SchedLocked.ActiveList);
                K2LIST_Init(&pProc->Thread.Locked.DeadList);

                pProc->mId = K2ATOMIC_AddExchange(&gData.Proc.mNextId, 1);

                pProc->mVirtMapKVA = virtMapKVA;

                stat = KernPhys_AllocPow2Bytes(&res, K2_VA32_PROC_TRANSTAB_SIZE, &pProc->Virt.mpPageDirTrack);
                K2_ASSERT(!K2STAT_IS_ERROR(stat));

                pagesLeft = gData.User.mInitPageCount - (K2_VA32_PROC_TRANSTAB_SIZE / K2_VA_MEMPAGE_BYTES);

                KernPhys_AllocSparsePages(&res, pagesLeft, &pProc->PageList.Locked.Reserved_Dirty);
                KernPhys_CutTrackListIntoPages(&pProc->PageList.Locked.Reserved_Dirty, FALSE, NULL, TRUE, KernPhysPageList_Proc_Res_Dirty);
                K2_ASSERT(pagesLeft == pProc->PageList.Locked.Reserved_Dirty.mNodeCount);

                do
                {
                    if (!KernArch_UserProcPrep(pProc))
                    {
                        KernHeap_Free(pProc);
                        pProc = NULL;
                        stat = K2STAT_ERROR_OUT_OF_MEMORY;
                        break;
                    }

                    //
                    // should not fail from here on in
                    //
                    pProc->mState = KernProcState_InBuild;

                    disp = K2OSKERN_SeqLock(&gData.Proc.SeqLock);

#if K2_TARGET_ARCH_IS_INTEL
                    //
                    // duplicate kernel part of kernel-only translation table into process transtable at 
                    // the point the process is added to the process list
                    //
                    K2MEM_Copy(
                        (void *)(pProc->mVirtTransBase + 0x800),
                        (void *)(K2OS_KVA_TRANSTAB_BASE + 0x800),
                        0x800);
#endif

                    K2ATOMIC_Inc(&gData.Proc.mExistCount);
                    K2LIST_AddAtTail(&gData.Proc.List, &pProc->GlobalProcListLink);

                    KernObj_CreateRef(&pProc->XdlTrackPageArrayRef, xdlTrackPageArrayRef.AsAny);

                    K2OSKERN_SeqUnlock(&gData.Proc.SeqLock, disp);

                } while (0);

                if (K2STAT_IS_ERROR(stat))
                {
                    KernPhys_FreeTrackList(&pProc->PageList.Locked.Reserved_Dirty);
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                KernVirt_Release(virtMapKVA);
            }

        } while (0);

        KernObj_ReleaseRef(&xdlTrackPageArrayRef);

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        KernPhys_Reserve_Release(&res);
    }
    else
    {
        *appRetProc = pProc;
    }

    return stat;
}

K2STAT
KernProc_UserVirtHeapReplenish(
    K2OSKERN_OBJ_PROCESS *apProc
)
{
    UINT32                          ix;
    K2OSKERN_PROCHEAP_NODEBLOCK*    pBlock;
    K2LIST_ANCHOR                   newList;
    BOOL                            disp;

    pBlock = (K2OSKERN_PROCHEAP_NODEBLOCK *)KernHeap_Alloc(sizeof(K2OSKERN_PROCHEAP_NODEBLOCK));
    if (NULL == pBlock)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2LIST_Init(&newList);

    pBlock->mUseMask = 0;
    for (ix = 0; ix < 64; ix++)
    {
        pBlock->Node[ix].mIxInBlock = ix;
        pBlock->Node[ix].mUserOwned = 0;
        pBlock->Node[ix].mMapCount = 0;
        K2LIST_AddAtTail(&newList, (K2LIST_LINK *)&pBlock->Node[ix]);
    }

    disp = K2OSKERN_SeqLock(&apProc->Virt.SeqLock);

    if (apProc->Virt.Locked.FreeNodeList.mNodeCount < 8)
    {
        K2LIST_AppendToTail(&apProc->Virt.Locked.FreeNodeList, &newList);
        K2LIST_AddAtTail(&apProc->Virt.Locked.BlockList, &pBlock->BlockListLink);
        pBlock = NULL;
    }

    K2OSKERN_SeqUnlock(&apProc->Virt.SeqLock, disp);

    if (NULL == pBlock)
        return K2STAT_NO_ERROR;

    //
    // block was needed but now it is not
    //

    KernHeap_Free(pBlock);

    return K2STAT_NO_ERROR;
}

K2STAT  
KernProc_UserVirtHeapInit(
    K2OSKERN_OBJ_PROCESS *apProc
)
{
    K2STAT stat;

    apProc->Virt.Locked.mBotPt = gData.User.mBotInitPt;
    apProc->Virt.Locked.mTopPt = gData.User.mTopInitPt;

    K2_ASSERT(0 == apProc->Virt.Locked.FreeNodeList.mNodeCount);
    stat = KernProc_UserVirtHeapReplenish(apProc);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    stat = K2HEAP_AddFreeSpaceNode(&apProc->Virt.Locked.HeapAnchor, K2OS_UVA_LOW_BASE, apProc->Virt.Locked.mBotPt - K2OS_UVA_LOW_BASE, NULL);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    return K2HEAP_AddFreeSpaceNode(&apProc->Virt.Locked.HeapAnchor, apProc->Virt.Locked.mTopPt, K2OS_KVA_KERN_BASE - apProc->Virt.Locked.mTopPt, NULL);
}

K2STAT
KernProc_UserVirtHeapExpandToFit(
    K2OSKERN_OBJ_PROCESS *      apProc,
    UINT32                      aBytes
)
{
    K2STAT                  stat;
    UINT32                  maxNumPageTablesNeeded;
    K2LIST_ANCHOR           trackList;
    K2OSKERN_PHYSRES        res;
    BOOL                    disp;
    UINT32                  availAtTop;
    UINT32                  availAtBot;
    UINT32                  endOfBotNode;
    K2HEAP_NODE *           pHeapNode;
    UINT32                  availUnusedSpace;
    K2OSKERN_PHYSTRACK *    pTrack;

    maxNumPageTablesNeeded = (aBytes + (K2_VA32_PAGETABLE_MAP_BYTES - 1)) / K2_VA32_PAGETABLE_MAP_BYTES;

    stat = KernPhys_Reserve_Init(&res, maxNumPageTablesNeeded);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    stat = KernPhys_AllocSparsePages(&res, maxNumPageTablesNeeded, &trackList);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    KernPhys_CutTrackListIntoPages(&trackList, FALSE, NULL, FALSE, KernPhysPageList_None);

    disp = K2OSKERN_SeqLock(&apProc->Virt.SeqLock);

    do
    {
        availUnusedSpace = (apProc->Virt.Locked.mTopPt - apProc->Virt.Locked.mBotPt);
        if (0 == availUnusedSpace)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
            break;
        }

        pHeapNode = K2HEAP_GetFirstNodeInRange(
            &apProc->Virt.Locked.HeapAnchor,
            apProc->Virt.Locked.mTopPt,
            K2OS_KVA_KERN_BASE - apProc->Virt.Locked.mTopPt);
        if (NULL == pHeapNode)
        {
            availAtTop = 0;
        }
        else
        {
            K2_ASSERT(pHeapNode->AddrTreeNode.mUserVal >= apProc->Virt.Locked.mTopPt);
            availAtTop = (pHeapNode->AddrTreeNode.mUserVal - apProc->Virt.Locked.mTopPt);
        }

        if (NULL == pHeapNode)
        {
            pHeapNode = K2HEAP_GetLastNode(
                &apProc->Virt.Locked.HeapAnchor
            );
        }
        else
        {
            pHeapNode = K2HEAP_GetPrevNode(
                &apProc->Virt.Locked.HeapAnchor,
                pHeapNode
            );
        }
        K2_ASSERT(pHeapNode != NULL);
        endOfBotNode = K2HEAP_NodeAddr(pHeapNode) + K2HEAP_NodeSize(pHeapNode);
        K2_ASSERT(endOfBotNode <= apProc->Virt.Locked.mBotPt);
        availAtBot = apProc->Virt.Locked.mBotPt - endOfBotNode;
        if (K2HEAP_NodeIsFree(pHeapNode))
        {
            availAtBot += K2HEAP_NodeSize(pHeapNode);
        }

        //
        // which part has to expand the least?
        //
        if (availAtTop > availAtBot)
        {
            do
            {
                K2_ASSERT(apProc->Virt.Locked.mTopPt > apProc->Virt.Locked.mBotPt);

                pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, trackList.mpHead, ListLink);
                
                K2LIST_Remove(&trackList, &pTrack->ListLink);
                
                pTrack->Flags.Field.PageListIx = KernPhysPageList_Proc_PageTables;
                
                K2OSKERN_SeqLock(&apProc->PageList.SeqLock);
                K2LIST_AddAtTail(&apProc->PageList.Locked.PageTables, &pTrack->ListLink);
                K2OSKERN_SeqUnlock(&apProc->PageList.SeqLock, FALSE);

                KernArch_InstallPageTable(
                    apProc, 
                    apProc->Virt.Locked.mTopPt - K2_VA32_PAGETABLE_MAP_BYTES, 
                    K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack)
                );

                apProc->Virt.Locked.mTopPt -= K2_VA32_PAGETABLE_MAP_BYTES;

                K2HEAP_AddFreeSpaceNode(
                    &apProc->Virt.Locked.HeapAnchor, 
                    apProc->Virt.Locked.mTopPt, 
                    K2_VA32_PAGETABLE_MAP_BYTES,
                    NULL);

                availAtTop += K2_VA32_PAGETABLE_MAP_BYTES;

            } while (availAtTop < aBytes);
        }
        else
        {
            do
            {
                K2_ASSERT(apProc->Virt.Locked.mBotPt < apProc->Virt.Locked.mTopPt);

                pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, trackList.mpHead, ListLink);
                
                K2LIST_Remove(&trackList, &pTrack->ListLink);
                
                pTrack->Flags.Field.PageListIx = KernPhysPageList_Proc_PageTables;
                
                K2OSKERN_SeqLock(&apProc->PageList.SeqLock);
                K2LIST_AddAtTail(&apProc->PageList.Locked.PageTables, &pTrack->ListLink);
                K2OSKERN_SeqUnlock(&apProc->PageList.SeqLock, FALSE);

                KernArch_InstallPageTable(
                    apProc, 
                    apProc->Virt.Locked.mBotPt, 
                    K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack)
                );

                K2HEAP_AddFreeSpaceNode(
                    &apProc->Virt.Locked.HeapAnchor, 
                    apProc->Virt.Locked.mBotPt, 
                    K2_VA32_PAGETABLE_MAP_BYTES,
                    NULL);

                apProc->Virt.Locked.mBotPt += K2_VA32_PAGETABLE_MAP_BYTES;

                availAtBot += K2_VA32_PAGETABLE_MAP_BYTES;

            } while (availAtBot < aBytes);
        }

    } while (0);

    K2OSKERN_SeqUnlock(&apProc->Virt.SeqLock, disp);

    KernPhys_FreeTrackList(&trackList);

    KernPhys_Reserve_Release(&res);

    return stat;
}

K2STAT
KernProc_UserVirtHeapExpandToCover(
    K2OSKERN_OBJ_PROCESS *  apProc,
    UINT32                  aVirtAddr,
    UINT32                  aBytes
)
{
    K2STAT              stat;
    UINT32              workAddr;
    BOOL                mapUp;
    UINT32              ptCount;
    K2OSKERN_PHYSRES    res;
    K2LIST_ANCHOR       trackList;
    K2OSKERN_PHYSTRACK *pTrack;

    if (((~aVirtAddr) + 1) <= aBytes)
    {
        // wraps end of address space
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    workAddr = aVirtAddr + (aBytes - 1);

    if ((workAddr < apProc->Virt.Locked.mBotPt) ||
        (aVirtAddr >= apProc->Virt.Locked.mTopPt) ||
        (apProc->Virt.Locked.mBotPt == apProc->Virt.Locked.mTopPt))
    {
        return K2STAT_ERROR_IN_USE;
    }

    //
    // requested range overlaps unmappable area somewhere
    //
    if (aVirtAddr < apProc->Virt.Locked.mBotPt)
    {
        // first byte is below bottom bar - map up to last Byte
        mapUp = TRUE;
    }
    else if (workAddr >= apProc->Virt.Locked.mTopPt)
    {
        // last byte is above top bar - map down to Virt Addr
        mapUp = FALSE;
        workAddr = aVirtAddr;
    }
    else
    {
        //
        // requested area is completely within unmappable space
        // so we need to go up or down to the nearest end
        //
        if ((apProc->Virt.Locked.mTopPt - aVirtAddr) <
            (workAddr - apProc->Virt.Locked.mBotPt))
        {
            mapUp = FALSE;
            workAddr = aVirtAddr;
        }
        else
        {
            mapUp = TRUE;
        }
    }

    //
    // map up or down to include workaddr
    //
    if (mapUp)
    {
        K2_ASSERT(workAddr > apProc->Virt.Locked.mBotPt);
        ptCount = ((workAddr - apProc->Virt.Locked.mBotPt) + (K2_VA32_PAGETABLE_MAP_BYTES - 1)) / K2_VA32_PAGETABLE_MAP_BYTES;
    }
    else
    {
        K2_ASSERT(workAddr < apProc->Virt.Locked.mTopPt);
        ptCount = ((apProc->Virt.Locked.mTopPt - workAddr) + (K2_VA32_PAGETABLE_MAP_BYTES - 1)) / K2_VA32_PAGETABLE_MAP_BYTES;
    }

    stat = KernPhys_Reserve_Init(&res, ptCount);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    stat = KernPhys_AllocSparsePages(&res, ptCount, &trackList);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    KernPhys_CutTrackListIntoPages(&trackList, FALSE, NULL, FALSE, KernPhysPageList_None);

    do
    {
        K2_ASSERT(NULL != trackList.mpHead);

        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, trackList.mpHead, ListLink);

        K2LIST_Remove(&trackList, &pTrack->ListLink);

        pTrack->Flags.Field.PageListIx = KernPhysPageList_Proc_PageTables;

        K2OSKERN_SeqLock(&apProc->PageList.SeqLock);
        K2LIST_AddAtTail(&apProc->PageList.Locked.PageTables, &pTrack->ListLink);
        K2OSKERN_SeqUnlock(&apProc->PageList.SeqLock, FALSE);

        if (mapUp)
        {
            KernArch_InstallPageTable(
                apProc,
                apProc->Virt.Locked.mBotPt,
                K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack)
            );

            apProc->Virt.Locked.mBotPt += K2_VA32_PAGETABLE_MAP_BYTES;
        }
        else
        {
            apProc->Virt.Locked.mTopPt -= K2_VA32_PAGETABLE_MAP_BYTES;

            KernArch_InstallPageTable(
                apProc,
                apProc->Virt.Locked.mTopPt,
                K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack)
            );
        }
    } while (--ptCount);

    if (mapUp)
    {
        K2_ASSERT(workAddr < apProc->Virt.Locked.mBotPt);
    }
    else
    {
        K2_ASSERT(workAddr >= apProc->Virt.Locked.mTopPt);
    }

    return K2STAT_NO_ERROR;
}

K2STAT
KernProc_UserVirtHeapAlloc_Internal(
    K2OSKERN_OBJ_PROCESS *      apProc,
    UINT32                      aVirtAddr,
    UINT32                      aPageCount,
    K2OSKERN_PROCHEAP_NODE **   apRetProcHeapNode
)
{
    BOOL                            disp;
    BOOL                            replenish;
    UINT32                          bytesRequested;
    K2STAT                          stat;
    K2HEAP_NODE *                   pHeapNode;
    K2OSKERN_PROCHEAP_NODEBLOCK*    pBlock;
    BOOL                            triedOnce;

    K2_ASSERT(0 != aPageCount);
    bytesRequested = aPageCount * K2_VA_MEMPAGE_BYTES;

    pBlock = NULL;
    triedOnce = FALSE;

    do
    {
        disp = K2OSKERN_SeqLock(&apProc->Virt.SeqLock);

        replenish = (apProc->Virt.Locked.FreeNodeList.mNodeCount < 3) ? TRUE : FALSE;
        if (!replenish)
        {
            if (aVirtAddr == 0xFFFFFFFF)
            {
//                stat = K2HEAP_AllocNodeBest(&apProc->Virt.Locked.HeapAnchor, bytesRequested, 0, &pHeapNode);
                stat = K2HEAP_AllocNodeLowest(&apProc->Virt.Locked.HeapAnchor, bytesRequested, 0, &pHeapNode);
            }
            else
            {
                stat = K2HEAP_AllocNodeAt(&apProc->Virt.Locked.HeapAnchor, aVirtAddr, bytesRequested, &pHeapNode);
            }
            if (!K2STAT_IS_ERROR(stat))
            {
                *apRetProcHeapNode = K2_GET_CONTAINER(K2OSKERN_PROCHEAP_NODE, pHeapNode, HeapNode);
                pBlock = apProc->Virt.Locked.mpBlockToFree;
                apProc->Virt.Locked.mpBlockToFree = NULL;
            }
        }

        K2OSKERN_SeqUnlock(&apProc->Virt.SeqLock, disp);

        if (replenish)
        {
            stat = KernProc_UserVirtHeapReplenish(apProc);
            if (K2STAT_IS_ERROR(stat))
                break;
        }
        else
        {
            if (!K2STAT_IS_ERROR(stat))
                break;

            if (stat == K2STAT_ERROR_OUT_OF_MEMORY)
            {
                if (aVirtAddr == 0xFFFFFFFF)
                {
                    // allocbest - try to expand to fit
                    stat = KernProc_UserVirtHeapExpandToFit(apProc, bytesRequested);
                }
            }
            else if ((stat == K2STAT_ERROR_OUT_OF_BOUNDS) ||
                     (stat == K2STAT_ERROR_TOO_BIG))
            {
                if ((aVirtAddr != 0xFFFFFFFF) && (!triedOnce))
                { 
                    // allocAt - try to cover requested range
                    triedOnce = TRUE;
                    stat = KernProc_UserVirtHeapExpandToCover(apProc, aVirtAddr, bytesRequested);
                }
            }
            if (K2STAT_IS_ERROR(stat))
                break;
        }

    } while (1);

    if (NULL != pBlock)
    {
        KernHeap_Free(pBlock);
    }

    return stat;
}

K2STAT 
KernProc_UserVirtHeapAlloc(
    K2OSKERN_OBJ_PROCESS *      apProc, 
    UINT32                      aPageCount,
    K2OSKERN_PROCHEAP_NODE **   apRetProcHeapNode
)
{
    K2_ASSERT(aPageCount > 0);
    return KernProc_UserVirtHeapAlloc_Internal(apProc, 0xFFFFFFFF, aPageCount, apRetProcHeapNode);
}

K2STAT  
KernProc_UserVirtHeapAllocAt(
    K2OSKERN_OBJ_PROCESS *      apProc,
    UINT32                      aVirtAddr,
    UINT32                      aPageCount,
    K2OSKERN_PROCHEAP_NODE **   apRetProcHeapNode
)
{
    K2_ASSERT(0 == (K2_VA_MEMPAGE_OFFSET_MASK & aVirtAddr));
    K2_ASSERT(aVirtAddr < K2OS_KVA_KERN_BASE);
    K2_ASSERT(aPageCount > 0);
    return KernProc_UserVirtHeapAlloc_Internal(apProc, aVirtAddr, aPageCount, apRetProcHeapNode);
}

K2STAT
KernProc_UserVirtHeapFree(
    K2OSKERN_OBJ_PROCESS *  apProc, 
    UINT32                  aVirtAddr
)
{
    BOOL                            disp;
    BOOL                            replenish;
    K2STAT                          stat;
    K2OSKERN_PROCHEAP_NODEBLOCK*    pBlock;
    K2HEAP_NODE *                   pHeapNode;
    K2OSKERN_PROCHEAP_NODE *        pProcHeapNode;

    K2_ASSERT(0 == (K2_VA_MEMPAGE_OFFSET_MASK & aVirtAddr));
    K2_ASSERT(aVirtAddr < K2OS_KVA_KERN_BASE);

    pBlock = NULL;

    do
    {
        disp = K2OSKERN_SeqLock(&apProc->Virt.SeqLock);

        replenish = (apProc->Virt.Locked.FreeNodeList.mNodeCount < 3) ? TRUE : FALSE;
        if (!replenish)
        {
            pHeapNode = K2HEAP_FindNodeContainingAddr(&apProc->Virt.Locked.HeapAnchor, aVirtAddr);
            if (NULL == pHeapNode)
            {
                stat = K2STAT_ERROR_NOT_FOUND;
            }
            else if (K2HEAP_NodeAddr(pHeapNode) != aVirtAddr)
            {
                stat = K2STAT_ERROR_BAD_ARGUMENT;
            }
            else
            {
                pProcHeapNode = K2_GET_CONTAINER(K2OSKERN_PROCHEAP_NODE, pHeapNode, HeapNode);
                pProcHeapNode->mMapCount = 0;
                stat = K2HEAP_FreeNode(&apProc->Virt.Locked.HeapAnchor, pHeapNode);
                K2_ASSERT(!K2STAT_IS_ERROR(stat));
                pBlock = apProc->Virt.Locked.mpBlockToFree;
                apProc->Virt.Locked.mpBlockToFree = NULL;
            }
        }

        K2OSKERN_SeqUnlock(&apProc->Virt.SeqLock, disp);

        if (!replenish)
            break;

        stat = KernProc_UserVirtHeapReplenish(apProc);
        if (K2STAT_IS_ERROR(stat))
            break;

    } while (1);

    if (NULL != pBlock)
    {
        KernHeap_Free(pBlock);
    }

    return stat;
}

K2STAT
KernProc_CreateDefaultMap(
    K2OSKERN_OBJ_PROCESS *      apProc,
    K2OSKERN_OBJ_PAGEARRAY *    apPageArray,
    UINT32                      aProcVirt,
    K2OS_VirtToPhys_MapType     aMapType,
    K2OSKERN_OBJREF *           apRetMapRef
)
{
    K2STAT                      stat;
    K2STAT                      stat2;
    K2OSKERN_PROCHEAP_NODE *    pProcHeapNode;

    stat = KernProc_UserVirtHeapAllocAt(
        apProc,
        aProcVirt,
        apPageArray->mPageCount,
        &pProcHeapNode);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));
    if (K2STAT_IS_ERROR(stat))
        return stat;

    stat = KernVirtMap_CreateUser(
        apProc,
        apPageArray,
        0,
        aProcVirt,
        apPageArray->mPageCount,
        aMapType,
        apRetMapRef);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));
    if (K2STAT_IS_ERROR(stat))
    {
        stat2 = KernProc_UserVirtHeapFree(apProc, aProcVirt);
        K2_ASSERT(!K2STAT_IS_ERROR(stat2));
        return stat;
    }

    pProcHeapNode->mUserOwned = FALSE;

    return K2STAT_NO_ERROR;
}

K2STAT
KernProc_BuildProcess(
    K2OSKERN_OBJ_PROCESS *  apProc
)
{
    K2STAT                  stat;
    UINT32                  bar;
    K2OSKERN_PHYSTRACK *    pTrack;
    UINT32                  physPageAddr;

//    K2OSKERN_Debug("Process %d being built\n", apProc->mId);

    //
    // start by shoving in pagetables at top and bottom
    //
    bar = K2OS_KVA_KERN_BASE; 
    do
    {
        bar -= K2_VA32_PAGETABLE_MAP_BYTES;
        K2_ASSERT(0 != apProc->PageList.Locked.Reserved_Dirty.mNodeCount);
        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, apProc->PageList.Locked.Reserved_Dirty.mpHead, ListLink);
        K2LIST_Remove(&apProc->PageList.Locked.Reserved_Dirty, &pTrack->ListLink);
        pTrack->Flags.Field.PageListIx = KernPhysPageList_Proc_PageTables;
        K2LIST_AddAtTail(&apProc->PageList.Locked.PageTables, &pTrack->ListLink);
        physPageAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);
        KernPhys_ZeroPage(physPageAddr);
        KernArch_InstallPageTable(apProc, bar, physPageAddr);
    } while (bar > gData.User.mTopInitPt);
    apProc->Virt.Locked.mTopPt = gData.User.mTopInitPt;

    bar = 0;    // first page is tls pagetable for this process
    do
    {
        K2_ASSERT(0 != apProc->PageList.Locked.Reserved_Dirty.mNodeCount);
        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, apProc->PageList.Locked.Reserved_Dirty.mpHead, ListLink);
        K2LIST_Remove(&apProc->PageList.Locked.Reserved_Dirty, &pTrack->ListLink);
        pTrack->Flags.Field.PageListIx = KernPhysPageList_Proc_PageTables;
        K2LIST_AddAtTail(&apProc->PageList.Locked.PageTables, &pTrack->ListLink);
        physPageAddr = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);
        KernPhys_ZeroPage(physPageAddr);
        KernArch_InstallPageTable(apProc, bar, physPageAddr);
        bar += K2_VA32_PAGETABLE_MAP_BYTES;
    } while (bar < gData.User.mBotInitPt);
    apProc->Virt.Locked.mBotPt = gData.User.mBotInitPt;

    //
    // init proces virtual heap now (adds Free space that is mappable)
    //
    KernProc_UserVirtHeapInit(apProc);

    do
    {
        //
        // publicapi
        //
//        K2OSKERN_Debug("Create PublicApi Map\n");
        stat = KernProc_CreateDefaultMap(
            apProc,
            &gData.User.PublicApiPageArray,
            K2OS_UVA_PUBLICAPI_BASE,
            K2OS_MapType_Text,
            &apProc->PublicApiMapRef
        );
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        if (K2STAT_IS_ERROR(stat))
            break;

        //
        // timerio page
        //
//        K2OSKERN_Debug("Create TimerIo Map\n");
        stat = KernProc_CreateDefaultMap(
            apProc,
            &gData.User.TimerPageArray,
            K2OS_UVA_TIMER_IOPAGE_BASE,
            K2OS_MapType_MemMappedIo_ReadOnly,
            &apProc->TimerPageMapRef
        );
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        if (K2STAT_IS_ERROR(stat))
            break;

        //
        // rofs range
        //
//        K2OSKERN_Debug("Create FileSys Map\n");
        K2_ASSERT((K2OS_UVA_TIMER_IOPAGE_BASE - (gData.FileSys.RefRofsVirtMap.AsVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES)) == gData.User.mTopInitVirtBar);
        stat = KernProc_CreateDefaultMap(
            apProc,
            gData.FileSys.RefRofsVirtMap.AsVirtMap->PageArrayRef.AsPageArray,
            K2OS_UVA_TIMER_IOPAGE_BASE - (gData.FileSys.RefRofsVirtMap.AsVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES),
            K2OS_MapType_Data_ReadOnly,
            &apProc->FileSysMapRef
        );
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        if (K2STAT_IS_ERROR(stat))
            break;

        bar = K2OS_UVA_LOW_BASE;
//        K2OSKERN_Debug("%5d bar=%08X\n", __LINE__, bar);

        //
        // crt xdltrack
        //
//        K2OSKERN_Debug("Create XdlTrack Map\n");
        stat = KernProc_CreateDefaultMap(
            apProc,
            apProc->XdlTrackPageArrayRef.AsPageArray,
            bar,
            K2OS_MapType_Data_ReadWrite,
            &apProc->XdlTrackMapRef
        );
        KernObj_ReleaseRef(&apProc->XdlTrackPageArrayRef);
        apProc->XdlTrackPageArrayRef.AsAny = NULL;
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        if (K2STAT_IS_ERROR(stat))
            break;
        bar += (apProc->XdlTrackMapRef.AsVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES);

        //
        // crt xdlheader
        //
//        K2OSKERN_Debug("Create XdlHeader Map\n");
        stat = KernProc_CreateDefaultMap(
            apProc,
            gData.User.mpKernUserCrtSeg[KernUserCrtSeg_XdlHeader],
            bar,
            K2OS_MapType_Data_ReadOnly,
            &apProc->CrtMapRef[KernUserCrtSeg_XdlHeader]
        );
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        if (K2STAT_IS_ERROR(stat))
            break;
        bar += (apProc->CrtMapRef[KernUserCrtSeg_XdlHeader].AsVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES);

        //
        // crt text
        //
//        K2OSKERN_Debug("Create Text Map\n");
        stat = KernProc_CreateDefaultMap(
            apProc,
            gData.User.mpKernUserCrtSeg[KernUserCrtSeg_Text],
            bar,
            K2OS_MapType_Text,
            &apProc->CrtMapRef[KernUserCrtSeg_Text]
        );
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        if (K2STAT_IS_ERROR(stat))
            break;
        bar += (apProc->CrtMapRef[KernUserCrtSeg_Text].AsVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES);

        //
        // crt readonly
        //
//        K2OSKERN_Debug("Create Read Map\n");
        stat = KernProc_CreateDefaultMap(
            apProc,
            gData.User.mpKernUserCrtSeg[KernUserCrtSeg_Read],
            bar,
            K2OS_MapType_Data_ReadOnly,
            &apProc->CrtMapRef[KernUserCrtSeg_Read]
        );
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        if (K2STAT_IS_ERROR(stat))
            break;
        bar += (apProc->CrtMapRef[KernUserCrtSeg_Read].AsVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES);

        //
        // crt data
        //
//        K2OSKERN_Debug("Create Data Map\n");
        stat = KernProc_CreateDefaultMap(
            apProc,
            gData.User.mpKernUserCrtSeg[KernUserCrtSeg_Data],
            bar,
            K2OS_MapType_Data_CopyOnWrite,
            &apProc->CrtMapRef[KernUserCrtSeg_Data]
        );
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        if (K2STAT_IS_ERROR(stat))
            break;
        bar += (apProc->CrtMapRef[KernUserCrtSeg_Data].AsVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES);

        //
        // crt symbols
        //
//        K2OSKERN_Debug("Create Symbols Map\n");
        stat = KernProc_CreateDefaultMap(
            apProc,
            gData.User.mpKernUserCrtSeg[KernUserCrtSeg_Sym],
            bar,
            K2OS_MapType_Data_ReadOnly, 
            &apProc->CrtMapRef[KernUserCrtSeg_Sym]
        );
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        if (K2STAT_IS_ERROR(stat))
            break;
        bar += (apProc->CrtMapRef[KernUserCrtSeg_Sym].AsVirtMap->mPageCount * K2_VA_MEMPAGE_BYTES);

        K2_ASSERT(bar == gData.User.mBotInitVirtBar);

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        //
        // set process state as exited and purge it
        //
        K2OSKERN_Panic("KernProc_BuildProcess failed during create - TBD\n");
    }

    return stat;
}

void 
KernProc_Init(
    void
)
{
    UINT32 *    pThreadPtrs;
    UINT32      ixThread;

    K2OSKERN_SeqInit(&gData.Proc.SeqLock);

    K2LIST_Init(&gData.Proc.List);

    //
    // make sure threadptrs page is clear
    //
    K2MEM_Zero((void *)K2OS_KVA_THREADPTRS_BASE, K2_VA_MEMPAGE_BYTES);

    //
    // init threadptrs list to a free list.  thread 0 is never used. thread 0x3FF is never used
    //
    pThreadPtrs = (UINT32 *)K2OS_KVA_THREADPTRS_BASE;
    pThreadPtrs[0] = 0; // never used, not valid
    for (ixThread = 1; ixThread < K2OS_MAX_THREAD_COUNT - 1; ixThread++)
        pThreadPtrs[ixThread] = ixThread + 1;
    pThreadPtrs[K2OS_MAX_THREAD_COUNT - 1] = 0;
    gData.Proc.mNextThreadSlotIx = 1;
    gData.Proc.mLastThreadSlotIx = K2OS_MAX_THREAD_COUNT;

    gData.Proc.mNextId = 1;

    KernThread_Init();
}

K2STAT
KernProc_FindMapAndCreateRef(
    K2OSKERN_OBJ_PROCESS *  apProc,
    UINT32                  aProcVirtAddr,
    K2OSKERN_OBJREF *       apFillMapRef,
    UINT32 *                apRetMapPageIx
)
{
    BOOL                    disp;
    K2STAT                  stat;
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_OBJ_VIRTMAP *  pMap;
    UINT32                  pageIx;

    if (aProcVirtAddr >= K2OS_KVA_KERN_BASE)
        return K2STAT_ERROR_NOT_FOUND;

    stat = K2STAT_ERROR_NOT_FOUND;

    disp = K2OSKERN_SeqLock(&apProc->Virt.SeqLock);

    pTreeNode = K2TREE_FindOrAfter(&apProc->Virt.Locked.MapTree, aProcVirtAddr);
    if (NULL == pTreeNode)
    {
        pTreeNode = K2TREE_LastNode(&apProc->Virt.Locked.MapTree);
        K2_ASSERT(NULL != pTreeNode);
    }
    else
    {
        if (pTreeNode->mUserVal != aProcVirtAddr)
            pTreeNode = K2TREE_PrevNode(&apProc->Virt.Locked.MapTree, pTreeNode);
    }
    if (NULL != pTreeNode)
    {
        pMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, pTreeNode, OwnerMapTreeNode);
        if (0 == (pMap->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO))
        {
            pageIx = (aProcVirtAddr - pTreeNode->mUserVal) / K2_VA_MEMPAGE_BYTES;
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

    K2OSKERN_SeqUnlock(&apProc->Virt.SeqLock, disp);

    return stat;
}

void
KernProc_FreeUnusedTokenPage(
    K2OSKERN_OBJ_PROCESS *  apProc,
    K2OSKERN_TOKEN_PAGE *   apPage)
{
    UINT32                  tokenPagePhys;
    K2OSKERN_PHYSTRACK *    pTrack;
    BOOL                    disp;

    tokenPagePhys = KernPte_BreakPageMap(NULL, (UINT32)apPage, 0);
    pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(tokenPagePhys);
    K2_ASSERT(KernPhysPageList_Proc_Token == pTrack->Flags.Field.PageListIx);
    disp = K2OSKERN_SeqLock(&apProc->PageList.SeqLock);
        K2LIST_Remove(&apProc->PageList.Locked.Tokens, &pTrack->ListLink);
    K2OSKERN_SeqUnlock(&apProc->PageList.SeqLock, disp);
    pTrack->Flags.Field.PageListIx = KernPhysPageList_None;
    KernPhys_FreeTrack(pTrack);
    KernVirt_Release((UINT32)apPage);
}

K2STAT  
KernProc_TokenCreate(
    K2OSKERN_OBJ_PROCESS *  apProc,
    K2OSKERN_OBJ_HEADER *   apObjHdr,
    K2OS_TOKEN *            apRetToken
)
{
    K2OSKERN_TOKEN_PAGE *   pNewTokenPage;
    K2OSKERN_TOKEN_PAGE **  ppNewTokenPageList;
    K2OSKERN_TOKEN_PAGE **  ppOldTokenPageList;
    UINT32                  newTokenPageListLen;
    UINT32                  value;
    K2LIST_ANCHOR           newTokenList;
    K2OSKERN_TOKEN *        pToken;
    K2LIST_LINK *           pListLink;
    K2OSKERN_TOKEN_PAGE *   pTokenPage;
    UINT32                  disp;
    K2OSKERN_PHYSTRACK *    pTrack;
    K2STAT                  stat;
    K2OSKERN_PHYSRES        res;

    K2_ASSERT(apObjHdr != NULL);
    K2_ASSERT(apRetToken != NULL);

    // these guys aren't allowed to have tokens.  use the 'user' variant
    K2_ASSERT(KernObj_Sem != apObjHdr->mObjType);

    *apRetToken = NULL;
    stat = K2STAT_NO_ERROR;

    //
    // make sure we can get the right # of tokens
    //
    pNewTokenPage = NULL;
    ppNewTokenPageList = NULL;
    newTokenPageListLen = 0;
    ppOldTokenPageList = NULL;
    do {
        disp = K2OSKERN_SeqLock(&apProc->Token.SeqLock);

        if (apProc->mState == KernProcState_Stopped)
        {
            stat = K2STAT_ERROR_PROCESS_EXITED;
            break;
        }

        if (0 != apProc->Token.Locked.FreeList.mNodeCount)
        {
            //
            // do not need to increase the token list size
            //
            break;
        }

        //
        // if we have already allocated resources for more tokens then
        // insert them and keep going
        //
        if ((pNewTokenPage != NULL) && (newTokenPageListLen == (apProc->Token.Locked.mPageCount + 1)))
        {
            //
            // expand the token list here. we are guaranteed to have enough tokens
            // for the request after this
            //
            K2_ASSERT(ppNewTokenPageList != NULL);
            K2MEM_Copy(ppNewTokenPageList, apProc->Token.Locked.mppPages, sizeof(K2OSKERN_TOKEN_PAGE *) * apProc->Token.Locked.mPageCount);
            ppNewTokenPageList[apProc->Token.Locked.mPageCount] = pNewTokenPage;
            pNewTokenPage->Hdr.mInUseCount = 0;
            pNewTokenPage->Hdr.mPageIndex = apProc->Token.Locked.mPageCount;
            K2LIST_AppendToTail(&apProc->Token.Locked.FreeList, &newTokenList);
            pNewTokenPage = NULL;
            ppOldTokenPageList = apProc->Token.Locked.mppPages;
            apProc->Token.Locked.mppPages = ppNewTokenPageList;
            apProc->Token.Locked.mPageCount++;
            break;
        }

        //
        // we need to expand the token page list
        //

        K2OSKERN_SeqUnlock(&apProc->Token.SeqLock, disp);

        if (ppNewTokenPageList != NULL)
        {
            KernHeap_Free(ppNewTokenPageList);
            ppNewTokenPageList = NULL;
        }
        newTokenPageListLen = apProc->Token.Locked.mPageCount + 1;    // snapshot
        ppNewTokenPageList = (K2OSKERN_TOKEN_PAGE **)KernHeap_Alloc(sizeof(K2OSKERN_TOKEN_PAGE *) * newTokenPageListLen);
        if (NULL == ppNewTokenPageList)
        {
            if (pNewTokenPage != NULL)
            {
                KernProc_FreeUnusedTokenPage(apProc,pNewTokenPage);
            }
            return K2STAT_ERROR_OUT_OF_MEMORY;
        }

        if (NULL == pNewTokenPage)
        {
            if (!KernPhys_Reserve_Init(&res, 1))
            {
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
            }
            else
            {
                stat = KernPhys_AllocPow2Bytes(&res, K2_VA_MEMPAGE_BYTES, &pTrack);
                if (K2STAT_IS_ERROR(stat))
                {
                    KernPhys_Reserve_Release(&res);
                }
            }
            if (K2STAT_IS_ERROR(stat))
            {
                KernHeap_Free(ppNewTokenPageList);
                return stat;
            }

            pNewTokenPage = (K2OSKERN_TOKEN_PAGE *)KernVirt_Reserve(1);
            if (NULL == pNewTokenPage)
            {
                KernPhys_FreeTrack(pTrack);
                KernHeap_Free(ppNewTokenPageList);
                return K2STAT_ERROR_OUT_OF_MEMORY;
            }

            pTrack->Flags.Field.PageListIx = KernPhysPageList_Proc_Token;
            disp = K2OSKERN_SeqLock(&apProc->PageList.SeqLock);
                K2LIST_AddAtTail(&apProc->PageList.Locked.Tokens, &pTrack->ListLink);
            K2OSKERN_SeqUnlock(&apProc->PageList.SeqLock, disp);

            KernPte_MakePageMap(NULL, (UINT32)pNewTokenPage, K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack), K2OS_MAPTYPE_KERN_DATA);

            K2LIST_Init(&newTokenList);

            for (value = 1; value < K2OSKERN_TOKENS_PER_PAGE; value++)
            {
                K2LIST_AddAtTail(&newTokenList, &pNewTokenPage->Tokens[value].FreeListLink);
            }
        }

    } while (1);

    if (!K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(0 < apProc->Token.Locked.FreeList.mNodeCount);

        pListLink = apProc->Token.Locked.FreeList.mpHead;
        K2LIST_Remove(&apProc->Token.Locked.FreeList, pListLink);
        pToken = K2_GET_CONTAINER(K2OSKERN_TOKEN, pListLink, FreeListLink);

        //
        // get the page of free token we just took
        //
        pTokenPage = (K2OSKERN_TOKEN_PAGE *)(((UINT32)pToken) & ~K2_VA_MEMPAGE_OFFSET_MASK);

        //
        // create the token value. low bit is ALWAYS set in a valid token
        // and ALWAYS clear in an invalid token
        //
        value = apProc->Token.Locked.mSalt | K2OSKERN_TOKEN_VALID_BIT;
        apProc->Token.Locked.mSalt = (apProc->Token.Locked.mSalt - K2OSKERN_TOKEN_SALT_DEC) & K2OSKERN_TOKEN_SALT_MASK;

        value |= pTokenPage->Hdr.mPageIndex * K2OSKERN_TOKENS_PER_PAGE * 2;
        value += (((UINT32)(pToken - ((K2OSKERN_TOKEN *)pTokenPage))) * 2);
        pToken->InUse.mTokValue = value;
        //    K2OSKERN_Debug("Proc %d Create token %08X\n", apProc->mId, value);

        pTokenPage->Hdr.mInUseCount++;

        KernObj_CreateRef(&pToken->InUse.Ref, apObjHdr);
        K2ATOMIC_Inc((INT32 volatile *)&apObjHdr->mTokenCount);

        *apRetToken = (K2OS_TOKEN)pToken->InUse.mTokValue;
        apProc->Token.Locked.mCount++;
    }

    K2OSKERN_SeqUnlock(&apProc->Token.SeqLock, disp);

    if (ppOldTokenPageList != NULL)
    {
        KernHeap_Free(ppOldTokenPageList);
    }

    if (pNewTokenPage != NULL)
    {
        //
        // it turned out we didn't need to expand the token list
        //
        KernProc_FreeUnusedTokenPage(apProc, pNewTokenPage);
        K2_ASSERT(NULL != ppNewTokenPageList);
        KernHeap_Free(ppNewTokenPageList);
    }

    return stat;
}

K2STAT  
KernProc_TokenTranslate(
    K2OSKERN_OBJ_PROCESS *  apProc,
    K2OS_TOKEN              aToken,
    K2OSKERN_OBJREF *       apRetRef
)
{
    K2STAT                  stat;
    UINT32                  tokenPageIndex;
    UINT32                  tokValue;
    K2OSKERN_TOKEN_PAGE *   pTokenPage;
    K2OSKERN_TOKEN *        pToken;
    UINT32                  disp;

    K2_ASSERT(aToken != NULL);
    K2_ASSERT(apRetRef != NULL);
    K2_ASSERT(NULL == apRetRef->AsAny);

    stat = K2STAT_NO_ERROR;

    disp = K2OSKERN_SeqLock(&apProc->Token.SeqLock);

    if (KernProcState_Stopped == apProc->mState)
    {
        stat = K2STAT_ERROR_PROCESS_EXITED;
    }
    else
    {
        do {
            tokValue = (UINT32)aToken;
            if ((0 == (tokValue & K2OSKERN_TOKEN_VALID_BIT)) ||
                (0 != (tokValue & K2OSKERN_TOKEN_KERNEL_BIT)))
            {
                stat = K2STAT_ERROR_BAD_TOKEN;
                break;
            }
            tokValue &= ~(K2OSKERN_TOKEN_SALT_MASK | K2OSKERN_TOKEN_VALID_BIT);

            tokenPageIndex = tokValue / (2 * K2OSKERN_TOKENS_PER_PAGE);
            if (tokenPageIndex >= apProc->Token.Locked.mPageCount)
            {
                stat = K2STAT_ERROR_BAD_TOKEN;
                break;
            }

            pTokenPage = apProc->Token.Locked.mppPages[tokenPageIndex];
            if (pTokenPage == NULL)
            {
                stat = K2STAT_ERROR_BAD_TOKEN;
                break;
            }

            pToken = &pTokenPage->Tokens[(tokValue / 2) % K2OSKERN_TOKENS_PER_PAGE];

            if (pToken->InUse.mTokValue != (UINT32)aToken)
            {
                stat = K2STAT_ERROR_BAD_TOKEN;
                break;
            }

            KernObj_CreateRef(apRetRef, pToken->InUse.Ref.AsAny);

        } while (0);
    }

    K2OSKERN_SeqUnlock(&apProc->Token.SeqLock, disp);

    return stat;
}

K2STAT
KernProc_TokenLocked_Destroy(
    K2OSKERN_OBJ_PROCESS *  apProc,
    K2OS_TOKEN              aToken,
    BOOL *                  apRetHitZero,
    K2OSKERN_OBJREF *       apRetObjRefIfHitZero
)
{
    K2STAT                  stat;
    UINT32                  tokValue;
    UINT32                  tokenPageIndex;
    K2OSKERN_TOKEN_PAGE *   pTokenPage;
    K2OSKERN_TOKEN *        pToken;

    stat = K2STAT_NO_ERROR;

    if (NULL != apRetHitZero)
    {
        *apRetHitZero = FALSE;
    }

    do
    {
        tokValue = (UINT32)aToken;
        if ((0 == (tokValue & K2OSKERN_TOKEN_VALID_BIT)) ||
            (0 != (tokValue & K2OSKERN_TOKEN_KERNEL_BIT)))
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
            break;
        }
        tokValue &= ~(K2OSKERN_TOKEN_SALT_MASK | K2OSKERN_TOKEN_VALID_BIT);

        tokenPageIndex = tokValue / (2 * K2OSKERN_TOKENS_PER_PAGE);
        if (tokenPageIndex >= apProc->Token.Locked.mPageCount)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
            break;
        }

        pTokenPage = apProc->Token.Locked.mppPages[tokenPageIndex];
        if (pTokenPage == NULL)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
            break;
        }

        pToken = &pTokenPage->Tokens[(tokValue / 2) % K2OSKERN_TOKENS_PER_PAGE];
        if (pToken->InUse.mTokValue != (UINT32)aToken)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
            break;
        }

        //
        // this is the right token. release the reference for the linked object
        //
//        K2OSKERN_Debug("Proc %d Delete token %08X\n", apProc->mId, aToken);
        K2_ASSERT(0 < pToken->InUse.Ref.AsAny->mTokenCount);
        if (0 == K2ATOMIC_Dec((INT32 volatile *)&pToken->InUse.Ref.AsAny->mTokenCount))
        {
            if (NULL != apRetHitZero)
            {
                *apRetHitZero = TRUE;
                if (NULL != apRetObjRefIfHitZero)
                {
                    KernObj_CreateRef(apRetObjRefIfHitZero, pToken->InUse.Ref.AsAny);
                }
            }
        }

        KernObj_ReleaseRef(&pToken->InUse.Ref);

        //
        // this will destroy the token value
        //
        pToken->InUse.mTokValue = 0;
        K2LIST_AddAtTail(&apProc->Token.Locked.FreeList, &pToken->FreeListLink);
        apProc->Token.Locked.mCount--;

        pTokenPage->Hdr.mInUseCount--;

    } while (0);

    return stat;
}

K2STAT  
KernProc_TokenDestroyInSysCall(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    K2OS_TOKEN                  aToken
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2STAT                  stat;
    UINT32                  disp;
    BOOL                    hitZero;
    K2OSKERN_OBJREF         objRef;
    K2OSKERN_SCHED_ITEM *   pSchedItem;
    BOOL                    doSchedCall;

    K2_ASSERT(aToken != NULL);

    pProc = apCurThread->User.ProcRef.AsProc;
    objRef.AsAny = NULL;
    hitZero = FALSE;

    disp = K2OSKERN_SeqLock(&pProc->Token.SeqLock);

    stat = KernProc_TokenLocked_Destroy(pProc, aToken, &hitZero, &objRef);

    K2OSKERN_SeqUnlock(&pProc->Token.SeqLock, disp);

    if (hitZero)
    {
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        K2_ASSERT(NULL != objRef.AsAny);

        doSchedCall = FALSE;

        switch (objRef.AsAny->mObjType)
        {
        case KernObj_Notify:
        case KernObj_Gate:
        case KernObj_MailboxOwner:
            // if this something somebody is waiting on, that wait needs to be cancelled
            // convert into scheduler call
            doSchedCall = TRUE;
            break;

        case KernObj_IfInst:
            // if this is a public interface it needs to be removed via the scheduler
            // so that notifications can be sent to those subscribed for such an event
            disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
            if (objRef.AsIfInst->mIsPublic)
            {
                objRef.AsIfInst->mIsDeparting = TRUE;
                doSchedCall = TRUE;
            }
            K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);
            break;

        default:
            break;
        }

        if (doSchedCall)
        {
            pSchedItem = &apCurThread->SchedItem;
            pSchedItem->mType = KernSchedItem_Thread_SysCall;
            KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
            KernObj_CreateRef(&pSchedItem->ObjRef, objRef.AsAny);
            KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
            KernSched_QueueItem(pSchedItem);
        }

        KernObj_ReleaseRef(&objRef);
    }
    else
    {
        K2_ASSERT(NULL == objRef.AsAny);
    }

    return stat;
}

void    
KernProc_SysCall_GetLaunchInfo(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_THREAD_PAGE *      pThreadPage;
    UINT32                  workAddr;
    UINT32                  lockMapCount;
    K2OSKERN_OBJREF         lockedMapRefs[2];
    UINT32                  map0FirstPageIx;
    K2OS_PROC_START_INFO *  pUserLaunchInfo;
    UINT32                  ix;

    pProc = apCurThread->User.ProcRef.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    if (apCurThread->User.mSysCall_Arg0 != pProc->mId)
    {
        stat = K2STAT_ERROR_NOT_ALLOWED;
    }
    else
    {
        workAddr = pThreadPage->mSysCall_Arg1;
        K2MEM_Zero(lockedMapRefs, sizeof(K2OSKERN_OBJREF) * 2);

        stat = KernProc_AcqMaps(
            pProc, 
            pThreadPage->mSysCall_Arg1, 
            sizeof(K2OS_PROC_START_INFO), 
            TRUE, 
            &lockMapCount, 
            lockedMapRefs, 
            &map0FirstPageIx
        );

        if (!K2STAT_IS_ERROR(stat))
        {
            pUserLaunchInfo = (K2OS_PROC_START_INFO *)workAddr;
            if (1 == pProc->mId)
            {
                //
                // hard coded that proc 1 is SysProct
                //
                K2ASC_Copy(pUserLaunchInfo->mPath, "sysproc.xdl");
                pUserLaunchInfo->mArgStr[0] = 0;
            }
            else
            {
                //
                // copy in launch info from process that started this process
                //
                K2_ASSERT(NULL != pProc->mpLaunchInfo);
                K2MEM_Copy(pUserLaunchInfo, pProc->mpLaunchInfo, sizeof(K2OS_PROC_START_INFO));
            }

            //
            // release maps for target range
            //
            for (ix = 0; ix < lockMapCount; ix++)
            {
                KernObj_ReleaseRef(&lockedMapRefs[ix]);
            }
    
            apCurThread->User.mSysCall_Result = (UINT32)TRUE;
        }
//        else
//        {
//            K2OSKERN_Debug("GetLaunchInfo - AcqMaps failed\n");
//        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

K2STAT  
KernProc_AcqMaps(
    K2OSKERN_OBJ_PROCESS *  apProc,
    UINT32                  aProcVirtAddr,
    UINT32                  aDataSize,
    BOOL                    aWriteable,
    UINT32 *                apRetMapCount,
    K2OSKERN_OBJREF *       apRetMapRefs,
    UINT32 *                apRetMap0StartPage
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_VIRTMAP *  pMap;
    UINT32                  pageIx;
    UINT32                  pagesLeft;
    UINT32                  mapCount;

    if ((aProcVirtAddr >= (K2OS_UVA_TIMER_IOPAGE_BASE - aDataSize)) ||
        (aProcVirtAddr < K2OS_UVA_LOW_BASE))
    {
        return K2STAT_ERROR_OUT_OF_BOUNDS;
    }

    stat = KernProc_FindMapAndCreateRef(apProc, aProcVirtAddr, apRetMapRefs, &pageIx);
    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    pMap = apRetMapRefs->AsVirtMap;

    if ((pMap->mMapType != K2OS_MapType_Data_ReadWrite) &&
        (pMap->mMapType != K2OS_MapType_Data_CopyOnWrite) &&
        (pMap->mMapType != K2OS_MapType_Thread_Stack))
    {
        if (aWriteable)
        {
            KernObj_ReleaseRef(apRetMapRefs);
            return K2STAT_ERROR_ADDR_NOT_ACCESSIBLE;
        }
    }

    //
    // convert data size to # of pages
    //
    aDataSize = 
        ((aProcVirtAddr & K2_VA_MEMPAGE_OFFSET_MASK) +
         (aDataSize + (K2_VA_MEMPAGE_BYTES - 1)))
        / K2_VA_MEMPAGE_BYTES;

    pagesLeft = pMap->mPageCount - pageIx;

    *apRetMapCount = 1;
    ++apRetMapRefs;
    *apRetMap0StartPage = pageIx;

    if (aDataSize <= pagesLeft)
        return K2STAT_NO_ERROR;

    //
    // infrequent case where data spans more than one map
    //
    mapCount = 1;

    aProcVirtAddr += pagesLeft * K2_VA_MEMPAGE_BYTES;
    aDataSize -= pagesLeft;

    stat = K2STAT_NO_ERROR;
    do
    {
        stat = KernProc_FindMapAndCreateRef(apProc, aProcVirtAddr, apRetMapRefs, &pageIx);
        if (K2STAT_IS_ERROR(stat))
            break;

        K2_ASSERT(0 == pageIx);

        ++apRetMapRefs;
        mapCount++;

        pMap = apRetMapRefs->AsVirtMap;

        if ((pMap->mMapType != K2OS_MapType_Data_ReadWrite) &&
            (pMap->mMapType != K2OS_MapType_Data_CopyOnWrite) &&
            (pMap->mMapType != K2OS_MapType_Thread_Stack))
        {
            if (aWriteable)
            {
                stat = K2STAT_ERROR_ADDR_NOT_ACCESSIBLE;
                break;
            }
        }

        pagesLeft = pMap->mPageCount - pageIx;

        if (aDataSize <= pagesLeft)
            break;

        aProcVirtAddr += pagesLeft * K2_VA_MEMPAGE_BYTES;
        aDataSize -= pagesLeft;

    } while (1);
                
    if (K2STAT_IS_ERROR(stat))
    {
        //
        // walk back acquires. always at least one to release
        //
        K2_ASSERT(mapCount > 0);
        do
        {
            --mapCount;
            --apRetMapRefs;
            KernObj_ReleaseRef(apRetMapRefs);
        } while (mapCount > 0);
    }
    else
    {
        *apRetMapCount = mapCount;
    }

    return stat;
}

void    
KernProc_SysCall_VirtReserve(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    UINT32                  pageCount;
    K2OSKERN_PROCHEAP_NODE *pProcHeapNode;

    pageCount = apCurThread->User.mSysCall_Arg0;

    if (0 == pageCount)
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        stat = KernProc_UserVirtHeapAlloc(
            apCurThread->User.ProcRef.AsProc,
            pageCount,
            &pProcHeapNode);
        if (!K2STAT_IS_ERROR(stat))
        {
            pProcHeapNode->mUserOwned = 1;
            apCurThread->User.mSysCall_Result = K2HEAP_NodeAddr(&pProcHeapNode->HeapNode);
        }
        else
        {
            apCurThread->User.mSysCall_Result = 0;
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
    else
    {
        K2_ASSERT(0 != apCurThread->User.mSysCall_Result);
    }
}

void    
KernProc_SysCall_VirtGet(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    BOOL                    disp;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2HEAP_NODE *           pHeapNode;

    pProc = apCurThread->User.ProcRef.AsProc;

    disp = K2OSKERN_SeqLock(&pProc->Virt.SeqLock);

    pHeapNode = K2HEAP_FindNodeContainingAddr(&pProc->Virt.Locked.HeapAnchor, apCurThread->User.mSysCall_Arg0);
    if (NULL != pHeapNode)
    {
        apCurThread->User.mSysCall_Result = pHeapNode->AddrTreeNode.mUserVal;
        apCurThread->mpKernRwViewOfThreadPage->mSysCall_Arg7_Result0 = pHeapNode->SizeTreeNode.mUserVal / K2_VA_MEMPAGE_BYTES;
    }
    else
    {
        apCurThread->User.mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = K2STAT_ERROR_NOT_FOUND;
    }

    K2OSKERN_SeqUnlock(&pProc->Virt.SeqLock, disp);
}

void    
KernProc_SysCall_VirtRelease(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    BOOL                    disp;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2STAT                  stat;
    K2HEAP_NODE *           pHeapNode;
    K2OSKERN_PROCHEAP_NODE *pProcHeapNode;

    pProc = apCurThread->User.ProcRef.AsProc;

    disp = K2OSKERN_SeqLock(&pProc->Virt.SeqLock);

    pHeapNode = K2HEAP_FindNodeContainingAddr(&pProc->Virt.Locked.HeapAnchor, apCurThread->User.mSysCall_Arg0);
    if (NULL != pHeapNode)
    {
        pProcHeapNode = K2_GET_CONTAINER(K2OSKERN_PROCHEAP_NODE, pHeapNode, HeapNode);
        if (0 != pProcHeapNode->mMapCount)
        {
            stat = K2STAT_ERROR_IN_USE;
        }
        else
        {
            //
            // need to mark as unusable so nobody tries to map to it between here
            // and when the node is freed below
            //
            pProcHeapNode->mMapCount = 0xFFFF;
            stat = K2STAT_NO_ERROR;
        }
    }
    else
        stat = K2STAT_ERROR_NOT_FOUND;

    K2OSKERN_SeqUnlock(&pProc->Virt.SeqLock, disp);

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
    else
    {
        stat = KernProc_UserVirtHeapFree(pProc, apCurThread->User.mSysCall_Arg0);
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        apCurThread->User.mSysCall_Result = (UINT32)TRUE;
    }
}

void    
KernProc_SysCall_TokenDestroy(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT          stat;

    //
    // this may devolve into a scheduler call
    // where the thread is taken off the core
    // but that doesn't affect the syscall result
    //
    stat = KernProc_TokenDestroyInSysCall(apThisCore, apCurThread, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0);

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
        apCurThread->User.mSysCall_Result = FALSE;
    }
    else
    {
        apCurThread->User.mSysCall_Result = TRUE;
    }
}

void    
KernProc_SysCall_TokenClone(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF objRef;
    K2STAT          stat;

    objRef.AsAny = NULL;

    stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &objRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        switch (objRef.AsAny->mObjType)
        {
        case KernObj_VirtMap:
            stat = K2STAT_ERROR_NOT_ALLOWED;
            break;
        default:
            stat = KernProc_TokenCreate(apCurThread->User.ProcRef.AsProc, objRef.AsAny, (K2OS_TOKEN *)&apCurThread->User.mSysCall_Result);
            break;
        }

        KernObj_ReleaseRef(&objRef);
    }
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
        apCurThread->User.mSysCall_Result = 0;
    }
}

void    
KernProc_SysCall_TrapMount(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT              stat;
    K2_EXCEPTION_TRAP * pTrap;
    UINT32              mapCount;
    K2OSKERN_OBJREF     mapRef[2];
    UINT32              map0PageIx;

    pTrap = (K2_EXCEPTION_TRAP *)apCurThread->User.mSysCall_Arg0;

    mapRef[0].AsAny = mapRef[1].AsAny = NULL;

    stat = KernProc_AcqMaps(apCurThread->User.ProcRef.AsProc,
        (UINT32)pTrap,
        sizeof(K2_EXCEPTION_TRAP),
        TRUE,
        &mapCount,
        mapRef,
        &map0PageIx);

    if (!K2STAT_IS_ERROR(stat))
    {
        pTrap->mpNextTrap = apCurThread->mpTrapStack;
        apCurThread->mpTrapStack = pTrap;
        //
        // successful mount returns FALSE.  if the trap fires it returns TRUE (altered context)
        //
        apCurThread->User.mSysCall_Result = FALSE;

        KernObj_ReleaseRef(&mapRef[0]);
        if (mapCount > 1)
            KernObj_ReleaseRef(&mapRef[1]);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        //
        // unsuccessful mount returns TRUE.  trap result already set by CRT before this syscall
        // since we can't touch the trap because we couldn't acquire the maps
        //
        apCurThread->User.mSysCall_Result = TRUE;
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
}

void    
KernProc_SysCall_TrapDismount(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT              stat;
    K2_EXCEPTION_TRAP * pTrap;
    UINT32              mapCount;
    K2OSKERN_OBJREF     mapRef[2];
    UINT32              map0PageIx;

    pTrap = (K2_EXCEPTION_TRAP *)apCurThread->User.mSysCall_Arg0;

    if (pTrap != apCurThread->mpTrapStack)
    {
        //
        // convert to raiseException
        //
        K2OSKERN_Debug("*** Thread %d INVALID TrapDismount (%08X)\n", apCurThread->mGlobalIx, pTrap);
        apCurThread->User.mSysCall_Arg0 = K2STAT_EX_LOGIC;
        KernThread_SysCall_RaiseException(apThisCore, apCurThread);
        return;
    }

    mapRef[0].AsAny = mapRef[1].AsAny = NULL;

    stat = KernProc_AcqMaps(apCurThread->User.ProcRef.AsProc,
        (UINT32)pTrap,
        sizeof(K2_EXCEPTION_TRAP),
        TRUE,
        &mapCount,
        mapRef,
        &map0PageIx);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("*** Thread %d Trap Maps could not be acquired at dismount (%08X)\n", apCurThread->mGlobalIx, pTrap);
        apCurThread->User.mSysCall_Arg0 = K2STAT_EX_LOGIC;
        KernThread_SysCall_RaiseException(apThisCore, apCurThread);
        return;
    }

    apCurThread->mpTrapStack = pTrap->mpNextTrap;

    //
    // double release the acquired maps
    //
    KernObj_ReleaseRef(&mapRef[0]);
    if (mapCount > 1)
        KernObj_ReleaseRef(&mapRef[1]);

    apCurThread->User.mSysCall_Result = 0;
}

void    
KernProc_SysCall_Exit(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_SCHED_ITEM *   pSchedItem;

    pSchedItem = &apCurThread->SchedItem;
    pSchedItem->mType = KernSchedItem_Thread_SysCall;
    KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
    KernSched_QueueItem(pSchedItem);
}

void    
KernProc_SysCall_AtStart(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    UINT32                  pageIx;
    UINT32                  mapAddr;

    pProc = apCurThread->User.ProcRef.AsProc;

    if ((apCurThread->User.StackMapRef.AsAny != NULL) ||
        (pProc->Thread.Locked.CreateList.mNodeCount != 0) ||
        (pProc->Thread.SchedLocked.ActiveList.mNodeCount != 1) ||
        (pProc->Thread.Locked.DeadList.mNodeCount != 0) ||
        (pProc->mState != KernProcState_Starting))
    {
        //
        // catastrophic error. exit process
        //
        apCurThread->User.mSysCall_Arg0 = K2STAT_ERROR_CORRUPTED;
        apCurThread->User.mSysCall_Id = K2OS_SYSCALL_ID_PROCESS_EXIT;
        KernProc_SysCall_Exit(apThisCore, apCurThread);
        return;
    }

    mapAddr = apCurThread->User.mSysCall_Arg0;

    stat = KernProc_FindMapAndCreateRef(pProc, mapAddr, &apCurThread->User.StackMapRef, &pageIx);
    if ((K2STAT_IS_ERROR(stat)) ||
        (pageIx != 0) ||
        (apCurThread->User.StackMapRef.AsVirtMap->User.mpProcHeapNode->HeapNode.AddrTreeNode.mUserVal != (mapAddr - K2_VA_MEMPAGE_BYTES)) ||
        (apCurThread->User.StackMapRef.AsVirtMap->mMapType != K2OS_MapType_Thread_Stack))
    {
        //
        // catastrophic error. exit process
        //
        apCurThread->User.mSysCall_Arg0 = K2STAT_ERROR_CORRUPTED;
        apCurThread->User.mSysCall_Id = K2OS_SYSCALL_ID_PROCESS_EXIT;
        KernProc_SysCall_Exit(apThisCore, apCurThread);
        return;
    }

    //
    // kernel takes ownership of thread stack and the virtual alloc for it
    //
    apCurThread->User.StackMapRef.AsVirtMap->User.mpProcHeapNode->mUserOwned = FALSE;
    //
    // crt will release the map, leaving only the kernel reference to the map
    //

    if (K2OS_SYSPROC_ID == pProc->mId)
    {
        // set up notify message queue for sysproc

    }

#if K2OSKERN_TRACE_THREAD_LIFE
    K2OSKERN_Debug("-- Proc %d Started\n", pProc->mId);
#endif
    pProc->mState = KernProcState_Running;

    if (K2OS_SYSPROC_ID != pProc->mId)
    {
        apCurThread->User.mSysCall_Result = TRUE;
    }
}

void
KernProc_Stop_CheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;

    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apKey, Hdr.ObjDpc.Func);

    KTRACE(apThisCore, 2, KTRACE_PROC_STOP_CHECK_DPC, pProc->mId);

    if (0 == pProc->mStopCoresRemaining)
    {
        if (apThisCore->MappedProcRef.AsProc == pProc)
        {
            KernCpu_StopProc(apThisCore, pProc);
        }
        pProc->StoppedSchedItem.mType = KernSchedItem_ProcStopped;
        KernArch_GetHfTimerTick(&pProc->StoppedSchedItem.mHfTick);
        KernSched_QueueItem(&pProc->StoppedSchedItem);
    }
    else
    {
        pProc->Hdr.ObjDpc.Func = KernProc_Stop_CheckComplete;
        KernCpu_QueueDpc(&pProc->Hdr.ObjDpc.Dpc, &pProc->Hdr.ObjDpc.Func, KernDpcPrio_Med);
    }
}

void
KernProc_Stop_SendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    UINT32                  sentMask;

    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pProc->mIciSendMask);

    KTRACE(apThisCore, 2, KTRACE_PROC_STOP_SENDICI_DPC, pProc->mId);

    sentMask = KernArch_SendIci(
        apThisCore,
        pProc->mIciSendMask,
        KernIci_StopProc,
        pProc
    );

    pProc->mIciSendMask &= ~sentMask;

    if (0 == pProc->mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        pProc->Hdr.ObjDpc.Func = KernProc_Stop_CheckComplete;
    }
    else
    {
        pProc->Hdr.ObjDpc.Func = KernProc_Stop_SendIci;
    }
    KernCpu_QueueDpc(&pProc->Hdr.ObjDpc.Dpc, &pProc->Hdr.ObjDpc.Func, KernDpcPrio_Med);
}

void
KernProc_StopDpc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apArg
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;

    //
    // queued by scheduler when process state transitions to stopping
    //
    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apArg, Hdr.ObjDpc.Func);
    K2_ASSERT(pProc->Hdr.mObjType == KernObj_Process);
    K2_ASSERT(pProc->mState == KernProcState_Stopping);

    KTRACE(apThisCore, 2, KTRACE_PROC_STOP_DPC, pProc->mId);

    //
    // send the process stop ici to all other cores
    //
    if (gData.mCpuCoreCount > 1)
    {
        pProc->mStopCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
        pProc->mIciSendMask = pProc->mStopCoresRemaining;
        KernProc_Stop_SendIci(apThisCore, &pProc->Hdr.ObjDpc.Func);
    }
    else
    {
        if (apThisCore->MappedProcRef.AsProc == pProc)
        {
            KernCpu_StopProc(apThisCore, pProc);
        }
        pProc->StoppedSchedItem.mType = KernSchedItem_ProcStopped;
        KernArch_GetHfTimerTick(&pProc->StoppedSchedItem.mHfTick);
        KernSched_QueueItem(&pProc->StoppedSchedItem);
    }
}

void    
KernProc_ExitedDpc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apArg
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    UINT32                  ixSeg;

    //
    // queued by scheduler when process state transitions to stopped
    //
    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apArg, Hdr.ObjDpc.Func);
    K2_ASSERT(pProc->Hdr.mObjType == KernObj_Process);
    K2_ASSERT(pProc->mState == KernProcState_Stopped);

    KTRACE(apThisCore, 2, KTRACE_PROC_EXITED_DPC, pProc->mId);

    //
    // all tokens should have been destroyed in the scheduler when 
    // the process' state changed to stopping
    //
    K2_ASSERT(0 == pProc->Token.Locked.mCount);

    //
    // release reference to xdl tracking map
    //
    KernObj_ReleaseRef(&pProc->XdlTrackMapRef);

    //
    // release references to default maps
    // 
    KernObj_ReleaseRef(&pProc->PublicApiMapRef);
    KernObj_ReleaseRef(&pProc->TimerPageMapRef);
    KernObj_ReleaseRef(&pProc->FileSysMapRef);
    for (ixSeg = 0; ixSeg < KernUserCrtSegType_Count; ixSeg++)
    {
        KernObj_ReleaseRef(&pProc->CrtMapRef[ixSeg]);
    }

    if (pProc->mpLaunchInfo != NULL)
    {
        KernHeap_Free(pProc->mpLaunchInfo);
        pProc->mpLaunchInfo = NULL;
    }

    //
    // this ref was taken when the process entered stopping state
    //
    KernObj_ReleaseRef(&pProc->StoppingRef);
}

void
KernProc_MergePageList(
    K2LIST_ANCHOR *     apTarget,
    K2LIST_ANCHOR *     apSrc,
    KernPhysPageList    aVerifySrc
)
{
    K2OSKERN_PHYSTRACK *    pTrack;
    K2LIST_LINK *           pListLink;

    pListLink = apSrc->mpHead;
    while (NULL != pListLink)
    {
        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, pListLink, ListLink);
        pListLink = pListLink->mpNext;
        K2LIST_Remove(apSrc, &pTrack->ListLink);
        K2_ASSERT(pTrack->Flags.Field.PageListIx == aVerifySrc);
        pTrack->Flags.Field.PageListIx = KernPhysPageList_None;
        K2LIST_AddAtTail(apTarget, &pTrack->ListLink);
    }
}

void
KernProc_Clean_TransBaseDone(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_PROCESS *      apProc
)
{
    K2LIST_ANCHOR   collectList;
    UINT32          procId;
    K2OS_MSG        sysProcMsg;
    BOOL            doSignal;

    KernVirt_Release(apProc->mVirtTransBase);
    apProc->mVirtTransBase = 0;

    K2LIST_Init(&collectList);

    //
    // free pages in process pagelists  
    // 
    KernProc_MergePageList(&collectList, &apProc->PageList.Locked.Reserved_Dirty, KernPhysPageList_Proc_Res_Dirty);
    KernProc_MergePageList(&collectList, &apProc->PageList.Locked.Reserved_Clean, KernPhysPageList_Proc_Res_Clean);
    KernProc_MergePageList(&collectList, &apProc->PageList.Locked.PageTables, KernPhysPageList_Proc_PageTables);
    KernProc_MergePageList(&collectList, &apProc->PageList.Locked.ThreadTls, KernPhysPageList_Proc_ThreadTls);
    KernProc_MergePageList(&collectList, &apProc->PageList.Locked.Working, KernPhysPageList_Proc_Working);
    KernProc_MergePageList(&collectList, &apProc->PageList.Locked.Tokens, KernPhysPageList_Proc_Token);
    KernPhys_FreeTrackList(&collectList);

    KernPhys_FreeTrack(apProc->Virt.mpPageDirTrack);

    //
    // purge the memory used for the process structure
    //
    procId = apProc->mId;
    K2MEM_Zero(apProc, sizeof(K2OSKERN_OBJ_PROCESS));
    KernHeap_Free(apProc);

//    K2OSKERN_Debug("Process %d purged\n", procId);
    sysProcMsg.mType = K2OS_SYSTEM_MSGTYPE_SYSPROC;
    sysProcMsg.mShort = K2OS_SYSTEM_MSG_SYSPROC_SHORT_PROCESS_PURGED;
    sysProcMsg.mPayload[0] = procId;
    doSignal = FALSE;
    KernSysProc_Notify(FALSE, &sysProcMsg, &doSignal);
    if (doSignal)
    {
        if (!KernNotifyProxy_Fire(gData.SysProc.RefNotify.AsNotify))
        {
            K2OSKERN_Debug("*** Process %d purged but could not notify sysproc\n", procId);
        }
    }

    //
    // all done. should be nothing left from the process in the system.
    //
#if 1
    if (K2OS_SYSPROC_ID == procId)
    {
        K2OSKERN_Panic("SysProc exited\n");
    }
#else
    if (0 == K2ATOMIC_Dec(&gData.Proc.mExistCount))
    {
        K2OSKERN_Panic("All processes exited\n");
    }
#endif
}

void
KernProc_Clean_TransBaseCheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;

    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apKey, Hdr.ObjDpc.Func);

    KTRACE(apThisCore, 2, KTRACE_PROC_TRANS_CHECK_DPC, pProc->mId);

    if (0 == pProc->ProcStoppedTlbShoot.mCoresRemaining)
    {
        KernProc_Clean_TransBaseDone(apThisCore, pProc);
    }
    else
    {
        pProc->Hdr.ObjDpc.Func = KernProc_Clean_TransBaseCheckComplete;
        KernCpu_QueueDpc(&pProc->Hdr.ObjDpc.Dpc, &pProc->Hdr.ObjDpc.Func, KernDpcPrio_Med);
    }
}

void
KernProc_Clean_TransBaseSendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;;
    UINT32                  sentMask;

    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pProc->mIciSendMask);
    
    KTRACE(apThisCore, 2, KTRACE_PROC_TRANS_SENDICI_DPC, pProc->mId);

    sentMask = KernArch_SendIci(
        apThisCore,
        pProc->mIciSendMask,
        KernIci_TlbInv,
        &pProc->ProcStoppedTlbShoot
    );

    pProc->mIciSendMask &= ~sentMask;

    if (0 == pProc->mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        pProc->Hdr.ObjDpc.Func = KernProc_Clean_TransBaseCheckComplete;
    }
    else
    {
        pProc->Hdr.ObjDpc.Func = KernProc_Clean_TransBaseSendIci;
    }
    KernCpu_QueueDpc(&pProc->Hdr.ObjDpc.Dpc, &pProc->Hdr.ObjDpc.Func, KernDpcPrio_Med);
}

void
KernProc_Clean_HighVirtDone(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_PROCESS *      apProc
)
{
    UINT32      pageCount;
    UINT32      workVirt;
    UINT32      pagesLeft;
 
    apProc->Virt.Locked.mTopPt = K2OS_KVA_KERN_BASE;

    KernVirt_Release(apProc->mVirtMapKVA);
    apProc->mVirtMapKVA = 0;

    //
    // break map of the translation base page(s) (apProc->mVirtTransBase);
    //
    pageCount = K2_VA32_PROC_TRANSTAB_SIZE / K2_VA_MEMPAGE_BYTES;
    
    workVirt = apProc->mVirtTransBase;
    pagesLeft = pageCount;
    do
    {
        KernPte_BreakPageMap(NULL, workVirt, 0);
        workVirt += K2_VA_MEMPAGE_BYTES;
    } while (--pagesLeft);

    if (gData.mCpuCoreCount > 1)
    {
        apProc->ProcStoppedTlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
        apProc->ProcStoppedTlbShoot.mpProc = NULL;
        apProc->ProcStoppedTlbShoot.mVirtBase = apProc->mVirtTransBase;
        apProc->ProcStoppedTlbShoot.mPageCount = pageCount;
        apProc->mIciSendMask = apProc->ProcStoppedTlbShoot.mCoresRemaining;
        KernProc_Clean_TransBaseSendIci(apThisCore, &apProc->Hdr.ObjDpc.Func);
    }

    workVirt = apProc->mVirtTransBase;
    pagesLeft = pageCount;
    do
    {
        KernArch_InvalidateTlbPageOnCurrentCore(workVirt);
        workVirt += K2_VA_MEMPAGE_BYTES;
    } while (--pagesLeft);

    if (gData.mCpuCoreCount == 1)
    {
        KernProc_Clean_TransBaseDone(apThisCore, apProc);
    }
}

void
KernProc_Clean_HighVirtCheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;

    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apKey, Hdr.ObjDpc.Func);

    KTRACE(apThisCore, 2, KTRACE_PROC_HIVIRT_CHECK_DPC, pProc->mId);

    if (0 == pProc->ProcStoppedTlbShoot.mCoresRemaining)
    {
        KernProc_Clean_HighVirtDone(apThisCore, pProc);
    }
    else
    {
        pProc->Hdr.ObjDpc.Func = KernProc_Clean_HighVirtCheckComplete;
        KernCpu_QueueDpc(&pProc->Hdr.ObjDpc.Dpc, &pProc->Hdr.ObjDpc.Func, KernDpcPrio_Med);
    }
}

void
KernProc_Clean_HighVirtSendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;;
    UINT32                  sentMask;

    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pProc->mIciSendMask);

    KTRACE(apThisCore, 2, KTRACE_PROC_HIVIRT_SENDICI_DPC, pProc->mId);

    sentMask = KernArch_SendIci(
        apThisCore,
        pProc->mIciSendMask,
        KernIci_TlbInv,
        &pProc->ProcStoppedTlbShoot
    );

    pProc->mIciSendMask &= ~sentMask;

    if (0 == pProc->mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        pProc->Hdr.ObjDpc.Func = KernProc_Clean_HighVirtCheckComplete;
    }
    else
    {
        pProc->Hdr.ObjDpc.Func = KernProc_Clean_HighVirtSendIci;
    }
    KernCpu_QueueDpc(&pProc->Hdr.ObjDpc.Dpc, &pProc->Hdr.ObjDpc.Func, KernDpcPrio_Med);
}

void
KernProc_Clean_LowVirtDone(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_PROCESS *      apProc
)
{
    UINT32 virtKernPT;
    UINT32 pageCount;
    UINT32 workVirt;
    UINT32 pagesLeft;

    apProc->Virt.Locked.mBotPt = 0;

    //
    // continue to clean up process pagetables (Virt.Locked.mTopPt)
    //
    pageCount = (K2OS_KVA_KERN_BASE - apProc->Virt.Locked.mTopPt) / K2_VA32_PAGETABLE_MAP_BYTES;
    if (pageCount > 0)
    {
        virtKernPT = K2_VA32_TO_PT_ADDR(apProc->mVirtMapKVA, apProc->Virt.Locked.mTopPt);

        workVirt = virtKernPT;
        pagesLeft = pageCount;
        do
        {
            KernPte_BreakPageMap(NULL, workVirt, 0);
            workVirt += K2_VA_MEMPAGE_BYTES;
        } while (--pagesLeft);

        if (gData.mCpuCoreCount > 1)
        {
            apProc->ProcStoppedTlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
            apProc->ProcStoppedTlbShoot.mpProc = NULL;
            apProc->ProcStoppedTlbShoot.mVirtBase = virtKernPT;
            apProc->ProcStoppedTlbShoot.mPageCount = pageCount;
            apProc->mIciSendMask = apProc->ProcStoppedTlbShoot.mCoresRemaining;
            KernProc_Clean_HighVirtSendIci(apThisCore, &apProc->Hdr.ObjDpc.Func);
        }

        workVirt = virtKernPT;
        pagesLeft = pageCount;
        do
        {
            KernArch_InvalidateTlbPageOnCurrentCore(workVirt);
            workVirt += K2_VA_MEMPAGE_BYTES;
        } while (--pagesLeft);

        if (gData.mCpuCoreCount == 1)
        {
            KernProc_Clean_HighVirtDone(apThisCore, apProc);
        }
    }
    else
    {
        KernProc_Clean_HighVirtDone(apThisCore, apProc);
    }
}

void
KernProc_Clean_LowVirtCheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;

    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apKey, Hdr.ObjDpc.Func);

    KTRACE(apThisCore, 2, KTRACE_PROC_LOVIRT_CHECK_DPC, pProc->mId);

    if (0 == pProc->ProcStoppedTlbShoot.mCoresRemaining)
    {
        KernProc_Clean_LowVirtDone(apThisCore, pProc);
    }
    else
    {
        pProc->Hdr.ObjDpc.Func = KernProc_Clean_LowVirtCheckComplete;
        KernCpu_QueueDpc(&pProc->Hdr.ObjDpc.Dpc, &pProc->Hdr.ObjDpc.Func, KernDpcPrio_Med);
    }
}

void
KernProc_Clean_LowVirtSendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;;
    UINT32                  sentMask;

    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pProc->mIciSendMask);
    
    KTRACE(apThisCore, 2, KTRACE_PROC_LOVIRT_SENDICI_DPC, pProc->mId);

    sentMask = KernArch_SendIci(
        apThisCore,
        pProc->mIciSendMask,
        KernIci_TlbInv,
        &pProc->ProcStoppedTlbShoot
    );

    pProc->mIciSendMask &= ~sentMask;

    if (0 == pProc->mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        pProc->Hdr.ObjDpc.Func = KernProc_Clean_LowVirtCheckComplete;
    }
    else
    {
        pProc->Hdr.ObjDpc.Func = KernProc_Clean_LowVirtSendIci;
    }
    KernCpu_QueueDpc(&pProc->Hdr.ObjDpc.Dpc, &pProc->Hdr.ObjDpc.Func, KernDpcPrio_Med);
}

void
KernProc_Clean_TokenDone(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_PROCESS *      apProc
)
{
    UINT32 virtKernPT;
    UINT32 pageCount;
    UINT32 workVirt;
    UINT32 pagesLeft;

    if (NULL != apProc->Token.Locked.mppPages)
    {
        KernHeap_Free(apProc->Token.Locked.mppPages);
        K2MEM_Zero(&apProc->Token, sizeof(K2OSKERN_PROCTOKEN));
    }

    //
    // start to clean up process pagetables (Virt.Locked.mBotPt)
    //
    pageCount = apProc->Virt.Locked.mBotPt / K2_VA32_PAGETABLE_MAP_BYTES;
    if (pageCount > 0)
    {
        virtKernPT = K2_VA32_TO_PT_ADDR(apProc->mVirtMapKVA, 0);
        workVirt = virtKernPT;
        pagesLeft = pageCount;
        do
        {
            KernPte_BreakPageMap(NULL, workVirt, 0);
            workVirt += K2_VA_MEMPAGE_BYTES;
        } while (--pagesLeft);

        if (gData.mCpuCoreCount > 1)
        {
            apProc->ProcStoppedTlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
            apProc->ProcStoppedTlbShoot.mpProc = NULL;
            apProc->ProcStoppedTlbShoot.mVirtBase = virtKernPT;
            apProc->ProcStoppedTlbShoot.mPageCount = pageCount;
            apProc->mIciSendMask = apProc->ProcStoppedTlbShoot.mCoresRemaining;
            KernProc_Clean_LowVirtSendIci(apThisCore, &apProc->Hdr.ObjDpc.Func);
        }

        workVirt = virtKernPT;
        pagesLeft = pageCount;
        do
        {
            KernArch_InvalidateTlbPageOnCurrentCore(workVirt);
            workVirt += K2_VA_MEMPAGE_BYTES;
        } while (--pagesLeft);

        if (gData.mCpuCoreCount == 1)
        {
            KernProc_Clean_LowVirtDone(apThisCore, apProc);
        }
    }
    else
    {
        KernProc_Clean_LowVirtDone(apThisCore, apProc);
    }
}

void
KernProc_Clean_TokenSendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
);

void
KernProc_Clean_TokenCheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    UINT32                  tokenPageVirt;

    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apKey, Hdr.ObjDpc.Func);

    KTRACE(apThisCore, 2, KTRACE_PROC_TOKEN_CHECK_DPC, pProc->mId);

    if (0 == pProc->ProcStoppedTlbShoot.mCoresRemaining)
    {
        K2_ASSERT(pProc->Token.Locked.mPageCount > 0);

        // free the page now
        tokenPageVirt = (UINT32)pProc->Token.Locked.mppPages[pProc->Token.Locked.mPageCount - 1];
        KernVirt_Release(tokenPageVirt);

        if (--pProc->Token.Locked.mPageCount == 0)
        {
            KernProc_Clean_TokenDone(apThisCore, pProc);
        }
        else
        {
            //
            // do the next page
            //
            tokenPageVirt = (UINT32)pProc->Token.Locked.mppPages[pProc->Token.Locked.mPageCount - 1];
            KernPte_BreakPageMap(NULL, tokenPageVirt, 0);
            KernArch_InvalidateTlbPageOnCurrentCore(tokenPageVirt);

            pProc->ProcStoppedTlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
            pProc->ProcStoppedTlbShoot.mpProc = NULL;
            pProc->ProcStoppedTlbShoot.mVirtBase = tokenPageVirt;
            pProc->ProcStoppedTlbShoot.mPageCount = 1;
            pProc->mIciSendMask = pProc->ProcStoppedTlbShoot.mCoresRemaining;
            KernProc_Clean_TokenSendIci(apThisCore, &pProc->Hdr.ObjDpc.Func);
        }
    }
    else
    {
        pProc->Hdr.ObjDpc.Func = KernProc_Clean_TokenCheckComplete;
        KernCpu_QueueDpc(&pProc->Hdr.ObjDpc.Dpc, &pProc->Hdr.ObjDpc.Func, KernDpcPrio_Med);
    }
}

void
KernProc_Clean_TokenSendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;;
    UINT32                  sentMask;

    pProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pProc->mIciSendMask);
    
    KTRACE(apThisCore, 2, KTRACE_PROC_TOKEN_SENDICI_DPC, pProc->mId);

    sentMask = KernArch_SendIci(
        apThisCore,
        pProc->mIciSendMask,
        KernIci_TlbInv,
        &pProc->ProcStoppedTlbShoot
    );

    pProc->mIciSendMask &= ~sentMask;

    if (0 == pProc->mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        pProc->Hdr.ObjDpc.Func = KernProc_Clean_TokenCheckComplete;
    }
    else
    {
        pProc->Hdr.ObjDpc.Func = KernProc_Clean_TokenSendIci;
    }
    KernCpu_QueueDpc(&pProc->Hdr.ObjDpc.Dpc, &pProc->Hdr.ObjDpc.Func, KernDpcPrio_Med);
}

void    
KernProc_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_PROCESS *      apProc
)
{
    BOOL                            disp;
    UINT32                          tokenPageVirt;
    K2LIST_LINK *                   pListLink;
    K2OSKERN_PROCHEAP_NODEBLOCK *   pNodeBlock;

//    K2OSKERN_Debug("Process %d refcount zero\n", apProc->mId);

    K2_ASSERT(apProc->Hdr.RefObjList.mNodeCount == 0);
    K2_ASSERT(apProc->mState == KernProcState_Stopped);
    K2_ASSERT(apProc->mVirtMapKVA != 0);
    K2_ASSERT(apProc->Virt.Locked.mBotPt != 0);
    K2_ASSERT(apProc->Virt.Locked.mTopPt != 0);
    K2_ASSERT(apProc->Thread.Locked.CreateList.mNodeCount == 0);
    K2_ASSERT(apProc->Thread.Locked.DeadList.mNodeCount == 0);
    K2_ASSERT(apProc->Thread.SchedLocked.ActiveList.mNodeCount == 0);
    K2_ASSERT(apProc->Virt.Locked.MapTree.mNodeCount == 0);
    K2_ASSERT(apProc->XdlTrackMapRef.AsAny == NULL);
    K2_ASSERT(apProc->XdlTrackPageArrayRef.AsAny == NULL);
    K2_ASSERT(apProc->PublicApiMapRef.AsAny == NULL);
    K2_ASSERT(apProc->TimerPageMapRef.AsAny == NULL);
    K2_ASSERT(apProc->FileSysMapRef.AsAny == NULL);
    for (tokenPageVirt = 0; tokenPageVirt < KernUserCrtSegType_Count; tokenPageVirt++)
    {
        K2_ASSERT(apProc->CrtMapRef[tokenPageVirt].AsAny == NULL);
    }
    K2_ASSERT(apProc->mpLaunchInfo == NULL);

    // take proc off global process list
    disp = K2OSKERN_SeqLock(&gData.Proc.SeqLock);
    K2LIST_Remove(&gData.Proc.List, &apProc->GlobalProcListLink);
    K2OSKERN_SeqUnlock(&gData.Proc.SeqLock, disp);

    // dont need this anymore
    apProc->mpUserXdlList = NULL;

    // release tracking memory for process' user virtual space
    pListLink = apProc->Virt.Locked.BlockList.mpHead;
    if (NULL != pListLink)
    {
        do
        {
            pNodeBlock = K2_GET_CONTAINER(K2OSKERN_PROCHEAP_NODEBLOCK, pListLink, BlockListLink);
            pListLink = pListLink->mpNext;
            K2LIST_Remove(&apProc->Virt.Locked.BlockList, &pNodeBlock->BlockListLink);
            KernHeap_Free(pNodeBlock);
        } while (NULL != pListLink);
    }

    // start to clean up token pages
    if (apProc->Token.Locked.mPageCount > 0)
    {
        tokenPageVirt = (UINT32)apProc->Token.Locked.mppPages[apProc->Token.Locked.mPageCount - 1];

        KernPte_BreakPageMap(NULL, tokenPageVirt, 0);

        if (gData.mCpuCoreCount > 1)
        {
            apProc->ProcStoppedTlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
            apProc->ProcStoppedTlbShoot.mpProc = NULL;
            apProc->ProcStoppedTlbShoot.mVirtBase = tokenPageVirt;
            apProc->ProcStoppedTlbShoot.mPageCount = 1;
            apProc->mIciSendMask = apProc->ProcStoppedTlbShoot.mCoresRemaining;
            KernProc_Clean_TokenSendIci(apThisCore, &apProc->Hdr.ObjDpc.Func);
        }

        KernArch_InvalidateTlbPageOnCurrentCore(tokenPageVirt);

        if (gData.mCpuCoreCount == 1)
        {
            while (--apProc->Token.Locked.mPageCount > 0)
            {
                tokenPageVirt = (UINT32)apProc->Token.Locked.mppPages[apProc->Token.Locked.mPageCount - 1];
                KernPte_BreakPageMap(NULL, tokenPageVirt, 0);
                KernArch_InvalidateTlbPageOnCurrentCore(tokenPageVirt);
            }
            KernProc_Clean_TokenDone(apThisCore, apProc);
        }
    }
    else
    {
        KernProc_Clean_TokenDone(apThisCore, apProc);
    }
}

K2STAT  
KernProc_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OS_PROC_START_INFO *      apLaunchInfo,
    K2OSKERN_OBJREF *           apRetRef
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_OBJ_THREAD *   pThread;
    K2OS_THREAD_CONFIG      config;
    BOOL                    disp;

    stat = KernProc_CreateRawProc(&pProc);
    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    do
    {
        pThread = NULL;

        stat = KernProc_BuildProcess(pProc);
        if (K2STAT_IS_ERROR(stat))
        {
            break;
        }

        config.mStackPages = 0;
        config.mPriority = 0;
        config.mAffinityMask = (1 << gData.mCpuCoreCount) - 1;
        config.mQuantum = 30;

        stat = KernThread_Create(pProc, &config, &pThread);

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        if (NULL != pThread)
        {
            pThread->mState = KernThreadState_Exited;
            pThread->mExitCode = stat;
            disp = K2OSKERN_SeqLock(&pProc->Thread.SeqLock);
            K2LIST_Remove(&pProc->Thread.Locked.CreateList, &pThread->OwnerThreadListLink);
            K2LIST_AddAtTail(&pProc->Thread.Locked.DeadList, &pThread->OwnerThreadListLink);
            K2OSKERN_SeqUnlock(&pProc->Thread.SeqLock, disp);
        }
        pProc->mState = KernProcState_Stopped;
        if (NULL != pThread)
        {
            KernThread_Cleanup(apThisCore, pThread);
        }
        else
        {
            KernProc_Cleanup(apThisCore, pProc);
        }
        return stat;
    }

    pProc->mpLaunchInfo = apLaunchInfo;

    KernArch_UserCrtPrep(pProc, pThread);

    //
    // number of physical pages left on reserved list should match number of data pages + number of sym pages
    // for copy-on-write operations of crt data segment and crt sym segment
    //
    K2_ASSERT(pProc->PageList.Locked.Reserved_Dirty.mNodeCount == 2 + gData.User.mCrtDataPagesCount);

    pProc->mState = KernProcState_Launching;

    KernObj_CreateRef(apRetRef, &pProc->Hdr);

    return K2STAT_NO_ERROR;
}

BOOL    
KernProc_SysCall_Fast_TlsAlloc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_THREAD_PAGE * pThreadPage;
    UINT64                  oldVal;
    UINT64                  nextBit;
    UINT64                  alt;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;
    pProc = apCurThread->User.ProcRef.AsProc;
    do
    {
        oldVal = pProc->mTlsBitmap;
        K2_CpuReadBarrier();
        if (0xFFFFFFFFFFFFFFFFull == oldVal)
        {
            pThreadPage->mLastStatus = K2STAT_ERROR_OUT_OF_RESOURCES;
            return FALSE;
        }

        //
        // find first set bit in inverse
        //
        nextBit = ~oldVal;
        alt = nextBit - 1;
        alt = ((nextBit | alt) ^ alt);

    } while (oldVal != K2ATOMIC_CompareExchange64(&pProc->mTlsBitmap, oldVal | alt, oldVal));

    pThreadPage->mSysCall_Arg7_Result0 = KernBit_LowestSet_Index64(&alt);

    return TRUE;
}

BOOL    
KernProc_SysCall_Fast_TlsFree(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_THREAD_PAGE * pThreadPage;
    UINT32                slotIndex;
    UINT64                  oldVal;
    UINT64                  newVal;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;
    pProc = apCurThread->User.ProcRef.AsProc;

    slotIndex = apCurThread->User.mSysCall_Arg0;

    if (K2OS_NUM_THREAD_TLS_SLOTS <= slotIndex)
    {
        pThreadPage->mLastStatus = K2STAT_ERROR_OUT_OF_BOUNDS;
        return FALSE;
    }

    do
    {
        oldVal = pProc->mTlsBitmap;
        K2_CpuReadBarrier();
        if (0 == (oldVal & (1ull << slotIndex)))
        {
            pThreadPage->mLastStatus = K2STAT_ERROR_NOT_IN_USE;
            return FALSE;
        }
        newVal = oldVal & ~(1ull << slotIndex);
    } while (oldVal != K2ATOMIC_CompareExchange64(&pProc->mTlsBitmap, newVal, oldVal));

    return TRUE;
}

void
KernProc_SysCall_GetExitCode(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJREF         procRef;

    pProc = apCurThread->User.ProcRef.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    if (pProc->mId != 1)
    {
        stat = K2STAT_ERROR_NOT_ALLOWED;
    }
    else
    {
        procRef.AsAny = NULL;
        stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &procRef);
        if (!K2STAT_IS_ERROR(stat))
        {
            if (procRef.AsAny->mObjType != KernObj_Process)
            {
                stat = K2STAT_ERROR_BAD_TOKEN;
            }
            else
            {
                if (procRef.AsProc->mState < KernProcState_Stopping)
                {
                    stat = K2STAT_ERROR_RUNNING;
                }
                else
                {
                    apCurThread->User.mSysCall_Result = procRef.AsProc->mExitCode;
                }
            }
            KernObj_ReleaseRef(&procRef);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = stat;
        pThreadPage->mLastStatus = stat;
    }
}

void    
KernProc_SysCall_CacheOp(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    BOOL                    disp;
    K2OS_THREAD_PAGE * pThreadPage;
    UINT32                op;
    UINT32                baseAddr;
    UINT32                length;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    op = apCurThread->User.mSysCall_Arg0;
    baseAddr = pThreadPage->mSysCall_Arg1;
    length = pThreadPage->mSysCall_Arg2;

    if ((op > K2OS_CACHEOP_InvalidateInstructionRange) ||
        (baseAddr >= K2OS_KVA_KERN_BASE) ||
        (length > (K2OS_KVA_KERN_BASE - baseAddr)))
    {
        apCurThread->User.mSysCall_Result = 1;
        pThreadPage->mLastStatus = K2STAT_ERROR_BAD_ARGUMENT;
        return;
    }

    if (length > 0)
    {
        disp = K2OSKERN_SeqLock(&apCurThread->User.ProcRef.AsProc->Virt.SeqLock);
        K2OS_CacheOperation(op, baseAddr, length);
        K2OSKERN_SeqUnlock(&apCurThread->User.ProcRef.AsProc->Virt.SeqLock, disp);
    }

    apCurThread->User.mSysCall_Result = 1;
    pThreadPage->mLastStatus = K2STAT_NO_ERROR;
}
