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

#include <lib/k2ramheap.h>

#define DBG_ALLOC_PATTERN   0
#define K2_STATIC           static      

K2_STATIC
BOOL
sCheckHdrSent(
    K2RAMHEAP_NODE *    apNode,
    BOOL                aCheckFree
)
{
    UINT_PTR sent;

    sent = apNode->mSentinel;

    if (sent == K2RAMHEAP_NODE_HDRSENT_FREE)
    {
        return aCheckFree;
    }

    if (sent == K2RAMHEAP_NODE_HDRSENT_USED)
    {
        return !aCheckFree;
    }

    return FALSE;
}

K2_STATIC
BOOL
sCheckEndSent(
    K2RAMHEAP_NODE *apNode
)
{
    UINT_PTR * pEnd;
    pEnd = (UINT_PTR *)(((UINT_PTR)apNode) + sizeof(K2RAMHEAP_NODE) + apNode->TreeNode.mUserVal);
    return (*pEnd == K2RAMHEAP_NODE_ENDSENT);
}

K2_STATIC
K2RAMHEAP_CHUNK *
sScanForAddrChunk(
    K2RAMHEAP * apHeap,
    UINT_PTR    aAddr
)
{
    K2LIST_LINK *       pLink;
    K2RAMHEAP_CHUNK *   pChunk;

    pLink = apHeap->ChunkList.mpHead;
    do {
        pChunk = K2_GET_CONTAINER(K2RAMHEAP_CHUNK, pLink, ChunkListLink);
        if ((aAddr >= pChunk->mStart) &&
            (aAddr < pChunk->mBreak))
        {
            return pChunk;
        }
        pLink = pChunk->ChunkListLink.mpNext;
    } while (pLink != NULL);

    return NULL;
}

K2_STATIC
void
sLockHeap(
    K2RAMHEAP * apHeap
)
{
    if (apHeap->Supp.fLock == NULL)
        return;
    apHeap->mLockDisp = apHeap->Supp.fLock(apHeap);
}

K2_STATIC
void
sUnlockHeap(
    K2RAMHEAP * apHeap
)
{
    if (apHeap->Supp.fUnlock == NULL)
        return;
    apHeap->Supp.fUnlock(apHeap, apHeap->mLockDisp);
}

#if 0

K2_STATIC
void
sDump(
    K2RAMHEAP *apHeap
)
{
    K2LIST_LINK *        pLink;
    K2LIST_LINK *        pChunkLink;
    K2RAMHEAP_CHUNK *    pChunk;
    K2RAMHEAP_NODE *     pNode;
    char                 sentCode[8];
    
    sentCode[4] = 0;
    printf("\n--------ADDRLIST-------------\n");
    pChunkLink = apHeap->ChunkList.mpHead;
    if (pChunkLink != NULL)
    {
        printf("--BYCHUNK\n");
        do {
            pChunk = K2_GET_CONTAINER(K2RAMHEAP_CHUNK, pChunkLink, ChunkListLink);
            printf("%08X CHUNK\n", (UINT_PTR)pChunk);
            pNode = pChunk->mpFirstNodeInChunk;
            if (pNode != NULL)
            {
                do {
                    K2MEM_Copy(sentCode, &pNode->mSentinel, 4);
                    printf("   %08X %s %08X %d\n", (UINT_PTR)pNode, sentCode, pNode->TreeNode.mUserVal, pNode->TreeNode.mUserVal);
                    if (pNode == pChunk->mpLastNodeInChunk)
                        break;
                    pNode = K2_GET_CONTAINER(K2RAMHEAP_NODE, pNode->AddrListLink.mpNext, AddrListLink);
                } while (TRUE);
            }
            pChunkLink = pChunkLink->mpNext;
        } while (pChunkLink != NULL);
    }

    pLink = apHeap->AddrList.mpHead;
    if (pLink != NULL)
    {
        printf("--LINEAR\n");
        do {
            pNode = K2_GET_CONTAINER(K2RAMHEAP_NODE, pLink, AddrListLink);
            K2MEM_Copy(sentCode, &pNode->mSentinel, 4);
            printf("%08X %s %08X %d\n", (UINT_PTR)pNode, sentCode, pNode->TreeNode.mUserVal, pNode->TreeNode.mUserVal);

            pLink = pLink->mpNext;
        } while (pLink != NULL);
    }

    printf("------------------------------\n");
}

#endif

