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
KernMailbox_Cleanup_Complete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_MAILBOX *      apMailbox
)
{
    KernObj_ReleaseRef(&apMailbox->RefPageArray);

    KernVirt_Release(apMailbox->mKernVirtAddr);

    KernObj_Free(&apMailbox->Hdr);
}

void
KernMailbox_Cleanup_CheckComplete(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apArg
)
{
    K2OSKERN_OBJ_MAILBOX *  pMailbox;

    pMailbox = K2_GET_CONTAINER(K2OSKERN_OBJ_MAILBOX, apArg, Hdr.ObjDpc.Func);

    if (0 == pMailbox->PurgeTlbShoot.mCoresRemaining)
    {
        KernMailbox_Cleanup_Complete(apThisCore, pMailbox);
        return;
    }

    pMailbox->Hdr.ObjDpc.Func = KernMailbox_Cleanup_CheckComplete;
    KernCpu_QueueDpc(&pMailbox->Hdr.ObjDpc.Dpc, &pMailbox->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
}

void
KernMailbox_Cleanup_SendIci(
    K2OSKERN_CPUCORE volatile * apThisCore,
    void *                      apArg
)
{
    K2OSKERN_OBJ_MAILBOX *  pMailbox;
    UINT32                  sentMask;

    pMailbox = K2_GET_CONTAINER(K2OSKERN_OBJ_MAILBOX, apArg, Hdr.ObjDpc.Func);

    sentMask = KernArch_SendIci(
        apThisCore,
        pMailbox->mIciSendMask,
        KernIci_TlbInv,
        &pMailbox->PurgeTlbShoot
    );

    pMailbox->mIciSendMask &= ~sentMask;

    if (0 == pMailbox->mIciSendMask)
    {
        pMailbox->Hdr.ObjDpc.Func = KernMailbox_Cleanup_CheckComplete;
    }
    else
    {
        pMailbox->Hdr.ObjDpc.Func = KernMailbox_Cleanup_SendIci;
    }

    KernCpu_QueueDpc(&pMailbox->Hdr.ObjDpc.Dpc, &pMailbox->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
}

void    
KernMailbox_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_MAILBOX *      apMailbox
)
{
    UINT32 ixPage;
    UINT32 virtAddr;

    KernObj_ReleaseRef(&apMailbox->RefGate);

    virtAddr = apMailbox->mKernVirtAddr;
    for (ixPage = 0; ixPage < 3; ixPage++)
    {
        KernPte_BreakPageMap(NULL, virtAddr, 0);
        virtAddr += K2_VA_MEMPAGE_BYTES;
    }

    //
    // start the kernel address space shootdown
    //
    apMailbox->PurgeTlbShoot.mpProc = NULL;
    apMailbox->PurgeTlbShoot.mVirtBase = apMailbox->mKernVirtAddr;
    apMailbox->PurgeTlbShoot.mPageCount = 3;
    apMailbox->PurgeTlbShoot.mCoresRemaining = ((1 << gData.mCpuCoreCount) - 1) & ~(1 << apThisCore->mCoreIx);
    apMailbox->mIciSendMask = apMailbox->PurgeTlbShoot.mCoresRemaining;
    
    if (gData.mCpuCoreCount > 1)
    {
        apMailbox->Hdr.ObjDpc.Func = KernMailbox_Cleanup_SendIci;
        KernCpu_QueueDpc(&apMailbox->Hdr.ObjDpc.Dpc, &apMailbox->Hdr.ObjDpc.Func, KernDpcPrio_Hi);
    }

    virtAddr = apMailbox->mKernVirtAddr;
    for (ixPage = 0; ixPage < 3; ixPage++)
    {
        KernArch_InvalidateTlbPageOnCurrentCore(virtAddr);
        virtAddr += K2_VA_MEMPAGE_BYTES;
    }

    if (1 == gData.mCpuCoreCount)
    {
        KernMailbox_Cleanup_Complete(apThisCore, apMailbox);
    }
}

BOOL
KernMailbox_Reserve(
    K2OSKERN_OBJ_MAILBOX *  apMailbox,
    UINT32                  aCount
)
{
    UINT32 volatile *               pAvail;
    UINT32                          v;
    K2OS_MAILBOX_PRODUCER_PAGE *    pProd;

    if (apMailbox->mOwnerDead)
        return FALSE;

    pProd = (K2OS_MAILBOX_PRODUCER_PAGE *)(apMailbox->mKernVirtAddr + K2_VA_MEMPAGE_BYTES);
 
    pAvail = &pProd->AvailCount.mVal;
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
    K2OSKERN_OBJ_MAILBOX *  apMailbox,
    UINT32                  aCount
)
{
    K2OS_MAILBOX_PRODUCER_PAGE *    pProd;

    if ((0 == aCount) ||
        (apMailbox->mOwnerDead))
        return;

    pProd = (K2OS_MAILBOX_PRODUCER_PAGE *)(apMailbox->mKernVirtAddr + K2_VA_MEMPAGE_BYTES);

    K2ATOMIC_Add((INT32 volatile *)&pProd->AvailCount.mVal, aCount);
}

void    
KernMailbox_SysCall_Create(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJ_MAILBOX *      pMailbox;

    K2OSKERN_OBJ_MAILBOXOWNER * pOwner;
    K2OSKERN_OBJREF             refOwner;
    UINT32                      userOwnerVirt;
    K2OSKERN_OBJREF             refOwnerMap;

    K2OSKERN_OBJ_PROCESS *      pProc;

    BOOL                        disp;
    UINT32                      kernMapVirt;
    K2OSKERN_OBJREF             refGate;
    K2OSKERN_OBJREF             refPageArray;
    K2STAT                      stat;
    K2OS_THREAD_PAGE *          pThreadPage;
    K2OSKERN_PROCHEAP_NODE *    pOwnerUserHeapNode;
    
    K2OS_MAILBOX_PRODUCER_PAGE *pProd;
    K2OS_MAILBOX_CONSUMER_PAGE *pCons;

    refOwner.AsAny = NULL;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    pMailbox = (K2OSKERN_OBJ_MAILBOX *)KernObj_Alloc(KernObj_Mailbox);
    if (NULL == pMailbox)
    {
        stat = K2STAT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        do {
            pOwner = (K2OSKERN_OBJ_MAILBOXOWNER *)KernObj_Alloc(KernObj_MailboxOwner);
            if (NULL == pOwner)
            {
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
                break;
            }

            do {
                refGate.AsAny = NULL;
                stat = KernGate_Create(FALSE, &refGate);
                if (K2STAT_IS_ERROR(stat))
                {
                    break;
                }

                do {
                    refPageArray.AsAny = NULL;
                    stat = KernPageArray_CreateSparse(3, K2OS_MAPTYPE_USER_DATA, &refPageArray);
                    if (K2STAT_IS_ERROR(stat))
                    {
                        break;
                    }

                    do {
                        kernMapVirt = KernVirt_Reserve(3);
                        if (0 == kernMapVirt)
                        {
                            stat = K2STAT_ERROR_OUT_OF_MEMORY;
                            break;
                        }

                        do {
                            pProc = apCurThread->RefProc.AsProc;

                            stat = KernProc_UserVirtHeapAlloc(pProc, 3, &pOwnerUserHeapNode);
                            if (K2STAT_IS_ERROR(stat))
                            { 
                                break;
                            }
                            userOwnerVirt = K2HEAP_NodeAddr(&pOwnerUserHeapNode->HeapNode);

                            do {
                                refOwnerMap.AsAny = NULL;
                                stat = KernVirtMap_CreateUser(pProc,
                                    refPageArray.AsPageArray,
                                    0, userOwnerVirt, 3,
                                    K2OS_MapType_Data_ReadWrite,
                                    &refOwnerMap);
                                if (K2STAT_IS_ERROR(stat))
                                {
                                    break;
                                }

                                //
                                // have all resources now
                                //
                                KernPte_MakePageMap(NULL, kernMapVirt, KernPageArray_PagePhys(refPageArray.AsPageArray, 0), K2OS_MAPTYPE_KERN_DATA);
                                KernPte_MakePageMap(NULL, kernMapVirt + K2_VA_MEMPAGE_BYTES, KernPageArray_PagePhys(refPageArray.AsPageArray, 1), K2OS_MAPTYPE_KERN_DATA);
                                KernPte_MakePageMap(NULL, kernMapVirt + (K2_VA_MEMPAGE_BYTES * 2), KernPageArray_PagePhys(refPageArray.AsPageArray, 2), K2OS_MAPTYPE_KERN_DATA);

                                K2MEM_Zero((void *)kernMapVirt, 3 * K2_VA_MEMPAGE_BYTES);

                                pCons = (K2OS_MAILBOX_CONSUMER_PAGE *)kernMapVirt;
                                pCons->IxConsumer.mVal = K2OS_MAILBOX_GATE_CLOSED_BIT;

                                pProd = (K2OS_MAILBOX_PRODUCER_PAGE *)(kernMapVirt + K2_VA_MEMPAGE_BYTES);
                                pProd->AvailCount.mVal = K2OS_MAILBOX_MSG_COUNT;

                                K2_CpuWriteBarrier();

                                pMailbox->mKernVirtAddr = kernMapVirt;
                                KernObj_CreateRef(&pMailbox->RefGate, refGate.AsAny);
                                KernObj_CreateRef(&pMailbox->RefPageArray, refPageArray.AsAny);

                                pOwner->mProcVirt = userOwnerVirt;
                                KernObj_CreateRef(&pOwner->RefProcMap, refOwnerMap.AsAny);
                                KernObj_CreateRef(&pOwner->RefProc, &pProc->Hdr);
                                KernObj_CreateRef(&pOwner->RefMailbox, &pMailbox->Hdr);

                                KernObj_CreateRef(&refOwner, &pOwner->Hdr);
                                pOwner = NULL;

                                kernMapVirt = 0;

                                KernObj_ReleaseRef(&refOwnerMap);

                            } while (0);

                            if (K2STAT_IS_ERROR(stat))
                            {
                                K2_ASSERT(0 != userOwnerVirt);
                                KernProc_UserVirtHeapFree(pProc, userOwnerVirt);
                            }

                        } while (0);

                        if (K2STAT_IS_ERROR(stat))
                        {
                            K2_ASSERT(0 != kernMapVirt);
                            KernVirt_Release(kernMapVirt);
                        }

                    } while (0);

                    KernObj_ReleaseRef(&refPageArray);

                } while (0);

                KernObj_ReleaseRef(&refGate);

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2_ASSERT(NULL != pOwner);
                KernObj_Free(&pOwner->Hdr);
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2_ASSERT(NULL != pMailbox);
            KernObj_Free(&pMailbox->Hdr);
        }
    }

    if (!K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(NULL != refOwner.AsAny);

        stat = KernProc_TokenCreate(apCurThread->RefProc.AsProc, refOwner.AsAny, (K2OS_TOKEN *)&apCurThread->User.mSysCall_Result);
        if (!K2STAT_IS_ERROR(stat))
        {
            disp = K2OSKERN_SeqLock(&pProc->MboxOwner.SeqLock);
            K2LIST_AddAtTail(&pProc->MboxOwner.Locked.List, &refOwner.AsMailboxOwner->OwnerMboxOwnerListLocked.ListLink);
            K2OSKERN_SeqUnlock(&pProc->MboxOwner.SeqLock, disp);

            pThreadPage->mSysCall_Arg7_Result0 = userOwnerVirt;
        }
        
        KernObj_ReleaseRef(&refOwner);
    }
    else
    {
        K2_ASSERT(NULL == refOwner.AsAny);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
    }
}

void
KernMailbox_RecvRes(
    K2OSKERN_OBJ_MAILBOX *  apMailbox,
    UINT32                  aSlotIx
)
{
    K2OSKERN_MAILBOX_CONSUMER_PAGE *    pCons;
    UINT32                              ixWord;
    UINT32                              ixBit;
    UINT32                              bitsValue;

    pCons = (K2OSKERN_MAILBOX_CONSUMER_PAGE *)apMailbox->mKernVirtAddr;
    ixWord = aSlotIx >> 5;
    ixBit = aSlotIx & 0x1F;

    do {
        bitsValue = pCons->UserVisible.ReserveMask[ixWord].mVal;
        K2_CpuReadBarrier();
        if (0 == (bitsValue & (1 << ixBit)))
            break;
    } while (bitsValue != K2ATOMIC_CompareExchange(&pCons->UserVisible.ReserveMask[ixWord].mVal, bitsValue & ~(1 << ixBit), bitsValue));

    if (0 != (bitsValue & (1 << ixBit)))
    {
        //
        // we cleared the bit so we release the reserve
        //
        K2_ASSERT(NULL != pCons->ReserveHolder[aSlotIx].AsAny);

        switch (pCons->ReserveHolder[aSlotIx].AsAny->mObjType)
        {
        case KernObj_IpcEnd:
            KernIpcEnd_RecvRes(pCons->ReserveHolder[aSlotIx].AsIpcEnd, TRUE);
            break;

        case KernObj_IfSubs:
            K2ATOMIC_Inc(&pCons->ReserveHolder[aSlotIx].AsIfSubs->mBacklogRemaining);
            break;

        default:
            break;
        }

        KernObj_ReleaseRef(&pCons->ReserveHolder[aSlotIx]);
    }
}

void    
KernMailboxOwner_SysCall_RecvRes(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF                 refMailboxOwner;
    K2STAT                          stat;
    UINT32                          slotIx;
    K2OS_THREAD_PAGE *              pThreadPage;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    refMailboxOwner.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->RefProc.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refMailboxOwner);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_MailboxOwner != refMailboxOwner.AsAny->mObjType)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            slotIx = pThreadPage->mSysCall_Arg1;
            if (slotIx >= K2OS_MAILBOX_MSG_COUNT)
            {
                stat = K2STAT_ERROR_BAD_ARGUMENT;
            }
            else
            {
                KernMailbox_RecvRes(refMailboxOwner.AsMailboxOwner->RefMailbox.AsMailbox, slotIx);
                apCurThread->User.mSysCall_Result = (UINT32)TRUE;
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
KernMailboxOwner_SysCall_RecvLast(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF         refMailboxOwner;
    K2STAT                  stat;
    K2OS_THREAD_PAGE *      pThreadPage;
    K2OSKERN_SCHED_ITEM *   pSchedItem;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    refMailboxOwner.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->RefProc.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refMailboxOwner);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_MailboxOwner != refMailboxOwner.AsAny->mObjType)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            pSchedItem = &apCurThread->SchedItem;
            pSchedItem->mSchedItemType = KernSchedItem_Thread_SysCall;
            KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
            KernObj_CreateRef(&pSchedItem->ObjRef, refMailboxOwner.AsMailboxOwner->RefMailbox.AsAny);
            KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
            KernSched_QueueItem(pSchedItem);
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
KernMailboxOwner_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_MAILBOXOWNER * apMailboxOwner
)
{
    K2OSKERN_OBJ_PROCESS *  pProc;
    BOOL                    disp;

    pProc = apMailboxOwner->RefProc.AsProc;
    if (NULL != pProc)
    {
        disp = K2OSKERN_SeqLock(&pProc->MboxOwner.SeqLock);
        K2LIST_Remove(&pProc->MboxOwner.Locked.List, &apMailboxOwner->OwnerMboxOwnerListLocked.ListLink);
        K2OSKERN_SeqUnlock(&pProc->MboxOwner.SeqLock, disp);

        K2_ASSERT(NULL != apMailboxOwner->RefProcMap.AsVirtMap);
        KernObj_ReleaseRef(&apMailboxOwner->RefProcMap);

        KernObj_ReleaseRef(&apMailboxOwner->RefProc);
    }
    else
    {
        disp = K2OSKERN_SeqLock(&gData.MboxOwner.SeqLock);
        K2LIST_Remove(&gData.MboxOwner.KernList, &apMailboxOwner->OwnerMboxOwnerListLocked.ListLink);
        K2OSKERN_SeqUnlock(&gData.MboxOwner.SeqLock, disp);

        K2_ASSERT(NULL == apMailboxOwner->RefProcMap.AsVirtMap);
    }

    KernObj_ReleaseRef(&apMailboxOwner->RefMailbox);

    KernObj_Free(&apMailboxOwner->Hdr);
}

