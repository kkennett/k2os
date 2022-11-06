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

#ifndef __IK2OSRPC_H
#define __IK2OSRPC_H

#include <k2osrpc.h>

//
//------------------------------------------------------------------------
//

typedef struct _K2OSRPC_SERVER_CLIENT   K2OSRPC_SERVER_CLIENT;

typedef enum _K2OSRpc_ServerClientRequestType K2OSRpc_ServerClientRequestType;
enum _K2OSRpc_ServerClientType
{
    K2OSRpc_ServerClientRequest_Invalid = 0,

    K2OSRpc_ServerClientRequest_Create,        // targetId is 0
    K2OSRpc_ServerClientRequest_Acquire,       // targetId is Server object id
    K2OSRpc_ServerClientRequest_Release,       // targetId is Server usage id
    K2OSRpc_ServerClientRequest_Call,          // targetId is Server usage id

    K2OSRpc_ServerClientRequestType_Count
};

K2_PACKED_PUSH
typedef struct _K2OSRPC_MSG_REQUEST_HDR K2OSRPC_MSG_REQUEST_HDR;
struct _K2OSRPC_MSG_REQUEST_HDR
{
    UINT32  mCallerRef;             // caller context
    UINT32  mRequestType;           // see above K2OSRpc_ServerClientRequestType
    UINT32  mTargetId;              // see above depends on mRequestType
    UINT32  mTargetMethodId;        // only used in K2OSRpc_ServerMethod_Call
    UINT32  mInByteCount;           // # of bytes following this header
    UINT32  mOutBufSizeProvided;    // max space allowed in response
} K2_PACKED_ATTRIB;
K2_PACKED_POP
K2_STATIC_ASSERT(24 == sizeof(K2OSRPC_MSG_REQUEST_HDR));

K2_PACKED_PUSH
typedef struct _K2OSRPC_MSG_RESPONSE_HDR K2OSRPC_MSG_RESPONSE_HDR;
struct _K2OSRPC_MSG_RESPONSE_HDR
{
    UINT32  mCallerRef;             // caller context
    K2STAT  mStatus;                // result fo the call.  if K2STAT_IS_ERROR(mStatus) mResultsByteCount will be zero
    UINT32  mResultsByteCount;      // # of bytes following this header
} K2_PACKED_ATTRIB;
K2_PACKED_POP
K2_STATIC_ASSERT(12 == sizeof(K2OSRPC_MSG_RESPONSE_HDR));

// mRequestType == K2OSRpc_ServerClientRequest_Create
// request data follows K2OSRPC_MSG_REQUEST_HDR
// response to this is K2OSRPC_MSG_RESPONSE_HDR followed by K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA
K2_PACKED_PUSH
typedef struct _K2OSRPC_MSG_CREATE_REQUEST_DATA K2OSRPC_MSG_CREATE_REQUEST_DATA;
struct _K2OSRPC_MSG_CREATE_REQUEST_DATA
{
    K2_GUID128  mClassId;
    UINT32      mCreatorContext;
} K2_PACKED_ATTRIB;
K2_PACKED_POP


// mRequestType == K2OSRpc_ServerClientRequest_Acquire
// request data is empty (hdr.mTargetId is remote object id)
// response to this is K2OSRPC_MSG_RESPONSE_HDR followed by K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA

