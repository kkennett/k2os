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
#include <lib/k2heap.h>

static 
void
sRemoveHeapNode(
    K2HEAP_ANCHOR * apHeap,
    K2HEAP_NODE *   apNode
    )
{
    K2TREE_Remove(&apHeap->AddrTree, &apNode->AddrTreeNode);
    if (K2TREE_NODE_USERBIT(&apNode->AddrTreeNode) != 0)
        K2TREE_Remove(&apHeap->SizeTree, &apNode->SizeTreeNode);
}

static
void
sInsertHeapNode(
    K2HEAP_ANCHOR * apHeap,
    K2HEAP_NODE *   apNode
    )
{
    K2TREE_Insert(&apHeap->AddrTree, apNode->AddrTreeNode.mUserVal, &apNode->AddrTreeNode);
    if (K2TREE_NODE_USERBIT(&apNode->AddrTreeNode) != 0)
        K2TREE_Insert(&apHeap->SizeTree, apNode->SizeTreeNode.mUserVal, &apNode->SizeTreeNode);
}

static
K2STAT
sAllocFromNode(
    K2HEAP_ANCHOR * apHeap,
    K2HEAP_NODE *   apFreeNode,
    UINT_PTR        aAddr,
    UINT_PTR        aSize
    )
{
    K2HEAP_NODE *pNewPrev;
    K2HEAP_NODE *pNewNext;

    K2_ASSERT(aAddr >= K2HEAP_NodeAddr(apFreeNode));
    K2_ASSERT(aSize <= K2HEAP_NodeSize(apFreeNode));
    K2_ASSERT(K2HEAP_NodeSize(apFreeNode) - (aAddr - K2HEAP_NodeAddr(apFreeNode)) >= aSize);

    if (K2HEAP_NodeAddr(apFreeNode) < aAddr)
    {
        pNewPrev = apHeap->mfAcquireNode(apHeap);
        if (pNewPrev == NULL)
            return K2STAT_ERROR_OUT_OF_RESOURCES;
    }
    else
        pNewPrev = NULL;

    if (((aAddr - K2HEAP_NodeAddr(apFreeNode)) + aSize) < K2HEAP_NodeSize(apFreeNode))
    {
        pNewNext = apHeap->mfAcquireNode(apHeap);
        if (pNewNext == NULL)
        {
            if (pNewPrev != NULL)
                apHeap->mfReleaseNode(apHeap, pNewPrev);
            return K2STAT_ERROR_OUT_OF_RESOURCES;
        }
    }
    else
        pNewNext = NULL;

    sRemoveHeapNode(apHeap, apFreeNode);

    if (pNewPrev != NULL)
    {
        pNewPrev->SizeTreeNode.mUserVal = aAddr - K2HEAP_NodeAddr(apFreeNode);
        pNewPrev->AddrTreeNode.mUserVal = K2HEAP_NodeAddr(apFreeNode);
        K2TREE_NODE_SETUSERBIT(&pNewPrev->AddrTreeNode);

        sInsertHeapNode(apHeap, pNewPrev);

        apFreeNode->AddrTreeNode.mUserVal = aAddr;
        apFreeNode->SizeTreeNode.mUserVal -= K2HEAP_NodeSize(pNewPrev);
    }

    if (pNewNext != NULL)
    {
        pNewNext->SizeTreeNode.mUserVal = K2HEAP_NodeSize(apFreeNode) - aSize;
        pNewNext->AddrTreeNode.mUserVal = K2HEAP_NodeAddr(apFreeNode) + aSize;
        K2TREE_NODE_SETUSERBIT(&pNewNext->AddrTreeNode);

        sInsertHeapNode(apHeap, pNewNext);

        apFreeNode->SizeTreeNode.mUserVal = aSize;
    }

    K2TREE_NODE_CLRUSERBIT(&apFreeNode->AddrTreeNode);

    sInsertHeapNode(apHeap, apFreeNode);

    return K2STAT_OK;
}

