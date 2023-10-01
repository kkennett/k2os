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

void
KernIpcEnd_ConnLocked_DisconnectAck(
    K2OSKERN_OBJ_IPCEND * apIpcEnd
)
{
    // may be happening inside or outside of monitor
    if (apIpcEnd->Connection.Locked.Connected.mTookPartnerReserve)
    {
        K2OSKERN_Debug("Undo reserve for load (disconnected while loaded)\n");
        KernMailbox_UndoReserve(apIpcEnd->Connection.Locked.Connected.PartnerIpcEndRef.AsIpcEnd->MailboxOwnerRef.AsMailboxOwner->RefMailbox.AsMailbox, 1);
        apIpcEnd->Connection.Locked.Connected.mTookPartnerReserve = FALSE;
    }
    KernObj_ReleaseRef(&apIpcEnd->Connection.Locked.Connected.WriteMapRef);
    KernObj_ReleaseRef(&apIpcEnd->Connection.Locked.Connected.PartnerIpcEndRef);
    // this is the only place where this happens (set to disconnected)
    apIpcEnd->Connection.Locked.mState = KernIpcEndState_Disconnected;
}

void
KernIpcEnd_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_IPCEND *       apIpcEnd
)
{
    BOOL                    disp;
    K2OSKERN_IPC_REQUEST *  pReq;
    K2OSKERN_OBJ_PROCESS *  pProc;

    K2_ASSERT(0 != (apIpcEnd->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO));

    disp = K2OSKERN_SeqLock(&apIpcEnd->Connection.SeqLock);

    K2_ASSERT(apIpcEnd->Connection.Locked.mState != KernIpcEndState_Connected);

    if (apIpcEnd->Connection.Locked.mState == KernIpcEndState_WaitDisAck)
    {
        KernIpcEnd_ConnLocked_DisconnectAck(apIpcEnd);
    }

    pReq = apIpcEnd->Connection.Locked.mpOpenRequest;

    if (NULL != pReq)
    {
        K2OSKERN_SeqLock(&gData.Iface.SeqLock); // ordering with connection seqlock
        K2TREE_Remove(&gData.Iface.RequestTree, &pReq->TreeNode);
        pReq->mpRequestor = NULL;
        apIpcEnd->Connection.Locked.mpOpenRequest = NULL;
        K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, FALSE);
    }

    K2_ASSERT(NULL == apIpcEnd->Connection.Locked.Connected.WriteMapRef.AsAny);
    K2_ASSERT(NULL == apIpcEnd->Connection.Locked.Connected.PartnerIpcEndRef.AsAny);

    K2OSKERN_SeqUnlock(&apIpcEnd->Connection.SeqLock, disp);

    if (NULL != pReq)
    {
        K2MEM_Zero(pReq, sizeof(K2OSKERN_IPC_REQUEST));
        KernHeap_Free(pReq);
    }

    pProc = apIpcEnd->MailboxOwnerRef.AsMailboxOwner->RefProc.AsProc;
    if (NULL != pProc)
    {
        disp = K2OSKERN_SeqLock(&pProc->IpcEnd.SeqLock);
        K2LIST_Remove(&pProc->IpcEnd.Locked.List, &apIpcEnd->OwnerIpcEndListLocked.ListLink);
        K2OSKERN_SeqUnlock(&pProc->IpcEnd.SeqLock, disp);
    }
    else
    {
        disp = K2OSKERN_SeqLock(&gData.IpcEnd.SeqLock);
        K2LIST_Remove(&gData.IpcEnd.KernList, &apIpcEnd->OwnerIpcEndListLocked.ListLink);
        K2OSKERN_SeqUnlock(&gData.IpcEnd.SeqLock, disp);
    }

    KernObj_ReleaseRef(&apIpcEnd->VirtMapRef);

    KernObj_ReleaseRef(&apIpcEnd->MailboxOwnerRef);

    K2MEM_Zero(apIpcEnd, sizeof(K2OSKERN_OBJ_IPCEND));

    KernHeap_Free(apIpcEnd);
}

