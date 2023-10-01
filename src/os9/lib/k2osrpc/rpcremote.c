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
#include "rpcserver.h"

typedef struct _K2OSRPC_SERVER_CREATE_ACQUIRE_RESPONSE K2OSRPC_SERVER_CREATE_ACQUIRE_RESPONSE;
struct _K2OSRPC_SERVER_CREATE_ACQUIRE_RESPONSE
{
    K2OSRPC_MSG_RESPONSE_HDR                    ResponseHdr;
    K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA    Data;
};

void K2OSRPC_ServerThread_AtExit(K2OSRPC_THREAD *apThread);
void K2OSRPC_ServerThread_DoWork(K2OSRPC_THREAD *apRpcThread);

RPC_WORKITEM *
K2OSRPC_Server_GetWorkItem(
    void
)
{
    RPC_WORKITEM * pWorkItem;

    FUNC_ENTER;

    K2OS_CritSec_Enter(&gRpcGraphSec);

    if (0 != gpRpcServer->IdleWorkItemList.mNodeCount)
    {
        pWorkItem = K2_GET_CONTAINER(RPC_WORKITEM, gpRpcServer->IdleWorkItemList.mpHead, ListLink);
        K2LIST_Remove(&gpRpcServer->IdleWorkItemList, &pWorkItem->ListLink);
    }
    else
    {
        pWorkItem = NULL;
    }

    K2OS_CritSec_Leave(&gRpcGraphSec);

    if (NULL == pWorkItem)
    {
        pWorkItem = (RPC_WORKITEM *)K2OS_Heap_Alloc(sizeof(RPC_WORKITEM));
        if (NULL == pWorkItem)
        {
            FUNC_EXIT;
            return NULL;
        }
    }
    else
    {
        K2_ASSERT(NULL == pWorkItem->mDoneGateToken);
    }

    K2MEM_Zero(pWorkItem, sizeof(RPC_WORKITEM));

    FUNC_EXIT;
    return pWorkItem;
}

void
K2OSRPC_Server_PutWorkItem(
    RPC_WORKITEM *  apWorkItem
)
{
    FUNC_ENTER;

    K2_ASSERT(NULL == apWorkItem->mpHandle);
    K2_ASSERT(NULL == apWorkItem->mDoneGateToken);

    K2OS_CritSec_Enter(&gRpcGraphSec);

    K2LIST_AddAtTail(&gpRpcServer->IdleWorkItemList, &apWorkItem->ListLink);

    K2OS_CritSec_Leave(&gRpcGraphSec);

    FUNC_EXIT;
}

RPC_THREAD *
K2OSRPC_Server_GetThread(
    void
)
{
    RPC_THREAD *pThread;
    K2STAT      stat;

    FUNC_ENTER;

    K2OS_CritSec_Enter(&gRpcGraphSec);

    if (gpRpcServer->IdleThreadList.mNodeCount > 0)
    {
        pThread = K2_GET_CONTAINER(RPC_THREAD, gpRpcServer->IdleThreadList.mpHead, ListLink);
        K2_ASSERT(pThread->mIsOnList);
        K2_ASSERT(!pThread->mIsActive);
        K2LIST_Remove(&gpRpcServer->IdleThreadList, &pThread->ListLink);
        pThread->mIsOnList = FALSE;
    }
    else
    {
        pThread = NULL;
    }

    K2OS_CritSec_Leave(&gRpcGraphSec);

    if (NULL != pThread)
    {
        FUNC_EXIT;
        return pThread;
    }

    pThread = (RPC_THREAD *)K2OS_Heap_Alloc(sizeof(RPC_THREAD));
    if (NULL == pThread)
    {
        FUNC_EXIT;
        return NULL;
    }

    K2MEM_Zero(pThread, sizeof(RPC_THREAD));

    stat = K2OSRPC_Thread_Create("RpcServer Worker", &pThread->RpcThread, K2OSRPC_ServerThread_AtExit, K2OSRPC_ServerThread_DoWork);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Heap_Free(pThread);
        FUNC_EXIT;
        return NULL;
    }

    FUNC_EXIT;
    return pThread;
}

void
K2OSRPC_Server_PutThread(
    RPC_THREAD * apThread
)
{
    FUNC_ENTER;

    K2OS_CritSec_Enter(&gRpcGraphSec);

    if (apThread->mIsActive)
    {
        K2_ASSERT(apThread->mIsOnList);
        K2LIST_Remove(&gpRpcServer->ActiveThreadList, &apThread->ListLink);
        apThread->mIsOnList = FALSE;
        apThread->mIsActive = FALSE;
    }

    K2LIST_AddAtTail(&gpRpcServer->IdleThreadList, &apThread->ListLink);
    apThread->mIsOnList = TRUE;

    K2OS_CritSec_Leave(&gRpcGraphSec);
    FUNC_EXIT;
}