K2_STATIC
void
sCheckLockedHeap(
    K2RAMHEAP *apHeap
)
{
    K2LIST_LINK *       pLink;
    K2RAMHEAP_CHUNK *   pChunk;
    K2RAMHEAP_NODE *    pNode;
    K2LIST_LINK *       pNextLink;
    K2RAMHEAP_CHUNK *   pNextChunk;
    K2RAMHEAP_NODE *    pNextNode;
    UINT_PTR            checkOverhead;
    UINT_PTR            checkAllocBytes;
    UINT_PTR            checkFreeBytes;
    UINT_PTR            checkAllocCount;
    UINT_PTR            checkFreeCount;
    K2TREE_NODE *       pNextTreeNode;

    pLink = apHeap->AddrList.mpHead;
    if (pLink == NULL)
    {
        K2_ASSERT(apHeap->AddrList.mpTail == NULL);
        K2_ASSERT(K2TREE_IsEmpty(&apHeap->UsedTree));
        K2_ASSERT(K2TREE_IsEmpty(&apHeap->FreeTree));
        K2_ASSERT(apHeap->HeapState.mAllocCount == 0);
        K2_ASSERT(apHeap->HeapState.mTotalAlloc == 0);
        K2_ASSERT(apHeap->HeapState.mTotalFree == 0);
        K2_ASSERT(apHeap->HeapState.mTotalOverhead == 0);
        return;
    }

    K2_ASSERT(apHeap->HeapState.mAllocCount > 0);
    K2_ASSERT(apHeap->HeapState.mTotalAlloc > 0);
    K2_ASSERT(!K2TREE_IsEmpty(&apHeap->UsedTree));
    if (apHeap->HeapState.mTotalFree > 0)
    {
        K2_ASSERT(!K2TREE_IsEmpty(&apHeap->FreeTree));
    }

    pChunk = sScanForAddrChunk(apHeap, (UINT_PTR)pLink);
    K2_ASSERT(pChunk != NULL);
    K2_ASSERT(pChunk->mStart == (UINT_PTR)pChunk);
    K2_ASSERT(pChunk->ChunkListLink.mpPrev == NULL);

    pNode = K2_GET_CONTAINER(K2RAMHEAP_NODE, pLink, AddrListLink);
    K2_ASSERT(pChunk->mpFirstNodeInChunk == pNode);
    K2_ASSERT(sCheckHdrSent(pNode, (pNode->mSentinel == K2RAMHEAP_NODE_HDRSENT_FREE)));
    K2_ASSERT(sCheckEndSent(pNode));

    if (pNode->mSentinel == K2RAMHEAP_NODE_HDRSENT_USED)
    {
        K2_ASSERT(K2TREE_FirstNode(&apHeap->UsedTree) == &pNode->TreeNode);
        pNextTreeNode = K2TREE_NextNode(&apHeap->UsedTree, &pNode->TreeNode);
    }
    else
    {
        pNextTreeNode = K2TREE_FirstNode(&apHeap->UsedTree);
    }

    checkOverhead = sizeof(K2RAMHEAP_CHUNK);
    checkAllocCount = 0;
    checkFreeCount = 0;
    checkAllocBytes = 0;
    checkFreeBytes = 0;

    //
    // pLink = first link in all blocks
    // pChunk = first chunk in all chunks
    // pNode = first node in all nodes
    //
    do {
        checkOverhead += K2RAMHEAP_NODE_OVERHEAD;
        if (pNode->mSentinel == K2RAMHEAP_NODE_HDRSENT_USED)
        {
            checkAllocCount++;
            checkAllocBytes += pNode->TreeNode.mUserVal;
        }
        else
        {
            checkFreeCount++;
            checkFreeBytes += pNode->TreeNode.mUserVal;
        }

        pNextLink = pLink->mpNext;
        if (pNextLink == NULL)
        {
            //
            // should be last node in heap
            //

            //
            // last node in chunk
            //
            K2_ASSERT(pNode == pChunk->mpLastNodeInChunk);

            //
            // last chunk in heap
            //
            K2_ASSERT(pChunk->ChunkListLink.mpNext == NULL);

            //
            // last node in tree
            //
            K2_ASSERT(pNextTreeNode == NULL);
            if (pNode->mSentinel == K2RAMHEAP_NODE_HDRSENT_USED)
            {
                K2_ASSERT(K2TREE_LastNode(&apHeap->UsedTree) == &pNode->TreeNode);
            }

            break;
        }

        //
        // next node should not be last node in heap
        //
        if (pChunk->mpLastNodeInChunk == pNode)
        {
            K2_ASSERT(pChunk->ChunkListLink.mpNext != NULL);

            pNextChunk = K2_GET_CONTAINER(K2RAMHEAP_CHUNK, pChunk->ChunkListLink.mpNext, ChunkListLink);
            K2_ASSERT(pNextChunk->ChunkListLink.mpPrev = &pChunk->ChunkListLink);
            checkOverhead += sizeof(K2RAMHEAP_CHUNK);

            K2_ASSERT(pNextLink == &pNextChunk->mpFirstNodeInChunk->AddrListLink);
            K2_ASSERT(pNextChunk->mpFirstNodeInChunk->AddrListLink.mpPrev == pLink);
        }
        else
            pNextChunk = pChunk;

        K2_ASSERT(sScanForAddrChunk(apHeap, (UINT_PTR)pNextLink) == pNextChunk);

        pNextNode = K2_GET_CONTAINER(K2RAMHEAP_NODE, pNextLink, AddrListLink);

        K2_ASSERT(sCheckHdrSent(pNextNode, pNextNode->mSentinel == K2RAMHEAP_NODE_HDRSENT_FREE));
        K2_ASSERT(sCheckEndSent(pNextNode));

        if (pChunk == pNextChunk)
        {
            K2_ASSERT((((UINT_PTR)pNode) + pNode->TreeNode.mUserVal + K2RAMHEAP_NODE_OVERHEAD) == (UINT_PTR)pNextNode);
        }

        if (pNextNode->mSentinel == K2RAMHEAP_NODE_HDRSENT_USED)
        {
            K2_ASSERT(pNextTreeNode == &pNextNode->TreeNode);
            pNextTreeNode = K2TREE_NextNode(&apHeap->UsedTree, &pNextNode->TreeNode);
        }
        else
        {

        }

        pLink = pNextLink;
        pNode = pNextNode;
        pChunk = pNextChunk;

    } while (1);

    K2_ASSERT(checkAllocBytes == apHeap->HeapState.mTotalAlloc);
    K2_ASSERT(checkFreeBytes == apHeap->HeapState.mTotalFree);
    K2_ASSERT(checkAllocCount == apHeap->UsedTree.mNodeCount);
    K2_ASSERT(checkAllocCount == apHeap->HeapState.mAllocCount);
    K2_ASSERT(checkFreeCount == apHeap->FreeTree.mNodeCount);
    K2_ASSERT(checkOverhead == apHeap->HeapState.mTotalOverhead);
}

