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

static DEVNODE          sgDevTreeRootDevNode;
static K2OS_CRITSEC     sgRefSec;

DEVNODE * const gpDevTree = &sgDevTreeRootDevNode;

void
Dev_Init(
    void
)
{
    BOOL ok;

    ok = K2OS_CritSec_Init(&sgRefSec);
    K2_ASSERT(ok);

    K2MEM_Zero(gpDevTree, sizeof(DEVNODE));

    //
    // ensures root node always has a reference
    //
    Dev_CreateRef(&gpDevTree->RefToSelf_OwnedByParent, gpDevTree);

    ok = K2OS_CritSec_Init(&gpDevTree->Sec);
    K2_ASSERT(ok);

    K2LIST_Init(&gpDevTree->InSec.ChildList);

    gpDevTree->InSec.mState = DevNodeState_Offline_NoDriver;

    K2LIST_Init(&gpDevTree->InSec.IrqResList);
    K2LIST_Init(&gpDevTree->InSec.IoResList);
    K2LIST_Init(&gpDevTree->InSec.PhysResList);
}

void
Dev_CreateRef(
    DEVNODE_REF *   apRef,
    DEVNODE *       apNode
)
{
    K2_ASSERT(NULL == apRef->mpDevNode);
    K2_ASSERT(NULL != apNode);

    apRef->mpDevNode = apNode;

    K2OS_CritSec_Enter(&sgRefSec);

    K2LIST_AddAtTail(&apNode->RefList, &apRef->ListLink);

//    Debug_Printf("Node(%08X)++ == %d (%08X)\n", apNode, apNode->RefList.mNodeCount, apNode->RefToParent.mpDevNode);

    K2OS_CritSec_Leave(&sgRefSec);
}

void
Dev_ReleaseRef(
    DEVNODE_REF *   apRef
)
{
    DEVNODE *   pNode;

    K2_ASSERT(NULL != apRef);
    
    pNode = apRef->mpDevNode;
    
    K2_ASSERT(NULL != pNode);

    K2_ASSERT((pNode == gpDevTree) || (NULL != pNode->RefToParent.mpDevNode));

    K2OS_CritSec_Enter(&sgRefSec);
    
    K2LIST_Remove(&pNode->RefList, &apRef->ListLink);

//    Debug_Printf("Node(%08X)-- == %d (%08X)\n", pNode, pNode->RefList.mNodeCount, pNode->RefToParent.mpDevNode);

    if (0 == pNode->RefList.mNodeCount)
    {
        K2_ASSERT(NULL == pNode->RefToSelf_OwnedByParent.mpDevNode);
        DevMgr_QueueEvent(pNode->RefToParent.mpDevNode, DevNodeEvent_ChildRefsHitZero, (UINT_PTR)pNode);
    }

    apRef->mpDevNode = NULL;
    
    K2OS_CritSec_Leave(&sgRefSec);
}

DEVNODE *
Dev_NodeLocked_CreateChildNode(
    DEVNODE *               apParent,
    K2_DEVICE_IDENT const * apIdent
)
{
    DEVNODE *   pChild;
    BOOL        ok;

    pChild = (DEVNODE *)K2OS_Heap_Alloc(sizeof(DEVNODE));
    if (NULL == pChild)
    {
        Debug_Printf("*** CreateChildNode - failed memory allocation\n");
        return NULL;
    }

    K2MEM_Zero(pChild, sizeof(DEVNODE));

    ok = K2OS_CritSec_Init(&pChild->Sec);
    if (!ok)
    {
        Debug_Printf("*** CreateChildNode - failed critical section init\n");
        K2OS_Heap_Free(pChild);
        return NULL;
    }

    K2LIST_Init(&pChild->RefList);
    Dev_CreateRef(&pChild->RefToParent, apParent);
    Dev_CreateRef(&pChild->RefToSelf_OwnedByParent, pChild);

    K2MEM_Copy(&pChild->DeviceIdent, apIdent, sizeof(K2_DEVICE_IDENT));

    K2LIST_Init(&pChild->InSec.ChildList);

    pChild->InSec.mState = DevNodeState_Offline_NoDriver;

    K2LIST_Init(&pChild->InSec.IrqResList);
    K2LIST_Init(&pChild->InSec.IoResList);
    K2LIST_Init(&pChild->InSec.PhysResList);

    K2LIST_AddAtTail(&apParent->InSec.ChildList, &pChild->InParentSec.ParentChildListLink);

    return pChild;
}

void
Dev_NodeLocked_DeleteChildNode(
    DEVNODE *   apNode,
    DEVNODE *   apChild
)
{
    K2_ASSERT(NULL != apNode);
    K2_ASSERT(NULL != apChild);
    K2_ASSERT(apChild->RefToParent.mpDevNode == apNode);

    K2LIST_Remove(&apNode->InSec.ChildList, &apChild->InParentSec.ParentChildListLink);

    K2OS_CritSec_Enter(&apChild->Sec);

    K2_ASSERT(DEVNODESTATE_OFFLINE_FIRST <= apChild->InSec.mState);

    K2_ASSERT(0 == apChild->InSec.ChildList.mNodeCount);
    K2_ASSERT(0 == apChild->InSec.IrqResList.mNodeCount);
    K2_ASSERT(0 == apChild->InSec.IoResList.mNodeCount);
    K2_ASSERT(0 == apChild->InSec.PhysResList.mNodeCount);

    K2_ASSERT(NULL == apChild->InSec.Driver.mpLaunchCmd);
    K2_ASSERT(FALSE == apChild->InSec.Driver.mObjectExists);
    K2_ASSERT(NULL == apChild->InSec.Driver.mMailslotToken);
    K2_ASSERT(NULL == apChild->InSec.Driver.mProcessToken);
    
    K2OS_CritSec_Leave(&apChild->Sec);

    K2OS_CritSec_Done(&apChild->Sec);

    Dev_ReleaseRef(&apChild->RefToParent);

    K2OS_Heap_Free(apChild);
}

