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
KernMailbox_DeliverInternal(
    K2OSKERN_MBOX_COMMON *  apMailboxCommon,
    K2OS_MSG const *        apMsg,
    BOOL                    aDoAvailAlloc,
    BOOL                    aSetReserveBit,
    K2OSKERN_OBJ_HEADER *   apReserveUser,
    BOOL *                  apRetDoSignal
)
{
    K2OS_MAILBOX_OWNER *    pMailboxOwner;
    UINT32                  slotIndex;
    UINT32                  exchangeVal;
    UINT32                  valResult;
    UINT32                  wordIndex;
    UINT32                  bitIndex;
    UINT32                  ownerMask;

    *apRetDoSignal = FALSE;

    if (apMailboxCommon->SchedOnlyCanSet.mIsClosed)
    {
        return K2STAT_ERROR_CLOSED;
    }

    K2_ASSERT((NULL == apReserveUser) || (aSetReserveBit));

    pMailboxOwner = apMailboxCommon->KernelSpace.mpMailboxOwner;

    if (aDoAvailAlloc)
    {
        K2_ASSERT(FALSE == aSetReserveBit);

        //
        // try to allocate a slot to send from
        //
        do
        {
            ownerMask = pMailboxOwner->mAvail;
            if (0 == ownerMask)
                return K2STAT_ERROR_FULL;
        } while (ownerMask != K2ATOMIC_CompareExchange(&pMailboxOwner->mAvail, ownerMask - 1, ownerMask));
    }

    //
    // atomically increment producer count
    //
    do
    {
        slotIndex = apMailboxCommon->KernelSpace.mIxProducer;
        K2_CpuReadBarrier();

        wordIndex = slotIndex >> 5;
        bitIndex = slotIndex & 0x1F;
        // only reason for failure is somebody sending using a reserve they don't have.
        K2_ASSERT(0 == (pMailboxOwner->mFlagBitArray[wordIndex] & (1 << bitIndex)));

        exchangeVal = (slotIndex + 1) & 0xFF;
        valResult = K2ATOMIC_CompareExchange(&apMailboxCommon->KernelSpace.mIxProducer, exchangeVal, slotIndex);
    } while (valResult != slotIndex);

    //
    // we have the target slot number.  do the copy now
    //
    K2MEM_Copy((void *)(apMailboxCommon->KernelSpace.mVirtBase + (slotIndex * sizeof(K2OS_MSG))), apMsg, sizeof(K2OS_MSG));
    K2_CpuFullBarrier();

    //
    // set or clear the reserve marker bit
    //
    do
    {
        ownerMask = pMailboxOwner->mFlagBitArray[wordIndex + K2OS_MAILBOX_OWNER_FLAG_DWORDS];
        K2_CpuReadBarrier();
        if (aSetReserveBit)
            exchangeVal = ownerMask | (1 << bitIndex);
        else
            exchangeVal = ownerMask & ~(1 << bitIndex);
        if (exchangeVal == ownerMask)
            break;
        valResult = K2ATOMIC_CompareExchange(&pMailboxOwner->mFlagBitArray[wordIndex + K2OS_MAILBOX_OWNER_FLAG_DWORDS], exchangeVal, ownerMask);
    } while (valResult != ownerMask);

    //
    // if a reserve user wants to be notified when the reserve is available to be used
    // again, then store a reference to that 
    //
    if ((aSetReserveBit) && (NULL != apReserveUser))
    {
        KernObj_CreateRef(&apMailboxCommon->KernelSpace.ReserveUser[slotIndex], apReserveUser);
    }

    //
    // set the slot to 'full' status now
    //
    do
    {
        ownerMask = pMailboxOwner->mFlagBitArray[wordIndex];
        K2_CpuReadBarrier();
        exchangeVal = ownerMask | (1 << bitIndex);
        valResult = K2ATOMIC_CompareExchange(&pMailboxOwner->mFlagBitArray[wordIndex], exchangeVal, ownerMask);
    } while (valResult != ownerMask);
//    K2OSKERN_Debug("%d - Delivered %08X to index %08X\n", apMailboxCommon->ProcRef.Ptr.AsProc->mId, apMsg->mControl, slotIndex);

    //
    // if the consumer is not active we return that it should be signalled to wake it up
    //
    valResult = pMailboxOwner->mIxConsumer;
    K2_CpuReadBarrier();
    if (0 == (valResult & 0x80000000))
    {
        //
        // consumer is not active so if the consumer next recv index is the same as the slot we just
        // produced into, we need to wake it up by signalling the notify
        //
        if ((valResult & 0xFF) == slotIndex)
        {
            //
            // this may wake a receiver if one is waiting
            //
            *apRetDoSignal = TRUE;
        }
    }

    return K2STAT_NO_ERROR;
}