void    
KernMailslot_SysCall_Get(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF             refMailslot;
    K2STAT                      stat;
    K2OSKERN_OBJ_PROCESS *      pProc;
    UINT32                      userSlotVirt;
    K2OSKERN_OBJREF             refSlotMap[2];
    K2OSKERN_PROCHEAP_NODE *    pSlotUserHeapNode;
    K2OSKERN_OBJ_MAILBOX *      pMailbox;

    pProc = apCurThread->RefProc.AsProc;

    refMailslot.AsAny = NULL;
    stat = KernProc_TokenTranslate(pProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refMailslot);
    if (!K2STAT_IS_ERROR(stat))
    {
        if (KernObj_Mailslot != refMailslot.AsAny->mObjType)
        {
            stat = K2STAT_ERROR_BAD_TOKEN;
        }
        else
        {
            if (0 != refMailslot.AsMailslot->mProcVirt)
            {
                apCurThread->User.mSysCall_Result = refMailslot.AsMailslot->mProcVirt;
            }
            else
            {
                //
                // map it into the process now
                //
                pMailbox = refMailslot.AsMailslot->RefMailbox.AsMailbox;
                stat = KernProc_UserVirtHeapAlloc(pProc, 3, &pSlotUserHeapNode);
                if (!K2STAT_IS_ERROR(stat))
                {
                    userSlotVirt = K2HEAP_NodeAddr(&pSlotUserHeapNode->HeapNode);
                    refSlotMap[0].AsAny = NULL;
                    stat = KernVirtMap_CreateUser(pProc,
                        pMailbox->RefPageArray.AsPageArray,
                        0, userSlotVirt, 1,
                        K2OS_MapType_Data_ReadOnly,
                        &refSlotMap[0]);

                    if (!K2STAT_IS_ERROR(stat))
                    {
                        refSlotMap[1].AsAny = NULL;
                        stat = KernVirtMap_CreateUser(pProc,
                            pMailbox->RefPageArray.AsPageArray,
                            1, userSlotVirt, 2,
                            K2OS_MapType_Data_ReadWrite,
                            &refSlotMap[1]);

                        if (!K2STAT_IS_ERROR(stat))
                        {
                            KernObj_CreateRef(&refMailslot.AsMailslot->RefProcMap[0], refSlotMap[0].AsAny);
                            KernObj_CreateRef(&refMailslot.AsMailslot->RefProcMap[1], refSlotMap[1].AsAny);

                            refMailslot.AsMailslot->mProcVirt = userSlotVirt;
                            userSlotVirt = 0;
                            KernObj_ReleaseRef(&refSlotMap[1]);
                        }

                        KernObj_ReleaseRef(&refSlotMap[0]);
                    }

                    if (0 != userSlotVirt)
                    {
                        KernProc_UserVirtHeapFree(pProc, userSlotVirt);
                    }
                }
            }
        }

        KernObj_ReleaseRef(&refMailslot);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        apCurThread->mpKernRwViewOfThreadPage->mLastStatus = stat;
    }
}

