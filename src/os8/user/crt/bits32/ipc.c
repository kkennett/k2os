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
#include "crt32.h"

static K2OS_CRITSEC     sgSec;
static K2TREE_ANCHOR    sgTree;

void
CrtIpcEnd_Init(
    void
)
{
    BOOL ok;

    ok = K2OS_CritSec_Init(&sgSec);
    K2_ASSERT(ok);
    if (!ok)
    {
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }
    K2TREE_Init(&sgTree, NULL);
}

CRT_USER_IPCEND *
CrtIpcEnd_FindAddRef(
    K2OS_IPCEND   aEndpoint
)
{
    UINT32              userEndpointAddr;
    K2TREE_NODE *       pTreeNode;
    CRT_USER_IPCEND *   pIpcEnd;

    userEndpointAddr = (UINT32)aEndpoint;
    if ((userEndpointAddr > (K2OS_UVA_TIMER_IOPAGE_BASE - sizeof(CRT_USER_IPCEND))) ||
        (userEndpointAddr < K2OS_UVA_LOW_BASE))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    pIpcEnd = (CRT_USER_IPCEND *)aEndpoint;

    K2OS_CritSec_Enter(&sgSec);

    pTreeNode = K2TREE_Find(&sgTree, userEndpointAddr);
    if (NULL != pTreeNode)
    {
        K2_ASSERT(pTreeNode = &pIpcEnd->TreeNode);
        if (!pIpcEnd->mUserDeleted)
        {
            K2ATOMIC_Inc(&pIpcEnd->mRefs);
        }
        else
        {
            pIpcEnd = NULL;
        }
    }
    else
    {
        pIpcEnd = NULL;
    }

    K2OS_CritSec_Leave(&sgSec);

    if (NULL == pIpcEnd)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
    }

    return pIpcEnd;
}

BOOL
CrtIpcEnd_Release(
    K2OS_IPCEND   aEndpoint,
    BOOL          aIsUserDelete
)
{
    UINT32              userEndpointAddr;
    K2TREE_NODE *       pTreeNode;
    CRT_USER_IPCEND *   pIpcEnd;
    BOOL                result;
    UINT32              virtAddr;

    userEndpointAddr = (UINT32)aEndpoint;
    if ((userEndpointAddr > (K2OS_UVA_TIMER_IOPAGE_BASE - sizeof(CRT_USER_IPCEND))) ||
        (userEndpointAddr < K2OS_UVA_LOW_BASE))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pIpcEnd = (CRT_USER_IPCEND *)aEndpoint;

    K2OS_CritSec_Enter(&sgSec);

    pTreeNode = K2TREE_Find(&sgTree, userEndpointAddr);
    if (NULL != pTreeNode)
    {
        if (aIsUserDelete)
        {
            if (pIpcEnd->mUserDeleted)
            {
                pIpcEnd = NULL;
            }
            else
            {
                pIpcEnd->mUserDeleted = TRUE;
            }
        }
        if (NULL != pIpcEnd)
        {
            if (0 != K2ATOMIC_Dec(&pIpcEnd->mRefs))
            {
                pTreeNode = NULL;
            }
            else
            {
                K2TREE_Remove(&sgTree, &pIpcEnd->TreeNode);
            }
        }
    }
    else
    {
        pIpcEnd = NULL;
    }

    K2OS_CritSec_Leave(&sgSec);

    if (NULL == pIpcEnd)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return FALSE;
    }

    if (NULL == pTreeNode)
        return TRUE;

    result = (BOOL)CrtKern_SysCall1(K2OS_SYSCALL_ID_IPCEND_CLOSE, (UINT32)pIpcEnd->mIpcEndToken);
    K2_ASSERT(result);

    K2_ASSERT(!pIpcEnd->mConnected);

    K2OS_CritSec_Done(&pIpcEnd->Sec);

    CrtMailboxRef_DecUse(pIpcEnd->mLocalMailbox, NULL, FALSE);
    pIpcEnd->mLocalMailbox = NULL;

    K2OS_Token_Destroy(pIpcEnd->mRecvMapToken);
    virtAddr = (UINT32)pIpcEnd->mpRecvRing;

    K2_ASSERT(pIpcEnd->mSendMapToken == NULL);
    K2_ASSERT(pIpcEnd->mpSendRing == NULL);

    K2MEM_Zero(pIpcEnd, sizeof(CRT_USER_IPCEND));  // clear sentinels

    K2OS_Heap_Free(pIpcEnd);

    K2OS_Virt_Release(virtAddr);

    return TRUE;
}

