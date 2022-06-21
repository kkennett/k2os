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

K2OS_CRITSEC  gRootDriverInstanceTreeSec;
K2TREE_ANCHOR gRootDriverInstanceTree;

K2STAT
DriverHostObj_Create(
    K2OSRPC_OBJECT_CREATE const *   apCreate,
    UINT_PTR *                      apRetRef
)
{
    K2TREE_NODE *   pTreeNode;
    ROOTDEV_EVENT * pEvent;
    ROOTDEV_NODE *  pDevNode;
    K2STAT          stat;

    K2OS_CritSec_Enter(&gRootDriverInstanceTreeSec);

    pTreeNode = K2TREE_Find(&gRootDriverInstanceTree, apCreate->mCallerProcessId);
    if (NULL != pTreeNode)
    {
        pDevNode = K2_GET_CONTAINER(ROOTDEV_NODE, pTreeNode, GlobalDriverInstanceTreeNode);
        if (NULL != pDevNode->DriverNodeSelfRef.mpRefNode)
        {
            pDevNode = NULL;
        }
        else
        {
            RootDevNode_CreateRef(&pDevNode->DriverNodeSelfRef, pDevNode);
        }
    }
    else
    {
        pDevNode = NULL;
    }

    K2OS_CritSec_Leave(&gRootDriverInstanceTreeSec);

    if (NULL == pDevNode)
        return K2STAT_ERROR_NOT_FOUND;

    K2OS_CritSec_Enter(&pDevNode->Sec);

    if (RootDevNodeState_DriverStarted == pDevNode->InSec.mState)
    {
        *apRetRef = (UINT_PTR)pDevNode;
        stat = K2STAT_NO_ERROR;

        pEvent = RootDevMgr_AllocEvent(0, pDevNode);
        K2_ASSERT(NULL != pEvent);
        pEvent->mType = RootEvent_DriverEvent;
        pEvent->mDriverType = RootDriverEvent_ObjectCreated;

        RootDevMgr_EventOccurred(pEvent);
    }
    else
    {
        stat = K2STAT_ERROR_TIMEOUT;
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    return stat;
}

K2STAT
DriverHostObj_SetMailbox(
    K2OSRPC_OBJECT_CALL const * apCall,
    UINT_PTR *                  apRetUsedOutBytes,
    ROOTDEV_NODE *              apDevNode
)
{
    ROOTDEV_EVENT *             pEvent;
    K2STAT                      stat;
    K2OSDRV_SET_MAILSLOT_IN *   pArgsIn;

    if (apCall->Args.mInBufByteCount < sizeof(K2OSDRV_SET_MAILSLOT_IN))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pArgsIn = (K2OSDRV_SET_MAILSLOT_IN *)apCall->Args.mpInBuf;

    K2OS_CritSec_Enter(&apDevNode->Sec);

    if (RootDevNodeState_ObjectCreated == apDevNode->InSec.mState)
    {
        apDevNode->InSec.Driver.mMailslotToken = K2OS_Root_CreateTokenForRoot(
            apDevNode->InSec.Driver.mProcessToken,
            (UINT_PTR)pArgsIn->mMailslotToken
        );
        if (NULL == apDevNode->InSec.Driver.mMailslotToken)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            RootDebug_Printf("*** Failed to create driver mailslot token for root to use (stat=0x%08X)\n", stat);
        }
        else
        {
            pEvent = RootDevMgr_AllocEvent(0, apDevNode);
            K2_ASSERT(NULL != pEvent);
            pEvent->mType = RootEvent_DriverEvent;
            pEvent->mDriverType = RootDriverEvent_MailslotSet;
            RootDevMgr_EventOccurred(pEvent);
            stat = K2STAT_NO_ERROR;
        }
    }
    else
    {
        stat = K2STAT_ERROR_API_ORDER;
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);

    return stat;
}

K2STAT
DriverHostObj_Call(
    K2OSRPC_OBJECT_CALL const * apCall,
    UINT_PTR *                  apRetUsedOutBytes
)
{
    ROOTDEV_NODE *  pDevNode;
    K2STAT          stat;

    pDevNode = (ROOTDEV_NODE *)apCall->mCreatorRef;
    K2_ASSERT(pDevNode->DriverNodeSelfRef.mpRefNode == pDevNode);

    RootDebug_Printf("%s\n", __FUNCTION__);

    stat = K2STAT_ERROR_UNKNOWN;
    switch (apCall->Args.mMethodId)
    {
    case K2OSDRV_HOSTOBJECT_METHOD_SET_MAILSLOT:
        stat = DriverHostObj_SetMailbox(apCall, apRetUsedOutBytes, pDevNode);
        break;

    default:
        stat = K2STAT_ERROR_NOT_IMPL;
        break;
    }

    return stat;
}

