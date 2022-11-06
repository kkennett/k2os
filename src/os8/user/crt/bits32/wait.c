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

UINT_PTR 
K2OS_Wait_OnMailbox(
    K2OS_MAILBOX    aMailbox,
    K2OS_MSG *      apRetMsg,
    UINT_PTR        aTimeoutMs
)
{
    return K2OS_Wait_OnMailboxAndMany(aMailbox, apRetMsg, 0, NULL, aTimeoutMs);
}

UINT_PTR
K2_CALLCONV_REGS
K2OS_Wait_One(
    K2OS_WAITABLE_TOKEN aToken,
    UINT_PTR            aTimeoutMs
)
{
    return K2OS_Wait_Many(1, &aToken, FALSE, aTimeoutMs);
}

UINT_PTR
K2OS_Wait_Many(
    UINT_PTR                    aCount,
    K2OS_WAITABLE_TOKEN const * apWaitableTokens,
    BOOL                        aWaitAll,
    UINT_PTR                    aTimeoutMs
)
{
    UINT32                  threadIx;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    UINT_PTR                ix;

    if (aCount > K2OS_THREAD_WAIT_MAX_ITEMS)
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if (aCount > 0)
    {
        threadIx = CRT_GET_CURRENT_THREAD_INDEX;
        pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (threadIx * K2_VA_MEMPAGE_BYTES));

        if ((0 != aTimeoutMs) && (0 != pThreadPage->mCsDepth))
        {
            CrtDbg_Printf("*** Process %d Thread %d waiting on object(s) while inside critical section(s)\n", gProcessId, threadIx);
            K2_ASSERT(0);
        }

        for (ix = 0; ix < aCount; ix++)
        {
            if (NULL == apWaitableTokens[ix])
            {
                K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
                return K2OS_THREAD_WAIT_FAILED_0 + ix;
            }
            pThreadPage->mWaitToken[ix] = apWaitableTokens[ix];
        }
    }

    return CrtKern_SysCall3(K2OS_SYSCALL_ID_THREAD_WAIT, aCount, aWaitAll, aTimeoutMs);
}

UINT_PTR
K2OS_Wait_OnMailboxAndOne(
    K2OS_MAILBOX        aMailbox,
    K2OS_MSG *          apRetMsg,
    K2OS_WAITABLE_TOKEN aToken,
    UINT_PTR            aTimeoutMs
)
{
    return K2OS_Wait_OnMailboxAndMany(aMailbox, apRetMsg, 1, &aToken, aTimeoutMs);
}

