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

K2OS_CRITSEC        gDevMgrEventSec;
K2LIST_ANCHOR       gDevMgrEventList;
K2OS_NOTIFY_TOKEN   gDevMgrNotifyToken;
K2OS_THREAD_TOKEN   gDevMgrThreadToken;
UINT_PTR            gDevMgrThreadId;
UINT_PTR            gDevMgrInterfaceInstanceId;
K2LIST_ANCHOR       gDevMgrTimerQueueList;

void 
RootEvent_DefaultRelease(
    ROOTDEV_EVENT *apEvent
)
{
    K2_ASSERT(FALSE == apEvent->mOnEventList);

    RootDevNode_ReleaseRef(&apEvent->RefDevNode);

    K2OS_Heap_Free(apEvent);
}

ROOTDEV_EVENT *  
RootDevMgr_AllocEvent(
    UINT_PTR        aSize,
    ROOTDEV_NODE *  apDevNode
)
{
    ROOTDEV_EVENT *  pResult;

    K2_ASSERT(NULL != apDevNode);

    if (0 == aSize)
    {
        aSize = sizeof(ROOTDEV_EVENT);
    }
    else
    {
        K2_ASSERT(aSize >= sizeof(ROOTDEV_EVENT));
    }

    pResult = (ROOTDEV_EVENT *)K2OS_Heap_Alloc(aSize);
    if (NULL == pResult)
    {
        return NULL;
    }

    K2MEM_Zero(pResult, aSize);

    RootDevNode_CreateRef(&pResult->RefDevNode, apDevNode);

    return pResult;
}