K2STAT
DriverHostObj_Delete(
    K2OSRPC_OBJECT_DELETE const *   apDelete
)
{
    ROOTDEV_NODE *  pDevNode;
    ROOTDEV_EVENT * pEvent;

    pDevNode = (ROOTDEV_NODE *)apDelete->mCreatorRef;
    K2_ASSERT(pDevNode->DriverNodeSelfRef.mpRefNode == pDevNode);

    pEvent = RootDevMgr_AllocEvent(0, pDevNode);
    K2_ASSERT(NULL != pEvent);
    pEvent->mType = RootEvent_DriverEvent;
    pEvent->mDriverType = RootDriverEvent_ObjectDeleted;

    RootDevNode_ReleaseRef(&pDevNode->DriverNodeSelfRef);
    K2_CpuWriteBarrier();

    RootDevMgr_EventOccurred(pEvent);

    return K2STAT_NO_ERROR;
}

void
RootDriver_Init(
    void
)
{
    static K2OSRPC_OBJECT_CLASS const sDriverHostObjectClassId =
    {
        K2OSDRV_HOSTOBJECT_CLASS_ID,
        TRUE,
        DriverHostObj_Create,
        DriverHostObj_Call,
        DriverHostObj_Delete
    };

    BOOL ok;

    ok = K2OS_CritSec_Init(&gRootDriverInstanceTreeSec);
    K2_ASSERT(ok);

    K2TREE_Init(&gRootDriverInstanceTree, NULL);

    //
    // ready to publish driver host object class availability
    //
    ok = K2OSRPC_ObjectClass_Publish(&sDriverHostObjectClassId, 0);
    K2_ASSERT(ok);
}

void
RootDevNode_OnDriverEvent_Purged(
    ROOTDEV_NODE * apDevNode
)
{
    RootDebug_Printf("%s\n", __FUNCTION__);
    K2_ASSERT(apDevNode->InSec.ChildList.mNodeCount == 0);
}

void
RootDevNode_OnDriverEvent_MailslotSet(
    ROOTDEV_NODE * apDevNode
)
{
    K2OS_MSG    msg;
    K2STAT      stat;

    K2OS_CritSec_Enter(&apDevNode->Sec);

    //
    // stop timer if it is running
    //
    RootDevMgr_DelTimer(&apDevNode->InSec.Driver.Timer);

    //
    // node is in running state now. must set this prior to sending it
    // the start command, because other process could call its object
    // right away before we get a chance to set the node to running state
    //
    apDevNode->InSec.mState = RootDevNodeState_Running;

    //
    // send driver start command
    //
    K2MEM_Zero(&msg, sizeof(msg));
    msg.mControl = K2OS_MSG_CONTROL_DRV_START;
    stat = K2OS_Mailslot_Send(apDevNode->InSec.Driver.mMailslotToken, &msg);
    if (K2STAT_IS_ERROR(stat))
    {
        //
        // failed to send start command so ask kernel to stop the process
        // once it stops the object will get deleted which will trigger
        // all the cleanup
        //
        apDevNode->InSec.Driver.mLastStatus = stat;
        K2OS_Root_StopProcess(apDevNode->InSec.Driver.mProcessToken, (UINT_PTR)stat, FALSE);
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);
}

void
RootDevNode_OnDriverEvent_Stopped(
    ROOTDEV_NODE * apDevNode
)
{
    ROOTDEV_EVENT * pEvent;
    BOOL            sendPurge;

    RootDebug_Printf("%s\n", __FUNCTION__);

    K2OS_CritSec_Enter(&apDevNode->Sec);

    K2_ASSERT(!apDevNode->InSec.Driver.mObjectExists);

    RootDevMgr_DelTimer(&apDevNode->InSec.Driver.Timer);

    apDevNode->InSec.Driver.mIsEnabled = FALSE;

    if (NULL != apDevNode->InSec.Driver.mMailslotToken)
    {
        K2OS_Token_Destroy(apDevNode->InSec.Driver.mMailslotToken);
        apDevNode->InSec.Driver.mMailslotToken = NULL;
    }

    if (NULL != apDevNode->InSec.Driver.mProcessToken)
    {
        K2OS_Token_Destroy(apDevNode->InSec.Driver.mProcessToken);
        apDevNode->InSec.Driver.mProcessToken = NULL;
    }

    if (apDevNode->InSec.ChildList.mNodeCount > 0)
    {
        //
        // start to stop the children drivers
        // the purge will come when they are all done
        //
        sendPurge = FALSE;
    }
    else
    {
        //
        // no children so we can purge
        //
        sendPurge = TRUE;
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);

    if (sendPurge)
    {
        pEvent = RootDevMgr_AllocEvent(0, apDevNode);
        K2_ASSERT(NULL != pEvent);
        pEvent->mType = RootEvent_DriverEvent;
        pEvent->mDriverType = RootDriverEvent_Purged;
        RootDevMgr_EventOccurred(pEvent);
    }
}