BOOL    
KernMailbox_InIntr_Fast_Check_SentFirst(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF                 refSender;
    K2OS_THREAD_PAGE *              pThreadPage;
    K2STAT                          stat;
    K2OS_MAILBOX_CONSUMER_PAGE *    pCons;
    UINT32                          ixCons;
    BOOL                            result;
    K2OSKERN_OBJ_MAILBOX *          pMailbox;

    //
    // return TRUE for fast exit
    //
    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    refSender.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->RefProc.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refSender);
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
        return TRUE;
    }

    result = TRUE;

    if ((refSender.AsAny->mObjType != KernObj_Mailslot) &&
        (refSender.AsAny->mObjType != KernObj_MailboxOwner))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = K2STAT_ERROR_BAD_TOKEN;
    }
    else
    {
        do {
            if (refSender.AsAny->mObjType == KernObj_Mailslot)
            {
                pMailbox = refSender.AsMailslot->RefMailbox.AsMailbox;
                if (pMailbox->mOwnerDead)
                {
                    apCurThread->User.mSysCall_Result = 0;
                    pThreadPage->mLastStatus = K2STAT_ERROR_CLOSED;
                    break;
                }
            }
            else
            {
                pMailbox = refSender.AsMailboxOwner->RefMailbox.AsMailbox;
            }

            pCons = (K2OS_MAILBOX_CONSUMER_PAGE *)pMailbox->mKernVirtAddr;

            ixCons = pCons->IxConsumer.mVal;
            K2_CpuReadBarrier();

            if (0 == (ixCons & K2OS_MAILBOX_GATE_CLOSED_BIT))
            {
                // gate is already open
                apCurThread->User.mSysCall_Result = (UINT32)TRUE;
            }
            else
            {
                result = FALSE; // no fast return
            }
        } while (0);
    }

    KernObj_ReleaseRef(&refSender);

    return result;
}

