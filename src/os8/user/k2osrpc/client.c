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

#define CONNECT_TO_SERVER_TIMEOUT_MS 5000

typedef struct _K2OSRPC_CLIENT_CONN K2OSRPC_CLIENT_CONN;
struct _K2OSRPC_CLIENT_CONN
{
    INT_PTR             mRefCount;

    INT_PTR             mRunningRef;

    K2OS_MAILBOX        mMailbox;
    K2OS_IPCEND         mIpcEnd;

    K2OS_THREAD_TOKEN   mThreadToken;
    UINT_PTR            mThreadId;
    K2OS_NOTIFY_TOKEN   mStopNotifyToken;
    K2OS_NOTIFY_TOKEN   mConnectStatusNotifyToken;

    BOOL                mIsConnected;
    BOOL                mIsRejected;

    K2OS_CRITSEC        IoListSec;
    K2LIST_ANCHOR       IoList;

    K2TREE_NODE         ConnTreeNode;       // mUserVal is server interface instance id
};

struct _K2OSRPC_CLIENT_OBJECT_USAGE
{
    K2OSRPC_OBJECT_USAGE_HDR    Hdr;

    K2OSRPC_CLIENT_CONN *       mpConn;

    UINT_PTR                    mServerUsageId;
};

typedef struct _K2OSRPC_IOMSG K2OSRPC_IOMSG;
struct _K2OSRPC_IOMSG
{
    K2OS_NOTIFY_TOKEN           mDoneNotifyToken;

    K2LIST_LINK                 ConnListLink;

    K2OSRPC_MSG_REQUEST_HDR     RequestHdr;

    K2STAT                      mResultStatus;

    UINT8 *                     mpOutBuffer;

    UINT32                      mActualOutBytes;
};

static K2OS_CRITSEC     sgConnSec;
static K2TREE_ANCHOR    sgConnTree;

