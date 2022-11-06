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
#ifndef __K2TREE_H
#define __K2TREE_H

#include <k2systype.h>

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _K2TREE_NODE K2TREE_NODE;
struct _K2TREE_NODE
{
    UINT_PTR        mUserVal;       // ususally the key so comparisons will work
    UINT_PTR        mParentBal;     // bit 1 is user bit, bit 0 is balance bit (black==0,red==1)
    K2TREE_NODE *   mpLeftChild;
    K2TREE_NODE *   mpRightChild;
};

typedef
int
(*K2TREE_pfCompareKeyToNode)(
    UINT_PTR        aKey,
    K2TREE_NODE *   apNode
    );

#define K2TREE_NODE_PARENT(pNode)             ((K2TREE_NODE *)((pNode)->mParentBal & ~3))
#define K2TREE_NODE_SETPARENT(pNode, pParent) (pNode)->mParentBal = ((pNode)->mParentBal & 3) | (((UINT_PTR)pParent) & ~3)

#define K2TREE_NODE_ISRED(pNode)              ((((pNode)->mParentBal & 1)!=0)?TRUE:FALSE)
#define K2TREE_NODE_SETRED(pNode)             (pNode)->mParentBal = ((pNode)->mParentBal | 1)
#define K2TREE_NODE_SETBLACK(pNode)           (pNode)->mParentBal &= ~1
#define K2TREE_NODE_COPYCOLOR(pNode, pOther)  (pNode)->mParentBal = ((pNode)->mParentBal & ~1) | ((pOther)->mParentBal & 1)

#define K2TREE_NODE_SETUSERBIT(pNode)         (pNode)->mParentBal |= 2
#define K2TREE_NODE_CLRUSERBIT(pNode)         (pNode)->mParentBal &= ~2
#define K2TREE_NODE_USERBIT(pNode)            ((((pNode)->mParentBal & 2)!=0)?TRUE:FALSE)

typedef struct _K2TREE_ANCHOR K2TREE_ANCHOR;
struct _K2TREE_ANCHOR
{
    K2TREE_NODE                 RootNode;
    K2TREE_NODE                 NilNode;
    UINT_PTR                    mNodeCount;
    K2TREE_pfCompareKeyToNode   mfCompareKeyToNode;
};

void
K2TREE_Init(
    K2TREE_ANCHOR *           apAnchor,
    K2TREE_pfCompareKeyToNode afCompare
    );

static
K2_INLINE
BOOL
K2TREE_IsEmpty(
    K2TREE_ANCHOR * apAnchor
    )
{
    K2_ASSERT(apAnchor != NULL);
    return (apAnchor->mNodeCount == 0);
}

void       
K2TREE_Insert(
    K2TREE_ANCHOR * apAnchor,
    UINT_PTR        aInsertionKey,
    K2TREE_NODE *   apNode
    );

void
K2TREE_Remove(
    K2TREE_ANCHOR * apAnchor,
    K2TREE_NODE *   apNode
    );

K2TREE_NODE * 
K2TREE_PrevNode(
    K2TREE_ANCHOR * apAnchor,
    K2TREE_NODE *   apNode
    );

K2TREE_NODE * 
K2TREE_NextNode(
    K2TREE_ANCHOR * apAnchor,
    K2TREE_NODE *   apNode
    );

K2TREE_NODE * 
K2TREE_FirstNode(
    K2TREE_ANCHOR * apAnchor
    );

K2TREE_NODE *
K2TREE_LastNode(
    K2TREE_ANCHOR * apAnchor
    );

K2TREE_NODE *
K2TREE_Find(
    K2TREE_ANCHOR * apAnchor,
    UINT_PTR        aFindKey
    );

K2TREE_NODE *
K2TREE_FindOrAfter(
    K2TREE_ANCHOR * apAnchor,
    UINT_PTR        aFindKey
    );

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif  // __K2TREE_H

