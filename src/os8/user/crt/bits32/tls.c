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

static UINT64 volatile sgSlotBitfield = 0;  

BOOL 
K2_CALLCONV_REGS
K2OS_Tls_AllocSlot(
    UINT_PTR *apRetNewIndex
)
{
    BOOL                    result;
    UINT_PTR                bitNumber;
    K2OS_USER_THREAD_PAGE * pThreadPage;

    if (NULL == apRetNewIndex)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    result = CrtKern_SysCall1(K2OS_SYSCALL_ID_TLS_ALLOC, 0);
    if (!result)
        return FALSE;

    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    bitNumber = pThreadPage->mSysCall_Arg7_Result0;
    K2_ASSERT(bitNumber < K2OS_NUM_THREAD_TLS_SLOTS);
    K2_ASSERT(0 == ((((UINT64)1) << bitNumber) & sgSlotBitfield));

    K2ATOMIC_Or64(&sgSlotBitfield, ((UINT64)1) << bitNumber);

    *apRetNewIndex = bitNumber;

    return TRUE;
}

BOOL
K2_CALLCONV_REGS
K2OS_Tls_FreeSlot(
    UINT_PTR aSlotIndex
)
{
    BOOL result;

    if (aSlotIndex >= K2OS_NUM_THREAD_TLS_SLOTS)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_BOUNDS);
        return FALSE;
    }
       
    if (0 == ((((UINT64)1) << aSlotIndex) & sgSlotBitfield))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IN_USE);
        return FALSE;
    }

    result = CrtKern_SysCall1(K2OS_SYSCALL_ID_TLS_FREE, aSlotIndex);
    if (!result)
        return FALSE;

    K2ATOMIC_And64(&sgSlotBitfield, ~(((UINT64)1) << aSlotIndex));

    return TRUE;
}

BOOL
K2_CALLCONV_REGS
K2OS_Tls_SetValue(
    UINT_PTR aSlotIndex,
    UINT_PTR aValue
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;

    if (K2OS_NUM_THREAD_TLS_SLOTS <= aSlotIndex)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_BOUNDS);
        return FALSE;
    }

    if (0 == (sgSlotBitfield & (1ull << aSlotIndex)))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IN_USE);
        return FALSE;
    }

    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    pThreadPage->mTlsValue[aSlotIndex] = aValue;

    return TRUE;
}

BOOL
K2_CALLCONV_REGS
K2OS_Tls_GetValue(
    UINT_PTR aSlotIndex,
    UINT_PTR *apRetValue
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;

    if (K2OS_NUM_THREAD_TLS_SLOTS <= aSlotIndex)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_BOUNDS);
        return FALSE;
    }

    if (0 == (sgSlotBitfield & (1ull << aSlotIndex)))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_IN_USE);
        return FALSE;
    }

    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    *apRetValue = pThreadPage->mTlsValue[aSlotIndex];

    return TRUE;
}

