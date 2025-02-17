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

#define CONNECT_TO_SERVER_TIMEOUT_MS 5000

typedef struct _K2OSRPC_IOMSG K2OSRPC_IOMSG;
struct _K2OSRPC_IOMSG
{
    K2OS_SIGNAL_TOKEN       mTokDoneNotify;
    K2LIST_LINK             ConnListLink;
    K2OSRPC_MSG_REQUEST_HDR RequestHdr;
    K2STAT                  mResultStatus;
    UINT8 *                 mpOutBuffer;
    UINT32                  mActualOutBytes;
};

static K2OS_CRITSEC     sgConnSec;
static K2TREE_ANCHOR    sgConnTree;
static K2TREE_ANCHOR    sgServerHandleTree;

void
K2OSRPC_ClientConn_SendRequest(
    K2OSRPC_CLIENT_CONN *   apConn,
    K2OSRPC_IOMSG *         apIoMsg,
    UINT8 const *           apInBuf,
    UINT32                  aInBufBytes
)
{
    BOOL            wasConn;
    K2MEM_BUFVECTOR memVec[2];
    UINT32          vecCount;
    K2OS_WaitResult waitResult;

    //
    // result must be set in apIoMsg->mResultStatus
    //

    FUNC_ENTER;

    apIoMsg->mActualOutBytes = 0;

    if (!apConn->mIsConnected)
    {
        apIoMsg->mResultStatus = K2STAT_ERROR_DISCONNECTED;
        FUNC_EXIT;
        return;
    }

    apIoMsg->mTokDoneNotify = K2OS_Notify_Create(FALSE);
    if (NULL == apIoMsg->mTokDoneNotify)
    {
        apIoMsg->mResultStatus = K2OS_Thread_GetLastStatus();
        FUNC_EXIT;
        return;
    }

    K2OS_CritSec_Enter(&apConn->IoListSec);

    wasConn = apConn->mIsConnected;
    if (wasConn)
    {
        K2LIST_AddAtTail(&apConn->IoList, &apIoMsg->ConnListLink);
    }

    K2OS_CritSec_Leave(&apConn->IoListSec);

    if (wasConn)
    {
        vecCount = 1;
        memVec[0].mpBuffer = (UINT8 *)&apIoMsg->RequestHdr;
        memVec[0].mByteCount = sizeof(K2OSRPC_MSG_REQUEST_HDR);

        if (aInBufBytes > 0)
        {
            K2_ASSERT(NULL != apInBuf);
            memVec[1].mpBuffer = (UINT8 *)apInBuf;
            memVec[1].mByteCount = aInBufBytes;
            vecCount++;
            apIoMsg->RequestHdr.mInByteCount = aInBufBytes;
        }

        apIoMsg->RequestHdr.mCallerRef = K2ATOMIC_Inc(&apConn->mRunningRef);
        apIoMsg->mResultStatus = K2STAT_NO_ERROR;

        if (K2OS_IpcEnd_SendVector(apConn->mIpcEnd, vecCount, memVec))
        {
//            K2OSRPC_Debug("+ClientWaitForRequest\n");
            K2OS_Thread_WaitOne(&waitResult, apIoMsg->mTokDoneNotify, K2OS_TIMEOUT_INFINITE);
//            K2OSRPC_Debug("-ClientWaitForRequest(iomsg.mresultstatus = %08X)\n", apIoMsg->mResultStatus);
        }
        else
        {
            apIoMsg->mResultStatus = K2OS_Thread_GetLastStatus();
        }

        K2OS_CritSec_Enter(&apConn->IoListSec);

        K2LIST_Remove(&apConn->IoList, &apIoMsg->ConnListLink);

        K2OS_CritSec_Leave(&apConn->IoListSec);
    }
    else
    {
        apIoMsg->mResultStatus = K2STAT_ERROR_NOT_CONNECTED;
    }

    K2OS_Token_Destroy(apIoMsg->mTokDoneNotify);

    FUNC_EXIT;
}

void
K2OSRPC_ClientConn_OnConnect(
    K2OS_IPCEND aEndpoint,
    void *      apContext,
    UINT32      aRemoteMaxMsgBytes
)
{
    K2OSRPC_CLIENT_CONN *pConn;

    FUNC_ENTER;

    pConn = (K2OSRPC_CLIENT_CONN *)apContext;
    pConn->mIsConnected = TRUE;
    K2_CpuWriteBarrier();

    FUNC_EXIT;
}