/* ------------------------------------------------------------------------- */

static
int
sValueCompare(
    UINT_PTR        aKey,
    K2TREE_NODE *   apNode
)
{
    if (aKey < apNode->mUserVal)
        return -1;
    return (aKey != apNode->mUserVal);
}

void
K2HEAP_Init(
    K2HEAP_ANCHOR *         apHeap,
    pf_K2HEAP_AcquireNode   afAcquireNode,
    pf_K2HEAP_ReleaseNode   afReleaseNode
    )
{
    K2_ASSERT(apHeap != NULL);
    K2_ASSERT(afAcquireNode != NULL);
    K2_ASSERT(afReleaseNode != NULL);
    apHeap->mfAcquireNode = afAcquireNode;
    apHeap->mfReleaseNode = afReleaseNode;
    K2TREE_Init(&apHeap->AddrTree, sValueCompare);
    K2TREE_Init(&apHeap->SizeTree, sValueCompare);
}

static
K2HEAP_NODE *
sFindStartAtOrStartPrevTo(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aAddr
    )
{
    K2TREE_NODE *pTreeNode;
   
    pTreeNode = K2TREE_FindOrAfter(&apHeap->AddrTree, aAddr);
    if (pTreeNode == NULL)
    {
        //
        // no node STARTS at or after aAddr. but aAddr may still be 
        // inside of the last node
        //
        pTreeNode = K2TREE_LastNode(&apHeap->AddrTree);
        if (pTreeNode == NULL)
            return NULL;
    }
    else
    {
        //
        // if we hit the node bang on then return it
        //
        if (pTreeNode->mUserVal == aAddr)
            return K2_GET_CONTAINER(K2HEAP_NODE, pTreeNode, AddrTreeNode);

        //
        // pTreeNode STARTS strictly after aAddr, so back up
        // if we can to the previous node that will be guaranteed
        // to start before it
        //

        pTreeNode = K2TREE_PrevNode(&apHeap->AddrTree, pTreeNode);
        if (pTreeNode == NULL)
            return NULL;
    }

    //
    // pTreeNode points to the tree node that STARTS strictly before aAddr,
    // (does NOT start AT aAddr)
    //
    K2_ASSERT(pTreeNode->mUserVal < aAddr);

    return K2_GET_CONTAINER(K2HEAP_NODE, pTreeNode, AddrTreeNode);
}

K2HEAP_NODE *
K2HEAP_FindNodeContainingAddr(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aAddr
    )
{
    K2HEAP_NODE *pHeapNode;

    K2_ASSERT(apHeap != NULL);

    pHeapNode = sFindStartAtOrStartPrevTo(apHeap, aAddr);
    if (pHeapNode == NULL)
        return NULL;
    
    if ((aAddr - K2HEAP_NodeAddr(pHeapNode)) >= K2HEAP_NodeSize(pHeapNode))
    {
        // node does not contain aAddr in its span
        return NULL;
    }

    return pHeapNode;
}

K2HEAP_NODE *
K2HEAP_GetFirstNodeInRange(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aStart,
    UINT_PTR        aSize
    )
{
    K2HEAP_NODE *   pHeapNode;
    UINT_PTR        nextNodeAddr;

    K2_ASSERT(apHeap != NULL);

    if (aSize == 0)
        return NULL;

    if (aStart + aSize < aStart)
        aSize = 0 - aStart;

    pHeapNode = sFindStartAtOrStartPrevTo(apHeap, aStart);
    if (pHeapNode == NULL)
        return NULL;
    
    if ((aStart - K2HEAP_NodeAddr(pHeapNode)) < K2HEAP_NodeSize(pHeapNode))
        return pHeapNode;

    pHeapNode = K2HEAP_GetNextNode(apHeap, pHeapNode);
    if (pHeapNode == NULL)
        return NULL;

    nextNodeAddr = K2HEAP_NodeAddr(pHeapNode);
    K2_ASSERT(nextNodeAddr > aStart);
    if ((nextNodeAddr - aStart) >= aSize)
        return NULL;

    return pHeapNode;
}

