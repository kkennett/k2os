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

typedef struct _CRT_MAIL_TRACK CRT_MAIL_TRACK;
struct _CRT_MAIL_TRACK
{
    K2TREE_NODE     TreeNode;   // mUserVal is token
    UINT32          mVirtAddr;  
    BOOL            mIsSlot;
};

K2TREE_ANCHOR sgTrackTree;
K2OS_CRITSEC  sgTrackSec;

void
CrtMail_Init(
    void
)
{
    BOOL ok;
    ok = K2OS_CritSec_Init(&sgTrackSec);
    if (!ok)
    {
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
        K2OS_RaiseException(K2STAT_EX_LOGIC);
    }
    K2TREE_Init(&sgTrackTree, NULL);
}

BOOL
CrtMail_TokenDestroy(
    K2OS_TOKEN aToken
)
{
    CRT_MAIL_TRACK *pTrack;
    K2TREE_NODE *   pTreeNode;
    BOOL            ok;

    K2OS_CritSec_Enter(&sgTrackSec);
    pTreeNode = K2TREE_Find(&sgTrackTree, (UINT32)aToken);
    if (NULL != pTreeNode)
    {
        K2TREE_Remove(&sgTrackTree, pTreeNode);
    }
    K2OS_CritSec_Leave(&sgTrackSec);

    if (NULL != pTreeNode)
    {
        pTrack = K2_GET_CONTAINER(CRT_MAIL_TRACK, pTreeNode, TreeNode);
        ok = CrtKern_SysCall1(K2OS_SYSCALL_ID_TOKEN_DESTROY, (UINT32)aToken);
        K2OS_Heap_Free(pTrack);
        return ok;
    }

    return FALSE;
}

K2OS_MAILBOX_TOKEN
K2OS_Mailbox_Create(
    void
)
{
    K2OS_THREAD_PAGE *  pThreadPage;
    CRT_MAIL_TRACK *    pTrackBox;

    pTrackBox = (CRT_MAIL_TRACK *)K2OS_Heap_Alloc(sizeof(CRT_MAIL_TRACK));
    if (NULL == pTrackBox)
        return NULL;

    K2MEM_Zero(pTrackBox, sizeof(CRT_MAIL_TRACK));

    pTrackBox->TreeNode.mUserVal = CrtKern_SysCall1(K2OS_SYSCALL_ID_MAILBOX_CREATE, 0);

    if (0 == pTrackBox->TreeNode.mUserVal)
    {
        K2OS_Heap_Free(pTrackBox);
        return NULL;
    }

    pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));

    pTrackBox->mIsSlot = FALSE;
    pTrackBox->mVirtAddr = pThreadPage->mSysCall_Arg7_Result0;
    K2_ASSERT(0 != pTrackBox->mVirtAddr);

    K2OS_CritSec_Enter(&sgTrackSec);
    K2TREE_Insert(&sgTrackTree, pTrackBox->TreeNode.mUserVal, &pTrackBox->TreeNode);
    K2OS_CritSec_Leave(&sgTrackSec);

    return (K2OS_MAILBOX_TOKEN)pTrackBox->TreeNode.mUserVal;
}