K2_STATIC
int
sAddrCompare(
    UINT_PTR        aKey,
    K2TREE_NODE *   apTreeNode
)
{
    //
    // key is address of first byte in a block
    //
    K2_ASSERT(aKey >= sizeof(K2RAMHEAP_NODE));
    return (int)((aKey - sizeof(K2RAMHEAP_NODE)) - ((UINT_PTR)K2_GET_CONTAINER(K2RAMHEAP_NODE, apTreeNode, TreeNode)));
}

K2_STATIC
int
sSizeCompare(
    UINT_PTR        aKey,
    K2TREE_NODE *   apTreeNode
)
{
    return (int)(aKey - apTreeNode->mUserVal);
}

void
K2RAMHEAP_Init(
    K2RAMHEAP *             apHeap,
    K2RAMHEAP_SUPP const *  apSupp
)
{
    K2_ASSERT(NULL != apSupp);
    K2_ASSERT(NULL != apSupp->fCreateRange);
    K2_ASSERT(NULL != apSupp->fCommitPages);

    K2MEM_Zero(apHeap, sizeof(K2RAMHEAP));
    K2LIST_Init(&apHeap->ChunkList);
    K2LIST_Init(&apHeap->AddrList);
    K2TREE_Init(&apHeap->UsedTree, sAddrCompare);
    K2TREE_Init(&apHeap->FreeTree, sSizeCompare);
    K2MEM_Copy(&apHeap->Supp, apSupp, sizeof(K2RAMHEAP_SUPP));
    apHeap->mLockDisp = 0;
}

K2_STATIC 
void
sAllocFromNode(
    K2RAMHEAP *      apHeap,
    K2RAMHEAP_NODE * apAllocFromNode,
    UINT_PTR         aByteCount,
    UINT_PTR *       apRetAllocAddr
)
{
    UINT_PTR            nodeFreeSpace;
    UINT_PTR *          pEnd;
    K2RAMHEAP_NODE *    pNewNode;
    K2RAMHEAP_CHUNK *   pChunk;
    UINT_PTR            key;

    K2_ASSERT(sCheckHdrSent(apAllocFromNode, TRUE));
    K2_ASSERT(sCheckEndSent(apAllocFromNode));

    K2_ASSERT(aByteCount > 0);
    K2_ASSERT((aByteCount & (sizeof(UINT_PTR) - 1)) == 0);
    nodeFreeSpace = apAllocFromNode->TreeNode.mUserVal;
    K2_ASSERT(nodeFreeSpace >= aByteCount);

    pChunk = sScanForAddrChunk(apHeap, (UINT_PTR)apAllocFromNode);
    K2_ASSERT(pChunk != NULL);

    K2TREE_Remove(&apHeap->FreeTree, &apAllocFromNode->TreeNode);

    if ((nodeFreeSpace - aByteCount) >= (K2RAMHEAP_NODE_OVERHEAD + sizeof(UINT_PTR)))
    {
        //
        // cut the node into two and create a new free node
        //
        pEnd = (UINT_PTR *)(((UINT8 *)apAllocFromNode) + sizeof(K2RAMHEAP_NODE) + aByteCount);
        *pEnd = K2RAMHEAP_NODE_ENDSENT;
        pNewNode = (K2RAMHEAP_NODE *)(pEnd + 1);
        if (apAllocFromNode == pChunk->mpLastNodeInChunk)
            pChunk->mpLastNodeInChunk = pNewNode;
        pNewNode->mSentinel = K2RAMHEAP_NODE_HDRSENT_FREE;
        pNewNode->TreeNode.mUserVal = nodeFreeSpace - (K2RAMHEAP_NODE_OVERHEAD + aByteCount);
        apAllocFromNode->TreeNode.mUserVal = aByteCount;
        K2LIST_AddAfter(&apHeap->AddrList, &pNewNode->AddrListLink, &apAllocFromNode->AddrListLink);
        K2TREE_Insert(&apHeap->FreeTree, pNewNode->TreeNode.mUserVal, &pNewNode->TreeNode);
        pEnd = (UINT_PTR *)(((UINT8 *)pNewNode) + sizeof(K2RAMHEAP_NODE) + pNewNode->TreeNode.mUserVal);
        K2_ASSERT(sCheckEndSent(pNewNode));
        apHeap->HeapState.mTotalOverhead += K2RAMHEAP_NODE_OVERHEAD;
        apHeap->HeapState.mTotalFree -= aByteCount + K2RAMHEAP_NODE_OVERHEAD;
    }
    else
    {
        //
        // else we take the whole node - whatever is left is not big enough for another node
        //
        apHeap->HeapState.mTotalFree -= apAllocFromNode->TreeNode.mUserVal;
    }

    apAllocFromNode->mSentinel = K2RAMHEAP_NODE_HDRSENT_USED;

    key = ((UINT_PTR)apAllocFromNode) + sizeof(K2RAMHEAP_NODE);

    K2TREE_Insert(&apHeap->UsedTree, key, &apAllocFromNode->TreeNode);

    *apRetAllocAddr = key;

    apHeap->HeapState.mAllocCount++;
    apHeap->HeapState.mTotalAlloc += apAllocFromNode->TreeNode.mUserVal;
}

