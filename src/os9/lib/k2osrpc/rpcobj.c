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

void K2OSRPC_HandleLocked_AtClientServerHandleDestroy(K2OSRPC_CLIENT_OBJ_HANDLE *apHandle);

K2OS_RPC_OBJ_HANDLE 
K2OS_Rpc_CreateObj(
    K2OS_IFINST_ID      aRpcServerIfInstId, 
    K2_GUID128 const *  apClassId, 
    UINT32              aCreatorContext
) 
{ 
    K2OS_RPC_OBJ_HANDLE result;

    FUNC_ENTER;

    if ((aRpcServerIfInstId != 0) && (aRpcServerIfInstId != gRpcServerIfInstId))
    { 
        // not a local server
        result = K2OSRPC_Client_CreateObj(aRpcServerIfInstId, apClassId, aCreatorContext);
        FUNC_EXIT;
        return result;
    }

    // is a local server in this process
    result = K2OSRPC_Server_LocalCreateObj(apClassId, aCreatorContext, K2OS_Process_GetId(), NULL, NULL);
    FUNC_EXIT;
    return result;
}

K2OS_RPC_OBJ_HANDLE 
K2OS_Rpc_AttachByObjId(
    K2OS_IFINST_ID  aRpcServerIfInstId, 
    UINT32          aObjId
) 
{ 
    K2OS_RPC_OBJ_HANDLE         result;
    K2_EXCEPTION_TRAP           trap;
    K2OSRPC_SERVER_OBJ_HANDLE * pServerHandle;
    K2STAT                      stat;

    FUNC_ENTER;

    if ((aRpcServerIfInstId != 0) && (aRpcServerIfInstId != gRpcServerIfInstId))
    {
        // not a local server
        result = K2OSRPC_Client_AttachByObjId(aRpcServerIfInstId, aObjId);
        FUNC_EXIT;
        return result;
    }

    // is a local server in this process
    pServerHandle = K2OSRPC_Server_LocalAttachByObjId(aObjId);

    if ((NULL != pServerHandle) && 
        (NULL != pServerHandle->mpObj->mpClass->Def.OnAttach))
    {
#if TRAP_EXCEPTIONS
        stat = K2_EXTRAP(&trap, pServerHandle->mpObj->mpClass->Def.OnAttach(
            (K2OS_RPC_OBJ)pServerHandle->mpObj,
            pServerHandle->mpObj->mUserContext,
            K2OS_Process_GetId(),
            &pServerHandle->mUseContext
        ));
#else
        stat = pServerHandle->mpObj->mpClass->Def.OnAttach(
            (K2OS_RPC_OBJ)pServerHandle->mpObj,
            pServerHandle->mpObj->mUserContext,
            K2OS_Process_GetId(),
            &pServerHandle->mUseContext
        );
#endif
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSRPC_ReleaseInternal((K2OS_RPC_OBJ_HANDLE)pServerHandle, FALSE);
            K2OS_Thread_SetLastStatus(stat);
            pServerHandle = NULL;
        }
    }

    FUNC_EXIT;
    return (K2OS_RPC_OBJ_HANDLE)pServerHandle;
}