void    
KernMailbox_SysCall_SentFirst(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_THREAD *       apCurThread
)
{
    K2OSKERN_OBJREF                 refSender;
    K2OS_THREAD_PAGE *              pThreadPage;
    K2STAT                          stat;
    K2OS_MAILBOX_CONSUMER_PAGE *    pCons;
    UINT32                          ixCons;
    K2OSKERN_SCHED_ITEM *           pSchedItem;
    K2OSKERN_OBJ_MAILBOX *          pMailbox;

    pThreadPage = apCurThread->mpKernRwViewOfThreadPage;

    refSender.AsAny = NULL;
    stat = KernProc_TokenTranslate(apCurThread->RefProc.AsProc, (K2OS_TOKEN)apCurThread->User.mSysCall_Arg0, &refSender);
    if (K2STAT_IS_ERROR(stat))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = stat;
        return;
    }

    if ((refSender.AsAny->mObjType != KernObj_Mailslot) &&
        (refSender.AsAny->mObjType != KernObj_MailboxOwner))
    {
        apCurThread->User.mSysCall_Result = 0;
        pThreadPage->mLastStatus = K2STAT_ERROR_BAD_TOKEN;
    }
    else
    {
        do {
            if (refSender.AsAny->mObjType == KernObj_Mailslot)
            {
                pMailbox = refSender.AsMailslot->RefMailbox.AsMailbox;
                if (pMailbox->mOwnerDead)
                {
                    apCurThread->User.mSysCall_Result = 0;
                    pThreadPage->mLastStatus = K2STAT_ERROR_CLOSED;
                    break;
                }
            }
            else
            {
                pMailbox = refSender.AsMailboxOwner->RefMailbox.AsMailbox;
            }

            pCons = (K2OS_MAILBOX_CONSUMER_PAGE *)pMailbox->mKernVirtAddr;

            ixCons = pCons->IxConsumer.mVal;
            K2_CpuReadBarrier();

            apCurThread->User.mSysCall_Result = (UINT32)TRUE;

            if (0 != (ixCons & K2OS_MAILBOX_GATE_CLOSED_BIT))
            {
                pSchedItem = &apCurThread->SchedItem;
                pSchedItem->mSchedItemType = KernSchedItem_Thread_SysCall;
                KernArch_GetHfTimerTick(&pSchedItem->mHfTick);
                KernObj_CreateRef(&pSchedItem->ObjRef, &pMailbox->Hdr);
                KernCpu_TakeCurThreadOffThisCore(apThisCore, apCurThread, KernThreadState_InScheduler);
                KernSched_QueueItem(pSchedItem);
            }
        } while (0);

        KernObj_ReleaseRef(&refSender);
    }
}

