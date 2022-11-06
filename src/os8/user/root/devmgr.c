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
#include "root.h"

K2OS_CRITSEC        gTimerSec;
K2LIST_ANCHOR       gTimerQueue;

K2OS_CRITSEC        gEventSec;
K2LIST_ANCHOR       gEventQueue;

K2OS_NOTIFY_TOKEN   gKickNotifyToken;
UINT64              gLastTick;

void
DevMgr_Init(
     void
)
{
    BOOL ok;

    ok = K2OS_CritSec_Init(&gTimerSec);
    K2_ASSERT(ok);
    K2LIST_Init(&gTimerQueue);

    ok = K2OS_CritSec_Init(&gEventSec);
    K2_ASSERT(ok);
    K2LIST_Init(&gEventQueue);

    gKickNotifyToken = K2OS_Notify_Create(FALSE);
    K2_ASSERT(NULL != gKickNotifyToken);
}

BOOL
DevMgr_QueueEvent(
    DEVNODE *           apNode,
    DevNodeEventType    aType,
    UINT_PTR            aArg
)
{
    DEVEVENT *  pEvent;
    BOOL        sendKick;

    pEvent = (DEVEVENT *)K2OS_Heap_Alloc(sizeof(DEVEVENT));
    if (NULL == pEvent)
    {
        Debug_Printf("*** DevMgr_QueueEvent(%08X, %d, %08X) memory alloc failure\n", apNode, aType, aArg);
        return FALSE;
    }

    pEvent->mType = aType;

    pEvent->mArg = aArg;

    pEvent->DevNodeRef.mpDevNode = NULL;
    Dev_CreateRef(&pEvent->DevNodeRef, apNode);

    pEvent->mPending = TRUE;

    sendKick = FALSE;

    K2OS_CritSec_Enter(&gEventSec);
    
    K2LIST_AddAtTail(&gEventQueue, &pEvent->ListLink);
    
    if (gEventQueue.mNodeCount == 1)
        sendKick = TRUE;
    
    K2OS_CritSec_Leave(&gEventSec);

    if ((sendKick) && (gMainThreadId != K2OS_Thread_GetId()))
    {
        K2OS_Notify_Signal(gKickNotifyToken);
    }

    return TRUE;
}

void
DevMgr_NodeLocked_AddTimer(
    DEVNODE *   apNode,
    UINT_PTR    aTimeoutMs
)
{
    K2LIST_LINK *   pListLink;
    DEVNODE *       pOtherNode;
    BOOL            sendKick;

    K2_ASSERT(NULL != apNode);
    K2_ASSERT(0 != aTimeoutMs);
    K2_ASSERT(K2OS_TIMEOUT_INFINITE != aTimeoutMs);

    sendKick = FALSE;

    K2OS_CritSec_Enter(&gTimerSec);

    K2_ASSERT(!apNode->InTimerSec.mOnQueue);

    if (0 == gTimerQueue.mNodeCount)
    {
        apNode->InTimerSec.mRemainingMs = aTimeoutMs;
        K2LIST_AddAtHead(&gTimerQueue, &apNode->InTimerSec.ListLink);
        //
        // first node on queue - kick dev mgr thread to start keeping time
        //
        sendKick = TRUE;
    }
    else
    {
        pListLink = gTimerQueue.mpHead;
        do {
            pOtherNode = K2_GET_CONTAINER(DEVNODE, pListLink, InTimerSec.ListLink);
            K2_ASSERT(pOtherNode->InTimerSec.mOnQueue);
            if (aTimeoutMs < pOtherNode->InTimerSec.mRemainingMs)
            {
                if (NULL == pOtherNode->InTimerSec.ListLink.mpPrev)
                {
                    //
                    // head node on queue changed - kick dev mgr thread to resync
                    //
                    sendKick = TRUE;
                }
                pOtherNode->InTimerSec.mRemainingMs -= aTimeoutMs;
                K2LIST_AddBefore(&gTimerQueue, &apNode->InTimerSec.ListLink, &pOtherNode->InTimerSec.ListLink);
                break;
            }
            aTimeoutMs -= pOtherNode->InTimerSec.mRemainingMs;
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);

        apNode->InTimerSec.mRemainingMs = aTimeoutMs;

        if (NULL == pListLink)
        {
            K2LIST_AddAtTail(&gTimerQueue, &apNode->InTimerSec.ListLink);
        }
    }

    apNode->InTimerSec.mOnQueue = TRUE;
    Dev_CreateRef(&apNode->InTimerSec.RefToSelf_TimerActive, apNode);

    K2OS_CritSec_Leave(&gTimerSec);

    if ((sendKick) && (gMainThreadId != K2OS_Thread_GetId()))
    {
        K2OS_Notify_Signal(gKickNotifyToken);
    }
}