K2STAT
K2HEAP_AllocNodeAt(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aAddr,
    UINT_PTR        aSize,
    K2HEAP_NODE **  apRetNode
    )
{
    K2HEAP_NODE *   pHeapNode;
    UINT_PTR        leftInNode;
    K2STAT          status;

    K2_ASSERT(apHeap != NULL);

    if (apRetNode != NULL)
        *apRetNode = NULL;
    
    if (aSize == 0)
        return K2STAT_ERROR_BAD_ARGUMENT;

    pHeapNode = K2HEAP_FindNodeContainingAddr(apHeap, aAddr);
    if (pHeapNode == NULL)
    {
        // nonexistent space
        return K2STAT_ERROR_OUT_OF_BOUNDS;
    }

    if (!K2HEAP_NodeIsFree(pHeapNode))
    {
        // inside allocated node
        return K2STAT_ERROR_IN_USE;
    }

    leftInNode = K2HEAP_NodeSize(pHeapNode) - (aAddr - K2HEAP_NodeAddr(pHeapNode));
    if (leftInNode < aSize)
    {
        // will not fit into free space
        return K2STAT_ERROR_TOO_BIG;
    }

    status = sAllocFromNode(apHeap, pHeapNode, aAddr, aSize);

    if (!K2STAT_IS_ERROR(status))
    {
        if (apRetNode != NULL)
            *apRetNode = pHeapNode;
    }

    return status;
}

K2STAT
K2HEAP_AllocNodeLowest(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aSize,
    UINT_PTR        aAlign,
    K2HEAP_NODE **  apRetNode
    )
{
    K2HEAP_NODE *   pHeapNode;
    UINT_PTR        chkAddr;
    UINT_PTR        chkEnd;
    K2STAT          status;

    K2_ASSERT(apHeap != NULL);
    K2_ASSERT(apRetNode != NULL);

    *apRetNode = NULL;
    
    if (aSize == 0)
        return K2STAT_ERROR_BAD_ARGUMENT;

    pHeapNode = K2HEAP_GetFirstNode(apHeap);
    if (pHeapNode == NULL)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    if (aAlign == 0)
        aAlign = 1;

    do
    {
        do
        {
            if ((K2TREE_NODE_USERBIT(&pHeapNode->AddrTreeNode)!=0) &&
                (K2HEAP_NodeSize(pHeapNode) >= aSize))
                break;
            pHeapNode = K2HEAP_GetNextNode(apHeap, pHeapNode);
            if (pHeapNode == NULL)
                return K2STAT_ERROR_OUT_OF_MEMORY;
        } while (pHeapNode != NULL);

        chkAddr = ((K2HEAP_NodeAddr(pHeapNode) + (aAlign - 1)) / aAlign) * aAlign;
        chkEnd = chkAddr + aSize;
        if ((chkEnd - K2HEAP_NodeAddr(pHeapNode)) <= K2HEAP_NodeSize(pHeapNode))
            break;

        pHeapNode = K2HEAP_GetNextNode(apHeap, pHeapNode);
        if (pHeapNode == NULL)
            return K2STAT_ERROR_OUT_OF_MEMORY;
    } while (1);

    status = sAllocFromNode(apHeap, pHeapNode, chkAddr, aSize);

    if (!K2STAT_IS_ERROR(status))
        *apRetNode = pHeapNode;

    return status;
}

