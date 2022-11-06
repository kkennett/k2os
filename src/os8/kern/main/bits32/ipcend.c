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

#include "kern.h"

K2STAT
KernIpcEnd_Create(
    void *                  apUserKey,
    K2OSKERN_OBJ_MAILBOX *  apMailbox,
    K2OSKERN_OBJ_MAP *      apMap,
    UINT32                  aMsgConfig,
    K2OSKERN_OBJREF *       apFillRef
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_OBJ_IPCEND *   pIpcEnd;
    BOOL                    disp;

    pProc = apMailbox->Common.ProcRef.Ptr.AsProc;
    K2_ASSERT(pProc == apMap->ProcRef.Ptr.AsProc);

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

        stat = KernSignalProxy_Create(&pIpcEnd->DeleteSignalProxyRef);
        if (K2STAT_IS_ERROR(stat))
        {
            break;
        }

        do
        {
            // reserved single-use mailbox messages - create, connect, disconnect, delete = 4
            if (!KernMailbox_Reserve(&apMailbox->Common, 4))
            {
                stat = K2STAT_ERROR_OUT_OF_RESOURCES;
                break;
            }

            pIpcEnd->Hdr.mObjType = KernObj_IpcEnd;
            K2LIST_Init(&pIpcEnd->Hdr.RefObjList);

            pIpcEnd->mpUserKey = apUserKey;

            KernObj_CreateRef(&pIpcEnd->MailboxRef, &apMailbox->Hdr);
            KernObj_CreateRef(&pIpcEnd->ReadMapRef, &apMap->Hdr);
            pIpcEnd->mMaxMsgBytes = (aMsgConfig & 0xFFFF);
            pIpcEnd->mMaxMsgCount = (aMsgConfig >> 16) & 0xFFFF;

            K2OSKERN_SeqInit(&pIpcEnd->Connection.SeqLock);
            pIpcEnd->Connection.Locked.mState = KernIpcEndState_Disconnected;

            disp = K2OSKERN_SeqLock(&pProc->IpcEnd.SeqLock);
            K2LIST_AddAtTail(&pProc->IpcEnd.Locked.List, &pIpcEnd->ProcIpcEndListLocked.ListLink);
            K2OSKERN_SeqUnlock(&pProc->IpcEnd.SeqLock, disp);

            KernObj_CreateRef(apFillRef, &pIpcEnd->Hdr);

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            KernObj_ReleaseRef(&pIpcEnd->DeleteSignalProxyRef);
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        KernHeap_Free(pIpcEnd);
    }

    return stat;
}