void
K2OS_IpcEnd_Connected(
    CRT_USER_IPCEND *   apIpcEnd,
    UINT32              aRemoteMsgConfig,
    K2OS_TOKEN          aTokRemoteBufferMap
)
{
    UINT_PTR work;
    UINT_PTR chunkBytes;
    UINT_PTR chunkCount;

    K2OS_CritSec_Enter(&apIpcEnd->Sec);

    apIpcEnd->mConnected = TRUE;
    K2ATOMIC_Inc(&apIpcEnd->mRefs);
    apIpcEnd->mSendMapToken = aTokRemoteBufferMap;
    apIpcEnd->mpSendRing = (K2RING *)K2OS_Map_GetInfo(aTokRemoteBufferMap, NULL, NULL);
    K2_ASSERT(NULL != apIpcEnd->mpSendRing);
    apIpcEnd->mRemoteMaxMsgCount = (aRemoteMsgConfig >> 16) & 0xFFFF;
    apIpcEnd->mRemoteMaxMsgBytes = aRemoteMsgConfig & 0xFFFF;

    work = (apIpcEnd->mRemoteMaxMsgCount + 1) * apIpcEnd->mRemoteMaxMsgBytes;
    chunkBytes = 1;
    chunkCount = work;
    while (chunkCount > 0x7FFF)
    {
        chunkBytes <<= 1;
        chunkCount = (chunkCount / 2) + 1;
    }
    apIpcEnd->mRemoteChunkBytes = chunkBytes;

    K2OS_CritSec_Leave(&apIpcEnd->Sec);

    if (apIpcEnd->FuncTab.OnConnect != NULL)
    {
        apIpcEnd->FuncTab.OnConnect(apIpcEnd, apIpcEnd->mpContext, apIpcEnd->mRemoteMaxMsgBytes);
    }
}

void
K2OS_IpcEnd_User_Disconnected(
    CRT_USER_IPCEND *   apIpcEnd
)
{
    BOOL didDisconnect;

    K2OS_CritSec_Enter(&apIpcEnd->Sec);

    didDisconnect = apIpcEnd->mConnected;
    if (didDisconnect)
    {
        apIpcEnd->mConnected = FALSE;

        K2OS_CritSec_Leave(&apIpcEnd->Sec);

        if (apIpcEnd->FuncTab.OnDisconnect != NULL)
        {
            apIpcEnd->FuncTab.OnDisconnect(apIpcEnd, apIpcEnd->mpContext);
        }

        K2OS_CritSec_Enter(&apIpcEnd->Sec);

        K2OS_Token_Destroy(apIpcEnd->mSendMapToken);
        apIpcEnd->mSendMapToken = NULL;
        apIpcEnd->mpSendRing = NULL;
    }

    K2OS_CritSec_Leave(&apIpcEnd->Sec);

    if (didDisconnect)
    {
        CrtIpcEnd_Release((K2OS_IPCEND)apIpcEnd, FALSE);
    }
}

void
K2OS_IpcEnd_User_Rejected(
    CRT_USER_IPCEND *   apIpcEnd,
    UINT_PTR            aReasonCode
)
{
    if (apIpcEnd->FuncTab.OnRejected != NULL)
    {
        apIpcEnd->FuncTab.OnRejected(apIpcEnd, apIpcEnd->mpContext, aReasonCode);
    }
}

void
K2OS_IpcEnd_User_Recv(
    CRT_USER_IPCEND *   apIpcEnd,
    UINT32              aRecvBytes
)
{
    UINT8 const *   pData;
    UINT32          recvCount;
    UINT32          availCount;
    UINT32          offsetCount;

    // convert bytes to ring buffer counts
    recvCount = (aRecvBytes + (apIpcEnd->mLocalChunkBytes - 1)) / apIpcEnd->mLocalChunkBytes;

    offsetCount = (UINT32)-1;
    availCount = K2RING_Reader_GetAvail(apIpcEnd->mpRecvRing, &offsetCount, FALSE);
    K2_ASSERT(availCount >= recvCount);
    K2_ASSERT(offsetCount < apIpcEnd->mpRecvRing->mSize);

    if (apIpcEnd->FuncTab.OnRecv != NULL)
    {
        pData = ((UINT8 const *)(apIpcEnd->mpRecvRing)) + sizeof(K2RING) + (offsetCount * apIpcEnd->mLocalChunkBytes);
        apIpcEnd->FuncTab.OnRecv(apIpcEnd, apIpcEnd->mpContext, pData, aRecvBytes);
    }

    K2RING_Reader_Consumed(apIpcEnd->mpRecvRing, recvCount);
}