void
K2OSRPC_ClientConn_OnRecv(
    K2OS_IPCEND     aEndpoint,
    void *          apContext,
    UINT8 const *   apData,
    UINT32          aByteCount
)
{
    K2STAT                              stat;
    K2OSRPC_MSG_RESPONSE_HDR const *    pRespHdr;
    K2LIST_LINK *                       pListLink;
    K2OSRPC_IOMSG *                     pIoMsg;
    K2OSRPC_CLIENT_CONN *               pConn;
    K2TREE_NODE *                       pTreeNode;
    K2OS_MSG                            msg;
    K2OSRPC_CLIENT_OBJ_HANDLE *         pObjHandle;

    FUNC_ENTER;

    pConn = (K2OSRPC_CLIENT_CONN *)apContext;

    //    K2OSRPC_Debug("ClientConn Recv %d\n", aByteCount);
    if (aByteCount < sizeof(UINT32))
    {
        K2OSRPC_Debug("RpcServer %d sent malformed response (too small %d)\n", pConn->ConnTreeNode.mUserVal, aByteCount);
        K2_ASSERT(0);
        FUNC_EXIT;
        return;
    }

    if (*((UINT32 *)apData) == K2OSRPC_OBJ_NOTIFY_MARKER)
    {
        if (aByteCount == sizeof(K2OSRPC_OBJ_NOTIFY))
        {
            msg.mMsgType = 0;

            K2OS_CritSec_Enter(&gRpcGraphSec);

            pTreeNode = K2TREE_Find(&sgServerHandleTree, ((K2OSRPC_OBJ_NOTIFY *)apData)->mServerHandle);
            if (NULL != pTreeNode)
            {
                pObjHandle = K2_GET_CONTAINER(K2OSRPC_CLIENT_OBJ_HANDLE, pTreeNode, ServerHandleTreeNode);

                msg.mMsgType = K2OS_SYSTEM_MSGTYPE_RPC;
                msg.mShort = K2OS_SYSTEM_MSG_RPC_SHORT_NOTIFY;
                msg.mPayload[0] = (UINT32)pObjHandle;
                msg.mPayload[1] = ((K2OSRPC_OBJ_NOTIFY const *)apData)->mCode;
                msg.mPayload[2] = ((K2OSRPC_OBJ_NOTIFY const *)apData)->mData;
            }

            K2OS_CritSec_Leave(&gRpcGraphSec);

            if (0 != msg.mMsgType)
            {
                if (NULL != pObjHandle->mTokNotifyMailbox)
                {
                    if (!K2OS_Mailbox_Send(pObjHandle->mTokNotifyMailbox, &msg))
                    {
                        K2OSRPC_Debug("***failed to send msg - %08X\n", K2OS_Thread_GetLastStatus());
                    }
                }
                else
                {
                    K2OSRPC_Debug("Process %d Object %08X received notify but no mailbox set\n", K2OS_Process_GetId(), pObjHandle);
                }
            }

            FUNC_EXIT;
            return;
        }
        K2OSRPC_Debug("RpcServer %d sent malformed response (too small %d)\n", pConn->ConnTreeNode.mUserVal, aByteCount);
        FUNC_EXIT;
        return;
    }

    if (*((UINT32 *)apData) != K2OSRPC_MSG_RESPONSE_HDR_MARKER)
    {
        K2OSRPC_Debug("RpcServer %d sent malformed response (bad marker)\n", pConn->ConnTreeNode.mUserVal);
        FUNC_EXIT;
        return;
    }

    pRespHdr = (K2OSRPC_MSG_RESPONSE_HDR const *)apData;
    if (0 == pRespHdr->mCallerRef)
    {
        K2OSRPC_Debug("RpcServer %d sent malformed response (caller ref zero)\n", pConn->ConnTreeNode.mUserVal);
        FUNC_EXIT;
        return;
    }

    K2OS_CritSec_Enter(&pConn->IoListSec);

    pListLink = pConn->IoList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pIoMsg = K2_GET_CONTAINER(K2OSRPC_IOMSG, pListLink, ConnListLink);
            if (pIoMsg->RequestHdr.mCallerRef == pRespHdr->mCallerRef)
            {
                break;
            }
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }

    K2OS_CritSec_Leave(&pConn->IoListSec);

    if (NULL == pListLink)
    {
        K2OSRPC_Debug("Response for ref %d from rpcServer %d ignored (caller abandoned)\n", pRespHdr->mCallerRef, pConn->ConnTreeNode.mUserVal);
        FUNC_EXIT;
        return;
    }

    //
    // going to complete the request
    //

    stat = pRespHdr->mStatus;
    if (!K2STAT_IS_ERROR(stat))
    {
        aByteCount -= sizeof(K2OSRPC_MSG_RESPONSE_HDR);

        pIoMsg->mActualOutBytes = aByteCount;

        if (0 < aByteCount)
        {
            if (aByteCount > pIoMsg->RequestHdr.mOutBufSizeProvided)
            {
                K2OSRPC_Debug("Response for ref %d from rpcServer %d was too big for output buffer\n", pRespHdr->mCallerRef, pConn->ConnTreeNode.mUserVal);
                stat = K2STAT_ERROR_OUTBUF_TOO_SMALL;
            }
            else
            {
                K2MEM_Copy(pIoMsg->mpOutBuffer, ((UINT8 *)pRespHdr) + sizeof(K2OSRPC_MSG_RESPONSE_HDR), aByteCount);
            }
        }
    }

    pIoMsg->mResultStatus = stat;
    K2_CpuWriteBarrier();

    K2OS_Notify_Signal(pIoMsg->mTokDoneNotify);
    FUNC_EXIT;
}