void
RootDevNode_TimerExpired_CheckForStoppedProcess(
    ROOTDEV_TIMER * apTimer,
    ROOTDEV_NODE *  apDevNode,
    UINT_PTR        aContextVal
)
{
    UINT_PTR        waitResult;
    ROOTDEV_EVENT * pEvent;

    RootDebug_Printf("%s\n", __FUNCTION__);

    K2_ASSERT(NULL != apDevNode);

    K2OS_CritSec_Enter(&apDevNode->Sec);

    K2_ASSERT(RootDevNodeState_DriverStopping == apDevNode->InSec.mState);

    if (NULL == apDevNode->DriverNodeSelfRef.mpRefNode)
    {
        //
        // object has been deleted or was never created
        //
        waitResult = K2OS_Wait_One(apDevNode->InSec.Driver.mProcessToken, 0);

        if (K2OS_THREAD_WAIT_SIGNALLED_0 == waitResult)
        {
            apDevNode->InSec.Driver.mLastStatus = (K2STAT)K2OS_Root_GetProcessExitCode(apDevNode->InSec.Driver.mProcessToken);
            apDevNode->InSec.mState = RootDevNodeState_DriverStopped;
            pEvent = RootDevMgr_AllocEvent(0, apDevNode);
            K2_ASSERT(NULL != pEvent);
            pEvent->mType = RootEvent_DriverEvent;
            pEvent->mDriverType = RootDriverEvent_Stopped;
            RootDevMgr_EventOccurred(pEvent);
        }
    }

    if (RootDevNodeState_DriverStopping == apDevNode->InSec.mState)
    {
        // check again in 500 ms
        RootDevMgr_AddTimer(&apDevNode->InSec.Driver.Timer, 500, RootDevNode_TimerExpired_CheckForStoppedProcess, apDevNode, 0);
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);
}

void
RootDevNode_TimerExpired_MailboxSet(
    ROOTDEV_TIMER * apTimer,
    ROOTDEV_NODE *  apDevNode,
    UINT_PTR        aContextVal
)
{
    RootDebug_Printf("%s\n", __FUNCTION__);

    K2_ASSERT(NULL != apDevNode);

    K2OS_CritSec_Enter(&apDevNode->Sec);

    if (RootDevNodeState_ObjectCreated == apDevNode->InSec.mState)
    {
        //
        // timeout expired waiting for driver to set mailslot
        //
        K2_ASSERT(NULL != apDevNode->InSec.Driver.mProcessToken);
        K2OS_Root_StopProcess(apDevNode->InSec.Driver.mProcessToken, (UINT_PTR)K2STAT_ERROR_TIMEOUT, FALSE);

        apDevNode->InSec.Driver.mLastStatus = K2STAT_ERROR_TIMEOUT;
        apDevNode->InSec.mState = RootDevNodeState_DriverStopping;

        //
        // handle timeout case here, then start check-for-process-exit timer
        //
        RootDevMgr_AddTimer(&apDevNode->InSec.Driver.Timer, 500, RootDevNode_TimerExpired_CheckForStoppedProcess, apDevNode, 0);
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);
}

