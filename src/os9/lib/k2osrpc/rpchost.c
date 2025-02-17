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

RPC_SERVER * volatile   gpRpcServer;
static INT32 volatile   sgNextServerObjId;

K2OS_MAILBOX_TOKEN 
K2OSRPC_Server_SwapNotifyTarget(
    K2OSRPC_SERVER_OBJ_HANDLE * apHandle,
    K2OS_MAILBOX_TOKEN         aTokMailslot
)
{
    K2OS_MAILBOX_TOKEN tokMailslot;

    FUNC_ENTER;

    // not possible since handle existed
    K2_ASSERT(NULL != gpRpcServer);

    K2OS_CritSec_Enter(&gRpcGraphSec);

    tokMailslot = apHandle->mTokNotifyMailbox;
    apHandle->mTokNotifyMailbox = aTokMailslot;

    K2OS_CritSec_Leave(&gRpcGraphSec);

    FUNC_EXIT;

    return tokMailslot;
}

BOOL
K2OS_RpcObj_GetDetail(
    K2OS_RPC_OBJ    aObj,
    UINT32 *        apRetContext, 
    UINT32 *        apRetObjId
)
{
    K2TREE_NODE *   pTreeNode;
    RPC_OBJ *       pObj;

    FUNC_ENTER;

    if (NULL == aObj)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return FALSE;
    }

    if (NULL == gpRpcServer)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return FALSE;
    }

    K2OS_CritSec_Enter(&gRpcGraphSec);

    pTreeNode = K2TREE_Find(&gpRpcServer->ObjByPtrTree, (UINT_PTR)aObj);
    if (NULL != pTreeNode)
    {
        pObj = K2_GET_CONTAINER(RPC_OBJ, pTreeNode, ServerObjPtrTreeNode);
        if (NULL != apRetContext)
        {
            *apRetContext = pObj->mUserContext;
        }
        if (NULL != apRetObjId)
        {
            *apRetObjId = pObj->ServerObjIdTreeNode.mUserVal;
        }
    }

    K2OS_CritSec_Leave(&gRpcGraphSec);

    if (NULL == pTreeNode)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return FALSE;
    }

    FUNC_EXIT;
    return TRUE;
}

BOOL            
K2OS_RpcObj_SendNotify(
    K2OS_RPC_OBJ    aObj,
    UINT32          aSpecificUseOrZero,
    UINT32          aNotifyCode,
    UINT32          aNotifyData
)
{
    K2TREE_NODE *               pTreeNode;
    RPC_OBJ *                   pObj;
    K2LIST_LINK *               pListLink;
    K2OSRPC_SERVER_OBJ_HANDLE * pHandle;
    K2OS_MSG                    msg;
    K2OSRPC_OBJ_NOTIFY          notify;

    FUNC_ENTER;

    if (NULL == aObj)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return FALSE;
    }

    if (NULL == gpRpcServer)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return FALSE;
    }

    msg.mMsgType = K2OS_SYSTEM_MSGTYPE_RPC;
    msg.mShort = K2OS_SYSTEM_MSG_RPC_SHORT_NOTIFY;
    notify.mMarker = K2OSRPC_OBJ_NOTIFY_MARKER;
    notify.mCode = msg.mPayload[1] = aNotifyCode;
    notify.mData = msg.mPayload[2] = aNotifyData;

    K2OS_CritSec_Enter(&gRpcGraphSec);

    pTreeNode = K2TREE_Find(&gpRpcServer->ObjByPtrTree, (UINT_PTR)aObj);
    if (NULL != pTreeNode)
    {
        pObj = K2_GET_CONTAINER(RPC_OBJ, pTreeNode, ServerObjPtrTreeNode);

        pListLink = pObj->HandleList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pHandle = K2_GET_CONTAINER(K2OSRPC_SERVER_OBJ_HANDLE, pListLink, ObjHandleListLink);
                K2_ASSERT(pHandle->mpObj == pObj);

                if ((0 == aSpecificUseOrZero) ||
                    (aSpecificUseOrZero == pHandle->mUseContext))
                {
                    notify.mServerHandle = msg.mPayload[0] = (UINT32)pHandle;

                    if (NULL == pHandle->mpConnToClient)
                    {
                        // no connection means that the handle is for a local user
                        if (NULL != pHandle->mTokNotifyMailbox)
                        {
                            K2OS_Mailbox_Send(pHandle->mTokNotifyMailbox, &msg);
                        }
                    }
                    else
                    {
                        K2OS_IpcEnd_Send(pHandle->mpConnToClient->mIpcEnd, &notify, sizeof(notify));
                    }
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }
    }

    K2OS_CritSec_Leave(&gRpcGraphSec);

    if (NULL == pTreeNode)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return FALSE;
    }

    FUNC_EXIT;
    return TRUE;
}