void 
KernIpcEnd_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJREF         mailboxRef;
    K2OSKERN_OBJREF         mapRef;
    K2OSKERN_OBJREF         ipcEndRef;
    K2OS_MSG                createdMsg;

    pProc = apCurThread->ProcRef.Ptr.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    mailboxRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)pThreadPage->mSysCall_Arg1, &mailboxRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if ((KernObj_Mailbox != mailboxRef.Ptr.AsHdr->mObjType) ||
            (mailboxRef.Ptr.AsMailbox->Common.SchedOnlyCanSet.mIsClosed))
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            mapRef.Ptr.AsHdr = NULL;
            stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)pThreadPage->mSysCall_Arg2, &mapRef);
            if (!K2STAT_IS_ERROR(stat))
            {
                if ((KernObj_Map != mapRef.Ptr.AsHdr->mObjType) ||
                    (mapRef.Ptr.AsMap->mUserMapType != K2OS_MapType_Data_ReadWrite))
                {
                    stat = K2STAT_ERROR_BAD_TOKEN;
                }
                else
                {
                    ipcEndRef.Ptr.AsHdr = NULL;
                    stat = KernIpcEnd_Create((void *)apCurThread->mSysCall_Arg0, mailboxRef.Ptr.AsMailbox, mapRef.Ptr.AsMap, pThreadPage->mSysCall_Arg3, &ipcEndRef);
                    if (!K2STAT_IS_ERROR(stat))
                    {
                        stat = KernProc_TokenCreate(pProc, ipcEndRef.Ptr.AsHdr, (K2OS_TOKEN *)&apCurThread->mSysCall_Result);
                        if (!K2STAT_IS_ERROR(stat))
                        {
                            createdMsg.mControl = K2OS_SYSTEM_MSG_IPC_CREATED;
                            createdMsg.mPayload[0] = (UINT32)ipcEndRef.Ptr.AsIpcEnd->mpUserKey;
                            createdMsg.mPayload[1] = apCurThread->mSysCall_Result;
                            //
                            // cannot fail send because we're sending one of our 4 reserved
                            // messages.  do not set the reserved bit for the create message
                            // as we want it to be consumed by the receiver, which should only
                            // leave us with 3 remaining reserves (connect, disconnect, delete)
                            // 
                            // mailbox deliver with a thread object as the sender will
                            // result in this system call going into the scheduler
                            //
                            stat = KernMailbox_Deliver(apThisCore, &mailboxRef.Ptr.AsMailbox->Common, &createdMsg, &apCurThread->Hdr, FALSE, FALSE, NULL);
                            K2_ASSERT(!K2STAT_IS_ERROR(stat));
                        }
                        else
                        {
                            //
                            // create message never sent.  release the reserve for it.  other reserves will
                            // get cleaned up in ipc endpoint cleanup
                            //
                            KernMailbox_UndoReserve(&mailboxRef.Ptr.AsMailbox->Common, 1);
                        }
                        KernObj_ReleaseRef(&ipcEndRef);
                    }
                }
                KernObj_ReleaseRef(&mapRef);
            }
        }
        KernObj_ReleaseRef(&mailboxRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void
KernIpcEnd_SysCall_Send(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJREF         ipcEndRef;
    K2OS_MSG                sendMsg;
    BOOL                    disp;

    pProc = apCurThread->ProcRef.Ptr.AsProc;
    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    ipcEndRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &ipcEndRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if ((KernObj_IpcEnd != ipcEndRef.Ptr.AsHdr->mObjType) ||
            (ipcEndRef.Ptr.AsIpcEnd->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc != pProc))
        {
            //
            // either its not an endpoint, or its not an endpoint that belongs to the current process
            //
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            K2MEM_Zero(&sendMsg, sizeof(sendMsg));
            sendMsg.mControl = K2OS_SYSTEM_MSG_IPC_RECV;
            sendMsg.mPayload[1] = pThreadPage->mSysCall_Arg1;
            sendMsg.mPayload[2] = 0;

            disp = K2OSKERN_SeqLock(&ipcEndRef.Ptr.AsIpcEnd->Connection.SeqLock);
            if ((ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mState != KernIpcEndState_Connected) ||
                (ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mInClose))
            {
                stat = K2STAT_ERROR_NOT_CONNECTED;
            }
            else
            {
                if (!ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.Connected.mIsLoaded)
                {
                    stat = K2STAT_ERROR_API_ORDER;
                }
                else
                {
                    if (0 == pThreadPage->mSysCall_Arg1)
                    {
                        //
                        // sending 0 bytes on a loaded endpoint means abandon the send
                        //
                        stat = K2STAT_ERROR_ABANDONED;
                    }
                    else
                    {
                        sendMsg.mPayload[0] = (UINT32)ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.Connected.PartnerIpcEndRef.Ptr.AsIpcEnd->mpUserKey;

                        apCurThread->mSysCall_Result = TRUE;

                        // 
                        // mailbox deliver with a thread object as the sender will
                        // result in this system call going into the scheduler
                        //
                        KernMailbox_Deliver(
                            apThisCore,
                            &ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.Connected.PartnerIpcEndRef.Ptr.AsIpcEnd->MailboxRef.Ptr.AsMailbox->Common,
                            &sendMsg,
                            &apCurThread->Hdr,
                            FALSE, FALSE, NULL  // do not do alloc (we held the reserve), and do not set the reserve bit
                        );
                    }

                    ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.Connected.mIsLoaded = FALSE;
                }
            }
            K2OSKERN_SeqUnlock(&ipcEndRef.Ptr.AsIpcEnd->Connection.SeqLock, disp);
        }
        KernObj_ReleaseRef(&ipcEndRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = FALSE;
        pThreadPage->mLastStatus = stat;
    }
}

void
KernIpcEnd_ConnLocked_DisconnectAck(
    K2OSKERN_OBJ_IPCEND * apIpcEnd
)
{
    if (apIpcEnd->Connection.Locked.Connected.mIsLoaded)
    {
        KernMailbox_UndoReserve(&apIpcEnd->Connection.Locked.Connected.PartnerIpcEndRef.Ptr.AsIpcEnd->MailboxRef.Ptr.AsMailbox->Common, 1);
        apIpcEnd->Connection.Locked.Connected.mIsLoaded = FALSE;
    }
    KernObj_ReleaseRef(&apIpcEnd->Connection.Locked.Connected.WriteMapRef);
    KernObj_ReleaseRef(&apIpcEnd->Connection.Locked.Connected.PartnerIpcEndRef);
    apIpcEnd->Connection.Locked.mState = KernIpcEndState_Disconnected;
}

void
KernIpcEnd_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_IPCEND *       apIpcEnd
)
{
    BOOL                    disp;
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_IPC_REQUEST *  pReq;

    K2_ASSERT(apIpcEnd->DeleteSignalProxyRef.Ptr.AsHdr != NULL);

    pProc = apIpcEnd->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc;
    K2_ASSERT(NULL != pProc);

    disp = K2OSKERN_SeqLock(&apIpcEnd->Connection.SeqLock);
    K2_ASSERT(apIpcEnd->Connection.Locked.mState != KernIpcEndState_Connected);
    if (apIpcEnd->Connection.Locked.mState == KernIpcEndState_WaitDisAck)
    {
        KernIpcEnd_ConnLocked_DisconnectAck(apIpcEnd);
    }
    pReq = apIpcEnd->Connection.Locked.mpOpenRequest;
    if (NULL != pReq)
    {
        K2OSKERN_SeqLock(&gData.Iface.SeqLock);
        K2TREE_Remove(&gData.Iface.Locked.RequestTree, &pReq->GlobalTreeNode);
        pReq->mpRequestor = NULL;
        apIpcEnd->Connection.Locked.mpOpenRequest = NULL;
        K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, FALSE);
    }
    K2OSKERN_SeqUnlock(&apIpcEnd->Connection.SeqLock, disp);
    if (NULL != pReq)
    {
        K2MEM_Zero(pReq, sizeof(K2OSKERN_IPC_REQUEST));
        KernHeap_Free(pReq);
    }

    KernObj_ReleaseRef(&apIpcEnd->DeleteSignalProxyRef);

    disp = K2OSKERN_SeqLock(&pProc->IpcEnd.SeqLock);
    K2LIST_Remove(&pProc->IpcEnd.Locked.List, &apIpcEnd->ProcIpcEndListLocked.ListLink);
    K2OSKERN_SeqUnlock(&pProc->IpcEnd.SeqLock, disp);

    KernObj_ReleaseRef(&apIpcEnd->ReadMapRef);

    KernObj_ReleaseRef(&apIpcEnd->MailboxRef);

    K2MEM_Zero(apIpcEnd, sizeof(K2OSKERN_OBJ_IPCEND));

    KernHeap_Free(apIpcEnd);
}