void
DevMgr_NodeLocked_CheckProcessStopped(
    DEVNODE * apNode
)
{
    K2_ASSERT(0 == apNode->Driver.InstanceTreeNode.mUserVal);

    if (K2OS_THREAD_WAIT_SIGNALLED_0 == K2OS_Wait_One(apNode->InSec.Driver.mProcessToken, 0))
    {
        apNode->InSec.Driver.mLastStatus = K2OS_Root_GetProcessExitCode(apNode->InSec.Driver.mProcessToken);
        K2OS_Token_Destroy(apNode->InSec.Driver.mProcessToken);
        apNode->InSec.Driver.mProcessToken = NULL;
        Dev_NodeLocked_OnStop(apNode);
    }
    else
    {
        DevMgr_NodeLocked_AddTimer(apNode, DRIVER_PROCEXIT_CHECK_INTERVAL_MS);
    }
}

void        
Dev_NodeLocked_OnEvent_ChildDriverSpecChanged(
    DEVNODE *apNode,
    DEVNODE *apChild
)
{
    UINT_PTR        sLen;
    K2LIST_LINK *   pListLink;
    DEVNODE *       pCheck;

    //
    // verify child still exists
    //
    pListLink = apNode->InSec.ChildList.mpHead;
    if (NULL == pListLink)
    {
        return;
    }
    do {
        pCheck = K2_GET_CONTAINER(DEVNODE, pListLink, InParentSec.ParentChildListLink);
        if (pCheck == apChild)
            break;
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);
    if (NULL == pListLink)
    {
        return;
    }

    K2OS_CritSec_Enter(&apChild->Sec);

    do {
        if (DevNodeState_Offline_CheckForDriver != apChild->InSec.mState)
        {
            break;
        }

        if (NULL == apChild->InSec.mpDriverCandidate)
        {
            apChild->InSec.mState = DevNodeState_Offline_NoDriver;
            break;
        }

        sLen = K2ASC_Len(apChild->InSec.mpDriverCandidate);
        if (apChild->InSec.Driver.mpLaunchCmd)
        {
            K2OS_Heap_Free(apChild->InSec.Driver.mpLaunchCmd);
            apChild->InSec.Driver.mpLaunchCmd = NULL;
        }

        apChild->InSec.Driver.mpLaunchCmd = (char *)K2OS_Heap_Alloc((sLen + 36) & ~3);
        if (NULL == apChild->InSec.Driver.mpLaunchCmd)
        {
            apChild->InSec.Driver.mLastStatus = K2STAT_ERROR_OUT_OF_MEMORY;
            apChild->InSec.mState = DevNodeState_Offline_ErrorStopped;
            DevMgr_QueueEvent(apNode, DevNodeEvent_ChildStopped, (UINT_PTR)apChild);
            break;
        }

        K2ASC_PrintfLen(apChild->InSec.Driver.mpLaunchCmd, 
            (sLen + 36) & ~3, "%s %d %d", apChild->InSec.mpDriverCandidate,
            apNode->Driver.InstanceTreeNode.mUserVal,
            apChild->mParentDriverOwned_RemoteObjectId);

        apChild->InSec.mState = DevNodeState_GoingOnline_WaitObject;

        K2OS_CritSec_Enter(&gDriverInstanceTreeSec);

        apChild->InSec.Driver.mProcessToken = K2OS_Root_Launch(
            ":k2osdrv.xdl",
            apChild->InSec.Driver.mpLaunchCmd,
            &apChild->Driver.InstanceTreeNode.mUserVal);

        if (NULL == apChild->InSec.Driver.mProcessToken)
        {
            K2OS_CritSec_Leave(&gDriverInstanceTreeSec);
            apChild->InSec.Driver.mLastStatus = K2OS_Thread_GetLastStatus();
            apChild->InSec.mState = DevNodeState_Offline_ErrorStopped;
            DevMgr_QueueEvent(apChild->RefToParent.mpDevNode, DevNodeEvent_ChildStopped, (UINT_PTR)apChild);
            break;
        }

        //
        // add the wait-for-object timeout timer
        // 
        DevMgr_NodeLocked_AddTimer(apChild, DRIVER_CREATEOBJECT_TIMEOUT_MS);

        K2_ASSERT(0 != apChild->Driver.InstanceTreeNode.mUserVal);

        K2TREE_Insert(&gDriverInstanceTree, apChild->Driver.InstanceTreeNode.mUserVal, &apChild->Driver.InstanceTreeNode);

        K2OS_CritSec_Leave(&gDriverInstanceTreeSec);

    } while (0);

    K2OS_CritSec_Leave(&apChild->Sec);
}

K2STAT      
Dev_NodeLocked_OnDriverHostObjCreate(
    DEVNODE *   apNode
)
{
    if (apNode->InSec.mState != DevNodeState_GoingOnline_WaitObject)
    {
        Debug_Printf("*** node not in waitobject state when receive driver host obj create\n");
        return K2STAT_ERROR_REJECTED;
    }

    DevMgr_NodeLocked_DelTimer(apNode);

    apNode->InSec.Driver.mObjectExists = TRUE;
    apNode->InSec.mState = DevNodeState_GoingOnline_WaitMailbox;

    DevMgr_NodeLocked_AddTimer(apNode, DRIVER_SETMAILBOX_TIMEOUT_MS);

    return K2STAT_NO_ERROR;
}

