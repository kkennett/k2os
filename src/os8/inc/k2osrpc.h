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
#ifndef __K2OSRPC_H
#define __K2OSRPC_H

#include "k2os.h"

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

UINT_PTR
K2OSRPC_DebugPrintf(
    char const *apFormat,
    ...
);

//
//------------------------------------------------------------------------
//

// {2F300AA6-F09E-4FC5-8E67-4043F4CED633}
#define K2OS_IFACE_ID_RPC_SERVER  { 0x2f300aa6, 0xf09e, 0x4fc5, { 0x8e, 0x67, 0x40, 0x43, 0xf4, 0xce, 0xd6, 0x33 } }

typedef struct _K2OSRPC_OBJECT_CLASS K2OSRPC_OBJECT_CLASS;

typedef struct _K2OSRPC_OBJECT_CREATE K2OSRPC_OBJECT_CREATE;
struct _K2OSRPC_OBJECT_CREATE
{
    K2OS_GATE_TOKEN     mRemoteDisconnectedGateToken;
    UINT_PTR            mClassPublishContext;
    UINT_PTR            mCallerProcessId;
    UINT_PTR            mCallerContext;
};
typedef K2STAT (*K2OSRPC_pf_Object_Create)(K2OSRPC_OBJECT_CREATE const *apCreate, UINT_PTR *apRetCreatorRef);

typedef struct _K2OSRPC_CALLARGS K2OSRPC_CALLARGS;
struct _K2OSRPC_CALLARGS
{
    UINT_PTR        mMethodId;
    UINT8 const *   mpInBuf;
    UINT8 *         mpOutBuf;
    UINT_PTR        mInBufByteCount;
    UINT_PTR        mOutBufByteCount;
};

typedef struct _K2OSRPC_OBJECT_CALL K2OSRPC_OBJECT_CALL;
struct _K2OSRPC_OBJECT_CALL
{
    K2OS_GATE_TOKEN     mRemoteDisconnectedGateToken;
    UINT_PTR            mCreatorRef;
    K2OSRPC_CALLARGS    Args;
};
typedef K2STAT (*K2OSRPC_pf_Object_Call)(K2OSRPC_OBJECT_CALL const *apCall, UINT_PTR *apRetUsedOutBytes);

typedef struct _K2OSRPC_OBJECT_DELETE K2OSRPC_OBJECT_DELETE;
struct _K2OSRPC_OBJECT_DELETE
{
    K2OS_GATE_TOKEN     mRemoteDisconnectedGateToken;
    UINT_PTR            mCreatorRef;
};
typedef K2STAT (*K2OSRPC_pf_Object_Delete)(K2OSRPC_OBJECT_DELETE const *apDelete);

struct _K2OSRPC_OBJECT_CLASS
{
    K2_GUID128                  ClassId;

    BOOL                        mSingleThreadPerObject;

    K2OSRPC_pf_Object_Create    Create;
    K2OSRPC_pf_Object_Call      Call;
    K2OSRPC_pf_Object_Delete    Delete;
};

BOOL
K2OSRPC_ObjectClass_Publish(
    K2OSRPC_OBJECT_CLASS const *    apClass,
    UINT_PTR                        aPublishContext
);

BOOL
K2OSRPC_ObjectClass_Unpublish(
    K2_GUID128 const *  apClassId
);

//
//------------------------------------------------------------------------
//

typedef
BOOL
(*K2OSRPC_pf_Client_Object_Create)(
    UINT_PTR            aLocalSystemInterfaceInstanceIdOfRemoteRpcServer,
    K2_GUID128 const *  apClassId,
    UINT_PTR            aCreatorContext,
    UINT_PTR *          apRetNewServerObjectId,
    UINT_PTR *          apRetNewUsageId
);

BOOL
K2OSRPC_Object_Create(
    UINT_PTR            aLocalSystemInterfaceInstanceIdOfRemoteRpcServer,
    K2_GUID128 const *  apClassId,
    UINT_PTR            aCreatorContext,
    UINT_PTR *          apRetNewServerObjectId,
    UINT_PTR *          apRetNewUsageId
);

typedef
BOOL
(*K2OSRPC_pf_Client_Object_Acquire)(
    UINT_PTR    aLocalSystemInterfaceInstanceIdOfRemoteRpcServer,
    UINT_PTR    aExistingServerObjectId,
    UINT_PTR *  apRetNewUsageId
);
BOOL
K2OSRPC_Object_Acquire(
    UINT_PTR    aLocalSystemInterfaceInstanceIdOfRemoteRpcServer,
    UINT_PTR    aExistingServerObjectId,
    UINT_PTR *  apRetNewUsageId
);

typedef 
K2STAT
(*K2OSRPC_pf_Client_Object_Call)(
    UINT_PTR                    aUsageId,
    K2OSRPC_CALLARGS const *    apArgs,
    UINT_PTR *                  apRetUsedOutByteCount
);
K2STAT
K2OSRPC_Object_Call(
    UINT_PTR                    aUsageId,
    K2OSRPC_CALLARGS const *    apArgs,
    UINT_PTR *                  apRetUsedOutByteCount
);

typedef 
BOOL
(*K2OSRPC_pf_Client_Object_Release)(
    UINT_PTR    aUsageId
);
BOOL
K2OSRPC_Object_Release(
    UINT_PTR    aUsageId
);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __K2OSRPC_H