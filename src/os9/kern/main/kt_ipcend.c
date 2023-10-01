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

#include "kern.h"

typedef struct _KERN_IPCEND KERN_IPCEND;

typedef void (*KernIpcEnd_pf_SysMsgRecv)(void *apKey, K2OS_MSG const *apMsg);

struct _KERN_IPCEND
{
    INT32 volatile                  mRefs;

    K2OS_IPCEND_TOKEN               mIpcEndToken;

    K2OSKERN_OBJREF                 RefRecvVirtMap;
    K2RING *                        mpRecvRing;

    BOOL                            mDeleted;

    BOOL                            mRequesting;        // StartConnect has been called 
    K2OS_IFINST_ID                  mRequestIfInstId;   // only valid while 'mRequesting' is true

    BOOL                            mConnected;         // true between recv connect msg and issue of ack disconnect
    K2OS_VIRTMAP_TOKEN              mSendVirtMapToken;  // valid between recv connect msg and issue of ack disconnect
    K2RING *                        mpSendRing;         // valid between recv connect msg and issue of ack disconnect
    UINT32                          mRemoteMaxMsgCount; // valid between recv connect msg and issue of ack disconnect
    UINT32                          mRemoteMaxMsgBytes; // valid between recv connect msg and issue of ack disconnect
    UINT32                          mRemoteChunkBytes;

    K2OS_IPCPROCESSMSG_CALLBACKS    Callbacks;

    void *                          mpContext;
    UINT32                          mMaxMsgCount;
    UINT32                          mMaxMsgBytes;
    UINT32                          mLocalChunkBytes;

    KernIpcEnd_pf_SysMsgRecv        mfRecv;

    K2OS_CRITSEC                    Sec;

    K2TREE_NODE                     TreeNode;
};

static K2OS_CRITSEC     sgSec;
static K2TREE_ANCHOR    sgTree;

void
KernIpcEnd_Threaded_Init(
    void
)
{
    BOOL ok;

    ok = K2OS_CritSec_Init(&sgSec);
    if (!ok)
    {
        K2OSKERN_Panic("*** CritSec init failed for ipcend sec\n");
    }

    K2TREE_Init(&sgTree, NULL);
}

KERN_IPCEND *
KernIpcEnd_Threaded_FindAddRef(
    K2OS_IPCEND aEndpoint
)
{
    K2TREE_NODE *       pTreeNode;
    KERN_IPCEND *       pKernIpcEnd;

    pKernIpcEnd = (KERN_IPCEND *)aEndpoint;

    K2OS_CritSec_Enter(&sgSec);

    pTreeNode = K2TREE_Find(&sgTree, (UINT32)aEndpoint);
    if (NULL != pTreeNode)
    {
        K2_ASSERT(pTreeNode = &pKernIpcEnd->TreeNode);
        if (!pKernIpcEnd->mDeleted)
        {
            K2ATOMIC_Inc(&pKernIpcEnd->mRefs);
        }
        else
        {
            pKernIpcEnd = NULL;
        }
    }
    else
    {
        pKernIpcEnd = NULL;
    }

    K2OS_CritSec_Leave(&sgSec);

    if (NULL == pKernIpcEnd)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
    }

    return pKernIpcEnd;
}

BOOL
KernIpcEnd_Threaded_Release(
    K2OS_IPCEND   aEndpoint,
    BOOL          aIsDelete
)
{
    K2TREE_NODE *       pTreeNode;
    KERN_IPCEND *       pKernIpcEnd;
    BOOL                result;
    UINT32              virtAddr;

    pKernIpcEnd = (KERN_IPCEND *)aEndpoint;

    K2OS_CritSec_Enter(&sgSec);

    pTreeNode = K2TREE_Find(&sgTree, (UINT32)aEndpoint);
    if (NULL != pTreeNode)
    {
        if (aIsDelete)
        {
            if (pKernIpcEnd->mDeleted)
            {
                pKernIpcEnd = NULL;
            }
            else
            {
                pKernIpcEnd->mDeleted = TRUE;
            }
        }
        if (NULL != pKernIpcEnd)
        {
            if (0 != K2ATOMIC_Dec(&pKernIpcEnd->mRefs))
            {
                pTreeNode = NULL;
            }
            else
            {
                K2TREE_Remove(&sgTree, &pKernIpcEnd->TreeNode);
            }
        }
    }
    else
    {
        pKernIpcEnd = NULL;
    }

    K2OS_CritSec_Leave(&sgSec);

    if (NULL == pKernIpcEnd)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return FALSE;
    }

    if (NULL == pTreeNode)
        return TRUE;

    result = K2OS_Token_Destroy(pKernIpcEnd->mIpcEndToken);
    K2_ASSERT(result);

    K2_ASSERT(!pKernIpcEnd->mConnected);

    K2OS_CritSec_Done(&pKernIpcEnd->Sec);

    KernObj_ReleaseRef(&pKernIpcEnd->RefRecvVirtMap);
    virtAddr = (UINT32)pKernIpcEnd->mpRecvRing;

    K2_ASSERT(pKernIpcEnd->mSendVirtMapToken == NULL);
    K2_ASSERT(pKernIpcEnd->mpSendRing == NULL);

    K2MEM_Zero(pKernIpcEnd, sizeof(KERN_IPCEND));  // clear sentinels

    K2OS_Heap_Free(pKernIpcEnd);

    K2OS_Virt_Release(virtAddr);

    return TRUE;
}