K2STAT
KernMailbox_Deliver(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_MBOX_COMMON *      apMailboxCommon,
    K2OS_MSG const *            apMsg,
    K2OSKERN_OBJ_HEADER *       apFromObj,
    BOOL                        aDoAvailAlloc,
    BOOL                        aSetReserveBit,
    K2OSKERN_OBJ_HEADER *       apReserveUser
)
{
    K2STAT                      stat;
    BOOL                        doSignal;
    K2OSKERN_SCHED_ITEM *       pSchedItem;
    K2OSKERN_OBJ_THREAD *       pFromThread;

    K2_ASSERT(NULL != apFromObj);

    if (apMailboxCommon->SchedOnlyCanSet.mIsClosed)
    {
        return K2STAT_ERROR_CLOSED;
    }

    stat = KernMailbox_DeliverInternal(apMailboxCommon, apMsg, aDoAvailAlloc, aSetReserveBit, apReserveUser, &doSignal);

    if ((!K2STAT_IS_ERROR(stat)) && (doSignal))
    {
        //
        // this may wake a receiver if one is waiting
        //
        if (apFromObj->mObjType == KernObj_Thread)
        {
            //
            // this is done through scheduler call because
            // unblocking a thread may cause 'fromthread' to
            // never execute another instruction.  so this
            // thread has to not exec again until after the notify is processed
            //
            pFromThread = (K2OSKERN_OBJ_THREAD *)apFromObj;
            K2_ASSERT(pFromThread->mIsInSysCall);
            K2_ASSERT(apThisCore->mpActiveThread == pFromThread);
            pSchedItem = &pFromThread->SchedItem;
            pSchedItem->mType = KernSchedItem_Thread_SysCall;
            KernTimer_GetHfTick(&pSchedItem->mHfTick);
            KernObj_CreateRef(&pSchedItem->ObjRef, apMailboxCommon->NotifyRef.Ptr.AsHdr);
            KernCpu_TakeCurThreadOffThisCore(apThisCore, pFromThread, KernThreadState_InScheduler);
            KernSched_QueueItem(pSchedItem);
        }
        else if (apFromObj->mObjType == KernObj_SignalProxy)
        {
            KernSignalProxy_Fire((K2OSKERN_OBJ_SIGNALPROXY *)apFromObj, apMailboxCommon->NotifyRef.Ptr.AsNotify);
        }
        else
        {
            K2OSKERN_Panic("Unknown object type %d sending to mailbox\n", apFromObj->mObjType);
        }
    }

    return stat;
}

void    
KernMailslot_SysCall_Send(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJREF         mailslotRef;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    mailslotRef.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(
        apCurThread->ProcRef.Ptr.AsProc,
        (K2OS_TOKEN)apCurThread->mSysCall_Arg0,
        &mailslotRef
    );
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_Mailslot == mailslotRef.Ptr.AsHdr->mObjType)
        {
            // 
            // mailbox deliver with a thread object as the sender will
            // result in this system call going into the scheduler
            //
            stat = KernMailbox_Deliver(
                apThisCore,
                &mailslotRef.Ptr.AsMailslot->RefMailbox.Ptr.AsMailbox->Common,
                (K2OS_MSG const *)&pThreadPage->mSysCall_Arg1,
                &apCurThread->Hdr,
                TRUE, FALSE, NULL);  // do alloc (raw syscall), set reserve bit ignored
            if (!K2STAT_IS_ERROR(stat))
            {
                apCurThread->mSysCall_Result = TRUE;
            }
        }
        else
        {
//            K2OSKERN_Debug("MailslotSend - token does not refer to a mailslot\n");
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        KernObj_ReleaseRef(&mailslotRef);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = FALSE;
        pThreadPage->mLastStatus = stat;
    }
}