K2STAT 
KernIpcEnd_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    void *                      apKey,
    K2OSKERN_OBJ_MAILBOXOWNER * apMailboxOwner,
    K2OSKERN_OBJ_VIRTMAP *      apVirtMap,
    UINT32                      aMsgConfig,
    K2OS_IPCEND_TOKEN *         apRetToken
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_IPCEND *   pIpcEnd;
    BOOL                    disp;
    K2OS_MSG                createMsg;
    BOOL                    doSignal;
    K2OSKERN_SCHED_ITEM *   pSchedItem;
    K2OSKERN_OBJ_PROCESS *  pProc;

    pProc = apMailboxOwner->RefProc.AsProc;

    // sanity
    if (apCurThread->mIsKernelThread)
    {
        K2_ASSERT(NULL == pProc);
        K2_ASSERT(apVirtMap->ProcRef.AsProc == NULL);
        K2_ASSERT(apMailboxOwner->RefProc.AsProc == NULL);
    }
    else
    {
        K2_ASSERT(apCurThread->User.ProcRef.AsProc == pProc);
        K2_ASSERT(apVirtMap->ProcRef.AsProc == pProc);
        K2_ASSERT(apMailboxOwner->RefProc.AsProc == pProc);
    }

    if ((0 == ((aMsgConfig >> 16) & 0xFFFF)) ||
        (0 == (aMsgConfig & 0xFFFF)))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pIpcEnd = (K2OSKERN_OBJ_IPCEND *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_IPCEND));
    if (NULL == pIpcEnd)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    do
    {
        K2MEM_Zero(pIpcEnd, sizeof(K2OSKERN_OBJ_IPCEND));

        // reserved single-use mailbox messages - create, connect, disconnect, delete = 4
        if (!KernMailbox_Reserve(apMailboxOwner->RefMailbox.AsMailbox, 4))
        {
            stat = K2STAT_ERROR_OUT_OF_RESOURCES;
            break;
        }

        pIpcEnd->Hdr.mObjType = KernObj_IpcEnd;
        K2LIST_Init(&pIpcEnd->Hdr.RefObjList);

        pIpcEnd->mpUserKey = apKey;

        KernObj_CreateRef(&pIpcEnd->MailboxOwnerRef, &apMailboxOwner->Hdr);
        KernObj_CreateRef(&pIpcEnd->VirtMapRef, &apVirtMap->Hdr);

        pIpcEnd->mMaxMsgBytes = (aMsgConfig & 0xFFFF);
        pIpcEnd->mMaxMsgCount = (aMsgConfig >> 16) & 0xFFFF;

        K2OSKERN_SeqInit(&pIpcEnd->Connection.SeqLock);
        pIpcEnd->Connection.Locked.mState = KernIpcEndState_Disconnected;

        //
        // the whole reason this message exists is because
        // the mailbox may receive the message and process it on
        // a different thread than the endpoint-creating thread, before
        // the endpoint-creating thread has a chance to run and see the 
        // token corresponding to the ipc endpoint
        // 
        // in this case the endpoint needs to know its token in case
        // it wants to do anything, so the message contains the token
        // for the endpoint.  
        // 
        createMsg.mType = K2OS_SYSTEM_MSGTYPE_IPCEND;
        createMsg.mShort = K2OS_SYSTEM_MSG_IPCEND_SHORT_CREATED;
        createMsg.mPayload[0] = (UINT32)apKey;

        if (NULL != pProc)
        {
            K2_ASSERT(apCurThread->User.mIsInSysCall);
            stat = KernProc_TokenCreate(pProc, &pIpcEnd->Hdr, apRetToken);
            if (!K2STAT_IS_ERROR(stat))
            {
                disp = K2OSKERN_SeqLock(&pProc->IpcEnd.SeqLock);
                K2LIST_AddAtTail(&pProc->IpcEnd.Locked.List, &pIpcEnd->OwnerIpcEndListLocked.ListLink);
                K2OSKERN_SeqUnlock(&pProc->IpcEnd.SeqLock, disp);
            }
        }
        else
        {
            stat = KernToken_Create(&pIpcEnd->Hdr, apRetToken);
            if (!K2STAT_IS_ERROR(stat))
            {
                disp = K2OSKERN_SeqLock(&gData.IpcEnd.SeqLock);
                K2LIST_AddAtTail(&gData.IpcEnd.KernList, &pIpcEnd->OwnerIpcEndListLocked.ListLink);
                K2OSKERN_SeqUnlock(&gData.IpcEnd.SeqLock, disp);
            }
        }

        if (!K2STAT_IS_ERROR(stat))
        {
            createMsg.mPayload[1] = (UINT32)(*apRetToken);

            doSignal = FALSE;
            KernMailbox_Deliver(
                apMailboxOwner->RefMailbox.AsMailbox,
                &createMsg,
                FALSE,  // no alloc since we are sending from a reserve
                NULL,   // creation message is not something we notify of
                &doSignal
            );

            if (doSignal)
            {
                if (NULL != pProc)
                {
                    // in system call from user thread.  go into scheduler
                    K2_ASSERT(NULL != apThisCore);
                    pSchedItem = &apCurThread->SchedItem;
                    pSchedItem->mType = KernSchedItem_Thread_SysCall;
                    KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
                    KernObj_CreateRef(&pSchedItem->ObjRef, apMailboxOwner->RefMailbox.AsAny);
                    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
                    KernSched_QueueItem(pSchedItem);
                }
                else
                {
                    // in kernel thread. call scheduler right here
                    K2_ASSERT(NULL == apThisCore);

                    disp = K2OSKERN_SetIntr(FALSE);
                    K2_ASSERT(disp);

                    apThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
                    K2_ASSERT(apCurThread == apThisCore->mpActiveThread);

                    pSchedItem = &apCurThread->SchedItem;

                    pSchedItem->mType = KernSchedItem_KernThread_MailboxSentFirst;
                    KernObj_CreateRef(&pSchedItem->ObjRef, apMailboxOwner->RefMailbox.AsAny);

                    KernThread_CallScheduler(apThisCore);

                    // interrupts will be back on again here
                    KernObj_ReleaseRef(&pSchedItem->ObjRef);
                }
            }
        }
        else
        {
            KernObj_ReleaseRef(&pIpcEnd->VirtMapRef);
            KernMailbox_UndoReserve(apMailboxOwner->RefMailbox.AsMailbox, 4);
            KernObj_ReleaseRef(&pIpcEnd->MailboxOwnerRef);
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        KernHeap_Free(pIpcEnd);
    }

    return stat;
}