K2_STATIC
K2STAT
sPushBreak(
    K2RAMHEAP *         apHeap,
    K2RAMHEAP_CHUNK *   apChunk,
    UINT_PTR            aPushAmount,
    K2RAMHEAP_NODE **   apRetEndNode
)
{
    //
    // find the last node in this chunk 
    //
    K2RAMHEAP_NODE *    pNode;
    K2RAMHEAP_NODE *    pNewNode;
    UINT_PTR *          pEnd;
    K2STAT              stat;
        
    //
    // push the break in the node
    //
    stat = apHeap->Supp.fCommitPages(
        apChunk->mRange,
        apChunk->mBreak, 
        aPushAmount / K2_VA_MEMPAGE_BYTES);
    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    //
    // may not be used but we set it here
    //
    pNewNode = (K2RAMHEAP_NODE *)apChunk->mBreak;

    //
    // break has moved
    //
    apChunk->mBreak += aPushAmount;

    //
    // now expand the chunk by adding the free space
    //
    pNode = apChunk->mpLastNodeInChunk;
    if (pNode->mSentinel == K2RAMHEAP_NODE_HDRSENT_FREE)
    {
        //
        // last node in chunk is free and will be the end node returned (no new node)
        //
        K2TREE_Remove(&apHeap->FreeTree, &pNode->TreeNode);
        pEnd = (UINT_PTR *)(((UINT8 *)pNode) + sizeof(K2RAMHEAP_NODE) + pNode->TreeNode.mUserVal);
        K2_ASSERT(*pEnd == K2RAMHEAP_NODE_ENDSENT);
        *pEnd = 0;
        pNode->TreeNode.mUserVal += aPushAmount;
        apHeap->HeapState.mTotalFree += aPushAmount;
    }
    else
    {
        //
        // last node in chunk is used and a new node will be the end node returned
        //
        pNewNode->mSentinel = K2RAMHEAP_NODE_HDRSENT_FREE;
        pNewNode->TreeNode.mUserVal = aPushAmount - K2RAMHEAP_NODE_OVERHEAD;
        K2LIST_AddAfter(&apHeap->AddrList, &pNewNode->AddrListLink, &pNode->AddrListLink);
        apChunk->mpLastNodeInChunk = pNewNode;
        pNode = pNewNode;
        apHeap->HeapState.mTotalFree += pNewNode->TreeNode.mUserVal;
        apHeap->HeapState.mTotalOverhead += K2RAMHEAP_NODE_OVERHEAD;
    }

    *apRetEndNode = pNode;

    pEnd = (UINT_PTR *)(((UINT8 *)pNode) + sizeof(K2RAMHEAP_NODE) + pNode->TreeNode.mUserVal);
    *pEnd = K2RAMHEAP_NODE_ENDSENT;
    pEnd++;

    K2_ASSERT(pEnd == ((UINT_PTR*)apChunk->mBreak));

    K2TREE_Insert(&apHeap->FreeTree, pNode->TreeNode.mUserVal, &pNode->TreeNode);

    return K2STAT_OK;
}