void
KernMailbox_SysCall_RecvRes(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJREF         refMailbox;
    K2OSKERN_MBOX_COMMON *  pMailboxCommon;
    UINT32                  slotIx;
    UINT32                  wordIndex;
    UINT32                  bitIndex;
    K2OS_MAILBOX_OWNER *    pMailboxOwner;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    K2OSKERN_OBJ_HEADER *   pRefObj;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    refMailbox.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(apCurThread->ProcRef.Ptr.AsProc, (K2OS_TOKEN)apCurThread->mSysCall_Arg0, &refMailbox);
    if (!K2STAT_IS_ERROR(stat))
    {
        do
        {
            // check for bad arg. use full call to return error
            if (refMailbox.Ptr.AsHdr->mObjType != KernObj_Mailbox)
            {
                stat = K2STAT_ERROR_BAD_TOKEN;
                break;
            }

            pMailboxCommon = &refMailbox.Ptr.AsMailbox->Common;

            slotIx = pThreadPage->mSysCall_Arg1;

            if (slotIx >= 0x100)
            {
                stat = K2STAT_ERROR_BAD_ARGUMENT;
                break;
            }

            wordIndex = (slotIx & 0xFF) >> 5;
            bitIndex = slotIx & 0x1F;
            pMailboxOwner = pMailboxCommon->KernelSpace.mpMailboxOwner;

            // slot must be owned by consumer.  use full call to return error
            if (0 == (pMailboxOwner->mFlagBitArray[wordIndex] & (1 << bitIndex)))
            {
                stat = K2STAT_ERROR_BAD_ARGUMENT;
                break;
            }

            if (0 == (pMailboxOwner->mFlagBitArray[wordIndex + K2OS_MAILBOX_OWNER_FLAG_DWORDS] & (1 << bitIndex)))
            {
                stat = K2STAT_ERROR_BAD_ARGUMENT;
                break;
            }

            pRefObj = pMailboxCommon->KernelSpace.ReserveUser[slotIx].Ptr.AsHdr;

            // we should not be here if this is NULL
            K2_ASSERT(NULL != pRefObj);

            //
            // set the slot to 'empty' status so producer can produce into it again
            //
            K2ATOMIC_And(&pMailboxOwner->mFlagBitArray[wordIndex], ~(1 << bitIndex));

            //
            // tell the producer its reserve is available for use again
            //
            switch (pRefObj->mObjType)
            {
            case KernObj_IpcEnd:
                KernIpcEnd_RecvRes((K2OSKERN_OBJ_IPCEND *)pRefObj, TRUE);
                break;

            case KernObj_IfSubs:
                K2ATOMIC_Inc(&((K2OSKERN_OBJ_IFSUBS *)pRefObj)->mBacklogRemaining);
                break;
                
            default:
                K2OSKERN_Panic("Unknown referring object type %d used reserved mailbox slot\n", pRefObj->mObjType);
                break;
            }

            KernObj_ReleaseRef(&pMailboxCommon->KernelSpace.ReserveUser[slotIx]);

        } while (0);

        KernObj_ReleaseRef(&refMailbox);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

BOOL
KernMailbox_InIntr_SysCall_RecvRes(
    K2OSKERN_OBJ_THREAD *   apCallingThread,
    K2OS_MAILBOX_TOKEN      aTokMailbox,
    UINT32                  aSlotIx
)
{
    K2STAT                  stat;
    K2OSKERN_OBJREF         refMailbox;
    BOOL                    result;
    UINT32                  wordIndex;
    UINT32                  bitIndex;
    K2OSKERN_MBOX_COMMON *  pMailboxCommon;
    K2OS_MAILBOX_OWNER *    pMailboxOwner;

    refMailbox.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(apCallingThread->ProcRef.Ptr.AsProc, aTokMailbox, &refMailbox);
    if (K2STAT_IS_ERROR(stat))
        return FALSE;

    result = FALSE;

    do
    {
        // check for bad arg. use full call to return error
        if (refMailbox.Ptr.AsHdr->mObjType != KernObj_Mailbox)
        {
            break;
        }

        // check for bad arg. use full call to return error
        if (aSlotIx >= 0x100)
            break;

        pMailboxCommon = &refMailbox.Ptr.AsMailbox->Common;

        wordIndex = (aSlotIx & 0xFF) >> 5;
        bitIndex = aSlotIx & 0x1F;

        pMailboxOwner = pMailboxCommon->KernelSpace.mpMailboxOwner;

        // slot must be owned by consumer.  use full call to return error
        if (0 == (pMailboxOwner->mFlagBitArray[wordIndex] & (1 << bitIndex)))
            break;

        // slot must be a reserve slot.  use full call to return error
        if (0 == (pMailboxOwner->mFlagBitArray[wordIndex + K2OS_MAILBOX_OWNER_FLAG_DWORDS] & (1 << bitIndex)))
            break;

        // if slot has an observer then the full call is made
        if (NULL != pMailboxCommon->KernelSpace.ReserveUser[aSlotIx].Ptr.AsHdr)
            break;

        //
        // reserve consumed but nobody cares.  don't bother with the full
        // syscall and just return success
        //
        result = TRUE;

        //
        // set the slot to 'empty' status so producer can produce into it again
        //
        K2ATOMIC_And(&pMailboxOwner->mFlagBitArray[wordIndex], ~(1 << bitIndex));

    } while (0);

    KernObj_ReleaseRef(&refMailbox);

    return result;
}

BOOL
KernMailbox_Reserve(
    K2OSKERN_MBOX_COMMON *  apMailboxCommon,
    UINT32                  aCount
)
{
    UINT32 volatile *   pAvail;
    UINT32              v;

    if (apMailboxCommon->SchedOnlyCanSet.mIsClosed)
        return FALSE;

    pAvail = &apMailboxCommon->KernelSpace.mpMailboxOwner->mAvail;
    do
    {
        v = *pAvail;

        if (v < aCount)
            return FALSE;

    } while (v != K2ATOMIC_CompareExchange(pAvail, v - aCount, v));

    return TRUE;
}

void
KernMailbox_UndoReserve(
    K2OSKERN_MBOX_COMMON *  apMailboxCommon,
    UINT32                  aCount
)
{
    if ((0 == aCount) ||
        (apMailboxCommon->SchedOnlyCanSet.mIsClosed))
        return;

    K2ATOMIC_Add((INT32 volatile *)&apMailboxCommon->KernelSpace.mpMailboxOwner->mAvail, aCount);
}

void
KernMailbox_SysCall_Close(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJREF         refMailbox;
    K2OSKERN_SCHED_ITEM *   pSchedItem;

    refMailbox.Ptr.AsHdr = NULL;
    stat = KernProc_TokenTranslate(
        apCurThread->ProcRef.Ptr.AsProc,
        (K2OS_TOKEN)apCurThread->mSysCall_Arg0,
        &refMailbox
    );
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_Mailbox != refMailbox.Ptr.AsHdr->mObjType)
        {
//            K2OSKERN_Debug("MailboxClose - token does not refer to a mailbox\n");
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            if (refMailbox.Ptr.AsMailbox->Common.SchedOnlyCanSet.mIsClosed)
            {
                stat = K2STAT_ERROR_CLOSED;
            }
            else
            {
                pSchedItem = &apCurThread->SchedItem;
                pSchedItem->mType = KernSchedItem_Thread_SysCall;
                KernTimer_GetHfTick(&pSchedItem->mHfTick);
                KernObj_CreateRef(&pSchedItem->ObjRef, refMailbox.Ptr.AsHdr);
                KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
                KernSched_QueueItem(pSchedItem);
            }
        }
        KernObj_ReleaseRef(&refMailbox);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfUserThreadPage->mLastStatus = stat;
    }
}