K2STAT
KernIpcEnd_Accept(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    K2OSKERN_OBJ_IPCEND *       apIpcEnd,
    UINT32                      aRequestId
)
{
    K2STAT                  stat;
    BOOL                    disp;
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_IPC_REQUEST *  pReq;
    K2OSKERN_OBJREF         ipcRemoteEndRef;
    K2OSKERN_OBJ_PROCESS *  pLocalProc;
    K2OSKERN_OBJ_PROCESS *  pRemoteProc;
    K2OSKERN_OBJ_VIRTMAP *  pLocalVirtMap;
    K2OSKERN_OBJ_VIRTMAP *  pRemoteVirtMap;
    UINT32                  remoteVirtAddrOfLocalBuffer;
    UINT32                  localVirtAddrOfRemoteBuffer;
    K2OSKERN_OBJREF         refLocalMapOfRemoteBuffer;
    K2OSKERN_OBJREF         refRemoteMapOfLocalBuffer;
    K2OSKERN_PROCHEAP_NODE *pUserVirtHeapNode;
    K2OSKERN_SCHED_ITEM *   pSchedItem;
    K2OS_TOKEN              tokLocalForRemote;
    K2OS_TOKEN              tokRemoteForLocal;

    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

    pTreeNode = K2TREE_Find(&gData.Iface.RequestTree, aRequestId);
    if (NULL == pTreeNode)
    {
        stat = K2STAT_ERROR_NOT_FOUND;
    }
    else
    {
        stat = K2STAT_NO_ERROR;
        pReq = K2_GET_CONTAINER(K2OSKERN_IPC_REQUEST, pTreeNode, TreeNode);
        K2_ASSERT(pReq->mpRequestor != NULL);
        ipcRemoteEndRef.AsAny = NULL;
        KernObj_CreateRef(&ipcRemoteEndRef, &pReq->mpRequestor->Hdr);
        pReq->mPending = (UINT32)&apIpcEnd->Hdr;
    }

    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    // 
    // have ref in ipcRemoteEndRef of the requestor here
    // we need to create a remote map of the local buffer
    // we need to create a local map of the remote buffer
    //
    pLocalProc = apCurThread->mIsKernelThread ? NULL : apCurThread->User.ProcRef.AsProc;
    pRemoteProc = ipcRemoteEndRef.AsIpcEnd->MailboxOwnerRef.AsMailboxOwner->RefProc.AsProc;
    pLocalVirtMap = apIpcEnd->VirtMapRef.AsVirtMap;
    pRemoteVirtMap = ipcRemoteEndRef.AsIpcEnd->VirtMapRef.AsVirtMap;

    //
    // create local map of remote buffer
    //
    tokLocalForRemote = NULL;
    refLocalMapOfRemoteBuffer.AsAny = NULL;
    if (NULL != pLocalProc)
    {
        //
        // local is a process
        //
        stat = KernProc_UserVirtHeapAlloc(pLocalProc, pRemoteVirtMap->mPageCount, &pUserVirtHeapNode);
        if (!K2STAT_IS_ERROR(stat))
        {
            localVirtAddrOfRemoteBuffer = K2HEAP_NodeAddr(&pUserVirtHeapNode->HeapNode);
            stat = KernVirtMap_CreateUser(
                pLocalProc,
                pRemoteVirtMap->PageArrayRef.AsPageArray,
                pRemoteVirtMap->mPageArrayStartPageIx,
                localVirtAddrOfRemoteBuffer,
                pRemoteVirtMap->mPageCount,
                K2OS_MapType_Data_ReadWrite,
                &refLocalMapOfRemoteBuffer
            );
            if (K2STAT_IS_ERROR(stat))
            {
                KernProc_UserVirtHeapFree(pLocalProc, localVirtAddrOfRemoteBuffer);
            }
            else
            {
                stat = KernProc_TokenCreate(pLocalProc, refLocalMapOfRemoteBuffer.AsAny, &tokLocalForRemote);
                if (K2STAT_IS_ERROR(stat))
                {
                    KernObj_ReleaseRef(&refLocalMapOfRemoteBuffer);
                    KernProc_UserVirtHeapFree(pLocalProc, localVirtAddrOfRemoteBuffer);
                }
            }
        }
    }
    else
    {
        //
        // local is the kernel
        //
        localVirtAddrOfRemoteBuffer = KernVirt_Reserve(pRemoteVirtMap->mPageCount);
        if (0 == localVirtAddrOfRemoteBuffer)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            stat = KernVirtMap_Create(
                pRemoteVirtMap->PageArrayRef.AsPageArray,
                pRemoteVirtMap->mPageArrayStartPageIx,
                pRemoteVirtMap->mPageCount,
                localVirtAddrOfRemoteBuffer,
                K2OS_MapType_Data_ReadWrite,
                &refLocalMapOfRemoteBuffer
            );
            if (K2STAT_IS_ERROR(stat))
            {
                KernVirt_Release(localVirtAddrOfRemoteBuffer);
            }
            else
            {
                stat = KernToken_Create(refLocalMapOfRemoteBuffer.AsAny, &tokLocalForRemote);
                if (K2STAT_IS_ERROR(stat))
                {
                    KernObj_ReleaseRef(&refLocalMapOfRemoteBuffer);
                    KernVirt_Release(localVirtAddrOfRemoteBuffer);
                }
            }
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    K2_ASSERT(NULL != tokLocalForRemote);

    // 
    // create remote map of local buffer
    //
    refRemoteMapOfLocalBuffer.AsAny = NULL;
    if (NULL != pRemoteProc)
    {
        //
        // remote is a process
        //
        stat = KernProc_UserVirtHeapAlloc(pRemoteProc, pLocalVirtMap->mPageCount, &pUserVirtHeapNode);
        if (!K2STAT_IS_ERROR(stat))
        {
            remoteVirtAddrOfLocalBuffer = K2HEAP_NodeAddr(&pUserVirtHeapNode->HeapNode);
            stat = KernVirtMap_CreateUser(
                pRemoteProc,
                pLocalVirtMap->PageArrayRef.AsPageArray,
                pLocalVirtMap->mPageArrayStartPageIx,
                remoteVirtAddrOfLocalBuffer,
                pLocalVirtMap->mPageCount,
                K2OS_MapType_Data_ReadWrite,
                &refRemoteMapOfLocalBuffer
            );
            if (K2STAT_IS_ERROR(stat))
            {
                KernProc_UserVirtHeapFree(pRemoteProc, remoteVirtAddrOfLocalBuffer);
            }
            else
            {
                stat = KernProc_TokenCreate(pRemoteProc, refRemoteMapOfLocalBuffer.AsAny, &tokRemoteForLocal);
                if (K2STAT_IS_ERROR(stat))
                {
                    KernObj_ReleaseRef(&refRemoteMapOfLocalBuffer);
                    KernProc_UserVirtHeapFree(pRemoteProc, remoteVirtAddrOfLocalBuffer);
                }
            }
        }
    }
    else
    {
        //
        // remote is the kernel
        //
        remoteVirtAddrOfLocalBuffer = KernVirt_Reserve(pLocalVirtMap->mPageCount);
        if (0 == remoteVirtAddrOfLocalBuffer)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            stat = KernVirtMap_Create(
                pLocalVirtMap->PageArrayRef.AsPageArray,
                pLocalVirtMap->mPageArrayStartPageIx,
                pLocalVirtMap->mPageCount,
                remoteVirtAddrOfLocalBuffer,
                K2OS_MapType_Data_ReadWrite,
                &refRemoteMapOfLocalBuffer
            );
            if (K2STAT_IS_ERROR(stat))
            {
                KernVirt_Release(localVirtAddrOfRemoteBuffer);
            }
            else
            {
                stat = KernToken_Create(refRemoteMapOfLocalBuffer.AsAny, &tokRemoteForLocal);
                if (K2STAT_IS_ERROR(stat))
                {
                    KernObj_ReleaseRef(&refRemoteMapOfLocalBuffer);
                    KernVirt_Release(remoteVirtAddrOfLocalBuffer);
                }
            }
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        if (NULL != pLocalProc)
        {
            KernProc_TokenDestroyInSysCall(apThisCore, apCurThread, tokLocalForRemote);
        }
        else
        {
            K2OS_Token_Destroy(tokLocalForRemote);
        }
        KernObj_ReleaseRef(&refLocalMapOfRemoteBuffer);
        return stat;
    }

    K2_ASSERT(NULL != tokRemoteForLocal);

    pSchedItem = &apCurThread->SchedItem;

    KernObj_CreateRef(&pSchedItem->Args.Ipc_Accept.LocalMapOfRemoteBufferRef, refLocalMapOfRemoteBuffer.AsAny);
    KernObj_ReleaseRef(&refLocalMapOfRemoteBuffer);
    pSchedItem->Args.Ipc_Accept.mTokLocalMapOfRemote = tokLocalForRemote;

    KernObj_CreateRef(&pSchedItem->Args.Ipc_Accept.RemoteMapOfLocalBufferRef, refRemoteMapOfLocalBuffer.AsAny);
    KernObj_ReleaseRef(&refRemoteMapOfLocalBuffer);
    pSchedItem->Args.Ipc_Accept.mTokRemoteMapOfLocal = tokRemoteForLocal;

    KernObj_CreateRef(&pSchedItem->Args.Ipc_Accept.RemoteEndRef, ipcRemoteEndRef.AsAny);
    KernObj_ReleaseRef(&ipcRemoteEndRef);

    KernObj_CreateRef(&pSchedItem->ObjRef, &apIpcEnd->Hdr);

    if (NULL != pLocalProc)
    {
        K2_ASSERT(NULL != apThisCore);

        pSchedItem->mType = KernSchedItem_Thread_SysCall;

        KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
        KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
        KernSched_QueueItem(pSchedItem);
    }
    else
    {
        // in kernel thread. call scheduler right here
        K2_ASSERT(NULL == apThisCore);

        disp = K2OSKERN_SetIntr(FALSE);
        K2_ASSERT(disp);

        apThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
        K2_ASSERT(apCurThread == apThisCore->mpActiveThread);

        pSchedItem->mType = KernSchedItem_KernThread_IpcAccept;

        KernThread_CallScheduler(apThisCore);

        // interrupts will be back on again here
        KernObj_ReleaseRef(&pSchedItem->ObjRef);

        if (0 == apCurThread->Kern.mSchedCall_Result)
        {
            stat = apCurThread->mpKernRwViewOfThreadPage->mLastStatus;
            K2_ASSERT(K2STAT_IS_ERROR(stat));
        }
    }

    return stat;
}

K2STAT
KernIpcEnd_RejectRequest(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    UINT32                      aRequestId,
    UINT32                      aReasonCode
)
{
    // sends reject message back to mailbox of open requestor
    K2STAT                  stat;
    BOOL                    disp;
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_IPC_REQUEST *  pReq;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_OBJREF         ipcRemoteEndRef;
    K2OSKERN_SCHED_ITEM *   pSchedItem;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    stat = K2STAT_NO_ERROR;

    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

    pTreeNode = K2TREE_Find(&gData.Iface.RequestTree, aRequestId);
    if (NULL == pTreeNode)
    {
        stat = K2STAT_ERROR_NOT_FOUND;
    }
    else
    {
        pReq = K2_GET_CONTAINER(K2OSKERN_IPC_REQUEST, pTreeNode, TreeNode);
        K2_ASSERT(pReq->mpRequestor != NULL);
        ipcRemoteEndRef.AsAny = NULL;
        KernObj_CreateRef(&ipcRemoteEndRef, &pReq->mpRequestor->Hdr);
        pReq->mPending = (UINT32)0xFFFFFFFF;
    }

    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    if (!K2STAT_IS_ERROR(stat))
    {
        pSchedItem = &apCurThread->SchedItem;

        disp = K2OSKERN_SeqLock(&ipcRemoteEndRef.AsIpcEnd->Connection.SeqLock);

        if (ipcRemoteEndRef.AsIpcEnd->Connection.Locked.mState != KernIpcEndState_Disconnected)
        {
            stat = K2STAT_ERROR_CONNECTED;
        }
        else if (NULL == ipcRemoteEndRef.AsIpcEnd->Connection.Locked.mpOpenRequest)
        {
            // connection request vanished between global lock above and connection lock here
            stat = K2STAT_ERROR_ABANDONED;
        }

        K2OSKERN_SeqUnlock(&ipcRemoteEndRef.AsIpcEnd->Connection.SeqLock, disp);

        if (!K2STAT_IS_ERROR(stat))
        {
            KernObj_CreateRef(&pSchedItem->ObjRef, ipcRemoteEndRef.AsAny);
        }

        KernObj_ReleaseRef(&ipcRemoteEndRef);
    }

    if (!K2STAT_IS_ERROR(stat))
    {
        pSchedItem->Args.Ipc_Reject.mRequestId = aRequestId;
        pSchedItem->Args.Ipc_Reject.mReasonCode = aReasonCode;

        if (!apCurThread->mIsKernelThread)
        {
            K2_ASSERT(NULL != apThisCore);
            pSchedItem->mType = KernSchedItem_Thread_SysCall;
            KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
            KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
            KernSched_QueueItem(pSchedItem);
        }
        else
        {
            // in kernel thread. call scheduler right here
            K2_ASSERT(NULL == apThisCore);

            disp = K2OSKERN_SetIntr(FALSE);
            K2_ASSERT(disp);

            apThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
            K2_ASSERT(apCurThread == apThisCore->mpActiveThread);

            pSchedItem->mType = KernSchedItem_KernThread_IpcRejectRequest;

            KernThread_CallScheduler(apThisCore);

            // interrupts will be back on again here
            KernObj_ReleaseRef(&pSchedItem->ObjRef);

            if (0 == apCurThread->Kern.mSchedCall_Result)
            {
                stat = pThreadPage->mLastStatus;
                K2_ASSERT(K2STAT_IS_ERROR(stat));
            }
        }
    }

    return stat;
}

K2STAT
KernIpcEnd_Sent(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    K2OSKERN_OBJ_IPCEND *       apIpcEnd,
    UINT32                      aByteCount
)
{
    K2STAT                  stat;
    K2OS_MSG                sendMsg;
    BOOL                    disp;
    K2OSKERN_OBJ_IPCEND *   pPartner;
    K2OSKERN_OBJ_MAILBOX *  pPartnerMailbox;
    BOOL                    doSignal;
    K2OSKERN_SCHED_ITEM *   pSchedItem;
    BOOL                    doSchedCall;

    K2MEM_Zero(&sendMsg, sizeof(sendMsg));
    sendMsg.mType = K2OS_SYSTEM_MSGTYPE_IPCEND;
    sendMsg.mShort = K2OS_SYSTEM_MSG_IPCEND_SHORT_RECV;
    sendMsg.mPayload[1] = aByteCount;
    sendMsg.mPayload[2] = 0;

    doSchedCall = FALSE;

    disp = K2OSKERN_SeqLock(&apIpcEnd->Connection.SeqLock);
    if (apIpcEnd->Connection.Locked.mState != KernIpcEndState_Connected)
    {
        stat = K2STAT_ERROR_NOT_CONNECTED;
    }
    else
    {
        if (!apIpcEnd->Connection.Locked.Connected.mTookPartnerReserve)
        {
            // not loaded
            stat = K2STAT_ERROR_API_ORDER;
        }
        else
        {
            pPartner = apIpcEnd->Connection.Locked.Connected.PartnerIpcEndRef.AsIpcEnd;
            pPartnerMailbox = pPartner->MailboxOwnerRef.AsMailboxOwner->RefMailbox.AsMailbox;

            if (0 == aByteCount)
            {
                //
                // sending 0 bytes on a loaded endpoint means abandon the send
                //
                stat = K2STAT_ERROR_ABANDONED;
//                K2OSKERN_Debug("undo reserve for load (intentional abandon)\n");
                KernMailbox_UndoReserve(pPartnerMailbox, 1);
            }
            else
            {
                stat = K2STAT_NO_ERROR;

                sendMsg.mPayload[0] = (UINT32)pPartner->mpUserKey;

                doSignal = FALSE;
                KernMailbox_Deliver(
                    pPartnerMailbox,
                    &sendMsg,
                    FALSE,              // this is a reserve, so do not alloc
                    &pPartner->Hdr,     // this is the object notified when message has completed receive
                    &doSignal
                );

                if (doSignal)
                {
                    //
                    // enter scheduler to release thread waiting for message
                    //
                    pSchedItem = &apCurThread->SchedItem;
                    KernObj_CreateRef(&pSchedItem->ObjRef, &pPartnerMailbox->Hdr);

                    if (!apCurThread->mIsKernelThread)
                    {
                        // in system call from user thread.  go into scheduler
                        K2_ASSERT(NULL != apThisCore);
                        K2_ASSERT(apCurThread->User.mIsInSysCall);
                        pSchedItem->mType = KernSchedItem_Thread_SysCall;
                        KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
                        KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
                        KernSched_QueueItem(pSchedItem);
                    }
                    else
                    {
                        // in kernel thread. call scheduler 
                        doSchedCall = TRUE;
                    }
                }
            }

            apIpcEnd->Connection.Locked.Connected.mTookPartnerReserve = FALSE;
        }
    }

    K2OSKERN_SeqUnlock(&apIpcEnd->Connection.SeqLock, disp);

    if (doSchedCall)
    {
        K2_ASSERT(NULL == apThisCore);

        disp = K2OSKERN_SetIntr(FALSE);
        K2_ASSERT(disp);

        apThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
        K2_ASSERT(apCurThread == apThisCore->mpActiveThread);

        pSchedItem->mType = KernSchedItem_KernThread_MailboxSentFirst;

        KernThread_CallScheduler(apThisCore);

        // interrupts will be back on again here
        KernObj_ReleaseRef(&pSchedItem->ObjRef);
    }

    return stat;
}

K2STAT
KernIpcEnd_Load(
    K2OSKERN_OBJ_IPCEND * apIpcEnd
)
{
    BOOL                    disp;
    K2STAT                  stat;
    K2OSKERN_OBJ_IPCEND *   pPartnerIpcEnd;

    //
    // DO NOT ENTER SCHEDULER.  may be in intr
    //

    disp = K2OSKERN_SeqLock(&apIpcEnd->Connection.SeqLock);

    if (apIpcEnd->Connection.Locked.mState != KernIpcEndState_Connected)
    {
        stat = K2STAT_ERROR_NOT_CONNECTED;
    }
    else
    {
        pPartnerIpcEnd = apIpcEnd->Connection.Locked.Connected.PartnerIpcEndRef.AsIpcEnd;
        if (!KernMailbox_Reserve(pPartnerIpcEnd->MailboxOwnerRef.AsMailboxOwner->RefMailbox.AsMailbox, 1))
        {
            stat = K2STAT_ERROR_OUT_OF_RESOURCES;
        }
        else
        {
            apIpcEnd->Connection.Locked.Connected.mTookPartnerReserve = TRUE;
        }
    }

    K2OSKERN_SeqUnlock(&apIpcEnd->Connection.SeqLock, disp);

    return stat;
}

K2STAT
KernIpcEnd_ManualDisconnect(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    K2OSKERN_OBJ_IPCEND *       apIpcEnd
)
{
    BOOL                    disp;
    K2OSKERN_IPC_REQUEST *  pReq;
    K2STAT                  stat;
    K2OSKERN_SCHED_ITEM *   pSchedItem;
    BOOL                    doSchedCall;

    stat = K2STAT_NO_ERROR;
    doSchedCall = FALSE;

    disp = K2OSKERN_SeqLock(&apIpcEnd->Connection.SeqLock);

    if ((apIpcEnd->Connection.Locked.mState == KernIpcEndState_Disconnected) &&
        (NULL != apIpcEnd->Connection.Locked.mpOpenRequest))
    {
        pReq = apIpcEnd->Connection.Locked.mpOpenRequest;
        apIpcEnd->Connection.Locked.mpOpenRequest = NULL;
        K2OSKERN_SeqLock(&gData.Iface.SeqLock);
        pReq->mpRequestor = NULL;
        K2TREE_Remove(&gData.Iface.RequestTree, &pReq->TreeNode);
        K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, FALSE);
    }
    else if (apIpcEnd->Connection.Locked.mState == KernIpcEndState_Connected)
    {
        pSchedItem = &apCurThread->SchedItem;
        KernObj_CreateRef(&pSchedItem->ObjRef, &apIpcEnd->Hdr);
        if (!apCurThread->mIsKernelThread)
        {
            K2_ASSERT(NULL != apThisCore);
            K2_ASSERT(apCurThread->User.mIsInSysCall);
            KernArch_GetHfTimerTick(&apCurThread->SchedItem.mHfTick);
            KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
            KernSched_QueueItem(&apCurThread->SchedItem);
        }
        else
        {
            doSchedCall = TRUE;
        }
    }
    else
    {
        stat = K2STAT_ERROR_NOT_CONNECTED;
    }

    K2OSKERN_SeqUnlock(&apIpcEnd->Connection.SeqLock, disp);

    if (doSchedCall)
    {
        // in kernel thread. call scheduler right here
        K2_ASSERT(NULL == apThisCore);

        disp = K2OSKERN_SetIntr(FALSE);
        K2_ASSERT(disp);

        apThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
        K2_ASSERT(apCurThread == apThisCore->mpActiveThread);

        pSchedItem->mType = KernSchedItem_KernThread_IpcEndManualDisconnect;

        KernThread_CallScheduler(apThisCore);

        // interrupts will be back on again here
        KernObj_ReleaseRef(&pSchedItem->ObjRef);

        if (0 == apCurThread->Kern.mSchedCall_Result)
        {
            stat = apCurThread->mpKernRwViewOfThreadPage->mLastStatus;
            K2_ASSERT(K2STAT_IS_ERROR(stat));
        }
    }

    return stat;
}

