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
#define INITGUID
#include "fatwork.h"

static
void
myAssert(
    char const *    apFile,
    int             aLineNum,
    char const *    apCondition
)
{
    DebugBreak();
}

extern "C" K2_pf_ASSERT K2_Assert = myAssert;



typedef struct _K2OS_RPC_SERVER_OPAQUE K2OS_RPC_SERVER_OPAQUE;
typedef struct _K2OS_RPC_CLASS_OPAQUE K2OS_RPC_CLASS_OPAQUE;
typedef struct _K2OS_RPC_OBJ_HANDLE_OPAQUE K2OS_RPC_OBJ_HANDLE_OPAQUE;

typedef K2OS_RPC_SERVER_OPAQUE *        K2OS_RPC_SERVER;
typedef K2OS_RPC_CLASS_OPAQUE *         K2OS_RPC_CLASS;
typedef K2OS_RPC_OBJ_HANDLE_OPAQUE *    K2OS_RPC_OBJ_HANDLE;

K2OS_RPC_SERVER K2OS_RpcServer_Create(void);
UINT32          K2OS_RpcServer_Run(K2OS_RPC_SERVER aServer, K2OS_IFINST_ID *apRetIfInst, BOOL aOnThisThread);
K2OS_IFINST_ID  K2OS_RpcServer_GetIfInstId(K2OS_RPC_SERVER aServer);
BOOL            K2OS_RpcServer_GetState(K2OS_RPC_SERVER aServer, UINT32 *apRetState);
BOOL            K2OS_RpcServer_RequestStop(K2OS_RPC_SERVER aServer);
BOOL            K2OS_RpcServer_Delete(K2OS_RPC_SERVER aServer);
K2OS_RPC_CLASS  K2OS_RpcServer_Register(K2OS_RPC_SERVER aServer, K2OS_RPC_OBJ_CLASS const *apClassDef);
BOOL            K2OS_RpcServer_Deregister(K2OS_RPC_SERVER aServer, K2OS_RPC_CLASS aRegisteredClass);
BOOL            K2OS_RpcServer_SendNotify(K2OS_RPC_OBJ aObject, UINT32 aNotify1, UINT32 aNotify2);

K2OS_RPC_OBJ_HANDLE K2OS_Rpc_CreateObj(K2OS_IFINST_ID aRpcServerIfInstId, K2_GUID128 const *apClassId, UINT32 aCreatorContext);
K2OS_RPC_OBJ_HANDLE K2OS_Rpc_AttachToObj(K2OS_IFINST_ID aRpcServerIfInstId, UINT32 aObjId);
BOOL                K2OS_Rpc_GetObjId(K2OS_RPC_OBJ_HANDLE aObjHandle, UINT32 *apRetObjId);
BOOL                K2OS_Rpc_SetNotifyTarget(K2OS_RPC_OBJ_HANDLE aObjHandle, K2OS_MAILSLOT_TOKEN aTokMailslot);
K2STAT              K2OS_Rpc_Call(K2OS_RPC_OBJ_HANDLE aObjHandle, K2OS_RPC_CALL const *apCall, UINT32 *apRetActualOutBytes);
BOOL                K2OS_Rpc_Release(K2OS_RPC_OBJ_HANDLE aObjHandle);


int main(int argc, char **argv)
{

    return 0;
}
