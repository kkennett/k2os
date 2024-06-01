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
#ifndef __RPCSERVER_H
#define __RPCSERVER_H

#include "k2osrpc.h"

#if __cplusplus
extern "C" {
#endif


//
//------------------------------------------------------------------------
//

typedef struct _K2OSRPC_THREAD K2OSRPC_THREAD;

typedef void (*K2OSRPC_pf_Thread_AtExit)(K2OSRPC_THREAD* apThread);
typedef void (*K2OSRPC_pf_Thread_DoWork)(K2OSRPC_THREAD* apThread);

struct _K2OSRPC_THREAD
{
    INT32 volatile        	    mRefCount;
    UINT32                	    mThreadId;
    K2OS_SIGNAL_TOKEN           mTokWorkNotify;
    K2OSRPC_pf_Thread_AtExit    mfAtExit;
    K2OSRPC_pf_Thread_DoWork    mfDoWork;
};

K2STAT
K2OSRPC_Thread_Create(
    char const *                apName,
    K2OSRPC_THREAD *            apThread,
    K2OSRPC_pf_Thread_AtExit    afAtExit,
    K2OSRPC_pf_Thread_DoWork    afDoWork
);

INT32
K2OSRPC_Thread_AddRef(
    K2OSRPC_THREAD* apThread
);

INT32
K2OSRPC_Thread_Release(
    K2OSRPC_THREAD* apThread
);

void
K2OSRPC_Thread_WakeUp(
    K2OSRPC_THREAD* apThread
);
    
//
//------------------------------------------------------------------------
//

typedef struct _RPC_CLASS       RPC_CLASS;
typedef struct _RPC_SERVER      RPC_SERVER;
typedef struct _RPC_OBJ         RPC_OBJ;
typedef struct _RPC_IFINST      RPC_IFINST;
typedef struct _RPC_THREAD      RPC_THREAD;
typedef struct _RPC_CONN        RPC_CONN;
typedef struct _RPC_WORKITEM    RPC_WORKITEM;

struct _K2OSRPC_SERVER_OBJ_HANDLE
{
    K2OSRPC_OBJ_HANDLE_HDR  Hdr;

    RPC_OBJ *               mpObj;
    K2LIST_LINK             ObjHandleListLink;

    RPC_CONN *              mpConnToClient;
    K2LIST_LINK             ConnHandleListListLink;
    BOOL                    mOnConnHandleList;

    UINT32                  mUseContext;

    K2OS_MAILBOX_TOKEN      mTokNotifyMailbox;
};

struct _RPC_CLASS
{
    INT32 volatile          mRefCount;

    K2OS_RPC_OBJ_CLASSDEF   Def;

    K2TREE_NODE             ServerClassByIdTreeNode;
    K2TREE_NODE             ServerClassByPtrTreeNode;

    K2LIST_ANCHOR           ObjList;

    BOOL                    mIsRegistered;
    UINT32                  mUserContext;

    K2OS_XDL                mXdlHandle[3];
};

struct _RPC_SERVER
{
    K2TREE_ANCHOR       ClassByIdTree;
    K2TREE_ANCHOR       ClassByPtrTree;

    K2TREE_ANCHOR       ObjByIdTree;
    K2TREE_ANCHOR       ObjByPtrTree;

    K2TREE_ANCHOR       IfInstIdTree;
    K2TREE_ANCHOR       IfInstPtrTree;

    UINT32              mThreadId;
    K2OS_THREAD_TOKEN   mTokThread;

    K2TREE_ANCHOR       ConnTree;

    K2OS_MAILBOX_TOKEN  mTokMailbox;

    K2OS_IFINST_TOKEN   mTokIfInst;

    K2OS_SIGNAL_TOKEN   mTokStartupNotify;
    K2STAT              mStartupStatus;

    BOOL volatile       mShutdownStarted;
    K2OS_SIGNAL_TOKEN   mTokShutdownGate;

    K2LIST_ANCHOR       IdleThreadList;
    K2LIST_ANCHOR       ActiveThreadList;

    K2LIST_ANCHOR       IdleWorkItemList;
};

struct _RPC_OBJ
{
    INT32 volatile      mRefCount;

    K2LIST_ANCHOR       HandleList;

    RPC_CLASS *         mpClass;
    K2LIST_LINK         ClassObjListLink;

    K2TREE_NODE         ServerObjIdTreeNode;
    K2TREE_NODE         ServerObjPtrTreeNode;

    UINT32              mUserContext;

    K2LIST_ANCHOR       IfInstList;

    RPC_WORKITEM *      mpDeleteWorkItem;
    K2OS_SIGNAL_TOKEN   mTokDeleteGate;
};

struct _RPC_IFINST
{
    K2OS_IFINST_TOKEN   mTokSysInst;

    K2TREE_NODE         ServerIfInstPtrTreeNode;
    K2TREE_NODE         ServerIfInstIdTreeNode;

    RPC_OBJ *           mpObj;
    K2LIST_LINK         ObjIfInstListLink;
};

struct _RPC_THREAD
{
    K2OSRPC_THREAD  RpcThread;

    K2LIST_LINK     ListLink;
    BOOL            mIsOnList;

    BOOL            mIsActive;

    RPC_WORKITEM *  mpWorkItem;
};

struct _RPC_CONN
{
    K2OSRPC_THREAD      CrtThread;

    K2TREE_NODE         ServerConnTreeNode;

    BOOL                mIsConnected;

    K2LIST_ANCHOR       HandleList;

    UINT32              mRequestId;
    K2OS_MAILBOX_TOKEN  mTokMailbox;
    K2OS_IPCEND         mIpcEnd;
    K2OS_SIGNAL_TOKEN   mDisconnectedGateToken;

    K2OS_CRITSEC        WorkItemListSec;
    K2LIST_ANCHOR       WorkItemList;
};

struct _RPC_WORKITEM
{
    K2OSRPC_SERVER_OBJ_HANDLE * mpHandle;

    K2OSRPC_MSG_REQUEST_HDR *   mpReqHdr;
    K2OSRPC_MSG_RESPONSE_HDR *  mpRespHdr;
    UINT8 const *               mpInBuf;
    UINT8 *                     mpOutBuf;

    K2OS_SIGNAL_TOKEN           mDoneGateToken;

    RPC_CONN *                  mpConnToClient;

    BOOL volatile               mIsCancelled;

    K2LIST_LINK                 ListLink;
};

extern RPC_SERVER * volatile    gpRpcServer;
extern K2OS_CRITSEC             gpRpcServerSec;

void K2OSRPC_Server_OnConnectRequest(UINT32 aRequestorProcessId, UINT32 aRequestId);

BOOL K2OSRPC_ReleaseInternal(K2OS_RPC_OBJ_HANDLE aObjHandle, BOOL aUndoUse);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __RPCSERVER_H