K2STAT      
Dev_NodeLocked_OnSetMailbox(
    DEVNODE *   apNode,
    UINT_PTR    aRemoteToken
)
{
    K2OS_MSG    msg;
    K2STAT      stat;

    if (apNode->InSec.mState != DevNodeState_GoingOnline_WaitMailbox)
    {
        Debug_Printf("*** node not in waitmailbox state when receive mailbox spec\n");
        return K2STAT_ERROR_REJECTED;
    }

    DevMgr_NodeLocked_DelTimer(apNode);

    apNode->InSec.Driver.mMailslotToken = K2OS_Root_CreateTokenForRoot(apNode->InSec.Driver.mProcessToken, aRemoteToken);
    if (NULL == apNode->InSec.Driver.mMailslotToken)
    {
        return K2OS_Thread_GetLastStatus();
    }

    K2MEM_Zero(&msg, sizeof(msg));
    msg.mControl = K2OS_MSG_CONTROL_DRV_START;

    apNode->InSec.mState = DevNodeState_Online;

    stat = K2OS_Mailslot_Send(apNode->InSec.Driver.mMailslotToken, &msg);

    if (K2STAT_IS_ERROR(stat))
    {
        //
        // failed to send start command so ask kernel to stop the process
        // once it stops the object will get deleted which will trigger
        // all the cleanup
        //
        apNode->InSec.mState = DevNodeState_GoingOffline_WaitObjDel;
        apNode->InSec.Driver.mLastStatus = stat;

        K2OS_Token_Destroy(apNode->InSec.Driver.mMailslotToken);
        apNode->InSec.Driver.mMailslotToken = NULL;

        K2OS_Root_StopProcess(apNode->InSec.Driver.mProcessToken, (UINT_PTR)stat, FALSE);
        
        return stat;
    }

    return K2STAT_NO_ERROR;
}

void
Dev_NodeLocked_OnDriverHostObjDelete(
    DEVNODE *apNode
)
{
    BOOL delayKill;

    DevMgr_NodeLocked_DelTimer(apNode);

    delayKill = FALSE;

    if (DevNodeState_Online == apNode->InSec.mState)
    {
        if (DevNodeIntent_None == apNode->InSec.mIntent)
        {
            //
            // no intent and object was deleted. the process
            // is either exiting to restart or crashed.
            //
            Debug_Printf("Node %08X deleted its host object unexpectedly. deferring intent.\n", apNode);
            apNode->InSec.mIntent = DevNodeIntent_DeferToProcessExitCode;
            delayKill = TRUE;
        }
    }
    else
    {
        //
        // intent must have been set before switching out of online state
        //
        K2_ASSERT(DevNodeIntent_None != apNode->InSec.mIntent);
    }

    apNode->InSec.Driver.mObjectExists = FALSE;

    K2OS_CritSec_Enter(&gDriverInstanceTreeSec);
    K2_ASSERT(0 != apNode->Driver.InstanceTreeNode.mUserVal);
    K2TREE_Remove(&gDriverInstanceTree, &apNode->Driver.InstanceTreeNode);
    apNode->Driver.InstanceTreeNode.mUserVal = 0;
    K2OS_CritSec_Leave(&gDriverInstanceTreeSec);

    if (NULL != apNode->InSec.Driver.mMailslotToken)
    {
        K2OS_Token_Destroy(apNode->InSec.Driver.mMailslotToken);
        apNode->InSec.Driver.mMailslotToken = NULL;
    }

    if (delayKill)
    {
        if (K2OS_THREAD_WAIT_SIGNALLED_0 == K2OS_Wait_One(apNode->InSec.Driver.mProcessToken, 0))
        {
            apNode->InSec.Driver.mLastStatus = K2OS_Root_GetProcessExitCode(apNode->InSec.Driver.mProcessToken);
            K2OS_Token_Destroy(apNode->InSec.Driver.mProcessToken);
            apNode->InSec.Driver.mProcessToken = NULL;
            Dev_NodeLocked_OnStop(apNode);
        }
        else
        {
            apNode->InSec.mState = DevNodeState_GoingOffline_DelayKill;
            apNode->InSec.Driver.mForcedExitCountLeft = 4;
            DevMgr_NodeLocked_AddTimer(apNode, DRIVER_DELAYKILL_CHECK_INTERVAL_MS);
        }
    }
    else
    {
        apNode->InSec.mState = DevNodeState_GoingOffline_WaitProcExit;
        DevMgr_NodeLocked_CheckProcessStopped(apNode);
    }
}

void
Dev_NodeLocked_OnStop(
    DEVNODE *apNode
)
{
    if (apNode->InSec.mIntent == DevNodeIntent_DeferToProcessExitCode)
    {
        if (K2STAT_IS_ERROR(apNode->InSec.Driver.mLastStatus))
        {
            Debug_Printf("Node %08X deferred intent on object delete reified to Error-Stop (exit code %08X)\n", apNode, apNode->InSec.Driver.mLastStatus);
            apNode->InSec.mIntent = DevNodeIntent_ErrorStop;
        }
        else
        {
            Debug_Printf("Node %08X deferred intent on object delete reified to Restart (exit code %08X)\n", apNode, apNode->InSec.Driver.mLastStatus);
            apNode->InSec.mIntent = DevNodeIntent_Restart;
        }
    }

    if (apNode->InSec.mIntent == DevNodeIntent_ErrorStop)
    {
        apNode->InSec.mState = DevNodeState_Offline_ErrorStopped;
        DevMgr_QueueEvent(apNode->RefToParent.mpDevNode, DevNodeEvent_ChildStopped, (UINT_PTR)apNode);
        Debug_Printf("Node %08X stopped due to error (last status %08X)\n", apNode, apNode->InSec.Driver.mLastStatus);
    }
    else if (apNode->InSec.mIntent == DevNodeIntent_NormalStop)
    {
        apNode->InSec.mState = DevNodeState_Offline_Stopped;
        DevMgr_QueueEvent(apNode->RefToParent.mpDevNode, DevNodeEvent_ChildStopped, (UINT_PTR)apNode);
        Debug_Printf("Node %08X stopped normally (last status %08X)\n", apNode, apNode->InSec.Driver.mLastStatus);
    }
    else
    {
        K2_ASSERT(DevNodeIntent_Restart == apNode->InSec.mIntent);
        apNode->InSec.mIntent = DevNodeIntent_None;
        Debug_Printf("TBD: Restart node %08x\n", apNode);
    }
}