K2OS_RPC_OBJ_HANDLE 
K2OS_Rpc_AttachByIfInstId(
    K2OS_IFINST_ID  aIfInstId,
    UINT32 *        apRetObjId
)
{
    K2OS_RPC_OBJ_HANDLE         result;
    K2OS_IFINST_DETAIL          detail;
    K2OSRPC_SERVER_OBJ_HANDLE * pServerHandle;
    K2STAT                      stat;
    K2_EXCEPTION_TRAP           trap;

    FUNC_ENTER;

    if (!K2OS_IfInstId_GetDetail(aIfInstId, &detail))
    {
        return NULL;
    }

    if (detail.mOwningProcessId == K2OS_Process_GetId())
    {
        //
        // local server
        //
        result = NULL;
        pServerHandle = K2OSRPC_Server_LocalAttachByIfInstId(aIfInstId, apRetObjId);
        if (NULL != pServerHandle)  
        {
            if (NULL != pServerHandle->mpObj->mpClass->Def.OnAttach)
            {
#if TRAP_EXCEPTIONS
                stat = K2_EXTRAP(&trap, pServerHandle->mpObj->mpClass->Def.OnAttach(
                    (K2OS_RPC_OBJ)pServerHandle->mpObj,
                    pServerHandle->mpObj->mUserContext,
                    K2OS_Process_GetId(),
                    &pServerHandle->mUseContext
                ));
#else
                stat = pServerHandle->mpObj->mpClass->Def.OnAttach(
                    (K2OS_RPC_OBJ)pServerHandle->mpObj,
                    pServerHandle->mpObj->mUserContext,
                    K2OS_Process_GetId(),
                    &pServerHandle->mUseContext
                );
#endif
            }
            else
            {
                stat = K2STAT_NO_ERROR;
            }

            if (K2STAT_IS_ERROR(stat))
            {
                K2OSRPC_ReleaseInternal((K2OS_RPC_OBJ_HANDLE)pServerHandle, FALSE);
                K2OS_Thread_SetLastStatus(stat);
                pServerHandle = NULL;
            }
            else
            {
                result = (K2OS_RPC_OBJ_HANDLE)pServerHandle;
            }
        }
    }
    else
    {
        //
        // remote server
        //
        result = K2OSRPC_Client_AttachByIfInstId(aIfInstId, apRetObjId);
    }

    FUNC_EXIT;
    return result;
}

K2OSRPC_OBJ_HANDLE_HDR *
K2OSRPC_AcquireHandle(
    K2OS_RPC_OBJ_HANDLE aObjHandle
)
{
    K2OSRPC_OBJ_HANDLE_HDR *    pHdr;
    K2TREE_NODE *               pTreeNode;

    FUNC_ENTER;

    if (NULL == aObjHandle)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return NULL;
    }

    pHdr = NULL;

    K2OS_CritSec_Enter(&gRpcGraphSec);

    pTreeNode = K2TREE_Find(&gRpcHandleTree, (UINT32)aObjHandle);
    if (NULL != pTreeNode)
    {
        pHdr = K2_GET_CONTAINER(K2OSRPC_OBJ_HANDLE_HDR, pTreeNode, GlobalHandleTreeNode);
        K2ATOMIC_Inc(&pHdr->mRefs);
    }

    K2OS_CritSec_Leave(&gRpcGraphSec);

    FUNC_EXIT;
    return pHdr;
}

K2OS_IFINST_ID      
K2OS_Rpc_GetObjRpcServerIfInstId(
    K2OS_RPC_OBJ_HANDLE aObjHandle
)
{
    return K2OSRPC_GetObjIds(aObjHandle, NULL);
}

K2OS_RPC_OBJ_HANDLE 
K2OS_Rpc_Acquire(
    K2OS_RPC_OBJ_HANDLE aObjHandle
)
{
    K2OS_IFINST_ID  serverIfInstId;
    UINT32          objId;

    FUNC_ENTER;

    objId = 0;
    serverIfInstId = K2OSRPC_GetObjIds(aObjHandle, &objId);
    if ((0 == serverIfInstId) || (0 == objId))
    {
        FUNC_EXIT;
        return NULL;
    }

    return K2OS_Rpc_AttachByObjId(serverIfInstId, objId);
}

BOOL                
K2OS_Rpc_GetObjId(
    K2OS_RPC_OBJ_HANDLE aObjHandle, 
    UINT32 *            apRetObjId
) 
{ 
    K2OSRPC_OBJ_HANDLE_HDR * pHdr;

    FUNC_ENTER;

    if ((NULL == aObjHandle) || (NULL == apRetObjId))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return FALSE;
    }

    pHdr = K2OSRPC_AcquireHandle(aObjHandle);
    if (NULL == pHdr)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return FALSE;
    }

    if (pHdr->mIsServer)
    {
        *apRetObjId = K2OSRPC_Server_GetObjectId((K2OSRPC_SERVER_OBJ_HANDLE *)pHdr);
    }
    else
    {
        *apRetObjId = K2OSRPC_Client_GetObjectId((K2OSRPC_CLIENT_OBJ_HANDLE *)pHdr);
    }

    K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)pHdr);

    FUNC_EXIT;
    return TRUE;
}

