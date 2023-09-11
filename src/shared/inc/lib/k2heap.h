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
#ifndef __K2HEAP_H
#define __K2HEAP_H

#include <k2systype.h>
#include "k2tree.h"

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _K2HEAP_NODE K2HEAP_NODE;
struct _K2HEAP_NODE
{
    K2TREE_NODE AddrTreeNode;
    K2TREE_NODE SizeTreeNode;
};

//
// Addr Tree bit is used for free/used
// Size tree node is available for use
//
#define K2HEAP_NODE_SETUSERBIT(pNode)   K2TREE_NODE_SETUSERBIT(&pNode->SizeTreeNode)
#define K2HEAP_NODE_CLRUSERBIT(pNode)   K2TREE_NODE_CLRUSERBIT(&pNode->SizeTreeNode)
#define K2HEAP_NODE_USERBIT(pNode)      K2TREE_NODE_USERBIT(&pNode->SizeTreeNode)   

typedef struct _K2HEAP_ANCHOR K2HEAP_ANCHOR;

typedef K2HEAP_NODE *   (*pf_K2HEAP_AcquireNode)(K2HEAP_ANCHOR *apHeap);
typedef void            (*pf_K2HEAP_ReleaseNode)(K2HEAP_ANCHOR *apHeap, K2HEAP_NODE *apNode);
typedef void            (*pf_K2HEAP_DumpNode)(K2HEAP_ANCHOR *apHeap, K2HEAP_NODE *apNode);

struct _K2HEAP_ANCHOR
{
    pf_K2HEAP_AcquireNode   mfAcquireNode;
    pf_K2HEAP_ReleaseNode   mfReleaseNode;
    K2TREE_ANCHOR           AddrTree;
    K2TREE_ANCHOR           SizeTree;
};

/* ------------------------------------------------------------------------- */

void 
K2HEAP_Init(
    K2HEAP_ANCHOR *         apHeap,
    pf_K2HEAP_AcquireNode   afAcquireNode,
    pf_K2HEAP_ReleaseNode   afReleaseNode
    );

void
K2HEAP_Dump(
    K2HEAP_ANCHOR *     apHeap,
    pf_K2HEAP_DumpNode  afDumpNode
    );

static
K2_INLINE
UINT_PTR
K2HEAP_NodeSize(
    K2HEAP_NODE *apNode
    )
{
    K2_ASSERT(apNode != NULL);
    return apNode->SizeTreeNode.mUserVal;
}

static
K2_INLINE
UINT_PTR
K2HEAP_NodeAddr(
    K2HEAP_NODE *apNode
    )
{
    K2_ASSERT(apNode != NULL);
    return apNode->AddrTreeNode.mUserVal;
}

static
K2_INLINE
BOOL
K2HEAP_NodeIsFree(
    K2HEAP_NODE *apNode
    )
{
    K2_ASSERT(apNode != NULL);
    return K2TREE_NODE_USERBIT(&apNode->AddrTreeNode) ? TRUE : FALSE;
}

static
K2_INLINE
K2HEAP_NODE *
K2HEAP_GetFirstNode(
    K2HEAP_ANCHOR * apHeap
    )
{
    K2TREE_NODE *pTreeNode;

    K2_ASSERT(apHeap != NULL);
    pTreeNode = K2TREE_FirstNode(&apHeap->AddrTree);
    if (pTreeNode == NULL)
        return NULL;
    return K2_GET_CONTAINER(K2HEAP_NODE, pTreeNode, AddrTreeNode);
}

static
K2_INLINE
K2HEAP_NODE *
K2HEAP_GetLastNode(
    K2HEAP_ANCHOR * apHeap
    )
{
    K2TREE_NODE *pTreeNode;

    K2_ASSERT(apHeap != NULL);
    pTreeNode = K2TREE_LastNode(&apHeap->AddrTree);
    if (pTreeNode == NULL)
        return NULL;
    return K2_GET_CONTAINER(K2HEAP_NODE, pTreeNode, AddrTreeNode);
}

static
K2_INLINE
K2HEAP_NODE *
K2HEAP_GetPrevNode(
    K2HEAP_ANCHOR * apHeap,
    K2HEAP_NODE *   apNode
    )
{
    K2TREE_NODE *pTreeNode;

    K2_ASSERT(apHeap != NULL);
    K2_ASSERT(apNode != NULL);
    pTreeNode = K2TREE_PrevNode(&apHeap->AddrTree, &apNode->AddrTreeNode);
    if (pTreeNode == NULL)
        return NULL;
    return K2_GET_CONTAINER(K2HEAP_NODE, pTreeNode, AddrTreeNode);
}