K2STAT
Dev_NodeLocked_OnEnumRes(
    DEVNODE *               apNode,
    K2OSDRV_ENUM_RES_OUT *  apOut
)
{
    if (DevNodeState_Online != apNode->InSec.mState)
    {
        return K2STAT_ERROR_NOT_RUNNING;
    }
    if (apNode->InSec.Driver.mEnabled)
    {
        return K2STAT_ERROR_API_ORDER;
    }

    apOut->mAcpiInfoBytes = apNode->InSec.mMountedInfoBytes;
    apOut->mCountIo = apNode->InSec.IoResList.mNodeCount;
    apOut->mCountPhys = apNode->InSec.PhysResList.mNodeCount;
    apOut->mCountIrq = apNode->InSec.IrqResList.mNodeCount;

    return K2STAT_NO_ERROR;
}

void
Dev_NodeLocked_OnEvent_TimerExpired(
    DEVNODE *   apNode
)
{
    if (apNode->InSec.mState == DevNodeState_GoingOnline_WaitObject)
    {
        K2OS_Root_StopProcess(apNode->InSec.Driver.mProcessToken, K2STAT_ERROR_TIMEOUT, FALSE);
        apNode->InSec.Driver.mLastStatus = K2STAT_ERROR_TIMEOUT;
        apNode->InSec.mState = DevNodeState_GoingOffline_WaitProcExit;
        apNode->InSec.mIntent = DevNodeIntent_ErrorStop;
        DevMgr_NodeLocked_AddTimer(apNode, DRIVER_PROCEXIT_CHECK_INTERVAL_MS);
    }
    else if (apNode->InSec.mState == DevNodeState_GoingOnline_WaitMailbox)
    {
        K2OS_CritSec_Enter(&gDriverInstanceTreeSec);
        K2_ASSERT(0 != apNode->Driver.InstanceTreeNode.mUserVal);
        K2TREE_Remove(&gDriverInstanceTree, &apNode->Driver.InstanceTreeNode);
        apNode->Driver.InstanceTreeNode.mUserVal = 0;
        K2OS_CritSec_Leave(&gDriverInstanceTreeSec);

        K2OS_Root_StopProcess(apNode->InSec.Driver.mProcessToken, K2STAT_ERROR_TIMEOUT, FALSE);
        apNode->InSec.Driver.mLastStatus = K2STAT_ERROR_TIMEOUT;
        apNode->InSec.mState = DevNodeState_GoingOffline_WaitObjDel;
        apNode->InSec.mIntent = DevNodeIntent_ErrorStop;
    }
    else if (apNode->InSec.mState == DevNodeState_GoingOffline_DelayKill)
    {
        if (K2OS_THREAD_WAIT_SIGNALLED_0 == K2OS_Wait_One(apNode->InSec.Driver.mProcessToken, 0))
        {
            //
            // process stopped on its own before the timers expired
            //
            apNode->InSec.Driver.mLastStatus = K2OS_Root_GetProcessExitCode(apNode->InSec.Driver.mProcessToken);
            K2OS_Token_Destroy(apNode->InSec.Driver.mProcessToken);
            apNode->InSec.Driver.mProcessToken = NULL;
            Dev_NodeLocked_OnStop(apNode);
        }
        else
        {
            //
            // still not stopped
            //
            if (0 == --apNode->InSec.Driver.mForcedExitCountLeft)
            {
                //
                // deleted driver host object but did not stop in a timely fashion after that
                // so we are killing the process
                //
                K2OS_Root_StopProcess(apNode->InSec.Driver.mProcessToken, K2STAT_ERROR_TIMEOUT, FALSE);
                apNode->InSec.Driver.mLastStatus = K2STAT_ERROR_TIMEOUT;
                apNode->InSec.mState = DevNodeState_GoingOffline_WaitProcExit;
                apNode->InSec.mIntent = DevNodeIntent_ErrorStop;
            }
            else
            {
                //
                // counts left
                //
                DevMgr_NodeLocked_AddTimer(apNode, DRIVER_DELAYKILL_CHECK_INTERVAL_MS);
            }
        }
    }
    else if (apNode->InSec.mState == DevNodeState_GoingOffline_WaitProcExit)
    {
        DevMgr_NodeLocked_CheckProcessStopped(apNode);
    }
}

void        
Dev_NodeLocked_OnEvent(
    DEVNODE *           apNode,
    DevNodeEventType    aType,
    UINT_PTR            aArg
)
{
    switch (aType)
    {
    case DevNodeEvent_TimerExpired:
        Dev_NodeLocked_OnEvent_TimerExpired(apNode);
        break;

    case DevNodeEvent_ChildStopped:
        Debug_Printf("Node(%08X): ChildStopped (%08X)\n", apNode, aArg);
        break;

    case DevNodeEvent_ChildRefsHitZero:
        Debug_Printf("Node(%08X): ChildRefsHitZero (%08X)\n", apNode, aArg);
        break;

    case DevNodeEvent_NewChildMounted:
        Debug_Printf("Node(%08X): NewChildMounted (%08X)\n", apNode, aArg);
        break;

    case DevNodeEvent_ChildDriverSpecChanged:
        Dev_NodeLocked_OnEvent_ChildDriverSpecChanged(apNode, (DEVNODE *)aArg);
        break;

    default:
        K2_ASSERT(0);
    };
}