void        
DevMgr_NodeLocked_DelTimer(
    DEVNODE *   apNode
)
{
    K2LIST_LINK *   pListLink;
    DEVNODE *       pOtherNode;
    BOOL            sendKick;
    DEVNODE_REF     tempRef;

    K2_ASSERT(NULL != apNode);

    sendKick = FALSE;

    tempRef.mpDevNode = NULL;

    K2OS_CritSec_Enter(&gTimerSec);

    if (apNode->InTimerSec.mOnQueue)
    {
        pListLink = apNode->InTimerSec.ListLink.mpNext;
        if (NULL != pListLink)
        {
            pOtherNode = K2_GET_CONTAINER(DEVNODE, pListLink, InTimerSec.ListLink);
            pOtherNode->InTimerSec.mRemainingMs += apNode->InTimerSec.mRemainingMs;
        }
        if (NULL == apNode->InTimerSec.ListLink.mpPrev)
        {
            //
            // head node on the timer queue is changing - kick dev mgr thread to resync
            //
            sendKick = TRUE;
        }
        K2LIST_Remove(&gTimerQueue, &apNode->InTimerSec.ListLink);
        apNode->InTimerSec.mOnQueue = FALSE;

        //
        // reference released outside of timer sec
        //
        Dev_CreateRef(&tempRef, apNode);
        Dev_ReleaseRef(&apNode->InTimerSec.RefToSelf_TimerActive);
    }

    K2OS_CritSec_Leave(&gTimerSec);

    if (NULL != tempRef.mpDevNode)
    {
        //
        // reference released outside of timer sec
        //
        Dev_ReleaseRef(&tempRef);
    }

    if ((sendKick) && (gMainThreadId != K2OS_Thread_GetId()))
    {
        K2OS_Notify_Signal(gKickNotifyToken);
    }
}

void
DevMgr_RecvMsg(
    K2OS_MSG const * apMsg
)
{
    switch (apMsg->mControl)
    {
    case K2OS_SYSTEM_MSG_IPC_REQUEST:
        //
        // attempt to connect to us over IPC
        //
        // msg.mPayload[0] is interface publish context
        // msg.mPayload[1] is requestor process id
        // msg.mPayload[2] is global request id
        //
        K2OS_Ipc_RejectRequest(apMsg->mPayload[2], K2STAT_ERROR_REJECTED);
        break;

    default:
        Debug_Printf("DevMgr_RecvMsg(%08X,%08X,%08X,%08X)\n", apMsg->mControl, apMsg->mPayload[0], apMsg->mPayload[1], apMsg->mPayload[2]);
        break;
    }
}