K2_STATIC
K2STAT
sGrowHeapChunk(
    K2RAMHEAP *         apHeap,
    K2RAMHEAP_CHUNK *   apChunk,
    UINT_PTR            aByteCount,
    UINT_PTR *          apRetAllocAddr
)
{
    K2STAT              stat;
    UINT_PTR            spaceLeft;
    UINT_PTR            breakPushAmount;
    K2RAMHEAP_NODE *    pNewEndNode;

    spaceLeft = apChunk->mTop - apChunk->mBreak;

    breakPushAmount = K2_ROUNDUP(aByteCount + K2RAMHEAP_NODE_OVERHEAD, K2_VA_MEMPAGE_BYTES);

    K2_ASSERT(spaceLeft >= breakPushAmount); // caller will have validated this before call

    stat = sPushBreak(apHeap, apChunk, breakPushAmount, &pNewEndNode);

    if (K2STAT_IS_ERROR(stat))
        return stat;

    K2_ASSERT(sCheckHdrSent(pNewEndNode, TRUE));
    K2_ASSERT(sCheckEndSent(pNewEndNode));
    K2_ASSERT(pNewEndNode->TreeNode.mUserVal >= aByteCount);

    sAllocFromNode(apHeap, pNewEndNode, aByteCount, apRetAllocAddr);

    return K2STAT_NO_ERROR;
}

K2_STATIC
void
iAddAndAllocFromChunk(
    K2RAMHEAP * apHeap,
    void *      aRange,
    UINT_PTR    aChunkBase,
    UINT_PTR    aChunkInit,
    UINT_PTR    aChunkFull,
    UINT_PTR    aByteCount,
    UINT_PTR *  apRetAllocAddr
)
{
    K2RAMHEAP_CHUNK *    pChunk;
    K2RAMHEAP_CHUNK *    pChunkPrev;
    K2RAMHEAP_CHUNK *    pChunkNext;
    K2RAMHEAP_NODE *     pNode;
    K2LIST_LINK *        pScan;
    K2RAMHEAP_CHUNK *    pScanChunk;
    UINT_PTR *           pEnd;

    //
    // find prev and next node to new chunk
    //
    pScan = apHeap->ChunkList.mpHead;
    if (pScan != NULL)
    {
        pChunkPrev = NULL;
        do {
            pScanChunk = K2_GET_CONTAINER(K2RAMHEAP_CHUNK, pScan, ChunkListLink);
            if (pScanChunk->mStart > aChunkBase)
                break;
            pChunkPrev = pScanChunk;
            pScan = pScan->mpNext;
        } while (pScan != NULL);
        if (pScan != NULL)
            pChunkNext = pScanChunk;
        else
            pChunkNext = NULL;
    }
    else
    {
        pChunkPrev = NULL;
        pChunkNext = NULL;
    }
    if ((pChunkPrev != NULL) && (pChunkNext != NULL))
    {
        K2_ASSERT(pChunkPrev->mpLastNodeInChunk->AddrListLink.mpNext == &pChunkNext->mpFirstNodeInChunk->AddrListLink);
        K2_ASSERT(pChunkNext->mpFirstNodeInChunk->AddrListLink.mpPrev == &pChunkPrev->mpLastNodeInChunk->AddrListLink);
    }

    pChunk = (K2RAMHEAP_CHUNK *)aChunkBase;
    pChunk->mRange = aRange;
    pChunk->mStart = aChunkBase;
    pChunk->mBreak = aChunkBase + aChunkInit;
    pChunk->mTop = aChunkBase + aChunkFull;
    pChunk->mpFirstNodeInChunk = (K2RAMHEAP_NODE *)(((UINT8 *)pChunk) + sizeof(K2RAMHEAP_CHUNK));
    pChunk->mpLastNodeInChunk = pChunk->mpFirstNodeInChunk;
    apHeap->HeapState.mTotalOverhead += sizeof(K2RAMHEAP_CHUNK) + K2RAMHEAP_NODE_OVERHEAD;
    pNode = pChunk->mpFirstNodeInChunk;
    pNode->mSentinel = K2RAMHEAP_NODE_HDRSENT_FREE;
    pNode->TreeNode.mUserVal = aChunkInit - (sizeof(K2RAMHEAP_CHUNK) + K2RAMHEAP_NODE_OVERHEAD);
    apHeap->HeapState.mTotalFree += pNode->TreeNode.mUserVal;
    pEnd = (UINT_PTR *)(((UINT8 *)pNode) + sizeof(K2RAMHEAP_NODE) + pNode->TreeNode.mUserVal);
    *pEnd = K2RAMHEAP_NODE_ENDSENT;
    K2_ASSERT(((UINT_PTR)(pEnd + 1)) == pChunk->mBreak);

    K2TREE_Insert(&apHeap->FreeTree, pNode->TreeNode.mUserVal, &pNode->TreeNode);

    if (pChunkNext == NULL)
    {
        K2LIST_AddAtTail(&apHeap->ChunkList, &pChunk->ChunkListLink);
        K2LIST_AddAtTail(&apHeap->AddrList, &pNode->AddrListLink);
    }
    else
    {
        K2LIST_AddBefore(&apHeap->ChunkList, &pChunk->ChunkListLink, &pChunkNext->ChunkListLink);
        K2LIST_AddBefore(&apHeap->AddrList, &pNode->AddrListLink, &pChunkNext->mpFirstNodeInChunk->AddrListLink);
    }

    sAllocFromNode(apHeap, pNode, aByteCount, apRetAllocAddr);

    sCheckLockedHeap(apHeap);
}