K2OS_RPC_IFINST 
K2OS_RpcObj_AddIfInst(
    K2OS_RPC_OBJ        aObj,
    UINT32              aClassCode,
    K2_GUID128 const *  apSpecific, 
    K2OS_IFINST_ID *    apRetId,
    BOOL                aPublish
)
{
    K2TREE_NODE *   pTreeNode;
    RPC_OBJ *       pObj;
    K2STAT          stat;
    RPC_IFINST *    pIfInst;

    FUNC_ENTER;

    if (apRetId != NULL)
    {
        *apRetId = 0;
    }

    if ((NULL == aObj) || 
        (NULL == apSpecific) ||
        (K2MEM_VerifyZero(apSpecific, sizeof(K2_GUID128))))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return FALSE;
    }

    if (NULL == gpRpcServer)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return FALSE;
    }

    pIfInst = (RPC_IFINST *)K2OS_Heap_Alloc(sizeof(RPC_IFINST));
    if (NULL == pIfInst)
    {
        FUNC_EXIT;
        return NULL;
    }

    stat = K2STAT_NO_ERROR;

    K2MEM_Zero(pIfInst, sizeof(RPC_IFINST));

    K2OS_CritSec_Enter(&gRpcGraphSec);

    pTreeNode = K2TREE_Find(&gpRpcServer->ObjByPtrTree, (UINT_PTR)aObj);
    if (NULL != pTreeNode)
    {
        pObj = K2_GET_CONTAINER(RPC_OBJ, pTreeNode, ServerObjPtrTreeNode);

        pIfInst->mTokSysInst = K2OS_IfInst_Create((UINT32)pIfInst, &pIfInst->ServerIfInstIdTreeNode.mUserVal);
        if (NULL != pIfInst->mTokSysInst)
        {
            if (!K2OS_IfInst_SetMailbox(pIfInst->mTokSysInst, gpRpcServer->mTokMailbox))
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                K2OS_Token_Destroy(pIfInst->mTokSysInst);
            }
            else
            {
                if ((aPublish) && (!K2OS_IfInst_Publish(pIfInst->mTokSysInst, aClassCode, apSpecific)))
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2OS_Token_Destroy(pIfInst->mTokSysInst);
                    pIfInst->mTokSysInst = NULL;
                }
                else
                {
                    pIfInst->mpObj = pObj;

                    K2TREE_Insert(&gpRpcServer->IfInstIdTree, pIfInst->ServerIfInstIdTreeNode.mUserVal, &pIfInst->ServerIfInstIdTreeNode);
                    pIfInst->ServerIfInstPtrTreeNode.mUserVal = (UINT32)pIfInst;

                    K2TREE_Insert(&gpRpcServer->IfInstPtrTree, pIfInst->ServerIfInstPtrTreeNode.mUserVal, &pIfInst->ServerIfInstPtrTreeNode);

                    K2LIST_AddAtTail(&pObj->IfInstList, &pIfInst->ObjIfInstListLink);

                    if (NULL != apRetId)
                    {
                        *apRetId = pIfInst->ServerIfInstIdTreeNode.mUserVal;
                    }
                }
            }
        }
        else
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
        }
    }
    else
    {
        stat = K2STAT_ERROR_NOT_FOUND;
    }

    K2OS_CritSec_Leave(&gRpcGraphSec);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Heap_Free(pIfInst);
        pIfInst = NULL;
        K2OS_Thread_SetLastStatus(stat);
    }
    else
    {
        K2_ASSERT(NULL != pIfInst);
    }

    FUNC_EXIT;
    return (K2OS_RPC_IFINST)pIfInst;
}

BOOL            
K2OS_RpcObj_RemoveIfInst(
    K2OS_RPC_OBJ    aObject,
    K2OS_RPC_IFINST aIfInst
)
{
    RPC_IFINST *    pIfInst;
    RPC_OBJ *       pObj;
    K2TREE_NODE *   pTreeNode;
    BOOL            ok;

    FUNC_ENTER;

    if ((NULL == aObject) || (NULL == aIfInst))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return FALSE;
    }

    if (NULL == gpRpcServer)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return FALSE;
    }

    pIfInst = NULL;

    K2OS_CritSec_Enter(&gRpcGraphSec);

    pTreeNode = K2TREE_Find(&gpRpcServer->IfInstPtrTree, (UINT_PTR)aIfInst);
    if (NULL != pTreeNode)
    {
        pIfInst = K2_GET_CONTAINER(RPC_IFINST, pTreeNode, ServerIfInstPtrTreeNode);
        pObj = (RPC_OBJ *)aObject;
        if (pIfInst->mpObj != pObj)
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
            pIfInst = NULL;
        }
        else
        {
            ok = K2OS_Token_Destroy(pIfInst->mTokSysInst);
            K2_ASSERT(ok);
            if (ok)
            {
                pIfInst->mTokSysInst = NULL;
                K2LIST_Remove(&pObj->IfInstList, &pIfInst->ObjIfInstListLink);
                K2TREE_Remove(&gpRpcServer->IfInstIdTree, &pIfInst->ServerIfInstIdTreeNode);
                K2TREE_Remove(&gpRpcServer->IfInstPtrTree, &pIfInst->ServerIfInstPtrTreeNode);
                pIfInst->mpObj = NULL;
            }
            else
            {
                pIfInst = NULL;
            }
        }
    }

    K2OS_CritSec_Leave(&gRpcGraphSec);

    if (NULL == pIfInst)
    {
        FUNC_EXIT;
        return FALSE;
    }

    K2OS_Heap_Free(pIfInst);

    FUNC_EXIT;
    return TRUE;
}

UINT32 
K2OSRPC_Server_GetObjectId(
    K2OSRPC_SERVER_OBJ_HANDLE * pHandle
)
{
    UINT32 result;

    FUNC_ENTER;

    // not possible since handle existed
    K2_ASSERT(NULL != gpRpcServer);

    // handle ref held.  object cannot disappear from id tree here
    K2_ASSERT(NULL != pHandle->mpObj);
    result = pHandle->mpObj->ServerObjIdTreeNode.mUserVal;

    FUNC_EXIT;

    return result;
}

