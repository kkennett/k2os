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
#ifndef __K2OSRPC_H
#define __K2OSRPC_H

#include <k2os.h>

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

#define EMIT_TRACE  0
#if EMIT_TRACE
#define FUNC_ENTER      K2OSRPC_Debug("%d:%d:ENTER %s\n", K2OS_Process_GetId(), K2OS_Thread_GetId(), __FUNCTION__);
#define FUNC_EXIT       K2OSRPC_Debug("%d:%d:EXIT  %s(%d)\n", K2OS_Process_GetId(), K2OS_Thread_GetId(), __FUNCTION__, __LINE__);
#else
#define FUNC_ENTER 
#define FUNC_EXIT
#endif

#define TRAP_EXCEPTIONS 1

//
//------------------------------------------------------------------------
//

typedef enum _K2OSRPC_ServerClientRequestType K2OSRPC_ServerClientRequestType;
enum _K2OSRPC_ServerClientRequestType
{
    K2OSRPC_ServerClientRequest_Invalid = 0,

    K2OSRPC_ServerClientRequest_Create,              // targetId is 0
    K2OSRPC_ServerClientRequest_AcquireByObjId,      // targetId is Server object id
    K2OSRPC_ServerClientRequest_AcquireByIfInstId,   // targetId is interface instance id
    K2OSRPC_ServerClientRequest_Release,             // targetId is Server handle
    K2OSRPC_ServerClientRequest_Call,                // targetId is Server handle

    K2OSRPC_ServerClientRequestType_Count
};

struct _K2OSRPC_OBJ_HANDLE_HDR
{
    INT32 volatile  mRefs;
    BOOL            mIsServer;
    K2TREE_NODE     GlobalHandleTreeNode;
};

K2_PACKED_PUSH
typedef struct _K2OSRPC_MSG_REQUEST_HDR K2OSRPC_MSG_REQUEST_HDR;
struct _K2OSRPC_MSG_REQUEST_HDR
{
    UINT32  mCallerRef;             // caller context
    UINT32  mRequestType;           // see above K2OSRPC_ServerClientRequestType
    UINT32  mTargetId;              // see above depends on mRequestType
    UINT32  mTargetMethodId;        // only used in K2OSRPC_ServerMethod_Call
    UINT32  mInByteCount;           // # of bytes following this header
    UINT32  mOutBufSizeProvided;    // max space allowed in response
} K2_PACKED_ATTRIB;
K2_PACKED_POP
K2_STATIC_ASSERT(24 == sizeof(K2OSRPC_MSG_REQUEST_HDR));

#define K2OSRPC_MSG_RESPONSE_HDR_MARKER K2_MAKEID4('R','E','S','P')
K2_PACKED_PUSH
typedef struct _K2OSRPC_MSG_RESPONSE_HDR K2OSRPC_MSG_RESPONSE_HDR;
struct _K2OSRPC_MSG_RESPONSE_HDR
{
    UINT32  mMarker;                // K2OSRPC_MSG_RESPONSE_HDR_MARKER
    UINT32  mCallerRef;             // caller context
    K2STAT  mStatus;                // result fo the call.  if K2STAT_IS_ERROR(mStatus) mResultsByteCount will be zero
    UINT32  mResultsByteCount;      // # of bytes following this header
} K2_PACKED_ATTRIB;
K2_PACKED_POP
K2_STATIC_ASSERT(16 == sizeof(K2OSRPC_MSG_RESPONSE_HDR));

// mRequestType == K2OSRPC_ServerClientRequest_Create
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

// mRequestType == K2OSRPC_ServerClientRequest_AcquireByObjId, K2OSRPC_ServerClientRequest_AcquireByIfInstId
// request data is empty
// response to this is K2OSRPC_MSG_RESPONSE_HDR followed by K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA

K2_PACKED_PUSH
typedef struct _K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA;
struct _K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA
{
    UINT32  mServerObjId;
    UINT32  mServerHandle;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

#define K2OSRPC_OBJ_NOTIFY_MARKER    K2_MAKEID4('N','O','T','F')
K2_PACKED_PUSH
typedef struct _K2OSRPC_OBJ_NOTIFY K2OSRPC_OBJ_NOTIFY;
struct _K2OSRPC_OBJ_NOTIFY
{
    UINT32  mMarker;            // K2OSRPC_OBJ_NOTIFY_MARKER
    UINT32  mServerHandle;
    UINT32  mCode;
    UINT32  mData;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

typedef struct _K2OSRPC_OBJ_HANDLE_HDR      K2OSRPC_OBJ_HANDLE_HDR;
typedef struct _K2OSRPC_SERVER_OBJ_HANDLE   K2OSRPC_SERVER_OBJ_HANDLE;
typedef struct _K2OSRPC_CLIENT_OBJ_HANDLE   K2OSRPC_CLIENT_OBJ_HANDLE;
typedef struct _K2OSRPC_CLIENT_CONN         K2OSRPC_CLIENT_CONN;

struct _K2OSRPC_CLIENT_CONN
{
    INT32               mRefCount;
    INT32               mRunningRef;
    K2OS_MAILBOX_TOKEN  mTokMailbox;
    K2OS_IPCEND         mIpcEnd;
    K2OS_THREAD_TOKEN   mTokThread;
    UINT32              mThreadId;
    K2OS_SIGNAL_TOKEN   mTokStopNotify;
    K2OS_SIGNAL_TOKEN   mTokConnectStatusGate;
    BOOL                mIsConnected;
    BOOL                mIsRejected;
    K2OS_CRITSEC        IoListSec;
    K2LIST_ANCHOR       IoList;
    K2TREE_NODE         ConnTreeNode; // mUserVal is server interface instance id
};

struct _K2OSRPC_CLIENT_OBJ_HANDLE
{
    K2OSRPC_OBJ_HANDLE_HDR  Hdr;
    K2OS_MAILBOX_TOKEN      mTokNotifyMailbox;
    UINT32                  mServerObjId;
    K2_GUID128              ClassId;
    K2OSRPC_CLIENT_CONN *   mpConnToServer;
    K2TREE_NODE             ServerHandleTreeNode;   // mUserVal is server handle 
};

//
//------------------------------------------------------------------------
//

K2STAT K2OSRPC_Init(void);

UINT32 K2OSRPC_Debug(char const *apFormat, ...);

K2OS_RPC_OBJ_HANDLE K2OSRPC_Client_CreateObj(K2OS_IFINST_ID aRemoteRpcInstId, K2_GUID128 const *apClassId, UINT32 aCreatorContext);
K2OS_RPC_OBJ_HANDLE K2OSRPC_Server_LocalCreateObj(K2_GUID128 const *apClassId, UINT32 aCreatorContext, UINT32 aCreatorProcessId, K2OS_SIGNAL_TOKEN aTokRemoteDisconnect, UINT32 *apRetObjId);

K2OS_RPC_OBJ_HANDLE K2OSRPC_Client_AttachByObjId(K2OS_IFINST_ID aRemoteRpcInstId, UINT32 aRemoteObjectId);
K2OSRPC_SERVER_OBJ_HANDLE * K2OSRPC_Server_LocalAttachByObjId(UINT32 aObjectId);

K2OS_RPC_OBJ_HANDLE K2OSRPC_Client_AttachByIfInstId(K2OS_IFINST_ID aIfInstId, UINT32 *apRetObjId);
K2OSRPC_SERVER_OBJ_HANDLE * K2OSRPC_Server_LocalAttachByIfInstId(K2OS_IFINST_ID aIfInstId, UINT32 *apRetObjId);

K2STAT K2OSRPC_Client_Call(K2OSRPC_CLIENT_OBJ_HANDLE *apObjHandle, K2OS_RPC_CALLARGS const *apCallArgs, UINT32 *apRetActualOut);
K2STAT K2OSRPC_Server_LocalCall(K2OSRPC_SERVER_OBJ_HANDLE *apObjHandle, K2OS_RPC_CALLARGS const *apCallArgs, UINT32 *apRetActualOut);

void K2OSRPC_Client_PurgeHandle(K2OSRPC_CLIENT_OBJ_HANDLE *apObjHandle);
void K2OSRPC_Server_PurgeHandle(K2OSRPC_SERVER_OBJ_HANDLE *apObjHandle, BOOL aUndoUse);

K2OS_MAILBOX_TOKEN K2OSRPC_Server_SwapNotifyTarget(K2OSRPC_SERVER_OBJ_HANDLE * pServerHandle, K2OS_MAILBOX_TOKEN aTokMailbox);
K2OS_MAILBOX_TOKEN K2OSRPC_Client_SwapNotifyTarget(K2OSRPC_CLIENT_OBJ_HANDLE * pClientHandle, K2OS_MAILBOX_TOKEN aTokMailbox);

UINT32 K2OSRPC_Server_GetObjectId(K2OSRPC_SERVER_OBJ_HANDLE * pServerHandle);
UINT32 K2OSRPC_Client_GetObjectId(K2OSRPC_CLIENT_OBJ_HANDLE * pClientHandle);

K2OS_IFINST_ID K2OSRPC_GetObjIds(K2OS_RPC_OBJ_HANDLE aObjHandle, UINT32 *apRetObjId);

//
//------------------------------------------------------------------------
//

extern K2_GUID128 const gRpcServerClassId;

extern K2OS_CRITSEC     gRpcGraphSec;
extern K2TREE_ANCHOR    gRpcHandleTree;     // keyed on pointer to handle structure
extern K2OS_IFINST_ID   gRpcServerIfInstId;

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __K2OSRPC_H
