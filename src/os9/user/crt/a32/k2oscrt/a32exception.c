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
#include "a32crtuser.h"

BOOL
K2_CALLCONV_REGS
A32Crt_ExTrap_Mount_C(
    K2_EXCEPTION_TRAP *apTrap
)
{
    //
    // context saved.  LR is original LR value before call thru pointer to trap mount
    //
    CrtMem_Touch(apTrap, sizeof(K2_EXCEPTION_TRAP));

    // move LR to PC so we return direct to caller of func through pointer
    apTrap->SavedContext.R[15] = apTrap->SavedContext.R[14];

    // returned as value if system call fails
    apTrap->mTrapResult = K2STAT_ERROR_BAD_ARGUMENT;

    //
    // this should return FALSE if the trap is mounted properly.  in which case the
    // trap's mTrapResult will be set to NO_ERROR
    //
    return (BOOL)CrtKern_SysCall1(K2OS_SYSCALL_ID_TRAP_MOUNT, (UINT32)apTrap);
}

BOOL K2_CALLCONV_REGS A32CrtAsm_ExTrap_Mount(K2_EXCEPTION_TRAP *apTrap);

K2_pf_EXTRAP_MOUNT K2_ExTrap_Mount = A32CrtAsm_ExTrap_Mount;