K2STAT
KernIpcEnd_SendRequest(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    K2OSKERN_OBJ_IPCEND *       apIpcEnd,
    UINT32                      aIfInstId
)
{
    K2OSKERN_IPC_REQUEST *  pReq;
    K2STAT                  stat;
    BOOL                    disp;
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_OBJ_IFINST *   pIfInst;
    K2OSKERN_OBJREF         refTargetMailbox;
    K2OS_MSG                requestMsg;
    BOOL                    firstReq;
    BOOL                    doSignal;
    K2OSKERN_SCHED_ITEM *   pSchedItem;

    pReq = (K2OSKERN_IPC_REQUEST *)KernHeap_Alloc(sizeof(K2OSKERN_IPC_REQUEST));
    if (NULL == pReq)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pReq, sizeof(K2OSKERN_IPC_REQUEST));

    pReq->mIfInstId = aIfInstId;
    pReq->mpRequestor = apIpcEnd;

    refTargetMailbox.AsAny = NULL;

    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

    pTreeNode = K2TREE_Find(&gData.Iface.IdTree, aIfInstId);
    if (NULL != pTreeNode)
    {
        pIfInst = K2_GET_CONTAINER(K2OSKERN_OBJ_IFINST, pTreeNode, IdTreeNode);
        if (NULL == pIfInst->RefMailboxOwner.AsAny)
        {
            pTreeNode = NULL;
            stat = K2STAT_ERROR_NOT_OWNED;
        }
        else
        {
            if ((!apCurThread->mIsKernelThread) &&
                (apCurThread->User.ProcRef.AsProc == pIfInst->RefMailboxOwner.AsMailboxOwner->RefProc.AsProc))
            {
                // cannot send a request to yourself
                stat = K2STAT_ERROR_NOT_ALLOWED;
            }
            else
            {
                KernObj_CreateRef(&refTargetMailbox, pIfInst->RefMailboxOwner.AsMailboxOwner->RefMailbox.AsAny);
                pReq->TreeNode.mUserVal = ++gData.Iface.mLastRequestId;
                K2TREE_Insert(&gData.Iface.RequestTree, pReq->TreeNode.mUserVal, &pReq->TreeNode);
                stat = K2STAT_NO_ERROR;
            }
        }
    }

    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    if (!K2STAT_IS_ERROR(stat))
    {
        firstReq = FALSE;

        requestMsg.mType = K2OS_SYSTEM_MSGTYPE_IPC;
        requestMsg.mShort = K2OS_SYSTEM_MSG_IPC_SHORT_REQUEST;
        requestMsg.mPayload[0] = aIfInstId;
        requestMsg.mPayload[1] = apCurThread->mIsKernelThread ? 0 : apCurThread->User.ProcRef.AsProc->mId;
        requestMsg.mPayload[2] = pReq->TreeNode.mUserVal;

        disp = K2OSKERN_SeqLock(&apIpcEnd->Connection.SeqLock);

        if (apIpcEnd->Connection.Locked.mState != KernIpcEndState_Disconnected)
        {
            stat = K2STAT_ERROR_CONNECTED;
        }
        else
        {
            stat = K2STAT_NO_ERROR;

            if (NULL != apIpcEnd->Connection.Locked.mpOpenRequest)
            {
                //
                // this endpoint already has a pending request
                //
                if (apIpcEnd->Connection.Locked.mpOpenRequest->mIfInstId != aIfInstId)
                {
                    // 
                    // already an open rquest for another interface on this endpoint
                    //
                    stat = K2STAT_ERROR_API_ORDER;
                }
            }
            else
            {
                apIpcEnd->Connection.Locked.mpOpenRequest = pReq;
                pReq = NULL;
            }
        }

        K2OSKERN_SeqUnlock(&apIpcEnd->Connection.SeqLock, disp);

        if (NULL != pReq)
        {
            disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

            K2TREE_Remove(&gData.Iface.RequestTree, &pReq->TreeNode);

            K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

            // will get freed below
        }
        else
        {
            firstReq = TRUE;
        }

        if (!K2STAT_IS_ERROR(stat))
        {
            // 
            // deliver request message to target mailbox
            //
            doSignal = FALSE;
            stat = KernMailbox_Deliver(
                refTargetMailbox.AsMailbox,
                &requestMsg,
                TRUE,   // do alloc
                NULL,   // not a reserve send
                &doSignal
            );
            if (K2STAT_IS_ERROR(stat))
            {
                if (firstReq)
                {
                    // undo the request
                    disp = K2OSKERN_SeqLock(&apIpcEnd->Connection.SeqLock);
                    pReq = apIpcEnd->Connection.Locked.mpOpenRequest;
                    if (NULL != pReq)
                    {
                        K2OSKERN_SeqLock(&gData.Iface.SeqLock); // ordering with connection seqlock
                        apIpcEnd->Connection.Locked.mpOpenRequest = NULL;
                        pReq->mpRequestor = NULL;
                        K2TREE_Remove(&gData.Iface.RequestTree, &pReq->TreeNode);
                        K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, FALSE);
                        // freed below
                    }
                    K2OSKERN_SeqUnlock(&apIpcEnd->Connection.SeqLock, disp);
                }
            }
            else if (doSignal)
            {
                //
                // enter scheduler to release thread waiting for message
                //
                if (!apCurThread->mIsKernelThread)
                {
                    // in system call from user thread.  go into scheduler
                    K2_ASSERT(NULL != apThisCore);
                    K2_ASSERT(apCurThread->User.mIsInSysCall);
                    pSchedItem = &apCurThread->SchedItem;
                    pSchedItem->mType = KernSchedItem_Thread_SysCall;
                    KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
                    KernObj_CreateRef(&pSchedItem->ObjRef, refTargetMailbox.AsAny);
                    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
                    KernSched_QueueItem(pSchedItem);
                }
                else
                {
                    // in kernel thread. call scheduler right here
                    K2_ASSERT(NULL == apThisCore);

                    disp = K2OSKERN_SetIntr(FALSE);
                    K2_ASSERT(disp);

                    apThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
                    K2_ASSERT(apCurThread == apThisCore->mpActiveThread);

                    pSchedItem = &apCurThread->SchedItem;
                    pSchedItem->mType = KernSchedItem_KernThread_MailboxSentFirst;
                    KernObj_CreateRef(&pSchedItem->ObjRef, refTargetMailbox.AsAny);

                    KernThread_CallScheduler(apThisCore);

                    // interrupts will be back on again here
                    KernObj_ReleaseRef(&pSchedItem->ObjRef);
                }
            }
        }

        KernObj_ReleaseRef(&refTargetMailbox);
    }

    if (NULL != pReq)
    {
        KernHeap_Free(pReq);
    }

    return stat;
}