K2STAT
KernMailbox_Create(
    K2OSKERN_OBJ_PROCESS *      apProc,
    UINT32                      aUserMailboxOwnerAddr,
    K2OSKERN_OBJREF *           apRetMailboxRef
)
{
    K2STAT                      stat;
    K2OSKERN_PROCHEAP_NODE *    pProcHeapNode;
    UINT32                      mailboxUserPageCount;
    K2OSKERN_OBJREF             mapRef;
    UINT32                      mapPageIx;
    K2OSKERN_OBJREF             pageArrayRef;
    K2OSKERN_OBJREF             notifyRef;
    UINT32                      kernVirtAddr;
    K2OSKERN_OBJ_MAILBOX *      pMailbox;
    BOOL                        disp;
    K2OSKERN_OBJ_MAILBOX *      pOtherMailbox;
    K2LIST_LINK *               pListLink;

    pOtherMailbox = NULL;
    disp = K2OSKERN_SeqLock(&apProc->Mail.SeqLock);
    pListLink = apProc->Mail.Locked.MailboxList.mpHead;
    if (NULL != pListLink)
    {
        do
        {
            pOtherMailbox = K2_GET_CONTAINER(K2OSKERN_OBJ_MAILBOX, pListLink, Common.ProcMailboxListLink);
            pListLink = pListLink->mpNext;

            if (pOtherMailbox->UserSpace.mRawAddr <= aUserMailboxOwnerAddr)
            {
                if ((aUserMailboxOwnerAddr - pOtherMailbox->UserSpace.mRawAddr) < sizeof(K2OS_MAILBOX_OWNER))
                {
                    // new mailbox owner structure starts partway into an existing mailbox owner structure
                    break;
                }
            }
            else
            {
                if ((pOtherMailbox->UserSpace.mRawAddr - aUserMailboxOwnerAddr) < sizeof(K2OS_MAILBOX_OWNER))
                {
                    // new mailbox owner structure extends partway into an existing mailbox owner structure
                    break;
                }
            }

            pOtherMailbox = NULL;

        } while (NULL != pListLink);
    }

    K2OSKERN_SeqUnlock(&apProc->Mail.SeqLock, disp);

    if (NULL != pOtherMailbox)
    {
        // address of new mailbox owner structure overlaps an existing mailbox owner structure
        return K2STAT_ERROR_ALREADY_MAPPED;
    }

    mapRef.Ptr.AsHdr = NULL;
    stat = KernProc_FindCreateMapRef(apProc, aUserMailboxOwnerAddr, &mapRef, &mapPageIx);
    if (K2STAT_IS_ERROR(stat))
    {
        return stat;
    }

    do
    {
        if (((aUserMailboxOwnerAddr + (sizeof(K2OS_MAILBOX_OWNER) - 1)) & K2_VA_PAGEFRAME_MASK) ==
            (aUserMailboxOwnerAddr & K2_VA_PAGEFRAME_MASK))
            mailboxUserPageCount = 0;
        else
            mailboxUserPageCount = 1;
        mailboxUserPageCount++;

        if ((mapPageIx + mailboxUserPageCount) > mapRef.Ptr.AsMap->mPageCount)
        {
            //
            // blows past end of map
            //
            stat = K2STAT_ERROR_BAD_ARGUMENT;
            break;
        }

        if (mapRef.Ptr.AsMap->mUserMapType != K2OS_MapType_Data_ReadWrite)
        {
            //
            // trying to use a mailbox owner structure not in R/W data mapping
            //
            stat = K2STAT_ERROR_NOT_ALLOWED;
            break;
        }

        stat = KernProc_UserVirtHeapAlloc(
            apProc,
            1,
            &pProcHeapNode);
        if (K2STAT_IS_ERROR(stat))
        {
            break;
        }

        do
        {
            pageArrayRef.Ptr.AsHdr = NULL;
            stat = KernPageArray_CreateSparse(1, K2OS_MEMPAGE_ATTR_READABLE, &pageArrayRef);
            if (K2STAT_IS_ERROR(stat))
            {
                break;
            }

            do
            {
                notifyRef.Ptr.AsHdr = NULL;
                stat = KernNotify_Create(FALSE, &notifyRef);
                if (K2STAT_IS_ERROR(stat))
                {
                    break;
                }

                do
                {
                    kernVirtAddr = KernVirt_AllocPages(1 + mailboxUserPageCount);
                    if (0 == kernVirtAddr)
                    {
                        stat = K2STAT_ERROR_OUT_OF_MEMORY;
                        break;
                    }

                    do
                    {
                        pMailbox = (K2OSKERN_OBJ_MAILBOX *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_MAILBOX));
                        if (NULL == pMailbox)
                        {
                            stat = K2STAT_ERROR_OUT_OF_MEMORY;
                            break;
                        }

                        do
                        {
                            K2MEM_Zero(pMailbox, sizeof(K2OSKERN_OBJ_MAILBOX));

                            pMailbox->Hdr.mObjType = KernObj_Mailbox;
                            K2LIST_Init(&pMailbox->Hdr.RefObjList);

                            stat = KernMap_Create(apProc,
                                pageArrayRef.Ptr.AsPageArray,
                                0,
                                K2HEAP_NodeAddr(&pProcHeapNode->HeapNode),
                                1,
                                K2OS_MapType_Data_ReadOnly,
                                &pMailbox->UserSpace.RoDataPageMapRef
                            );

                            if (K2STAT_IS_ERROR(stat))
                            {
                                break;
                            }

                            //
                            // cannot fail from this point on
                            //
                            stat = K2STAT_NO_ERROR;
                            pProcHeapNode->mUserOwned = 0;

                            KernObj_CreateRef(&pMailbox->Common.ProcRef, &apProc->Hdr);
                            KernObj_CreateRef(&pMailbox->DataPageArrayRef, pageArrayRef.Ptr.AsHdr);
                            KernObj_CreateRef(&pMailbox->Common.NotifyRef, notifyRef.Ptr.AsHdr);

                            pMailbox->KernelSpace.mClientPageCount = mailboxUserPageCount;

                            pMailbox->Common.KernelSpace.mVirtBase = kernVirtAddr;
                            pMailbox->Common.KernelSpace.mpMailboxOwner = (K2OS_MAILBOX_OWNER *)
                                (kernVirtAddr + K2_VA_MEMPAGE_BYTES + (aUserMailboxOwnerAddr & K2_VA_MEMPAGE_OFFSET_MASK));

                            pMailbox->Common.KernelSpace.mIxProducer = 0;

                            pMailbox->UserSpace.mRawAddr = aUserMailboxOwnerAddr;

                            KernObj_CreateRef(&pMailbox->UserSpace.RawMapRef, mapRef.Ptr.AsHdr);

                            //pMailbox->UserSpace.mpMap_RoDataPage already set above in KernMap_Create call
                            K2_CpuWriteBarrier();

                            //
                            // map the data page into the kernel R/W
                            //
                            KernPte_MakePageMap(NULL,
                                kernVirtAddr,
                                KernPageArray_PagePhys(pageArrayRef.Ptr.AsPageArray, 0),
                                K2OS_MAPTYPE_KERN_DATA);

                            //
                            // map the user mailbox owner structure first page into the kernel R/W
                            //
                            KernPte_MakePageMap(NULL,
                                kernVirtAddr + K2_VA_MEMPAGE_BYTES,
                                KernPageArray_PagePhys(mapRef.Ptr.AsMap->PageArrayRef.Ptr.AsPageArray, mapRef.Ptr.AsMap->mPageArrayStartPageIx + mapPageIx),
                                K2OS_MAPTYPE_KERN_DATA);

                            if (mailboxUserPageCount > 1)
                            {
                                //
                                // map the user mailbox owner structure second page into the kernel R/W
                                //
                                KernPte_MakePageMap(NULL,
                                    kernVirtAddr + (K2_VA_MEMPAGE_BYTES * 2),
                                    KernPageArray_PagePhys(mapRef.Ptr.AsMap->PageArrayRef.Ptr.AsPageArray, mapRef.Ptr.AsMap->mPageArrayStartPageIx + mapPageIx + 1),
                                    K2OS_MAPTYPE_KERN_DATA);
                            }

                            pMailbox->Common.KernelSpace.mpMailboxOwner->mAvail = 0x100;

                        } while (0);

                        if (K2STAT_IS_ERROR(stat))
                        {
                            KernHeap_Free(pMailbox);
                        }

                    } while (0);

                    if (K2STAT_IS_ERROR(stat))
                    {
                        KernVirt_FreePages(kernVirtAddr);
                    }

                } while (0);

                KernObj_ReleaseRef(&notifyRef);

            } while (0);

            KernObj_ReleaseRef(&pageArrayRef);

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            KernProc_UserVirtHeapFree(apProc, K2HEAP_NodeAddr(&pProcHeapNode->HeapNode));
        }

    } while (0);

    KernObj_ReleaseRef(&mapRef);

    if (K2STAT_IS_ERROR(stat))
        return stat;

    //
    // finally add to proc mailbox list, verifying (again) that
    // we do not have a mailbox owner structure that overlaps with another mailbox owner structure
    //
    pOtherMailbox = NULL;
    disp = K2OSKERN_SeqLock(&apProc->Mail.SeqLock);
    pListLink = apProc->Mail.Locked.MailboxList.mpHead;
    if (NULL != pListLink)
    {
        do
        {
            pOtherMailbox = K2_GET_CONTAINER(K2OSKERN_OBJ_MAILBOX, pListLink, Common.ProcMailboxListLink);
            pListLink = pListLink->mpNext;

            if (pOtherMailbox->UserSpace.mRawAddr <= aUserMailboxOwnerAddr)
            {
                if ((aUserMailboxOwnerAddr - pOtherMailbox->UserSpace.mRawAddr) < sizeof(K2OS_MAILBOX_OWNER))
                {
                    // new mailbox owner structure starts partway into an existing mailbox owner structure
                    break;
                }
            }
            else
            {
                if ((pOtherMailbox->UserSpace.mRawAddr - aUserMailboxOwnerAddr) < sizeof(K2OS_MAILBOX_OWNER))
                {
                    // new mailbox owner structure extends partway into an existing mailbox owner structure
                    break;
                }
            }

            pOtherMailbox = NULL;

        } while (NULL != pListLink);
    }

    if (NULL == pOtherMailbox)
    {
        // no overlap
        K2LIST_AddAtTail(&apProc->Mail.Locked.MailboxList, &pMailbox->Common.ProcMailboxListLink);
        //        K2OSKERN_Debug("Create Mailbox %08X\n", pMailbox);
    }

    K2OSKERN_SeqUnlock(&apProc->Mail.SeqLock, disp);

    if (NULL != pOtherMailbox)
    {
        KernMailbox_Cleanup(K2OSKERN_GET_CURRENT_CPUCORE, pMailbox);
        return K2STAT_ERROR_ALREADY_MAPPED;
    }

    //
    // no overlap - can write to this without corrupting anything
    //
    pMailbox->Common.KernelSpace.mpMailboxOwner->mSentinel1 = K2OS_MAILBOX_OWNER_SENTINEL_1;
    pMailbox->Common.KernelSpace.mpMailboxOwner->mIxConsumer = 0x80000000;
    pMailbox->Common.KernelSpace.mpMailboxOwner->mpRoMsgPage = (K2OS_MSG *)K2HEAP_NodeAddr(&pProcHeapNode->HeapNode);
    pMailbox->Common.KernelSpace.mpMailboxOwner->mMailboxToken = NULL;
    K2MEM_Zero((void *)pMailbox->Common.KernelSpace.mpMailboxOwner->mFlagBitArray, K2OS_MAILBOX_OWNER_FLAG_DWORDS * 2 * sizeof(UINT32));
    pMailbox->Common.KernelSpace.mpMailboxOwner->mSentinel2 = K2OS_MAILBOX_OWNER_SENTINEL_2;
    K2_CpuWriteBarrier();

    KernObj_CreateRef(apRetMailboxRef, &pMailbox->Hdr);

    return K2STAT_NO_ERROR;
}

