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

UINT32
K2OS_Virt_Get(
    UINT32 aVirtAddr,
    UINT32 *apRetPageCount
)
{
    BOOL            disp;
    BOOL            result;
    K2HEAP_NODE *   pHeapNode;

    K2_ASSERT(NULL != apRetPageCount);
    *apRetPageCount = 0;

    disp = K2OSKERN_SeqLock(&gData.Virt.HeapSeqLock);

    pHeapNode = K2HEAP_FindNodeContainingAddr(&gData.Virt.Heap, aVirtAddr);
    if (NULL == pHeapNode)
    {
        result = 0;
    }
    else
    {
        if (K2HEAP_NodeIsFree(pHeapNode))
        {
            result = 0;
        }
        else
        {
            result = pHeapNode->AddrTreeNode.mUserVal;
            *apRetPageCount = pHeapNode->SizeTreeNode.mUserVal / K2_VA_MEMPAGE_BYTES;
        }
    }

    K2OSKERN_SeqUnlock(&gData.Virt.HeapSeqLock, disp);

    return result;
}

UINT32
K2OS_Virt_Reserve(
    UINT32  aPagesCount
)
{
    UINT32 result;

    result = KernVirt_Reserve(aPagesCount);

    if (0 == result)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_MEMORY);
    }

    return result;
}

BOOL
K2OS_Virt_Release(
    UINT32 aPagesAddr
)
{
    BOOL result;

    result = KernVirt_Release(aPagesAddr);

    if (!result)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
    }

    return result;
}

void *
K2OS_Heap_Alloc(
    UINT32 aByteCount
)
{
    void *pResult;
    pResult = KernHeap_Alloc(aByteCount);
    if (NULL == pResult)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_MEMORY);
    }
    return pResult;
}

BOOL
K2OS_Heap_Free(
    void *aPtr
)
{
    BOOL result;

    result = KernHeap_Free(aPtr);
    if (!result)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
    }

    return result;
}