UINT_PTR
RootDevMgr_Thread(
    void *apArg
)
{
    UINT_PTR        waitResult;
    ROOTDEV_EVENT * pEvent;
    UINT64          lastTime;
    UINT64          curTime;
    UINT64          elapsedTime;
    UINT_PTR        elapsedMs;
    UINT_PTR        timeOut;
    ROOTDEV_TIMER * pTimer;
    BOOL            ranTimerCallbacks;
    ROOTDEV_NODE *  pDevNode;
    ROOTDEV_NODEREF devNodeRef;

    K2OS_System_GetHfTick(&lastTime);
    curTime = lastTime;

    do {
        elapsedTime = curTime - lastTime;
        lastTime = curTime;
        elapsedMs = K2OS_System_MsTick32FromHfTick(&elapsedTime);

        ranTimerCallbacks = FALSE;

        if ((0 != gDevMgrTimerQueueList.mNodeCount) &&
            (elapsedMs > 0))
        {
            pTimer = K2_GET_CONTAINER(ROOTDEV_TIMER, gDevMgrTimerQueueList.mpHead, ListLink);

            do {
                if (pTimer->mMsTimeoutRemaining > elapsedMs)
                {
                    pTimer->mMsTimeoutRemaining -= elapsedMs;
                    break;
                }

                elapsedMs -= pTimer->mMsTimeoutRemaining;
                pTimer->mMsTimeoutRemaining = 0;

                //
                // any items at the head of the queue get their callbacks issued
                // which may result in events being added to the event queue
                //
                do {
                    K2LIST_Remove(&gDevMgrTimerQueueList, &pTimer->ListLink);
                    pTimer->mIsActive = FALSE;

                    devNodeRef.mpRefNode = NULL;
                    pDevNode = pTimer->Context.RefDevNode.mpRefNode;
                    if (NULL != pDevNode)
                    {
                        RootDevNode_CreateRef(&devNodeRef, pDevNode);
                        RootDevNode_ReleaseRef(&pTimer->Context.RefDevNode);
                    }

                    if (NULL != pTimer->mfCallback)
                    {
                        pTimer->mfCallback(pTimer, pDevNode, pTimer->Context.mUserVal);
                        ranTimerCallbacks = TRUE;
                    }

                    if (NULL != devNodeRef.mpRefNode)
                    {
                        RootDevNode_ReleaseRef(&devNodeRef);
                    }
                    
                    if (0 == gDevMgrTimerQueueList.mNodeCount)
                        break;

                    pTimer = K2_GET_CONTAINER(ROOTDEV_TIMER, gDevMgrTimerQueueList.mpHead, ListLink);

                } while (0 == pTimer->mMsTimeoutRemaining);

            } while ((0 != gDevMgrTimerQueueList.mNodeCount) && (0 != elapsedMs));
        }

        //
        // if we ran callbacks they may have taken time and new timers
        // may have expired. so we only go execute events if timer callbacks
        // have not fired. otherwise we go around again to add time that has passed
        //

        if (!ranTimerCallbacks)
        {
            //
            // if there is a pending event service it
            //
            K2OS_CritSec_Enter(&gDevMgrEventSec);

            if (0 != gDevMgrEventList.mNodeCount)
            {
                pEvent = K2_GET_CONTAINER(ROOTDEV_EVENT, gDevMgrEventList.mpHead, EventListLink);
                K2_ASSERT(pEvent->mOnEventList);
                K2LIST_Remove(&gDevMgrEventList, &pEvent->EventListLink);
                pEvent->mOnEventList = FALSE;
            }
            else
            {
                pEvent = NULL;
            }

            K2OS_CritSec_Leave(&gDevMgrEventSec);

            if (NULL == pEvent)
            {
                //
                // no events currently active, so we're going to wait
                //
                if (0 != gDevMgrTimerQueueList.mNodeCount)
                {
                    pTimer = K2_GET_CONTAINER(ROOTDEV_TIMER, gDevMgrTimerQueueList.mpHead, ListLink);
                    timeOut = pTimer->mMsTimeoutRemaining;
                    K2_ASSERT(timeOut != K2OS_TIMEOUT_INFINITE);
                    K2_ASSERT(timeOut != 0);
                }
                else
                {
                    K2OS_CritSec_Enter(&gRootDevNode_RefSec);
                    pDevNode = gpRootDevNode_CleanupList;
                    if (NULL != pDevNode)
                    {
                        gpRootDevNode_CleanupList = (ROOTDEV_NODE *)(*((ROOTDEV_NODE **)pDevNode));
                        K2OS_CritSec_Leave(&gRootDevNode_RefSec);
                        RootDevNode_Cleanup(pDevNode);
                        timeOut = 0;
                    }
                    else
                    {
                        K2OS_CritSec_Leave(&gRootDevNode_RefSec);
                        timeOut = K2OS_TIMEOUT_INFINITE;
                    }
                }

                if (timeOut != 0)
                {
                    waitResult = K2OS_Wait_One(gDevMgrNotifyToken, timeOut);
                    if ((waitResult != K2STAT_ERROR_TIMEOUT) &&
                        (waitResult != K2OS_THREAD_WAIT_SIGNALLED_0))
                    {
                        RootDebug_Printf("DevMgr thread wait failed (0x%08X)\n", waitResult);
                        break;
                    }
                }
            }
            else
            {
//                RootDebug_Printf("Exec event %d:%d\n", pEvent->mType, pEvent->mDriverType);
                pDevNode = pEvent->RefDevNode.mpRefNode;
                pDevNode->mfOnEvent(pDevNode, pEvent);
                if (NULL == pEvent->mfRelease)
                {
                    RootEvent_DefaultRelease(pEvent);
                }
                else
                {
                    pEvent->mfRelease(pEvent);
                }
                pEvent = NULL;
            }
        }

        K2OS_System_GetHfTick(&curTime);

    } while (1);

    return waitResult;
}

void
RootDevMgr_Init(
    void
)
{
    static const K2_GUID128 sDevMgrIfaceId = K2OS_IFACE_ID_DEVMGR_HOST;

    BOOL ok;

    ok = K2OS_CritSec_Init(&gDevMgrEventSec);
    K2_ASSERT(ok);

    K2LIST_Init(&gDevMgrEventList);

    gDevMgrNotifyToken = K2OS_Notify_Create(FALSE);
    K2_ASSERT(NULL != gDevMgrNotifyToken);

    K2LIST_Init(&gDevMgrTimerQueueList);

    gDevMgrThreadToken = K2OS_Thread_Create(RootDevMgr_Thread, NULL, NULL, &gDevMgrThreadId);
    K2_ASSERT(NULL != gDevMgrThreadToken);
    K2_ASSERT(0 != gDevMgrThreadId);

    gDevMgrInterfaceInstanceId = K2OS_IfInstance_Create(gMainMailbox, 0, (void *)&gDevMgrInterfaceInstanceId);
    K2_ASSERT(0 != gDevMgrInterfaceInstanceId);

    ok = K2OS_IfInstance_Publish(gDevMgrInterfaceInstanceId, K2OS_IFCLASS_DEVMGR, &sDevMgrIfaceId);
    K2_ASSERT(ok);
}