K2STAT
K2RAMHEAP_AddAndAllocFromChunk(
    K2RAMHEAP * apHeap,
    void *      aRange,
    UINT_PTR    aChunkBase,
    UINT_PTR    aChunkInit,
    UINT_PTR    aChunkFull,
    UINT_PTR    aByteCount,
    UINT_PTR *  apRetAllocAddr
)
{
    UINT_PTR  disp;

    if ((apHeap == NULL) || 
        (aByteCount == 0) || 
        (aByteCount & (sizeof(UINT_PTR) - 1)) || 
        (apRetAllocAddr == NULL) ||
        (aChunkBase & K2_VA_MEMPAGE_OFFSET_MASK) ||
        (aChunkInit & K2_VA_MEMPAGE_OFFSET_MASK) ||
        (aChunkFull & K2_VA_MEMPAGE_OFFSET_MASK) ||
        (aChunkFull < K2RAMHEAP_CHUNK_MIN) ||
        (aChunkInit > aChunkFull) || 
        (aChunkBase + aChunkFull < aChunkBase))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    disp = apHeap->Supp.fLock(apHeap);

    iAddAndAllocFromChunk(apHeap, aRange, aChunkBase, aChunkInit, aChunkFull, aByteCount, apRetAllocAddr);

    apHeap->Supp.fUnlock(apHeap, disp);

    return K2STAT_NO_ERROR;
}

K2_STATIC
K2STAT
sNewHeapChunk(
    K2RAMHEAP * apHeap,
    UINT_PTR    aByteCount,
    UINT_PTR *  apRetAllocAddr
)
{
    UINT_PTR    heapChunkInit;
    UINT_PTR    heapChunkBytes;
    UINT_PTR    chunkBase;
    K2STAT      stat;
    void *      chunkRange;

    heapChunkInit = aByteCount + K2RAMHEAP_NODE_OVERHEAD + sizeof(K2RAMHEAP_CHUNK);
    heapChunkInit = K2_ROUNDUP(heapChunkInit, K2_VA_MEMPAGE_BYTES);

    heapChunkBytes = heapChunkInit;
    if (heapChunkBytes < K2RAMHEAP_CHUNK_MIN)
        heapChunkBytes = K2RAMHEAP_CHUNK_MIN;

    stat = apHeap->Supp.fCreateRange(
        heapChunkBytes / K2_VA_MEMPAGE_BYTES, 
        heapChunkInit / K2_VA_MEMPAGE_BYTES, 
        &chunkRange,
        &chunkBase);
    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    sCheckLockedHeap(apHeap);

    iAddAndAllocFromChunk(apHeap, chunkRange, chunkBase, heapChunkInit, heapChunkBytes, aByteCount, apRetAllocAddr);

    sCheckLockedHeap(apHeap);

    return K2STAT_NO_ERROR;
}

K2_STATIC
K2STAT
sGrowHeap(
    K2RAMHEAP * apHeap,
    UINT_PTR    aByteCount,
    UINT_PTR *  apRetAllocAddr
)
{
    K2LIST_LINK *       pLink;
    K2RAMHEAP_CHUNK *   pChunk;
    UINT_PTR            space;

    pLink = apHeap->ChunkList.mpHead;
    
    if (pLink != NULL)
    {
        do {
            pChunk = K2_GET_CONTAINER(K2RAMHEAP_CHUNK, pLink, ChunkListLink);
            space = pChunk->mTop - pChunk->mBreak;
            if (space >= (aByteCount + K2RAMHEAP_NODE_OVERHEAD))
                break;
            pLink = pChunk->ChunkListLink.mpNext;
        } while (pLink != NULL);

        if (pLink != NULL)
        {
            return sGrowHeapChunk(apHeap, pChunk, aByteCount, apRetAllocAddr);
        }
    }

    return sNewHeapChunk(apHeap, aByteCount, apRetAllocAddr);
}