void
KernIpcEnd_Threaded_Connected(
    KERN_IPCEND *       apKernIpcEnd,
    UINT32              aRemoteMsgConfig,
    K2OS_VIRTMAP_TOKEN  aRemoteVirtMapToken
)
{
    UINT32 work;
    UINT32 chunkBytes;
    UINT32 chunkCount;

    K2OS_CritSec_Enter(&apKernIpcEnd->Sec);

    apKernIpcEnd->mConnected = TRUE;
    K2ATOMIC_Inc(&apKernIpcEnd->mRefs);
    apKernIpcEnd->mSendVirtMapToken = aRemoteVirtMapToken;
    apKernIpcEnd->mpSendRing = (K2RING *)K2OS_VirtMap_GetInfo(aRemoteVirtMapToken, NULL, NULL);
    K2_ASSERT(NULL != apKernIpcEnd->mpSendRing);
    apKernIpcEnd->mRemoteMaxMsgCount = (aRemoteMsgConfig >> 16) & 0xFFFF;
    apKernIpcEnd->mRemoteMaxMsgBytes = aRemoteMsgConfig & 0xFFFF;

    work = (apKernIpcEnd->mRemoteMaxMsgCount + 1) * apKernIpcEnd->mRemoteMaxMsgBytes;
    chunkBytes = 1;
    chunkCount = work;
    while (chunkCount > 0x7FFF)
    {
        chunkBytes <<= 1;
        chunkCount = (chunkCount / 2) + 1;
    }
    apKernIpcEnd->mRemoteChunkBytes = chunkBytes;

    K2OS_CritSec_Leave(&apKernIpcEnd->Sec);

    if (apKernIpcEnd->Callbacks.OnConnect != NULL)
    {
        apKernIpcEnd->Callbacks.OnConnect((K2OS_IPCEND)apKernIpcEnd, apKernIpcEnd->mpContext, apKernIpcEnd->mRemoteMaxMsgBytes);
    }
}

void
KernIpcEnd_Threaded_Disconnected(
    KERN_IPCEND *   apKernIpcEnd
)
{
    BOOL didDisconnect;

    K2OS_CritSec_Enter(&apKernIpcEnd->Sec);

    didDisconnect = apKernIpcEnd->mConnected;
    if (didDisconnect)
    {
        apKernIpcEnd->mConnected = FALSE;

        K2OS_CritSec_Leave(&apKernIpcEnd->Sec);

        if (apKernIpcEnd->Callbacks.OnDisconnect != NULL)
        {
            apKernIpcEnd->Callbacks.OnDisconnect((K2OS_IPCEND)apKernIpcEnd, apKernIpcEnd->mpContext);
        }

        apKernIpcEnd->mRemoteChunkBytes = 0;
        apKernIpcEnd->mRemoteMaxMsgBytes = 0;
        apKernIpcEnd->mRemoteMaxMsgCount = 0;

        K2OS_CritSec_Enter(&apKernIpcEnd->Sec);

        K2OS_Token_Destroy(apKernIpcEnd->mSendVirtMapToken);
        apKernIpcEnd->mSendVirtMapToken = NULL;
        apKernIpcEnd->mpSendRing = NULL;
    }

    K2OS_CritSec_Leave(&apKernIpcEnd->Sec);

    if (didDisconnect)
    {
        KernIpcEnd_Threaded_Release((K2OS_IPCEND)apKernIpcEnd, FALSE);
    }
}

