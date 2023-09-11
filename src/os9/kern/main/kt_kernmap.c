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

K2OS_VIRTMAP_TOKEN
K2OS_VirtMap_Create(
    K2OS_PAGEARRAY_TOKEN    aTokPageArray,
    UINT32                  aPageOffset,
    UINT32                  aPageCount,
    UINT32                  aVirtAddr,
    K2OS_VirtToPhys_MapType aMapType
)
{
    K2OSKERN_OBJREF     pageArrayRef;
    K2STAT              stat;
    K2OSKERN_OBJREF     virtMapRef;
    K2OS_VIRTMAP_TOKEN  tokVirtMap;

    if ((NULL == aTokPageArray) ||
        (0 == aPageCount) ||
        (aVirtAddr < K2OS_KVA_KERN_BASE) ||
        (0 != (aVirtAddr & K2_VA_MEMPAGE_OFFSET_MASK)) ||
        ((aVirtAddr + (aPageCount * K2_VA_MEMPAGE_BYTES)) < aVirtAddr))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    pageArrayRef.AsAny = NULL;
    stat = KernToken_Translate(aTokPageArray, &pageArrayRef);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    virtMapRef.AsAny = NULL;

    if (pageArrayRef.AsAny->mObjType != KernObj_PageArray)
    {
        stat = K2STAT_ERROR_BAD_TOKEN;
    }
    else
    {
        if ((aPageOffset >= pageArrayRef.AsPageArray->mPageCount) ||
            ((pageArrayRef.AsPageArray->mPageCount - aPageOffset) < aPageCount))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            stat = KernVirtMap_Create(
                pageArrayRef.AsPageArray,
                aPageOffset,
                aPageCount,
                aVirtAddr,
                aMapType,
                &virtMapRef
            );
        }
    }

    KernObj_ReleaseRef(&pageArrayRef);

    if (K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(NULL == virtMapRef.AsAny);
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    tokVirtMap = NULL;
    stat = KernToken_Create(virtMapRef.AsAny, &tokVirtMap);
    KernObj_ReleaseRef(&virtMapRef);

    if (K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(NULL == tokVirtMap);
        return NULL;
    }

    K2_ASSERT(NULL != tokVirtMap);
    return tokVirtMap;
}

K2OS_PAGEARRAY_TOKEN
K2OS_VirtMap_AcquirePageArray(
    K2OS_VIRTMAP_TOKEN  aTokVirtMap,
    UINT32 *            apRetStartPageIndex
)
{
    K2OS_TOKEN      result;
    K2STAT          stat;
    K2OSKERN_OBJREF refVirtMap;

    if (NULL == aTokVirtMap)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    refVirtMap.AsAny = NULL;
    stat = KernToken_Translate(aTokVirtMap, &refVirtMap);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return NULL;
    }

    result = NULL;

    if (refVirtMap.AsAny->mObjType != KernObj_VirtMap)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
    }
    else
    {
        stat = KernToken_Create(refVirtMap.AsVirtMap->PageArrayRef.AsAny, &result);
        if (K2STAT_IS_ERROR(stat))
        {
            K2_ASSERT(NULL == result);
            K2OS_Thread_SetLastStatus(stat);
        }
        else
        {
            K2_ASSERT(NULL != result);
            if (NULL != apRetStartPageIndex)
            {
                *apRetStartPageIndex = refVirtMap.AsVirtMap->mPageArrayStartPageIx;
            }
        }
    }

    KernObj_ReleaseRef(&refVirtMap);

    return result;
}

K2OS_VIRTMAP_TOKEN
K2OS_VirtMap_Acquire(
    UINT32                      aVirtAddr,
    K2OS_VirtToPhys_MapType *   apRetMapType,
    UINT32 *                    apRetMapPageIndex
)
{
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_OBJ_VIRTMAP *  pVirtMap;
    K2OS_VIRTMAP_TOKEN      result;
    K2STAT                  stat;
    UINT32                  offset;
    BOOL                    disp;
    K2OSKERN_OBJREF         refVirtMap;

    pVirtMap = NULL;
    offset = 0;

    disp = K2OSKERN_SeqLock(&gData.VirtMap.SeqLock);

    pTreeNode = K2TREE_FindOrAfter(&gData.VirtMap.Tree, aVirtAddr);
    if (NULL != pTreeNode)
    {
        pVirtMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, pTreeNode, OwnerMapTreeNode);
        if (pVirtMap->OwnerMapTreeNode.mUserVal > aVirtAddr)
        {
            //
            // back up
            //
            pTreeNode = K2TREE_PrevNode(&gData.VirtMap.Tree, pTreeNode);
            pVirtMap = K2_GET_CONTAINER(K2OSKERN_OBJ_VIRTMAP, pTreeNode, OwnerMapTreeNode);

            // greater than or it would have matched
            K2_ASSERT(aVirtAddr > pVirtMap->OwnerMapTreeNode.mUserVal);
            offset = (aVirtAddr - pVirtMap->OwnerMapTreeNode.mUserVal) / K2_VA_MEMPAGE_BYTES;
            if (offset >= pVirtMap->mPageCount)
            {
                pTreeNode = NULL;
                pVirtMap = NULL;
            }
        }
    }

    if (NULL != pVirtMap)
    {
        refVirtMap.AsAny = NULL;
        KernObj_CreateRef(&refVirtMap, &pVirtMap->Hdr);
    }

    K2OSKERN_SeqUnlock(&gData.VirtMap.SeqLock, disp);

    if (NULL == pVirtMap)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return NULL;
    }

    result = NULL;
    stat = KernToken_Create(refVirtMap.AsAny, &result);

    KernObj_ReleaseRef(&refVirtMap);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    if (NULL != apRetMapPageIndex)
    {
        *apRetMapPageIndex = offset;
    }

    if (NULL != apRetMapType)
    {
        *apRetMapType = pVirtMap->mMapType;
    }

    return result;
}

UINT32
K2OS_VirtMap_GetInfo(
    K2OS_VIRTMAP_TOKEN          aTokVirtMap,
    K2OS_VirtToPhys_MapType *   apRetMapType,
    UINT32 *                    apRetMapPageCount
)
{
    UINT32          result;
    K2STAT          stat;
    K2OSKERN_OBJREF refVirtMap;

    if (NULL == aTokVirtMap)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return 0;
    }

    refVirtMap.AsAny = NULL;
    stat = KernToken_Translate(aTokVirtMap, &refVirtMap);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return 0;
    }

    if (refVirtMap.AsAny->mObjType != KernObj_VirtMap)
    {
        result = 0;
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
    }
    else
    {
        result = refVirtMap.AsVirtMap->OwnerMapTreeNode.mUserVal;
        if (NULL != apRetMapType)
        {
            *apRetMapType = refVirtMap.AsVirtMap->mMapType;
        }
        if (NULL != apRetMapPageCount)
        {
            *apRetMapPageCount = refVirtMap.AsVirtMap->mPageCount;
        }
    }

    KernObj_ReleaseRef(&refVirtMap);

    return result;
}

