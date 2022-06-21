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

UINT_PTR
K2_CALLCONV_REGS
K2OS_Virt_Reserve(
    UINT_PTR aPageCount
)
{
    return CrtKern_SysCall1(K2OS_SYSCALL_ID_VIRT_RESERVE, aPageCount);
}

UINT_PTR
K2_CALLCONV_REGS
K2OS_Virt_Get(
    UINT_PTR aVirtAddr,
    UINT_PTR *apRetPageCount
)
{
    UINT32                  result;
    K2OS_USER_THREAD_PAGE * pThreadPage;

    result = CrtKern_SysCall1(K2OS_SYSCALL_ID_VIRT_GET, aVirtAddr);
    if ((0 != result) &&
        (NULL != apRetPageCount))
    {
        pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
        *apRetPageCount = pThreadPage->mSysCall_Arg7_Result0;
    }

    return result;
}

BOOL    
K2_CALLCONV_REGS
K2OS_Virt_Release(
    UINT_PTR aVirtAddr
)
{
    return (BOOL)CrtKern_SysCall1(K2OS_SYSCALL_ID_VIRT_RELEASE, aVirtAddr);
}
