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

#include "ik2osacpi.h"

#define CACHE_BLOCK_EXPAND_COUNT  96

typedef struct _CACHE_HDR       CACHE_HDR;
typedef struct _CACHE_BLOCK_HDR CACHE_BLOCK_HDR;
typedef struct _CACHE_ITEM_HDR  CACHE_ITEM_HDR;

struct _CACHE_ITEM_HDR
{
    CACHE_BLOCK_HDR *   mpBlock;
    K2LIST_LINK         ItemListLink;
    UINT8               Payload[4];
};

#define CACHE_ITEM_BYTES(alignedPayloadBytes) \
    (sizeof(CACHE_ITEM_HDR) - 4 + (alignedPayloadBytes))

struct _CACHE_BLOCK_HDR
{
    CACHE_HDR *     mpCache;
    K2LIST_LINK     BlockListLink;
    UINT32          mBlockFreeCount;
    CACHE_ITEM_HDR  Items[1];           // first of many(array)
};

#define CACHE_BLOCK_BYTES(blockItemCount, alignedPayloadBytes) \
    (sizeof(CACHE_BLOCK_HDR) - sizeof(CACHE_ITEM_HDR) + ((blockItemCount) * CACHE_ITEM_BYTES(alignedPayloadBytes)))

struct _CACHE_HDR
{
    K2LIST_LINK         CacheListLink;
    UINT32              mBlockExpandItemCount;
    UINT32              mTruePayloadBytes;
    UINT32              mInitBlockItemCount;
    char *              mpCacheName;
    K2OS_CRITSEC        Sec;
    K2LIST_ANCHOR       BlockList;
    K2LIST_ANCHOR       ItemAllocatedList;
    K2LIST_ANCHOR       ItemFreeList;
    CACHE_BLOCK_HDR     Block0;         // only block 0 lives here
};

#define CACHE_BYTES(blockItemCount, alignedPayloadBytes) \
    ((sizeof(CACHE_HDR) - sizeof(CACHE_BLOCK_HDR)) + CACHE_BLOCK_BYTES(blockItemCount, alignedPayloadBytes))

void
sInitNewBlock(
    CACHE_HDR *         apCache,
    CACHE_BLOCK_HDR *   apBlock,
    UINT32              aItemCount,
    UINT32              aAlignedPayloadBytes
)
{
    UINT32              ix;
    CACHE_ITEM_HDR *    pItemHdr;
    UINT32              itemBytes;

    apBlock->mpCache = apCache;
    pItemHdr = &apBlock->Items[0];
    itemBytes = CACHE_ITEM_BYTES(aAlignedPayloadBytes);
    for (ix = 0; ix < aItemCount; ix++)
    {
        pItemHdr->mpBlock = apBlock;
        K2LIST_AddAtTail(&apCache->ItemFreeList, &pItemHdr->ItemListLink);
        K2MEM_Set(&pItemHdr->Payload[0], 0xFE, apCache->mTruePayloadBytes);
        apBlock->mBlockFreeCount++;
        pItemHdr = (CACHE_ITEM_HDR *)(((UINT8 *)pItemHdr) + itemBytes);
    }
}

void
sPurgeBlockItems(
    CACHE_HDR *         apCache,
    CACHE_BLOCK_HDR *   apBlock,
    UINT32              aItemCount,
    UINT32              aAlignedPayloadBytes
)
{
    UINT32              ix;
    CACHE_ITEM_HDR *    pItemHdr;
    UINT32              itemBytes;

    K2_ASSERT(apBlock->mBlockFreeCount == aItemCount);

    pItemHdr = &apBlock->Items[0];
    itemBytes = CACHE_ITEM_BYTES(aAlignedPayloadBytes);
    for (ix = 0; ix < aItemCount; ix++)
    {
        pItemHdr->mpBlock = NULL;
        K2_ASSERT(K2MEM_Verify(&pItemHdr->Payload[0], 0xFE, apCache->mTruePayloadBytes));
        K2LIST_Remove(&apCache->ItemFreeList, &pItemHdr->ItemListLink);
        K2MEM_Set(&pItemHdr->Payload[0], 0, apCache->mTruePayloadBytes);
        apBlock->mBlockFreeCount--;
        pItemHdr = (CACHE_ITEM_HDR *)(((UINT8 *)pItemHdr) + itemBytes);
    }
}

