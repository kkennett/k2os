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

static K2_GUID128 const sgKernelServerRpcObjClassId = K2OS_KERNEL_PROCESS_RPC_CLASS;

static K2OS_RPC_OBJ_HANDLE sghKernelServerObj = NULL;

#define TEST_TRAP 0

#if TEST_TRAP
K2STAT Test(void)
{
    *((UINT32 *)0) = 0;
    return K2STAT_NO_ERROR;
}
#endif

void
CrtRpc_Init(
    void
)
{
#if TEST_TRAP
    K2_EXCEPTION_TRAP   trap;
    K2STAT              stat;
#endif

    sghKernelServerObj = K2OS_Rpc_CreateObj(1, &sgKernelServerRpcObjClassId, gProcessId);
    if (NULL == sghKernelServerObj)
    {
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }
#if TEST_TRAP
    stat = K2_EXTRAP(&trap, Test());

    CrtDbg_Printf("trap result = %08X\n", stat);
#endif
}

K2OS_RPC_OBJ_HANDLE
CrtRpc_Obj(
    void
)
{
    return sghKernelServerObj;
}