void
K2OSRPC_ServerThread_AtExit(
    K2OSRPC_THREAD* apThread
)
{
    RPC_THREAD *pThread;

    FUNC_ENTER;

    pThread = K2_GET_CONTAINER(RPC_THREAD, apThread, RpcThread);
    K2_ASSERT(!pThread->mIsActive);
    K2_ASSERT(!pThread->mIsOnList);

    K2OS_Heap_Free(pThread);

    FUNC_EXIT;
}

void
K2OSRPC_ServerThread_DoWork(
    K2OSRPC_THREAD* apRpcThread
)
{
    RPC_THREAD *                                pThisThread;
    RPC_WORKITEM *                              pWorkItem;
    RPC_CONN *                                  pConn;
    K2OSRPC_MSG_REQUEST_HDR *                   pReqHdr;
    K2OSRPC_MSG_RESPONSE_HDR *                  pRespHdr;
    K2OSRPC_MSG_CREATE_REQUEST_DATA *           pCreateReq;
    UINT32                                      respBytes;
    K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA *  pCretResp;
    K2OSRPC_SERVER_OBJ_HANDLE *                 pServHandle;
    K2OS_RPC_CALLARGS                           callArgs;

    // called whenever thread is woken up
    FUNC_ENTER;

    K2OS_Thread_SetLastStatus(K2STAT_NO_ERROR);

    pThisThread = K2_GET_CONTAINER(RPC_THREAD, apRpcThread, RpcThread);
    K2_ASSERT(pThisThread->mIsActive);
    K2_ASSERT(pThisThread->mIsOnList);

    pWorkItem = pThisThread->mpWorkItem;
    K2_ASSERT(NULL != pWorkItem);

    pConn = pWorkItem->mpConn;
    K2_ASSERT(NULL != pConn);

    if (!pWorkItem->mIsCancelled)
    {
        pReqHdr = pWorkItem->mpReqHdr;
        K2_ASSERT(NULL != pReqHdr);

        pRespHdr = pWorkItem->mpRespHdr;
        K2_ASSERT(NULL != pRespHdr);

        respBytes = sizeof(K2OSRPC_MSG_RESPONSE_HDR);

        switch (pReqHdr->mRequestType)
        {
        case K2OSRPC_ServerClientRequest_Create:
            pCreateReq = (K2OSRPC_MSG_CREATE_REQUEST_DATA *)pWorkItem->mpInBuf;

            pCretResp = (K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA *)pWorkItem->mpOutBuf;

            pServHandle = (K2OSRPC_SERVER_OBJ_HANDLE *)K2OSRPC_Server_LocalCreateObj(
                &pCreateReq->mClassId, 
                pCreateReq->mCreatorContext, 
                pConn->ServerConnTreeNode.mUserVal,
                pConn->mDisconnectedGateToken, 
                &pCretResp->mServerObjId
            );

            if (NULL == pServHandle)
            {
                pRespHdr->mStatus = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(pRespHdr->mStatus));
            }
            else
            {
                K2_ASSERT(0 != pCretResp->mServerObjId);
                pCretResp->mServerHandle = (UINT32)pServHandle;
                respBytes += sizeof(K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA);

                K2OS_CritSec_Enter(&gRpcGraphSec);

                pServHandle->mpConn = pConn;
                K2LIST_AddAtTail(&pConn->HandleList, &pServHandle->ConnHandleListListLink);

                K2OS_CritSec_Leave(&gRpcGraphSec);

                pRespHdr->mStatus = K2STAT_NO_ERROR;
            }

            break;

        case K2OSRPC_ServerClientRequest_Call:
            pServHandle = pWorkItem->mpHandle;
            K2_ASSERT(NULL != pServHandle);

            callArgs.mMethodId = pReqHdr->mTargetMethodId;
            callArgs.mpInBuf = pWorkItem->mpInBuf;
            callArgs.mInBufByteCount = pReqHdr->mInByteCount;
            callArgs.mpOutBuf = pWorkItem->mpOutBuf;
            callArgs.mOutBufByteCount = pReqHdr->mOutBufSizeProvided;

            pRespHdr->mStatus = K2OSRPC_Server_LocalCall(pServHandle, &callArgs, &pRespHdr->mResultsByteCount);
            if (K2STAT_IS_ERROR(pRespHdr->mStatus))
            {
                pRespHdr->mResultsByteCount = 0;
            }

            respBytes += pRespHdr->mResultsByteCount;

            break;

        case K2OSRPC_ServerClientRequest_Release:
            pRespHdr->mStatus = K2STAT_NO_ERROR;
            break;

        default:
            K2_ASSERT(0);
            break;
        }

        K2OS_IpcEnd_Send(pConn->mIpcEnd, pRespHdr, respBytes);
    }

    K2OS_Heap_Free(pWorkItem->mpReqHdr);

    pWorkItem->mpReqHdr = NULL;
    pWorkItem->mpRespHdr = NULL;

    K2OS_CritSec_Enter(&pConn->WorkItemListSec);

    K2LIST_Remove(&pConn->WorkItemList, &pWorkItem->ListLink);
    pWorkItem->mpConn = NULL;

    K2OS_CritSec_Leave(&pConn->WorkItemListSec);

    if (NULL != pWorkItem->mpHandle)
    {
        //
        // this may call the object's delete method which can
        // take an indeterminate amount of time
        //
        K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)pWorkItem->mpHandle);
        pWorkItem->mpHandle = NULL;
    }

    pWorkItem->mpInBuf = pWorkItem->mpOutBuf = NULL;

    K2OSRPC_Server_PutWorkItem(pWorkItem);

    pThisThread->mpWorkItem = NULL;

    K2OSRPC_Server_PutThread(pThisThread);

    FUNC_EXIT;
}