void
K2OSRPC_ClientConn_OnDisconnect(
    K2OS_IPCEND aEndpoint,
    void *      apContext
)
{
    K2LIST_LINK *           pListLink;
    K2OSRPC_IOMSG *         pIoMsg;
    K2OSRPC_CLIENT_CONN *   pConn;

    FUNC_ENTER;

    pConn = (K2OSRPC_CLIENT_CONN *)apContext;

    K2OS_CritSec_Enter(&sgConnSec);
    K2TREE_Remove(&sgConnTree, &pConn->ConnTreeNode);
    K2OS_CritSec_Leave(&sgConnSec);

    K2OS_CritSec_Enter(&pConn->IoListSec);

    pConn->mIsConnected = FALSE;
    K2_CpuWriteBarrier();

    //
    // all pending ios are aboreted with disconnected result
    //
    pListLink = pConn->IoList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pIoMsg = K2_GET_CONTAINER(K2OSRPC_IOMSG, pListLink, ConnListLink);
            pIoMsg->mResultStatus = K2STAT_ERROR_DISCONNECTED;
            K2OS_Notify_Signal(pIoMsg->mTokDoneNotify);
        } while (NULL != pListLink);
    }

    K2OS_CritSec_Leave(&pConn->IoListSec);
    FUNC_EXIT;
}

void
K2OSRPC_ClientConn_OnRejected(
    K2OS_IPCEND aEndpoint,
    void *      apContext,
    UINT32      aReasonCode
)
{
    K2OSRPC_CLIENT_CONN *pConn;

    FUNC_ENTER;

    pConn = (K2OSRPC_CLIENT_CONN *)apContext;
    pConn->mIsRejected = TRUE;
    K2_CpuWriteBarrier();

    FUNC_EXIT;
}

void
K2OSRPC_ClientConn_Purge(
    K2OSRPC_CLIENT_CONN * apConn
)
{
    FUNC_ENTER;

    K2_ASSERT(NULL == apConn->mTokThread);

    K2_ASSERT(0 == apConn->IoList.mNodeCount);

    K2_ASSERT(NULL == apConn->mIpcEnd);
    K2_ASSERT(NULL == apConn->mTokMailbox);

    K2OS_Token_Destroy(apConn->mTokConnectStatusGate);

    K2OS_Token_Destroy(apConn->mTokStopNotify);

    K2OS_CritSec_Done(&apConn->IoListSec);

    K2OS_Heap_Free(apConn);

    FUNC_EXIT;
}

void
K2OSRPC_ClientConn_Release(
    K2OSRPC_CLIENT_CONN * apConn
)
{
    FUNC_ENTER;

    if (0 != K2ATOMIC_Dec(&apConn->mRefCount))
    {
        FUNC_EXIT;
        return;
    }

    if (NULL == apConn->mTokThread)
    {
        K2OSRPC_ClientConn_Purge(apConn);
    }
    else
    {
        K2OS_Token_Destroy(apConn->mTokThread);
        apConn->mTokThread = NULL;
        K2_CpuWriteBarrier();

        K2OS_Notify_Signal(apConn->mTokStopNotify);
    }

    FUNC_EXIT;
}

