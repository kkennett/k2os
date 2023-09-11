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
#include "crtuser.h"

static K2TREE_ANCHOR    sgSegTree;
static K2OS_CRITSEC     sgSegTreeSec;

typedef struct _CRT_SEGMENT CRT_SEGMENT;
struct _CRT_SEGMENT
{
    K2TREE_NODE             TreeNode;
    UINT32                  mPageCount;
    K2OS_PAGEARRAY_TOKEN    mPageArrayToken;
    K2OS_VIRTMAP_TOKEN      mVirtMapToken;
    BOOL                    mIsStack;
};

void    
CrtMem_Init(
    void
)
{
    BOOL ok;

    CrtHeap_Init();

    ok = K2OS_CritSec_Init(&sgSegTreeSec);
    K2_ASSERT(ok);
    if (!ok)
    {
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    K2TREE_Init(&sgSegTree, NULL);
}

UINT32
CrtMem_CreateRwSegment(
    UINT32 aPageCount
)
{
    K2OS_PAGEARRAY_TOKEN    tokPageArray;
    K2OS_VIRTMAP_TOKEN      tokVirtMap;
    UINT32                  virtAddr;
    BOOL                    ok;
    CRT_SEGMENT *           pSeg;

    if (0 == aPageCount)
        return 0;

    pSeg = K2OS_Heap_Alloc(sizeof(CRT_SEGMENT));
    if (NULL == pSeg)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return 0;
    }

    virtAddr = 0;

    do
    {
        tokPageArray = K2OS_PageArray_Create(aPageCount);
        if (NULL == tokPageArray)
            break;

        do
        {
            virtAddr = K2OS_Virt_Reserve(aPageCount);
            if (0 == virtAddr)
                break;

            tokVirtMap = K2OS_VirtMap_Create(
                tokPageArray,
                0,
                aPageCount,
                virtAddr,
                K2OS_MapType_Data_ReadWrite
            );

            if (tokVirtMap == NULL)
            {
                K2OS_Virt_Release(virtAddr);
                virtAddr = 0;
                break;
            }

            pSeg->mPageArrayToken = tokPageArray;
            pSeg->mVirtMapToken = tokVirtMap;
            pSeg->mPageCount = aPageCount;
            pSeg->mIsStack = FALSE;
            pSeg->TreeNode.mUserVal = virtAddr;
            K2OS_CritSec_Enter(&sgSegTreeSec);
            K2TREE_Insert(&sgSegTree, pSeg->TreeNode.mUserVal, &pSeg->TreeNode);
            K2OS_CritSec_Leave(&sgSegTreeSec);

        } while (0);

        if (0 == virtAddr)
        {
            ok = K2OS_Token_Destroy(tokPageArray);
            K2_ASSERT(ok);
        }

    } while (0);

    return virtAddr;
}

UINT32
CrtMem_CreateStackSegment(
    UINT32 aPageCount
)
{
    K2OS_PAGEARRAY_TOKEN    tokPageArray;
    K2OS_VIRTMAP_TOKEN      tokVirtMap;
    UINT32                  virtAddr;
    BOOL                    ok;
    CRT_SEGMENT *           pSeg;

    if (0 == aPageCount)
        return 0;

    pSeg = K2OS_Heap_Alloc(sizeof(CRT_SEGMENT));
    if (NULL == pSeg)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return 0;
    }

    virtAddr = 0;

    do
    {
        tokPageArray = K2OS_PageArray_Create(aPageCount);
        if (NULL == tokPageArray)
            break;

        do
        {
            virtAddr = K2OS_Virt_Reserve(aPageCount + 1);
            if (0 == virtAddr)
                break;

            tokVirtMap = K2OS_VirtMap_Create(
                tokPageArray,
                0,
                aPageCount,
                virtAddr + K2_VA_MEMPAGE_BYTES,
                K2OS_MapType_Thread_Stack
            );

            if (tokVirtMap == NULL)
            {
                K2OS_Virt_Release(virtAddr);
                virtAddr = 0;
                break;
            }

            pSeg->mPageArrayToken = tokPageArray;
            pSeg->mVirtMapToken = tokVirtMap;
            pSeg->mPageCount = aPageCount;
            pSeg->mIsStack = TRUE;
            pSeg->TreeNode.mUserVal = virtAddr + K2_VA_MEMPAGE_BYTES;
            K2OS_CritSec_Enter(&sgSegTreeSec);
            K2TREE_Insert(&sgSegTree, pSeg->TreeNode.mUserVal, &pSeg->TreeNode);
            K2OS_CritSec_Leave(&sgSegTreeSec);

        } while (0);

        if (0 == virtAddr)
        {
            ok = K2OS_Token_Destroy(tokPageArray);
            K2_ASSERT(ok);
        }

    } while (0);

    if (0 == virtAddr)
        return 0;

    return virtAddr + K2_VA_MEMPAGE_BYTES;
}