void
KernIpcEnd_RecvRes(
    K2OSKERN_OBJ_IPCEND *   apIpcEnd,
    BOOL                    aDelivered
)
{
    BOOL disp;

    //
    // called from mailbox code when a reserve has completed receive
    // the only place where the object is non-null is when it is a 
    // disconnect message, which is an 'ack' of the disconnect
    // by the user of the endpoint.  this confirms the completion
    // of the disconnect and allows the endpoint to be used again
    // for another purpose
    //

    disp = K2OSKERN_SeqLock(&apIpcEnd->Connection.SeqLock);

    //
    // if this fires a reserve had an object attached but the reserve
    // was not used for a disconnect message.  which is bad.
    //
    if (apIpcEnd->Connection.Locked.mState == KernIpcEndState_WaitDisAck)
    {
        KernIpcEnd_ConnLocked_DisconnectAck(apIpcEnd);
    }
    else if (apIpcEnd->Connection.Locked.mState == KernIpcEndState_Connected)
    {
        // this reserve is the load the sender took before they completed their send
//        K2OSKERN_Debug("undo reserve for load (received)\n");
        KernMailbox_UndoReserve(apIpcEnd->MailboxOwnerRef.AsMailboxOwner->RefMailbox.AsMailbox, 1);
    }

    K2OSKERN_SeqUnlock(&apIpcEnd->Connection.SeqLock, disp);
}