UINT32
K2OSRPC_ClientConn_Thread(
    void *apArg
)
{
    static const K2OS_IPCPROCESSMSG_CALLBACKS clientConnIpcFuncTab =
    {
        K2OSRPC_ClientConn_OnConnect,
        K2OSRPC_ClientConn_OnRecv,
        K2OSRPC_ClientConn_OnDisconnect,
        K2OSRPC_ClientConn_OnRejected
    };
    K2OSRPC_CLIENT_CONN *   pConn;
    K2OS_MSG                msg;
    UINT32                  timeoutLeft;
    UINT32                  elapsedMs;
    UINT64                  oldTick;
    UINT64                  newTick;
    BOOL                    wasStopSignal;
    BOOL                    ok;
    K2OS_WaitResult         waitResult;
    K2OS_WAITABLE_TOKEN     tokWait[2];

    FUNC_ENTER;

    pConn = (K2OSRPC_CLIENT_CONN *)apArg;

    pConn->mTokMailbox = K2OS_Mailbox_Create();

    if (NULL != pConn->mTokMailbox)
    {
        pConn->mIpcEnd = K2OS_IpcEnd_Create(pConn->mTokMailbox, 32, 1024, pConn, &clientConnIpcFuncTab);

        if (NULL != pConn->mIpcEnd)
        {
            if (!K2OS_IpcEnd_SendRequest(pConn->mIpcEnd, (K2OS_IFINST_ID)pConn->ConnTreeNode.mUserVal))
            {
                K2OSRPC_Debug("***ClientConn ipc send request failed\n");
                K2OS_IpcEnd_Delete(pConn->mIpcEnd);
                pConn->mIpcEnd = NULL;
            }
        }
        else
        {
            K2OSRPC_Debug("***ClientConn ipcend create failed\n");
        }
        if (NULL == pConn->mIpcEnd)
        {
            K2OS_Token_Destroy(pConn->mTokMailbox);
            pConn->mTokMailbox = NULL;
        }
    }
    else
    {
        K2OSRPC_Debug("***ClientConn mailbox create failed\n");
    }

    if (NULL != pConn->mIpcEnd)
    {
        timeoutLeft = CONNECT_TO_SERVER_TIMEOUT_MS;
        K2OS_System_GetHfTick(&oldTick);
        do {
            ok = K2OS_Thread_WaitOne(&waitResult, pConn->mTokMailbox, timeoutLeft);
            if (!ok)
            {
                if (K2STAT_ERROR_TIMEOUT == K2OS_Thread_GetLastStatus())
                {
                    K2OSRPC_Debug("***RPC client mailbox wait failed with timeout\n");
                }
                else
                {
                    K2OSRPC_Debug("***RPC client mailbox wait failed with lasterror %08X\n", K2OS_Thread_GetLastStatus());
                }
                break;
            }

            K2_ASSERT(waitResult == K2OS_Wait_Signalled_0);

            K2OS_System_GetHfTick(&newTick);
            oldTick = newTick - oldTick;

            elapsedMs = K2OS_System_MsTick32FromHfTick(&oldTick);

            if (elapsedMs > timeoutLeft)
                timeoutLeft = 0;
            else
                timeoutLeft -= elapsedMs;
            oldTick = newTick;

            if (K2OS_Mailbox_Recv(pConn->mTokMailbox, &msg))
            {
                if (!K2OS_IpcEnd_ProcessMsg(&msg))
                {
                    if ((msg.mMsgType == K2OS_SYSTEM_MSGTYPE_IPC) &&
                        (msg.mShort == K2OS_SYSTEM_MSG_IPC_SHORT_REQUEST))
                    {
                        //
                        // attempt to connect to this endpoint. reject it
                        // 
                        // msg.mPayload[0] is interface
                        // msg.mPayload[1] is requestor process id
                        // msg.mPayload[2] is global request id
                        K2OS_Ipc_RejectRequest(msg.mPayload[2], K2STAT_ERROR_NOT_ALLOWED);
                    }
                    // else ignore the message
                }

                if (pConn->mIsConnected)
                {
                    break;
                }

                if (timeoutLeft == 0)
                {
                    K2OSRPC_Debug("***RPC Client timed out trying to connect to server.\n");
                    break;
                }

                if (pConn->mIsRejected)
                {
                    K2OSRPC_Debug("***RPC Client was rejected by server.\n");
                    break;
                }
            }
            else
            {
                if (K2STAT_ERROR_EMPTY != K2OS_Thread_GetLastStatus())
                {
                    K2OSRPC_Debug("***RPC client mailbox recv1 failed with error %08X\n", K2OS_Thread_GetLastStatus());
                }
            }

        } while (1);

        if (!pConn->mIsConnected)
        {
            K2OS_IpcEnd_Delete(pConn->mIpcEnd);
            pConn->mIpcEnd = NULL;
        }
    }

    if (!pConn->mIsConnected)
    {
        //
        // failed startup
        //
        K2OS_CritSec_Enter(&sgConnSec);
        K2TREE_Remove(&sgConnTree, &pConn->ConnTreeNode);
        K2OS_CritSec_Leave(&sgConnSec);

        if (NULL != pConn->mTokMailbox)
        {
            K2OS_Token_Destroy(pConn->mTokMailbox);
            pConn->mTokMailbox = NULL;
        }

        K2OS_Gate_Open(pConn->mTokConnectStatusGate);

        K2OS_Thread_WaitOne(&waitResult, pConn->mTokStopNotify, K2OS_TIMEOUT_INFINITE);
    }
    else
    {
        //
        // connected - run the connection
        //
        K2OS_Gate_Open(pConn->mTokConnectStatusGate);
        
        tokWait[0] = pConn->mTokMailbox;
        tokWait[1] = pConn->mTokStopNotify;

        do {
            ok = K2OS_Thread_WaitMany(&waitResult, 2, tokWait, FALSE, K2OS_TIMEOUT_INFINITE);
            K2_ASSERT(ok);

            if (waitResult != K2OS_Wait_Signalled_0)
            {
                break;
            }

            if (K2OS_Mailbox_Recv(pConn->mTokMailbox, &msg))
            {
                if (!K2OS_IpcEnd_ProcessMsg(&msg))
                {
                    if ((msg.mMsgType == K2OS_SYSTEM_MSGTYPE_IPC) &&
                        (msg.mShort == K2OS_SYSTEM_MSG_IPC_SHORT_REQUEST))
                    {
                        //
                        // attempt to connect to this endpoint. reject it
                        // 
                        // msg.mPayload[0] is interface
                        // msg.mPayload[1] is requestor process id
                        // msg.mPayload[2] is global request id
                        K2OS_Ipc_RejectRequest(msg.mPayload[2], K2STAT_ERROR_NOT_ALLOWED);
                    }
                    // else ignore the message
                }
            }
            else
            {
                if (K2STAT_ERROR_EMPTY != K2OS_Thread_GetLastStatus())
                {
                    K2OSRPC_Debug("***RPC client mailbox recv2 failed with error %08X\n", K2OS_Thread_GetLastStatus());
                }
            }

        } while (1);

        //
        // if still connected then we stopped the loop due to the stop signal
        // otherwise we stopped the loop because we disconnected
        //
        wasStopSignal = pConn->mIsConnected;
        if (wasStopSignal)
        {
            //
            // told to stop - must disconnect first
            //
            K2OS_IpcEnd_Disconnect(pConn->mIpcEnd);

            do {
                ok = K2OS_Thread_WaitOne(&waitResult, pConn->mTokMailbox, K2OS_TIMEOUT_INFINITE);
                K2_ASSERT(ok);
                K2_ASSERT(waitResult == K2OS_Wait_Signalled_0);

                if (K2OS_Mailbox_Recv(pConn->mTokMailbox, &msg))
                {
                    if (!K2OS_IpcEnd_ProcessMsg(&msg))
                    {
                        if ((msg.mMsgType == K2OS_SYSTEM_MSGTYPE_IPC) &&
                            (msg.mShort == K2OS_SYSTEM_MSG_IPC_SHORT_REQUEST))
                        {
                            //
                            // attempt to connect to this endpoint. reject it
                            // 
                            // msg.mPayload[0] is interface
                            // msg.mPayload[1] is requestor process id
                            // msg.mPayload[2] is global request id
                            K2OS_Ipc_RejectRequest(msg.mPayload[2], K2STAT_ERROR_NOT_ALLOWED);
                        }
                        // else ignore the message
                    }
                }
                else
                {
                    if (K2STAT_ERROR_EMPTY != K2OS_Thread_GetLastStatus())
                    {
                        K2OSRPC_Debug("***RPC client mailbox recv3 failed with error %08X\n", K2OS_Thread_GetLastStatus());
                    }
                }
            } while (pConn->mIsConnected);
        }

        K2OS_IpcEnd_Delete(pConn->mIpcEnd);
        pConn->mIpcEnd = NULL;

        K2OS_Token_Destroy(pConn->mTokMailbox);
        pConn->mTokMailbox = NULL;

        if (!wasStopSignal)
        {
            K2OS_Thread_WaitOne(&waitResult, pConn->mTokStopNotify, K2OS_TIMEOUT_INFINITE);
        }
    }

    K2OSRPC_ClientConn_Purge(pConn);

    FUNC_EXIT;
    return 0;
}