K2STAT
KernIpcEnd_TryAccept(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread,
    K2OSKERN_OBJ_IPCEND *       apLocal,
    K2OSKERN_OBJ_IPCEND *       apRemote
)
{
    K2STAT                      stat;
    K2STAT                      stat2;
    K2OS_TOKEN                  tokLocal;
    K2OS_TOKEN                  tokRemote;
    K2OSKERN_OBJREF             localMapOfRemoteBufferRef;
    K2OSKERN_OBJREF             remoteMapOfLocalBufferRef;
    K2OSKERN_PROCHEAP_NODE *    pProcHeapNode;

    //
    // create local map of remote buffer and a token for it
    //
    stat = KernProc_UserVirtHeapAlloc(
        apCurThread->ProcRef.Ptr.AsProc,
        apRemote->ReadMapRef.Ptr.AsMap->mPageCount,
        &pProcHeapNode);
    if (K2STAT_IS_ERROR(stat))
        return stat;
    localMapOfRemoteBufferRef.Ptr.AsHdr = NULL;
    stat = KernMap_Create(
        apCurThread->ProcRef.Ptr.AsProc,
        apRemote->ReadMapRef.Ptr.AsMap->PageArrayRef.Ptr.AsPageArray,
        apRemote->ReadMapRef.Ptr.AsMap->mPageArrayStartPageIx,
        pProcHeapNode->HeapNode.AddrTreeNode.mUserVal,
        apRemote->ReadMapRef.Ptr.AsMap->mPageCount,
        K2OS_MapType_Data_ReadWrite,
        &localMapOfRemoteBufferRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        pProcHeapNode->mUserOwned = 0;
        stat = KernProc_TokenCreate(
            apCurThread->ProcRef.Ptr.AsProc,
            localMapOfRemoteBufferRef.Ptr.AsHdr,
            &tokLocal
        );
        if (K2STAT_IS_ERROR(stat))
        {
            KernObj_ReleaseRef(&localMapOfRemoteBufferRef);
        }
    }
    else
    {
        stat2 = KernProc_UserVirtHeapFree(
            apCurThread->ProcRef.Ptr.AsProc,
            pProcHeapNode->HeapNode.AddrTreeNode.mUserVal
        );
        K2_ASSERT(!K2STAT_IS_ERROR(stat2));
    }
    if (K2STAT_IS_ERROR(stat))
        return stat;

    //
    // create remote map of local buffer and a remote token for it
    //
    stat = KernProc_UserVirtHeapAlloc(
        apRemote->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc,
        apLocal->ReadMapRef.Ptr.AsMap->mPageCount,
        &pProcHeapNode);
    if (!K2STAT_IS_ERROR(stat))
    {
        remoteMapOfLocalBufferRef.Ptr.AsHdr = NULL;
        stat = KernMap_Create(
            apRemote->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc,
            apLocal->ReadMapRef.Ptr.AsMap->PageArrayRef.Ptr.AsPageArray,
            apLocal->ReadMapRef.Ptr.AsMap->mPageArrayStartPageIx,
            pProcHeapNode->HeapNode.AddrTreeNode.mUserVal,
            apLocal->ReadMapRef.Ptr.AsMap->mPageCount,
            K2OS_MapType_Data_ReadWrite,
            &remoteMapOfLocalBufferRef);
        if (!K2STAT_IS_ERROR(stat))
        {
            pProcHeapNode->mUserOwned = 0;
            stat = KernProc_TokenCreate(
                apRemote->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc,
                remoteMapOfLocalBufferRef.Ptr.AsHdr,
                &tokRemote
            );
            if (K2STAT_IS_ERROR(stat))
            {
                KernObj_ReleaseRef(&remoteMapOfLocalBufferRef);
            }
            // else cannot fail between here and sched item queue
        }
        else
        {
            stat2 = KernProc_UserVirtHeapFree(
                apRemote->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc,
                pProcHeapNode->HeapNode.AddrTreeNode.mUserVal
            );
            K2_ASSERT(!K2STAT_IS_ERROR(stat2));
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        KernObj_ReleaseRef(&localMapOfRemoteBufferRef);
        KernProc_TokenDestroy(apCurThread->ProcRef.Ptr.AsProc, tokLocal);
        return stat;
    }

    apCurThread->SchedItem.mType = KernSchedItem_Thread_SysCall;

    apCurThread->SchedItem.Args.Ipc_Accept.mTokLocalMapOfRemoteBuffer = tokLocal;
    apCurThread->SchedItem.Args.Ipc_Accept.LocalMapOfRemoteBufferRef.Ptr.AsHdr = NULL;
    KernObj_CreateRef(&apCurThread->SchedItem.Args.Ipc_Accept.LocalMapOfRemoteBufferRef, localMapOfRemoteBufferRef.Ptr.AsHdr);
    KernObj_ReleaseRef(&localMapOfRemoteBufferRef);

    apCurThread->SchedItem.Args.Ipc_Accept.mTokRemoteMapOfLocalBuffer = tokRemote;
    apCurThread->SchedItem.Args.Ipc_Accept.RemoteMapOfLocalBufferRef.Ptr.AsHdr = NULL;
    KernObj_CreateRef(&apCurThread->SchedItem.Args.Ipc_Accept.RemoteMapOfLocalBufferRef, remoteMapOfLocalBufferRef.Ptr.AsHdr);
    KernObj_ReleaseRef(&remoteMapOfLocalBufferRef);

    apCurThread->SchedItem.Args.Ipc_Accept.RemoteEndRef.Ptr.AsHdr = NULL;
    KernObj_CreateRef(&apCurThread->SchedItem.Args.Ipc_Accept.RemoteEndRef, &apRemote->Hdr);

    KernObj_CreateRef(&apCurThread->SchedItem.ObjRef, &apLocal->Hdr);

    KernTimer_GetHfTick(&apCurThread->SchedItem.mHfTick);

    KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);

    KernSched_QueueItem(&apCurThread->SchedItem);

    return K2STAT_NO_ERROR;
}

