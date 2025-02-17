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
#ifndef __K2OSFS_H
#define __K2OSFS_H

#include <k2osstor.h>

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

BOOL FsMgr_Connected(void);

typedef struct _K2OSFS_CLIENT K2OSFS_CLIENT;
struct _K2OSFS_CLIENT
{
    K2OS_RPC_OBJ_HANDLE mhRpcObj;
    UINT32              mObjId;
    K2TREE_NODE         TreeNode;   // userval is pointer to client
};

extern K2OS_RPC_OBJ_HANDLE  gK2OSFS_FsMgrRpcObjHandle;
extern K2OS_IFINST_ID       gK2OSFS_FsMgrRpcServerIfInstId;

extern K2_GUID128 const     gFsClientRpcClassId;
extern K2OSFS_CLIENT        gK2OSFS_DefaultClient;

K2OS_RPC_OBJ_HANDLE K2OSFS_GetClientRpc(K2OS_FSCLIENT aClient, UINT32 *apRetObjId);

// temp proto decl
UINT32 K2OSRPC_Debug(char const *apFormat, ...);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __K2OSFS_H