void
RootDevMgr_EventOccurred(
    ROOTDEV_EVENT *apEvent
)
{
    K2_ASSERT(NULL != apEvent);
    K2_ASSERT(0 == apEvent->mRefCount);
    K2_ASSERT(NULL != apEvent->RefDevNode.mpRefNode);

    K2ATOMIC_Inc(&apEvent->mRefCount);

    K2OS_CritSec_Enter(&gDevMgrEventSec);
//    RootDebug_Printf("EventOccurred(%d:%d)\n", apEvent->mType, apEvent->mDriverType);
    apEvent->mOnEventList = TRUE;
    K2LIST_AddAtTail(&gDevMgrEventList, &apEvent->EventListLink);
    K2OS_CritSec_Leave(&gDevMgrEventSec);

    K2OS_Notify_Signal(gDevMgrNotifyToken);
}

void
RootDevMgr_AddTimer(
    ROOTDEV_TIMER *             apTimer,
    UINT_PTR                    aTimeoutMs, 
    RootTimer_pf_ExpiryCallback afCallback, 
    ROOTDEV_NODE *              apDevNode, 
    UINT_PTR                    aContextVal
)
{
    ROOTDEV_TIMER *  pTimerOnQueue;

    K2_ASSERT(gDevMgrThreadId ==  K2OS_Thread_GetId());

    K2_ASSERT(FALSE == apTimer->mIsActive);
    K2_ASSERT(NULL == apTimer->Context.RefDevNode.mpRefNode);

    apTimer->mMsTimeoutRemaining = aTimeoutMs;
    if (apTimer->mMsTimeoutRemaining == K2OS_TIMEOUT_INFINITE)
        apTimer->mMsTimeoutRemaining--;

    K2_ASSERT(NULL != afCallback);
    apTimer->mfCallback = afCallback;

    if (NULL != apDevNode)
    {
        RootDevNode_CreateRef(&apTimer->Context.RefDevNode, apDevNode);
    }

    apTimer->Context.mUserVal = aContextVal;

    //
    // put timer onto timer queue in the right place
    //
    if (0 == gDevMgrTimerQueueList.mNodeCount)
    {
        //
        // timer queue is empty
        //
        K2LIST_AddAtHead(&gDevMgrTimerQueueList, &apTimer->ListLink);
    }
    else
    {
        pTimerOnQueue = K2_GET_CONTAINER(ROOTDEV_TIMER, gDevMgrTimerQueueList.mpHead, ListLink);

        do {
            if (apTimer->mMsTimeoutRemaining < pTimerOnQueue->mMsTimeoutRemaining)
            {
                //
                // found insertion point - between pPrev and pTimer
                //
                pTimerOnQueue->mMsTimeoutRemaining -= apTimer->mMsTimeoutRemaining;
                K2LIST_AddBefore(&gDevMgrTimerQueueList, &apTimer->ListLink, &pTimerOnQueue->ListLink);
                break;
            }

            //
            // adjust for prior item expiry
            //
            apTimer->mMsTimeoutRemaining -= pTimerOnQueue->mMsTimeoutRemaining;

            if (NULL == pTimerOnQueue->ListLink.mpNext)
            {
                //
                // goes at the end of the timer queue
                //
                K2LIST_AddAtTail(&gDevMgrTimerQueueList, &apTimer->ListLink);
                break;
            }

            //
            // have not found insertion point yet
            //
            pTimerOnQueue = K2_GET_CONTAINER(ROOTDEV_TIMER, pTimerOnQueue->ListLink.mpNext, ListLink);
        } while (1);
    }

    apTimer->mIsActive = TRUE;
}

void                
RootDevMgr_DelTimer(
    ROOTDEV_TIMER * apTimer
)
{
    ROOTDEV_TIMER *  pTimerOnQueue;

    K2_ASSERT(gDevMgrThreadId == K2OS_Thread_GetId());

    if (!apTimer->mIsActive)
        return;

    //
    // remove timer from timer queue and update the queue
    //
    if (NULL != apTimer->ListLink.mpNext)
    {
        pTimerOnQueue = K2_GET_CONTAINER(ROOTDEV_TIMER, apTimer->ListLink.mpNext, ListLink);
        pTimerOnQueue->mMsTimeoutRemaining += apTimer->mMsTimeoutRemaining;
    }
    K2LIST_Remove(&gDevMgrTimerQueueList, &apTimer->ListLink);

    if (NULL != apTimer->Context.RefDevNode.mpRefNode)
    {
        RootDevNode_ReleaseRef(&apTimer->Context.RefDevNode);
    }

    apTimer->mIsActive = FALSE;
}