BOOL
KernIpcEnd_InIntr_SysCall_Load(
    K2OSKERN_OBJ_THREAD *   apCallingThread,
    K2OS_IPCEND_TOKEN       aTokIpcEnd
)
{
    K2STAT                  stat;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJREF         ipcEndRef;
    BOOL                    disp;

    pThreadPage = apCallingThread->mpKernRwViewOfUserThreadPage;

    ipcEndRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(apCallingThread->ProcRef.Ptr.AsProc, aTokIpcEnd, &ipcEndRef);
    if (K2STAT_IS_ERROR(stat))
    {
        pThreadPage->mLastStatus = stat;
//        K2OSKERN_Debug("Reserve failed on token translate\n");
        return FALSE;
    }

    do
    {
        if ((KernObj_IpcEnd != ipcEndRef.Ptr.AsHdr->mObjType) ||
            (ipcEndRef.Ptr.AsIpcEnd->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc != apCallingThread->ProcRef.Ptr.AsProc))
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
            break;
        }

        do
        {
            disp = K2OSKERN_SeqLock(&ipcEndRef.Ptr.AsIpcEnd->Connection.SeqLock);

            if ((ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mState == KernIpcEndState_Connected) &&
                (!(ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mInClose)))
            {
                if (ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.Connected.mIsLoaded)
                {
                    stat = K2STAT_ERROR_API_ORDER;
                }
                else
                {
                    if (!KernMailbox_Reserve(&ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.Connected.PartnerIpcEndRef.Ptr.AsIpcEnd->MailboxRef.Ptr.AsMailbox->Common, 1))
                    {
                        stat = K2STAT_ERROR_OUT_OF_RESOURCES;
                    }
                    else
                    {
                        ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.Connected.mIsLoaded = TRUE;
                    }
                }
            }
            else
            {
                stat = K2STAT_ERROR_NOT_CONNECTED;
            }

            K2OSKERN_SeqUnlock(&ipcEndRef.Ptr.AsIpcEnd->Connection.SeqLock, disp);

        } while (0);

    } while (0);

    KernObj_ReleaseRef(&ipcEndRef);

    if (K2STAT_IS_ERROR(stat))
    {
        pThreadPage->mLastStatus = stat;
        return FALSE;
    }

    return TRUE;
}