void
KernIpcEnd_Threaded_Rejected(
    KERN_IPCEND *   apKernIpcEnd,
    UINT32          aReasonCode
)
{
    if (apKernIpcEnd->Callbacks.OnRejected != NULL)
    {
        apKernIpcEnd->Callbacks.OnRejected((K2OS_IPCEND)apKernIpcEnd, apKernIpcEnd->mpContext, aReasonCode);
    }
}

void
KernIpcEnd_Threaded_RecvData(
    KERN_IPCEND *   apKernIpcEnd,
    UINT32          aRecvBytes
)
{
    UINT8 const *   pData;
    UINT32          recvCount;
    UINT32          availCount;
    UINT32          offsetCount;

    // convert bytes to ring buffer counts
    recvCount = (aRecvBytes + (apKernIpcEnd->mLocalChunkBytes - 1)) / apKernIpcEnd->mLocalChunkBytes;

    offsetCount = (UINT32)-1;
    availCount = K2RING_Reader_GetAvail(apKernIpcEnd->mpRecvRing, &offsetCount, FALSE);
    K2_ASSERT(availCount >= recvCount);
    K2_ASSERT(offsetCount < apKernIpcEnd->mpRecvRing->mSize);

    if (apKernIpcEnd->Callbacks.OnRecv != NULL)
    {
        pData = ((UINT8 const *)(apKernIpcEnd->mpRecvRing)) + sizeof(K2RING) + (offsetCount * apKernIpcEnd->mLocalChunkBytes);
        apKernIpcEnd->Callbacks.OnRecv((K2OS_IPCEND)apKernIpcEnd, apKernIpcEnd->mpContext, pData, aRecvBytes);
    }

    K2RING_Reader_Consumed(apKernIpcEnd->mpRecvRing, recvCount);
}