UINT_PTR
K2OS_Wait_OnMailboxAndMany(
    K2OS_MAILBOX                aMailbox,
    K2OS_MSG *                  apRetMsg,
    UINT_PTR                    aCount,
    K2OS_WAITABLE_TOKEN const * apWaitableTokens,
    UINT_PTR                    aTimeoutMs
)
{
    UINT32                  threadIx;
    K2OS_USER_THREAD_PAGE * pThreadPage;
    UINT_PTR                ix;
    UINT_PTR                waitResult;
    K2OS_MAILBOX_OWNER *    pMailboxOwner;
    UINT32                  slotIndex;
    UINT32                  exchangeVal;
    UINT32                  valResult;
    UINT32                  wordIndex;
    UINT32                  bitIndex;
    UINT32                  ownerMask;
    UINT32                  reserveBit;
    BOOL                    doWait;
    BOOL                    gotMsg;

    if ((NULL == aMailbox) ||
        (NULL == apRetMsg) || 
        (aCount > K2OS_THREAD_WAIT_MAX_ITEMS) ||
        ((aCount > 0) && (NULL == apWaitableTokens)))
    {
        K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_ARGUMENT);
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    threadIx = CRT_GET_CURRENT_THREAD_INDEX;
    pThreadPage = (K2OS_USER_THREAD_PAGE *)(K2OS_UVA_TLSAREA_BASE + (threadIx * K2_VA_MEMPAGE_BYTES));

    if (aCount > 0)
    {
        for (ix = 0; ix < aCount; ix++)
        {
            if (NULL == apWaitableTokens[ix])
            {
                K2OS_Thread_SetLastStatus(K2STAT_ERROR_BAD_TOKEN);
                return K2OS_THREAD_WAIT_FAILED_0 + ix;
            }
            pThreadPage->mWaitToken[ix] = apWaitableTokens[ix];
        }
    }

    pMailboxOwner = CrtMailboxRef_FindAddUse(aMailbox);
    if (NULL == pMailboxOwner)
        return K2OS_Thread_GetLastStatus();

    //
    // check value at current consume count to see if it is 'full'
    //
    gotMsg = FALSE;

    do
    {
        slotIndex = pMailboxOwner->mIxConsumer;
        K2_CpuReadBarrier();

        wordIndex = (slotIndex & 0xFF) >> 5;
        bitIndex = slotIndex & 0x1F;

        if (0 != (pMailboxOwner->mFlagBitArray[wordIndex] & (1 << bitIndex)))
        {
            // user-is-owner flag set. try to take this slot by incrementing
            // the consumer count
            gotMsg = TRUE;
            exchangeVal = ((slotIndex + 1) & 0xFF) | 0x80000000;
            valResult = K2ATOMIC_CompareExchange(&pMailboxOwner->mIxConsumer, exchangeVal, slotIndex);
            if (valResult == slotIndex)
            {
                waitResult = K2OS_THREAD_WAIT_MAILBOX_SIGNALLED;
                break;
            }
            // else try again from top of loop
            gotMsg = FALSE;
        }
        else
        {
//            CrtDbg_Printf("%d - Slot index %08X empty\n", gProcessId, slotIndex);
            //
            // next slot is empty, so nothing to consume.  we may need to block this thread
            //
            if (slotIndex & 0x80000000)
            {
                exchangeVal = slotIndex & 0xFF;
                valResult = K2ATOMIC_CompareExchange(&pMailboxOwner->mIxConsumer, exchangeVal, slotIndex);
                if (valResult == slotIndex)
                {
                    //
                    // we successfully cleared the active flag
                    //
                    if (0 != (pMailboxOwner->mFlagBitArray[wordIndex] & (1 << bitIndex)))
                    {
                        // 
                        // slot is full now (set in between first check and after we cleared the active flag
                        // so make sure our active flag is set and go around again
                        //
                        do
                        {
                            slotIndex = pMailboxOwner->mIxConsumer;
                            K2_CpuReadBarrier();

                            if (slotIndex & 0x80000000)
                            {
                                //
                                // somebody else set the active flag so we can stop trying to do that
                                //
                                break;
                            }
                            exchangeVal = slotIndex | 0x80000000;
                            valResult = K2ATOMIC_CompareExchange(&pMailboxOwner->mIxConsumer, exchangeVal, slotIndex);
                        } while (valResult != slotIndex);

                        //
                        // active flag is now set. go around again
                        //
                        doWait = FALSE;
                    }
                    else
                    {
                        //
                        // still nothing for us to consume after we cleared the active flag. so wait
                        //
                        doWait = TRUE;
                    }
                }
                else
                {
                    //
                    // we couldn't clear the active flag.  go around again
                    //
                    doWait = FALSE;
                }
            }
            else
            {
                //
                // active flag is already clear. just wait
                //
                doWait = TRUE;
            }

            if (doWait)
            {
                if (0 == aTimeoutMs)
                {
                    waitResult = K2STAT_ERROR_TIMEOUT;
                    break;
                }

                if ((0 != aTimeoutMs) && (0 != pThreadPage->mCsDepth))
                {
                    CrtDbg_Printf("*** Process %d Thread %d waiting on mailbox,object(s) while inside critical section(s)\n", gProcessId, threadIx);
                }

                pThreadPage->mWaitToken[aCount] = pMailboxOwner->mMailboxToken;

                waitResult = CrtKern_SysCall3(K2OS_SYSCALL_ID_THREAD_WAIT, aCount + 1, FALSE, aTimeoutMs);
                if (K2STAT_IS_ERROR(waitResult) ||
                    (waitResult > (K2OS_THREAD_WAIT_SIGNALLED_0 + aCount)))
                {
                    //
                    // wait failed
                    //
                    break;
                }
                if (waitResult != (K2OS_THREAD_WAIT_SIGNALLED_0 + aCount))
                {
                    //
                    // some other non-messagebox event signalled
                    // gotMsg remains set zero
                    //
                    break;
                }
            }
        }

    } while (1);

    if (gotMsg)
    {
        K2_ASSERT(K2OS_THREAD_WAIT_MAILBOX_SIGNALLED == waitResult);

        slotIndex &= 0xFF;

        K2MEM_Copy(apRetMsg, &pMailboxOwner->mpRoMsgPage[slotIndex], sizeof(K2OS_MSG));

//        CrtDbg_Printf("%d - Retrieved %08X from index %08X\n", gProcessId, apRetMsg->mControl, slotIndex);

        ownerMask = pMailboxOwner->mFlagBitArray[wordIndex + K2OS_MAILBOX_OWNER_FLAG_DWORDS];
        reserveBit = (ownerMask & (1 << bitIndex));

        if (0 != reserveBit)
        {
            //
            // reserve is available again.  do the slot set via system call so the 
            // reserver knows that there is a slot free again
            //
            CrtKern_SysCall2(K2OS_SYSCALL_ID_MAILBOX_RECV_RES, (UINT_PTR)pMailboxOwner->mMailboxToken, slotIndex);
        }
        else
        {
            //
            // set the slot to 'empty' status so producer can produce into it again
            //
            do
            {
                ownerMask = pMailboxOwner->mFlagBitArray[wordIndex];
                K2_CpuReadBarrier();
                K2_ASSERT(0 != (ownerMask & (1 << bitIndex)));
                exchangeVal = ownerMask & ~(1 << bitIndex);
                valResult = K2ATOMIC_CompareExchange(&pMailboxOwner->mFlagBitArray[wordIndex], exchangeVal, ownerMask);
            } while (valResult != ownerMask);

            // message was not sent from the reserve, so increment the available count
            do
            {
                valResult = pMailboxOwner->mAvail;
                K2_ASSERT(valResult < 0x100);
            } while (valResult != K2ATOMIC_CompareExchange(&pMailboxOwner->mAvail, valResult + 1, valResult));
        }
    }

    CrtMailboxRef_DecUse(aMailbox, pMailboxOwner, FALSE);

    return waitResult;
}
