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
#include "crtuser.h"

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

    result = K2OS_Token_Destroy(pIpcEnd->mIpcEndToken);
    K2_ASSERT(result);

    K2_ASSERT(!pIpcEnd->mConnected);

    K2OS_CritSec_Done(&pIpcEnd->Sec);

    K2OS_Token_Destroy(pIpcEnd->mTokRecvVirtMap);
    virtAddr = (UINT32)pIpcEnd->mpRecvRing;

    K2_ASSERT(pIpcEnd->mTokSendVirtMap == NULL);
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
    K2OS_VIRTMAP_TOKEN  aTokRemoteVirtMap
)
{
    UINT32 work;
    UINT32 chunkBytes;
    UINT32 chunkCount;

    K2OS_CritSec_Enter(&apIpcEnd->Sec);

    apIpcEnd->mConnected = TRUE;
    K2ATOMIC_Inc(&apIpcEnd->mRefs);
    apIpcEnd->mTokSendVirtMap = aTokRemoteVirtMap;
    apIpcEnd->mpSendRing = (K2RING *)K2OS_VirtMap_GetInfo(aTokRemoteVirtMap, NULL, NULL);
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

    if (apIpcEnd->Callbacks.OnConnect != NULL)
    {
        apIpcEnd->Callbacks.OnConnect((K2OS_IPCEND)apIpcEnd, apIpcEnd->mpContext, apIpcEnd->mRemoteMaxMsgBytes);
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

        if (apIpcEnd->Callbacks.OnDisconnect != NULL)
        {
            apIpcEnd->Callbacks.OnDisconnect((K2OS_IPCEND)apIpcEnd, apIpcEnd->mpContext);
        }

        apIpcEnd->mRemoteChunkBytes = 0;
        apIpcEnd->mRemoteMaxMsgBytes = 0;
        apIpcEnd->mRemoteMaxMsgCount = 0;

        K2OS_CritSec_Enter(&apIpcEnd->Sec);

        K2OS_Token_Destroy(apIpcEnd->mTokSendVirtMap);
        apIpcEnd->mTokSendVirtMap = NULL;
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
    UINT32            aReasonCode
)
{
    if (apIpcEnd->Callbacks.OnRejected != NULL)
    {
        apIpcEnd->Callbacks.OnRejected((K2OS_IPCEND)apIpcEnd, apIpcEnd->mpContext, aReasonCode);
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
    availCount = K2RING_Reader_GetAvail(apIpcEnd->mpRecvRing, &offsetCount);
    K2_ASSERT(availCount >= recvCount);
    K2_ASSERT(offsetCount < apIpcEnd->mpRecvRing->mSize);

    if (apIpcEnd->Callbacks.OnRecv != NULL)
    {
        pData = ((UINT8 const *)(apIpcEnd->mpRecvRing)) + sizeof(K2RING) + (offsetCount * apIpcEnd->mLocalChunkBytes);
        apIpcEnd->Callbacks.OnRecv((K2OS_IPCEND)apIpcEnd, apIpcEnd->mpContext, pData, aRecvBytes);
    }

    K2RING_Reader_Consumed(apIpcEnd->mpRecvRing, recvCount);
}

void
K2OS_IpcEnd_Recv(
    void *              apKey,
    K2OS_MSG const *    apMsg
)
{
    CRT_USER_IPCEND * pIpcEnd;

    pIpcEnd = K2_GET_CONTAINER(CRT_USER_IPCEND, apKey, mfRecv);
    switch (apMsg->mShort)
    {
    case K2OS_SYSTEM_MSG_IPCEND_SHORT_CREATED:
        pIpcEnd->mIpcEndToken = (K2OS_TOKEN)apMsg->mPayload[1];
        break;
    case K2OS_SYSTEM_MSG_IPCEND_SHORT_CONNECTED:
        pIpcEnd->mRequesting = FALSE;
        pIpcEnd->mRequestIfInstId = 0;
        K2OS_IpcEnd_Connected(pIpcEnd, apMsg->mPayload[1], (K2OS_VIRTMAP_TOKEN)apMsg->mPayload[2]);
        break;
    case K2OS_SYSTEM_MSG_IPCEND_SHORT_RECV:
        K2OS_IpcEnd_User_Recv(pIpcEnd, apMsg->mPayload[1]);
        break;
    case K2OS_SYSTEM_MSG_IPCEND_SHORT_DISCONNECTED:
        K2OS_IpcEnd_User_Disconnected(pIpcEnd);
        break;
    case K2OS_SYSTEM_MSG_IPCEND_SHORT_REJECTED:
        K2OS_IpcEnd_User_Rejected(pIpcEnd, apMsg->mPayload[1]);
        break;
    default:
        K2_ASSERT(0);   // if this fires in debug we want to know about it
        break;
    };
}

K2OS_IPCEND 
K2OS_IpcEnd_Create(
    K2OS_MAILBOX_TOKEN  aTokMailbox,
    UINT32              aMaxMsgCount,
    UINT32              aMaxMsgBytes,
    void *              apContext,
    K2OS_IPCPROCESSMSG_CALLBACKS const *apCallbacks
)
{
    CRT_USER_IPCEND *       pIpcEnd;
    UINT32                  work;
    K2OS_PAGEARRAY_TOKEN    tokPageArray;
    K2OS_VIRTMAP_TOKEN      tokVirtMap;
    UINT32                  virtAddr;
    UINT32                  chunkCount;
    UINT32                  chunkBytes;

    if ((aTokMailbox == NULL) ||
        (aMaxMsgCount == 0) ||
        (aMaxMsgBytes == 0) ||
        (apCallbacks == NULL) ||
        (aMaxMsgCount > 0xFFFE) ||
        (aMaxMsgBytes > 0xFFFE))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return NULL;
    }

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

            tokVirtMap = K2OS_VirtMap_Create(tokPageArray, 0, work, virtAddr, K2OS_MapType_Data_ReadWrite);

            K2OS_Token_Destroy(tokPageArray);

            if (NULL == tokVirtMap)
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

                K2MEM_Copy(&pIpcEnd->Callbacks, apCallbacks, sizeof(K2OS_IPCPROCESSMSG_CALLBACKS));
                pIpcEnd->mfRecv = K2OS_IpcEnd_Recv;
                pIpcEnd->mpContext = apContext;

                pIpcEnd->mMaxMsgBytes = aMaxMsgBytes;
                pIpcEnd->mMaxMsgCount = aMaxMsgCount;
                pIpcEnd->mLocalChunkBytes = chunkBytes;

                pIpcEnd->mTokRecvVirtMap = tokVirtMap;
                pIpcEnd->mpRecvRing = (K2RING *)virtAddr;
                K2RING_Init(pIpcEnd->mpRecvRing, chunkCount);

                //
                // successful create may trigger a message before this thread
                // can return.  if that is the case the endpoint needs to be found
                // so we add it before we do the create and remove it if it fails
                //
                K2OS_CritSec_Enter(&sgSec);
                pIpcEnd->TreeNode.mUserVal = (UINT32)pIpcEnd;
                K2TREE_Insert(&sgTree, (UINT32)pIpcEnd, &pIpcEnd->TreeNode);
                K2OS_CritSec_Leave(&sgSec);

                pIpcEnd->mIpcEndToken = (K2OS_TOKEN)CrtKern_SysCall4(K2OS_SYSCALL_ID_IPCEND_CREATE, (UINT32)&pIpcEnd->mfRecv, (UINT32)aTokMailbox, (UINT32)tokVirtMap, (aMaxMsgCount & 0xFFFF) << 16 | (aMaxMsgBytes & 0xFFFF));
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
                K2OS_Token_Destroy(tokVirtMap);
            }

        } while (0);

        if (NULL == pIpcEnd)
        {
            K2OS_Virt_Release(virtAddr);
        }

    } while (0);

    return (K2OS_IPCEND)pIpcEnd;
}