K2OS_RPC_OBJ_HANDLE
K2OSRPC_Server_LocalCreateObj(
    K2_GUID128 const *  apClassId,
    UINT32              aCreatorContext,
    UINT32              aCreatorProcessId,
    K2OS_SIGNAL_TOKEN   aTokRemoteDisconnect,
    UINT32 *            apRetObjId
)
{
    RPC_CLASS *                 pClass;
    RPC_OBJ *                   pObj;
    K2OSRPC_SERVER_OBJ_HANDLE * pHandle;
    K2TREE_NODE *               pTreeNode;
    K2STAT                      stat;
    K2OS_RPC_OBJ_CREATE         cret;
#if TRAP_EXCEPTIONS
    K2_EXCEPTION_TRAP           trap;
#endif
    K2OS_SIGNAL_TOKEN           tokGate;

    FUNC_ENTER;

    if (NULL != apRetObjId)
    {
        *apRetObjId = 0;
    }

    if ((NULL == apClassId) ||
        (K2MEM_VerifyZero(apClassId, sizeof(K2_GUID128))))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return NULL;
    }

    if (NULL == gpRpcServer)
    {
        // just return class not found
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_CLASS_NOT_FOUND);
        FUNC_EXIT;
        return NULL;
    }

    pObj = (RPC_OBJ *)K2OS_Heap_Alloc(sizeof(RPC_OBJ));
    if (NULL == pObj)
    {
        FUNC_EXIT;
        return NULL;
    }

    stat = K2STAT_ERROR_UNKNOWN;

    do {
        pHandle = (K2OSRPC_SERVER_OBJ_HANDLE *)K2OS_Heap_Alloc(sizeof(K2OSRPC_SERVER_OBJ_HANDLE));
        if (NULL == pHandle)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            break;
        }

        do {
            if (NULL != aTokRemoteDisconnect)
            {
                if (!K2OS_Token_Clone(aTokRemoteDisconnect, &tokGate))
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                    break;
                }
            }
            else
            {
                tokGate = NULL;
            }

            do {
                K2MEM_Zero(pObj, sizeof(RPC_OBJ));
                K2LIST_Init(&pObj->HandleList);
                K2LIST_Init(&pObj->IfInstList);
                pObj->mRefCount = 1;
                pObj->ServerObjPtrTreeNode.mUserVal = (UINT32)pObj;

                K2MEM_Zero(pHandle, sizeof(K2OSRPC_SERVER_OBJ_HANDLE));
                pHandle->Hdr.mIsServer = TRUE;
                pHandle->Hdr.mRefs = 1;
                pHandle->Hdr.GlobalHandleTreeNode.mUserVal = (UINT32)pHandle;
                pHandle->mpObj = pObj;
                K2LIST_AddAtTail(&pObj->HandleList, &pHandle->ObjHandleListLink);

                K2OS_CritSec_Enter(&gRpcGraphSec);

                pTreeNode = K2TREE_Find(&gpRpcServer->ClassByIdTree, (UINT_PTR)apClassId);
                if (NULL == pTreeNode)
                {
                    K2OSRPC_Debug("Class not found\n");
                    stat = K2STAT_ERROR_CLASS_NOT_FOUND;
                    pClass = NULL;
                }
                else
                {
                    pClass = K2_GET_CONTAINER(RPC_CLASS, pTreeNode, ServerClassByIdTreeNode);
                    K2_ASSERT(pClass->mIsRegistered);
                    K2ATOMIC_Inc(&pClass->mRefCount);
                }

                K2OS_CritSec_Leave(&gRpcGraphSec);

                if (NULL == pClass)
                {
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                    K2OS_Thread_SetLastStatus(stat);
                    break;
                }

                pObj->mpClass = pClass;
                pObj->ServerObjIdTreeNode.mUserVal = K2ATOMIC_AddExchange(&sgNextServerObjId, 1);

                K2MEM_Zero(&cret, sizeof(cret));
                cret.mClassRegisterContext = pClass->mUserContext;
                cret.mCreatorContext = aCreatorContext;
                cret.mCreatorProcessId = aCreatorProcessId;
                cret.mRemoteDisconnectedGateToken = tokGate;

#if TRAP_EXCEPTIONS
                stat = K2_EXTRAP(&trap, pClass->Def.Create((K2OS_RPC_OBJ)pObj, &cret, &pObj->mUserContext));
#else
                stat = pClass->Def.Create((K2OS_RPC_OBJ)pObj, &cret, &pObj->mUserContext);
#endif
                K2OS_CritSec_Enter(&gRpcGraphSec);

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OS_Thread_SetLastStatus(stat);

                    //
                    // release class reference
                    //
                    if (0 == K2ATOMIC_Dec(&pClass->mRefCount))
                    {
                        K2_ASSERT(!pClass->mIsRegistered);
                        K2TREE_Remove(&gpRpcServer->ClassByPtrTree, &pClass->ServerClassByPtrTreeNode);
                    }
                    else
                    {
                        pClass = NULL;
                    }

                    K2OS_CritSec_Leave(&gRpcGraphSec);

                    if (NULL != pClass)
                    {
                        K2OS_Xdl_Release(pClass->mXdlHandle[2]);
                        K2OS_Xdl_Release(pClass->mXdlHandle[1]);
                        K2OS_Xdl_Release(pClass->mXdlHandle[0]);
                        K2OS_Heap_Free(pClass);
                    }
                }
                else
                {
                    // object created ok so add it to the graph
                    K2TREE_Insert(&gpRpcServer->ObjByIdTree, pObj->ServerObjIdTreeNode.mUserVal, &pObj->ServerObjIdTreeNode);
                    K2TREE_Insert(&gpRpcServer->ObjByPtrTree, pObj->ServerObjPtrTreeNode.mUserVal, &pObj->ServerObjPtrTreeNode);
                    K2LIST_AddAtTail(&pClass->ObjList, &pObj->ClassObjListLink);
                    K2TREE_Insert(&gRpcHandleTree, pHandle->Hdr.GlobalHandleTreeNode.mUserVal, &pHandle->Hdr.GlobalHandleTreeNode);
                    K2OS_CritSec_Leave(&gRpcGraphSec);
                    if (NULL != apRetObjId)
                    {
                        *apRetObjId = pObj->ServerObjIdTreeNode.mUserVal;
                    }
                }
            } while (0);

            if (NULL != tokGate)
            {
                K2OS_Token_Destroy(tokGate);
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Heap_Free(pHandle);
            pHandle = NULL;
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Heap_Free(pObj);
        FUNC_EXIT;
        return NULL;
    }

    K2_ASSERT(NULL != pHandle);

    if (NULL != pHandle->mpObj->mpClass->Def.OnAttach)
    {
#if TRAP_EXCEPTIONS
        stat = K2_EXTRAP(&trap, pHandle->mpObj->mpClass->Def.OnAttach(
            (K2OS_RPC_OBJ)pHandle->mpObj,
            pHandle->mpObj->mUserContext,
            aCreatorProcessId,
            &pHandle->mUseContext
        ));
#else
        pHandle->mpObj->mpClass->Def.OnAttach(
            (K2OS_RPC_OBJ)pHandle->mpObj,
            pHandle->mpObj->mUserContext,
            aCreatorProcessId,
            &pHandle->mUseContext
        );
#endif
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSRPC_ReleaseInternal((K2OS_RPC_OBJ_HANDLE)pHandle, FALSE);
            pHandle = NULL;
        }
    }

    FUNC_EXIT;
    return (K2OS_RPC_OBJ_HANDLE)pHandle;
}

