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

K2OS_VIRTMAP_TOKEN
K2OS_VirtMap_Create(
    K2OS_PAGEARRAY_TOKEN    aTokPageArray,
    UINT32                  aPageOffset,
    UINT32                  aPageCount,
    UINT32                  aVirtAddr,
    K2OS_VirtToPhys_MapType aMapType
)
{
    return (K2OS_VIRTMAP_TOKEN)CrtKern_SysCall5(K2OS_SYSCALL_ID_MAP_CREATE, 
        (UINT32)aTokPageArray,
        aPageOffset, aPageCount,
        aVirtAddr, aMapType);
}

K2OS_PAGEARRAY_TOKEN 
K2OS_VirtMap_AcquirePageArray(
    K2OS_VIRTMAP_TOKEN  aTokVirtMap,
    UINT32 *            apRetStartPageIndex
)
{
    K2OS_PAGEARRAY_TOKEN    tokResult;
    K2OS_THREAD_PAGE *      pThreadPage;

    tokResult = (K2OS_PAGEARRAY_TOKEN)CrtKern_SysCall1(K2OS_SYSCALL_ID_MAP_ACQ_PAGEARRAY, (UINT32)aTokVirtMap);

    if (NULL == tokResult)
        return NULL;

    if (NULL != apRetStartPageIndex)
    {
        pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
        *apRetStartPageIndex = pThreadPage->mSysCall_Arg7_Result0;
    }

    return tokResult;
}

UINT32
K2OS_VirtMap_GetInfo(
    K2OS_VIRTMAP_TOKEN          aTokVirtMap,
    K2OS_VirtToPhys_MapType *   apRetMapType,
    UINT32 *                    apRetMapPageCount
)
{
    UINT32              result;
    K2OS_THREAD_PAGE *  pThreadPage;
    
    result = CrtKern_SysCall1(K2OS_SYSCALL_ID_MAP_GET_INFO, (UINT32)aTokVirtMap);
    if (0 != result)
    {
        pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
        if (NULL != apRetMapType)   
            *apRetMapType = pThreadPage->mSysCall_Arg7_Result0;
        if (NULL != apRetMapPageCount)
            *apRetMapPageCount = pThreadPage->mSysCall_Arg6_Result1;
    }
    else
    {
        if (NULL != apRetMapType)
            *apRetMapType = 0;
        if (NULL != apRetMapPageCount)
            *apRetMapPageCount = 0;
    }

    return result;
}

K2OS_VIRTMAP_TOKEN
K2OS_VirtMap_Acquire(
    UINT32                      aProcAddr,
    K2OS_VirtToPhys_MapType *   apRetMapType,
    UINT32 *                    apRetMapPageIndex
)
{
    K2OS_VIRTMAP_TOKEN  result;
    K2OS_THREAD_PAGE *  pThreadPage;
    
    result = (K2OS_VIRTMAP_TOKEN)CrtKern_SysCall1(K2OS_SYSCALL_ID_MAP_ACQUIRE, aProcAddr);

    if (NULL == result)
        return NULL;

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    if (NULL != apRetMapType)
        *apRetMapType = pThreadPage->mSysCall_Arg7_Result0;
    if (NULL != apRetMapPageIndex)
        *apRetMapPageIndex = pThreadPage->mSysCall_Arg6_Result1;

    return result;
}