void
K2OS_IpcEnd_Recv(
    void *  apKey,
    UINT32  aMsgType,
    UINT32  aArg0,
    UINT32  aArg1
)
{
    CRT_USER_IPCEND *   pIpcEnd;

    pIpcEnd = K2_GET_CONTAINER(CRT_USER_IPCEND, apKey, mfRecv);
    switch (aMsgType)
    {
    case K2OS_SYSTEM_MSG_IPC_CREATED:
        pIpcEnd->mIpcEndToken = (K2OS_TOKEN)aArg0;
        break;
    case K2OS_SYSTEM_MSG_IPC_CONNECTED:
        pIpcEnd->mRequesting = FALSE;
        pIpcEnd->mRequestIfInstId = 0;
        K2OS_IpcEnd_Connected(pIpcEnd, aArg0, (K2OS_TOKEN)aArg1);
        break;
    case K2OS_SYSTEM_MSG_IPC_RECV:
        K2OS_IpcEnd_User_Recv(pIpcEnd, aArg0);
        break;
    case K2OS_SYSTEM_MSG_IPC_DISCONNECTED:
        K2OS_IpcEnd_User_Disconnected(pIpcEnd);
        break;
    case K2OS_SYSTEM_MSG_IPC_REJECTED:
        K2OS_IpcEnd_User_Rejected(pIpcEnd, aArg0);
        break;
    default:
        K2_ASSERT(0);   // if this fires in debug we want to know about it
        break;
    };
}