void
RootDevNode_TimerExpired_ForceExitCountdown(
    ROOTDEV_TIMER * apTimer,
    ROOTDEV_NODE *  apDevNode,
    UINT_PTR        aContextVal
)
{
    ROOTDEV_EVENT * pEvent;
    UINT_PTR        waitResult;

    RootDebug_Printf("%s\n", __FUNCTION__);

    K2OS_CritSec_Enter(&apDevNode->Sec);

    K2_ASSERT((RootDevNodeState_ObjectCreated == apDevNode->InSec.mState) ||  (RootDevNodeState_Running == apDevNode->InSec.mState));

    if (0 == --apDevNode->InSec.Driver.mForcedExitCountLeft)
    {
        RootDebug_Printf("ForceExitCountdown-Boom\n");

        K2_ASSERT(NULL != apDevNode->InSec.Driver.mProcessToken);
        K2OS_Root_StopProcess(apDevNode->InSec.Driver.mProcessToken, (UINT_PTR)K2STAT_ERROR_ABANDONED, FALSE);

        apDevNode->InSec.Driver.mLastStatus = K2STAT_ERROR_ABANDONED;
        apDevNode->InSec.mState = RootDevNodeState_DriverStopping;

        //
        // handle timeout case here, then start wait-for-process-exit timer
        //
        RootDevMgr_AddTimer(&apDevNode->InSec.Driver.Timer, 500, RootDevNode_TimerExpired_CheckForStoppedProcess, apDevNode, 0);
    }
    else
    {
        RootDebug_Printf("ForceExitCountdown-Tick\n");

        waitResult = K2OS_Wait_One(apDevNode->InSec.Driver.mProcessToken, 0);
        if (waitResult == K2OS_THREAD_WAIT_SIGNALLED_0)
        {
            //
            // process exited on its own
            //
            apDevNode->InSec.Driver.mLastStatus = (K2STAT)K2OS_Root_GetProcessExitCode(apDevNode->InSec.Driver.mProcessToken);
            apDevNode->InSec.mState = RootDevNodeState_DriverStopped;
            pEvent = RootDevMgr_AllocEvent(0, apDevNode);
            K2_ASSERT(NULL != pEvent);
            pEvent->mType = RootEvent_DriverEvent;
            pEvent->mDriverType = RootDriverEvent_Stopped;
            RootDevMgr_EventOccurred(pEvent);
        }
        else
        {
            RootDevMgr_AddTimer(&apDevNode->InSec.Driver.Timer, 500, RootDevNode_TimerExpired_ForceExitCountdown, apDevNode, 0);
        }
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);
}

void 
RootDevNode_OnDriverEvent_ObjectDeleted(
    ROOTDEV_NODE *      apDevNode
)
{
    RootDebug_Printf("%s\n", __FUNCTION__);

    K2OS_CritSec_Enter(&apDevNode->Sec);

    apDevNode->InSec.Driver.mObjectExists = FALSE;

    if ((RootDevNodeState_ObjectCreated == apDevNode->InSec.mState) ||
        (RootDevNodeState_Running == apDevNode->InSec.mState))
    {
        if (RootDevNodeState_ObjectCreated == apDevNode->InSec.mState)
        {
            //
            // mailslot not set yet and object deleted.
            //
            RootDebug_Printf("ObjectDeleted while waiting for mailbox set\n");
            RootDevMgr_DelTimer(&apDevNode->InSec.Driver.Timer);
        }
        //
        // process should exit on its own but we only give it two seconds to do that
        //
        apDevNode->InSec.Driver.mForcedExitCountLeft = 4;
        RootDevMgr_AddTimer(&apDevNode->InSec.Driver.Timer, 500, RootDevNode_TimerExpired_ForceExitCountdown, apDevNode, 0);
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);
}

void
RootDevNode_OnDriverEvent_ObjectCreated(
    ROOTDEV_NODE *      apDevNode
)
{
    RootDebug_Printf("%s\n", __FUNCTION__);

    K2OS_CritSec_Enter(&apDevNode->Sec);

    if (RootDevNodeState_DriverStarted == apDevNode->InSec.mState)
    {
        RootDevMgr_DelTimer(&apDevNode->InSec.Driver.Timer);

        apDevNode->InSec.Driver.mObjectExists = TRUE;
        apDevNode->InSec.mState = RootDevNodeState_ObjectCreated;

        RootDevMgr_AddTimer(&apDevNode->InSec.Driver.Timer, 5000, RootDevNode_TimerExpired_MailboxSet, apDevNode, 0);
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);
}

void 
RootDevNode_TimerExpired_ProcessStart(
    ROOTDEV_TIMER * apTimer,
    ROOTDEV_NODE *  apDevNode,
    UINT_PTR        aContextVal
)
{
    RootDebug_Printf("%s\n", __FUNCTION__);

    K2_ASSERT(NULL != apDevNode);

    K2OS_CritSec_Enter(&apDevNode->Sec);

    if (RootDevNodeState_DriverStarted == apDevNode->InSec.mState)
    {
        //
        // timeout expired waiting for driver to create object
        //
        K2_ASSERT(NULL != apDevNode->InSec.Driver.mProcessToken);
        K2OS_Root_StopProcess(apDevNode->InSec.Driver.mProcessToken, (UINT_PTR)K2STAT_ERROR_TIMEOUT, FALSE);

        apDevNode->InSec.Driver.mLastStatus = K2STAT_ERROR_TIMEOUT;
        apDevNode->InSec.mState = RootDevNodeState_DriverStopping;

        //
        // handle timeout case here, then start wait-for-process-exit timer
        //
        RootDevMgr_AddTimer(&apDevNode->InSec.Driver.Timer, 500, RootDevNode_TimerExpired_CheckForStoppedProcess, apDevNode, 0);
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);
}