void
KernMailbox_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2STAT                  stat;
    K2OSKERN_OBJ_PROCESS *  pProc;
    UINT32                  userMailboxOwnerAddr;
    K2OS_MAILBOX_TOKEN      tokMailbox;
    K2OS_MAILSLOT_TOKEN     tokMailslot;
    K2OSKERN_OBJ_MAILSLOT * pMailslot;
    K2OS_USER_THREAD_PAGE * pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfUserThreadPage;

    userMailboxOwnerAddr = apCurThread->mSysCall_Arg0;

    pProc = apCurThread->ProcRef.Ptr.AsProc;

    if ((userMailboxOwnerAddr > K2OS_UVA_TIMER_IOPAGE_BASE - sizeof(K2OS_MAILBOX_OWNER)) ||
        (userMailboxOwnerAddr < K2OS_UVA_LOW_BASE))
    {
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }
    else
    {
        pMailslot = (K2OSKERN_OBJ_MAILSLOT *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_MAILSLOT));
        if (NULL == pMailslot)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            K2MEM_Zero(pMailslot, sizeof(K2OSKERN_OBJ_MAILSLOT));
            pMailslot->Hdr.mObjType = KernObj_Mailslot;
            K2LIST_Init(&pMailslot->Hdr.RefObjList);

            stat = KernMailbox_Create(pProc, userMailboxOwnerAddr, &pMailslot->RefMailbox);
            if (!K2STAT_IS_ERROR(stat))
            {
                stat = KernProc_TokenCreate(pProc, pMailslot->RefMailbox.Ptr.AsHdr, &tokMailbox);
                if (!K2STAT_IS_ERROR(stat))
                {
                    stat = KernProc_TokenCreate(pProc, &pMailslot->Hdr, &tokMailslot);
                    if (!K2STAT_IS_ERROR(stat))
                    {
                        pMailslot->RefMailbox.Ptr.AsMailbox->Common.KernelSpace.mpMailboxOwner->mMailboxToken = tokMailbox;
                        apCurThread->mSysCall_Result = (UINT_PTR)tokMailbox;
                        pThreadPage->mSysCall_Arg7_Result0 = (UINT_PTR)tokMailslot;
                        pMailslot = NULL;
                    }
                    else
                    {
                        KernProc_TokenDestroy(pProc, tokMailbox);
                    }
                }
                if (NULL != pMailslot)
                {
                    KernObj_ReleaseRef(&pMailslot->RefMailbox);
                }
            }
            if (NULL != pMailslot)
            {
                KernHeap_Free(pMailslot);
            }
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void
KernMailbox_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_MAILBOX *      apMailbox
)
{
    UINT_PTR chk;

    K2_ASSERT(apMailbox->Hdr.mObjType == KernObj_Mailbox);
    K2_ASSERT(apMailbox->Common.SchedOnlyCanSet.mIsClosed == TRUE);
    K2_ASSERT(apMailbox->DataPageArrayRef.Ptr.AsHdr == NULL);
    K2_ASSERT(apMailbox->Common.NotifyRef.Ptr.AsHdr == NULL);
    K2_ASSERT(apMailbox->UserSpace.mRawAddr == 0);
    K2_ASSERT(apMailbox->UserSpace.RawMapRef.Ptr.AsHdr == NULL);
    K2_ASSERT(apMailbox->UserSpace.RoDataPageMapRef.Ptr.AsHdr == NULL);

    K2_ASSERT(apMailbox->Common.ProcRef.Ptr.AsHdr != NULL);
    KernObj_ReleaseRef(&apMailbox->Common.ProcRef);

    K2_ASSERT(apMailbox->Common.SchedOnlyCanSet.CloserThreadRef.Ptr.AsHdr == NULL);
    K2_ASSERT(apMailbox->Common.NotifyRef.Ptr.AsHdr == NULL);
    for (chk = 0;chk < K2OS_MAILBOX_OWNER_FLAG_DWORDS * 32; chk++)
    {
        K2_ASSERT(apMailbox->Common.KernelSpace.ReserveUser[chk].Ptr.AsHdr == NULL);
    }
    K2_ASSERT(apMailbox->DataPageArrayRef.Ptr.AsHdr == NULL);
    K2_ASSERT(apMailbox->UserSpace.RawMapRef.Ptr.AsHdr == NULL);
    K2_ASSERT(apMailbox->UserSpace.RoDataPageMapRef.Ptr.AsHdr == NULL);

    K2MEM_Zero(apMailbox, sizeof(K2OSKERN_OBJ_MAILBOX));
    KernHeap_Free(apMailbox);
}

