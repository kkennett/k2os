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

K2OS_MAILBOX_TOKEN  
K2OS_Mailbox_Create(
    void
)
{
    K2OSKERN_OBJ_MAILBOX *      pMailbox;

    K2OSKERN_OBJ_MAILBOXOWNER * pOwner;
    K2OSKERN_OBJREF             refOwner;

    K2OSKERN_OBJ_MAILSLOT *     pMailslot;

    UINT32                      kernMapVirt;
    K2OSKERN_OBJREF             refGate;
    K2OSKERN_OBJREF             refPageArray;
    K2STAT                      stat;

    K2OS_MAILBOX_PRODUCER_PAGE *pProd;
    K2OS_MAILBOX_CONSUMER_PAGE *pCons;

    K2OS_MAILBOX_TOKEN          tokMailbox;

    tokMailbox = NULL;
    refOwner.AsAny = NULL;

    pMailbox = (K2OSKERN_OBJ_MAILBOX *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_MAILBOX));
    if (NULL == pMailbox)
    {
        stat = K2STAT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        do {
            pMailslot = (K2OSKERN_OBJ_MAILSLOT *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_MAILSLOT));
            if (NULL == pMailslot)
            {
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
                break;
            }

            do {
                pOwner = (K2OSKERN_OBJ_MAILBOXOWNER *)KernHeap_Alloc(sizeof(K2OSKERN_OBJ_MAILBOXOWNER));
                if (NULL == pOwner)
                {
                    stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    break;
                }

                do {
                    refGate.AsAny = NULL;
                    stat = KernGate_Create(FALSE, &refGate);
                    if (K2STAT_IS_ERROR(stat))
                        break;

                    do {
                        refPageArray.AsAny = NULL;
                        stat = KernPageArray_CreateSparse(3, K2OS_MAPTYPE_USER_DATA, &refPageArray);
                        if (K2STAT_IS_ERROR(stat))
                            break;

                        do {
                            kernMapVirt = K2OS_Virt_Reserve(3);
                            if (0 == kernMapVirt)
                            {
                                stat = K2OS_Thread_GetLastStatus();
                                K2_ASSERT(K2STAT_IS_ERROR(stat));
                                break;
                            }

                            KernPte_MakePageMap(NULL, kernMapVirt, KernPageArray_PagePhys(refPageArray.AsPageArray, 0), K2OS_MAPTYPE_KERN_DATA);
                            KernPte_MakePageMap(NULL, kernMapVirt + K2_VA_MEMPAGE_BYTES, KernPageArray_PagePhys(refPageArray.AsPageArray, 1), K2OS_MAPTYPE_KERN_DATA);
                            KernPte_MakePageMap(NULL, kernMapVirt + (K2_VA_MEMPAGE_BYTES * 2), KernPageArray_PagePhys(refPageArray.AsPageArray, 2), K2OS_MAPTYPE_KERN_DATA);

                            K2MEM_Zero((void *)kernMapVirt, 3 * K2_VA_MEMPAGE_BYTES);

                            pCons = (K2OS_MAILBOX_CONSUMER_PAGE *)kernMapVirt;
                            pCons->IxConsumer.mVal = K2OS_MAILBOX_GATE_CLOSED_BIT;

                            pProd = (K2OS_MAILBOX_PRODUCER_PAGE *)(kernMapVirt + K2_VA_MEMPAGE_BYTES);
                            pProd->AvailCount.mVal = K2OS_MAILBOX_MSG_COUNT;

                            K2_CpuWriteBarrier();

                            //
                            // have all resources now
                            //
                            K2MEM_Zero(pMailbox, sizeof(K2OSKERN_OBJ_MAILBOX));
                            pMailbox->Hdr.mObjType = KernObj_Mailbox;
                            K2LIST_Init(&pMailbox->Hdr.RefObjList);
                            pMailbox->mKernVirtAddr = kernMapVirt;
                            KernObj_CreateRef(&pMailbox->RefGate, refGate.AsAny);
                            KernObj_CreateRef(&pMailbox->RefPageArray, refPageArray.AsAny);

                            K2MEM_Zero(pOwner, sizeof(K2OSKERN_OBJ_MAILBOXOWNER));
                            pOwner->Hdr.mObjType = KernObj_MailboxOwner;
                            K2LIST_Init(&pOwner->Hdr.RefObjList);
                            KernObj_CreateRef(&pOwner->RefMailbox, &pMailbox->Hdr);

                            KernObj_CreateRef(&refOwner, &pOwner->Hdr);
                            pOwner = NULL;

                            kernMapVirt = 0;

                        } while (0);

                        KernObj_ReleaseRef(&refPageArray);

                    } while (0);

                    KernObj_ReleaseRef(&refGate);

                } while (0);

                if (K2STAT_IS_ERROR(stat))
                {
                    K2_ASSERT(NULL != pOwner);
                    KernHeap_Free(pOwner);
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2_ASSERT(NULL != pMailslot);
                KernHeap_Free(pMailslot);
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2_ASSERT(NULL != pMailbox);
            KernHeap_Free(pMailbox);
        }
    }

    if (!K2STAT_IS_ERROR(stat))
    {
        K2_ASSERT(NULL != refOwner.AsAny);

        stat = KernToken_Create(refOwner.AsAny, (K2OS_TOKEN *)&tokMailbox);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Thread_SetLastStatus(stat);
        }

        KernObj_ReleaseRef(&refOwner);
    }
    else
    {
        K2_ASSERT(NULL == refOwner.AsAny);
        K2_ASSERT(NULL == tokMailbox);
        K2OS_Thread_SetLastStatus(stat);
    }

    return tokMailbox;
}