void
KernIpcEnd_SysCall_Close(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJREF         ipcEndRef;
    BOOL                    disp;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    ipcEndRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(apCurThread->ProcRef.Ptr.AsProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &ipcEndRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if ((KernObj_IpcEnd != ipcEndRef.Ptr.AsHdr->mObjType) ||
            (ipcEndRef.Ptr.AsIpcEnd->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc != apCurThread->ProcRef.Ptr.AsProc))
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            disp = K2OSKERN_SeqLock(&ipcEndRef.Ptr.AsIpcEnd->Connection.SeqLock);

            ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mInClose = TRUE;

            if (ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mState == KernIpcEndState_Connected)
            {
                apCurThread->SchedItem.mType = KernSchedItem_Thread_SysCall;
                apCurThread->SchedItem.Args.Ipc_Manual_Disconnect.mChainClose = TRUE;
                KernObj_CreateRef(&apCurThread->SchedItem.ObjRef, ipcEndRef.Ptr.AsHdr);
                KernTimer_GetHfTick(&apCurThread->SchedItem.mHfTick);
                KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
                KernSched_QueueItem(&apCurThread->SchedItem);
            }
            else
            {
                KernProc_TokenDestroy(apCurThread->ProcRef.Ptr.AsProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0);
                apCurThread->mSysCall_Result = TRUE;
            }

            K2OSKERN_SeqUnlock(&ipcEndRef.Ptr.AsIpcEnd->Connection.SeqLock, disp);
        }

        KernObj_ReleaseRef(&ipcEndRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        pThreadPage->mLastStatus = stat;
        apCurThread->mSysCall_Result = FALSE;
    }
}

void
KernIpcEnd_SysCall_ManualDisconnect(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJREF         ipcEndRef;
    BOOL                    disp;
    K2OSKERN_IPC_REQUEST *  pReq;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    ipcEndRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(apCurThread->ProcRef.Ptr.AsProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &ipcEndRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if ((KernObj_IpcEnd != ipcEndRef.Ptr.AsHdr->mObjType) ||
            (ipcEndRef.Ptr.AsIpcEnd->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc != apCurThread->ProcRef.Ptr.AsProc))
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            pReq = NULL;

            disp = K2OSKERN_SeqLock(&ipcEndRef.Ptr.AsIpcEnd->Connection.SeqLock);

            if ((ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mState == KernIpcEndState_Disconnected) &&
                (NULL != ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mpOpenRequest))
            {
                pReq = ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mpOpenRequest;
                ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mpOpenRequest = NULL;
                K2OSKERN_SeqLock(&gData.Iface.SeqLock);
                pReq->mpRequestor = NULL;
                K2TREE_Remove(&gData.Iface.Locked.RequestTree, &pReq->GlobalTreeNode);
                K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, FALSE);
            }
            else if ((ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mState == KernIpcEndState_Connected) &&
                (!(ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mInClose)))
            {
                apCurThread->SchedItem.mType = KernSchedItem_Thread_SysCall;
                apCurThread->SchedItem.Args.Ipc_Manual_Disconnect.mChainClose = FALSE;

                KernObj_CreateRef(&apCurThread->SchedItem.ObjRef, ipcEndRef.Ptr.AsHdr);
                KernTimer_GetHfTick(&apCurThread->SchedItem.mHfTick);
                KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
                KernSched_QueueItem(&apCurThread->SchedItem);
            }
            else
            {
                stat = K2STAT_ERROR_NOT_CONNECTED;
            }

            K2OSKERN_SeqUnlock(&ipcEndRef.Ptr.AsIpcEnd->Connection.SeqLock, disp);

            if (NULL != pReq)
            {
                K2MEM_Zero(pReq, sizeof(K2OSKERN_IPC_REQUEST));
                KernHeap_Free(pReq);
            }
        }

        KernObj_ReleaseRef(&ipcEndRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        pThreadPage->mLastStatus = stat;
        apCurThread->mSysCall_Result = FALSE;
    }
}

void
KernIpcEnd_CloseDpc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apArg
)
{
    K2OSKERN_OBJ_IPCEND *pIpcEnd;
    K2OSKERN_OBJ_THREAD *pCloserThread;

    pIpcEnd = K2_GET_CONTAINER(K2OSKERN_OBJ_IPCEND, apArg, Hdr.ObjDpc.Func);
    K2_ASSERT(pIpcEnd->Hdr.mObjType == KernObj_IpcEnd);
    K2_ASSERT(pIpcEnd->mHoldTokenOnDpcClose != NULL);

    KTRACE(apThisCore, 1, KTRACE_IPCEND_CLOSE_DPC);

    KernProc_TokenDestroy(pIpcEnd->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc, pIpcEnd->mHoldTokenOnDpcClose);

    pCloserThread = pIpcEnd->CloserThreadRef.Ptr.AsThread;
    if (NULL != pCloserThread)
    {
        //
        // we must be inside the scheduler on this core
        //
        K2_ASSERT(apThisCore == gData.Sched.mpSchedulingCore);
        K2_ASSERT(pCloserThread->mState == KernThreadState_InScheduler_ResumeDeferred);
        pCloserThread->SchedItem.mType = KernSchedItem_Thread_ResumeDeferral_Completed;
        KernObj_CreateRef(&pCloserThread->SchedItem.ObjRef, &pCloserThread->Hdr);
        KernTimer_GetHfTick(&pCloserThread->SchedItem.mHfTick);
        KernSched_QueueItem(&pCloserThread->SchedItem);
        KernObj_ReleaseRef(&pIpcEnd->CloserThreadRef);
    }
}

