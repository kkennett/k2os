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
#include "root.h"
#include <lib/k2heap.h>

typedef struct _RESALLOC RESALLOC;
struct _RESALLOC
{
    K2HEAP_NODE HeapNode;
    UINT_PTR    mContext;
};

K2HEAP_ANCHOR gResIoHeap;
K2HEAP_ANCHOR gResPhysHeap;

K2HEAP_NODE *
Res_Heap_AcquireNode(
    K2HEAP_ANCHOR * apHeap
)
{
    RESALLOC *   pResAlloc;
    pResAlloc = K2OS_Heap_Alloc(sizeof(RESALLOC));
    if (NULL == pResAlloc)
        return NULL;
    return &pResAlloc->HeapNode;
}

void
Res_Heap_ReleaseNode(
    K2HEAP_ANCHOR * apHeap,
    K2HEAP_NODE *   apNode
)
{
    RESALLOC *   pResAlloc;
    pResAlloc = K2_GET_CONTAINER(RESALLOC, apNode, HeapNode);
    K2OS_Heap_Free(pResAlloc);
}

BOOL
Res_IoHeap_Reserve(
    K2OS_IOADDR_INFO const *    apIoInfo,
    UINT_PTR                    aContext,
    BOOL                        aExclusive
)
{
    K2STAT          stat;
    RESALLOC *      pResAlloc;
    K2HEAP_NODE *   pHeapNode;
    UINT_PTR        base;
    UINT_PTR        count;
    UINT32          work;
    UINT32          nextNodeAddr;
    K2HEAP_NODE *   pNewHeapNode;

    Debug_Printf("IoReserve:   Base %04X Size %04X\n", apIoInfo->mPortBase, apIoInfo->mPortCount);

    stat = K2HEAP_AllocNodeAt(&gResIoHeap, apIoInfo->mPortBase, apIoInfo->mPortCount, &pNewHeapNode);
    if (!K2STAT_IS_ERROR(stat))
    {
        pResAlloc = K2_GET_CONTAINER(RESALLOC, pNewHeapNode, HeapNode);
        pResAlloc->mContext = aContext;
        return TRUE;
    }

    if (aExclusive)
    {
        Debug_Printf("***IoResource - context 0x%08X failed exclusive reserve of 0x%04X for 0x%04X\n",
            aContext,
            apIoInfo->mPortBase,
            apIoInfo->mPortCount);
        return FALSE;
    }

    //
    // nonexclusive alloc - make sure range of alloc is covered
    //
    base = apIoInfo->mPortBase;
    count = apIoInfo->mPortCount;
    pHeapNode = K2HEAP_GetFirstNodeInRange(&gResIoHeap, base, count);
    K2_ASSERT(NULL != pHeapNode);

    work = K2HEAP_NodeAddr(pHeapNode);
    nextNodeAddr = work + K2HEAP_NodeSize(pHeapNode);
    if (K2HEAP_NodeIsFree(pHeapNode))
    {
        // start of io range is inside a free space node
        K2_ASSERT((nextNodeAddr - base) < count); // or else allocat above should have been successful
        stat = K2HEAP_AllocNodeAt(&gResIoHeap, base, nextNodeAddr - base, &pNewHeapNode);
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        pResAlloc = K2_GET_CONTAINER(RESALLOC, pNewHeapNode, HeapNode);
        pResAlloc->mContext = aContext;
    }
    else
    {
        // start of io range is inside a node already in use
        if ((nextNodeAddr - base) >= count)
            return TRUE;
    }
    count -= (nextNodeAddr - base);
    base = nextNodeAddr;
    pHeapNode = K2HEAP_GetNextNode(&gResIoHeap, pHeapNode);
    K2_ASSERT(NULL != pHeapNode);

    K2_ASSERT(base == K2HEAP_NodeAddr(pHeapNode));
    do {
        nextNodeAddr = K2HEAP_NodeAddr(pHeapNode) + K2HEAP_NodeSize(pHeapNode);
        work = K2HEAP_NodeSize(pHeapNode);
        if (K2HEAP_NodeIsFree(pHeapNode))
        {
            if (work >= count)
            {
                stat = K2HEAP_AllocNodeAt(&gResIoHeap, base, count, &pNewHeapNode);
                K2_ASSERT(!K2STAT_IS_ERROR(stat));
                pResAlloc = K2_GET_CONTAINER(RESALLOC, pNewHeapNode, HeapNode);
                pResAlloc->mContext = aContext;
            }
            else
            {
                stat = K2HEAP_AllocNodeAt(&gResIoHeap, base, work, &pNewHeapNode);
                K2_ASSERT(!K2STAT_IS_ERROR(stat));
                pResAlloc = K2_GET_CONTAINER(RESALLOC, pNewHeapNode, HeapNode);
                pResAlloc->mContext = aContext;
            }
        }
        else
        {
            pResAlloc = K2_GET_CONTAINER(RESALLOC, pHeapNode, HeapNode);
            Debug_Printf("!!!IoResource - contexts 0x%08X and 0x%08X overlap\n", pResAlloc->mContext, aContext);
        }
        if (work >= count)
        {
            break;
        }

        base += work;
        count -= work;
        pHeapNode = K2HEAP_GetNextNode(&gResIoHeap, pHeapNode);
        K2_ASSERT(NULL != pHeapNode);
    } while (1);

    return TRUE;
}