K2STAT
Dev_NodeLocked_OnGetRes(
    DEVNODE *                   apNode,
    K2OSDRV_GET_RES_IN const *  apArgsIn,
    UINT8 *                     apOutBuf,
    UINT_PTR                    aOutBufBytes,
    UINT_PTR *                  apRetUsedOutBytes
)
{
    K2STAT                      stat;
    K2LIST_LINK *               pListLink;
    DEV_IO *                    pIo;
    DEV_PHYS *                  pPhys;
    DEV_IRQ *                   pIrq;
    K2OSDRV_RES_IO *            pOutIo;
    K2OSDRV_RES_PHYS *          pOutPhys;
    K2OSDRV_RES_IRQ *           pOutIrq;
    UINT_PTR                    target;

    if (DevNodeState_Online != apNode->InSec.mState)
    {
        return K2STAT_ERROR_NOT_RUNNING;
    }
    if (apNode->InSec.Driver.mEnabled)
    {
        return K2STAT_ERROR_API_ORDER;
    }

    stat = K2STAT_ERROR_UNKNOWN;

    switch (apArgsIn->mResType)
    {
    case K2OSDRV_RESTYPE_ACPI:
        if (aOutBufBytes < apNode->InSec.mMountedInfoBytes)
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else
        {
            K2MEM_Copy(apOutBuf, apNode->InSec.mpMountedInfo, apNode->InSec.mMountedInfoBytes);
            *apRetUsedOutBytes = apNode->InSec.mMountedInfoBytes;
            stat = K2STAT_NO_ERROR;
        }
        break;

    case K2OSDRV_RESTYPE_IO:
        target = apArgsIn->mIndex;

        if (target >= apNode->InSec.IoResList.mNodeCount)
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else if (aOutBufBytes < sizeof(K2OSDRV_RES_IO))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else
        {
            pListLink = apNode->InSec.IoResList.mpHead;
            while (target > 0)
            {
                pListLink = pListLink->mpNext;
                K2_ASSERT(NULL != pListLink);
                --target;
            }
            pIo = K2_GET_CONTAINER(DEV_IO, pListLink, Res.ListLink);

            pOutIo = (K2OSDRV_RES_IO *)apOutBuf;
            K2MEM_Copy(pOutIo, &pIo->DrvResIo, sizeof(K2OSDRV_RES_IO));

            if (!K2OS_Root_AddIoRange(apNode->InSec.Driver.mProcessToken, pOutIo->mBasePort, pOutIo->mSizeBytes))
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
            }
            else
            {
                *apRetUsedOutBytes = sizeof(K2OSDRV_RES_IO);
                stat = K2STAT_NO_ERROR;
            }
        }
        break;

    case K2OSDRV_RESTYPE_PHYS:
        target = apArgsIn->mIndex;

        if (target >= apNode->InSec.PhysResList.mNodeCount)
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else if (aOutBufBytes < sizeof(K2OSDRV_RES_PHYS))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else
        {
            pListLink = apNode->InSec.PhysResList.mpHead;
            while (target > 0)
            {
                pListLink = pListLink->mpNext;
                K2_ASSERT(NULL != pListLink);
                --target;
            }
            pPhys = K2_GET_CONTAINER(DEV_PHYS, pListLink, Res.ListLink);

            pOutPhys = (K2OSDRV_RES_PHYS *)apOutBuf;
            K2MEM_Copy(pOutPhys, &pPhys->DrvResPhys, sizeof(K2OSDRV_RES_PHYS));
            pOutPhys->mPageArrayToken = (K2OS_PAGEARRAY_TOKEN)K2OS_Root_CreateTokenForProcess(
                apNode->InSec.Driver.mProcessToken,
                pPhys->DrvResPhys.mPageArrayToken);
            if (NULL == pOutPhys->mPageArrayToken)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
            }
            else
            {
                *apRetUsedOutBytes = sizeof(K2OSDRV_RES_PHYS);
                stat = K2STAT_NO_ERROR;
            }
        }
        break;

    case K2OSDRV_RESTYPE_IRQ:
        target = apArgsIn->mIndex;

        if (target >= apNode->InSec.IrqResList.mNodeCount)
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else if (aOutBufBytes < sizeof(K2OSDRV_RES_IRQ))
        {
            stat = K2STAT_ERROR_BAD_SIZE;
        }
        else
        {
            pListLink = apNode->InSec.IrqResList.mpHead;
            while (target > 0)
            {
                pListLink = pListLink->mpNext;
                K2_ASSERT(NULL != pListLink);
                --target;
            }
            pIrq = K2_GET_CONTAINER(DEV_IRQ, pListLink, Res.ListLink);

            pOutIrq = (K2OSDRV_RES_IRQ *)apOutBuf;

            K2MEM_Copy(pOutIrq, &pIrq->DrvResIrq, sizeof(K2OSDRV_RES_IRQ));

            pOutIrq->mInterruptToken = (K2OS_INTERRUPT_TOKEN)K2OS_Root_CreateTokenForProcess(
                apNode->InSec.Driver.mProcessToken,
                pIrq->DrvResIrq.mInterruptToken);

            if (NULL == pOutIrq->mInterruptToken)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
            }
            else
            {
                *apRetUsedOutBytes = sizeof(K2OSDRV_RES_IRQ);
                stat = K2STAT_NO_ERROR;
            }
        }
        break;

    default:
        stat = K2STAT_ERROR_BAD_ARGUMENT;
    }

    return stat;
}

void
Dev_Describe(
    DEVNODE *       apNode,
    UINT64 const *  apBusAddress,
    UINT_PTR *      apRetNodeName,
    char **         appOut,
    UINT_PTR *      apLeft
)
{
    UINT_PTR    ate;
    UINT_PTR    left;
    char *      pEnd;

    left = *apLeft;
    pEnd = *appOut;

    if (0 != (*apBusAddress))
    {
        ate = K2ASC_PrintfLen(pEnd, left, "BUSADDR(%08X,%08X);", (UINT_PTR)((*apBusAddress) >> 32), (UINT_PTR)((*apBusAddress) & 0xFFFFFFFFull));
        left -= ate;
        pEnd += ate;
    }

    if (apNode->DeviceIdent.mVendorId != 0)
    {
        ate = K2ASC_PrintfLen(pEnd, left, "VEN(%04X);", apNode->DeviceIdent.mVendorId);
        left -= ate;
        pEnd += ate;
    }

    if (apNode->DeviceIdent.mVendorId != 0)
    {
        ate = K2ASC_PrintfLen(pEnd, left, "DEV(%04X);", apNode->DeviceIdent.mDeviceId);
        left -= ate;
        pEnd += ate;
    }

    if (NULL != apNode->InSec.mpAcpiNode)
    {
        K2MEM_Copy(apRetNodeName, apNode->InSec.mpAcpiNode->mName, sizeof(UINT32));
        Acpi_Describe(apNode->InSec.mpAcpiNode, &pEnd, &left);
    }
    else
    {
        switch (apNode->mBusType)
        {
        case K2OSDRV_BUSTYPE_CPU:
            K2MEM_Copy(apRetNodeName, "CPU_", sizeof(UINT32));
            break;
        case K2OSDRV_BUSTYPE_PCI:
            K2MEM_Copy(apRetNodeName, "PCI_", sizeof(UINT32));
            break;
        case K2OSDRV_BUSTYPE_USB:
            K2MEM_Copy(apRetNodeName, "USB_", sizeof(UINT32));
            break;
        case K2OSDRV_BUSTYPE_IDE:
            K2MEM_Copy(apRetNodeName, "IDE_", sizeof(UINT32));
            break;
        default:
            K2_ASSERT(0);
            break;
        }
    }

    *appOut = pEnd;
    *apLeft = left;
}

