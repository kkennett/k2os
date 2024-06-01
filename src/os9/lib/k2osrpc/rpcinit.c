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
#include "k2osrpc.h"

K2OS_CRITSEC        gRpcGraphSec;
K2TREE_ANCHOR       gRpcHandleTree;    // keyed on pointer to handle header
K2_GUID128 const    gRpcServerClassId = K2OS_IFACE_RPC_SERVER;
K2OS_IFINST_ID      gRpcServerIfInstId;

void K2OSRPC_Client_Init(void);
void K2OSRPC_Server_Init(void);

UINT32
K2OSRPC_Debug(
    char const *apFormat,
    ...
)
{
    char    outBuf[128];
    VALIST  vaList;
    UINT32  result;

    K2_VASTART(vaList, apFormat);
    result = K2ASC_PrintfVarLen(outBuf, 127, apFormat, vaList);
    K2_VAEND(vaList);
    outBuf[95] = 0;
    K2OS_Debug_OutputString(outBuf);
    return result;
}

K2STAT
K2OSRPC_Init(
    void
)
{
    BOOL ok;

    FUNC_ENTER;

    ok = K2OS_CritSec_Init(&gRpcGraphSec);
    K2_ASSERT(ok);
    if (!ok)
    {
        FUNC_EXIT;
        return K2OS_Thread_GetLastStatus();
    }

    K2TREE_Init(&gRpcHandleTree, NULL);

    K2OSRPC_Client_Init();

    gRpcServerIfInstId = 0;

    K2OSRPC_Server_Init();

    FUNC_EXIT;

    return K2STAT_NO_ERROR;
}