K2OSRPC_SERVER_OBJ_HANDLE *
K2OSRPC_Server_LocalAttachByObjId(
    UINT32  aObjectId
)
{
    K2OSRPC_SERVER_OBJ_HANDLE * pHandle;
    RPC_OBJ *                   pObj;
    K2TREE_NODE *               pTreeNode;

    FUNC_ENTER;

    if (NULL == gpRpcServer)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return NULL;
    }

    pHandle = (K2OSRPC_SERVER_OBJ_HANDLE *)K2OS_Heap_Alloc(sizeof(K2OSRPC_SERVER_OBJ_HANDLE));
    if (NULL == pHandle)
    {
        FUNC_EXIT;
        return NULL;
    }

    K2MEM_Zero(pHandle, sizeof(K2OSRPC_SERVER_OBJ_HANDLE));
    pHandle->Hdr.mRefs = 1;
    pHandle->Hdr.mIsServer = TRUE;
    pHandle->Hdr.GlobalHandleTreeNode.mUserVal = (UINT_PTR)pHandle;

    K2OS_CritSec_Enter(&gRpcGraphSec);

    pTreeNode = K2TREE_Find(&gpRpcServer->ObjByIdTree, aObjectId);
    if (NULL != pTreeNode)
    {
        pObj = K2_GET_CONTAINER(RPC_OBJ, pTreeNode, ServerObjIdTreeNode);
        pHandle->mpObj = pObj;
        K2ATOMIC_Inc(&pObj->mRefCount);
        K2LIST_AddAtTail(&pObj->HandleList, &pHandle->ObjHandleListLink);

        K2TREE_Insert(&gRpcHandleTree, pHandle->Hdr.GlobalHandleTreeNode.mUserVal, &pHandle->Hdr.GlobalHandleTreeNode);
    }

    K2OS_CritSec_Leave(&gRpcGraphSec);

    if (NULL == pTreeNode)
    {
        K2OS_Heap_Free(pHandle);
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return NULL;
    }

    FUNC_EXIT;
    return pHandle;
}

K2OSRPC_SERVER_OBJ_HANDLE *
K2OSRPC_Server_LocalAttachByIfInstId(
    K2OS_IFINST_ID  aLocalIfInstId,
    UINT32 *        apRetObjId
)
{
    K2OSRPC_SERVER_OBJ_HANDLE * pHandle;
    K2TREE_NODE *               pTreeNode;
    RPC_OBJ *                   pObj;
    RPC_IFINST *                pIfInst;

    FUNC_ENTER;

    if (NULL == gpRpcServer)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return NULL;
    }

    if (0 == aLocalIfInstId)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return NULL;
    }

    pHandle = (K2OSRPC_SERVER_OBJ_HANDLE *)K2OS_Heap_Alloc(sizeof(K2OSRPC_SERVER_OBJ_HANDLE));
    if (NULL == pHandle)
    {
        FUNC_EXIT;
        return NULL;
    }

    K2MEM_Zero(pHandle, sizeof(K2OSRPC_SERVER_OBJ_HANDLE));
    pHandle->Hdr.mRefs = 1;
    pHandle->Hdr.mIsServer = TRUE;
    pHandle->Hdr.GlobalHandleTreeNode.mUserVal = (UINT_PTR)pHandle;

    K2OS_CritSec_Enter(&gRpcGraphSec);

    pTreeNode = K2TREE_Find(&gpRpcServer->IfInstIdTree, aLocalIfInstId);
    if (NULL != pTreeNode)
    {
        pIfInst = K2_GET_CONTAINER(RPC_IFINST, pTreeNode, ServerIfInstIdTreeNode);
        pObj = pIfInst->mpObj;

        pHandle->mpObj = pObj;
        K2ATOMIC_Inc(&pObj->mRefCount);
        K2LIST_AddAtTail(&pObj->HandleList, &pHandle->ObjHandleListLink);

        K2TREE_Insert(&gRpcHandleTree, pHandle->Hdr.GlobalHandleTreeNode.mUserVal, &pHandle->Hdr.GlobalHandleTreeNode);
    }

    K2OS_CritSec_Leave(&gRpcGraphSec);

    if (NULL == pTreeNode)
    {
        K2OS_Heap_Free(pHandle);
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return NULL;
    }

    FUNC_EXIT;
    return pHandle;
}