void
KernIpcEnd_Threaded_Recv(
    void *              apKey,
    K2OS_MSG const *    apMsg
)
{
    KERN_IPCEND * pKernIpcEnd;

    pKernIpcEnd = K2_GET_CONTAINER(KERN_IPCEND, apKey, mfRecv);
    switch (apMsg->mShort)
    {
    case K2OS_SYSTEM_MSG_IPCEND_SHORT_CREATED:
        pKernIpcEnd->mIpcEndToken = (K2OS_TOKEN)apMsg->mPayload[1];
        break;
    case K2OS_SYSTEM_MSG_IPCEND_SHORT_CONNECTED:
        pKernIpcEnd->mRequesting = FALSE;
        pKernIpcEnd->mRequestIfInstId = 0;
        KernIpcEnd_Threaded_Connected(pKernIpcEnd, apMsg->mPayload[1], (K2OS_VIRTMAP_TOKEN)apMsg->mPayload[2]);
        break;
    case K2OS_SYSTEM_MSG_IPCEND_SHORT_RECV:
        KernIpcEnd_Threaded_RecvData(pKernIpcEnd, apMsg->mPayload[1]);
        break;
    case K2OS_SYSTEM_MSG_IPCEND_SHORT_DISCONNECTED:
        KernIpcEnd_Threaded_Disconnected(pKernIpcEnd);
        break;
    case K2OS_SYSTEM_MSG_IPCEND_SHORT_REJECTED:
        KernIpcEnd_Threaded_Rejected(pKernIpcEnd, apMsg->mPayload[1]);
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
    KERN_IPCEND *               pKernIpcEnd;
    UINT32                      work;
    UINT32                      virtAddr;
    UINT32                      chunkCount;
    UINT32                      chunkBytes;
    K2STAT                      stat;
    K2OSKERN_OBJREF             refMailboxOwner;
    K2OSKERN_CPUCORE volatile * pThisCore;
    BOOL                        disp;
    K2OSKERN_OBJ_THREAD *       pThisThread;
    K2OSKERN_OBJREF             refPageArray;
    K2OSKERN_OBJREF             refVirtMap;

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

    refMailboxOwner.AsAny = NULL;
    stat = KernToken_Translate(aTokMailbox, &refMailboxOwner);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (refMailboxOwner.AsAny->mObjType != KernObj_MailboxOwner)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
            KernObj_ReleaseRef(&refMailboxOwner);
        }
    }
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
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

        pKernIpcEnd = NULL;
        work = (work + sizeof(K2RING) + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES;

        virtAddr = K2OS_Virt_Reserve(work);
        if (0 == virtAddr)
            break;

        do
        {
            refPageArray.AsAny = NULL;
            stat = KernPageArray_CreateSparse(work, K2OS_MAPTYPE_USER_DATA, &refPageArray);
            if (K2STAT_IS_ERROR(stat))
                break;

            refVirtMap.AsAny = NULL;
            stat = KernVirtMap_Create(refPageArray.AsPageArray, 0, work, virtAddr, K2OS_MapType_Data_ReadWrite, &refVirtMap);
            KernObj_ReleaseRef(&refPageArray);

            if (K2STAT_IS_ERROR(stat))
                break;

            do
            {
                pKernIpcEnd = (KERN_IPCEND *)K2OS_Heap_Alloc(sizeof(KERN_IPCEND));
                if (NULL == pKernIpcEnd)
                    break;

                K2MEM_Zero(pKernIpcEnd, sizeof(KERN_IPCEND));

                if (!K2OS_CritSec_Init(&pKernIpcEnd->Sec))
                {
                    K2OS_Heap_Free(pKernIpcEnd);
                    pKernIpcEnd = NULL;
                    break;
                }

                pKernIpcEnd->mRefs = 1;

                K2MEM_Copy(&pKernIpcEnd->Callbacks, apCallbacks, sizeof(K2OS_IPCPROCESSMSG_CALLBACKS));
                pKernIpcEnd->mfRecv = KernIpcEnd_Threaded_Recv;
                pKernIpcEnd->mpContext = apContext;

                pKernIpcEnd->mMaxMsgBytes = aMaxMsgBytes;
                pKernIpcEnd->mMaxMsgCount = aMaxMsgCount;
                pKernIpcEnd->mLocalChunkBytes = chunkBytes;

                KernObj_CreateRef(&pKernIpcEnd->RefRecvVirtMap, refVirtMap.AsAny);
                pKernIpcEnd->mpRecvRing = (K2RING *)virtAddr;
                K2RING_Init(pKernIpcEnd->mpRecvRing, chunkCount);

                //
                // successful create may trigger a message before this thread
                // can return.  if that is the case the endpoint needs to be found
                // so we add it before we do the create and remove it if it fails
                //
                K2OS_CritSec_Enter(&sgSec);
                pKernIpcEnd->TreeNode.mUserVal = (UINT32)pKernIpcEnd;
                K2TREE_Insert(&sgTree, (UINT32)pKernIpcEnd, &pKernIpcEnd->TreeNode);
                K2OS_CritSec_Leave(&sgSec);

                disp = K2OSKERN_SetIntr(FALSE);
                K2_ASSERT(disp);
                pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
                pThisThread = pThisCore->mpActiveThread;
                K2_ASSERT(pThisThread->mIsKernelThread);
                K2OSKERN_SetIntr(TRUE);

                //
                // success at this will generate a create message
                // which contains the ipcend token
                //
                stat = KernIpcEnd_Create(
                    NULL,
                    pThisThread,
                    &pKernIpcEnd->mfRecv,
                    refMailboxOwner.AsMailboxOwner, 
                    refVirtMap.AsVirtMap, 
                    (aMaxMsgCount & 0xFFFF) << 16 | (aMaxMsgBytes & 0xFFFF),
                    &pKernIpcEnd->mIpcEndToken
                );

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OS_CritSec_Enter(&sgSec);
                    K2TREE_Remove(&sgTree, &pKernIpcEnd->TreeNode);
                    K2OS_CritSec_Leave(&sgSec);

                    K2OS_CritSec_Done(&pKernIpcEnd->Sec);
                    K2MEM_Zero(pKernIpcEnd, sizeof(KERN_IPCEND));
                    K2OS_Heap_Free(pKernIpcEnd);
                    pKernIpcEnd = NULL;
                }

            } while (0);

            KernObj_ReleaseRef(&refVirtMap);

        } while (0);

        if (NULL == pKernIpcEnd)
        {
            K2OS_Virt_Release(virtAddr);
        }

    } while (0);

    KernObj_ReleaseRef(&refMailboxOwner);

    return (K2OS_IPCEND)pKernIpcEnd;
}