//
// -----------------------------------------------------------------------------------
//

void 
KernIpcEnd_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OS_THREAD_PAGE *  pThreadPage;
    K2OSKERN_OBJREF     refMailboxOwner;
    K2OSKERN_OBJREF     refVirtMap;
    K2STAT              stat;

    // arg0 key
    // arg1 mailboxowner token
    // arg2 user map token
    // arg3 ipcend properties (max msg size and count)

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    refMailboxOwner.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)pThreadPage->mSysCall_Arg1, &refMailboxOwner);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_MailboxOwner != refMailboxOwner.AsAny->mObjType)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            refVirtMap.AsAny = NULL;
            stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)pThreadPage->mSysCall_Arg2, &refVirtMap);
            if (!K2STAT_IS_ERROR(stat))
            {
                if (KernObj_VirtMap != refVirtMap.AsAny->mObjType)
                {
                    stat = K2STAT_ERROR_BAD_TOKEN;
                }
                else
                {
                    stat = KernIpcEnd_Create(
                        apThisCore,
                        apCurThread,
                        (void *)apCurThread->User.mSysCall_Arg0,
                        refMailboxOwner.AsMailboxOwner,
                        refVirtMap.AsVirtMap,
                        pThreadPage->mSysCall_Arg3,
                        (K2OS_IPCEND_TOKEN *)&apCurThread->User.mSysCall_Result
                    );
                    // this goes to scheduler call if it succeeds and sets result and the last status
                }
                KernObj_ReleaseRef(&refVirtMap);
            }
        }
        KernObj_ReleaseRef(&refMailboxOwner);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void 