BOOL
Res_PhysHeap_Reserve(
    K2OS_PHYSADDR_INFO const *  apPhysInfo,
    UINT_PTR                    aContext,
    BOOL                        aExclusive
)
{
    K2STAT          stat;
    RESALLOC *      pResAlloc;
    K2HEAP_NODE *   pHeapNode;
    UINT_PTR        base;
    UINT_PTR        count;
    UINT32          work;
    UINT32          nextNodeAddr;
    K2HEAP_NODE *   pNewHeapNode;

    Debug_Printf("PhysReserve: Base %08X Size %08X\n", apPhysInfo->mBaseAddr, apPhysInfo->mSizeBytes);

    stat = K2HEAP_AllocNodeAt(&gResPhysHeap, apPhysInfo->mBaseAddr, apPhysInfo->mSizeBytes, &pNewHeapNode);
    if (!K2STAT_IS_ERROR(stat))
    {
        pResAlloc = K2_GET_CONTAINER(RESALLOC, pNewHeapNode, HeapNode);
        pResAlloc->mContext = aContext;
        return TRUE;
    }

    if (aExclusive)
    {
        Debug_Printf("***PhysResource - context 0x%08X failed exclusive reserve of 0x%08X for 0x%08X\n",
            aContext,
            apPhysInfo->mBaseAddr,
            apPhysInfo->mSizeBytes);
        return FALSE;
    }

    //
    // nonexclusive alloc - make sure range of alloc is covered
    //
    base = apPhysInfo->mBaseAddr;
    count = apPhysInfo->mSizeBytes;
    pHeapNode = K2HEAP_GetFirstNodeInRange(&gResPhysHeap, base, count);
    K2_ASSERT(NULL != pHeapNode);

    work = K2HEAP_NodeAddr(pHeapNode);
    nextNodeAddr = work + K2HEAP_NodeSize(pHeapNode);
    if (K2HEAP_NodeIsFree(pHeapNode))
    {
        // start of phys range is inside a free space node
        K2_ASSERT((nextNodeAddr - base) < count); // or else allocat above should have been successful
        stat = K2HEAP_AllocNodeAt(&gResPhysHeap, base, nextNodeAddr - base, &pNewHeapNode);
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
        pResAlloc = K2_GET_CONTAINER(RESALLOC, pNewHeapNode, HeapNode);
        pResAlloc->mContext = aContext;
    }
    else
    {
        // start of io range is inside a node already in use
        if ((nextNodeAddr - base) >= count)
            return TRUE;
    }
    count -= (nextNodeAddr - base);
    base = nextNodeAddr;
    pHeapNode = K2HEAP_GetNextNode(&gResPhysHeap, pHeapNode);
    K2_ASSERT(NULL != pHeapNode);

    K2_ASSERT(base == K2HEAP_NodeAddr(pHeapNode));
    do {
        nextNodeAddr = K2HEAP_NodeAddr(pHeapNode) + K2HEAP_NodeSize(pHeapNode);
        work = K2HEAP_NodeSize(pHeapNode);
        if (K2HEAP_NodeIsFree(pHeapNode))
        {
            if (work >= count)
            {
                stat = K2HEAP_AllocNodeAt(&gResPhysHeap, base, count, &pNewHeapNode);
                K2_ASSERT(!K2STAT_IS_ERROR(stat));
                pResAlloc = K2_GET_CONTAINER(RESALLOC, pNewHeapNode, HeapNode);
                pResAlloc->mContext = aContext;
            }
            else
            {
                stat = K2HEAP_AllocNodeAt(&gResPhysHeap, base, work, &pNewHeapNode);
                K2_ASSERT(!K2STAT_IS_ERROR(stat));
                pResAlloc = K2_GET_CONTAINER(RESALLOC, pNewHeapNode, HeapNode);
                pResAlloc->mContext = aContext;
            }
        }
        else
        {
            pResAlloc = K2_GET_CONTAINER(RESALLOC, pHeapNode, HeapNode);
            Debug_Printf("!!!PhysResource - contexts 0x%08X and 0x%08X overlap\n", pResAlloc->mContext, aContext);
        }
        if (work >= count)
        {
            break;
        }

        base += work;
        count -= work;
        pHeapNode = K2HEAP_GetNextNode(&gResPhysHeap, pHeapNode);
        K2_ASSERT(NULL != pHeapNode);
    } while (1);

    return TRUE;
}