BOOL                
K2OS_Mailbox_Recv(
    K2OS_MAILBOX_TOKEN  aTokMailbox,
    K2OS_MSG *          apRetMsg,
    UINT32              aTimeoutMs
)
{
    K2OSKERN_OBJREF                     refOwner;
    K2OSKERN_OBJ_MAILBOX *              pMailbox;
    K2OS_MAILBOX_CONSUMER_PAGE *        pCons;
    K2OS_MAILBOX_PRODUCER_PAGE *        pProd;
    K2OS_MAILBOX_MSGDATA_PAGE const *   pData;
    UINT32                              ixSlot;
    UINT32                              ixWord;
    UINT32                              ixBit;
    UINT32                              tick;
    UINT32                              bitsVal;
    K2STAT                              stat;
    K2OS_WaitResult                     waitResult;
    BOOL                                result;
    UINT32                              nextSlot;
    UINT32                              ixProd;
    BOOL                                disp;
    K2OSKERN_CPUCORE volatile *         pThisCore;
    K2OSKERN_OBJ_THREAD *               pThisThread;
    K2OSKERN_SCHED_ITEM *               pSchedItem;
    K2OS_THREAD_PAGE *                  pThreadPage;

    if ((NULL == aTokMailbox) ||
        (NULL == apRetMsg))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    refOwner.AsAny = NULL;
    stat = KernToken_Translate(aTokMailbox, &refOwner);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return FALSE;
    }

    result = FALSE;

    if (KernObj_MailboxOwner != refOwner.AsAny->mObjType)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
    }
    else
    {
        pMailbox = refOwner.AsMailboxOwner->RefMailbox.AsMailbox;
        pCons = (K2OS_MAILBOX_CONSUMER_PAGE *)pMailbox->mKernVirtAddr;
        pProd = (K2OS_MAILBOX_PRODUCER_PAGE *)(pMailbox->mKernVirtAddr + K2_VA_MEMPAGE_BYTES);
        pData = (K2OS_MAILBOX_MSGDATA_PAGE const *)(pMailbox->mKernVirtAddr + (2 * K2_VA_MEMPAGE_BYTES));

        stat = K2STAT_ERROR_UNKNOWN;

        do {
            ixSlot = pCons->IxConsumer.mVal;
            K2_CpuReadBarrier();

            if (0 != (ixSlot & K2OS_MAILBOX_GATE_CLOSED_BIT))
            {
                //
                // gate is closed
                //
                if (0 == aTimeoutMs)
                {
                    stat = K2STAT_ERROR_TIMEOUT;
                    break;
                }

                tick = K2OS_System_GetMsTick32();

                if (!K2OS_Thread_WaitOne(&waitResult, aTokMailbox, aTimeoutMs))
                {
                    if (waitResult == K2OS_Wait_TimedOut)
                    {
                        stat = K2STAT_ERROR_TIMEOUT;
                    }
                    else
                    {
                        stat = K2STAT_ERROR_WAIT_FAILED;
                    }
                    break;
                }

                tick = K2OS_System_GetMsTick32() - tick;
                if (aTimeoutMs != K2OS_TIMEOUT_INFINITE)
                {
                    if (aTimeoutMs >= tick)
                    {
                        aTimeoutMs -= tick;
                    }
                    else
                    {
                        aTimeoutMs = 0;
                    }
                }
            }
            else
            {
                //
                // gate is open
                //
                ixWord = ixSlot >> 5;
                ixBit = ixSlot & 0x1F;

                bitsVal = pProd->OwnerMask[ixWord].mVal;
                K2_CpuReadBarrier();
                if (0 != (bitsVal & (1 << ixBit)))
                {
                    //
                    // slot is full
                    //
                    nextSlot = (ixSlot + 1) & K2OS_MAILBOX_MSG_IX_MASK;
                    ixProd = pProd->IxProducer.mVal;
                    K2_CpuReadBarrier();
                    if (nextSlot == ixProd)
                    {
                        //
                        // if successful, this receive may make the mailbox empty and close the gate
                        //
                        disp = K2OSKERN_SetIntr(FALSE);
                        K2_ASSERT(disp);

                        pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
                        pThisThread = pThisCore->mpActiveThread;
                        K2_ASSERT(pThisThread->mIsKernelThread);
                        pThreadPage = pThisThread->mpKernRwViewOfThreadPage;

                        pSchedItem = &pThisThread->SchedItem;
                        pSchedItem->mType = KernSchedItem_KernThread_MailboxRecvLast;
                        KernObj_CreateRef(&pSchedItem->ObjRef, &pMailbox->Hdr);

                        KernThread_CallScheduler(pThisCore);

                        // interrupts will be back on again here
                        KernObj_ReleaseRef(&pSchedItem->ObjRef);

                        result = pThisThread->Kern.mSchedCall_Result;
                        if (result)
                        {
                            K2MEM_Copy(apRetMsg, pThreadPage->mMiscBuffer, sizeof(K2OS_MSG));
                            ixSlot = pThreadPage->mSysCall_Arg7_Result0;
                            ixWord = ixSlot >> 5;
                            ixBit = ixSlot & 0x1F;
                        }
                        else
                        {
                            //  gate closed already                             K2STAT_ERROR_TIMEOUT
                            //  no message found with open gate (transitory)    K2STAT_ERROR_EMPTY
                            //  someone else consumed the last message OOB      K2STAT_ERROR_CHANGED
                            //
                            //  in all these cases, go around
                            //
                        }
                    }
                    else
                    {
                        //
                        // if successful, this receive will not make the mailbox empty
                        //
                        if (ixSlot == K2ATOMIC_CompareExchange(&pCons->IxConsumer.mVal, (ixSlot + 1) & K2OS_MAILBOX_MSG_IX_MASK, ixSlot))
                        {
                            //
                            // we captured the message
                            //
                            K2MEM_Copy(apRetMsg, &pData->Msg[ixSlot], sizeof(K2OS_MSG));
                            result = TRUE;
                        }
                    }

                    if (result)
                    {
                        bitsVal = pCons->ReserveMask[ixWord].mVal;
                        K2_CpuReadBarrier();

                        if (0 != (bitsVal & (1 << ixBit)))
                        {
                            KernMailbox_RecvRes(pMailbox, ixSlot);
                            K2ATOMIC_And(&pProd->OwnerMask[ixWord].mVal, ~(1 << ixBit));
                        }
                        else
                        {
                            K2ATOMIC_And(&pProd->OwnerMask[ixWord].mVal, ~(1 << ixBit));
                            K2ATOMIC_Inc((INT32 *)&pProd->AvailCount.mVal);
                        }
                    }
                }
            }
        } while (!result);
    }

    KernObj_ReleaseRef(&refOwner);

    return result;
}