KernIpcEnd_SysCall_Send(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF     refIpcEnd;
    K2STAT              stat;
    K2OS_THREAD_PAGE *  pThreadPage;

    // arg0 endpoint token
    // arg1 bytecount

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    refIpcEnd.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refIpcEnd);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_IpcEnd != refIpcEnd.AsAny->mObjType)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            stat = KernIpcEnd_Sent(apThisCore, apCurThread, refIpcEnd.AsIpcEnd, pThreadPage->mSysCall_Arg1);
            // this goes to scheduler call if it succeeds and sets result and the last status
        }

        KernObj_ReleaseRef(&refIpcEnd);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void 
KernIpcEnd_SysCall_Accept(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF     refIpcEnd;
    K2STAT              stat;
    K2OS_THREAD_PAGE *  pThreadPage;

    // arg0 ipcend token
    // arg1 request id

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    refIpcEnd.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refIpcEnd);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_IpcEnd != refIpcEnd.AsAny->mObjType)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            stat = KernIpcEnd_Accept(apThisCore, apCurThread, refIpcEnd.AsIpcEnd, pThreadPage->mSysCall_Arg1);
            // this goes to scheduler call if it succeeds and sets result and the last status
        }

        KernObj_ReleaseRef(&refIpcEnd);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void 
KernIpcEnd_SysCall_RejectRequest(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT              stat;
    K2OS_THREAD_PAGE *  pThreadPage;

    // arg0 request id
    // arg1 reason code

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;
    stat = KernIpcEnd_RejectRequest(apThisCore, apCurThread, apCurThread->User.mSysCall_Arg0, pThreadPage->mSysCall_Arg1);
    // this goes to scheduler call if it succeeds and sets result and the last status
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void 
KernIpcEnd_SysCall_ManualDisconnect(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF     refIpcEnd;
    K2STAT              stat;

    // arg0 endpoint token

    refIpcEnd.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refIpcEnd);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_IpcEnd != refIpcEnd.AsAny->mObjType)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            stat = KernIpcEnd_ManualDisconnect(
                apThisCore,
                apCurThread,
                refIpcEnd.AsIpcEnd
            );
            // this goes to scheduler call if it succeeds and sets result and the last status
        }

        KernObj_ReleaseRef(&refIpcEnd);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
}