K2OS_IPCEND
K2OS_Ipc_CreateEndpoint(
    K2OS_MAILBOX    aMailbox,
    UINT32          aMaxMsgCount,
    UINT32          aMaxMsgBytes,
    void *          apContext,
    K2OS_IPCEND_FUNCTAB const *apFuncTab
)
{
    K2OS_MAILBOX_OWNER *    pMailboxOwner;
    CRT_USER_IPCEND *       pIpcEnd;
    UINT32                  work;
    K2OS_PAGEARRAY_TOKEN    tokPageArray;
    K2OS_MAP_TOKEN          tokMap;
    UINT32                  virtAddr;
    UINT32                  chunkCount;
    UINT32                  chunkBytes;
    CRT_MBOXREF *           ipcMailbox;

    if ((aMailbox == NULL) ||
        (aMaxMsgCount == 0) ||
        (aMaxMsgBytes == 0) ||
        (apFuncTab == NULL) ||
        (aMaxMsgCount > 0xFFFE) ||
        (aMaxMsgBytes > 0xFFFE))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

    pMailboxOwner = CrtMailboxRef_FindAddUse(aMailbox);
    if (NULL == pMailboxOwner)
    {
        return NULL;
    }

    ipcMailbox = CrtMailbox_CreateRef((CRT_MBOXREF *)aMailbox);

    CrtMailboxRef_DecUse(aMailbox, pMailboxOwner, FALSE);

    if (NULL == ipcMailbox)
    {
        return NULL;
    }

    pMailboxOwner = ipcMailbox->mpMailboxOwner;

    do
    {
        work = (aMaxMsgCount + 1) * aMaxMsgBytes;
        chunkBytes = 1;
        chunkCount = work;
        while (chunkCount > 0x7FFF)
        {
            chunkBytes <<= 1;
            chunkCount = (chunkCount / 2) + 1;
        }
        work = (chunkCount * chunkBytes);

        pIpcEnd = NULL;
        work = (work + sizeof(K2RING) + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES;

        virtAddr = K2OS_Virt_Reserve(work);
        if (0 == virtAddr)
            break;

        do
        {
            tokPageArray = K2OS_PageArray_Create(work);
            if (NULL == tokPageArray)
                break;

            tokMap = K2OS_Map_Create(tokPageArray, 0, work, virtAddr, K2OS_MapType_Data_ReadWrite);

            K2OS_Token_Destroy(tokPageArray);

            if (NULL == tokMap)
                break;

            do
            {
                pIpcEnd = (CRT_USER_IPCEND *)K2OS_Heap_Alloc(sizeof(CRT_USER_IPCEND));
                if (NULL == pIpcEnd)
                    break;

                K2MEM_Zero(pIpcEnd, sizeof(CRT_USER_IPCEND));

                if (!K2OS_CritSec_Init(&pIpcEnd->Sec))
                {
                    K2OS_Heap_Free(pIpcEnd);
                    break;
                }

                pIpcEnd->mRefs = 1;

                K2MEM_Copy(&pIpcEnd->FuncTab, apFuncTab, sizeof(K2OS_IPCEND_FUNCTAB));
                pIpcEnd->mfRecv = K2OS_IpcEnd_Recv;
                pIpcEnd->mpContext = apContext;

                pIpcEnd->mLocalMailbox = ipcMailbox;
                pIpcEnd->mMaxMsgBytes = aMaxMsgBytes;
                pIpcEnd->mMaxMsgCount = aMaxMsgCount;
                pIpcEnd->mLocalChunkBytes = chunkBytes;

                pIpcEnd->mRecvMapToken = tokMap;
                pIpcEnd->mpRecvRing = (K2RING *)virtAddr;
                K2RING_Init(pIpcEnd->mpRecvRing, chunkCount);

                //
                // successful creatioj may trigger a message before this thread
                // can return.  if that is the case the endpoint needs to be found
                // so we add it before we do the create and remove it if it fails
                //
                K2OS_CritSec_Enter(&sgSec);
                pIpcEnd->TreeNode.mUserVal = (UINT32)pIpcEnd;
                K2TREE_Insert(&sgTree, (UINT32)pIpcEnd, &pIpcEnd->TreeNode);
                K2OS_CritSec_Leave(&sgSec);

                pIpcEnd->mIpcEndToken = (K2OS_TOKEN)CrtKern_SysCall4(K2OS_SYSCALL_ID_IPCEND_CREATE, (UINT32)&pIpcEnd->mfRecv, (UINT32)pMailboxOwner->mMailboxToken, (UINT32)tokMap, (aMaxMsgCount&0xFFFF)<<16 | (aMaxMsgBytes & 0xFFFF));

                if (NULL == pIpcEnd->mIpcEndToken)
                {
                    K2OS_CritSec_Enter(&sgSec);
                    K2TREE_Remove(&sgTree, &pIpcEnd->TreeNode);
                    K2OS_CritSec_Leave(&sgSec);

                    K2OS_CritSec_Done(&pIpcEnd->Sec);
                    K2MEM_Zero(pIpcEnd, sizeof(CRT_USER_IPCEND));
                    K2OS_Heap_Free(pIpcEnd);
                    pIpcEnd = NULL;
                }

            } while (0);

            if (NULL == pIpcEnd)
            {
                K2OS_Token_Destroy(tokMap);
            }

        } while (0);

        if (NULL == pIpcEnd)
        {
            K2OS_Virt_Release(virtAddr);
        }

    } while (0);

    if (NULL == pIpcEnd)
    {
        CrtMailboxRef_DecUse(ipcMailbox, pMailboxOwner, FALSE);
    }

    return pIpcEnd;
}

UINT32      
K2OS_Ipc_GetEndpointParam(
    K2OS_IPCEND     aEndpoint,
    UINT32 *        apRetMaxMsgCount,
    void **         appRetContext
)
{
    CRT_USER_IPCEND *   pIpcEnd;
    UINT32              result;

    pIpcEnd = CrtIpcEnd_FindAddRef(aEndpoint);

    if (NULL == pIpcEnd)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return 0;
    }

    if (NULL != apRetMaxMsgCount)
    {
        *apRetMaxMsgCount = pIpcEnd->mMaxMsgCount;
    }

    if (NULL != appRetContext)
    {
        *appRetContext = pIpcEnd->mpContext;
    }

    result = pIpcEnd->mMaxMsgBytes;

    CrtIpcEnd_Release(aEndpoint, FALSE);

    return result;
}

