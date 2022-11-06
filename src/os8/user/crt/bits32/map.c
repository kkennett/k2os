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

K2OS_MAP_TOKEN 
K2OS_Map_Create(
    K2OS_PAGEARRAY_TOKEN    aTokPageArray,
    UINT_PTR                aPageOffset,
    UINT_PTR                aPageCount,
    UINT_PTR                aVirtAddr,
    K2OS_User_MapType       aMapType
)
{
    return (K2OS_MAP_TOKEN)CrtKern_SysCall5(K2OS_SYSCALL_ID_MAP_CREATE, 
        (UINT_PTR)aTokPageArray,
        aPageOffset, aPageCount,
        aVirtAddr, aMapType);
}

K2OS_PAGEARRAY_TOKEN 
K2_CALLCONV_REGS
K2OS_Map_AcquirePageArray(
    K2OS_MAP_TOKEN  aTokMap,
    UINT_PTR *      apRetStartPageIndex
)
{
    K2OS_PAGEARRAY_TOKEN    tokResult;
    K2OS_USER_THREAD_PAGE * pThreadPage;

    tokResult = (K2OS_PAGEARRAY_TOKEN)CrtKern_SysCall1(K2OS_SYSCALL_ID_MAP_ACQ_PAGEARRAY, (UINT_PTR)aTokMap);

    if (NULL == tokResult)
        return NULL;

    if (NULL != apRetStartPageIndex)
    {
        pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
        *apRetStartPageIndex = pThreadPage->mSysCall_Arg7_Result0;
    }

    return tokResult;
}

UINT_PTR
K2OS_Map_GetInfo(
    K2OS_MAP_TOKEN      aTokMap,
    K2OS_User_MapType * apRetMapType,
    UINT_PTR *          apRetMapPageCount
)
{
    UINT32                  result;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    
    result = CrtKern_SysCall1(K2OS_SYSCALL_ID_MAP_GET_INFO, (UINT_PTR)aTokMap);
    if (0 != result)
    {
        pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
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

K2OS_MAP_TOKEN 
K2OS_Map_Acquire(
    UINT_PTR            aProcAddr,
    K2OS_User_MapType * apRetMapType,
    UINT_PTR *          apRetMapPageIndex
)
{
    K2OS_MAP_TOKEN          tokResult;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    
    tokResult = (K2OS_MAP_TOKEN)CrtKern_SysCall1(K2OS_SYSCALL_ID_MAP_ACQUIRE, aProcAddr);

    if (NULL == tokResult)
        return NULL;

    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    if (NULL != apRetMapType)
        *apRetMapType = pThreadPage->mSysCall_Arg7_Result0;
    if (NULL != apRetMapPageIndex)
        *apRetMapPageIndex = pThreadPage->mSysCall_Arg6_Result1;

    return tokResult;
}