void
K2OSRPC_ServerConn_RespondWithError(
    K2OS_IPCEND aEndpoint,
    UINT32      aCallerRef,
    K2STAT      aErrorStatus
)
{
    K2OSRPC_MSG_RESPONSE_HDR  respHdr;

    FUNC_ENTER;

    K2_ASSERT(K2STAT_IS_ERROR(aErrorStatus));

    K2MEM_Zero(&respHdr, sizeof(respHdr));
    respHdr.mMarker = K2OSRPC_MSG_RESPONSE_HDR_MARKER;
    respHdr.mCallerRef = aCallerRef;
    respHdr.mStatus = aErrorStatus;

    K2OS_IpcEnd_Send(aEndpoint, &respHdr, sizeof(respHdr));

    FUNC_EXIT;
}

void
K2OSRPC_ServerConn_OnRecv(
    K2OS_IPCEND     aEndpoint,
    void *          apContext,
    UINT8 const *   apData,
    UINT32          aByteCount
)
{
    RPC_CONN *                              pConn;
    K2OSRPC_MSG_REQUEST_HDR const *         pReqHdr;
    BOOL                                    ok;
    UINT32                                  workItemBytes;
    RPC_WORKITEM *                          pWorkItem;
    K2LIST_LINK *                           pListLink;
    K2OSRPC_SERVER_OBJ_HANDLE *             pHandle;
    K2OSRPC_SERVER_CREATE_ACQUIRE_RESPONSE  acquireResp;
    UINT8 *                                 pIoBuffer;
    K2STAT                                  stat;
    RPC_THREAD *                            pWorkerThread;
    K2_EXCEPTION_TRAP                       trap;

    FUNC_ENTER;

    pConn = (RPC_CONN *)apContext;
    pReqHdr = (K2OSRPC_MSG_REQUEST_HDR const *)apData;

    if ((aByteCount < sizeof(K2OSRPC_MSG_REQUEST_HDR)) ||
        (pReqHdr->mCallerRef == 0))
    {
        K2OSRPC_Debug("***RPC Recv data too small to be a request (%d) or with null caller ref\n", aByteCount);
        FUNC_EXIT;
        return;
    }

    ok = FALSE;

    if ((pReqHdr->mRequestType != K2OSRPC_ServerClientRequest_Invalid) &&
        (pReqHdr->mRequestType < K2OSRPC_ServerClientRequestType_Count) &&
        (aByteCount == (pReqHdr->mInByteCount + sizeof(K2OSRPC_MSG_REQUEST_HDR))))
    {
        switch (pReqHdr->mRequestType)
        {
        case K2OSRPC_ServerClientRequest_Create:
            if ((pReqHdr->mInByteCount == sizeof(K2OSRPC_MSG_CREATE_REQUEST_DATA)) &&
                (pReqHdr->mOutBufSizeProvided >= sizeof(K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA)))
                ok = TRUE;
            break;

        case K2OSRPC_ServerClientRequest_AcquireByObjId:
        case K2OSRPC_ServerClientRequest_AcquireByIfInstId:
            if ((pReqHdr->mInByteCount == 0) &&
                (pReqHdr->mOutBufSizeProvided >= sizeof(K2OSRPC_MSG_CREATE_ACQUIRE_RESPONSE_DATA)))
                ok = TRUE;
            break;

        case K2OSRPC_ServerClientRequest_Call:
            ok = TRUE;
            break;

        case K2OSRPC_ServerClientRequest_Release:
            if (pReqHdr->mInByteCount == 0)
                ok = TRUE;
            break;
        }
    }

    if (!ok)
    {
        K2OSRPC_Debug("***RPC Recv malformed request (size %d)\n", aByteCount);
        K2OSRPC_Debug("mCallerRef              %08X\n", pReqHdr->mCallerRef);
        K2OSRPC_Debug("mInByteCount            %04X\n", pReqHdr->mInByteCount);
        K2OSRPC_Debug("mOutBufSizeProvided     %04X\n", pReqHdr->mOutBufSizeProvided);
        K2OSRPC_Debug("mRequestType            %04X\n", pReqHdr->mRequestType);
        K2OSRPC_Debug("mTargetId               %08X\n", pReqHdr->mTargetId);
        K2OSRPC_Debug("mTargetMethodId         %04X\n", pReqHdr->mTargetMethodId);
        K2OSRPC_ServerConn_RespondWithError(aEndpoint, pReqHdr->mCallerRef, K2STAT_ERROR_BAD_FORMAT);
        FUNC_EXIT;
        return;
    }

    if (pConn->WorkItemList.mNodeCount != 0)
    {
        K2OS_CritSec_Enter(&pConn->WorkItemListSec);
        pListLink = pConn->WorkItemList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pWorkItem = K2_GET_CONTAINER(RPC_WORKITEM, pListLink, ListLink);
                pListLink = pListLink->mpNext;
                if (pWorkItem->mpReqHdr->mCallerRef == pReqHdr->mCallerRef)
                    break;
            } while (NULL != pListLink);
        }
        K2OS_CritSec_Leave(&pConn->WorkItemListSec);
        if (NULL != pListLink)
        {
            //
            // client sent duplicate caller ref to one that is in flight
            //
            K2OSRPC_Debug("***RPC Recv caller ref (%d) that already is in flight\n", pReqHdr->mCallerRef);
            K2OSRPC_ServerConn_RespondWithError(aEndpoint, pReqHdr->mCallerRef, K2STAT_ERROR_IN_USE);
            FUNC_EXIT;
            return;
        }
    }

    //
    // acquire is just finding the object and creating a handle which is fast and can be 
    // done on the receive thread
    //
    if ((pReqHdr->mRequestType == K2OSRPC_ServerClientRequest_AcquireByObjId) ||
        (pReqHdr->mRequestType == K2OSRPC_ServerClientRequest_AcquireByIfInstId))
    {
        if (pReqHdr->mRequestType == K2OSRPC_ServerClientRequest_AcquireByObjId)
        {
//            K2OSRPC_Debug("RpcServer in process %d got attach by object id(%d) from process %d\n", 
//                K2OS_Process_GetId(), pReqHdr->mTargetId, pConn->ServerConnTreeNode.mUserVal);
            pHandle = (K2OSRPC_SERVER_OBJ_HANDLE *)K2OSRPC_Server_LocalAttachByObjId(pReqHdr->mTargetId);
            K2_ASSERT((NULL == pHandle) || (pHandle->mpObj->ServerObjIdTreeNode.mUserVal == pReqHdr->mTargetId));
        }
        else
        {
//            K2OSRPC_Debug("RpcServer in process %d got attach by interface instance id(%d) from process %d\n",
//                K2OS_Process_GetId(), pReqHdr->mTargetId, pConn->ServerConnTreeNode.mUserVal);
            pHandle = (K2OSRPC_SERVER_OBJ_HANDLE *)K2OSRPC_Server_LocalAttachByIfInstId(pReqHdr->mTargetId, NULL);
        }
        if (NULL == pHandle)
        {
            K2OSRPC_Debug("***RPC Recv call to acquire unknown object (method %d, id %d)\n", pReqHdr->mRequestType, pReqHdr->mTargetId);
            K2OSRPC_ServerConn_RespondWithError(aEndpoint, pReqHdr->mCallerRef, K2STAT_ERROR_OBJECT_NOT_FOUND);
        }
        else
        {
            if (pHandle->mpObj->mBlockAcquire)
            {
                K2OSRPC_Debug("!!!RPC acquire blocked\n");
                // this release is for the addref above, which is superfluous to the 
                // handle held by the connection's handle list. so the object delete
                // cannot occur here
                K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)pHandle);
                K2OSRPC_ServerConn_RespondWithError(aEndpoint, pReqHdr->mCallerRef, K2STAT_ERROR_NOT_ALLOWED);
            }
            else
            {
                if (NULL != pHandle->mpObj->mpClass->Def.OnAttach)
                {
#if TRAP_EXCEPTIONS
                    stat = K2_EXTRAP(&trap, pHandle->mpObj->mpClass->Def.OnAttach(
                        (K2OS_RPC_OBJ)pHandle->mpObj,
                        pHandle->mpObj->mUserContext,
                        pConn->ServerConnTreeNode.mUserVal,
                        &pHandle->mUseContext
                    ));
#else
                    stat = pHandle->mpObj->mpClass->Def.OnAttach(
                        (K2OS_RPC_OBJ)pHandle->mpObj,
                        pHandle->mpObj->mUserContext,
                        pConn->ServerConnTreeNode.mUserVal,
                        &pHandle->mUseContext
                    );
#endif
                }
                else
                {
                    stat = K2STAT_NO_ERROR;
                }

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OSRPC_ReleaseInternal((K2OS_RPC_OBJ_HANDLE)pHandle, FALSE);
                    K2OSRPC_ServerConn_RespondWithError(aEndpoint, pReqHdr->mCallerRef, stat);
                }
                else
                {
                    K2MEM_Zero(&acquireResp, sizeof(acquireResp));
                    acquireResp.ResponseHdr.mMarker = K2OSRPC_MSG_RESPONSE_HDR_MARKER;
                    acquireResp.ResponseHdr.mCallerRef = pReqHdr->mCallerRef;
                    acquireResp.ResponseHdr.mResultsByteCount = sizeof(acquireResp.Data);
                    acquireResp.ResponseHdr.mStatus = K2STAT_NO_ERROR;
                    acquireResp.Data.mServerObjId = pHandle->mpObj->ServerObjIdTreeNode.mUserVal;
                    acquireResp.Data.mServerHandle = (UINT32)pHandle;

                    if (!K2OS_IpcEnd_Send(aEndpoint, &acquireResp, sizeof(acquireResp)))
                    {
                        // this release call has a low-probability of
                        // indeterminate execution time as the object's delete may 
                        // conceivably occur if all other handles are released
                        // between the attachbyobjid above and this point.  which is
                        // super unlikely but possible
                        K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)pHandle);
                    }
                    else
                    {
                        K2OS_CritSec_Enter(&gRpcGraphSec);

                        K2LIST_AddAtTail(&pConn->HandleList, &pHandle->ConnHandleListListLink);

                        K2OS_CritSec_Leave(&gRpcGraphSec);
                    }
                }
            }
        }

        FUNC_EXIT;
        return;
    }

    //
    // create, call, or release; all of these have indeterminate execution time due to
    // callback or potential callback to class functions.  so all of these have to happen 
    // on a thread separate from the connection thread
    // 
    // if call or release the targetid is a handle
    // so lets try to find that handle and addref it here
    //
    pHandle = NULL;

    if (pReqHdr->mRequestType != K2OSRPC_ServerClientRequest_Create)
    {
        K2OS_CritSec_Enter(&gRpcGraphSec);

        pListLink = pConn->HandleList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pHandle = K2_GET_CONTAINER(K2OSRPC_SERVER_OBJ_HANDLE, pListLink, ConnHandleListListLink);
                if (pHandle->Hdr.GlobalHandleTreeNode.mUserVal == pReqHdr->mTargetId)
                {
                    K2ATOMIC_Inc(&pHandle->Hdr.mRefs);
                    break;
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }

        K2OS_CritSec_Leave(&gRpcGraphSec);

        if (NULL == pListLink)
        {
            //
            // specified handle not found
            //
            K2OSRPC_Debug("***RPC Recv call to unknown handle\n");
            K2OSRPC_ServerConn_RespondWithError(aEndpoint, pReqHdr->mCallerRef, K2STAT_ERROR_NOT_FOUND);
            FUNC_EXIT;
            return;
        }

        K2_ASSERT(NULL != pHandle);
    }

    //
    // method action will proceed on a worker thread
    //
    workItemBytes = ((aByteCount + 3) & ~3) +
        sizeof(K2OSRPC_MSG_RESPONSE_HDR) +
        pReqHdr->mOutBufSizeProvided;
    pIoBuffer = (UINT8 *)K2OS_Heap_Alloc(workItemBytes);
    if (NULL == pIoBuffer)
    {
        K2OSRPC_Debug("***RPC memory alloc failed (%d)\n", workItemBytes);
        K2OSRPC_ServerConn_RespondWithError(aEndpoint, pReqHdr->mCallerRef, K2STAT_ERROR_OUT_OF_MEMORY);
        if (NULL != pHandle)
        {
            // this release is for the addref above, which is superfluous to the 
            // handle held by the connection's handle list. so the object delete
            // cannot occur here
            K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)pHandle);
        }
        FUNC_EXIT;
        return;
    }

    stat = K2STAT_ERROR_UNKNOWN;

    do {
        pWorkItem = K2OSRPC_Server_GetWorkItem();
        if (NULL == pWorkItem)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            K2OSRPC_Debug("***RPC workitem acquire failed\n");
            break;
        }

        do {
            pWorkerThread = K2OSRPC_Server_GetThread();
            if (NULL == pWorkerThread)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                K2OSRPC_Debug("***RPC thread acquire failed\n");
                break;
            }

            //
            // cannot fail to move work to the worker thread from here on
            //
            stat = K2STAT_NO_ERROR;

            K2MEM_Copy(pIoBuffer, apData, aByteCount);
            K2MEM_Zero(pIoBuffer + aByteCount, workItemBytes - aByteCount);

            pWorkItem->mpConn = pConn;
            pWorkItem->mpHandle = pHandle;
            pWorkItem->mpReqHdr = (K2OSRPC_MSG_REQUEST_HDR *)pIoBuffer;
            pWorkItem->mpInBuf = pIoBuffer + sizeof(K2OSRPC_MSG_REQUEST_HDR);
            pWorkItem->mpRespHdr = (K2OSRPC_MSG_RESPONSE_HDR *)(pIoBuffer + ((aByteCount + 3) & ~3));
            pWorkItem->mpOutBuf = ((UINT8 *)pWorkItem->mpRespHdr) + sizeof(K2OSRPC_MSG_RESPONSE_HDR);
            pWorkItem->mIsCancelled = FALSE;
            pWorkItem->mpRespHdr->mMarker = K2OSRPC_MSG_RESPONSE_HDR_MARKER;
            pWorkItem->mpRespHdr->mCallerRef = pReqHdr->mCallerRef;
            pWorkItem->mpRespHdr->mStatus = K2STAT_ERROR_UNKNOWN;

            //
            // if this is a release remove the handle from the handle list now
            // as opposed to when the worker thread gets around to it
            //
            if (pReqHdr->mRequestType == K2OSRPC_ServerClientRequest_Release)
            {
                K2_ASSERT(NULL != pHandle);

                K2OS_CritSec_Enter(&gRpcGraphSec);

                K2LIST_Remove(&pConn->HandleList, &pHandle->ConnHandleListListLink);

                K2OS_CritSec_Leave(&gRpcGraphSec);

                //
                // additional handle refernce already held by K2ATOMIC_Inc above when 
                // handle was found.  so handle is still valid but the handle itself
                // has been removed from the connection handle list
                //
                K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)pHandle);
            }

            //
            // queue the work item to the connection workitem list and set the
            // worker thread as active
            //
            K2OS_CritSec_Enter(&pConn->WorkItemListSec);

            K2LIST_AddAtTail(&pConn->WorkItemList, &pWorkItem->ListLink);

            pWorkerThread->mpWorkItem = pWorkItem;
            K2LIST_AddAtTail(&gpRpcServer->ActiveThreadList, &pWorkerThread->ListLink);
            pWorkerThread->mIsActive = TRUE;
            pWorkerThread->mIsOnList = TRUE;

            K2OS_CritSec_Leave(&pConn->WorkItemListSec);

            //
            // now wake up the thread we got to handle the request
            //
            K2OSRPC_Thread_WakeUp(&pWorkerThread->RpcThread);

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OSRPC_Server_PutWorkItem(pWorkItem);
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Heap_Free(pIoBuffer);

        K2OSRPC_ServerConn_RespondWithError(aEndpoint, pReqHdr->mCallerRef, K2STAT_ERROR_OUT_OF_MEMORY);

        if (NULL != pHandle)
        {
            // this release is for the addref above, which is superfluous to the 
            // handle held by the connection's handle list. so the object delete
            // cannot occur here
            K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)pHandle);
        }
    }

    FUNC_EXIT;
}