K2OSRPC_CLIENT_CONN *
K2OSRPC_ClientConn_FindOrCreate(
    K2OS_IFINST_ID  aRpcIfInstId
)
{
    K2TREE_NODE *           pTreeNode;
    K2OSRPC_CLIENT_CONN *   pConn;
    K2OSRPC_CLIENT_CONN *   pNewConn;
    K2OS_WaitResult         waitResult;

    FUNC_ENTER;

    K2OS_CritSec_Enter(&sgConnSec);

    pTreeNode = K2TREE_Find(&sgConnTree, (UINT32)aRpcIfInstId);
    if (NULL != pTreeNode)
    {
        pConn = K2_GET_CONTAINER(K2OSRPC_CLIENT_CONN, pTreeNode, ConnTreeNode);
        K2ATOMIC_Inc(&pConn->mRefCount);
    }
    else
    {
        pConn = NULL;
    }

    K2OS_CritSec_Leave(&sgConnSec);

    if (NULL == pConn)
    {
        pNewConn = (K2OSRPC_CLIENT_CONN *)K2OS_Heap_Alloc(sizeof(K2OSRPC_CLIENT_CONN));
        if (NULL == pNewConn)
        {
            FUNC_EXIT;
            return NULL;
        }

        K2MEM_Zero(pNewConn, sizeof(K2OSRPC_CLIENT_CONN));

        if (!K2OS_CritSec_Init(&pNewConn->IoListSec))
        {
            K2OS_Heap_Free(pNewConn);
            FUNC_EXIT;
            return NULL;
        }

        pNewConn->mRefCount = 1;

        pNewConn->mTokConnectStatusGate = K2OS_Gate_Create(FALSE);
        if (NULL == pNewConn->mTokConnectStatusGate)
        {
            K2OS_CritSec_Done(&pNewConn->IoListSec);
            K2OS_Heap_Free(pNewConn);
            FUNC_EXIT;
            return NULL;
        }

        pNewConn->mTokStopNotify = K2OS_Notify_Create(FALSE);
        if (NULL == pNewConn->mTokStopNotify)
        {
            K2OS_Token_Destroy(pNewConn->mTokConnectStatusGate);
            pNewConn->mTokConnectStatusGate = NULL;
            K2OS_CritSec_Done(&pNewConn->IoListSec);
            K2OS_Heap_Free(pNewConn);
            FUNC_EXIT;
            return NULL;
        }

        K2OS_CritSec_Enter(&sgConnSec);

        pTreeNode = K2TREE_Find(&sgConnTree, (UINT32)aRpcIfInstId);
        if (NULL != pTreeNode)
        {
            pConn = K2_GET_CONTAINER(K2OSRPC_CLIENT_CONN, pTreeNode, ConnTreeNode);
            K2ATOMIC_Inc(&pConn->mRefCount);
        }
        else
        {
            pNewConn->ConnTreeNode.mUserVal = (UINT32)aRpcIfInstId;
            K2TREE_Insert(&sgConnTree, pNewConn->ConnTreeNode.mUserVal, &pNewConn->ConnTreeNode);
        }

        K2OS_CritSec_Leave(&sgConnSec);

        if (NULL == pConn)
        {
            pNewConn->mTokThread = K2OS_Thread_Create("RpcClientConn", K2OSRPC_ClientConn_Thread, pNewConn, NULL, &pNewConn->mThreadId);
            if (NULL == pNewConn->mTokThread)
            {
                K2OS_CritSec_Enter(&sgConnSec);
                K2TREE_Remove(&sgConnTree, &pNewConn->ConnTreeNode);
                K2OS_CritSec_Leave(&sgConnSec);

                K2OS_Gate_Open(pNewConn->mTokConnectStatusGate);

                K2OSRPC_ClientConn_Release(pNewConn);

                FUNC_EXIT;
                return NULL;
            }
            pConn = pNewConn;
        }
        else
        {
            K2OS_Token_Destroy(pNewConn->mTokStopNotify);
            K2OS_Token_Destroy(pNewConn->mTokConnectStatusGate);
            K2OS_CritSec_Done(&pNewConn->IoListSec);
            K2OS_Heap_Free(pNewConn);
        }
    }

    K2OS_Thread_WaitOne(&waitResult, pConn->mTokConnectStatusGate, K2OS_TIMEOUT_INFINITE);

    if (!pConn->mIsConnected)
    {
        K2OSRPC_ClientConn_Release(pConn);
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_CONNECTED);
        FUNC_EXIT;
        return NULL;
    }

    FUNC_EXIT;
    return pConn;
}