BOOL                
K2OS_Rpc_GetObjClass(
    K2OS_RPC_OBJ_HANDLE aObjHandle,
    K2_GUID128 *        apRetClassId
)
{
    K2OSRPC_OBJ_HANDLE_HDR * pHdr;

    FUNC_ENTER;

    if ((NULL == aObjHandle) || (NULL == apRetClassId))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return FALSE;
    }

    pHdr = K2OSRPC_AcquireHandle(aObjHandle);
    if (NULL == pHdr)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return FALSE;
    }

    if (pHdr->mIsServer)
    {
        K2MEM_Copy(apRetClassId, &((K2OSRPC_SERVER_OBJ_HANDLE *)pHdr)->mpObj->mpClass->Def.ClassId, sizeof(K2_GUID128));
    }
    else
    {
        K2MEM_Copy(apRetClassId, &((K2OSRPC_CLIENT_OBJ_HANDLE *)pHdr)->ClassId, sizeof(K2_GUID128));
    }

    K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)pHdr);

    FUNC_EXIT;
    return TRUE;
}

BOOL                
K2OS_Rpc_SetNotifyTarget(
    K2OS_RPC_OBJ_HANDLE aObjHandle,
    K2OS_MAILBOX_TOKEN  aTokMailbox
)
{
    K2OSRPC_OBJ_HANDLE_HDR *    pHdr;
    K2OS_MAILBOX_TOKEN          tokMailbox;
    BOOL                        result;

    FUNC_ENTER;

    if (NULL == aObjHandle)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return FALSE;
    }

    pHdr = K2OSRPC_AcquireHandle(aObjHandle);
    if (NULL == pHdr)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return FALSE;
    }

    result = FALSE;

    do {
        if (NULL != aTokMailbox)
        {
            if (!K2OS_Token_Clone(aTokMailbox, &tokMailbox))
                break;

            K2_ASSERT(NULL != tokMailbox);
        }
        else
        {
            tokMailbox = NULL;
        }

        K2OS_CritSec_Enter(&gRpcGraphSec);

        if (pHdr->mIsServer)
        {
            tokMailbox = K2OSRPC_Server_SwapNotifyTarget((K2OSRPC_SERVER_OBJ_HANDLE *)pHdr, tokMailbox);
        }
        else
        {
            tokMailbox = K2OSRPC_Client_SwapNotifyTarget((K2OSRPC_CLIENT_OBJ_HANDLE *)pHdr, tokMailbox);
        }

        K2OS_CritSec_Leave(&gRpcGraphSec);

        if (NULL != tokMailbox)
        {
            K2OS_Token_Destroy(tokMailbox);
        }

        result = TRUE;

    } while (0);

    K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)pHdr);

    FUNC_EXIT;
    return result;
}

K2STAT              
K2OS_Rpc_Call(
    K2OS_RPC_OBJ_HANDLE         aObjHandle,
    K2OS_RPC_CALLARGS const *   apCallArgs,
    UINT32 *                    apRetActualOutBytes
)
{
    K2OSRPC_OBJ_HANDLE_HDR *    pHdr;
    UINT32                      fakeOutBytes;
    K2STAT                      result;

    FUNC_ENTER;

    if ((NULL == aObjHandle) ||
        (NULL == apCallArgs) ||
        ((apCallArgs->mInBufByteCount > 0) && (NULL == apCallArgs->mpInBuf)) ||
        ((apCallArgs->mOutBufByteCount > 0) && (NULL == apCallArgs->mpOutBuf)))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if (NULL == apRetActualOutBytes)
        apRetActualOutBytes = &fakeOutBytes;

    pHdr = K2OSRPC_AcquireHandle(aObjHandle);
    if (NULL == pHdr)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return K2STAT_ERROR_NOT_FOUND;
    }

    if (pHdr->mIsServer)
    {
        result = K2OSRPC_Server_LocalCall((K2OSRPC_SERVER_OBJ_HANDLE *)pHdr, apCallArgs, apRetActualOutBytes);
    }
    else
    {
        result = K2OSRPC_Client_Call((K2OSRPC_CLIENT_OBJ_HANDLE *)pHdr, apCallArgs, apRetActualOutBytes);
    }

    K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)pHdr);

    FUNC_EXIT;
    return result;
}