BOOL 
K2OS_Mailbox_Send(
    K2OS_MAILBOX_TOKEN  aTokMailboxOwnerOrSlot,
    K2OS_MSG const *    apMsg
)
{
    K2OSKERN_OBJREF                     refSender;
    K2OSKERN_OBJ_MAILBOX *              pMailbox;
    K2STAT                              stat;
    BOOL                                result;
    BOOL                                disp;
    K2OSKERN_CPUCORE volatile *         pThisCore;
    K2OSKERN_OBJ_THREAD *               pThisThread;
    K2OSKERN_SCHED_ITEM *               pSchedItem;
    BOOL                                doSignal;

    if ((NULL == aTokMailboxOwnerOrSlot) ||
        (NULL == apMsg))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    refSender.AsAny = NULL;
    stat = KernToken_Translate(aTokMailboxOwnerOrSlot, &refSender);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
        return FALSE;
    }

    result = FALSE;

    if ((KernObj_Mailslot != refSender.AsAny->mObjType) &&
        (KernObj_MailboxOwner != refSender.AsAny->mObjType))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
    }
    else
    {
        do {
            if (KernObj_Mailslot == refSender.AsAny->mObjType)
            {
                pMailbox = refSender.AsMailslot->RefMailbox.AsMailbox;
                if (pMailbox->mOwnerDead)
                {
                    K2OS_Thread_SetLastStatus(K2STAT_ERROR_CLOSED);
                    break;
                }
            }
            else
            {
                pMailbox = refSender.AsMailboxOwner->RefMailbox.AsMailbox;
            }

            doSignal = FALSE;
            stat = KernMailbox_Deliver(
                pMailbox,
                apMsg,
                TRUE,   // do alloc
                NULL,   // no reserve user
                &doSignal);
            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_Thread_SetLastStatus(stat);
            }
            else
            {
                result = TRUE;
                if (doSignal)
                {
                    disp = K2OSKERN_SetIntr(FALSE);
                    K2_ASSERT(disp);

                    pThisCore = K2OSKERN_GET_CURRENT_CPUCORE;
                    pThisThread = pThisCore->mpActiveThread;
                    K2_ASSERT(pThisThread->mIsKernelThread);

                    pSchedItem = &pThisThread->SchedItem;
                    pSchedItem->mType = KernSchedItem_KernThread_MailboxSentFirst;
                    KernObj_CreateRef(&pSchedItem->ObjRef, &pMailbox->Hdr);

                    KernThread_CallScheduler(pThisCore);

                    // interrupts will be back on again here
                    KernObj_ReleaseRef(&pSchedItem->ObjRef);
                } while (0);
            }
        } while (0);
    }

    KernObj_ReleaseRef(&refSender);

    return result;
}