K2STAT
K2OSRPC_Server_LocalCall(
    K2OSRPC_SERVER_OBJ_HANDLE * apHandle,
    K2OS_RPC_CALLARGS const *   apCallArgs,
    UINT32 *                    apRetActualOut
)
{
    RPC_OBJ *           pObj;
    K2STAT              stat;
    K2OS_RPC_OBJ_CALL   objCall;
#if TRAP_EXCEPTIONS
    K2_EXCEPTION_TRAP   trap;
#endif

    FUNC_ENTER;

    // not possible since handle existed
    K2_ASSERT(NULL != gpRpcServer);

    K2MEM_Copy(&objCall.Args, apCallArgs, sizeof(K2OS_RPC_CALLARGS));
    if (((objCall.Args.mInBufByteCount > 0) && (NULL == objCall.Args.mpInBuf)) ||
        ((objCall.Args.mOutBufByteCount > 0) && (NULL == objCall.Args.mpOutBuf)))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pObj = apHandle->mpObj;
    if (NULL != apHandle->mpConnToClient)
    {
        stat = K2OS_Token_Clone(apHandle->mpConnToClient->mDisconnectedGateToken, &objCall.mRemoteDisconnectedGateToken);
        if (K2STAT_IS_ERROR(stat))
        {
            return stat;
        }
    }
    else
    {
        objCall.mRemoteDisconnectedGateToken = NULL;
    }

    objCall.mObj = (K2OS_RPC_OBJ)pObj;
    objCall.mObjContext = pObj->mUserContext;
    objCall.mUseContext = apHandle->mUseContext;

#if TRAP_EXCEPTIONS
    stat = K2_EXTRAP(&trap, pObj->mpClass->Def.Call(&objCall, apRetActualOut));
#else
    stat = pObj->mpClass->Def.Call(&objCall, apRetActualOut);
#endif

    if (NULL != objCall.mRemoteDisconnectedGateToken)
    {
        K2OS_Token_Destroy(objCall.mRemoteDisconnectedGateToken);
    }
   
    FUNC_EXIT;
    return stat;
}

void
K2OSRPC_Server_PurgeHandle(
    K2OSRPC_SERVER_OBJ_HANDLE * apHandle,
    BOOL                        aUndoUse
)
{
    RPC_OBJ *           pObj;
    RPC_CONN *          pConn;
    RPC_CLASS *         pClass;
#if TRAP_EXCEPTIONS
    K2_EXCEPTION_TRAP   trap;
#endif
    K2STAT              stat;

    FUNC_ENTER;

    // not possible since handle existed
    K2_ASSERT(NULL != gpRpcServer);

    //
    // handle is already removed from handle tree
    //
    pObj = apHandle->mpObj;
    pConn = apHandle->mpConnToClient;

    if ((aUndoUse) &&
        (NULL != pObj->mpClass->Def.OnDetach))
    {
#if TRAP_EXCEPTIONS
        stat = K2_EXTRAP(&trap, pObj->mpClass->Def.OnDetach((K2OS_RPC_OBJ)pObj, pObj->mUserContext, apHandle->mUseContext));
#else
        stat = pObj->mpClass->Def.OnDetach((K2OS_RPC_OBJ)pObj, pObj->mUserContext, apHandle->mUseContext);
#endif
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSRPC_Debug("*** Detach from object generated failure code (%08X)\n", stat);
        }
    }

    K2OS_CritSec_Enter(&gRpcGraphSec);

    K2_ASSERT(0 == apHandle->Hdr.mRefs);

    apHandle->mpObj = NULL;

    apHandle->mpConnToClient = NULL;

    K2LIST_Remove(&pObj->HandleList, &apHandle->ObjHandleListLink);

    if (NULL != pConn)
    {
        K2_ASSERT(!apHandle->mOnConnHandleList);
//        K2LIST_Remove(&pConn->HandleList, &apHandle->ConnHandleListListLink);
//        apHandle->mOnConnHandleList = FALSE;
    }

    if (NULL != apHandle->mTokNotifyMailbox)
    {
        K2OS_Token_Destroy(apHandle->mTokNotifyMailbox);
        apHandle->mTokNotifyMailbox = NULL;
    }

    if (0 == K2ATOMIC_Dec(&pObj->mRefCount))
    {
        K2_ASSERT(0 == pObj->HandleList.mNodeCount);
        K2_ASSERT(0 == pObj->IfInstList.mNodeCount);

        K2TREE_Remove(&gpRpcServer->ObjByIdTree, &pObj->ServerObjIdTreeNode);
        K2TREE_Remove(&gpRpcServer->ObjByPtrTree, &pObj->ServerObjPtrTreeNode);

        pClass = pObj->mpClass;
        K2LIST_Remove(&pClass->ObjList, &pObj->ClassObjListLink);
        // do not deref class yet as we need its definition
    }
    else
    {
        pObj = NULL;
        pClass = NULL;
    }

    K2OS_CritSec_Leave(&gRpcGraphSec);

    if (NULL != pObj)
    {
        //
        // object refcount hit zero
        //
#if TRAP_EXCEPTIONS
        K2_EXTRAP(&trap, pObj->mpClass->Def.Delete((K2OS_RPC_OBJ)pObj, pObj->mUserContext));
#else
        pObj->mpClass->Def.Delete((K2OS_RPC_OBJ)pObj, pObj->mUserContext);
#endif
        // stat result ignored

        K2OS_CritSec_Enter(&gRpcGraphSec);

        if (0 == K2ATOMIC_Dec(&pClass->mRefCount))
        {
            //
            // class refcount hit zero.  can only happen if already unregistered
            // in which case it has already been removed from the class id tree
            //
            K2_ASSERT(!pClass->mIsRegistered);
            K2_ASSERT(0 == pClass->ObjList.mNodeCount);
            K2TREE_Remove(&gpRpcServer->ClassByPtrTree, &pClass->ServerClassByPtrTreeNode);
        }
        else
        {
            pClass = NULL;
        }

        K2OS_CritSec_Leave(&gRpcGraphSec);

        if (NULL != pClass)
        {
            K2OS_Xdl_Release(pClass->mXdlHandle[2]);
            K2OS_Xdl_Release(pClass->mXdlHandle[1]);
            K2OS_Xdl_Release(pClass->mXdlHandle[0]);
            K2OS_Heap_Free(pClass);
        }
    }

    K2OS_Heap_Free(apHandle);

    FUNC_EXIT;
}