void
Res_Init(
    void
)
{
    K2OS_PHYSADDR_INFO  physInfo;
    K2OS_IOADDR_INFO    ioInfo;
    UINT_PTR            ioBytes;
    UINT_PTR            ix;
    K2STAT              stat;
    UINT32              chk32;
    UINT16              chk16;
    K2HEAP_NODE *       pNewHeapNode;
    RESALLOC *          pResAlloc;
    BOOL                first;

    K2HEAP_Init(&gResPhysHeap, Res_Heap_AcquireNode, Res_Heap_ReleaseNode);

    ix = 0;
    first = TRUE;
    ioBytes = sizeof(K2OS_PHYSADDR_INFO);
    do {
        stat = K2OS_System_GetInfo(K2OS_SYSINFO_PHYSADDR, ix, &physInfo, &ioBytes);
        if (K2STAT_IS_ERROR(stat))
            break;
        if (physInfo.mSizeBytes > 0)
        {
            chk32 = physInfo.mBaseAddr + physInfo.mSizeBytes;
            K2_ASSERT((chk32 > physInfo.mBaseAddr) || (0 == chk32));
            if (first)
            {
                Debug_Printf("PhysReserve: Base %08X Size %08X\n", physInfo.mBaseAddr, physInfo.mSizeBytes);
                stat = K2HEAP_AddFreeSpaceNode(&gResPhysHeap, physInfo.mBaseAddr, physInfo.mSizeBytes, NULL);
                K2_ASSERT(!K2STAT_IS_ERROR(stat));
                stat = K2HEAP_AllocNodeAt(&gResPhysHeap, physInfo.mBaseAddr, physInfo.mSizeBytes, &pNewHeapNode);
                K2_ASSERT(!K2STAT_IS_ERROR(stat));
                pResAlloc = K2_GET_CONTAINER(RESALLOC, pNewHeapNode, HeapNode);
                pResAlloc->mContext = 0;
                if (physInfo.mBaseAddr > 0)
                {
                    stat = K2HEAP_AddFreeSpaceNode(&gResPhysHeap, 0, physInfo.mBaseAddr, NULL);
                    K2_ASSERT(!K2STAT_IS_ERROR(stat));
                }
                chk32 = (physInfo.mBaseAddr + physInfo.mSizeBytes);
                if (chk32 != 0)
                {
                    stat = K2HEAP_AddFreeSpaceNode(&gResPhysHeap, chk32, (~chk32) + 1, NULL);
                    K2_ASSERT(!K2STAT_IS_ERROR(stat));
                }
                first = FALSE;
            }
            else
            {
                Res_PhysHeap_Reserve(&physInfo, 0, FALSE);
            }
        }
        ++ix;
    } while (1);
    K2_ASSERT(ix > 0);

    K2HEAP_Init(&gResIoHeap, Res_Heap_AcquireNode, Res_Heap_ReleaseNode);
    K2HEAP_AddFreeSpaceNode(&gResIoHeap, 0, 64 * 1024, NULL);

    ix = 0;
    ioBytes = sizeof(K2OS_IOADDR_INFO);
    do {
        stat = K2OS_System_GetInfo(K2OS_SYSINFO_IOADDR, ix, &ioInfo, &ioBytes);
        if (K2STAT_IS_ERROR(stat))
            break;
        if (ioInfo.mPortCount > 0)
        {
            chk16 = ioInfo.mPortBase + ioInfo.mPortCount;
            K2_ASSERT((chk16 > ioInfo.mPortBase) || (chk16 == 0));
            Res_IoHeap_Reserve(&ioInfo, 0, FALSE);
        }
        ++ix;
    } while (1);
}
