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

void 
K2_CALLCONV_REGS
K2OS_RaiseException(
    K2STAT aExceptionCode
)
{
    CrtKern_SysCall1(K2OS_SYSCALL_ID_RAISE_EXCEPTION, aExceptionCode);
}

K2STAT 
K2_CALLCONV_REGS
Crt_ExTrap_Dismount(
    K2_EXCEPTION_TRAP *apTrap
)
{
    K2OS_THREAD_PAGE *  pThreadPage;

    //
    // if we get here then no exception during trap lifetime
    //

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_THREADPAGES_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
    pThreadPage->mTrapStackTop = (UINT32)apTrap->mpNextTrap;

    return apTrap->mTrapResult;
}

void 
Crt_Assert(
    char const *    apFile, 
    int             aLineNum, 
    char const *    apCondition
)
{
    CrtDbg_Printf("ASSERT %s:%d (%s)\n", apFile, aLineNum, apCondition);
    K2OS_RaiseException(K2STAT_EX_LOGIC);
    *((UINT32 *)0) = 0;
    while (1);
}

K2_pf_ASSERT            K2_Assert =             Crt_Assert;
K2_pf_EXTRAP_DISMOUNT   K2_ExTrap_Dismount =    Crt_ExTrap_Dismount;
K2_pf_RAISE_EXCEPTION   K2_RaiseException =     K2OS_RaiseException;