void
K2OSRPC_ClientConn_SendRequest(
    K2OSRPC_CLIENT_CONN *   apConn,
    K2OSRPC_IOMSG *         apIoMsg,
    UINT8 const *           apInBuf,
    UINT32                  aInBufBytes
)
{
    BOOL                wasConn;
    K2MEM_BUFVECTOR     memVec[2];
    UINT_PTR            vecCount;

    apIoMsg->mActualOutBytes = 0;

    if (!apConn->mIsConnected)
    {
        apIoMsg->mResultStatus = K2STAT_ERROR_DISCONNECTED;
        return;
    }

    apIoMsg->mDoneNotifyToken = K2OS_Notify_Create(FALSE);
    if (NULL == apIoMsg->mDoneNotifyToken)
    {
        apIoMsg->mResultStatus = K2OS_Thread_GetLastStatus();
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

        if (K2OS_Ipc_SendVector(apConn->mIpcEnd, vecCount, memVec))
        {
            //            K2OSRPC_DebugPrintf("+ClientWaitForRequest\n");
            K2OS_Wait_One(apIoMsg->mDoneNotifyToken, K2OS_TIMEOUT_INFINITE);
            //            K2OSRPC_DebugPrintf("-ClientWaitForRequest(iomsg.mresultstatus = %08X)\n", apIoMsg->mResultStatus);
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

    K2OS_Token_Destroy(apIoMsg->mDoneNotifyToken);
}

void
K2OSRPC_ClientConn_OnConnect(
    K2OS_IPCEND     aEndpoint,
    void *          apContext,
    UINT_PTR        aRemoteMaxMsgBytes
)
{
    K2OSRPC_CLIENT_CONN *   pConn;
    pConn = (K2OSRPC_CLIENT_CONN *)apContext;
    pConn->mIsConnected = TRUE;
    K2_CpuWriteBarrier();
}

void
K2OSRPC_ClientConn_OnRecv(
    K2OS_IPCEND     aEndpoint,
    void *          apContext,
    UINT8 const *   apData,
    UINT_PTR        aByteCount
)
{
    K2STAT                              stat;
    K2OSRPC_MSG_RESPONSE_HDR const *    pRespHdr;
    K2LIST_LINK *                       pListLink;
    K2OSRPC_IOMSG *                     pIoMsg;
    K2OSRPC_CLIENT_CONN *               pConn;

    pConn = (K2OSRPC_CLIENT_CONN *)apContext;

    //    K2OSRPC_DebugPrintf("ClientConn Recv %d\n", aByteCount);

    if (aByteCount < sizeof(K2OSRPC_MSG_RESPONSE_HDR))
    {
        K2OSRPC_DebugPrintf("RpcServer %d sent malformed response (too small %d)\n", pConn->ConnTreeNode.mUserVal, aByteCount);
        return;
    }

    pRespHdr = (K2OSRPC_MSG_RESPONSE_HDR const *)apData;
    if (0 == pRespHdr->mCallerRef)
    {
        K2OSRPC_DebugPrintf("RpcServer %d sent malformed response (caller ref zero)\n", pConn->ConnTreeNode.mUserVal);
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
        K2OSRPC_DebugPrintf("Response for ref %d from rpcServer %d ignored (caller abandoned)\n", pRespHdr->mCallerRef, pConn->ConnTreeNode.mUserVal);
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
                K2OSRPC_DebugPrintf("Response for ref %d from rpcServer %d was too big for output buffer\n", pRespHdr->mCallerRef, pConn->ConnTreeNode.mUserVal);
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

    K2OS_Notify_Signal(pIoMsg->mDoneNotifyToken);
}

void
K2OSRPC_ClientConn_OnDisconnect(
    K2OS_IPCEND     aEndpoint,
    void *          apContext
)
{
    K2LIST_LINK *           pListLink;
    K2OSRPC_IOMSG *         pIoMsg;
    K2OSRPC_CLIENT_CONN *   pConn;
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
            K2OS_Notify_Signal(pIoMsg->mDoneNotifyToken);
        } while (NULL != pListLink);
    }

    K2OS_CritSec_Leave(&pConn->IoListSec);
}

void
K2OSRPC_ClientConn_OnRejected(
    K2OS_IPCEND     aEndpoint,
    void *          apContext,
    UINT_PTR        aReasonCode
)
{
    K2OSRPC_CLIENT_CONN *   pConn;
    pConn = (K2OSRPC_CLIENT_CONN *)apContext;
    pConn->mIsRejected = TRUE;
    K2_CpuWriteBarrier();
}

void
K2OSRPC_ClientConn_Purge(
    K2OSRPC_CLIENT_CONN *   apConn
)
{
    K2_ASSERT(NULL == apConn->mThreadToken);

    K2_ASSERT(0 == apConn->IoList.mNodeCount);

    K2_ASSERT(NULL == apConn->mIpcEnd);
    K2_ASSERT(NULL == apConn->mMailbox);

    K2OS_Token_Destroy(apConn->mConnectStatusNotifyToken);

    K2OS_Token_Destroy(apConn->mStopNotifyToken);

    K2OS_CritSec_Done(&apConn->IoListSec);

    K2OS_Heap_Free(apConn);
}

void
K2OSRPC_ClientConn_Release(
    K2OSRPC_CLIENT_CONN *   apConn
)
{
    if (0 != K2ATOMIC_Dec(&apConn->mRefCount))
    {
        return;
    }

    if (NULL == apConn->mThreadToken)
    {
        K2OSRPC_ClientConn_Purge(apConn);
    }
    else
    {
        K2OS_Token_Destroy(apConn->mThreadToken);
        apConn->mThreadToken = NULL;
        K2_CpuWriteBarrier();

        K2OS_Notify_Signal(apConn->mStopNotifyToken);
    }
}

UINT_PTR
K2OSRPC_ClientConn_Thread(
    void *apArg
)
{
    static const K2OS_IPCEND_FUNCTAB clientConnIpcFuncTab =
    {
        K2OSRPC_ClientConn_OnConnect,
        K2OSRPC_ClientConn_OnRecv,
        K2OSRPC_ClientConn_OnDisconnect,
        K2OSRPC_ClientConn_OnRejected
    };
    K2OSRPC_CLIENT_CONN *   pConn;
    K2OS_MSG                msg;
    UINT_PTR                timeoutLeft;
    UINT_PTR                waitResult;
    UINT_PTR                elapsedMs;
    UINT64                  oldTick;
    UINT64                  newTick;
    BOOL                    wasStopSignal;

    pConn = (K2OSRPC_CLIENT_CONN *)apArg;

    pConn->mMailbox = K2OS_Mailbox_Create(NULL);

    if (NULL != pConn->mMailbox)
    {
        pConn->mIpcEnd = K2OS_Ipc_CreateEndpoint(pConn->mMailbox, 32, 1024, pConn, &clientConnIpcFuncTab);

        if (NULL != pConn->mIpcEnd)
        {
            if (!K2OS_Ipc_SendRequest(pConn->mIpcEnd, pConn->ConnTreeNode.mUserVal))
            {
                K2OSRPC_DebugPrintf("***ClientConn ipc send request failed\n");
                K2OS_Ipc_DeleteEndpoint(pConn->mIpcEnd);
                pConn->mIpcEnd = NULL;
            }
        }
        else
        {
            K2OSRPC_DebugPrintf("***ClientConn ipcend create failed\n");
        }
        if (NULL == pConn->mIpcEnd)
        {
            K2OS_Mailbox_Delete(pConn->mMailbox);
            pConn->mMailbox = NULL;
        }
    }
    else
    {
        K2OSRPC_DebugPrintf("***ClientConn mailbox create failed\n");
    }

    if (NULL != pConn->mIpcEnd)
    {
        timeoutLeft = CONNECT_TO_SERVER_TIMEOUT_MS;
        K2OS_System_GetHfTick(&oldTick);
        do {
            waitResult = K2OS_Wait_OnMailbox(pConn->mMailbox, &msg, timeoutLeft);
            if (K2STAT_IS_ERROR(waitResult))
            {
                K2OSRPC_DebugPrintf("***RPC client mailbox wait failed with result 0x%08X\n", waitResult);
                break;
            }

            K2_ASSERT(waitResult == K2OS_THREAD_WAIT_MAILBOX_SIGNALLED);

            K2OS_System_GetHfTick(&newTick);
            oldTick = newTick - oldTick;

            elapsedMs = K2OS_System_MsTick32FromHfTick(&oldTick);

            if (elapsedMs > timeoutLeft)
                timeoutLeft = 0;
            else
                timeoutLeft -= elapsedMs;
            oldTick = newTick;

            if (!K2OS_System_ProcessMsg(&msg))
            {
                if (msg.mControl == K2OS_SYSTEM_MSG_IPC_REQUEST)
                {
                    //
                    // attempt to connect to this endpoint. reject it
                    // 
                    // msg.mPayload[0] is interface context
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
                K2OSRPC_DebugPrintf("***RPC Client timed out trying to connect to server.\n");
                break;
            }

            if (pConn->mIsRejected)
            {
                K2OSRPC_DebugPrintf("***RPC Client was rejected by server.\n");
                break;
            }

        } while (1);

        if (!pConn->mIsConnected)
        {
            K2OS_Ipc_DeleteEndpoint(pConn->mIpcEnd);
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

        if (NULL != pConn->mMailbox)
        {
            K2OS_Mailbox_Delete(pConn->mMailbox);
            pConn->mMailbox = NULL;
        }

        K2OS_Notify_Signal(pConn->mConnectStatusNotifyToken);

        K2OS_Wait_One(pConn->mStopNotifyToken, K2OS_TIMEOUT_INFINITE);
    }
    else
    {
        //
        // connected - run the connection
        //
        K2OS_Notify_Signal(pConn->mConnectStatusNotifyToken);

        do {
            waitResult = K2OS_Wait_OnMailboxAndOne(pConn->mMailbox, &msg, pConn->mStopNotifyToken, K2OS_TIMEOUT_INFINITE);
            K2_ASSERT(!K2STAT_IS_ERROR(waitResult));
            if (waitResult != K2OS_THREAD_WAIT_MAILBOX_SIGNALLED)
            {
                break;
            }

            if (!K2OS_System_ProcessMsg(&msg))
            {
                if (msg.mControl == K2OS_SYSTEM_MSG_IPC_REQUEST)
                {
                    //
                    // attempt to connect to this endpoint. reject it
                    // 
                    // msg.mPayload[0] is interface context
                    // msg.mPayload[1] is requestor process id
                    // msg.mPayload[2] is global request id
                    K2OS_Ipc_RejectRequest(msg.mPayload[2], K2STAT_ERROR_NOT_ALLOWED);
                }
                // else ignore the message
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
            K2OS_Ipc_DisconnectEndpoint(pConn->mIpcEnd);

            do {
                waitResult = K2OS_Wait_OnMailbox(pConn->mMailbox, &msg, K2OS_TIMEOUT_INFINITE);
                K2_ASSERT(waitResult == K2OS_THREAD_WAIT_MAILBOX_SIGNALLED);
                if (!K2OS_System_ProcessMsg(&msg))
                {
                    if (msg.mControl == K2OS_SYSTEM_MSG_IPC_REQUEST)
                    {
                        //
                        // attempt to connect to this endpoint. reject it
                        // 
                        // msg.mPayload[0] is interface context
                        // msg.mPayload[1] is requestor process id
                        // msg.mPayload[2] is global request id
                        K2OS_Ipc_RejectRequest(msg.mPayload[2], K2STAT_ERROR_NOT_ALLOWED);
                    }
                    // else ignore the message
                }
            } while (pConn->mIsConnected);
        }

        K2OS_Ipc_DeleteEndpoint(pConn->mIpcEnd);
        pConn->mIpcEnd = NULL;

        K2OS_Mailbox_Delete(pConn->mMailbox);
        pConn->mMailbox = NULL;

        if (!wasStopSignal)
        {
            K2OS_Wait_One(&pConn->mStopNotifyToken, K2OS_TIMEOUT_INFINITE);
        }
    }

    K2OSRPC_ClientConn_Purge(pConn);

    return 0;
}

K2OSRPC_CLIENT_CONN *
K2OSRPC_ClientConn_FindOrCreate(
    UINT_PTR    aRpcServerInterfaceInstanceId
)
{
    K2TREE_NODE *           pTreeNode;
    K2OSRPC_CLIENT_CONN *   pConn;
    K2OSRPC_CLIENT_CONN *   pNewConn;

    K2OS_CritSec_Enter(&sgConnSec);

    pTreeNode = K2TREE_Find(&sgConnTree, aRpcServerInterfaceInstanceId);
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
            return NULL;
        }

        K2MEM_Zero(pNewConn, sizeof(K2OSRPC_CLIENT_CONN));

        if (!K2OS_CritSec_Init(&pNewConn->IoListSec))
        {
            K2OS_Heap_Free(pNewConn);
            return NULL;
        }

        pNewConn->mRefCount = 1;

        pNewConn->mConnectStatusNotifyToken = K2OS_Notify_Create(FALSE);
        if (NULL == pNewConn->mConnectStatusNotifyToken)
        {
            K2OS_CritSec_Done(&pNewConn->IoListSec);
            K2OS_Heap_Free(pNewConn);
            return NULL;
        }

        pNewConn->mStopNotifyToken = K2OS_Notify_Create(FALSE);
        if (NULL == pNewConn->mStopNotifyToken)
        {
            K2OS_Token_Destroy(pNewConn->mConnectStatusNotifyToken);
            pNewConn->mConnectStatusNotifyToken = NULL;
            K2OS_CritSec_Done(&pNewConn->IoListSec);
            K2OS_Heap_Free(pNewConn);
            return NULL;
        }

        K2OS_CritSec_Enter(&sgConnSec);

        pTreeNode = K2TREE_Find(&sgConnTree, aRpcServerInterfaceInstanceId);
        if (NULL != pTreeNode)
        {
            pConn = K2_GET_CONTAINER(K2OSRPC_CLIENT_CONN, pTreeNode, ConnTreeNode);
            K2ATOMIC_Inc(&pConn->mRefCount);
        }
        else
        {
            pNewConn->ConnTreeNode.mUserVal = aRpcServerInterfaceInstanceId;
            K2TREE_Insert(&sgConnTree, pNewConn->ConnTreeNode.mUserVal, &pNewConn->ConnTreeNode);
        }

        K2OS_CritSec_Leave(&sgConnSec);

        if (NULL == pConn)
        {
            pNewConn->mThreadToken = K2OS_Thread_Create(K2OSRPC_ClientConn_Thread, pNewConn, NULL, &pNewConn->mThreadId);
            if (NULL == pNewConn->mThreadToken)
            {
                K2OS_CritSec_Enter(&sgConnSec);
                K2TREE_Remove(&sgConnTree, &pNewConn->ConnTreeNode);
                K2OS_CritSec_Leave(&sgConnSec);

                K2OS_Notify_Signal(pNewConn->mConnectStatusNotifyToken);

                K2OSRPC_ClientConn_Release(pNewConn);

                return NULL;
            }
            pConn = pNewConn;
        }
        else
        {
            K2OS_Token_Destroy(pNewConn->mStopNotifyToken);
            K2OS_Token_Destroy(pNewConn->mConnectStatusNotifyToken);
            K2OS_CritSec_Done(&pNewConn->IoListSec);
            K2OS_Heap_Free(pNewConn);
        }
    }

    K2OS_Wait_One(pConn->mConnectStatusNotifyToken, K2OS_TIMEOUT_INFINITE);

    if (!pConn->mIsConnected)
    {
        K2OSRPC_ClientConn_Release(pConn);
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_CONNECTED);
        return NULL;
    }

    return pConn;
}

BOOL
K2OSRPC_ClientUsage_Create(
    UINT_PTR            aRpcServerInterfaceInstanceId,
    K2_GUID128 const *  apClassId,
    UINT_PTR            aCreatorContext,
    UINT_PTR *          apRetServerObjectId,
    UINT_PTR *          apRetUsageId
)
{
    K2OSRPC_CLIENT_CONN *                       pConn;
    K2OSRPC_IOMSG                               ioMsg;
    K2OSRPC_MSG_CREATE_REQUEST_DATA             request;
    K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA    response;
    K2OSRPC_CLIENT_OBJECT_USAGE *               pUsage;

    if (NULL != apRetServerObjectId)
    {
        *apRetServerObjectId = 0;
    }

    if (NULL != apRetUsageId)
    {
        *apRetUsageId = 0;
    }

    pUsage = K2OS_Heap_Alloc(sizeof(K2OSRPC_CLIENT_OBJECT_USAGE));
    if (NULL == pUsage)
    {
        return FALSE;
    }

    pConn = K2OSRPC_ClientConn_FindOrCreate(aRpcServerInterfaceInstanceId);
    if (NULL == pConn)
    {
        K2OS_Heap_Free(pUsage);
        return FALSE;
    }

    K2MEM_Copy(&request.mClassId, apClassId, sizeof(K2_GUID128));
    request.mCreatorContext = aCreatorContext;

    K2MEM_Zero(&ioMsg, sizeof(ioMsg));
    
    ioMsg.RequestHdr.mRequestType = K2OSRpc_ServerClientRequest_Create;

    ioMsg.RequestHdr.mOutBufSizeProvided = sizeof(response);
    ioMsg.mpOutBuffer = (UINT8 *)&response;

    K2OSRPC_ClientConn_SendRequest(pConn, &ioMsg, (UINT8 const *)&request, sizeof(request));

    if (!K2STAT_IS_ERROR(ioMsg.mResultStatus))
    {
        K2_ASSERT(0 != response.mServerObjectId);
        K2_ASSERT(0 != response.mServerUsageId);

        if (NULL != apRetServerObjectId)
        {
            *apRetServerObjectId = response.mServerObjectId;
        }

        K2MEM_Zero(pUsage, sizeof(K2OSRPC_CLIENT_OBJECT_USAGE));

        pUsage->Hdr.mRefCount = 1;
        pUsage->Hdr.mIsServer = FALSE;
        pUsage->Hdr.GlobalUsageTreeNode.mUserVal = K2ATOMIC_AddExchange(&gK2OSRPC_NextGlobalUsageId, 1);
        pUsage->mpConn = pConn;
        // addref not done because we would release the one we got from FindOrCreate() above
        pUsage->mServerUsageId = response.mServerUsageId;

        if (NULL != apRetUsageId)
        {
            *apRetUsageId = pUsage->Hdr.GlobalUsageTreeNode.mUserVal;
        }

        K2OS_CritSec_Enter(&gK2OSRPC_GlobalUsageTreeSec);

        K2TREE_Insert(&gK2OSRPC_GlobalUsageTree, pUsage->Hdr.GlobalUsageTreeNode.mUserVal, &pUsage->Hdr.GlobalUsageTreeNode);

        K2OS_CritSec_Leave(&gK2OSRPC_GlobalUsageTreeSec);

        return TRUE;
    }

    K2OS_Thread_SetLastStatus(ioMsg.mResultStatus);

    K2OSRPC_ClientConn_Release(pConn);

    K2OS_Heap_Free(pUsage);

    return FALSE;
}

BOOL
K2OSRPC_ClientUsage_Acquire(
    UINT_PTR    aRpcServerInterfaceInstanceId,
    UINT_PTR    aServerObjectId,
    UINT_PTR *  apRetUsageId
)
{
    K2OSRPC_CLIENT_CONN *                       pConn;
    K2OSRPC_IOMSG                               ioMsg;
    K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA    response;
    K2OSRPC_CLIENT_OBJECT_USAGE *               pUsage;

    if (NULL != apRetUsageId)
    {
        *apRetUsageId = 0;
    }

    pUsage = K2OS_Heap_Alloc(sizeof(K2OSRPC_CLIENT_OBJECT_USAGE));
    if (NULL == pUsage)
    {
        return FALSE;
    }

    pConn = K2OSRPC_ClientConn_FindOrCreate(aRpcServerInterfaceInstanceId);
    if (NULL == pConn)
    {
        K2OS_Heap_Free(pUsage);
        return FALSE;
    }

    K2MEM_Zero(&ioMsg, sizeof(ioMsg));

    ioMsg.RequestHdr.mRequestType = K2OSRpc_ServerClientRequest_Acquire;
    ioMsg.RequestHdr.mTargetId = aServerObjectId;

    ioMsg.RequestHdr.mOutBufSizeProvided = sizeof(response);
    ioMsg.mpOutBuffer = (UINT8 *)&response;

    K2OSRPC_ClientConn_SendRequest(pConn, &ioMsg, NULL, 0);

    K2OSRPC_ClientConn_Release(pConn);

    if (!K2STAT_IS_ERROR(ioMsg.mResultStatus))
    {
        K2_ASSERT(0 != response.mServerObjectId);
        K2_ASSERT(0 != response.mServerUsageId);

        K2MEM_Zero(pUsage, sizeof(K2OSRPC_CLIENT_OBJECT_USAGE));

        pUsage->Hdr.mRefCount = 1;
        pUsage->Hdr.mIsServer = FALSE;
        pUsage->Hdr.GlobalUsageTreeNode.mUserVal = K2ATOMIC_AddExchange(&gK2OSRPC_NextGlobalUsageId, 1);
        pUsage->mpConn = pConn;
        K2ATOMIC_Inc(&pConn->mRefCount);
        pUsage->mServerUsageId = response.mServerUsageId;

        if (NULL != apRetUsageId)
        {
            *apRetUsageId = pUsage->Hdr.GlobalUsageTreeNode.mUserVal;
        }

        K2OS_CritSec_Enter(&gK2OSRPC_GlobalUsageTreeSec);

        K2TREE_Insert(&gK2OSRPC_GlobalUsageTree, pUsage->Hdr.GlobalUsageTreeNode.mUserVal, &pUsage->Hdr.GlobalUsageTreeNode);

        K2OS_CritSec_Leave(&gK2OSRPC_GlobalUsageTreeSec);

        return TRUE;
    }

    K2OS_Thread_SetLastStatus(ioMsg.mResultStatus);

    K2OS_Heap_Free(pUsage);

    return FALSE;
}

K2STAT
K2OSRPC_ClientUsage_Call(
    K2OSRPC_CLIENT_OBJECT_USAGE *   apClientUsage,
    K2OSRPC_CALLARGS const *        apArgs,
    UINT_PTR *                      apRetActualOut
)
{
    K2OSRPC_IOMSG   ioMsg;

    if (!apClientUsage->mpConn->mIsConnected)
    {
        return K2STAT_ERROR_DISCONNECTED;
    }

    K2MEM_Zero(&ioMsg, sizeof(K2OSRPC_IOMSG));

    ioMsg.RequestHdr.mRequestType = K2OSRpc_ServerClientRequest_Call;
    ioMsg.RequestHdr.mTargetId = apClientUsage->mServerUsageId;
    ioMsg.RequestHdr.mTargetMethodId = apArgs->mMethodId;

    ioMsg.RequestHdr.mOutBufSizeProvided = apArgs->mOutBufByteCount;
    ioMsg.mpOutBuffer = apArgs->mpOutBuf;

    K2OSRPC_ClientConn_SendRequest(
        apClientUsage->mpConn, 
        &ioMsg, 
        apArgs->mpInBuf,
        apArgs->mInBufByteCount
    );

    if (!K2STAT_IS_ERROR(ioMsg.mResultStatus))
    {
        *apRetActualOut = ioMsg.mActualOutBytes;
    }

    return ioMsg.mResultStatus;
}

K2STAT
K2OSRPC_ClientUsage_Release(
    K2OSRPC_CLIENT_OBJECT_USAGE * apClientUsage
)
{
    K2OSRPC_IOMSG   ioMsg;

    if (0 != K2ATOMIC_Dec(&apClientUsage->Hdr.mRefCount))
    {
        return K2STAT_NO_ERROR;
    }

    //
    // delete this usage
    //

    K2OS_CritSec_Enter(&gK2OSRPC_GlobalUsageTreeSec);

    K2TREE_Remove(&gK2OSRPC_GlobalUsageTree, &apClientUsage->Hdr.GlobalUsageTreeNode);

    K2OS_CritSec_Leave(&gK2OSRPC_GlobalUsageTreeSec);

    //
    // object removed from tree and no longer reference-able
    // but it is still on the server's object list if the server is still connected
    //

    if (apClientUsage->mpConn->mIsConnected)
    {
        K2MEM_Zero(&ioMsg, sizeof(ioMsg));

        ioMsg.RequestHdr.mRequestType = K2OSRpc_ServerClientRequest_Release;
        ioMsg.RequestHdr.mTargetId = apClientUsage->mServerUsageId;

        K2OSRPC_ClientConn_SendRequest(apClientUsage->mpConn, &ioMsg, NULL, 0);
    }

    K2OSRPC_ClientConn_Release(apClientUsage->mpConn);

    K2OS_Heap_Free(apClientUsage);

    return K2STAT_NO_ERROR;
}

K2STAT
K2OSRPC_Client_AtXdlEntry(
    UINT32 aReason
)
{
    if (XDL_ENTRY_REASON_LOAD == aReason)
    {
        K2OS_CritSec_Init(&sgConnSec);
        K2TREE_Init(&sgConnTree, NULL);
    }
    else if (XDL_ENTRY_REASON_UNLOAD == aReason)
    {
        K2_ASSERT(0 == sgConnTree.mNodeCount);
        K2OS_CritSec_Done(&sgConnSec);
    }

    return K2STAT_NO_ERROR;
}