BOOL        
K2_CALLCONV_REGS
K2OS_Ipc_SendRequest(
    K2OS_IPCEND     aEndpoint,
    UINT_PTR        aGlobalInterfaceInstanceId
)
{
    CRT_USER_IPCEND *   pIpcEnd;
    BOOL                result;

    if (NULL == aEndpoint)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pIpcEnd = CrtIpcEnd_FindAddRef(aEndpoint);
    if (NULL == pIpcEnd)
        return FALSE;

    K2OS_CritSec_Enter(&pIpcEnd->Sec);

    if (pIpcEnd->mConnected)
    {
        result = FALSE;
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_CONNECTED);
    }
    else if ((pIpcEnd->mRequesting) && (pIpcEnd->mRequestIfInstId != aGlobalInterfaceInstanceId))
    {
        result = FALSE;
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_API_ORDER);
    }
    else
    {
        pIpcEnd->mRequesting = TRUE;
        pIpcEnd->mRequestIfInstId = aGlobalInterfaceInstanceId;
        result = (BOOL)CrtKern_SysCall2(K2OS_SYSCALL_ID_IPCEND_REQUEST, (UINT_PTR)pIpcEnd->mIpcEndToken, aGlobalInterfaceInstanceId);
        if (!result)
        {
            pIpcEnd->mRequestIfInstId = 0;
            pIpcEnd->mRequesting = FALSE;
        }
    }

    K2OS_CritSec_Leave(&pIpcEnd->Sec);

    CrtIpcEnd_Release(aEndpoint, FALSE);

    return result;
}

BOOL        
K2_CALLCONV_REGS
K2OS_Ipc_AcceptRequest(
    K2OS_IPCEND     aEndpoint, 
    UINT_PTR        aRequestId
)
{
    CRT_USER_IPCEND *   pIpcEnd;
    BOOL                result;

    if ((NULL == aEndpoint) ||
        (0 == aRequestId))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pIpcEnd = CrtIpcEnd_FindAddRef(aEndpoint);
    if (NULL == pIpcEnd)
        return FALSE;

    K2OS_CritSec_Enter(&pIpcEnd->Sec);

    if ((pIpcEnd->mConnected) || (pIpcEnd->mRequesting))
    {
        // cannot accept on an endpoint that is requesting
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_API_ORDER);
        result = FALSE;
    }
    else
    {
        result = (BOOL)CrtKern_SysCall2(K2OS_SYSCALL_ID_IPCEND_ACCEPT, (UINT_PTR)pIpcEnd->mIpcEndToken, aRequestId);
    }

    K2OS_CritSec_Leave(&pIpcEnd->Sec);

    CrtIpcEnd_Release(aEndpoint, FALSE);

    return result;
}

BOOL        
K2_CALLCONV_REGS 
K2OS_Ipc_RejectRequest(
    UINT_PTR aRequestId,
    UINT_PTR aReasonCode
)
{
    if (0 == aRequestId)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }
    return (BOOL)CrtKern_SysCall2(K2OS_SYSCALL_ID_IPCREQ_REJECT, aRequestId, aReasonCode);
}

BOOL                         
K2OS_Ipc_SendVector(
    K2OS_IPCEND             aEndpoint,
    UINT_PTR                aVectorCount,
    K2MEM_BUFVECTOR const * apVectors
)
{
    K2STAT              stat;
    CRT_USER_IPCEND *   pIpcEnd;
    UINT32              offset;
    UINT32              count;
    BOOL                result;
    UINT_PTR            byteCount;
    UINT_PTR            bytesToSend;

    if ((NULL == aEndpoint) ||
        (0 == aVectorCount) ||
        (NULL == apVectors))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    byteCount = K2MEM_CountVectorsBytes(aVectorCount, apVectors);
    if (0 == byteCount)
    {
        return TRUE;
    }

    pIpcEnd = CrtIpcEnd_FindAddRef(aEndpoint);
    if (NULL == pIpcEnd)
        return FALSE;

    result = FALSE;

    if (byteCount > pIpcEnd->mRemoteMaxMsgBytes)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_TOO_BIG);
    }
    else
    {
        count = (byteCount + (pIpcEnd->mRemoteChunkBytes - 1)) / pIpcEnd->mRemoteChunkBytes;

        K2OS_CritSec_Enter(&pIpcEnd->Sec);

        if (!pIpcEnd->mConnected)
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_CONNECTED);
        }
        else
        {
            do
            {
                if (!K2RING_Writer_GetOffset(pIpcEnd->mpSendRing, count, &offset))
                {
                    K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_RESOURCES);
                    break;
                }

                if (!((BOOL)CrtKern_SysCall1(K2OS_SYSCALL_ID_IPCEND_LOAD, (UINT32)pIpcEnd->mIpcEndToken)))
                {
                    break;
                }

                bytesToSend = byteCount;
                stat = K2MEM_Gather(aVectorCount, apVectors, ((UINT8 *)pIpcEnd->mpSendRing) + sizeof(K2RING) + (offset * pIpcEnd->mRemoteChunkBytes), &byteCount);
                K2_ASSERT(!K2STAT_IS_ERROR(stat));
                K2_ASSERT(byteCount == bytesToSend);

                stat = K2RING_Writer_Wrote(pIpcEnd->mpSendRing, offset, count);
                if (K2STAT_IS_ERROR(stat))
                {
                    CrtDbg_Printf("*** Writer_Wrote error 0x%08X\n", stat);
                    K2_ASSERT(0);       // want to know about this.
                    byteCount = 0;
                }

                result = (BOOL)CrtKern_SysCall2(K2OS_SYSCALL_ID_IPCEND_SEND, (UINT32)pIpcEnd->mIpcEndToken, byteCount);

            } while (0);
        }

        K2OS_CritSec_Leave(&pIpcEnd->Sec);
    }

    CrtIpcEnd_Release(aEndpoint, FALSE);

    return result;
}