void
K2OSRPC_ServerConnThread_AtExit(
    K2OSRPC_THREAD* apThread
)
{
    RPC_CONN * pThisConn;

    FUNC_ENTER;

    pThisConn = K2_GET_CONTAINER(RPC_CONN, apThread, CrtThread);

    if (NULL != pThisConn->mIpcEnd)
    {
        K2OS_IpcEnd_Delete(pThisConn->mIpcEnd);
    }

    if (NULL != pThisConn->mTokMailbox)
    {
        K2OS_Token_Destroy(pThisConn->mTokMailbox);
        pThisConn->mTokMailbox = NULL;
    }

    K2OS_CritSec_Done(&pThisConn->WorkItemListSec);

    K2OS_Heap_Free(pThisConn);

    FUNC_EXIT;
}

void
K2OSRPC_ServerConnThread_DoWork(
    K2OSRPC_THREAD * apRpcThread
)
{
    RPC_CONN *                  pThisConn;
    K2OS_WaitResult             waitResult;
    K2OS_MSG                    msg;
    RPC_WORKITEM *              pWorkItem;
    K2LIST_LINK *               pListLink;
    K2OSRPC_SERVER_OBJ_HANDLE * pServerHandle;

    FUNC_ENTER;

    pThisConn = K2_GET_CONTAINER(RPC_CONN, apRpcThread, CrtThread);

    if (!K2OS_IpcEnd_AcceptRequest(pThisConn->mIpcEnd, pThisConn->mRequestId))
    {
        K2OS_CritSec_Enter(&gRpcGraphSec);

        K2TREE_Remove(&gpRpcServer->ConnTree, &pThisConn->ServerConnTreeNode);
            
        K2OS_CritSec_Leave(&gRpcGraphSec);

        // try to reject don't care if it fails
        K2OS_Ipc_RejectRequest(pThisConn->mRequestId, K2OS_Thread_GetLastStatus());
    }
    else
    {
        //
        // connected now. connection message should be sitting in mailbox now
        // drain all the messages until it is empty. if we don't get a connected
        // message (amongst anything else) we will close the connection
        //
        do {
            if (!K2OS_Mailbox_Recv(pThisConn->mTokMailbox, &msg, 0))
            {
                K2OS_Thread_WaitOne(&waitResult, pThisConn->mTokMailbox, K2OS_TIMEOUT_INFINITE);
            }
            else
            {
                if (!K2OS_IpcEnd_ProcessMsg(&msg))
                {
                    if ((msg.mType == K2OS_SYSTEM_MSGTYPE_IPC) &&
                        (msg.mShort == K2OS_SYSTEM_MSG_IPC_SHORT_REQUEST))
                    {
                        K2OS_Ipc_RejectRequest(msg.mPayload[2], K2STAT_ERROR_NOT_ALLOWED);
                    }
                }
            }
        } while (1);

        if (pThisConn->mIsConnected)
        {
            //
            // main conn processing after connection.  all actions are ipc msg driven
            // and handled through callbacks from the System_ProcessMsg function
            //
            do {
                if (!K2OS_Thread_WaitOne(&waitResult, pThisConn->mTokMailbox, K2OS_TIMEOUT_INFINITE))
                {
                    K2OSRPC_Debug("*** remote conn thread wait failed (%d), %08X\n", waitResult, K2OS_Thread_GetLastStatus());
                    break;
                }
                if (K2OS_Mailbox_Recv(pThisConn->mTokMailbox, &msg, 0))
                {
                    if (!K2OS_IpcEnd_ProcessMsg(&msg))
                    {
                        if ((msg.mType == K2OS_SYSTEM_MSGTYPE_IPC) &&
                            (msg.mShort == K2OS_SYSTEM_MSG_IPC_SHORT_REQUEST))
                        {
                            K2OS_Ipc_RejectRequest(msg.mPayload[2], K2STAT_ERROR_NOT_ALLOWED);
                        }
                    }
                }
            } while (pThisConn->mIsConnected);
        }

        //
        // disconnected; tear down the connection here
        //

        //
        // cancel any inflight calls we can
        // 
        K2OS_CritSec_Enter(&pThisConn->WorkItemListSec);

        pListLink = pThisConn->WorkItemList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pWorkItem = K2_GET_CONTAINER(RPC_WORKITEM, pListLink, ListLink);
                pListLink = pListLink->mpNext;
                pWorkItem->mIsCancelled = TRUE;
            } while (NULL != pListLink);
        }

        K2OS_CritSec_Leave(&pThisConn->WorkItemListSec);

        //
        // this will signal anybody who is blocking inside a class callback
        // that the caller has disconnected.  The inflight calls have all already
        // been cancelled.
        //
        K2OS_Gate_Open(pThisConn->mDisconnectedGateToken);

        //
        // wait for in-process work to be purged by worker threads by snapshotting the
        // work item list that the worker threads are servicing
        //
        do {
            waitResult = *((UINT32 volatile *)&pThisConn->WorkItemList.mNodeCount);
            if (0 == waitResult)
                break;
            K2OS_Thread_Sleep(200);
        } while (1);

        //
        // release any handles that this connection held
        //
        K2OS_CritSec_Enter(&gRpcGraphSec);

        do {
            pListLink = pThisConn->HandleList.mpHead;
            if (NULL == pListLink)
                break;

            pServerHandle = K2_GET_CONTAINER(K2OSRPC_SERVER_OBJ_HANDLE, pListLink, ConnHandleListListLink);
            pListLink = pListLink->mpNext;

            K2LIST_Remove(&pThisConn->HandleList, &pServerHandle->ConnHandleListListLink);

            pServerHandle->mpConn = NULL;

            K2OS_CritSec_Leave(&gRpcGraphSec);

            K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)pServerHandle);

            K2OS_CritSec_Enter(&gRpcGraphSec);

        } while (1);

        K2OS_CritSec_Leave(&gRpcGraphSec);
    }
        
    K2OSRPC_Thread_Release(apRpcThread);

    FUNC_EXIT;
}