BOOL
K2OS_IpcEnd_GetParam(
    K2OS_IPCEND aEndpoint,
    UINT32 *    apRetMaxMsgCount,
    UINT32 *    apRetMaxMsgBytes,
    void **     appRetContext
)
{
    CRT_USER_IPCEND *   pIpcEnd;

    pIpcEnd = CrtIpcEnd_FindAddRef(aEndpoint);

    if (NULL == pIpcEnd)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if (NULL != apRetMaxMsgCount)
    {
        *apRetMaxMsgCount = pIpcEnd->mMaxMsgCount;
    }

    if (NULL != apRetMaxMsgBytes)
    {
        *apRetMaxMsgBytes = pIpcEnd->mMaxMsgBytes;
    }

    if (NULL != appRetContext)
    {
        *appRetContext = pIpcEnd->mpContext;
    }

    CrtIpcEnd_Release(aEndpoint, FALSE);

    return TRUE;
}

BOOL        
K2OS_IpcEnd_SendRequest(
    K2OS_IPCEND     aEndpoint,
    K2OS_IFINST_ID  aIfInstId
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
    {
        return FALSE;
    }

    K2OS_CritSec_Enter(&pIpcEnd->Sec);

    if (pIpcEnd->mConnected)
    {
        result = FALSE;
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_CONNECTED);
    }
    else if ((pIpcEnd->mRequesting) && (pIpcEnd->mRequestIfInstId != aIfInstId))
    {
        result = FALSE;
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_API_ORDER);
    }
    else
    {
        pIpcEnd->mRequesting = TRUE;
        pIpcEnd->mRequestIfInstId = aIfInstId;
        result = (BOOL)CrtKern_SysCall2(K2OS_SYSCALL_ID_IPCEND_REQUEST, (UINT32)pIpcEnd->mIpcEndToken, (UINT32)aIfInstId);
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
K2OS_IpcEnd_AcceptRequest(
    K2OS_IPCEND aEndpoint,
    UINT32      aRequestId
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
        result = (BOOL)CrtKern_SysCall2(K2OS_SYSCALL_ID_IPCEND_ACCEPT, (UINT32)pIpcEnd->mIpcEndToken, aRequestId);
    }

    K2OS_CritSec_Leave(&pIpcEnd->Sec);

    CrtIpcEnd_Release(aEndpoint, FALSE);

    return result;
}