BOOL
K2OS_Ipc_Send(
    K2OS_IPCEND     aEndpoint,
    void const *    apBuffer,
    UINT32          aByteCount
)
{
    K2MEM_BUFVECTOR vec;
    vec.mpBuffer = (UINT8 *)apBuffer;
    vec.mByteCount = aByteCount;
    return K2OS_Ipc_SendVector(aEndpoint, 1, &vec);
}

BOOL        
K2_CALLCONV_REGS
K2OS_Ipc_DisconnectEndpoint(
    K2OS_IPCEND aEndpoint
)
{
    CRT_USER_IPCEND *   pIpcEnd;
    BOOL                result;

    if (NULL == aEndpoint)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pIpcEnd = CrtIpcEnd_FindAddRef(aEndpoint);
    if (NULL == pIpcEnd)
        return FALSE;

    K2OS_CritSec_Enter(&pIpcEnd->Sec);

    if ((!pIpcEnd->mConnected) && (!pIpcEnd->mRequesting))
    {
        result = FALSE;
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_CONNECTED);
    }
    else
    {
        result = (BOOL)CrtKern_SysCall1(K2OS_SYSCALL_ID_IPCEND_MANUAL_DISCONNECT, (UINT32)pIpcEnd->mIpcEndToken);
        if (result)
            pIpcEnd->mRequesting = FALSE;
    }

    K2OS_CritSec_Leave(&pIpcEnd->Sec);

    CrtIpcEnd_Release(aEndpoint, FALSE);

    return result;
}

BOOL        
K2_CALLCONV_REGS
K2OS_Ipc_DeleteEndpoint(
    K2OS_IPCEND aEndpoint
)
{
    //
    // try to disconnect at point of delete, even if it is not the last release
    //
    K2OS_Ipc_DisconnectEndpoint(aEndpoint);

    return CrtIpcEnd_Release(aEndpoint, TRUE);
}

BOOL        
K2_CALLCONV_REGS
K2OS_Ipc_ProcessMsg(
    K2OS_MSG const *    apMsg
)
{
    UINT32  ctrl;

    if (NULL == apMsg)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    ctrl = apMsg->mControl;
    if (K2OS_MSG_CONTROL_SYSTEM_IPC != (ctrl & K2OS_MSG_CONTROL_SYSTEM_IPC))
    {
        return FALSE;
    }

    if (K2OS_MSG_CONTROL_SYSTEM_FUNC != (ctrl & K2OS_MSG_CONTROL_SYSTEM_FUNC))
    {
        if (ctrl != K2OS_SYSTEM_MSG_IPC_REQUEST)
        {
            // unknown non-func IPC message.
            K2_ASSERT(0);
        }

        // this message specfically not handled
        return FALSE;
    }

//    CrtDbg_Printf("IpcMsg(control %08X)\n", apMsg->mControl);
//    CrtDbg_Printf(".Payload[0] = %08X\n", apMsg->mPayload[0]);
//    CrtDbg_Printf(".Payload[1] = %08X\n", apMsg->mPayload[1]);
//    CrtDbg_Printf(".Payload[2] = %08X\n", apMsg->mPayload[2]);

    (*((CRT_pf_SysMsgRecv *)apMsg->mPayload[0]))((void *)apMsg->mPayload[0], ctrl, apMsg->mPayload[1], apMsg->mPayload[2]);

    return TRUE;
}