K2OS_IFINST_ID  
K2OS_RpcServer_GetIfInstId(
    void
)
{
    FUNC_ENTER;
    FUNC_EXIT;
    return gRpcServerIfInstId;
}

UINT32
RpcServer_Thread(
    void *apArg
)
{
    RPC_SERVER *        pServer;
    K2STAT              stat;
    K2OS_WaitResult     waitResult;
    K2OS_MSG            msg;
    K2OS_WAITABLE_TOKEN tokWait[2];

    FUNC_ENTER;

    pServer = (RPC_SERVER *)apArg;
    stat = K2STAT_NO_ERROR;

    do {
        pServer->mTokMailbox = K2OS_Mailbox_Create();
        if (NULL == pServer->mTokMailbox)
        {
            stat = K2OS_Thread_GetLastStatus();
            break;
        }

        do {
            pServer->mTokShutdownGate = K2OS_Gate_Create(FALSE);
            if (NULL == pServer->mTokShutdownGate)
            {
                stat = K2OS_Thread_GetLastStatus();
                break;
            }

            do {
                pServer->mTokIfInst = K2OS_IfInst_Create(0, &gRpcServerIfInstId);
                if (NULL == pServer->mTokIfInst)
                {
                    waitResult = K2OS_Thread_GetLastStatus();
                    break;
                }

                if (!K2OS_IfInst_SetMailbox(pServer->mTokIfInst, pServer->mTokMailbox))
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                }
                else
                {
//                    K2OSRPC_Debug("RPC server in process %d has interface instance id %d\n", K2OS_Process_GetId(), gRpcServerIfInstId);

                    K2_ASSERT(0 != gRpcServerIfInstId);

                    if (!K2OS_IfInst_Publish(pServer->mTokIfInst, K2OS_IFACE_CLASSCODE_RPC, &gRpcServerClassId))
                    {
                        stat = K2OS_Thread_GetLastStatus();
                        K2_ASSERT(K2STAT_IS_ERROR(stat));
                    }
                    else
                    {
                        gpRpcServer = pServer;
                        K2_CpuWriteBarrier();
                        stat = K2STAT_NO_ERROR;
                    }
                }

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OS_Token_Destroy(pServer->mTokIfInst);
                    pServer->mTokIfInst = NULL;
                    gRpcServerIfInstId = 0;
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_Token_Destroy(pServer->mTokShutdownGate);
                pServer->mTokShutdownGate = NULL;
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Token_Destroy(pServer->mTokMailbox);
            pServer->mTokMailbox = NULL;
        }

    } while (0);

    pServer->mStartupStatus = stat;
    K2OS_Notify_Signal(pServer->mTokStartupNotify);
    if (K2STAT_IS_ERROR(stat))
    {
        FUNC_EXIT;
        return stat;
    }

    //
    // run server thread here
    //

//    K2OSRPC_Debug("RPC server in process %d started on thread %d\n", K2OS_Process_GetId(), gpRpcServer->mThreadId);

    tokWait[0] = gpRpcServer->mTokShutdownGate;
    tokWait[1] = gpRpcServer->mTokMailbox;
    do {
        if (!K2OS_Thread_WaitMany(&waitResult, 2, tokWait, FALSE, K2OS_TIMEOUT_INFINITE))
        {
            K2OSRPC_Debug("rpc server wait failed in process %d\n", K2OS_Process_GetId());
            break;
        }

        if ((waitResult == K2OS_Wait_Signalled_0) || (gpRpcServer->mShutdownStarted))
            break;

        if (K2OS_Mailbox_Recv(tokWait[1], &msg))
        {
            if (!K2OS_IpcEnd_ProcessMsg(&msg))
            {
                if ((msg.mMsgType == K2OS_SYSTEM_MSGTYPE_IPC) &&
                    (msg.mShort == K2OS_SYSTEM_MSG_IPC_SHORT_REQUEST))
                {
                    // msg.mPayload[0] is interface instance id
                    // msg.mPayload[1] is requestor process id
                    // msg.mPayload[2] is global request id

                    if (msg.mPayload[0] != gRpcServerIfInstId)
                    {
                        K2OSRPC_Debug("***RPC server in process %d recv IPC request for unknown ifid %d by process %d.\n", K2OS_Process_GetId(), msg.mPayload[0], msg.mPayload[1]);
                        K2OS_Ipc_RejectRequest(msg.mPayload[2], K2STAT_ERROR_NOT_FOUND);
                    }
                    else
                    {
//                        K2OSRPC_Debug("RPC server in process %d recv IPC request on ifid %d by process %d.\n", K2OS_Process_GetId(), msg.mPayload[0], msg.mPayload[1]);
                        K2OSRPC_Server_OnConnectRequest(msg.mPayload[1], msg.mPayload[2]);
                    }
                }
            }
        }

    } while (1);

//    K2OSRPC_Debug("RPC server in process %d told to stop\n", K2OS_Process_GetId());

    K2_ASSERT(0 == gpRpcServer->ClassByIdTree.mNodeCount);
    K2_ASSERT(0 == gpRpcServer->ClassByPtrTree.mNodeCount);
    K2_ASSERT(0 == gpRpcServer->ObjByIdTree.mNodeCount);
    K2_ASSERT(0 == gpRpcServer->ObjByPtrTree.mNodeCount);
    K2_ASSERT(0 == gpRpcServer->IfInstIdTree.mNodeCount);
    K2_ASSERT(0 == gpRpcServer->IfInstPtrTree.mNodeCount);
    K2_ASSERT(0 == gpRpcServer->ConnTree.mNodeCount);
    K2_ASSERT(0 == gpRpcServer->ActiveThreadList.mNodeCount);

    K2OS_Token_Destroy(gpRpcServer->mTokShutdownGate);
    gpRpcServer->mTokShutdownGate = NULL;

    K2OS_Token_Destroy(gpRpcServer->mTokIfInst);
    gpRpcServer->mTokIfInst = NULL;

    K2OS_Token_Destroy(gpRpcServer->mTokMailbox);
    gpRpcServer->mTokMailbox = NULL;

    //
    // shut down all idle threads
    //     K2LIST_ANCHOR       IdleThreadList;
    //
    K2_ASSERT(0);

    // 
    // purge all idle work items
    //

    gpRpcServer = NULL;
    K2_CpuWriteBarrier();

    FUNC_EXIT;
    return 0;
}