K2OS_RPC_OBJ_HANDLE 
K2OSRPC_Client_CreateObj(
    K2OS_IFINST_ID      aRemoteRpcInstId, 
    K2_GUID128 const *  apClassId, 
    UINT32              aCreatorContext
)
{
    K2OSRPC_CLIENT_CONN *                       pConn;
    K2OSRPC_IOMSG                               ioMsg;
    K2OSRPC_MSG_CREATE_REQUEST_DATA             request;
    K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA    response;
    K2OSRPC_CLIENT_OBJ_HANDLE *                 pHandle;

    FUNC_ENTER;

    pHandle = K2OS_Heap_Alloc(sizeof(K2OSRPC_CLIENT_OBJ_HANDLE));
    if (NULL == pHandle)
    {
        FUNC_EXIT;
        return NULL;
    }

    pConn = K2OSRPC_ClientConn_FindOrCreate(aRemoteRpcInstId);
    if (NULL == pConn)
    {
        K2OS_Heap_Free(pHandle);
        FUNC_EXIT;
        return NULL;
    }

    K2MEM_Copy(&request.mClassId, apClassId, sizeof(K2_GUID128));
    request.mCreatorContext = aCreatorContext;

    K2MEM_Zero(&ioMsg, sizeof(ioMsg));

    ioMsg.RequestHdr.mRequestType = K2OSRPC_ServerClientRequest_Create;

    ioMsg.RequestHdr.mOutBufSizeProvided = sizeof(response);
    ioMsg.mpOutBuffer = (UINT8 *)&response;

    K2OSRPC_ClientConn_SendRequest(pConn, &ioMsg, (UINT8 const *)&request, sizeof(request));

    if (!K2STAT_IS_ERROR(ioMsg.mResultStatus))
    {
        K2_ASSERT(0 != response.mServerObjId);
        K2_ASSERT(0 != response.mServerHandle);

        K2MEM_Zero(pHandle, sizeof(K2OSRPC_CLIENT_OBJ_HANDLE));

        pHandle->Hdr.mRefs = 1;
        pHandle->Hdr.mIsServer = FALSE;
        pHandle->Hdr.GlobalHandleTreeNode.mUserVal = (UINT32)pHandle;
        pHandle->mpConnToServer = pConn;
        // addref to conn not done because we would release the one we got from FindOrCreate() above
        pHandle->ServerHandleTreeNode.mUserVal = response.mServerHandle;
        pHandle->mServerObjId = response.mServerObjId;
        K2MEM_Copy(&pHandle->ClassId, apClassId, sizeof(K2_GUID128));

        K2OS_CritSec_Enter(&gRpcGraphSec);

        K2TREE_Insert(&gRpcHandleTree, pHandle->Hdr.GlobalHandleTreeNode.mUserVal, &pHandle->Hdr.GlobalHandleTreeNode);
        K2TREE_Insert(&sgServerHandleTree, pHandle->ServerHandleTreeNode.mUserVal, &pHandle->ServerHandleTreeNode);

        K2OS_CritSec_Leave(&gRpcGraphSec);

        FUNC_EXIT;
        return (K2OS_RPC_OBJ_HANDLE)pHandle;
    }

    K2OS_Thread_SetLastStatus(ioMsg.mResultStatus);

    K2OSRPC_ClientConn_Release(pConn);

    K2OS_Heap_Free(pHandle);

    FUNC_EXIT;
    return NULL;
}