K2_PACKED_PUSH
typedef struct _K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA;
struct _K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA
{
    UINT32  mServerObjectId;
    UINT32  mServerUsageId;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

typedef struct _K2OSRPC_OBJECT_USAGE_HDR    K2OSRPC_OBJECT_USAGE_HDR;
typedef struct _K2OSRPC_SERVER_OBJECT_USAGE K2OSRPC_SERVER_OBJECT_USAGE;
typedef struct _K2OSRPC_CLIENT_OBJECT_USAGE K2OSRPC_CLIENT_OBJECT_USAGE;

//
// used by both local and remote clients
//
struct _K2OSRPC_OBJECT_USAGE_HDR
{
    INT_PTR     mRefCount;
    K2TREE_NODE GlobalUsageTreeNode;   // mUserVal is usage id
    BOOL        mIsServer;
};

extern K2OS_CRITSEC    gK2OSRPC_GlobalUsageTreeSec;
extern K2TREE_ANCHOR   gK2OSRPC_GlobalUsageTree;
extern INT_PTR         gK2OSRPC_NextGlobalUsageId;

//
//------------------------------------------------------------------------
//

BOOL
K2OSRPC_ServerObject_Create(
    K2_GUID128 const *  apClassId,
    UINT_PTR            aCreatorContext,
    UINT_PTR *          apRetServerObjectId,
    UINT_PTR *          apRetUsageId
);

BOOL
K2OSRPC_ServerObject_Acquire(
    UINT_PTR    aServerObjectId,
    UINT_PTR *  apRetUsageId
);

K2STAT
K2OSRPC_ServerUsage_Call(
    K2OSRPC_SERVER_OBJECT_USAGE *   apServerObjectUsage,
    K2OSRPC_CALLARGS const *        apArgs,
    UINT_PTR *                      apRetActualOut
);

void
K2OSRPC_ServerUsage_Release(
    K2OSRPC_SERVER_OBJECT_USAGE *   apServerObjectUsage
);

//
//------------------------------------------------------------------------
//

BOOL
K2OSRPC_ClientUsage_Create(
    UINT_PTR            aRpcServerInterfaceInstanceId,
    K2_GUID128 const *  apClassId,
    UINT_PTR            aCreatorContext,
    UINT_PTR *          apRetServerObjectId,
    UINT_PTR *          apRetClientUsageId
);

BOOL
K2OSRPC_ClientUsage_Acquire(
    UINT_PTR    aRpcServerInterfaceInstanceId,
    UINT_PTR    aServerObjectId,
    UINT_PTR *  apRetClientUsageId
);

K2STAT
K2OSRPC_ClientUsage_Call(
    K2OSRPC_CLIENT_OBJECT_USAGE *   apClientUsage,
    K2OSRPC_CALLARGS const *        apArgs,
    UINT_PTR *                      apRetActualOut
);

K2STAT
K2OSRPC_ClientUsage_Release(
    K2OSRPC_CLIENT_OBJECT_USAGE *   apClientUsage
);

//
//------------------------------------------------------------------------
//

K2STAT K2OSRPC_Client_AtXdlEntry(UINT32 aReason);

//
//------------------------------------------------------------------------
//

K2STAT K2OSRPC_Server_AtXdlEntry(UINT32 aReason);

//
//------------------------------------------------------------------------
//

typedef struct _K2OSRPC_THREAD K2OSRPC_THREAD;

typedef void (*K2OSRPC_pf_Thread_AtExit)(K2OSRPC_THREAD *apThread);
typedef void (*K2OSRPC_pf_Thread_DoWork)(K2OSRPC_THREAD *apThread);

struct _K2OSRPC_THREAD
{
    INT_PTR volatile            mRefCount;
    UINT_PTR                    mThreadId;
    K2OS_NOTIFY_TOKEN           mWorkNotifyToken;
    K2OSRPC_pf_Thread_AtExit    mfAtExit;
    K2OSRPC_pf_Thread_DoWork    mfDoWork;
};

K2STAT
K2OSRPC_Thread_Create(
    K2OSRPC_THREAD *            apThread,
    K2OSRPC_pf_Thread_AtExit    afAtExit,
    K2OSRPC_pf_Thread_DoWork    afDoWork
);

INT_PTR
K2OSRPC_Thread_AddRef(
    K2OSRPC_THREAD *    apThread
);

INT_PTR
K2OSRPC_Thread_Release(
    K2OSRPC_THREAD *    apThread
);

void
K2OSRPC_Thread_WakeUp(
    K2OSRPC_THREAD *    apThread
);

//
//------------------------------------------------------------------------
//

#endif // __IK2OSRPC_H