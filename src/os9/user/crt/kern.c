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

UINT32 
CrtKern_SysCall2(
    UINT32 aId, 
    UINT32 aArg0, 
    UINT32 aArg1
)
{
    K2OS_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg1 = aArg1;
    return K2OS_SYSCALL(aId, aArg0);
}

UINT32 
CrtKern_SysCall3(
    UINT32 aId, 
    UINT32 aArg0, 
    UINT32 aArg1, 
    UINT32 aArg2
)
{
    K2OS_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg1 = aArg1;
    pThreadPage->mSysCall_Arg2 = aArg2;
    return K2OS_SYSCALL(aId, aArg0);
}

UINT32 
CrtKern_SysCall4(
    UINT32 aId, 
    UINT32 aArg0, 
    UINT32 aArg1, 
    UINT32 aArg2, 
    UINT32 aArg3
)
{
    K2OS_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg3 = aArg3;
    pThreadPage->mSysCall_Arg1 = aArg1;
    pThreadPage->mSysCall_Arg2 = aArg2;
    return K2OS_SYSCALL(aId, aArg0);
}

UINT32 
CrtKern_SysCall5(
    UINT32 aId, 
    UINT32 aArg0, 
    UINT32 aArg1, 
    UINT32 aArg2, 
    UINT32 aArg3, 
    UINT32 aArg4
)
{
    K2OS_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg3 = aArg3;
    pThreadPage->mSysCall_Arg4_Result3 = aArg4;
    pThreadPage->mSysCall_Arg1 = aArg1;
    pThreadPage->mSysCall_Arg2 = aArg2;
    return K2OS_SYSCALL(aId, aArg0);
}

UINT32 
CrtKern_SysCall6(
    UINT32 aId, 
    UINT32 aArg0, 
    UINT32 aArg1, 
    UINT32 aArg2, 
    UINT32 aArg3, 
    UINT32 aArg4, 
    UINT32 aArg5
)
{
    K2OS_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg3 = aArg3;
    pThreadPage->mSysCall_Arg4_Result3 = aArg4;
    pThreadPage->mSysCall_Arg5_Result2 = aArg5;
    pThreadPage->mSysCall_Arg1 = aArg1;
    pThreadPage->mSysCall_Arg2 = aArg2;
    return K2OS_SYSCALL(aId, aArg0);
}

UINT32 
CrtKern_SysCall7(
    UINT32 aId, 
    UINT32 aArg0, 
    UINT32 aArg1, 
    UINT32 aArg2, 
    UINT32 aArg3, 
    UINT32 aArg4, 
    UINT32 aArg5, 
    UINT32 aArg6
)
{
    K2OS_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg3 = aArg3;
    pThreadPage->mSysCall_Arg4_Result3 = aArg4;
    pThreadPage->mSysCall_Arg5_Result2 = aArg5;
    pThreadPage->mSysCall_Arg6_Result1 = aArg6;
    pThreadPage->mSysCall_Arg1 = aArg1;
    pThreadPage->mSysCall_Arg2 = aArg2;
    return K2OS_SYSCALL(aId, aArg0);
}

UINT32 
CrtKern_SysCall8(
    UINT32 aId, 
    UINT32 aArg0, 
    UINT32 aArg1, 
    UINT32 aArg2, 
    UINT32 aArg3, 
    UINT32 aArg4, 
    UINT32 aArg5, 
    UINT32 aArg6, 
    UINT32 aArg7
)
{
    K2OS_THREAD_PAGE * pThreadPage;
    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mSysCall_Arg3 = aArg3;
    pThreadPage->mSysCall_Arg4_Result3 = aArg4;
    pThreadPage->mSysCall_Arg5_Result2 = aArg5;
    pThreadPage->mSysCall_Arg6_Result1 = aArg6;
    pThreadPage->mSysCall_Arg7_Result0 = aArg7;
    pThreadPage->mSysCall_Arg1 = aArg1;
    pThreadPage->mSysCall_Arg2 = aArg2;
    return K2OS_SYSCALL(aId, aArg0);
}

