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

#if !K2_TARGET_ARCH_IS_ARM

UINT_PTR 
CrtKern_SysCall2(
    UINT_PTR aId, 
    UINT_PTR aArg0, 
    UINT_PTR aArg1
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg1 = aArg1;
    return K2OS_SYSCALL(aId, aArg0);
}

UINT_PTR 
CrtKern_SysCall3(
    UINT_PTR aId, 
    UINT_PTR aArg0, 
    UINT_PTR aArg1, 
    UINT_PTR aArg2
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg1 = aArg1;
    pThreadPage->mSysCall_Arg2 = aArg2;
    return K2OS_SYSCALL(aId, aArg0);
}

#endif

UINT_PTR 
CrtKern_SysCall4(
    UINT_PTR aId, 
    UINT_PTR aArg0, 
    UINT_PTR aArg1, 
    UINT_PTR aArg2, 
    UINT_PTR aArg3
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg3 = aArg3;
#if !K2_TARGET_ARCH_IS_ARM
    pThreadPage->mSysCall_Arg1 = aArg1;
    pThreadPage->mSysCall_Arg2 = aArg2;
    return K2OS_SYSCALL(aId, aArg0);
#else
    return K2OS_SYSCALL(aId, aArg0, aArg1, aArg2);
#endif
}

UINT_PTR 
CrtKern_SysCall5(
    UINT_PTR aId, 
    UINT_PTR aArg0, 
    UINT_PTR aArg1, 
    UINT_PTR aArg2, 
    UINT_PTR aArg3, 
    UINT_PTR aArg4
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg3 = aArg3;
    pThreadPage->mSysCall_Arg4_Result3 = aArg4;
#if !K2_TARGET_ARCH_IS_ARM
    pThreadPage->mSysCall_Arg1 = aArg1;
    pThreadPage->mSysCall_Arg2 = aArg2;
    return K2OS_SYSCALL(aId, aArg0);
#else
    return K2OS_SYSCALL(aId, aArg0, aArg1, aArg2);
#endif
}

UINT_PTR 
CrtKern_SysCall6(
    UINT_PTR aId, 
    UINT_PTR aArg0, 
    UINT_PTR aArg1, 
    UINT_PTR aArg2, 
    UINT_PTR aArg3, 
    UINT_PTR aArg4, 
    UINT_PTR aArg5
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg3 = aArg3;
    pThreadPage->mSysCall_Arg4_Result3 = aArg4;
    pThreadPage->mSysCall_Arg5_Result2 = aArg5;
#if !K2_TARGET_ARCH_IS_ARM
    pThreadPage->mSysCall_Arg1 = aArg1;
    pThreadPage->mSysCall_Arg2 = aArg2;
    return K2OS_SYSCALL(aId, aArg0);
#else
    return K2OS_SYSCALL(aId, aArg0, aArg1, aArg2);
#endif
}

UINT_PTR 
CrtKern_SysCall7(
    UINT_PTR aId, 
    UINT_PTR aArg0, 
    UINT_PTR aArg1, 
    UINT_PTR aArg2, 
    UINT_PTR aArg3, 
    UINT_PTR aArg4, 
    UINT_PTR aArg5, 
    UINT_PTR aArg6
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg3 = aArg3;
    pThreadPage->mSysCall_Arg4_Result3 = aArg4;
    pThreadPage->mSysCall_Arg5_Result2 = aArg5;
    pThreadPage->mSysCall_Arg6_Result1 = aArg6;
#if !K2_TARGET_ARCH_IS_ARM
    pThreadPage->mSysCall_Arg1 = aArg1;
    pThreadPage->mSysCall_Arg2 = aArg2;
    return K2OS_SYSCALL(aId, aArg0);
#else
    return K2OS_SYSCALL(aId, aArg0, aArg1, aArg2);
#endif
}

UINT_PTR 
CrtKern_SysCall8(
    UINT_PTR aId, 
    UINT_PTR aArg0, 
    UINT_PTR aArg1, 
    UINT_PTR aArg2, 
    UINT_PTR aArg3, 
    UINT_PTR aArg4, 
    UINT_PTR aArg5, 
    UINT_PTR aArg6, 
    UINT_PTR aArg7
)
{
    K2OS_USER_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg3 = aArg3;
    pThreadPage->mSysCall_Arg4_Result3 = aArg4;
    pThreadPage->mSysCall_Arg5_Result2 = aArg5;
    pThreadPage->mSysCall_Arg6_Result1 = aArg6;
    pThreadPage->mSysCall_Arg7_Result0 = aArg7;
#if !K2_TARGET_ARCH_IS_ARM
    pThreadPage->mSysCall_Arg1 = aArg1;
    pThreadPage->mSysCall_Arg2 = aArg2;
    return K2OS_SYSCALL(aId, aArg0);
#else
    return K2OS_SYSCALL(aId, aArg0, aArg1, aArg2);
#endif
}