ACPI_STATUS
AcpiOsCreateCache(
    char                    *CacheName,
    UINT16                  ObjectSize,
    UINT16                  MaxDepth,
    ACPI_CACHE_T            **ReturnCache)
{
    CACHE_HDR *         pRet;
    UINT32              alignedPayloadBytes;
    UINT32              cacheBytes;

    K2_ASSERT(ObjectSize > 0);
    K2_ASSERT(MaxDepth > 0);

    alignedPayloadBytes = (ObjectSize + 3) & ~3;

    if (MaxDepth < CACHE_BLOCK_EXPAND_COUNT)
        MaxDepth = CACHE_BLOCK_EXPAND_COUNT;

    cacheBytes = CACHE_BYTES(MaxDepth, alignedPayloadBytes);

    pRet = (CACHE_HDR *)K2OS_Heap_Alloc(cacheBytes);
    if (pRet == NULL)
    {
        return AE_ERROR;
    }

    K2MEM_Zero(pRet, cacheBytes);

    if (!K2OS_CritSec_Init(&pRet->Sec))
    {
        K2OS_Heap_Free(pRet);
        return AE_ERROR;
    }

    pRet->mBlockExpandItemCount = CACHE_BLOCK_EXPAND_COUNT;
    pRet->mTruePayloadBytes = ObjectSize;
    pRet->mInitBlockItemCount = MaxDepth;
    pRet->mpCacheName = CacheName;
    K2LIST_Init(&pRet->BlockList);
    K2LIST_Init(&pRet->ItemAllocatedList);
    K2LIST_Init(&pRet->ItemFreeList);

    sInitNewBlock(pRet, &pRet->Block0, pRet->mInitBlockItemCount, alignedPayloadBytes);

    K2LIST_AddAtTail(&pRet->BlockList, &pRet->Block0.BlockListLink);

    K2_ASSERT(pRet->ItemFreeList.mNodeCount == MaxDepth);

    K2OS_CritSec_Enter(&gK2OSACPI_CacheSec);
    K2LIST_AddAtTail(&gK2OSACPI_CacheList, &pRet->CacheListLink);
    K2OS_CritSec_Leave(&gK2OSACPI_CacheSec);

    *ReturnCache = (ACPI_CACHE_T *)pRet;

    return AE_OK;
}

static
void
sLocked_ReleaseObject(
    CACHE_HDR *         apCache,
    CACHE_ITEM_HDR *    apItem
)
{
    K2LIST_Remove(&apCache->ItemAllocatedList, &apItem->ItemListLink);

    K2LIST_AddAtTail(&apCache->ItemFreeList, &apItem->ItemListLink);

    apItem->mpBlock->mBlockFreeCount++;

    K2MEM_Set(&apItem->Payload[0], 0xFE, apCache->mTruePayloadBytes);
}

ACPI_STATUS
AcpiOsDeleteCache(
    ACPI_CACHE_T            *Cache)
{
    CACHE_HDR *         pCache;
    K2LIST_LINK *       pListLink;
    CACHE_BLOCK_HDR *   pBlock;
    UINT32              itemCount;
    K2LIST_ANCHOR       freeList;
    UINT32              alignedPayloadBytes;

    pCache = (CACHE_HDR *)Cache;

    alignedPayloadBytes = (pCache->mTruePayloadBytes + 3) & ~3;

    K2LIST_Init(&freeList);

    K2OS_CritSec_Enter(&pCache->Sec);

    pListLink = pCache->ItemAllocatedList.mpHead;
    if (pListLink != NULL)
    {
        do {
            sLocked_ReleaseObject(pCache, K2_GET_CONTAINER(CACHE_ITEM_HDR, pListLink, ItemListLink));
            pListLink = pCache->ItemAllocatedList.mpHead;
        } while (pListLink != NULL);
    }

    //
    // all blocks in the cache should be full of free items now
    //
    pListLink = pCache->BlockList.mpHead;
    K2_ASSERT(pListLink != NULL);
    do {
        pBlock = K2_GET_CONTAINER(CACHE_BLOCK_HDR, pBlock, BlockListLink);
        if (pBlock != &pCache->Block0)
            itemCount = pCache->mBlockExpandItemCount;
        else
            itemCount = pCache->mInitBlockItemCount;

        sPurgeBlockItems(pCache, pBlock, itemCount, alignedPayloadBytes);

        K2LIST_Remove(&pCache->BlockList, &pBlock->BlockListLink);

        K2LIST_AddAtTail(&freeList, &pBlock->BlockListLink);

        pListLink = pCache->BlockList.mpHead;
    } while (pListLink != NULL);

    K2OS_CritSec_Leave(&pCache->Sec);

    K2OS_CritSec_Enter(&gK2OSACPI_CacheSec);
    K2LIST_Remove(&gK2OSACPI_CacheList, &pCache->CacheListLink);
    K2OS_CritSec_Leave(&gK2OSACPI_CacheSec);

    K2OS_CritSec_Done(&pCache->Sec);

    pListLink = freeList.mpHead;
    K2_ASSERT(pListLink != NULL);
    do
    {
        pBlock = K2_GET_CONTAINER(CACHE_BLOCK_HDR, pListLink, BlockListLink);
        pListLink = pListLink->mpNext;

        if (pBlock != &pCache->Block0)
            K2OS_Heap_Free(pBlock);

    } while (pListLink != NULL);

    K2MEM_Zero(pCache, sizeof(CACHE_HDR));

    K2OS_Heap_Free(pCache);

    return AE_OK;
}