void
DevMgr_Run(
    void
)
{
    K2OS_MSG        msg;
    UINT_PTR        waitResult;
    UINT64          newTick;
    UINT64          deltaTick;
    UINT32          deltaMs32;
    UINT_PTR        timeOut;
    DEVNODE *       pDevNode;
    DEVNODE_REF     tempRef;
    K2LIST_LINK *   pListLink;
    K2LIST_ANCHOR   eventList;
    DEVEVENT *      pEvent;

    Debug_Printf("DevMgr_Run()\n");
    K2OS_System_GetHfTick(&gLastTick);

    do {
        K2OS_System_GetHfTick(&newTick);
        deltaTick = newTick - gLastTick;
        deltaMs32 = K2OS_System_MsTick32FromHfTick(&deltaTick);
        if (0 != deltaMs32)
        {
            //
            // some # of milliseconds passed.  convert ms back to 
            // ticks and add to last tick, so we don't lose ticks
            // in the conversion
            //
            K2OS_System_HfTickFromMsTick32(&deltaTick, deltaMs32);
            gLastTick += deltaTick;
        }

        do {
            K2OS_CritSec_Enter(&gTimerSec);

            if (0 == gTimerQueue.mNodeCount)
            {
                timeOut = K2OS_TIMEOUT_INFINITE;
                break;
            }

            pDevNode = K2_GET_CONTAINER(DEVNODE, gTimerQueue.mpHead, InTimerSec.ListLink);
            K2_ASSERT(pDevNode->InTimerSec.mOnQueue);

            if (pDevNode->InTimerSec.mRemainingMs > deltaMs32)
            {
                pDevNode->InTimerSec.mRemainingMs -= deltaMs32;
                timeOut = pDevNode->InTimerSec.mRemainingMs;
                break;
            }

            deltaMs32 -= pDevNode->InTimerSec.mRemainingMs;
            
            pDevNode->InTimerSec.mRemainingMs = 0;
            
            K2LIST_Remove(&gTimerQueue, &pDevNode->InTimerSec.ListLink);
            
            pDevNode->InTimerSec.mOnQueue = FALSE;

            tempRef.mpDevNode = NULL;

            DevMgr_QueueEvent(pDevNode, DevNodeEvent_TimerExpired, pDevNode->InTimerSec.mArg);

            //
            // release timer reference outside of timer sec
            //
            Dev_CreateRef(&tempRef, pDevNode);
            Dev_ReleaseRef(&pDevNode->InTimerSec.RefToSelf_TimerActive);

            K2OS_CritSec_Leave(&gTimerSec);

            //
            // release timer reference outside of timer sec
            //
            Dev_ReleaseRef(&tempRef);

        } while (1);

        K2OS_CritSec_Leave(&gTimerSec);

        //
        // check for events here
        //
        K2OS_CritSec_Enter(&gEventSec);
        K2MEM_Copy(&eventList, &gEventQueue, sizeof(K2LIST_ANCHOR));
        K2LIST_Init(&gEventQueue);
        K2OS_CritSec_Leave(&gEventSec);

        if (eventList.mNodeCount == 0)
        {
            //
            // no events to process. wait for something to happen
            //
            waitResult = K2OS_Wait_OnMailboxAndOne(gMainMailbox, &msg, gKickNotifyToken, timeOut);
            if (waitResult == K2OS_THREAD_WAIT_MAILBOX_SIGNALLED)
            {
                if (!K2OS_System_ProcessMsg(&msg))
                {
                    //
                    // received a non-system message
                    //
                    DevMgr_RecvMsg(&msg);
                }
            }
            else if (waitResult != K2STAT_ERROR_TIMEOUT)
            {
                if (waitResult != K2OS_THREAD_WAIT_SIGNALLED_0)
                {
                    Debug_Printf("*** Root wait on mailbox failed with unexpected status 0x%08X\n", waitResult);
                    break;
                }
            }
        }
        else
        {
            //
            // events occurred. do callbacks then go process timer queue again
            // don't worry about maintaining the list as we are just chewing through it
            //
            pListLink = eventList.mpHead;
            do {
                pEvent = K2_GET_CONTAINER(DEVEVENT, pListLink, ListLink);
                K2_ASSERT(pEvent->mPending);
                pListLink = pListLink->mpNext;
                pEvent->mPending = FALSE;

                K2OS_CritSec_Enter(&pEvent->DevNodeRef.mpDevNode->Sec);
                Dev_NodeLocked_OnEvent(pEvent->DevNodeRef.mpDevNode, pEvent->mType, pEvent->mArg);
                K2OS_CritSec_Leave(&pEvent->DevNodeRef.mpDevNode->Sec);

                Dev_ReleaseRef(&pEvent->DevNodeRef);

                K2OS_Heap_Free(pEvent);

            } while (NULL != pListLink);
        }
    } while (1);
}