K2STAT
Dev_NodeLocked_MountChild(
    DEVNODE *                       apNode,
    K2OSDRV_MOUNT_CHILD_IN const *  apArgsIn,
    K2OSDRV_MOUNT_CHILD_OUT *       apArgsOut
)
{
    K2STAT      stat;
    DEVNODE *   pChild;
    char *      pIoBuf;
    char *      pOut;
    UINT_PTR    left;
    UINT_PTR    ioBytes;
    UINT_PTR    nodeName;

    if (DevNodeState_Online != apNode->InSec.mState)
    {
        return K2STAT_ERROR_NOT_RUNNING;
    }

    if (0 == apArgsIn->mChildObjectId)
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    Debug_Printf("\nMountChild flags %08X, remote objid %d, adr %d/%d\n",
        apArgsIn->mMountFlags,
        apArgsIn->mChildObjectId,
        (UINT32)((apArgsIn->mBusSpecificAddress & 0xFFFF0000) >> 16), (UINT32)(apArgsIn->mBusSpecificAddress & 0xFFFF));

    pIoBuf = (char *)K2OS_Heap_Alloc(ROOTPLAT_MOUNT_MAX_BYTES);
    if (NULL == pIoBuf)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    stat = K2STAT_ERROR_UNKNOWN;
    do {
        pChild = Dev_NodeLocked_CreateChildNode(apNode, &apArgsIn->Ident);
        if (NULL == pChild)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
            break;
        }
        do {
            pChild->mBusType = (apArgsIn->mMountFlags & K2OSDRV_MOUNTFLAGS_BUSTYPE_MASK);

            pChild->mParentDriverOwned_RemoteObjectId = apArgsIn->mChildObjectId;

            pOut = pIoBuf;
            *pOut = 0;
            left = ROOTPLAT_MOUNT_MAX_BYTES;
            if (NULL != apNode->InSec.mpAcpiNode)
            {
                if (Acpi_MatchAndAttachChild(apNode->InSec.mpAcpiNode, &apArgsIn->mBusSpecificAddress, pChild))
                {
                    Debug_Printf("!!! Matched existing acpi node\n");
                    K2_ASSERT(NULL != pChild->InSec.mpAcpiNode);
                }
            }

            Dev_Describe(pChild, &apArgsIn->mBusSpecificAddress, &nodeName, &pOut, &left);

            ioBytes = ((UINT_PTR)(pOut - pIoBuf)) + 1;

            pChild->InSec.mpMountedInfo = (char *)K2OS_Heap_Alloc((ioBytes + 4) & ~3);
            if (NULL == pChild->InSec.mpMountedInfo)
            {
                Debug_Printf("*** Failed to allocate memory for mount info\n");
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
                break;
            }

            do {
                K2MEM_Copy(pChild->InSec.mpMountedInfo, pIoBuf, ioBytes);
                pChild->InSec.mMountedInfoBytes = ioBytes + 1;
                pChild->InSec.mpMountedInfo[ioBytes] = 0;

                pChild->InSec.mPlatContext = K2OS_Root_CreatePlatNode(
                    apNode->InSec.mPlatContext,
                    nodeName,
                    (UINT_PTR)pChild,
                    pIoBuf,
                    ROOTPLAT_MOUNT_MAX_BYTES,
                    &ioBytes);

                if (0 == pChild->InSec.mPlatContext)
                {
                    Debug_Printf("*** Mount of plat node failed (last status 0x%08X)\n", K2OS_Thread_GetLastStatus());
                    stat = K2OS_Thread_GetLastStatus();
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                }

                apArgsOut->mSystemDeviceInstanceId = (UINT_PTR)0xDEADF00D;

                if (ioBytes > 0)
                {
                    K2_ASSERT(ioBytes <= ROOTPLAT_MOUNT_MAX_BYTES);

                    pChild->InSec.mpPlatInfo = (UINT8 *)K2OS_Heap_Alloc((ioBytes + 4) & ~3);
                    if (NULL == pChild->InSec.mpPlatInfo)
                    {
                        Debug_Printf("*** Could not allocate memory to hold node plat output\n");
                        stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    }
                    else
                    {
                        K2MEM_Copy(pChild->InSec.mpPlatInfo, pIoBuf, ioBytes);
                        pChild->InSec.mpPlatInfo[ioBytes] = 0;
                        pChild->InSec.mPlatInfoBytes = ioBytes;
                        stat = K2STAT_NO_ERROR;
                    }
                }
                else
                {
                    stat = K2STAT_NO_ERROR;
                }

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OS_Root_DeletePlatNode(pChild->InSec.mPlatContext);
                    pChild->InSec.mPlatContext = 0;
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_Heap_Free(pChild->InSec.mpMountedInfo);
                pChild->InSec.mpMountedInfo = NULL;
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            Dev_ReleaseRef(&pChild->RefToSelf_OwnedByParent);
        }

    } while (0);

    K2OS_Heap_Free(pIoBuf);

    if (!K2STAT_IS_ERROR(stat))
    {
        if (NULL != pChild->InSec.mpAcpiNode)
        {
            Acpi_NodeLocked_PopulateRes(pChild);
        }

        DevMgr_QueueEvent(apNode, DevNodeEvent_NewChildMounted, (UINT_PTR)pChild);
    }

    return stat;
}