K2STAT
KernMailbox_Share(
    K2OSKERN_OBJ_MAILBOX  * apMailbox,
    K2OSKERN_OBJ_PROCESS *  apTargetProc,
    UINT32 *                apRetTargetProcTokenValue
)
{
    K2STAT                      stat;
    K2OSKERN_OBJ_MAILSLOT *     pMailslot;
    K2OSKERN_OBJREF             refSlot;

    K2_ASSERT(apMailbox->Hdr.mObjType == KernObj_Mailbox);

    if ((NULL != apTargetProc) && (apTargetProc->mState >= KernProcState_Stopped))
    {
        stat = K2STAT_ERROR_PROCESS_EXITED;
    }
    else
    {
        pMailslot = (K2OSKERN_OBJ_MAILSLOT *)KernObj_Alloc(KernObj_Mailslot);
        if (NULL == pMailslot)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            if (NULL != apTargetProc)
            {
                KernObj_CreateRef(&pMailslot->RefProc, &apTargetProc->Hdr);
            }
            KernObj_CreateRef(&pMailslot->RefMailbox, &apMailbox->Hdr);

            refSlot.AsAny = NULL;
            KernObj_CreateRef(&refSlot, &pMailslot->Hdr);

            if (NULL != apTargetProc)
            {
                stat = KernProc_TokenCreate(apTargetProc, refSlot.AsAny, (K2OS_TOKEN *)apRetTargetProcTokenValue);
            }
            else
            {
                stat = KernToken_Create(refSlot.AsAny, (K2OS_TOKEN *)apRetTargetProcTokenValue);
            }

            KernObj_ReleaseRef(&refSlot);
        }
    }

    return stat;
}