K2STAT
K2HEAP_AllocNodeHighest(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aSize,
    UINT_PTR        aAlign,
    K2HEAP_NODE **  apRetNode
    )
{
    K2HEAP_NODE *   pHeapNode;
    UINT_PTR        chkAddr;
    K2STAT          status;

    K2_ASSERT(apHeap != NULL);
    K2_ASSERT(apRetNode != NULL);

    *apRetNode = NULL;
    
    if (aSize == 0)
        return K2STAT_ERROR_BAD_ARGUMENT;

    pHeapNode = K2HEAP_GetLastNode(apHeap);
    if (pHeapNode == NULL)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    if (aAlign == 0)
        aAlign = 1;

    do
    {
        do
        {
            if ((K2TREE_NODE_USERBIT(&pHeapNode->AddrTreeNode)!=0) &&
                (K2HEAP_NodeSize(pHeapNode) >= aSize))
                break;
            pHeapNode = K2HEAP_GetPrevNode(apHeap, pHeapNode);
            if (pHeapNode == NULL)
                return K2STAT_ERROR_OUT_OF_MEMORY;
        } while (pHeapNode != NULL);

        chkAddr = ((K2HEAP_NodeAddr(pHeapNode) + K2HEAP_NodeSize(pHeapNode) - aSize) / aAlign) * aAlign;
        if (chkAddr >= K2HEAP_NodeAddr(pHeapNode))
            break;

        pHeapNode = K2HEAP_GetPrevNode(apHeap, pHeapNode);
        if (pHeapNode == NULL)
            return K2STAT_ERROR_OUT_OF_MEMORY;
    } while (1);
            
    status = sAllocFromNode(apHeap, pHeapNode, chkAddr, aSize);

    if (!K2STAT_IS_ERROR(status))
        *apRetNode = pHeapNode;

    return status;
}

K2STAT
K2HEAP_AllocNodeBest(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aSize,
    UINT_PTR        aAlign,
    K2HEAP_NODE **  apRetNode
    )
{
    K2TREE_NODE *   pTreeNode;
    K2HEAP_NODE *   pHeapNode;
    UINT_PTR        chkAddr;
    UINT_PTR        chkEnd;
    K2STAT          status;

    K2_ASSERT(apHeap != NULL);
    K2_ASSERT(apRetNode != NULL);

    *apRetNode = NULL;
    
    if (aSize == 0)
        return K2STAT_ERROR_BAD_ARGUMENT;

    pTreeNode = K2TREE_FindOrAfter(&apHeap->SizeTree, aSize);
    if (pTreeNode == NULL)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    if (aAlign == 0)
        aAlign = 1;

    pHeapNode = K2_GET_CONTAINER(K2HEAP_NODE, pTreeNode, SizeTreeNode);
    do
    {
        chkAddr = ((K2HEAP_NodeAddr(pHeapNode) + (aAlign - 1)) / aAlign) * aAlign;
        chkEnd = chkAddr + aSize;
        if ((chkEnd - K2HEAP_NodeAddr(pHeapNode)) <= K2HEAP_NodeSize(pHeapNode))
            break;
        pTreeNode = K2TREE_NextNode(&apHeap->SizeTree, pTreeNode);
        if (pTreeNode == NULL)
            return K2STAT_ERROR_OUT_OF_MEMORY;
        pHeapNode = K2_GET_CONTAINER(K2HEAP_NODE, pTreeNode, SizeTreeNode);
    } while (1);
            
    status = sAllocFromNode(apHeap, pHeapNode, chkAddr, aSize);

    if (!K2STAT_IS_ERROR(status))
        *apRetNode = pHeapNode;

    return status;
}