K2STAT
Dev_NodeLocked_RunAcpiMethod(
    DEVNODE *                   apNode,
    K2OSDRV_RUN_ACPI_IN const * apArgs,
    UINT_PTR *                  apRetResult
)
{
    char acpiMethod[8];

    if (DevNodeState_Online != apNode->InSec.mState)
    {
        Debug_Printf("*** RunAcpiMethod for node that is not online\n");
        return K2STAT_ERROR_NOT_RUNNING;
    }

    if (NULL == apNode->InSec.mpAcpiNode)
    {
        Debug_Printf("*** Attempt to run ACPI method on a node that has no ACPI\n");
        return K2STAT_ERROR_NOT_FOUND;
    }

    K2MEM_Copy(&acpiMethod, &apArgs->mMethod, sizeof(UINT32));
    acpiMethod[4] = 0;

    return Acpi_RunMethod(apNode->InSec.mpAcpiNode, acpiMethod,
                (0 == (apArgs->mFlags & K2OSDRV_RUN_ACPI_FLAG_HAS_INPUT)) ? FALSE : TRUE,
                apArgs->mIn,
                apRetResult);
}

K2STAT      
Dev_NodeLocked_Enable(
    DEVNODE *   apNode
)
{
    K2LIST_LINK *   pListLink;
    DEVNODE *       pChild;

    if (DevNodeState_Online != apNode->InSec.mState)
    {
        Debug_Printf("*** RunAcpiMethod for node that is not online\n");
        return K2STAT_ERROR_NOT_RUNNING;
    }

    if (apNode->InSec.Driver.mEnabled)
    {
        return K2STAT_NO_ERROR;
    }

    Debug_Printf("Node(%08X): Enabling\n", apNode);

    K2_ASSERT(DevNodeState_Online == apNode->InSec.mState);

    apNode->InSec.Driver.mEnabled = TRUE;

    pListLink = apNode->InSec.ChildList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pChild = K2_GET_CONTAINER(DEVNODE, pListLink, InParentSec.ParentChildListLink);
            pListLink = pListLink->mpNext;

            K2OS_CritSec_Enter(&pChild->Sec);

            if (0 != pChild->InSec.mPlatInfoBytes)
            {
                pChild->InSec.mpDriverCandidate = (char *)K2OS_Heap_Alloc((pChild->InSec.mPlatInfoBytes + 4) & ~3);
                if (NULL == pChild->InSec.mpDriverCandidate)
                {
                    Debug_Printf("*** Memory allocation failure creating driver candidate\n");
                    pChild->InSec.mState = DevNodeState_Offline_NoDriver;
                }
                else
                {
                    K2MEM_Copy(pChild->InSec.mpDriverCandidate, pChild->InSec.mpPlatInfo, pChild->InSec.mPlatInfoBytes);
                    pChild->InSec.mpDriverCandidate[pChild->InSec.mPlatInfoBytes] = 0;
                    pChild->InSec.mState = DevNodeState_Offline_CheckForDriver;
                    Debug_Printf("Sending ChildDriverSpecChanged event\n");
                    DevMgr_QueueEvent(apNode, DevNodeEvent_ChildDriverSpecChanged, (UINT_PTR)pChild);
                }
            }
            else
            {
                pChild->InSec.mState = DevNodeState_Offline_NoDriver;
            }

            K2OS_CritSec_Leave(&pChild->Sec);

        } while (NULL != pListLink);
    }

    return K2STAT_NO_ERROR;
}