void    
KernMailslot_Cleanup(
    K2OSKERN_CPUCORE volatile * apThisCore,
    K2OSKERN_OBJ_MAILSLOT *     apMailslot
)
{
    if (NULL != apMailslot->RefProc.AsAny)
    {
        if (0 != apMailslot->mProcVirt)
        {
            K2_ASSERT(NULL != apMailslot->RefProcMap[0].AsVirtMap);
            KernObj_ReleaseRef(&apMailslot->RefProcMap[0]);
            K2_ASSERT(NULL != apMailslot->RefProcMap[1].AsVirtMap);
            KernObj_ReleaseRef(&apMailslot->RefProcMap[1]);
            KernProc_UserVirtHeapFree(apMailslot->RefProc.AsProc, apMailslot->mProcVirt);
        }
        KernObj_ReleaseRef(&apMailslot->RefProc);
    }
    else
    {
        K2_ASSERT(0 == apMailslot->mProcVirt);
        K2_ASSERT(NULL == apMailslot->RefProcMap[0].AsVirtMap);
        K2_ASSERT(NULL == apMailslot->RefProcMap[1].AsVirtMap);
    }

    KernObj_ReleaseRef(&apMailslot->RefMailbox);

    KernObj_Free(&apMailslot->Hdr);
}

K2STAT
KernMailbox_Deliver(
    K2OSKERN_OBJ_MAILBOX *  apMailbox,
    K2OS_MSG const *        apMsg,
    BOOL                    aDoAvailAlloc,
    K2OSKERN_OBJ_HEADER *   apReserveUser,
    BOOL *                  apRetDoSignal
)
{
    K2OSKERN_MAILBOX_CONSUMER_PAGE  *   pCons;
    K2OS_MAILBOX_PRODUCER_PAGE *        pProd;
    K2OS_MAILBOX_MSGDATA_PAGE *         pData;
    UINT32                              ixSlot;
    UINT32                              ixWord;
    UINT32                              ixBit;
    UINT32                              ixCons;
    UINT32                              bitField;

    if (NULL != apReserveUser)
    {
        K2_ASSERT(!aDoAvailAlloc);
    }

    pCons = (K2OSKERN_MAILBOX_CONSUMER_PAGE *)apMailbox->mKernVirtAddr;
    pProd = (K2OS_MAILBOX_PRODUCER_PAGE *)(apMailbox->mKernVirtAddr + K2_VA_MEMPAGE_BYTES);

    if (aDoAvailAlloc)
    {
        do {
            ixSlot = pProd->AvailCount.mVal;
            K2_CpuReadBarrier();
            if (0 == ixSlot)
            {
                return K2STAT_ERROR_FULL;
            }
        } while (ixSlot != K2ATOMIC_CompareExchange(&pProd->AvailCount.mVal, ixSlot - 1, ixSlot));
    }

    pData = (K2OS_MAILBOX_MSGDATA_PAGE *)(apMailbox->mKernVirtAddr + (2 * K2_VA_MEMPAGE_BYTES));

    //
    // if we get here we have a slot, we just don't know its index yet
    //
    do {
        ixSlot = pProd->IxProducer.mVal;
        K2_CpuReadBarrier();
    } while (ixSlot != K2ATOMIC_CompareExchange(&pProd->IxProducer.mVal, (ixSlot + 1) & K2OS_MAILBOX_MSG_IX_MASK, ixSlot));

    //
    // now we have an index
    //
    ixWord = ixSlot >> 5;
    ixBit = ixSlot & 0x1F;

    //
    // this should never hit as we got an available slot
    //
    bitField = pProd->OwnerMask[ixWord].mVal;
    K2_CpuReadBarrier();
    K2_ASSERT(0 == (bitField & (1 << ixBit)));

    K2MEM_Copy(&pData->Msg[ixSlot], apMsg, sizeof(K2OS_MSG));
    K2_CpuWriteBarrier();

    if (NULL != apReserveUser)
    {
        KernObj_CreateRef(&pCons->ReserveHolder[ixSlot], apReserveUser);
        K2ATOMIC_Or(&pCons->UserVisible.ReserveMask[ixWord].mVal, (1 << ixBit));
    }
    else
    {
        K2ATOMIC_And(&pCons->UserVisible.ReserveMask[ixWord].mVal, ~(1 << ixBit));
        K2_ASSERT(NULL == pCons->ReserveHolder[ixSlot].AsAny);
    }

    K2ATOMIC_Or(&pProd->OwnerMask[ixWord].mVal, (1 << ixBit));

    ixCons = pCons->UserVisible.IxConsumer.mVal;
    K2_CpuReadBarrier();

    *apRetDoSignal = (0 != (ixCons & K2OS_MAILBOX_GATE_CLOSED_BIT)) ? TRUE : FALSE;

    return K2STAT_NO_ERROR;
}

