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

#include "ik2osrpc.h"

K2OS_CRITSEC    gK2OSRPC_GlobalUsageTreeSec;
K2TREE_ANCHOR   gK2OSRPC_GlobalUsageTree;
INT_PTR         gK2OSRPC_NextGlobalUsageId;

K2STAT
K2_CALLCONV_REGS
xdl_entry(
    XDL *   apXdl,
    UINT32  aReason
)
{
    K2STAT stat;

    if (XDL_ENTRY_REASON_LOAD == aReason)
    {
        if (!K2OS_CritSec_Init(&gK2OSRPC_GlobalUsageTreeSec))
        {
            K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
        }
        gK2OSRPC_NextGlobalUsageId = 1;
        K2TREE_Init(&gK2OSRPC_GlobalUsageTree, NULL);

        stat = K2OSRPC_Client_AtXdlEntry(XDL_ENTRY_REASON_LOAD);
        if (!K2STAT_IS_ERROR(stat))
        {
            stat = K2OSRPC_Server_AtXdlEntry(XDL_ENTRY_REASON_LOAD);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSRPC_Client_AtXdlEntry(XDL_ENTRY_REASON_UNLOAD);
            }
        }

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_CritSec_Done(&gK2OSRPC_GlobalUsageTreeSec);
        }

        return stat;
    }
    
    if (XDL_ENTRY_REASON_UNLOAD == aReason)
    {
        K2OSRPC_Server_AtXdlEntry(XDL_ENTRY_REASON_UNLOAD);
        K2OSRPC_Client_AtXdlEntry(XDL_ENTRY_REASON_UNLOAD);
        K2_ASSERT(0 == gK2OSRPC_GlobalUsageTree.mNodeCount);
        K2OS_CritSec_Done(&gK2OSRPC_GlobalUsageTreeSec);
    }

    return K2STAT_NO_ERROR;
}
 