ACPI_STATUS
AcpiOsPurgeCache(
    ACPI_CACHE_T            *Cache)
{
    CACHE_HDR *         pCache;
    K2LIST_LINK *       pListLink;
    CACHE_BLOCK_HDR *   pBlock;
    UINT32              alignedPayloadBytes;
    K2LIST_ANCHOR       freeList;

    pCache = (CACHE_HDR *)Cache;

    alignedPayloadBytes = (pCache->mTruePayloadBytes + 3) & ~3;

    K2LIST_Init(&freeList);

    K2OS_CritSec_Enter(&pCache->Sec);

    pListLink = pCache->BlockList.mpHead;
    do {
        pBlock = K2_GET_CONTAINER(CACHE_BLOCK_HDR, pListLink, BlockListLink);
        pListLink = pListLink->mpNext;

        if (pBlock != &pCache->Block0)
        {
            if (pBlock->mBlockFreeCount == pCache->mBlockExpandItemCount)
            {
                sPurgeBlockItems(pCache, pBlock, pCache->mBlockExpandItemCount, alignedPayloadBytes);

                K2LIST_Remove(&pCache->BlockList, &pBlock->BlockListLink);

                K2LIST_AddAtTail(&freeList, &pBlock->BlockListLink);
            }
        }
    } while (pListLink != NULL);

    K2OS_CritSec_Leave(&pCache->Sec);

    pListLink = freeList.mpHead;
    if (pListLink != NULL)
    {
        do
        {
            pBlock = K2_GET_CONTAINER(CACHE_BLOCK_HDR, pListLink, BlockListLink);
            pListLink = pListLink->mpNext;
            K2OS_Heap_Free(pBlock);
        } while (pListLink != NULL);
    }

    return AE_OK;
}

void *
AcpiOsAcquireObject(
    ACPI_CACHE_T            *Cache)
{
    CACHE_HDR *         pCache;
    UINT8 *             pAct;
    UINT32              alignedPayloadBytes;
    UINT32              blockBytes;
    CACHE_BLOCK_HDR *   pBlock;
    CACHE_ITEM_HDR *    pItem;

    pCache = (CACHE_HDR *)Cache;

    K2OS_CritSec_Enter(&pCache->Sec);

    do {
        if (pCache->ItemFreeList.mNodeCount > 0)
        {
            pItem = K2_GET_CONTAINER(CACHE_ITEM_HDR, pCache->ItemFreeList.mpHead, ItemListLink);

            K2_ASSERT(pItem->mpBlock->mBlockFreeCount > 0);

            K2LIST_Remove(&pCache->ItemFreeList, &pItem->ItemListLink);

            pItem->mpBlock->mBlockFreeCount--;

            K2LIST_AddAtTail(&pCache->ItemAllocatedList, &pItem->ItemListLink);

            K2OS_CritSec_Leave(&pCache->Sec);

            pAct = &pItem->Payload[0];

            break;
        }

        K2OS_CritSec_Leave(&pCache->Sec);

        alignedPayloadBytes = (pCache->mTruePayloadBytes + 3) & ~3;

        blockBytes = CACHE_BLOCK_BYTES(pCache->mBlockExpandItemCount, alignedPayloadBytes);

        pBlock = (CACHE_BLOCK_HDR *)K2OS_Heap_Alloc(blockBytes);
        if (pBlock == NULL)
        {
            return NULL;
        }

        K2MEM_Zero(pBlock, blockBytes);

        K2OS_CritSec_Enter(&pCache->Sec);

        sInitNewBlock(pCache, pBlock, pCache->mBlockExpandItemCount, alignedPayloadBytes);

        K2LIST_AddAtTail(&pCache->BlockList, &pBlock->BlockListLink);

    } while (1);

    K2_ASSERT(K2MEM_Verify(pAct, 0xFE, pCache->mTruePayloadBytes));

    K2MEM_Zero(pAct, pCache->mTruePayloadBytes);

    return pAct;
}

ACPI_STATUS
AcpiOsReleaseObject(
    ACPI_CACHE_T            *Cache,
    void                    *Object)
{
    CACHE_HDR *         pCache;
    CACHE_ITEM_HDR *    pItem;

    pCache = (CACHE_HDR *)Cache;

    pItem = K2_GET_CONTAINER(CACHE_ITEM_HDR, Object, Payload[0]);

    K2MEM_Set(&pItem->Payload[0], 0xFE, pCache->mTruePayloadBytes);

    K2OS_CritSec_Enter(&pCache->Sec);

    sLocked_ReleaseObject(pCache, pItem);

    K2OS_CritSec_Leave(&pCache->Sec);

    return AE_OK;
}