void 
KernIpcEnd_SysCall_Request(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF     refIpcEnd;
    K2STAT              stat;
    K2OS_THREAD_PAGE *  pThreadPage;

    // arg0 endpoint token
    // arg1 ifinst id

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    refIpcEnd.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refIpcEnd);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_IpcEnd != refIpcEnd.AsAny->mObjType)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            stat = KernIpcEnd_SendRequest(
                apThisCore,
                apCurThread,
                refIpcEnd.AsIpcEnd,
                pThreadPage->mSysCall_Arg1
            );
            // this goes to scheduler call if it succeeds and sets result and the last status
        }

        KernObj_ReleaseRef(&refIpcEnd);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

BOOL 
KernIpcEnd_InIntr_SysCall_Load(
    K2OSKERN_OBJ_THREAD *   apCurThread,
    K2OS_TOKEN              aTokIpcEnd
)
{
    K2OSKERN_OBJREF refIpcEnd;
    K2STAT          stat;

    refIpcEnd.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->User.ProcRef.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refIpcEnd);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_IpcEnd != refIpcEnd.AsAny->mObjType)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            stat = KernIpcEnd_Load(refIpcEnd.AsIpcEnd);
        }

        KernObj_ReleaseRef(&refIpcEnd);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
        return FALSE;
    }

    return TRUE;
}