void 
K2OSRPC_ServerConn_OnConnect(
    K2OS_IPCEND aEndpoint,
    void *      apContext,
    UINT32      aRemoteMaxMsgBytes
)
{
    RPC_CONN *  pConn;

    FUNC_ENTER;

    pConn = (RPC_CONN *)apContext;
    pConn->mIsConnected = TRUE;

    FUNC_EXIT;
}

void 
K2OSRPC_ServerConn_OnDisconnect(
    K2OS_IPCEND aEndpoint,
    void *      apContext
)
{
    RPC_CONN * pConn;

    FUNC_ENTER;

    pConn = (RPC_CONN *)apContext;

    K2OS_CritSec_Enter(&gRpcGraphSec);

    pConn->mIsConnected = FALSE;

    K2TREE_Remove(&gpRpcServer->ConnTree, &pConn->ServerConnTreeNode);

    K2OS_CritSec_Leave(&gRpcGraphSec);

    FUNC_EXIT;
}

void
K2OSRPC_Server_OnConnectRequest(
    UINT32 aRequestorProcessId,
    UINT32 aRequestId
)
{
    static const K2OS_IPCPROCESSMSG_CALLBACKS sServerConnIpcFuncTab =
    {
        K2OSRPC_ServerConn_OnConnect,
        K2OSRPC_ServerConn_OnRecv,
        K2OSRPC_ServerConn_OnDisconnect,
        NULL
    };
    K2TREE_NODE *   pTreeNode;
    RPC_CONN *      pConn;
    K2STAT          stat;
    char            threadName[K2OS_THREAD_NAME_BUFFER_CHARS];

    FUNC_ENTER;

    K2OS_CritSec_Enter(&gRpcGraphSec);

    pTreeNode = K2TREE_Find(&gpRpcServer->ConnTree, aRequestorProcessId);

    K2OS_CritSec_Leave(&gRpcGraphSec);

    if (NULL != pTreeNode)
    {
        K2OS_Ipc_RejectRequest(aRequestId, K2STAT_ERROR_CONNECTED);
        FUNC_EXIT;
        return;
    }

    //
    // try connecting
    //
    pConn = (RPC_CONN *)K2OS_Heap_Alloc(sizeof(RPC_CONN));
    if (NULL == pConn)
    {
        K2OS_Ipc_RejectRequest(aRequestId, K2OS_Thread_GetLastStatus());
        FUNC_EXIT;
        return;
    }

    do {
        K2MEM_Zero(pConn, sizeof(RPC_CONN));

        pConn->ServerConnTreeNode.mUserVal = aRequestorProcessId;
        pConn->mRequestId = aRequestId;
        K2LIST_Init(&pConn->HandleList);

        pConn->mTokMailbox = K2OS_Mailbox_Create();
        if (NULL == pConn->mTokMailbox)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            break;
        }

        do {
            pConn->mDisconnectedGateToken = K2OS_Gate_Create(FALSE);
            if (NULL == pConn->mDisconnectedGateToken)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                break;
            }

            do {
                if (!K2OS_CritSec_Init(&pConn->WorkItemListSec))
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                    break;
                }

                K2LIST_Init(&pConn->WorkItemList);

                pConn->mIpcEnd = K2OS_IpcEnd_Create(pConn->mTokMailbox, 32, 1024, pConn, &sServerConnIpcFuncTab);
                if (NULL == pConn->mIpcEnd)
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                }
                else
                {
                    K2OS_CritSec_Enter(&gRpcGraphSec);

                    pTreeNode = K2TREE_Find(&gpRpcServer->ConnTree, aRequestorProcessId);
                    if (NULL == pTreeNode)
                    {
                        K2ASC_PrintfLen(threadName, K2OS_THREAD_NAME_BUFFER_CHARS - 1, "RpcServerConn %d->%d", aRequestorProcessId, K2OS_Process_GetId());
                        threadName[K2OS_THREAD_NAME_BUFFER_CHARS - 1] = 0;
                        stat = K2OSRPC_Thread_Create(threadName, &pConn->CrtThread, K2OSRPC_ServerConnThread_AtExit, K2OSRPC_ServerConnThread_DoWork);
                        if (!K2STAT_IS_ERROR(stat))
                        {
                            //
                            // thread takes ownership of everything from here
                            // conn is only touched to wake up the thread outside
                            // the lock below
                            //
                            K2TREE_Insert(&gpRpcServer->ConnTree, pConn->ServerConnTreeNode.mUserVal, &pConn->ServerConnTreeNode);
                        }
                    }
                    else
                    {
                        // race condition and we lost
                        stat = K2STAT_ERROR_CONNECTED;
                    }

                    K2OS_CritSec_Leave(&gRpcGraphSec);

                    if (K2STAT_IS_ERROR(stat))
                    {
                        K2OS_IpcEnd_Delete(pConn->mIpcEnd);
                    }
                    else
                    {
                        K2OSRPC_Thread_WakeUp(&pConn->CrtThread);
                    }
                }

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OS_CritSec_Done(&pConn->WorkItemListSec);
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_Token_Destroy(pConn->mDisconnectedGateToken);
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Token_Destroy(pConn->mTokMailbox);
            pConn->mTokMailbox = NULL;
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Ipc_RejectRequest(aRequestId, stat);
        K2OS_Heap_Free(pConn);
    }

    FUNC_EXIT;
}