void
RootDevNode_OnDriverEvent_StartFailed(
    ROOTDEV_NODE *      apDevNode
)
{
    RootDebug_Printf("%s\n", __FUNCTION__);
}

void
RootDevNode_OnDriverEvent_CandidateChanged(
    ROOTDEV_NODE *      apDevNode
)
{
    ROOTDEV_EVENT *  pEvent;

    RootDebug_Printf("%s\n", __FUNCTION__);

    K2OS_CritSec_Enter(&apDevNode->Sec);

    if (RootDevNodeState_DriverSpec == apDevNode->InSec.mState)
    {
        K2_ASSERT(NULL == apDevNode->InSec.Driver.mProcessToken);

        do {
            if (NULL == apDevNode->InSec.mpDriverCandidate)
            {
                RootDebug_Printf("*** Driver attach but no candidate!\n");
                apDevNode->InSec.mState = RootDevNodeState_NoDriver;
                break;
            }

            apDevNode->InSec.Driver.mProcessToken = K2OS_Root_Launch(
                ":k2osdrv.dlx",
                (char const *)apDevNode->InSec.mpDriverCandidate,
                &apDevNode->GlobalDriverInstanceTreeNode.mUserVal);

            if (NULL == apDevNode->InSec.Driver.mProcessToken)
            {
                apDevNode->InSec.Driver.mLastStatus = K2OS_Thread_GetLastStatus();
                RootDebug_Printf("*** Failed to start k2osdrv for \"%s\"\n", apDevNode->InSec.mpDriverCandidate);

                apDevNode->InSec.mState = RootDevNodeState_DriverStopped;

                pEvent = RootDevMgr_AllocEvent(0, apDevNode);
                K2_ASSERT(NULL != pEvent);
                pEvent->mType = RootEvent_DriverEvent;
                pEvent->mDriverType = RootDriverEvent_Stopped;
                RootDevMgr_EventOccurred(pEvent);
                break;
            }

            RootDebug_Printf("Started Driver for \"%s\" in process %d\n", apDevNode->InSec.mpDriverCandidate, apDevNode->GlobalDriverInstanceTreeNode.mUserVal);

            K2OS_CritSec_Enter(&gRootDriverInstanceTreeSec);

            K2TREE_Insert(&gRootDriverInstanceTree, apDevNode->GlobalDriverInstanceTreeNode.mUserVal, &apDevNode->GlobalDriverInstanceTreeNode);

            K2OS_CritSec_Leave(&gRootDriverInstanceTreeSec);

            apDevNode->InSec.mState = RootDevNodeState_DriverStarted;

            RootDevMgr_AddTimer(&apDevNode->InSec.Driver.Timer, 5000, RootDevNode_TimerExpired_ProcessStart, apDevNode, 0);

        } while (0);
    }

    K2OS_CritSec_Leave(&apDevNode->Sec);
}

void
RootDevNode_OnDriverEvent(
    ROOTDEV_NODE *  apDevNode,
    ROOTDEV_EVENT * apEvent
)
{
    switch (apEvent->mDriverType)
    {
    case RootDriverEvent_CandidateChanged:
        RootDevNode_OnDriverEvent_CandidateChanged(apDevNode);
        break;

    case RootDriverEvent_StartFailed:
        RootDevNode_OnDriverEvent_StartFailed(apDevNode);
        break;

    case RootDriverEvent_ObjectCreated:
        RootDevNode_OnDriverEvent_ObjectCreated(apDevNode);
        break;

    case RootDriverEvent_MailslotSet:
        RootDevNode_OnDriverEvent_MailslotSet(apDevNode);
        break;

    case RootDriverEvent_ObjectDeleted:
        RootDevNode_OnDriverEvent_ObjectDeleted(apDevNode);
        break;

    case RootDriverEvent_Stopped:
        RootDevNode_OnDriverEvent_Stopped(apDevNode);
        break;

    case RootDriverEvent_Purged:
        RootDevNode_OnDriverEvent_Purged(apDevNode);
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}