K2STAT
K2HEAP_AddFreeSpaceNode(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aStart,
    UINT_PTR        aSize,
    K2HEAP_NODE **  apRetNode
    )
{
    K2TREE_NODE *   pTreeNode;
    K2HEAP_NODE *   pHeapNode;
    UINT_PTR        chk;

    K2_ASSERT(apHeap != NULL);

    if (apRetNode != NULL)
        *apRetNode = NULL;
    
    if (aSize == 0)
        return K2STAT_ERROR_BAD_ARGUMENT;

    chk = aStart + aSize;
    if ((chk < aStart) && (chk != 0))
        return K2STAT_ERROR_BAD_ARGUMENT;

    pHeapNode = K2HEAP_FindNodeContainingAddr(apHeap, aStart);
    if (pHeapNode != NULL)
        return K2STAT_ERROR_IN_USE;

    pTreeNode = K2TREE_FindOrAfter(&apHeap->AddrTree, aStart);
    if (pTreeNode != NULL)
    {
        // this node comes strictly after aStart
        if ((pTreeNode->mUserVal - aStart) < aSize)
            return K2STAT_ERROR_TOO_BIG;
    }

    pHeapNode = apHeap->mfAcquireNode(apHeap);
    if (pHeapNode == NULL)
        return K2STAT_ERROR_OUT_OF_RESOURCES;

    pHeapNode->AddrTreeNode.mUserVal = aStart;
    pHeapNode->SizeTreeNode.mUserVal = aSize;
    K2TREE_NODE_CLRUSERBIT(&pHeapNode->AddrTreeNode);
    if (apRetNode != NULL)
        *apRetNode = pHeapNode;

    sInsertHeapNode(apHeap, pHeapNode);

    return K2HEAP_FreeNode(apHeap, pHeapNode);
}

K2STAT
K2HEAP_FreeNode(
    K2HEAP_ANCHOR * apHeap,
    K2HEAP_NODE *   apNode
    )
{
    K2HEAP_NODE *pPrev;
    K2HEAP_NODE *pNext;

    K2_ASSERT(apHeap != NULL);
    
    if (K2HEAP_NodeIsFree(apNode))
        return K2STAT_ERROR_BAD_ARGUMENT;
    
    pPrev = K2HEAP_GetPrevNode(apHeap, apNode);
    if (pPrev != NULL)
    {
        if (K2HEAP_NodeAddr(pPrev) + K2HEAP_NodeSize(pPrev) != K2HEAP_NodeAddr(apNode))
            pPrev = NULL;
    }
    pNext = K2HEAP_GetNextNode(apHeap, apNode);
    if (pNext != NULL)
    {
        if (K2HEAP_NodeAddr(apNode) + K2HEAP_NodeSize(apNode) != K2HEAP_NodeAddr(pNext))
            pNext = NULL;
    }

    sRemoveHeapNode(apHeap, apNode);

    K2TREE_NODE_SETUSERBIT(&apNode->AddrTreeNode);

    if (pPrev != NULL)
    {
        if (K2HEAP_NodeIsFree(pPrev))
        {
            sRemoveHeapNode(apHeap, pPrev);
            apNode->AddrTreeNode.mUserVal = pPrev->AddrTreeNode.mUserVal;
            apNode->SizeTreeNode.mUserVal += pPrev->SizeTreeNode.mUserVal;
            apHeap->mfReleaseNode(apHeap, pPrev);
        }
    }

    if (pNext != NULL)
    {
        if (K2HEAP_NodeIsFree(pNext))
        {
            sRemoveHeapNode(apHeap, pNext);
            apNode->SizeTreeNode.mUserVal += pNext->SizeTreeNode.mUserVal;
            apHeap->mfReleaseNode(apHeap, pNext);
        }
    }

    sInsertHeapNode(apHeap, apNode);

    return K2STAT_OK;
}

void
K2HEAP_Dump(
    K2HEAP_ANCHOR *     apHeap,
    pf_K2HEAP_DumpNode  afDumpNode
    )
{
    K2HEAP_NODE *pNode;

    K2_ASSERT(apHeap != NULL);

    pNode = K2HEAP_GetFirstNode(apHeap);
    if (pNode != NULL)
    {
        do
        {
            afDumpNode(apHeap, pNode);
            pNode = K2HEAP_GetNextNode(apHeap, pNode);
        } while (pNode != NULL);
    }
}