K2STAT 
K2RAMHEAP_Alloc(
    K2RAMHEAP * apHeap, 
    UINT_PTR    aByteCount,
    BOOL        aAllowExpansion,
    void **     appRetPtr
)
{
    K2TREE_NODE *   pTreeNode;
    K2STAT          stat;
#if DBG_ALLOC_PATTERN
    char            ascBuf[40];

    K2ASC_Printf(ascBuf, "+ALOC(%d)\n", aByteCount);
    K2DebugPrint(ascBuf);
#endif

    *appRetPtr = NULL;

    aByteCount = K2_ROUNDUP(aByteCount, sizeof(UINT_PTR));
    if (aByteCount == 0)
        return K2STAT_OK;

    stat = K2STAT_ERROR_UNKNOWN;

    sLockHeap(apHeap);

    pTreeNode = K2TREE_FindOrAfter(&apHeap->FreeTree, aByteCount);
    if (pTreeNode != NULL)
    {
        sAllocFromNode(apHeap, K2_GET_CONTAINER(K2RAMHEAP_NODE, pTreeNode, TreeNode), aByteCount, (UINT_PTR *)appRetPtr);
        stat = K2STAT_NO_ERROR;
    }
    else if (aAllowExpansion)
    {
        stat = sGrowHeap(apHeap, aByteCount, (UINT_PTR *)appRetPtr);
    }
    else
    {
        stat = K2STAT_ERROR_OUT_OF_MEMORY;
    }

    sCheckLockedHeap(apHeap);

    sUnlockHeap(apHeap);

#if DBG_ALLOC_PATTERN
    K2ASC_Printf(ascBuf, "-ALOC(%08X)\n", *appRetPtr);
    K2DebugPrint(ascBuf);
#endif

    return stat;
}

K2_STATIC
void
sFreeNode(
    K2RAMHEAP *         apHeap,
    K2RAMHEAP_CHUNK *   apChunk,
    K2RAMHEAP_NODE *    apNode
)
{
    K2LIST_LINK *       pLink;
    K2RAMHEAP_NODE *    pPrev;
    K2RAMHEAP_NODE *    pNext;
    UINT_PTR *          pEnd;
    UINT_PTR            chunkBytes;
    UINT_PTR            chunkStart;
    void *              chunkRange;
    K2STAT              stat;

    K2_ASSERT(apNode->mSentinel == K2RAMHEAP_NODE_HDRSENT_USED);

    //
    // remove apNode from tree and from tracking, but not from addr list
    //
    K2TREE_Remove(&apHeap->UsedTree, &apNode->TreeNode);
    apNode->mSentinel = 0;
    apHeap->HeapState.mAllocCount--;
    apHeap->HeapState.mTotalAlloc -= apNode->TreeNode.mUserVal;
    apHeap->HeapState.mTotalOverhead -= K2RAMHEAP_NODE_OVERHEAD;

    //
    // check for merge with previous node in same chunk
    //
    if (apNode != apChunk->mpFirstNodeInChunk)
    {
        pLink = apNode->AddrListLink.mpPrev;
        pPrev = (pLink != NULL) ? K2_GET_CONTAINER(K2RAMHEAP_NODE, pLink, AddrListLink) : NULL;
        if ((pPrev != NULL) && (pPrev->mSentinel == K2RAMHEAP_NODE_HDRSENT_FREE))
         {
            //
            // clear pPrev end sentinel
            //
            pEnd = (UINT_PTR *)(((UINT8 *)pPrev) + sizeof(K2RAMHEAP_NODE) + pPrev->TreeNode.mUserVal);
            K2_ASSERT(*pEnd == K2RAMHEAP_NODE_ENDSENT);
            *pEnd = 0;
            pEnd++;
            K2_ASSERT((UINT_PTR)pEnd == (UINT_PTR)apNode);

            //
            // resize pPrev
            //
            K2TREE_Remove(&apHeap->FreeTree, &pPrev->TreeNode);
            pPrev->mSentinel = 0;
            K2LIST_Remove(&apHeap->AddrList, &apNode->AddrListLink);
            apHeap->HeapState.mTotalFree -= pPrev->TreeNode.mUserVal;
            apHeap->HeapState.mTotalOverhead -= K2RAMHEAP_NODE_OVERHEAD;
            pPrev->TreeNode.mUserVal += K2RAMHEAP_NODE_OVERHEAD + apNode->TreeNode.mUserVal;

            if (apNode == apChunk->mpLastNodeInChunk)
                apChunk->mpLastNodeInChunk = pPrev;

            //
            // switch so apNode is now pPrev
            //
            apNode = pPrev;
            sCheckEndSent(apNode);
        }
    }

    //
    // check for merge with next node in same chunk
    //
    if (apNode != apChunk->mpLastNodeInChunk)
    {
        pLink = apNode->AddrListLink.mpNext;
        pNext = (pLink != NULL) ? K2_GET_CONTAINER(K2RAMHEAP_NODE, pLink, AddrListLink) : NULL;
        if ((pNext != NULL) && (pNext->mSentinel == K2RAMHEAP_NODE_HDRSENT_FREE))
        {
            //
            // clear apNode end sentinel
            //
            pEnd = (UINT_PTR *)(((UINT8 *)apNode) + sizeof(K2RAMHEAP_NODE) + apNode->TreeNode.mUserVal);
            K2_ASSERT(*pEnd == K2RAMHEAP_NODE_ENDSENT);
            *pEnd = 0;
            pEnd++;
            K2_ASSERT((UINT_PTR)pEnd == (UINT_PTR)pNext);

            //
            // resize apNode and remove pNext
            //
            K2TREE_Remove(&apHeap->FreeTree, &pNext->TreeNode);
            pNext->mSentinel = 0;
            K2LIST_Remove(&apHeap->AddrList, &pNext->AddrListLink);
            apHeap->HeapState.mTotalFree -= pNext->TreeNode.mUserVal;
            apHeap->HeapState.mTotalOverhead -= K2RAMHEAP_NODE_OVERHEAD;
            apNode->TreeNode.mUserVal += K2RAMHEAP_NODE_OVERHEAD + pNext->TreeNode.mUserVal;

            if (pNext == apChunk->mpLastNodeInChunk)
                apChunk->mpLastNodeInChunk = apNode;

            sCheckEndSent(apNode);
        }
    }

    //
    // insert node into free tree
    //
    apNode->mSentinel = K2RAMHEAP_NODE_HDRSENT_FREE;
    K2TREE_Insert(&apHeap->FreeTree, apNode->TreeNode.mUserVal, &apNode->TreeNode);
    apHeap->HeapState.mTotalFree += apNode->TreeNode.mUserVal;
    apHeap->HeapState.mTotalOverhead += K2RAMHEAP_NODE_OVERHEAD;

    //
    // now see if entire chunk is free
    //
    if ((NULL != apHeap->Supp.fDecommitPages) &&
        (NULL != apHeap->Supp.fDestroyRange) &&
        (apChunk->mpFirstNodeInChunk == apChunk->mpLastNodeInChunk))
    {
        K2_ASSERT(apChunk->mpFirstNodeInChunk->mSentinel == K2RAMHEAP_NODE_HDRSENT_FREE);

        //
        // remove entire chunk from heap and release memory back to the system
        //
        chunkRange = apChunk->mRange;
        chunkStart = apChunk->mStart;
        chunkBytes = apChunk->mBreak - apChunk->mStart;
        pNext = apChunk->mpFirstNodeInChunk;
        apHeap->HeapState.mTotalOverhead -= sizeof(K2RAMHEAP_CHUNK) + K2RAMHEAP_NODE_OVERHEAD;
        apHeap->HeapState.mTotalFree -= pNext->TreeNode.mUserVal;
        K2TREE_Remove(&apHeap->FreeTree, &pNext->TreeNode);
        K2LIST_Remove(&apHeap->AddrList, &pNext->AddrListLink);
        K2LIST_Remove(&apHeap->ChunkList, &apChunk->ChunkListLink);
        K2MEM_Zero(apChunk, sizeof(K2RAMHEAP_CHUNK) + sizeof(K2RAMHEAP_NODE));

        stat = apHeap->Supp.fDecommitPages(chunkRange, chunkStart, chunkBytes / K2_VA_MEMPAGE_BYTES);
        // cant do anything about failure here except stop if we are in debug config
        K2_ASSERT(!K2STAT_IS_ERROR(stat));

        stat = apHeap->Supp.fDestroyRange(chunkRange);
        // cant do anything about failure here except stop if we are in debug config
        K2_ASSERT(!K2STAT_IS_ERROR(stat));
    }
}