BOOL
K2OSRPC_ReleaseInternal(
    K2OS_RPC_OBJ_HANDLE     aObjHandle,
    BOOL                    aUndoUse
)
{
    K2OSRPC_OBJ_HANDLE_HDR *    pHdr;
    K2TREE_NODE *               pTreeNode;

    FUNC_ENTER;

    if (NULL == aObjHandle)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        FUNC_EXIT;
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    K2OS_CritSec_Enter(&gRpcGraphSec);

    pTreeNode = K2TREE_Find(&gRpcHandleTree, (UINT32)aObjHandle);
    if (NULL != pTreeNode)
    {
        pHdr = K2_GET_CONTAINER(K2OSRPC_OBJ_HANDLE_HDR, pTreeNode, GlobalHandleTreeNode);
        if (0 == K2ATOMIC_Dec(&pHdr->mRefs))
        {
            K2TREE_Remove(&gRpcHandleTree, &pHdr->GlobalHandleTreeNode);
            if (!pHdr->mIsServer)
            {
                K2OSRPC_HandleLocked_AtClientServerHandleDestroy((K2OSRPC_CLIENT_OBJ_HANDLE *)pHdr);
            }
        }
        else
        {
            pHdr = NULL;
        }
    }

    K2OS_CritSec_Leave(&gRpcGraphSec);

    if (NULL == pTreeNode)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        FUNC_EXIT;
        return K2STAT_ERROR_NOT_FOUND;
    }

    if (NULL == pHdr)
    {
        FUNC_EXIT;
        return TRUE;
    }

    if (pHdr->mIsServer)
    {
        K2OSRPC_Server_PurgeHandle((K2OSRPC_SERVER_OBJ_HANDLE *)pHdr, aUndoUse);
    }
    else
    {
        K2OSRPC_Client_PurgeHandle((K2OSRPC_CLIENT_OBJ_HANDLE *)pHdr);
    }

    FUNC_EXIT;
    return TRUE;
}

K2OS_IFINST_ID
K2OSRPC_GetObjIds(
    K2OS_RPC_OBJ_HANDLE aObjHandle,
    UINT32 *            apRetObjId
)
{
    K2OSRPC_OBJ_HANDLE_HDR *    pHandleHdr;
    K2OSRPC_CLIENT_OBJ_HANDLE * pClientHandle;
    K2OS_IFINST_ID              ifInstId;

    pHandleHdr = K2OSRPC_AcquireHandle(aObjHandle);
    if (NULL == pHandleHdr)
    {
        FUNC_EXIT;
        return 0;
    }

    if (pHandleHdr->mIsServer)
    {
        // local object
        ifInstId = gRpcServerIfInstId;
        if (NULL != apRetObjId)
        {
            *apRetObjId = ((K2OSRPC_SERVER_OBJ_HANDLE *)pHandleHdr)->mpObj->ServerObjIdTreeNode.mUserVal;
        }
    }
    else
    {
        // remote object
        pClientHandle = (K2OSRPC_CLIENT_OBJ_HANDLE *)pHandleHdr;
        if (NULL != apRetObjId)
        {
            *apRetObjId = pClientHandle->mServerObjId;
        }
        ifInstId = pClientHandle->mpConnToServer->ConnTreeNode.mUserVal;
    }

    K2OS_Rpc_Release((K2OS_RPC_OBJ_HANDLE)pHandleHdr);

    FUNC_EXIT;
    return ifInstId;
}

BOOL                
K2OS_Rpc_Release(
    K2OS_RPC_OBJ_HANDLE aObjHandle
) 
{ 
    return K2OSRPC_ReleaseInternal(aObjHandle, TRUE);
}