K2OS_RPC_OBJ_HANDLE
K2OSRPC_Client_Attach(
    K2OS_IFINST_ID  aRemoteRpcInstId,
    UINT32          aRemoteId,
    UINT32          aMethod,
    UINT32 *        apRetObjId
)
{
    K2OSRPC_CLIENT_CONN *                       pConn;
    K2OSRPC_IOMSG                               ioMsg;
    K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA    response;
    K2OSRPC_CLIENT_OBJ_HANDLE *                 pHandle;

    FUNC_ENTER;

    pHandle = K2OS_Heap_Alloc(sizeof(K2OSRPC_CLIENT_OBJ_HANDLE));
    if (NULL == pHandle)
    {
        FUNC_EXIT;
        return NULL;
    }

    pConn = K2OSRPC_ClientConn_FindOrCreate(aRemoteRpcInstId);
    if (NULL == pConn)
    {
        K2OS_Heap_Free(pHandle);
        FUNC_EXIT;
        return NULL;
    }

    K2MEM_Zero(&ioMsg, sizeof(ioMsg));

    ioMsg.RequestHdr.mRequestType = aMethod;
    ioMsg.RequestHdr.mTargetId = aRemoteId;

    ioMsg.RequestHdr.mOutBufSizeProvided = sizeof(response);
    ioMsg.mpOutBuffer = (UINT8 *)&response;

    K2OSRPC_ClientConn_SendRequest(pConn, &ioMsg, NULL, 0);

    if (!K2STAT_IS_ERROR(ioMsg.mResultStatus))
    {
        K2_ASSERT(0 != response.mServerObjId);
        K2_ASSERT(0 != response.mServerHandle);

        K2_ASSERT((aMethod != K2OSRPC_ServerClientRequest_AcquireByObjId) || (aRemoteId == response.mServerObjId));

        K2MEM_Zero(pHandle, sizeof(K2OSRPC_CLIENT_OBJ_HANDLE));

        pHandle->Hdr.mRefs = 1;
        pHandle->Hdr.mIsServer = FALSE;
        pHandle->Hdr.GlobalHandleTreeNode.mUserVal = (UINT32)pHandle;
        pHandle->mpConnToServer = pConn;
        // addref to conn not done because we would release the one we got from FindOrCreate() above
        pHandle->mServerObjId = response.mServerObjId;
        pHandle->ServerHandleTreeNode.mUserVal = response.mServerHandle;

        if (NULL != apRetObjId)
        {
            *apRetObjId = response.mServerObjId;
        }

        K2OS_CritSec_Enter(&gRpcGraphSec);

        K2TREE_Insert(&gRpcHandleTree, pHandle->Hdr.GlobalHandleTreeNode.mUserVal, &pHandle->Hdr.GlobalHandleTreeNode);
        K2TREE_Insert(&sgServerHandleTree, pHandle->ServerHandleTreeNode.mUserVal, &pHandle->ServerHandleTreeNode);

        K2OS_CritSec_Leave(&gRpcGraphSec);

        FUNC_EXIT;
        return (K2OS_RPC_OBJ_HANDLE)pHandle;
    }

    K2OSRPC_ClientConn_Release(pConn);

    K2OS_Thread_SetLastStatus(ioMsg.mResultStatus);

    K2OS_Heap_Free(pHandle);

    FUNC_EXIT;
    return NULL;
}

K2OS_RPC_OBJ_HANDLE
K2OSRPC_Client_AttachByObjId(
    K2OS_IFINST_ID  aRemoteRpcInstId,
    UINT32          aRemoteObjectId
)
{
    K2OS_RPC_OBJ_HANDLE result;

    FUNC_ENTER;

    result = K2OSRPC_Client_Attach(aRemoteRpcInstId, aRemoteObjectId, K2OSRPC_ServerClientRequest_AcquireByObjId, NULL);

    FUNC_EXIT;

    return result;
}

K2OS_RPC_OBJ_HANDLE
K2OSRPC_Client_AttachByIfInstId(
    K2OS_IFINST_ID  aIfInstId,
    UINT32 *        apRetObjId
)
{
    K2OS_IFINST_DETAIL  detail;
    K2OS_IFENUM_TOKEN   tokEnum;
    UINT32              bufferCount;
    BOOL                ok;
    K2OS_RPC_OBJ_HANDLE hResult;

    FUNC_ENTER;

    if (!K2OS_IfInstId_GetDetail(aIfInstId, &detail))
    {
        FUNC_EXIT;
        return NULL;
    }

    tokEnum = K2OS_IfEnum_Create(TRUE, detail.mOwningProcessId, K2OS_IFACE_CLASSCODE_RPC, &gRpcServerClassId);
    if (NULL == tokEnum)
    {
        FUNC_EXIT;
        return NULL;
    }

    bufferCount = 1;
    ok = K2OS_IfEnum_Next(tokEnum, &detail, &bufferCount);

    K2OS_Token_Destroy(tokEnum);

    if (!ok)
    {
        FUNC_EXIT;
        return NULL;
    }

    if (0 == bufferCount)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return NULL;
    }

    if (NULL != apRetObjId)
    {
        *apRetObjId = 0;
    }

    hResult = K2OSRPC_Client_Attach(detail.mInstId, (UINT32)aIfInstId, K2OSRPC_ServerClientRequest_AcquireByIfInstId, apRetObjId);

    FUNC_EXIT;

    return hResult;
}

