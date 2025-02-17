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

#include "k2osexec.h"

K2OSKERN_SEQLOCK    gTimerSeqLock;
K2LIST_ANCHOR       gTimerQueue;

K2OSKERN_SEQLOCK    gEventSeqLock;
K2LIST_ANCHOR       gEventQueue;

K2OS_SIGNAL_TOKEN   gKickNotifyToken;

void
DevMgr_Init(
    void
)
{
    K2OSKERN_SeqInit(&gTimerSeqLock);
    K2LIST_Init(&gTimerQueue);

    K2OSKERN_SeqInit(&gEventSeqLock);
    K2LIST_Init(&gEventQueue);

    gKickNotifyToken = K2OS_Notify_Create(FALSE);
    K2_ASSERT(NULL != gKickNotifyToken);
}

BOOL
DevMgr_QueueEvent(
    DEVNODE *           apNode,
    DevNodeEventType    aType,
    UINT32              aArg
)
{
    DEVEVENT *  pEvent;
    BOOL        disp;
    BOOL        sendKick;

    pEvent = (DEVEVENT *)K2OS_Heap_Alloc(sizeof(DEVEVENT));
    if (NULL == pEvent)
    {
        K2OSKERN_Debug("*** DevMgr_QueueEvent(%08X, %d, %08X) memory alloc failure\n", apNode, aType, aArg);
        return FALSE;
    }

    pEvent->mDevNodeEventType = aType;

    pEvent->mArg = aArg;

    pEvent->DevNodeRef.mpDevNode = NULL;
    Dev_CreateRef(&pEvent->DevNodeRef, apNode);

    pEvent->mPending = TRUE;

    sendKick = FALSE;

    disp = K2OSKERN_SeqLock(&gEventSeqLock);

    K2LIST_AddAtTail(&gEventQueue, &pEvent->ListLink);

    if (gEventQueue.mNodeCount == 1)
        sendKick = TRUE;

    K2OSKERN_SeqUnlock(&gEventSeqLock, disp);

    if ((sendKick) && (gMainThreadId != K2OS_Thread_GetId()))
    {
//        K2OSKERN_Debug("<event kick>\n");
        K2OS_Notify_Signal(gKickNotifyToken);
    }

    return TRUE;
}