void
KernIpcEnd_RecvRes(
    K2OSKERN_OBJ_IPCEND *   apIpcEnd,
    BOOL                    aDelivered
)
{
    BOOL disp;

    //
    // dont care if it was delivered or not
    //
//    K2OSKERN_Debug("IpcEnd-DisconnectAck due to RecvRes(%d)\n", aDelivered);
    disp = K2OSKERN_SeqLock(&apIpcEnd->Connection.SeqLock);
    if (apIpcEnd->Connection.Locked.mState == KernIpcEndState_WaitDisAck)
    {
        KernIpcEnd_ConnLocked_DisconnectAck(apIpcEnd);
    }
    K2OSKERN_SeqUnlock(&apIpcEnd->Connection.SeqLock, disp);
}

void
KernIpcEnd_SysCall_Request(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJREF         ipcEndRef;
    K2OSKERN_IPC_REQUEST *  pReq;
    BOOL                    disp;
    K2OS_MSG                msg;
    K2OSKERN_IFACE *        pIface;
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_OBJREF         mailboxRef;
    BOOL                    firstReq;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    firstReq = FALSE;

    ipcEndRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(apCurThread->ProcRef.Ptr.AsProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &ipcEndRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if ((KernObj_IpcEnd != ipcEndRef.Ptr.AsHdr->mObjType) ||
            (ipcEndRef.Ptr.AsIpcEnd->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc != apCurThread->ProcRef.Ptr.AsProc))
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            pReq = (K2OSKERN_IPC_REQUEST *)KernHeap_Alloc(sizeof(K2OSKERN_IPC_REQUEST));
            if (NULL == pReq)
            {
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
            }
            else
            {
                msg.mControl = K2OS_SYSTEM_MSG_IPC_REQUEST;
                msg.mPayload[0] = 0;    // will be service interface register context when service is found

                K2MEM_Zero(pReq, sizeof(K2OSKERN_IPC_REQUEST));
                pReq->mpRequestor = ipcEndRef.Ptr.AsIpcEnd;
                msg.mPayload[1] = pReq->mpRequestor->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc->mId;
                disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
                pReq->GlobalTreeNode.mUserVal = gData.Iface.Locked.mNextRequestId++;
                K2TREE_Insert(&gData.Iface.Locked.RequestTree, pReq->GlobalTreeNode.mUserVal, &pReq->GlobalTreeNode);
                K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

                disp = K2OSKERN_SeqLock(&ipcEndRef.Ptr.AsIpcEnd->Connection.SeqLock);

                if (ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mState != KernIpcEndState_Disconnected)
                {
                    stat = K2STAT_ERROR_CONNECTED;
                }
                else
                {
                    if (NULL != ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mpOpenRequest)
                    {
                        if (ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mpOpenRequest->mIfaceInstanceId != pReq->mIfaceInstanceId)
                        {
                            //
                            // already an open request for another service on this endpoint
                            //
                            stat = K2STAT_ERROR_API_ORDER;
                        }
                    }
                    else
                    {
                        ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mpOpenRequest = pReq;
                        pReq = NULL;
                    }

                    // this happens even in error case (API_ORDER) but is harmless as the message will be discarded
                    // before it is sent
                    msg.mPayload[2] = ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mpOpenRequest->GlobalTreeNode.mUserVal;
                }

                K2OSKERN_SeqUnlock(&ipcEndRef.Ptr.AsIpcEnd->Connection.SeqLock, disp);

                if (NULL != pReq)
                {
                    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
                    K2TREE_Remove(&gData.Iface.Locked.RequestTree, &pReq->GlobalTreeNode);
                    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);
                    KernHeap_Free(pReq);
                }
                else
                {
                    firstReq = TRUE;
                }

                if (!K2STAT_IS_ERROR(stat))
                {
                    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

                    pTreeNode = K2TREE_Find(&gData.Iface.Locked.InstanceTree, pThreadPage->mSysCall_Arg1);
                    if (NULL != pTreeNode)
                    {
                        pIface = K2_GET_CONTAINER(K2OSKERN_IFACE, pTreeNode, GlobalTreeNode);

                        if (pIface->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc == apCurThread->ProcRef.Ptr.AsProc)
                        {
                            // process is not allowed to IPC to itself
                            stat = K2STAT_ERROR_NOT_ALLOWED;
                        }
                        else if ((pIface->mRequestorProcId != 0) && 
                                 (pIface->mRequestorProcId != apCurThread->ProcRef.Ptr.AsProc->mId))
                        {
                            // interface has a requestor process locked down
                            // and the requestor process is the wrong one
                            // so we act like the interface doesn't exist
                            stat = K2STAT_ERROR_NOT_FOUND;
                        }
                        else
                        {
                            // try to send connect request message to service's Mailbox
                            msg.mPayload[0] = (UINT_PTR)pIface->mContext;
                            mailboxRef.Ptr.AsHdr = NULL;
                            KernObj_CreateRef(&mailboxRef, pIface->MailboxRef.Ptr.AsHdr);
                        }
                    }
                    else
                    {
                        stat = K2STAT_ERROR_NOT_FOUND;
                    }

                    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

                    if (!K2STAT_IS_ERROR(stat))
                    {
                        // 
                        // mailbox deliver with a thread object as the sender will
                        // result in this system call going into the scheduler
                        //
                        stat = KernMailbox_Deliver(apThisCore, &mailboxRef.Ptr.AsMailbox->Common, &msg, &apCurThread->Hdr, TRUE, FALSE, NULL);
                        KernObj_ReleaseRef(&mailboxRef);
                        if (!K2STAT_IS_ERROR(stat))
                        {
                            apCurThread->mSysCall_Result = TRUE;
                        }
                    }

                    if ((K2STAT_IS_ERROR(stat)) && (firstReq))
                    {
                        //
                        // first request went onto endpoint but then failed - take open request off of ipc endpoint
                        //
                        disp = K2OSKERN_SeqLock(&ipcEndRef.Ptr.AsIpcEnd->Connection.SeqLock);
                        pReq = ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mpOpenRequest;
                        if (NULL != pReq)
                        {
                            K2OSKERN_SeqLock(&gData.Iface.SeqLock);
                            ipcEndRef.Ptr.AsIpcEnd->Connection.Locked.mpOpenRequest = NULL;
                            pReq->mpRequestor = NULL;
                            K2TREE_Remove(&gData.Iface.Locked.RequestTree, &pReq->GlobalTreeNode);
                            K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, FALSE);
                            KernHeap_Free(pReq);
                        }
                        K2OSKERN_SeqUnlock(&ipcEndRef.Ptr.AsIpcEnd->Connection.SeqLock, disp);
                    }
                }
            }
        }

        KernObj_ReleaseRef(&ipcEndRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        pThreadPage->mLastStatus = stat;
        apCurThread->mSysCall_Result = FALSE;
    }
}