K2STAT 
K2RAMHEAP_Free(
    K2RAMHEAP * apHeap,
    void *      aPtr
)
{
    K2TREE_NODE *       pTreeNode;
    K2RAMHEAP_CHUNK *   pChunk;
    K2STAT              stat;
#if DBG_ALLOC_PATTERN
    char                ascBuf[40];

    K2ASC_Printf(ascBuf, "_FREE(%08X)\n", aPtr);
    K2DebugPrint(ascBuf);
#endif

    sLockHeap(apHeap);

    sCheckLockedHeap(apHeap);

    pTreeNode = K2TREE_Find(&apHeap->UsedTree, (UINT_PTR)aPtr);
    if (pTreeNode != NULL)
    {
        pChunk = sScanForAddrChunk(apHeap, (UINT_PTR)aPtr);
        K2_ASSERT(pChunk != NULL);
        K2MEM_Set(aPtr, 0xA5, pTreeNode->mUserVal);
        sFreeNode(apHeap, pChunk, K2_GET_CONTAINER(K2RAMHEAP_NODE, pTreeNode, TreeNode));
        sCheckLockedHeap(apHeap);
        stat = K2STAT_OK;
    }
    else
        stat = K2STAT_ERROR_NOT_FOUND;

    sUnlockHeap(apHeap);
    
    return stat;
}

K2STAT 
K2RAMHEAP_GetState(
    K2RAMHEAP *         apHeap,
    K2RAMHEAP_STATE *   apRetState,
    UINT_PTR *          apRetLargestFree
)
{
    K2TREE_NODE *pTreeNode;

    sLockHeap(apHeap);

    if (apRetState != NULL)
    {
        K2MEM_Copy(apRetState, &apHeap->HeapState, sizeof(K2RAMHEAP_STATE));
    }

    if (apRetLargestFree != NULL)
    {
        pTreeNode = K2TREE_LastNode(&apHeap->FreeTree);
        if (pTreeNode == NULL)
            *apRetLargestFree = 0;
        else
            *apRetLargestFree = pTreeNode->mUserVal;
    }

    sUnlockHeap(apHeap);

    return K2STAT_OK;
}