BOOL        
K2OS_IpcEnd_Send(
    K2OS_IPCEND aEndpoint,
    void const *apBuffer,
    UINT32      aByteCount
)
{
    K2MEM_BUFVECTOR vec;
    vec.mpBuffer = (UINT8 *)apBuffer;
    vec.mByteCount = aByteCount;
    return K2OS_IpcEnd_SendVector(aEndpoint, 1, &vec);
}

BOOL        
K2OS_IpcEnd_SendVector(
    K2OS_IPCEND             aEndpoint,
    UINT32                  aVectorCount,
    K2MEM_BUFVECTOR const * apVectors
)
{
    K2STAT              stat;
    CRT_USER_IPCEND *   pIpcEnd;
    UINT32              offset;
    UINT32              count;
    BOOL                result;
    UINT32              byteCount;
    UINT32              bytesToSend;

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

                stat = K2RING_Writer_Wrote(pIpcEnd->mpSendRing);
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
K2OS_IpcEnd_Disconnect(
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
K2OS_IpcEnd_Delete(
    K2OS_IPCEND aEndpoint
)
{
    //
    // try to disconnect at point of delete, even if it is not the last release
    //
    K2OS_IpcEnd_Disconnect(aEndpoint);

    return CrtIpcEnd_Release(aEndpoint, TRUE);
}

BOOL
K2OS_Ipc_RejectRequest(
    UINT32 aRequestId,
    UINT32 aReasonCode
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
K2OS_IpcEnd_ProcessMsg(
    K2OS_MSG const *apMsg
)
{
    if ((NULL == apMsg) || (apMsg->mType != K2OS_SYSTEM_MSGTYPE_IPCEND))
    {
        return FALSE;
    }

    (*((CRT_pf_SysMsgRecv *)apMsg->mPayload[0]))((void *)apMsg->mPayload[0], apMsg);

    return TRUE;
}