void
KernIpcEnd_SysCall_Accept(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJ_PROCESS *  pLocalProc;
    K2OSKERN_OBJREF         ipcLocalEndRef;
    K2OSKERN_OBJREF         ipcRemoteEndRef;
    BOOL                    disp;
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_IPC_REQUEST *  pReq;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;
    pLocalProc = apCurThread->ProcRef.Ptr.AsProc;

    ipcLocalEndRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(pLocalProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &ipcLocalEndRef);
    if (!K2STAT_IS_ERROR(stat))
    {
        if ((KernObj_IpcEnd != ipcLocalEndRef.Ptr.AsHdr->mObjType) ||
            (pLocalProc != ipcLocalEndRef.Ptr.AsIpcEnd->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc))
        {
//            K2OSKERN_Debug("accept - token not an endpoint or not a local endtpoint\n");
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            disp = K2OSKERN_SeqLock(&ipcLocalEndRef.Ptr.AsIpcEnd->Connection.SeqLock);
            if (ipcLocalEndRef.Ptr.AsIpcEnd->Connection.Locked.mState != KernIpcEndState_Disconnected)
            {
                stat = K2STAT_ERROR_CONNECTED;
            }
            else if (NULL != ipcLocalEndRef.Ptr.AsIpcEnd->Connection.Locked.mpOpenRequest)
            {
                stat = K2STAT_ERROR_API_ORDER;
            }
            K2OSKERN_SeqUnlock(&ipcLocalEndRef.Ptr.AsIpcEnd->Connection.SeqLock, disp);
            if (!K2STAT_IS_ERROR(stat))
            {
                //
                // get ref to target request 
                //
                disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);
                pTreeNode = K2TREE_Find(&gData.Iface.Locked.RequestTree, pThreadPage->mSysCall_Arg1);
                if (NULL == pTreeNode)
                {
                    stat = K2STAT_ERROR_NOT_FOUND;
                }
                else
                {
                    pReq = K2_GET_CONTAINER(K2OSKERN_IPC_REQUEST, pTreeNode, GlobalTreeNode);
                    K2_ASSERT(pReq->mpRequestor != NULL);
                    ipcRemoteEndRef.Ptr.AsHdr = NULL;
                    KernObj_CreateRef(&ipcRemoteEndRef, &pReq->mpRequestor->Hdr);
                    K2_ASSERT(ipcRemoteEndRef.Ptr.AsIpcEnd->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc != pLocalProc);
                    pReq->mPending = (UINT_PTR)ipcLocalEndRef.Ptr.AsHdr;
                }
                K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

                if (!K2STAT_IS_ERROR(stat))
                {
                    disp = K2OSKERN_SeqLock(&ipcRemoteEndRef.Ptr.AsIpcEnd->Connection.SeqLock);
                    if (ipcRemoteEndRef.Ptr.AsIpcEnd->Connection.Locked.mState != KernIpcEndState_Disconnected)
                    {
                        stat = K2STAT_ERROR_CONNECTED;
                    }
                    else if (NULL == ipcRemoteEndRef.Ptr.AsIpcEnd->Connection.Locked.mpOpenRequest)
                    {
                        // connection request vanished between global lock above and connection lock here
                        stat = K2STAT_ERROR_ABANDONED;
                    }
                    K2OSKERN_SeqUnlock(&ipcRemoteEndRef.Ptr.AsIpcEnd->Connection.SeqLock, disp);
                    if (!K2STAT_IS_ERROR(stat))
                    {
                        stat = KernIpcEnd_TryAccept(
                            apThisCore,
                            apCurThread,
                            ipcLocalEndRef.Ptr.AsIpcEnd,
                            ipcRemoteEndRef.Ptr.AsIpcEnd
                        );
                    }
                    KernObj_ReleaseRef(&ipcRemoteEndRef);
                }
            }
        }
        KernObj_ReleaseRef(&ipcLocalEndRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = FALSE;
        pThreadPage->mLastStatus = stat;
    }
}