K2STAT 
K2OSRPC_Client_Call(
    K2OSRPC_CLIENT_OBJ_HANDLE * apObjHandle, 
    K2OS_RPC_CALLARGS const *   apCallArgs, 
    UINT32 *                    apRetActualOut
)
{
    K2OSRPC_IOMSG   ioMsg;

    FUNC_ENTER;

    K2_ASSERT(NULL != apObjHandle->mpConnToServer);

    if (!apObjHandle->mpConnToServer->mIsConnected)
    {
        FUNC_EXIT;
        return K2STAT_ERROR_DISCONNECTED;
    }

    K2MEM_Zero(&ioMsg, sizeof(K2OSRPC_IOMSG));

    ioMsg.RequestHdr.mRequestType = K2OSRPC_ServerClientRequest_Call;
    ioMsg.RequestHdr.mTargetId = (UINT32)apObjHandle->ServerHandleTreeNode.mUserVal;
    ioMsg.RequestHdr.mTargetMethodId = apCallArgs->mMethodId;

    ioMsg.RequestHdr.mOutBufSizeProvided = apCallArgs->mOutBufByteCount;
    ioMsg.mpOutBuffer = apCallArgs->mpOutBuf;

    K2OSRPC_ClientConn_SendRequest(
        apObjHandle->mpConnToServer,
        &ioMsg,
        apCallArgs->mpInBuf,
        apCallArgs->mInBufByteCount
    );

    if (!K2STAT_IS_ERROR(ioMsg.mResultStatus))
    {
        *apRetActualOut = ioMsg.mActualOutBytes;
    }

    FUNC_EXIT;

    return ioMsg.mResultStatus;
}

void 
K2OSRPC_Client_PurgeHandle(
    K2OSRPC_CLIENT_OBJ_HANDLE * apObjHandle
)
{
    K2OSRPC_IOMSG   ioMsg;

    FUNC_ENTER;

    //
    // handle has already been removed from global handle tree
    //
    K2_ASSERT(NULL != apObjHandle->mpConnToServer);

    if (apObjHandle->mpConnToServer->mIsConnected)
    {
        K2MEM_Zero(&ioMsg, sizeof(ioMsg));

        ioMsg.RequestHdr.mRequestType = K2OSRPC_ServerClientRequest_Release;
        ioMsg.RequestHdr.mTargetId = apObjHandle->ServerHandleTreeNode.mUserVal;

        // response to this is ignored
        K2OSRPC_ClientConn_SendRequest(apObjHandle->mpConnToServer, &ioMsg, NULL, 0);
    }

    K2OSRPC_ClientConn_Release(apObjHandle->mpConnToServer);
    apObjHandle->mpConnToServer = NULL;

    apObjHandle->ServerHandleTreeNode.mUserVal = 0;
    apObjHandle->mServerObjId = 0;
    K2MEM_Zero(&apObjHandle->ClassId, sizeof(K2_GUID128));

    if (NULL != apObjHandle->mTokNotifyMailbox)
    {
        K2OS_Token_Destroy(apObjHandle->mTokNotifyMailbox);
    }
       
    K2OS_Heap_Free(apObjHandle);

    FUNC_EXIT;
}

K2OS_MAILBOX_TOKEN 
K2OSRPC_Client_SwapNotifyTarget(
    K2OSRPC_CLIENT_OBJ_HANDLE * apClientHandle,
    K2OS_MAILBOX_TOKEN          aTokMailslot
)
{
    K2OS_MAILBOX_TOKEN swap;

    FUNC_ENTER;

    swap = apClientHandle->mTokNotifyMailbox;
    apClientHandle->mTokNotifyMailbox = aTokMailslot;

    FUNC_EXIT;
    return swap;
}

UINT32 
K2OSRPC_Client_GetObjectId(
    K2OSRPC_CLIENT_OBJ_HANDLE * pClientHandle
)
{
    FUNC_ENTER;
    FUNC_EXIT;
    return pClientHandle->mServerObjId;
}

void
K2OSRPC_Client_Init(
    void
)
{
    BOOL ok;

    FUNC_ENTER;

    ok = K2OS_CritSec_Init(&sgConnSec);
    K2_ASSERT(ok);
    if (!ok)
    {
        K2OSRPC_Debug("***Rpc client critsec init failed (%08X)\n", K2OS_Thread_GetLastStatus());
        return;
    }

    K2TREE_Init(&sgConnTree, NULL);

    K2TREE_Init(&sgServerHandleTree, NULL);

    FUNC_EXIT;
}

void 
K2OSRPC_HandleLocked_AtClientServerHandleDestroy(
    K2OSRPC_CLIENT_OBJ_HANDLE *apHandle
)
{
    K2TREE_Remove(&sgServerHandleTree, &apHandle->ServerHandleTreeNode);
}
