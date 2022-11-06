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

#ifndef __K2RAMHEAP_H
#define __K2RAMHEAP_H

/* --------------------------------------------------------------------------------- */

#include <k2systype.h>
#include <lib/k2mem.h>
#include <lib/k2tree.h>
#include <lib/k2list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _K2RAMHEAP_STATE K2RAMHEAP_STATE;
struct _K2RAMHEAP_STATE
{
    UINT_PTR  mAllocCount;
    UINT_PTR  mTotalAlloc;
    UINT_PTR  mTotalFree;
    UINT_PTR  mTotalOverhead;
};

#define K2RAMHEAP_CHUNK_MIN (64*1024)
K2_STATIC_ASSERT((K2RAMHEAP_CHUNK_MIN % K2_VA_MEMPAGE_BYTES) == 0);

#define K2RAMHEAP_NODE_HDRSENT_FREE  K2_MAKEID4('F','R','E','E')
#define K2RAMHEAP_NODE_HDRSENT_USED  K2_MAKEID4('U','S','E','D')
#define K2RAMHEAP_NODE_ENDSENT       K2_MAKEID4('S','E','N','T')

typedef struct _K2RAMHEAP_NODE K2RAMHEAP_NODE;
struct _K2RAMHEAP_NODE
{
    UINT_PTR    mSentinel;      // K2RAMHEAP_NODE_HDRSENT_xxx
    K2LIST_LINK AddrListLink;   // Always valid. Block on list sorted by address
    K2TREE_NODE TreeNode;       // Always valid. Free = Size Tree, Used = Address Tree
};

#define K2RAMHEAP_NODE_OVERHEAD  (sizeof(K2RAMHEAP_NODE) + sizeof(UINT_PTR))

K2_STATIC_ASSERT((8*sizeof(UINT_PTR)) == K2RAMHEAP_NODE_OVERHEAD);

typedef struct _K2RAMHEAP_CHUNK K2RAMHEAP_CHUNK;
struct _K2RAMHEAP_CHUNK
{
    UINT_PTR            mStart;
    UINT_PTR            mBreak;
    UINT_PTR            mTop;
    void *              mRange;
    K2RAMHEAP_NODE *    mpFirstNodeInChunk;
    K2RAMHEAP_NODE *    mpLastNodeInChunk;
    K2LIST_LINK         ChunkListLink;
};

typedef struct _K2RAMHEAP K2RAMHEAP;

typedef UINT32  (*pf_K2RAMHEAP_Lock)(K2RAMHEAP *apRamHeap);
typedef void    (*pf_K2RAMHEAP_Unlock)(K2RAMHEAP *apRamHeap, UINT_PTR aDisp);
typedef K2STAT  (*pf_K2RAMHEAP_CreateRange)(UINT_PTR aTotalPageCount, UINT_PTR aInitCommitPageCount, void **appRetRange, UINT_PTR *apRetStartAddr);
typedef K2STAT  (*pf_K2RAMHEAP_CommitPages)(void *aRange, UINT_PTR aAddr, UINT_PTR aPageCount);
typedef K2STAT  (*pf_K2RAMHEAP_DecommitPages)(void *aRange, UINT_PTR aAddr, UINT_PTR aPageCount);
typedef K2STAT  (*pf_K2RAMHEAP_DestroyRange)(void *aRange);

typedef struct _K2RAMHEAP_SUPP K2RAMHEAP_SUPP;
struct _K2RAMHEAP_SUPP
{
    pf_K2RAMHEAP_Lock            fLock;
    pf_K2RAMHEAP_Unlock          fUnlock;
    pf_K2RAMHEAP_CreateRange     fCreateRange;
    pf_K2RAMHEAP_CommitPages     fCommitPages;
    pf_K2RAMHEAP_DecommitPages   fDecommitPages;
    pf_K2RAMHEAP_DestroyRange    fDestroyRange;
};

struct _K2RAMHEAP
{
    K2LIST_ANCHOR   ChunkList;          // chunk list
    K2LIST_ANCHOR   AddrList;           // list of K2RAMHEAP_NODE.AddrListLink
    K2TREE_ANCHOR   UsedTree;           // key is effective address (&K2RAMHEAP_NODE + sizeof(K2RAMHEAP_NODE))
    K2TREE_ANCHOR   FreeTree;           // key is size (K2RAMHEAP_NODE.TreeNode.mUserVal);
    K2RAMHEAP_STATE HeapState;
    K2RAMHEAP_SUPP  Supp;
    UINT32          mLockDisp;
};

void
K2RAMHEAP_Init(
    K2RAMHEAP *             apHeap,
    K2RAMHEAP_SUPP const *  apSupp
);

K2STAT
K2RAMHEAP_Alloc(
    K2RAMHEAP * apHeap,
    UINT_PTR    aByteCount,
    BOOL        aAllowExpansion,
    void **     appRetPtr
);

K2STAT
K2RAMHEAP_Free(
    K2RAMHEAP * apHeap,
    void *      aPtr
);

K2STAT
K2RAMHEAP_GetState(
    K2RAMHEAP *         apHeap,
    K2RAMHEAP_STATE *   apRetState,
    UINT_PTR *          apRetLargestFree
);

#ifdef __cplusplus
};  // extern "C"
#endif

/* --------------------------------------------------------------------------------- */

#endif // __K2RAMHEAP_H