void    
KernMailboxOwner_AbortReserveHolders(
    K2OSKERN_OBJ_MAILBOXOWNER *apMailboxOwner
)
{
    K2OSKERN_MAILBOX_CONSUMER_PAGE  *   pCons;
    UINT32                              ixSlot;
    UINT32                              ixWord;
    UINT32                              ixBit;

    pCons = (K2OSKERN_MAILBOX_CONSUMER_PAGE *)apMailboxOwner->RefMailbox.AsMailbox->mKernVirtAddr;
    ixSlot = 0;
    for (ixWord = 0; ixWord < K2OS_MAILBOX_BITFIELD_DWORD_COUNT; ixWord++)
    {
        for (ixBit = 0; ixBit < 32; ixBit++)
        {
            if (0 != (pCons->UserVisible.ReserveMask[ixWord].mVal & (1 << ixBit)))
            {
//                K2OSKERN_Debug("Abort reserve holder in slot %d\n", ixSlot);
                K2_ASSERT(pCons->ReserveHolder[ixSlot].AsAny != NULL);
                KernObj_ReleaseRef(&pCons->ReserveHolder[ixSlot]);
            }
            else
            {
                K2_ASSERT(pCons->ReserveHolder[ixSlot].AsAny == NULL);
            }
            ixSlot++;
        }
    }
}