void
KernMailbox_Close_Done(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_MAILBOX *       apMailbox
)
{
    K2OSKERN_OBJ_THREAD *pCloserThread;

    KernObj_ReleaseRef(&apMailbox->DataPageArrayRef);
    KernVirt_FreePages(apMailbox->Common.KernelSpace.mVirtBase);
    apMailbox->Common.KernelSpace.mVirtBase = 0;
    KernProc_TokenDestroy(apMailbox->Common.ProcRef.Ptr.AsProc, apMailbox->Common.KernelSpace.mHoldTokenOnClose);
    apMailbox->Common.KernelSpace.mHoldTokenOnClose = NULL;

    pCloserThread = apMailbox->Common.SchedOnlyCanSet.CloserThreadRef.Ptr.AsThread;
    if (NULL != pCloserThread)
    {
        //
        // we must be inside the scheduler on this core
        //
        K2_ASSERT(apThisCore == gData.Sched.mpSchedulingCore);
        pCloserThread->SchedItem.mType = KernSchedItem_Thread_ResumeDeferral_Completed;
        KernObj_CreateRef(&pCloserThread->SchedItem.ObjRef, &pCloserThread->Hdr);
        KernTimer_GetHfTick(&pCloserThread->SchedItem.mHfTick);
        KernSched_QueueItem(&pCloserThread->SchedItem);
        KernObj_ReleaseRef(&apMailbox->Common.SchedOnlyCanSet.CloserThreadRef);
    }
}