K2STAT      
Dev_NodeLocked_Disable(
    DEVNODE *           apNode,
    DevNodeIntentType   aIntent
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT      
Dev_NodeLocked_AddChildRes(
    DEVNODE *                       apNode,
    K2OSDRV_ADD_CHILD_RES_IN const *apArgsIn
)
{
    K2STAT          stat;
    K2LIST_LINK *   pListLink;
    DEVNODE *       pChild;
    DEV_IO *        pIo;
    DEV_PHYS *      pPhys;
    DEV_IRQ *       pIrq;
    UINT32 *        pu;

    if (apNode->InSec.mState != DevNodeState_Online)
    {
        return K2STAT_ERROR_NOT_RUNNING;
    }

    if (apNode->InSec.Driver.mEnabled)
    {
        return K2STAT_ERROR_ENABLED;
    }

    //
    // verify child exists
    //
    pListLink = apNode->InSec.ChildList.mpHead;
    if (NULL == pListLink)
    {
        return K2STAT_ERROR_NOT_FOUND;
    }
    do {
        pChild = K2_GET_CONTAINER(DEVNODE, pListLink, InParentSec.ParentChildListLink);
        if (pChild->mParentDriverOwned_RemoteObjectId == apArgsIn->mChildObjectId)
            break;
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);
    if (NULL == pListLink)
    {
        return K2STAT_ERROR_NOT_FOUND;
    }

    K2OS_CritSec_Enter(&pChild->Sec);

    if (0 == pChild->InSec.mPlatContext)
    {
        stat = K2STAT_ERROR_NOT_READY;
    }
    else
    {
        switch (apArgsIn->mResType)
        {
        case K2OSDRV_RESTYPE_IO:
            pListLink = pChild->InSec.IoResList.mpHead;
            if (NULL != pListLink)
            {
                do {
                    pIo = K2_GET_CONTAINER(DEV_IO, pListLink, Res.ListLink);
                    if ((pIo->DrvResIo.mBasePort == apArgsIn->Desc.IoPortRange.mBasePort) &&
                        (pIo->DrvResIo.mSizeBytes == apArgsIn->Desc.IoPortRange.mSizeBytes))
                    {
                        //
                        // already know about this one
                        //
                        stat = K2STAT_NO_ERROR;
                        break;
                    }
                    pListLink = pListLink->mpNext;
                } while (NULL != pListLink);
            }
            if (NULL == pListLink)
            {
                //
                // range not known (yet) so try to add it
                //
                pIo = (DEV_IO *)K2OS_Heap_Alloc(sizeof(DEV_IO));
                if (NULL == pIo)
                {
                    stat = K2STAT_ERROR_NOT_FOUND;
                    break;
                }
                K2MEM_Zero(pIo, sizeof(DEV_IO));
                pIo->DrvResIo.mBasePort = apArgsIn->Desc.IoPortRange.mBasePort;
                pIo->DrvResIo.mSizeBytes = apArgsIn->Desc.IoPortRange.mSizeBytes;
                pIo->Res.mPlatContext = K2OS_Root_AddPlatResource(
                    pChild->InSec.mPlatContext, 
                    K2OS_PLATRES_IO, 
                    pIo->DrvResIo.mBasePort,
                    pIo->DrvResIo.mSizeBytes
                );
                if (0 == pIo->Res.mPlatContext)
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2OS_Heap_Free(pIo);
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                    break;
                }
                K2LIST_AddAtTail(&pChild->InSec.IoResList, &pIo->Res.ListLink);
                stat = K2STAT_NO_ERROR;
            }
            break;

        case K2OSDRV_RESTYPE_PHYS:
            pListLink = pChild->InSec.PhysResList.mpHead;
            if (NULL != pListLink)
            {
                do {
                    pPhys = K2_GET_CONTAINER(DEV_PHYS, pListLink, Res.ListLink);
                    if ((pPhys->DrvResPhys.Range.mBaseAddr == apArgsIn->Desc.PhysRange.mBaseAddr) &&
                        (pPhys->DrvResPhys.Range.mSizeBytes == apArgsIn->Desc.PhysRange.mSizeBytes))
                    {
                        //
                        // already know about this one
                        //
                        stat = K2STAT_NO_ERROR;
                        break;
                    }
                    pListLink = pListLink->mpNext;
                } while (NULL != pListLink);
            }
            if (NULL == pListLink)
            {
                //
                // range not known (yet) so try to add it
                //
                pPhys = (DEV_PHYS *)K2OS_Heap_Alloc(sizeof(DEV_PHYS));
                if (NULL == pPhys)
                {
                    stat = K2STAT_ERROR_NOT_FOUND;
                    break;
                }
                K2MEM_Zero(pPhys, sizeof(DEV_PHYS));
                pPhys->DrvResPhys.Range.mBaseAddr = apArgsIn->Desc.PhysRange.mBaseAddr;
                pPhys->DrvResPhys.Range.mSizeBytes = apArgsIn->Desc.PhysRange.mSizeBytes;
                pPhys->Res.mPlatContext = K2OS_Root_AddPlatResource(
                    pChild->InSec.mPlatContext,
                    K2OS_PLATRES_PHYS,
                    pPhys->DrvResPhys.Range.mBaseAddr,
                    pPhys->DrvResPhys.Range.mSizeBytes
                );
                if (0 == pPhys->Res.mPlatContext)
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2OS_Heap_Free(pPhys);
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                    break;
                }
                K2LIST_AddAtTail(&pChild->InSec.PhysResList, &pPhys->Res.ListLink);
                stat = K2STAT_NO_ERROR;
            }
            break;

        case K2OSDRV_RESTYPE_IRQ:
            pListLink = pChild->InSec.IrqResList.mpHead;
            if (NULL != pListLink)
            {
                do {
                    pIrq = K2_GET_CONTAINER(DEV_IRQ, pListLink, Res.ListLink);
                    if (pIrq->DrvResIrq.Config.mSourceIrq == apArgsIn->Desc.IrqConfig.mSourceIrq)
                    {
                        //
                        // already know about this one
                        //
                        stat = K2STAT_NO_ERROR;
                        break;
                    }
                    pListLink = pListLink->mpNext;
                } while (NULL != pListLink);
            }
            if (NULL == pListLink)
            {
                //
                // range not known (yet) so try to add it
                //
                pIrq = (DEV_IRQ *)K2OS_Heap_Alloc(sizeof(DEV_IRQ));
                if (NULL == pIrq)
                {
                    stat = K2STAT_ERROR_NOT_FOUND;
                    break;
                }
                K2MEM_Zero(pIrq, sizeof(DEV_IRQ));
                K2MEM_Copy(&pIrq->DrvResIrq.Config, &apArgsIn->Desc.IrqConfig, sizeof(K2OS_IRQ_CONFIG));
                pu = (UINT32 *)&pIrq->DrvResIrq.Config;
                pIrq->Res.mPlatContext = K2OS_Root_AddPlatResource(
                    pChild->InSec.mPlatContext,
                    K2OS_PLATRES_IRQ,
                    pu[0], pu[1]
                );
                if (0 == pIrq->Res.mPlatContext)
                {
                    stat = K2OS_Thread_GetLastStatus();
                    K2OS_Heap_Free(pIrq);
                    K2_ASSERT(K2STAT_IS_ERROR(stat));
                    break;
                }
                K2LIST_AddAtTail(&pChild->InSec.IrqResList, &pIrq->Res.ListLink);
                stat = K2STAT_NO_ERROR;
            }
            break;

        default:
            stat = K2STAT_ERROR_BAD_ARGUMENT;
            break;
        }
    }

    K2OS_CritSec_Leave(&pChild->Sec);

    return stat;
}

K2STAT      
Dev_NodeLocked_NewDevUser(
    DEVNODE *                           apNode,
    K2OSDRV_NEW_DEV_USER_IN const *     apArgsIn,
    K2OSDRV_NEW_DEV_USER_OUT *          apArgsOut
)
{
    return K2STAT_ERROR_NOT_IMPL;
}

K2STAT      
Dev_NodeLocked_AcceptDevUser(
    DEVNODE *                           apNode,
    K2OSDRV_ACCEPT_DEV_USER_IN const *  apArgsIn
)
{
    return K2STAT_ERROR_NOT_IMPL;
}