BOOL
CrtMem_FreeSegment(
    UINT32  aSegAddr,
    BOOL    aNoVirtFree
)
{
    K2TREE_NODE *   pTreeNode;
    CRT_SEGMENT *   pSeg;
    BOOL            ok;

    if (0 != (aSegAddr & K2_VA_MEMPAGE_OFFSET_MASK))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2OS_CritSec_Enter(&sgSegTreeSec);

    pTreeNode = K2TREE_Find(&sgSegTree, aSegAddr);
    if (NULL != pTreeNode)
    {
        pSeg = K2_GET_CONTAINER(CRT_SEGMENT, pTreeNode, TreeNode);
        if (NULL != pSeg->mVirtMapToken)
            K2TREE_Remove(&sgSegTree, &pSeg->TreeNode);
    }
    else
    {
        pSeg = NULL;
    }

    K2OS_CritSec_Leave(&sgSegTreeSec);

    if (NULL == pSeg)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return FALSE;
    }

    if (NULL == pSeg->mVirtMapToken)
    {
        //
        // in the middle of being remapped by another thread
        //
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_IN_USE);
        return FALSE;
    }

    //
    // this should destroy the map and the pagearray backing it
    //
    ok = K2OS_Token_Destroy(pSeg->mVirtMapToken);
    K2_ASSERT(ok);

    ok = K2OS_Token_Destroy(pSeg->mPageArrayToken);
    K2_ASSERT(ok);

    if (!aNoVirtFree)
    {
        if (pSeg->mIsStack)
            aSegAddr -= K2_VA_MEMPAGE_BYTES;
        ok = K2OS_Virt_Release(aSegAddr);
        K2_ASSERT(ok);
    }

    K2OS_Heap_Free(pSeg);

    return TRUE;
}

BOOL    
CrtMem_RemapSegment(
    UINT32                  aSegAddr,
    K2OS_VirtToPhys_MapType aNewMapType
)
{
    K2OS_VIRTMAP_TOKEN  tokVirtMap;
    K2TREE_NODE *       pTreeNode;
    CRT_SEGMENT *       pSeg;
    BOOL                ok;

    if (0 != (aSegAddr & K2_VA_MEMPAGE_OFFSET_MASK))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    K2OS_CritSec_Enter(&sgSegTreeSec);

    pTreeNode = K2TREE_Find(&sgSegTree, aSegAddr);
    if (NULL != pTreeNode)
    {
        pSeg = K2_GET_CONTAINER(CRT_SEGMENT, pTreeNode, TreeNode);
        tokVirtMap = pSeg->mVirtMapToken;
        pSeg->mVirtMapToken = NULL;   // mark as being remapped but do not remove from seg tree
    }
    else
    {
        pSeg = NULL;
    }

    K2OS_CritSec_Leave(&sgSegTreeSec);

    if (NULL == pSeg)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return FALSE;
    }

    //
    // this should destroy the existing map
    //
    ok = K2OS_Token_Destroy(tokVirtMap);
    K2_ASSERT(ok);

    tokVirtMap = K2OS_VirtMap_Create(
        pSeg->mPageArrayToken,
        0,
        pSeg->mPageCount,
        pSeg->TreeNode.mUserVal,
        aNewMapType
    );

    //
    // if this fails it is unrecoverable. somebody has messed with the
    // virtual address space in the middle of the remap
    //
    K2_ASSERT(NULL != tokVirtMap);

    //
    // dont need to lock sec to set this
    //
    pSeg->mVirtMapToken = tokVirtMap;
    K2_CpuWriteBarrier();

    return TRUE;
}
