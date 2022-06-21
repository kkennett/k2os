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
#include "crt32.h"

static K2OS_CRITSEC sgHeapSec;
static K2RAMHEAP    sgRamHeap;

UINT32 
CrtRamHeap_Lock(
    K2RAMHEAP *apRamHeap
)
{
    K2_ASSERT(apRamHeap == &sgRamHeap);
    K2OS_CritSec_Enter(&sgHeapSec);
    return 0;
}

void 
CrtRamHeap_Unlock(
    K2RAMHEAP * apRamHeap, 
    UINT32      aDisp
)
{
    K2_ASSERT(apRamHeap == &sgRamHeap);
    K2_ASSERT(0 == aDisp);
    K2OS_CritSec_Leave(&sgHeapSec);
}

K2STAT  
CrtRamHeap_CreateRange(
    UINT32 aTotalPageCount,
    UINT32 aInitCommitPageCount,
    void **appRetRange,
    UINT32 *apRetStartAddr
)
{
    K2STAT                  stat;
    K2OS_PAGEARRAY_TOKEN    tokPageArray;
    UINT32                  virtBase;
    K2OS_MAP_TOKEN          tokMap;

    tokPageArray = K2OS_PageArray_Create(aTotalPageCount);
    if (NULL == tokPageArray)
    {
        return K2OS_Thread_GetLastStatus();
    }

    stat = K2STAT_NO_ERROR;
    do
    {
        virtBase = K2OS_Virt_Reserve(aTotalPageCount);
        if (0 == virtBase)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            break;
        }

        do
        {
            tokMap = K2OS_Map_Create(tokPageArray, 0, aTotalPageCount, virtBase, K2OS_MapType_Data_ReadWrite);
            if (NULL == tokMap)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                break;
            }

            *appRetRange = tokMap;
            *apRetStartAddr = virtBase;

            stat = K2STAT_NO_ERROR;

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Virt_Release(virtBase);
        }

    } while (0);

    //
    // always destroy the range token.  map has taken a reference if
    // it was created.  if it was not then we are destroying range in the error path
    //
    K2OS_Token_Destroy(tokPageArray);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
    }

    return stat;
}

K2STAT  
CrtRamHeap_CommitPages(
    void *aRange,
    UINT32 aAddr,
    UINT32 aPageCount
)
{
    //
    // pages actually already committed - call is ignored
    //
    return K2STAT_NO_ERROR;
}

K2STAT  
CrtRamHeap_DecommitPages(
    void *aRange,
    UINT32 aAddr,
    UINT32 aPageCount
)
{
    //
    // pages cannot be decommitted - call is ignored
    //
    return K2STAT_NO_ERROR;
}

K2STAT  
CrtRamHeap_DestroyRange(
    void *aRange
)
{
    K2STAT              stat;
    K2OS_MAP_TOKEN      tokMap;
    UINT32              virtBase;
    K2OS_User_MapType   userMapType;
    BOOL                ok;

    tokMap = (K2OS_MAP_TOKEN)aRange;
    
    virtBase = K2OS_Map_GetInfo(tokMap, &userMapType, NULL);
    if (0 == virtBase)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }
    K2_ASSERT(userMapType == K2OS_MapType_Data_ReadWrite);

    //
    // destroying the map should destroy the pagearray since
    // we destroyed the original pagearray token on create.
    //
    ok = K2OS_Token_Destroy(tokMap);
    K2_ASSERT(ok);

    ok = K2OS_Virt_Release(virtBase);
    K2_ASSERT(ok);

    return K2STAT_NO_ERROR;
}

static K2RAMHEAP_SUPP const sgRamHeapSupp = {
    CrtRamHeap_Lock,
    CrtRamHeap_Unlock,
    CrtRamHeap_CreateRange,
    CrtRamHeap_CommitPages,
    CrtRamHeap_DecommitPages,
    CrtRamHeap_DestroyRange
};

void
CrtHeap_Init(
    void
)
{
    BOOL ok;

    ok = K2OS_CritSec_Init(&sgHeapSec);
    K2_ASSERT(ok);
    if (!ok)
    {
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    K2RAMHEAP_Init(&sgRamHeap, &sgRamHeapSupp);
}

void * 
K2_CALLCONV_REGS 
K2OS_Heap_Alloc(
    UINT_PTR aByteCount
)
{
    K2STAT  stat;
    void *  ptr;

    ptr = NULL;

    stat = K2RAMHEAP_Alloc(&sgRamHeap, aByteCount, TRUE, &ptr);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return NULL;
    }

    K2_ASSERT(NULL != ptr);

    return ptr;
}

BOOL
K2_CALLCONV_REGS 
K2OS_Heap_Free(
    void *aPtr
)
{
    K2STAT          stat;
#if 0
    K2RAMHEAP_STATE state;
    UINT32          largestFree;
#endif

    K2_ASSERT(NULL != aPtr);
    stat = K2RAMHEAP_Free(&sgRamHeap, aPtr);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));
    return (K2STAT_IS_ERROR(stat) ? FALSE : TRUE);
#if 0
    stat = K2RAMHEAP_GetState(&sgRamHeap, &state, &largestFree);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));
    CrtDbg_Printf("ramheap.mAllocCount = %08X\n", state.mAllocCount);
    CrtDbg_Printf("ramheap.mTotalAlloc = %08X\n", state.mTotalAlloc);
    CrtDbg_Printf("ramheap.mTotalFree = %08X\n", state.mTotalFree);
    CrtDbg_Printf("ramheap.mTotalOverhead = %08X\n", state.mTotalOverhead);
#endif
}