int
RpcClass_IdTreeCompare(
    UINT32          aKey,
    K2TREE_NODE *   apNode
)
{
    RPC_CLASS *     pClass;

    pClass = K2_GET_CONTAINER(RPC_CLASS, apNode, ServerClassByIdTreeNode);

    return K2MEM_Compare((void *)aKey, &pClass->Def.ClassId, sizeof(K2_GUID128));
}

K2STAT
RpcServer_Locked_Start(
    RPC_CLASS * apFirstClass
)
{
    RPC_SERVER *    pServer;
    K2STAT          stat;
    K2OS_WaitResult waitResult;
    BOOL            ok;
    char            threadName[K2OS_THREAD_NAME_BUFFER_CHARS];

    FUNC_ENTER;

    pServer = K2OS_Heap_Alloc(sizeof(RPC_SERVER));
    if (NULL == pServer)
    {
        FUNC_EXIT;
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    do {
        K2MEM_Zero(pServer, sizeof(RPC_SERVER));

        K2TREE_Init(&pServer->ClassByIdTree, RpcClass_IdTreeCompare);
        K2TREE_Init(&pServer->ClassByPtrTree, NULL);

        K2TREE_Init(&pServer->ObjByIdTree, NULL);
        K2TREE_Init(&pServer->ObjByPtrTree, NULL);

        K2TREE_Init(&pServer->IfInstIdTree, NULL);
        K2TREE_Init(&pServer->IfInstPtrTree, NULL);

        K2TREE_Init(&pServer->ConnTree, NULL);

        K2LIST_Init(&pServer->IdleThreadList);
        K2LIST_Init(&pServer->ActiveThreadList);

        K2LIST_Init(&pServer->IdleWorkItemList);

        pServer->mTokStartupNotify = K2OS_Notify_Create(FALSE);
        if (NULL == pServer->mTokStartupNotify)
        {
            stat = K2OS_Thread_GetLastStatus();
            break;
        }
        pServer->mStartupStatus = K2STAT_NO_ERROR;

        K2ASC_PrintfLen(threadName, K2OS_THREAD_NAME_BUFFER_CHARS - 1, "RpcServer Proc %d", K2OS_Process_GetId());
        threadName[K2OS_THREAD_NAME_BUFFER_CHARS - 1] = 0;

        pServer->mTokThread = K2OS_Thread_Create(threadName, RpcServer_Thread, pServer, NULL, &pServer->mThreadId);
        if (NULL != pServer->mTokThread)
        {
            ok = K2OS_Thread_WaitOne(&waitResult, pServer->mTokStartupNotify, K2OS_TIMEOUT_INFINITE);
            K2_ASSERT(ok);
            K2_ASSERT(K2OS_Wait_Signalled_0 == waitResult);
            K2_ASSERT(0 != pServer->mThreadId);

            stat = pServer->mStartupStatus;
            if (K2STAT_IS_ERROR(stat))
            {
                K2OSRPC_Debug("rpcserver startupstatus %08X\n", pServer->mStartupStatus);
                ok = K2OS_Thread_WaitOne(&waitResult, pServer->mTokThread, K2OS_TIMEOUT_INFINITE);
                K2_ASSERT(ok);
                K2_ASSERT(K2OS_Wait_Signalled_0 == waitResult);
                K2OS_Token_Destroy(pServer->mTokThread);
                pServer->mTokThread = NULL;
                pServer->mThreadId = 0;
            }
            else
            {
                K2_ASSERT(gpRpcServer == pServer);
            }
        }
        else
        {
            pServer->mThreadId = 0;
            stat = K2OS_Thread_GetLastStatus();
        }

        K2OS_Token_Destroy(pServer->mTokStartupNotify);
        pServer->mTokStartupNotify = NULL;

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Heap_Free(pServer);
        FUNC_EXIT;
        return stat;
    }

    K2TREE_Insert(&gpRpcServer->ClassByIdTree, (UINT_PTR)&apFirstClass->Def.ClassId, &apFirstClass->ServerClassByIdTreeNode);
    K2TREE_Insert(&gpRpcServer->ClassByPtrTree, apFirstClass->ServerClassByPtrTreeNode.mUserVal, &apFirstClass->ServerClassByPtrTreeNode);

    FUNC_EXIT;
    return K2STAT_NO_ERROR;
}

K2OS_RPC_CLASS  
K2OS_RpcServer_Register(
    K2OS_RPC_OBJ_CLASSDEF const *   apClassDef,
    UINT32                          aContext
)
{
    RPC_CLASS *     pClass;
    K2STAT          stat;
    K2TREE_NODE *   pTreeNode;

    FUNC_ENTER;

    if (NULL == apClassDef)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return NULL;
    }

    pClass = (RPC_CLASS *)K2OS_Heap_Alloc(sizeof(RPC_CLASS));
    if (NULL == pClass)
    {
        FUNC_EXIT;
        return NULL;
    }

    stat = K2STAT_NO_ERROR;

    do {
        K2MEM_Zero(pClass, sizeof(RPC_CLASS));

        K2MEM_Copy(&pClass->Def, apClassDef, sizeof(K2OS_RPC_OBJ_CLASSDEF));

        if ((K2MEM_VerifyZero(&pClass->Def.ClassId, sizeof(K2_GUID128))) ||
            (NULL == pClass->Def.Create) ||
            (NULL == pClass->Def.Call) ||
            (NULL == pClass->Def.Delete))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
            break;
        }

        pClass->mRefCount = 1;
        pClass->mIsRegistered = TRUE;
        pClass->mUserContext = aContext;
        K2LIST_Init(&pClass->ObjList);
        pClass->ServerClassByPtrTreeNode.mUserVal = (UINT_PTR)pClass;

        pClass->mXdlHandle[0] = K2OS_Xdl_AddRefContaining((UINT32)pClass->Def.Create);
        if (NULL == pClass->mXdlHandle[0])
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
            break;
        }

        do {
            pClass->mXdlHandle[1] = K2OS_Xdl_AddRefContaining((UINT32)pClass->Def.Call);
            if (NULL == pClass->mXdlHandle[1])
            {
                stat = K2STAT_ERROR_BAD_ARGUMENT;
                break;
            }

            do {
                pClass->mXdlHandle[2] = K2OS_Xdl_AddRefContaining((UINT32)pClass->Def.Delete);
                if (NULL == pClass->mXdlHandle[2])
                {
                    stat = K2STAT_ERROR_BAD_ARGUMENT;
                    break;
                }

                K2OS_CritSec_Enter(&gRpcGraphSec);

                if (NULL != gpRpcServer)
                {
                    pTreeNode = K2TREE_Find(&gpRpcServer->ClassByIdTree, (UINT_PTR)&pClass->Def.ClassId);
                    if (NULL == pTreeNode)
                    {
                        K2TREE_Insert(&gpRpcServer->ClassByIdTree, (UINT_PTR)&pClass->Def.ClassId, &pClass->ServerClassByIdTreeNode);
                        K2TREE_Insert(&gpRpcServer->ClassByPtrTree, pClass->ServerClassByPtrTreeNode.mUserVal, &pClass->ServerClassByPtrTreeNode);
                    }
                    else
                    {
                        stat = K2STAT_ERROR_ALREADY_EXISTS;
                    }
                }
                else
                {
                    stat = RpcServer_Locked_Start(pClass);
                }

                K2OS_CritSec_Leave(&gRpcGraphSec);

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OS_Xdl_Release(pClass->mXdlHandle[2]);
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_Xdl_Release(pClass->mXdlHandle[1]);
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Xdl_Release(pClass->mXdlHandle[0]);
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Heap_Free(pClass);
        K2OS_Thread_SetLastStatus(stat);
        FUNC_EXIT;
        return NULL;
    }

    return (K2OS_RPC_CLASS)pClass;
}