BOOL        
K2OS_IpcEnd_GetParam(
    K2OS_IPCEND aEndpoint,
    UINT32 *    apRetMaxMsgCount,
    UINT32 *    apRetMaxMsgBytes,
    void **     appRetContext
)
{
    KERN_IPCEND *pKernIpcEnd;

    pKernIpcEnd = KernIpcEnd_Threaded_FindAddRef(aEndpoint);
    if (NULL == pKernIpcEnd)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return FALSE;
    }

    if (NULL != apRetMaxMsgBytes)
    {
        *apRetMaxMsgBytes = pKernIpcEnd->mMaxMsgBytes;
    }

    if (NULL != apRetMaxMsgCount)
    {
        *apRetMaxMsgCount = pKernIpcEnd->mMaxMsgCount;
    }

    if (NULL != appRetContext)
    {
        *appRetContext = pKernIpcEnd->mpContext;
    }

    KernIpcEnd_Threaded_Release(aEndpoint, FALSE);

    return TRUE;
}

BOOL        
K2OS_IpcEnd_SendRequest(
    K2OS_IPCEND     aEndpoint,
    K2OS_IFINST_ID  aIfInstId
)
{
    KERN_IPCEND *               pKernIpcEnd;
    K2OSKERN_OBJREF             refIpcEnd;
    K2STAT                      stat;
    BOOL                        disp;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD  *      pThisThread;

    pKernIpcEnd = KernIpcEnd_Threaded_FindAddRef(aEndpoint);
    if (NULL == pKernIpcEnd)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return FALSE;
    }

    refIpcEnd.AsAny = NULL;
    stat = KernToken_Translate(pKernIpcEnd->mIpcEndToken, &refIpcEnd);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (refIpcEnd.AsAny->mObjType != KernObj_IpcEnd)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            disp = K2OSKERN_SetIntr(FALSE);
            K2_ASSERT(disp);
            pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
            pThisThread = pThisCore->mpActiveThread;
            K2_ASSERT(pThisThread->mIsKernelThread);
            K2OSKERN_SetIntr(TRUE);

            stat = KernIpcEnd_SendRequest(
                NULL,
                pThisThread,
                refIpcEnd.AsIpcEnd,
                aIfInstId
            );
        }

        KernObj_ReleaseRef(&refIpcEnd);
    }

    KernIpcEnd_Threaded_Release(aEndpoint, FALSE);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL        
K2OS_IpcEnd_AcceptRequest(
    K2OS_IPCEND aEndpoint,
    UINT32      aRequestId
)
{
    KERN_IPCEND *               pKernIpcEnd;
    K2OSKERN_OBJREF             refIpcEnd;
    K2STAT                      stat;
    BOOL                        disp;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD  *      pThisThread;

    pKernIpcEnd = KernIpcEnd_Threaded_FindAddRef(aEndpoint);
    if (NULL == pKernIpcEnd)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return FALSE;
    }

    refIpcEnd.AsAny = NULL;
    stat = KernToken_Translate(pKernIpcEnd->mIpcEndToken, &refIpcEnd);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (refIpcEnd.AsAny->mObjType != KernObj_IpcEnd)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            disp = K2OSKERN_SetIntr(FALSE);
            K2_ASSERT(disp);
            pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
            pThisThread = pThisCore->mpActiveThread;
            K2_ASSERT(pThisThread->mIsKernelThread);
            K2OSKERN_SetIntr(TRUE);

            stat = KernIpcEnd_Accept(
                NULL,
                pThisThread,
                refIpcEnd.AsIpcEnd,
                aRequestId
            );
        }

        KernObj_ReleaseRef(&refIpcEnd);
    }

    KernIpcEnd_Threaded_Release(aEndpoint, FALSE);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL
K2OS_Ipc_RejectRequest(
    UINT32 aRequestId,
    UINT32 aReasonCode
)
{
    K2STAT                      stat;
    BOOL                        disp;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD  *      pThisThread;

    disp = K2OSKERN_SetIntr(FALSE);
    K2_ASSERT(disp);
    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
    pThisThread = pThisCore->mpActiveThread;
    K2_ASSERT(pThisThread->mIsKernelThread);
    K2OSKERN_SetIntr(TRUE);

    stat = KernIpcEnd_RejectRequest(NULL, pThisThread, aRequestId, aReasonCode);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
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
    K2STAT                      stat;
    KERN_IPCEND *               pKernIpcEnd;
    UINT32                      offset;
    UINT32                      count;
    UINT32                      byteCount;
    UINT32                      bytesToSend;
    K2OSKERN_OBJREF             refIpcEnd;
    BOOL                        disp;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD  *      pThisThread;

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

    pKernIpcEnd = KernIpcEnd_Threaded_FindAddRef(aEndpoint);
    if (NULL == pKernIpcEnd)
        return FALSE;

    refIpcEnd.AsAny = NULL;
    stat = KernToken_Translate(pKernIpcEnd->mIpcEndToken, &refIpcEnd);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (refIpcEnd.AsAny->mObjType != KernObj_IpcEnd)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            if (byteCount > pKernIpcEnd->mRemoteMaxMsgBytes)
            {
                K2OS_Thread_SetLastStatus(K2STAT_ERROR_TOO_BIG);
            }
            else
            {
                count = (byteCount + (pKernIpcEnd->mRemoteChunkBytes - 1)) / pKernIpcEnd->mRemoteChunkBytes;

                K2OS_CritSec_Enter(&pKernIpcEnd->Sec);

                if (!pKernIpcEnd->mConnected)
                {
                    K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_CONNECTED);
                }
                else
                {
                    do
                    {
                        if (!K2RING_Writer_GetOffset(pKernIpcEnd->mpSendRing, count, &offset))
                        {
                            K2OS_Thread_SetLastStatus(K2STAT_ERROR_OUT_OF_RESOURCES);
                            break;
                        }

                        stat = KernIpcEnd_Load(refIpcEnd.AsIpcEnd);
                        if (!K2STAT_IS_ERROR(stat))
                        {
                            bytesToSend = byteCount;
                            stat = K2MEM_Gather(aVectorCount, apVectors, ((UINT8 *)pKernIpcEnd->mpSendRing) + sizeof(K2RING) + (offset * pKernIpcEnd->mRemoteChunkBytes), &byteCount);
                            K2_ASSERT(!K2STAT_IS_ERROR(stat));
                            K2_ASSERT(byteCount == bytesToSend);

                            stat = K2RING_Writer_Wrote(pKernIpcEnd->mpSendRing, offset, count);
                            if (K2STAT_IS_ERROR(stat))
                            {
                                K2OSKERN_Debug("*** Writer_Wrote error 0x%08X\n", stat);
                                K2_ASSERT(0);       // want to know about this.
                                byteCount = 0;
                            }
                        }

                        disp = K2OSKERN_SetIntr(FALSE);
                        K2_ASSERT(disp);
                        pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
                        pThisThread = pThisCore->mpActiveThread;
                        K2_ASSERT(pThisThread->mIsKernelThread);
                        K2OSKERN_SetIntr(TRUE);

                        stat = KernIpcEnd_Sent(NULL, pThisThread, refIpcEnd.AsIpcEnd, byteCount);

                    } while (0);
                }

                K2OS_CritSec_Leave(&pKernIpcEnd->Sec);
            }
        }

        KernObj_ReleaseRef(&refIpcEnd);
    }

    KernIpcEnd_Threaded_Release(aEndpoint, FALSE);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL        
K2OS_IpcEnd_Disconnect(
    K2OS_IPCEND aEndpoint
)
{
    KERN_IPCEND *               pKernIpcEnd;
    K2OSKERN_OBJREF             refIpcEnd;
    K2STAT                      stat;
    BOOL                        disp;
    K2OSKERN_CPUCORE volatile * pThisCore;
    K2OSKERN_OBJ_THREAD  *      pThisThread;

    pKernIpcEnd = KernIpcEnd_Threaded_FindAddRef(aEndpoint);
    if (NULL == pKernIpcEnd)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        return FALSE;
    }

    refIpcEnd.AsAny = NULL;
    stat = KernToken_Translate(pKernIpcEnd->mIpcEndToken, &refIpcEnd);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (refIpcEnd.AsAny->mObjType != KernObj_IpcEnd)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            disp = K2OSKERN_SetIntr(FALSE);
            K2_ASSERT(disp);
            pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
            pThisThread = pThisCore->mpActiveThread;
            K2_ASSERT(pThisThread->mIsKernelThread);
            K2OSKERN_SetIntr(TRUE);

            stat = KernIpcEnd_ManualDisconnect(
                NULL, 
                pThisThread,
                refIpcEnd.AsIpcEnd
            );
        }

        KernObj_ReleaseRef(&refIpcEnd);
    }

    KernIpcEnd_Threaded_Release(aEndpoint, FALSE);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(stat);
        return FALSE;
    }

    return TRUE;
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

    return KernIpcEnd_Threaded_Release(aEndpoint, TRUE);
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

    (*((KernIpcEnd_pf_SysMsgRecv *)apMsg->mPayload[0]))((void *)apMsg->mPayload[0], apMsg);

    return TRUE;
}