void
DevMgr_NodeLocked_AddTimer(
    DEVNODE *   apNode,
    UINT32      aTimeoutMs
)
{
    K2LIST_LINK *   pListLink;
    DEVNODE *       pOtherNode;
    BOOL            sendKick;
    BOOL            disp;
    UINT64          timeNow;
    UINT64          timeHfMs;

    K2_ASSERT(NULL != apNode);
    K2_ASSERT(0 != aTimeoutMs);
    K2_ASSERT(K2OS_TIMEOUT_INFINITE != aTimeoutMs);

    K2OS_System_GetHfTick(&timeNow);

    sendKick = FALSE;

    disp = K2OSKERN_SeqLock(&gTimerSeqLock);

    K2_ASSERT(!apNode->InTimerSec.mOnQueue);

    K2OS_System_HfTickFromMsTick32(&timeHfMs, aTimeoutMs);
    timeHfMs += timeNow;
    apNode->InTimerSec.mTargetHfTick = timeHfMs;

    if (0 == gTimerQueue.mNodeCount)
    {
        K2LIST_AddAtHead(&gTimerQueue, &apNode->InTimerSec.ListLink);
        //
        // first node on queue - kick dev mgr thread to start keeping time
        //
        sendKick = TRUE;
    }
    else
    {
        pListLink = gTimerQueue.mpHead;
        pOtherNode = K2_GET_CONTAINER(DEVNODE, pListLink, InTimerSec.ListLink);
        if (pOtherNode->InTimerSec.mTargetHfTick > timeHfMs)
        {
            //
            // item at head of queue happens after this does
            //
            sendKick = TRUE;
            pOtherNode->InTimerSec.mTargetHfTick -= timeHfMs;
            K2LIST_AddAtHead(&gTimerQueue, &apNode->InTimerSec.ListLink);
        }
        else
        {
            //
            // item at head of queue does not happen after this does
            //
            timeHfMs -= pOtherNode->InTimerSec.mTargetHfTick;
            pListLink = pOtherNode->InTimerSec.ListLink.mpNext;

            if (NULL != pListLink)
            {
                do {
                    pOtherNode = K2_GET_CONTAINER(DEVNODE, pListLink, InTimerSec.ListLink);
                    if (pOtherNode->InTimerSec.mTargetHfTick >= timeHfMs)
                    {
                        //
                        // pOtherNode happens after this one does
                        //
                        pOtherNode->InTimerSec.mTargetHfTick -= timeHfMs;
                        K2LIST_AddBefore(&gTimerQueue, &apNode->InTimerSec.ListLink, pListLink);
                        break;
                    }

                    timeHfMs -= pOtherNode->InTimerSec.mTargetHfTick;
                    pListLink = pListLink->mpNext;

                } while (NULL != pListLink);
            }

            if (NULL == pListLink)
            {
                K2LIST_AddAtTail(&gTimerQueue, &apNode->InTimerSec.ListLink);
            }
        }
    }

    apNode->InTimerSec.mOnQueue = TRUE;
    Dev_CreateRef(&apNode->InTimerSec.RefToSelf_TimerActive, apNode);

    pListLink = gTimerQueue.mpHead;
    timeHfMs = 0;
    do {
        pOtherNode = K2_GET_CONTAINER(DEVNODE, pListLink, InTimerSec.ListLink);
        timeHfMs += pOtherNode->InTimerSec.mTargetHfTick;
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);

    K2OSKERN_SeqUnlock(&gTimerSeqLock, disp);

    if ((sendKick) && (gMainThreadId != K2OS_Thread_GetId()))
    {
//        K2OSKERN_Debug("<addtimer kick>\n");
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
    BOOL            disp;

    K2_ASSERT(NULL != apNode);

    sendKick = FALSE;

    tempRef.mpDevNode = NULL;

    disp = K2OSKERN_SeqLock(&gTimerSeqLock);

    if (apNode->InTimerSec.mOnQueue)
    {
        pListLink = apNode->InTimerSec.ListLink.mpNext;
        if (NULL != pListLink)
        {
            pOtherNode = K2_GET_CONTAINER(DEVNODE, pListLink, InTimerSec.ListLink);
            pOtherNode->InTimerSec.mTargetHfTick += apNode->InTimerSec.mTargetHfTick;
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
        // reference released outside of timer seqlock
        //
        Dev_CreateRef(&tempRef, apNode);
        Dev_ReleaseRef(&apNode->InTimerSec.RefToSelf_TimerActive);
    }

    K2OSKERN_SeqUnlock(&gTimerSeqLock, disp);

    if (NULL != tempRef.mpDevNode)
    {
        //
        // reference released outside of timer seqlock
        //
        Dev_ReleaseRef(&tempRef);
    }

    if ((sendKick) && (gMainThreadId != K2OS_Thread_GetId()))
    {
//        K2OSKERN_Debug("<deltimer kick>\n");
        K2OS_Notify_Signal(gKickNotifyToken);
    }
}

void
DevMgr_NotifyRun(
    void
)
{
    K2OS_MSG runMsg;

    runMsg.mMsgType = K2OS_SYSTEM_MSGTYPE_SYSPROC;
    runMsg.mShort = K2OS_SYSTEM_MSG_SYSPROC_SHORT_RUN;

    K2OSKERN_SysProc_Notify(&runMsg);
}

void
DevMgr_Run(
    void
)
{
    UINT64              timeNow;
    UINT64              deltaLeft;
    UINT32              timeOutMs;
    DEVNODE *           pDevNode;
    DEVNODE_REF         tempRef;
    K2LIST_LINK *       pListLink;
    K2LIST_ANCHOR       eventList;
    DEVEVENT *          pEvent;
    K2OS_WaitResult     waitResult;
    BOOL                disp;
    DEVNODE *           pNextNode;

    DevMgr_NotifyRun();

    do {

        K2OS_System_GetHfTick(&timeNow);

        do {
            disp = K2OSKERN_SeqLock(&gTimerSeqLock);

            if (0 == gTimerQueue.mNodeCount)
            {
                timeOutMs = K2OS_TIMEOUT_INFINITE;
                break;
            }

            pDevNode = K2_GET_CONTAINER(DEVNODE, gTimerQueue.mpHead, InTimerSec.ListLink);
            K2_ASSERT(pDevNode->InTimerSec.mOnQueue);

            if (timeNow < pDevNode->InTimerSec.mTargetHfTick)
            {
                //
                // head item has not exired yet
                //
                deltaLeft = pDevNode->InTimerSec.mTargetHfTick - timeNow;
                timeOutMs = K2OS_System_MsTick32FromHfTick(&deltaLeft);
                break;
            }

            //
            // 
            //
            pListLink = pDevNode->InTimerSec.ListLink.mpNext;
            if (NULL != pListLink)
            {
                pNextNode = K2_GET_CONTAINER(DEVNODE, pListLink, InTimerSec.ListLink);
                pNextNode->InTimerSec.mTargetHfTick += pDevNode->InTimerSec.mTargetHfTick;
            }

            K2LIST_Remove(&gTimerQueue, &pDevNode->InTimerSec.ListLink);
            pDevNode->InTimerSec.mOnQueue = FALSE;

            DevMgr_QueueEvent(pDevNode, DevNodeEvent_TimerExpired, pDevNode->InTimerSec.mArg);

            //
            // release timer reference outside of timer sec
            //
            tempRef.mpDevNode = NULL;
            Dev_CreateRef(&tempRef, pDevNode);
            Dev_ReleaseRef(&pDevNode->InTimerSec.RefToSelf_TimerActive);

            K2OSKERN_SeqUnlock(&gTimerSeqLock, disp);

            //
            // release timer reference outside of timer sec
            //
            Dev_ReleaseRef(&tempRef);

        } while (1);

        K2OSKERN_SeqUnlock(&gTimerSeqLock, disp);

        //
        // check for events here
        //
        disp = K2OSKERN_SeqLock(&gEventSeqLock);
        K2MEM_Copy(&eventList, &gEventQueue, sizeof(K2LIST_ANCHOR));
        K2LIST_Init(&gEventQueue);
        K2OSKERN_SeqUnlock(&gEventSeqLock, disp);

        if (eventList.mNodeCount == 0)
        {
            //
            // no events to process. wait for something to happen
            //
            K2OS_Thread_WaitOne(&waitResult, gKickNotifyToken, timeOutMs);
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
                Dev_NodeLocked_OnEvent(pEvent->DevNodeRef.mpDevNode, pEvent->mDevNodeEventType, pEvent->mArg);
                K2OS_CritSec_Leave(&pEvent->DevNodeRef.mpDevNode->Sec);

                Dev_ReleaseRef(&pEvent->DevNodeRef);

                K2OS_Heap_Free(pEvent);

            } while (NULL != pListLink);
        }
    } while (1);
}