static
K2_INLINE
K2HEAP_NODE *
K2HEAP_GetNextNode(
    K2HEAP_ANCHOR * apHeap,
    K2HEAP_NODE *   apNode
    )
{
    K2TREE_NODE *pTreeNode;

    K2_ASSERT(apHeap != NULL);
    K2_ASSERT(apNode != NULL);
    pTreeNode = K2TREE_NextNode(&apHeap->AddrTree, &apNode->AddrTreeNode);
    if (pTreeNode == NULL)
        return NULL;
    return K2_GET_CONTAINER(K2HEAP_NODE, pTreeNode, AddrTreeNode);
}

K2HEAP_NODE *
K2HEAP_FindNodeContainingAddr(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aAddr
    );

K2HEAP_NODE *
K2HEAP_GetFirstNodeInRange(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aStart,
    UINT_PTR        aSize
    );

K2STAT
K2HEAP_AllocNodeAt(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aAddr,
    UINT_PTR        aSize,
    K2HEAP_NODE **  apRetNode
    );

K2STAT
K2HEAP_AllocNodeLowest(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aSize,
    UINT_PTR        aAlign,
    K2HEAP_NODE **  apRetNode
    );

K2STAT
K2HEAP_AllocNodeHighest(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aSize,
    UINT_PTR        aAlign,
    K2HEAP_NODE **  apRetNode
    );

K2STAT
K2HEAP_AllocNodeBest(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aSize,
    UINT_PTR        aAlign,
    K2HEAP_NODE **  apRetNode
    );

K2STAT
K2HEAP_AddFreeSpaceNode(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aStart,
    UINT_PTR        aSize,
    K2HEAP_NODE **  apRetNode
    );

K2STAT
K2HEAP_FreeNode(
    K2HEAP_ANCHOR * apHeap,
    K2HEAP_NODE *   apNode
    );

void
K2HEAP_MergeTags(
    K2HEAP_ANCHOR * apHeap
    );

/* ------------------------------------------------------------------------- */

static
K2_INLINE
K2STAT
K2HEAP_AllocAt(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aAddr,
    UINT_PTR        aSize
    )
{
    return K2HEAP_AllocNodeAt(apHeap, aAddr, aSize, NULL);
}

static
K2_INLINE
UINT_PTR
K2HEAP_AllocAlignedLowest(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aSize,
    UINT_PTR        aAlign
    )
{
    K2STAT status;
    K2HEAP_NODE * pHeapNode;

    status = K2HEAP_AllocNodeLowest(apHeap, aSize, aAlign, &pHeapNode);
    if (status == K2STAT_OK)
        return K2HEAP_NodeAddr(pHeapNode);
    K2_ASSERT(status == K2STAT_ERROR_OUT_OF_MEMORY);
    return 0;
}

static
K2_INLINE
UINT_PTR
K2HEAP_AllocAlignedHighest(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aSize,
    UINT_PTR        aAlign
    )
{
    K2STAT status;
    K2HEAP_NODE * pHeapNode;

    status = K2HEAP_AllocNodeHighest(apHeap, aSize, aAlign, &pHeapNode);
    if (status == K2STAT_OK)
        return K2HEAP_NodeAddr(pHeapNode);
    K2_ASSERT(status == K2STAT_ERROR_OUT_OF_MEMORY);
    return 0;
}

static
K2_INLINE
UINT_PTR
K2HEAP_AllocAligned(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aSize,
    UINT_PTR        aAlign
    )
{
    K2STAT status;
    K2HEAP_NODE * pHeapNode;

    status = K2HEAP_AllocNodeBest(apHeap, aSize, aAlign, &pHeapNode);
    if (status == K2STAT_OK)
        return K2HEAP_NodeAddr(pHeapNode);
    K2_ASSERT(status == K2STAT_ERROR_OUT_OF_MEMORY);
    return 0;
}

static
K2_INLINE
UINT_PTR
K2HEAP_AllocLowest(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aSize
    )
{
    return K2HEAP_AllocAlignedLowest(apHeap, aSize, 0);
}

static
K2_INLINE
UINT_PTR
K2HEAP_AllocHighest(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aSize
    )
{
    return K2HEAP_AllocAlignedHighest(apHeap, aSize, 0);
}

static
K2_INLINE
UINT_PTR
K2HEAP_Alloc(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aSize
    )
{
    return K2HEAP_AllocAligned(apHeap, aSize, 0);
}

static
K2_INLINE
BOOL
K2HEAP_Free(
    K2HEAP_ANCHOR * apHeap,
    UINT_PTR        aAddr
    )
{
    K2HEAP_NODE *pHeapNode = K2HEAP_FindNodeContainingAddr(apHeap, aAddr);
    if (pHeapNode == NULL)
        return FALSE;
    if (K2HEAP_NodeAddr(pHeapNode) != aAddr)
        return FALSE;
    if (K2STAT_IS_ERROR(K2HEAP_FreeNode(apHeap, pHeapNode)))
        return FALSE;
    return TRUE;
}

#ifdef __cplusplus
};  // extern "C"
#endif

/* ------------------------------------------------------------------------- */

#endif  // __K2_PUBLIC_K2HEAP_H