void
KernMailbox_Close_CheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_MAILBOX * pMailbox;

    pMailbox = K2_GET_CONTAINER(K2OSKERN_OBJ_MAILBOX, apKey, Hdr.ObjDpc.Func);

    KTRACE(apThisCore, 1, KTRACE_MBOX_CLOSE_CHECK_DPC);

    if (0 == pMailbox->Common.KernelSpace.ClosedTlbShoot.mCoresRemaining)
    {
        KernMailbox_Close_Done(apThisCore, pMailbox);
    }
    else
    {
        pMailbox->Hdr.ObjDpc.Func = KernMailbox_Close_CheckComplete;
        KernCpu_QueueDpc(&pMailbox->Hdr.ObjDpc.Dpc, &pMailbox->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
    }
}

void
KernMailbox_Close_SendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apKey
)
{
    K2OSKERN_OBJ_MAILBOX *   pMailbox;
    UINT32                  sentMask;

    pMailbox = K2_GET_CONTAINER(K2OSKERN_OBJ_MAILBOX, apKey, Hdr.ObjDpc.Func);
    K2_ASSERT(0 != pMailbox->Common.KernelSpace.mIciSendMask);

    KTRACE(apThisCore, 1, KTRACE_MBOX_CLOSE_SENDICI_DPC);

    sentMask = KernArch_SendIci(
        apThisCore,
        pMailbox->Common.KernelSpace.mIciSendMask,
        KernIci_TlbInv,
        &pMailbox->Common.KernelSpace.ClosedTlbShoot
    );

    pMailbox->Common.KernelSpace.mIciSendMask &= ~sentMask;

    if (0 == pMailbox->Common.KernelSpace.mIciSendMask)
    {
        //
        // sent all icis. proceed to wait state for other cores' icis to complete
        //
        pMailbox->Hdr.ObjDpc.Func = KernMailbox_Close_CheckComplete;
    }
    else
    {
        pMailbox->Hdr.ObjDpc.Func = KernMailbox_Close_SendIci;
    }
    KernCpu_QueueDpc(&pMailbox->Hdr.ObjDpc.Dpc, &pMailbox->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
}

void
KernMailbox_PostCloseDpc(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apArg
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    K2OSKERN_OBJ_MAILBOX *  pMailbox;
    BOOL                    disp;
    K2OS_MAILBOX_TOKEN      tokMailbox;
    UINT32                  ix;
    K2OSKERN_OBJ_HEADER *   pObj;

    pMailbox = K2_GET_CONTAINER(K2OSKERN_OBJ_MAILBOX, apArg, Hdr.ObjDpc.Func);
    K2_ASSERT(pMailbox->Hdr.mObjType == KernObj_Mailbox);
    K2_ASSERT(pMailbox->Common.SchedOnlyCanSet.mIsClosed == TRUE);

    KTRACE(apThisCore, 1, KTRACE_MBOX_POST_CLOSE_DPC);

    pProc = pMailbox->Common.ProcRef.Ptr.AsProc;

    //
    // any holders of reserve messages notified that the message will 
    // never be consumed.
    //
    for (ix = 0; ix < 0x100; ix++)
    {
        pObj = pMailbox->Common.KernelSpace.ReserveUser[ix].Ptr.AsHdr;
        if (pObj != NULL)
        {
            switch (pObj->mObjType)
            {
            case KernObj_IpcEnd:
                KernIpcEnd_RecvRes((K2OSKERN_OBJ_IPCEND *)pObj, FALSE);
                break;

            case KernObj_IfSubs:
                K2ATOMIC_Inc(&((K2OSKERN_OBJ_IFSUBS *)pObj)->mBacklogRemaining);
                break;

            default:
                K2OSKERN_Panic("Mailbox close - unhandled reserve user type\n");
                break;
            }
            KernObj_ReleaseRef(&pMailbox->Common.KernelSpace.ReserveUser[ix]);
        }
    }

    //
    // remove mailbox stuff for its process
    //
    KernObj_ReleaseRef(&pMailbox->UserSpace.RawMapRef);
    KernObj_ReleaseRef(&pMailbox->UserSpace.RoDataPageMapRef);
    pMailbox->UserSpace.mRawAddr = 0;

    disp = K2OSKERN_SeqLock(&pProc->Mail.SeqLock);
    K2LIST_Remove(&pProc->Mail.Locked.MailboxList, &pMailbox->Common.ProcMailboxListLink);
    K2OSKERN_SeqUnlock(&pProc->Mail.SeqLock, disp);

    KernObj_ReleaseRef(&pMailbox->Common.NotifyRef);

    //
    // unmap the kernel pages and run the tlb invalidate for the kernel range
    //
    tokMailbox = pMailbox->Common.KernelSpace.mpMailboxOwner->mMailboxToken;
    K2MEM_Zero(pMailbox->Common.KernelSpace.mpMailboxOwner, sizeof(K2OS_MAILBOX_OWNER));
    pMailbox->Common.KernelSpace.mHoldTokenOnClose = tokMailbox;
    KernPte_BreakPageMap(NULL, pMailbox->Common.KernelSpace.mVirtBase, 0);
    KernPte_BreakPageMap(NULL, pMailbox->Common.KernelSpace.mVirtBase + K2_VA_MEMPAGE_BYTES, 0);
    if (pMailbox->KernelSpace.mClientPageCount > 1)
    {
        KernPte_BreakPageMap(NULL, pMailbox->Common.KernelSpace.mVirtBase + (2 * K2_VA_MEMPAGE_BYTES), 0);
    }

    //
    // start TLB shootdown - Common.KernelSpace.mVirtBase for (2 + KernelSpace.mClientPageCount);
    //
    if (gData.mCpuCoreCount > 1)
    {
        pMailbox->Common.KernelSpace.ClosedTlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
        pMailbox->Common.KernelSpace.ClosedTlbShoot.mpProc = NULL;
        pMailbox->Common.KernelSpace.ClosedTlbShoot.mVirtBase = pMailbox->Common.KernelSpace.mVirtBase;
        pMailbox->Common.KernelSpace.ClosedTlbShoot.mPageCount = 1 + pMailbox->KernelSpace.mClientPageCount;
        pMailbox->Common.KernelSpace.mIciSendMask = pMailbox->Common.KernelSpace.ClosedTlbShoot.mCoresRemaining;
        KernMailbox_Close_SendIci(apThisCore, &pMailbox->Hdr.ObjDpc.Func);
    }

    KernArch_InvalidateTlbPageOnCurrentCore(pMailbox->Common.KernelSpace.mVirtBase);
    KernArch_InvalidateTlbPageOnCurrentCore(pMailbox->Common.KernelSpace.mVirtBase + K2_VA_MEMPAGE_BYTES);
    if (pMailbox->KernelSpace.mClientPageCount > 1)
    {
        KernArch_InvalidateTlbPageOnCurrentCore(pMailbox->Common.KernelSpace.mVirtBase + (2 * K2_VA_MEMPAGE_BYTES));
    }

    if (gData.mCpuCoreCount == 1)
    {
        KernMailbox_Close_Done(apThisCore, pMailbox);
    }
}

void    
KernMailslot_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_MAILSLOT *     apMailslot
)
{
    K2_ASSERT(0 != (apMailslot->Hdr.mObjFlags & K2OSKERN_OBJ_FLAG_REFS_DEC_TO_ZERO));

    K2_ASSERT(apMailslot->Hdr.mObjType == KernObj_Mailslot);
    K2_ASSERT(apMailslot->Hdr.RefObjList.mNodeCount == 0);

    KernObj_ReleaseRef(&apMailslot->RefMailbox);

    K2MEM_Zero(apMailslot, sizeof(K2OSKERN_OBJ_MAILSLOT));

    KernHeap_Free(apMailslot);
}