BOOL
K2OS_Mailbox_Recv(
    K2OS_MAILBOX_TOKEN  aTokMailbox,
    K2OS_MSG *          apRetMsg,
    UINT32              aTimeoutMs
)
{
    CRT_MAIL_TRACK *                    pTrackBox;
    K2TREE_NODE *                       pTreeNode;
    UINT32                              virtBase;
    K2OS_MAILBOX_CONSUMER_PAGE *        pCons;
    K2OS_MAILBOX_PRODUCER_PAGE *        pProd;
    K2OS_MAILBOX_MSGDATA_PAGE const *   pData;
    UINT32                              ixSlot;
    UINT32                              ixWord;
    UINT32                              ixBit;
    UINT32                              tick;
    UINT32                              bitsVal;
    UINT32                              nextSlot;
    UINT32                              ixProd;
    K2OS_WaitResult                     waitResult;
    K2OS_THREAD_PAGE *                  pThreadPage;
    BOOL                                result;

    if ((NULL == aTokMailbox) || (NULL == apRetMsg))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pTrackBox = NULL;
    virtBase = 0;

    K2OS_CritSec_Enter(&sgTrackSec);
    pTreeNode = K2TREE_Find(&sgTrackTree, (UINT32)aTokMailbox);
    if (NULL != pTreeNode)
    {
        pTrackBox = K2_GET_CONTAINER(CRT_MAIL_TRACK, pTreeNode, TreeNode);
        if (pTrackBox->mIsSlot)
        {
            pTrackBox = NULL;
        }
        else
        {
            virtBase = pTrackBox->mVirtAddr;
        }
    }
    K2OS_CritSec_Leave(&sgTrackSec);

    // track content may have disappeared here if somebody deleted the token!

    if (NULL == pTrackBox)
    {
        if (NULL == pTreeNode)
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        }
        else
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        }
        return FALSE;
    }

    K2_ASSERT(0 != virtBase);

    pCons = (K2OS_MAILBOX_CONSUMER_PAGE *)virtBase;
    pProd = (K2OS_MAILBOX_PRODUCER_PAGE *)(virtBase + K2_VA_MEMPAGE_BYTES);
    pData = (K2OS_MAILBOX_MSGDATA_PAGE const *)(virtBase + (2 * K2_VA_MEMPAGE_BYTES));

    //
    // if somebody deleted the token for the mailbox and it was removed 
    // from the process address space this stuff will crash the process
    //

    result = FALSE;

    do {
        ixSlot = pCons->IxConsumer.mVal;
        K2_CpuReadBarrier();

        if (0 != (ixSlot & K2OS_MAILBOX_GATE_CLOSED_BIT))
        {
            if (0 == aTimeoutMs)
            {
                K2OS_Thread_SetLastStatus(K2STAT_ERROR_TIMEOUT);
                return FALSE;
            }

            tick = K2OS_System_GetMsTick32();

            if (!K2OS_Thread_WaitOne(&waitResult, aTokMailbox, aTimeoutMs))
            {
                if (waitResult == K2OS_Wait_TimedOut)
                {
                    K2OS_Thread_SetLastStatus(K2STAT_ERROR_TIMEOUT);
                    break;
                }
                K2_ASSERT(K2STAT_IS_ERROR(K2OS_Thread_GetLastStatus()));
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
            // go around
        }
        else
        {
            // gate is open
            ixWord = ixSlot >> 5;
            ixBit = ixSlot & 0x1F;

            bitsVal = pProd->OwnerMask[ixWord].mVal;
            K2_CpuReadBarrier();
            if (0 != (bitsVal & (1 << ixBit)))
            {
                //
                // slot has a message in it. try to capture it
                //
                nextSlot = (ixSlot + 1) & K2OS_MAILBOX_MSG_IX_MASK;
                ixProd = pProd->IxProducer.mVal;
                K2_CpuReadBarrier();
                if (nextSlot == ixProd)
                {
                    //
                    // if successful, this receive may make the mailbox empty and close the gate
                    //
                    result = CrtKern_SysCall1(K2OS_SYSCALL_ID_MAILBOXOWNER_RECVLAST, (UINT32)aTokMailbox);
                    if (result)
                    {
                        pThreadPage = (K2OS_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (CRT_GET_CURRENT_THREAD_INDEX * K2_VA_MEMPAGE_BYTES));
                        K2MEM_Copy(apRetMsg, pThreadPage->mMiscBuffer, sizeof(K2OS_MSG));

                        // where did we actually receive from?
                        ixSlot = pThreadPage->mSysCall_Arg7_Result0;
                        ixWord = ixSlot >> 5;
                        ixBit = ixSlot & 0x1F;
                        result = TRUE;
                    }
                    else
                    {
                        //
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
                    if (ixSlot == K2ATOMIC_CompareExchange(&pCons->IxConsumer.mVal, nextSlot, ixSlot))
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
                        // received from a reserved slot.
                        CrtKern_SysCall2(K2OS_SYSCALL_ID_MAILBOXOWNER_RECVRES, (UINT32)aTokMailbox, ixSlot);
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

    return result;
}

BOOL
K2OS_Mailbox_Send(
    K2OS_MAILBOX_TOKEN  aTokMailboxOrSlot,
    K2OS_MSG const *    apMsg
)
{
    CRT_MAIL_TRACK *                    pTrackSlot;
    K2TREE_NODE *                       pTreeNode;
    UINT32                              virtBase;
    K2OS_MAILBOX_CONSUMER_PAGE const *  pCons;
    K2OS_MAILBOX_PRODUCER_PAGE *        pProd;
    K2OS_MAILBOX_MSGDATA_PAGE *         pData;
    UINT32                              ixSlot;
    UINT32                              ixWord;
    UINT32                              ixBit;
    UINT32                              ixCons;
    UINT32                              bitsVal;

    if ((NULL == aTokMailboxOrSlot) || (NULL == apMsg))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    pTrackSlot = NULL;
    virtBase = 0;

    K2OS_CritSec_Enter(&sgTrackSec);

    pTreeNode = K2TREE_Find(&sgTrackTree, (UINT32)aTokMailboxOrSlot);
    if (NULL != pTreeNode)
    {
        pTrackSlot = K2_GET_CONTAINER(CRT_MAIL_TRACK, pTreeNode, TreeNode);
        virtBase = pTrackSlot->mVirtAddr;
    }
    else
    {
        //
        // not found. this must be a mailslot.  
        // try to track it and see if the kernel lets us
        //
        pTrackSlot = (CRT_MAIL_TRACK *)K2OS_Heap_Alloc(sizeof(CRT_MAIL_TRACK));
        if (NULL == pTrackSlot)
        {
            K2OS_CritSec_Leave(&sgTrackSec);
            return FALSE;
        }

        pTrackSlot->mVirtAddr = CrtKern_SysCall1(K2OS_SYSCALL_ID_MAILSLOT_GET, (UINT32)aTokMailboxOrSlot);
        if (0 == pTrackSlot->mVirtAddr)
        {
            K2OS_CritSec_Leave(&sgTrackSec);
            K2OS_Heap_Free(pTrackSlot);
            return FALSE;
        }

        pTrackSlot->mIsSlot = TRUE;
        pTrackSlot->TreeNode.mUserVal = (UINT32)aTokMailboxOrSlot;

        K2TREE_Insert(&sgTrackTree, pTrackSlot->TreeNode.mUserVal, &pTrackSlot->TreeNode);

        virtBase = pTrackSlot->mVirtAddr;
    }
    K2OS_CritSec_Leave(&sgTrackSec);

    // track content may have disappeared here if somebody deleted the token!

    if (NULL == pTrackSlot)
    {
        if (NULL == pTreeNode)
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_NOT_FOUND);
        }
        else
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        }
        return FALSE;
    }

    K2_ASSERT(0 != virtBase);

    pCons = (K2OS_MAILBOX_CONSUMER_PAGE const *)virtBase;
    pProd = (K2OS_MAILBOX_PRODUCER_PAGE *)(virtBase + K2_VA_MEMPAGE_BYTES);
    pData = (K2OS_MAILBOX_MSGDATA_PAGE *)(virtBase + (2 * K2_VA_MEMPAGE_BYTES));

    do {
        ixCons = pCons->IxConsumer.mVal;
        K2_CpuReadBarrier();
        if (ixCons == 0xFFFFFFFF)
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_CLOSED);
            return FALSE;
        }

        ixSlot = pProd->AvailCount.mVal;
        K2_CpuReadBarrier();
        if (0 == ixSlot)
        {
            K2OS_Thread_SetLastStatus(K2STAT_ERROR_FULL);
            return FALSE;
        }

    } while (ixSlot != K2ATOMIC_CompareExchange(&pProd->AvailCount.mVal, ixSlot - 1, ixSlot));

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
    bitsVal = pProd->OwnerMask[ixWord].mVal;
    K2_CpuReadBarrier();
    K2_ASSERT(0 != (bitsVal & (1 << ixBit)));

    K2MEM_Copy(&pData->Msg[ixSlot], apMsg, sizeof(K2OS_MSG));
    K2_CpuWriteBarrier();

    K2ATOMIC_Or(&pProd->OwnerMask[ixWord].mVal, (1 << ixBit));

    ixCons = pCons->IxConsumer.mVal;
    K2_CpuReadBarrier();
    if (ixCons & K2OS_MAILBOX_GATE_CLOSED_BIT)
    {
        CrtKern_SysCall1(K2OS_SYSCALL_ID_MAILBOX_SENTFIRST, (UINT32)aTokMailboxOrSlot);
    }

    return TRUE;
}