BOOL            
K2OS_RpcServer_Deregister(
    K2OS_RPC_CLASS aRegisteredClass
)
{
    RPC_CLASS *     pClass;
    BOOL            result;
    K2TREE_NODE *   pTreeNode;

    FUNC_ENTER;

    if (NULL == aRegisteredClass)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return FALSE;
    }

    K2OS_CritSec_Enter(&gRpcGraphSec);

    if (NULL == gpRpcServer)
    {
        K2OS_CritSec_Leave(&gRpcGraphSec);
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return FALSE;
    }

    result = FALSE;

    pTreeNode = K2TREE_Find(&gpRpcServer->ClassByPtrTree, (UINT_PTR)aRegisteredClass);
    if (NULL != pTreeNode)
    {
        pClass = K2_GET_CONTAINER(RPC_CLASS, pTreeNode, ServerClassByPtrTreeNode);
        if (pClass->mIsRegistered)
        {
            result = TRUE;
            pClass->mIsRegistered = FALSE;
            K2TREE_Remove(&gpRpcServer->ClassByIdTree, &pClass->ServerClassByIdTreeNode);

            if (0 == K2ATOMIC_Dec(&pClass->mRefCount))
            {
                K2_ASSERT(0 == pClass->ObjList.mNodeCount);
                K2TREE_Remove(&gpRpcServer->ClassByPtrTree, &pClass->ServerClassByPtrTreeNode);
            }
            else
            {
                pClass = NULL;
            }
        }
        else
        {
            pClass = NULL;
        }
    }
    else
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
    }

    K2OS_CritSec_Leave(&gRpcGraphSec);

    if (NULL != pClass)
    {
        K2OS_Xdl_Release(pClass->mXdlHandle[2]);
        K2OS_Xdl_Release(pClass->mXdlHandle[1]);
        K2OS_Xdl_Release(pClass->mXdlHandle[0]);
        K2OS_Heap_Free(pClass);
    }

    FUNC_EXIT;
    return result;
}

void
K2OSRPC_Server_Init(
    void
)
{
    FUNC_ENTER;

    gpRpcServer = NULL;
    sgNextServerObjId = 1;

    FUNC_EXIT;
}