void 
KernIpcEnd_SysCall_RejectRequest(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    BOOL                    disp;
    K2TREE_NODE *           pTreeNode;
    K2OSKERN_IPC_REQUEST *  pReq;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJREF         ipcRemoteEndRef;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    disp = K2OSKERN_SeqLock(&gData.Iface.SeqLock);

    pTreeNode = K2TREE_Find(&gData.Iface.Locked.RequestTree, pThreadPage->mSysCall_Arg1);
    if (NULL == pTreeNode)
    {
        stat = K2STAT_ERROR_NOT_FOUND;
    }
    else
    {
        pReq = K2_GET_CONTAINER(K2OSKERN_IPC_REQUEST, pTreeNode, GlobalTreeNode);
        K2_ASSERT(pReq->mpRequestor != NULL);
        ipcRemoteEndRef.Ptr.AsHdr = NULL;
        KernObj_CreateRef(&ipcRemoteEndRef, &pReq->mpRequestor->Hdr);
        K2_ASSERT(ipcRemoteEndRef.Ptr.AsIpcEnd->MailboxRef.Ptr.AsMailbox->Common.ProcRef.Ptr.AsProc != apCurThread->ProcRef.Ptr.AsProc);
        pReq->mPending = (UINT_PTR)0xFFFFFFFF;
    }

    K2OSKERN_SeqUnlock(&gData.Iface.SeqLock, disp);

    if (!K2STAT_IS_ERROR(stat))
    {
        disp = K2OSKERN_SeqLock(&ipcRemoteEndRef.Ptr.AsIpcEnd->Connection.SeqLock);

        if (ipcRemoteEndRef.Ptr.AsIpcEnd->Connection.Locked.mState != KernIpcEndState_Disconnected)
        {
            stat = K2STAT_ERROR_CONNECTED;
        }
        else if (NULL == ipcRemoteEndRef.Ptr.AsIpcEnd->Connection.Locked.mpOpenRequest)
        {
            // connection request vanished between global lock above and connection lock here
            stat = K2STAT_ERROR_ABANDONED;
        }

        K2OSKERN_SeqUnlock(&ipcRemoteEndRef.Ptr.AsIpcEnd->Connection.SeqLock, disp);

        if (!K2STAT_IS_ERROR(stat))
        {
            KernObj_CreateRef(&apCurThread->SchedItem.ObjRef, ipcRemoteEndRef.Ptr.AsHdr);
        }

        KernObj_ReleaseRef(&ipcRemoteEndRef);
    }

    if (!K2STAT_IS_ERROR(stat))
    {
        apCurThread->SchedItem.mType = KernSchedItem_Thread_SysCall;
        apCurThread->SchedItem.Args.Ipc_Reject.mRequestId = apCurThread->mSysCall_Arg0;
        apCurThread->SchedItem.Args.Ipc_Reject.mReasonCode = pThreadPage->mSysCall_Arg1;
        KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
        KernTimer_GetHfTick(&apCurThread->SchedItem.mHfTick);
        KernSched_QueueItem(&apCurThread->SchedItem);
    }
    else
    {
        apCurThread->mSysCall_Result = FALSE;
        pThreadPage->mLastStatus = stat;
    }
}

