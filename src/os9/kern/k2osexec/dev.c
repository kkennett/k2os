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

static DEVNODE          sgDevTreeRootDevNode;
static K2OSKERN_SEQLOCK sgRefSeqLock;

DEVNODE * const gpDevTree = &sgDevTreeRootDevNode;

void
Dev_Init(
    void
)
{
    BOOL ok;

    K2OSKERN_SeqInit(&sgRefSeqLock);

    K2MEM_Zero(gpDevTree, sizeof(DEVNODE));

    //
    // ensures root node always has a reference
    //
    Dev_CreateRef(&gpDevTree->RefToSelf_OwnedByParent, gpDevTree);

    gpDevTree->mBusType = K2OS_BUSTYPE_CPU;
    gpDevTree->mParentRelativeBusAddress = 0;

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
    BOOL disp;

    K2_ASSERT(NULL == apRef->mpDevNode);
    K2_ASSERT(NULL != apNode);

    apRef->mpDevNode = apNode;

    disp = K2OSKERN_SeqLock(&sgRefSeqLock);

    K2LIST_AddAtTail(&apNode->RefList, &apRef->ListLink);

    //    Debug_Printf("Node(%08X)++ == %d (%08X)\n", apNode, apNode->RefList.mNodeCount, apNode->RefToParent.mpDevNode);

    K2OSKERN_SeqUnlock(&sgRefSeqLock, disp);
}

void
Dev_ReleaseRef(
    DEVNODE_REF *   apRef
)
{
    DEVNODE *   pNode;
    BOOL        disp;

    K2_ASSERT(NULL != apRef);

    pNode = apRef->mpDevNode;

    K2_ASSERT(NULL != pNode);

    K2_ASSERT((pNode == gpDevTree) || (NULL != pNode->RefToParent.mpDevNode));

    disp = K2OSKERN_SeqLock(&sgRefSeqLock);

    K2LIST_Remove(&pNode->RefList, &apRef->ListLink);

    //    Debug_Printf("Node(%08X)-- == %d (%08X)\n", pNode, pNode->RefList.mNodeCount, pNode->RefToParent.mpDevNode);

    if (0 == pNode->RefList.mNodeCount)
    {
        K2_ASSERT(NULL == pNode->RefToSelf_OwnedByParent.mpDevNode);
        DevMgr_QueueEvent(pNode->RefToParent.mpDevNode, DevNodeEvent_ChildRefsHitZero, (UINT32)pNode);
    }

    apRef->mpDevNode = NULL;

    K2OSKERN_SeqUnlock(&sgRefSeqLock, disp);
}

void
Dev_NodeLocked_OnEvent_TimerExpired(
    DEVNODE *   apNode
)
{

    if (apNode->InSec.mState == DevNodeState_GoingOnline_PreInstantiate)
    {
        K2OSKERN_Debug("Driver \"%s\" is taking a long time to instantiate\n", apNode->InSec.mpDriverCandidate);
        DevMgr_NodeLocked_AddTimer(apNode, DRIVER_INSTANTIATE_TIMEOUT_MS);
        return;
    }

    if (apNode->InSec.mState == DevNodeState_GoingOnline_WaitStarted)
    {
        K2OSKERN_Debug("Driver \"%s\" (instance %08X) is taking a long time to start\n", apNode->InSec.mpDriverCandidate, apNode->Driver.mpContext);
        DevMgr_NodeLocked_AddTimer(apNode, DRIVER_START_TIMEOUT_MS);
        return;
    }

    K2OSKERN_Debug("%d: Timer expired\n", K2OS_System_GetMsTick32());
    K2OSKERN_Debug("Driver \"%s\" (instance %08X) timer expired\n", apNode->InSec.mpDriverCandidate, apNode->Driver.mpContext);

    K2_ASSERT(0);
}

void
Dev_Start_Work(
    void *apArg
)
{
    DEVNODE *           pDevNode;
    K2_EXCEPTION_TRAP   exTrap;
    K2STAT              stat;

    pDevNode = (DEVNODE *)apArg;

    K2_ASSERT(pDevNode->InSec.mState == DevNodeState_GoingOnline_PreInstantiate);

    stat = K2_EXTRAP(&exTrap, pDevNode->Driver.mfCreateInstance((K2OS_DEVCTX)pDevNode, &pDevNode->Driver.mpContext));

    K2OS_CritSec_Enter(&pDevNode->Sec);

    DevMgr_NodeLocked_DelTimer(pDevNode);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("****Driver::CreateInstance failed with error code %08X\n", stat);
        pDevNode->InSec.Driver.mLastStatus = stat;
        pDevNode->InSec.mState = DevNodeState_Offline_ErrorStopped;
        DevMgr_QueueEvent(pDevNode->RefToParent.mpDevNode, DevNodeEvent_ChildStopped, (UINT32)pDevNode);
    }
    else
    {
//        K2OSKERN_Debug("Driver(\"%s\") Created instance %08X\n", pDevNode->InSec.mpDriverCandidate, pDevNode->Driver.mpContext);
        pDevNode->InSec.mState = DevNodeState_GoingOnline_WaitStarted;
        DevMgr_NodeLocked_AddTimer(pDevNode, DRIVER_START_TIMEOUT_MS);
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);

    if (K2STAT_IS_ERROR(stat))
        return;

    //
    // give driver a chance to do something outside Sec before we start it
    //
    K2OS_Thread_Sleep(100);

    K2OSKERN_Debug("Staring Driver(\"%s\") instance %08X\n", pDevNode->InSec.mpDriverCandidate, pDevNode->Driver.mpContext);
    stat = K2_EXTRAP(&exTrap, pDevNode->Driver.mfStartDriver(pDevNode->Driver.mpContext));

    K2OS_CritSec_Enter(&pDevNode->Sec);

    DevMgr_NodeLocked_DelTimer(pDevNode);

    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("****Driver::Start failed with error code %08X\n", stat);
        pDevNode->InSec.Driver.mLastStatus = stat;
        pDevNode->InSec.mState = DevNodeState_Offline_ErrorStopped;
        DevMgr_QueueEvent(pDevNode->RefToParent.mpDevNode, DevNodeEvent_ChildStopped, (UINT32)pDevNode);
    }
    else
    {
//        K2OSKERN_Debug("Driver(\"%s\") instance %08X start executed\n", pDevNode->InSec.mpDriverCandidate, pDevNode->Driver.mpContext);
    }

    K2OS_CritSec_Leave(&pDevNode->Sec);
}

void
Dev_NodeLocked_OnEvent_ChildDriverSpecChanged(
    DEVNODE *apNode,
    DEVNODE *apChild
)
{
    K2LIST_LINK *   pListLink;
    DEVNODE *       pCheck;
    K2STAT          stat;

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

    stat = K2STAT_ERROR_UNKNOWN;

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

//        K2OSKERN_Debug("+Acquire driver \"%s\"\n", apChild->InSec.mpDriverCandidate);
        apChild->InSec.Driver.mXdl = K2OS_Xdl_Acquire(apChild->InSec.mpDriverCandidate);
        if (NULL == apChild->InSec.Driver.mXdl)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
        }
        else
        {
            do {
                apChild->Driver.mfCreateInstance = (K2OS_pf_Driver_CreateInstance)K2OS_Xdl_FindExport(
                    apChild->InSec.Driver.mXdl, TRUE, "CreateInstance");
                if (NULL == apChild->Driver.mfCreateInstance)
                {
                    K2OSKERN_Debug("Driver \"%s\" did not export \"CreateInstance\"\n", apChild->InSec.mpDriverCandidate);
                    stat = K2STAT_ERROR_OBJECT_NOT_FOUND;
                    break;
                }

                do {
                    apChild->Driver.mfStartDriver = (K2OS_pf_Driver_Call)K2OS_Xdl_FindExport(
                        apChild->InSec.Driver.mXdl, TRUE, "StartDriver");
                    if (NULL == apChild->Driver.mfStartDriver)
                    {
                        K2OSKERN_Debug("Driver \"%s\" did not export \"StartDriver\"\n", apChild->InSec.mpDriverCandidate);
                        stat = K2STAT_ERROR_OBJECT_NOT_FOUND;
                        break;
                    }

                    do {
                        apChild->Driver.mfStopDriver = (K2OS_pf_Driver_Call)K2OS_Xdl_FindExport(
                            apChild->InSec.Driver.mXdl, TRUE, "StopDriver");
                        if (NULL == apChild->Driver.mfStopDriver)
                        {
                            K2OSKERN_Debug("Driver \"%s\" did not export \"StopDriver\"\n", apChild->InSec.mpDriverCandidate);
                            stat = K2STAT_ERROR_OBJECT_NOT_FOUND;
                            break;
                        }

                        apChild->Driver.mfDeleteInstance = (K2OS_pf_Driver_Call)K2OS_Xdl_FindExport(
                            apChild->InSec.Driver.mXdl, TRUE, "DeleteInstance");
                        if (NULL == apChild->Driver.mfDeleteInstance)
                        {
                            K2OSKERN_Debug("Driver \"%s\" did not export \"DeleteInstance\"\n", apChild->InSec.mpDriverCandidate);
                            stat = K2STAT_ERROR_OBJECT_NOT_FOUND;

                            apChild->Driver.mfStopDriver = NULL;
                        }
                        else
                        {
                            stat = K2STAT_NO_ERROR;
                        }

                    } while (0);

                    if (K2STAT_IS_ERROR(stat))
                    {
                        apChild->Driver.mfStartDriver = NULL;
                    }

                } while (0);

                if (K2STAT_IS_ERROR(stat))
                {
                    apChild->Driver.mfCreateInstance = NULL;
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_Xdl_Release(apChild->InSec.Driver.mXdl);
                apChild->InSec.Driver.mXdl = NULL;
            }
        }

        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("Failed to load driver \"%s\"; error code %08X\n", apChild->InSec.mpDriverCandidate, stat);
            apChild->InSec.Driver.mLastStatus = stat;
            apChild->InSec.mState = DevNodeState_Offline_ErrorStopped;
            DevMgr_QueueEvent(apChild->RefToParent.mpDevNode, DevNodeEvent_ChildStopped, (UINT32)apChild);
            break;
        }

        apChild->InSec.mState = DevNodeState_GoingOnline_PreInstantiate;

        stat = WorkerThread_Exec(Dev_Start_Work, apChild);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("Driver \"%s\" worker thread start failed with error code %08X\n", apChild->InSec.mpDriverCandidate, stat);
            K2OS_Xdl_Release(apChild->InSec.Driver.mXdl);
            apChild->InSec.Driver.mXdl = NULL;
            apChild->InSec.Driver.mLastStatus = stat;
            apChild->InSec.mState = DevNodeState_Offline_ErrorStopped;
            DevMgr_QueueEvent(apNode, DevNodeEvent_ChildStopped, (UINT32)apChild);
            break;
        }

        DevMgr_NodeLocked_AddTimer(apChild, DRIVER_INSTANTIATE_TIMEOUT_MS);

    } while (0);

    K2OS_CritSec_Leave(&apChild->Sec);
}

void
Dev_NodeLocked_OnEvent(
    DEVNODE *           apNode,
    DevNodeEventType    aType,
    UINT32              aArg
)
{
    switch (aType)
    {
    case DevNodeEvent_TimerExpired:
        Dev_NodeLocked_OnEvent_TimerExpired(apNode);
        break;

    case DevNodeEvent_ChildStopped:
        K2OSKERN_Debug("EXEC: Node(%08X): ChildStopped (%08X)\n", apNode, aArg);
        break;

    case DevNodeEvent_ChildRefsHitZero:
        K2OSKERN_Debug("EXEC: Node(%08X): ChildRefsHitZero (%08X)\n", apNode, aArg);
        break;

    case DevNodeEvent_NewChildMounted:
//        K2OSKERN_Debug("EXEC: %s %08X: NewChildMounted (%08X)\n", apNode->InSec.mpDriverCandidate, apNode, aArg);
        break;

    case DevNodeEvent_ChildDriverSpecChanged:
        Dev_NodeLocked_OnEvent_ChildDriverSpecChanged(apNode, (DEVNODE *)aArg);
        break;

    default:
        K2_ASSERT(0);
    };
}

DEVNODE *   
Dev_NodeLocked_CreateChildNode(
    DEVNODE *               apParent,
    UINT64 const *          apParentRelativeBusAddr,
    UINT32                  aChildBusType,
    K2_DEVICE_IDENT const * apChildIdent
)
{
    DEVNODE *   pChild;
    BOOL        ok;

    pChild = (DEVNODE *)K2OS_Heap_Alloc(sizeof(DEVNODE));
    if (NULL == pChild)
    {
        K2OSKERN_Debug("*** CreateChildNode - failed memory allocation\n");
        return NULL;
    }

    K2MEM_Zero(pChild, sizeof(DEVNODE));

    ok = K2OS_CritSec_Init(&pChild->Sec);
    if (!ok)
    {
        K2OSKERN_Debug("*** CreateChildNode - failed critical section init\n");
        K2OS_Heap_Free(pChild);
        return NULL;
    }

    pChild->mParentRelativeBusAddress = *apParentRelativeBusAddr;
    pChild->mBusType = aChildBusType;

    K2LIST_Init(&pChild->RefList);
    Dev_CreateRef(&pChild->RefToParent, apParent);
    Dev_CreateRef(&pChild->RefToSelf_OwnedByParent, pChild);

    K2MEM_Copy(&pChild->DeviceIdent, apChildIdent, sizeof(K2_DEVICE_IDENT));

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
    DEVNODE *   apParent,
    DEVNODE *   apChild
)
{
    K2_ASSERT(NULL != apParent);
    K2_ASSERT(NULL != apChild);
    K2_ASSERT(apChild->RefToParent.mpDevNode == apParent);

    K2LIST_Remove(&apParent->InSec.ChildList, &apChild->InParentSec.ParentChildListLink);

    K2OS_CritSec_Enter(&apChild->Sec);

    K2_ASSERT(DEVNODESTATE_OFFLINE_FIRST <= apChild->InSec.mState);

    K2_ASSERT(0 == apChild->InSec.ChildList.mNodeCount);
    K2_ASSERT(0 == apChild->InSec.IrqResList.mNodeCount);
    K2_ASSERT(0 == apChild->InSec.IoResList.mNodeCount);
    K2_ASSERT(0 == apChild->InSec.PhysResList.mNodeCount);

    K2OS_CritSec_Leave(&apChild->Sec);

    K2OS_CritSec_Done(&apChild->Sec);

    Dev_ReleaseRef(&apChild->RefToParent);

    K2OS_Heap_Free(apChild);
}

K2STAT      
Dev_NodeLocked_Enable(
    DEVNODE *   apNode
)
{
    K2LIST_LINK *   pListLink;
    DEVNODE *       pChild;

    if (apNode->InSec.mState != DevNodeState_Online)
    {
        K2OSKERN_Debug("*** Enable for node that is not online\n");
        return K2STAT_ERROR_NOT_RUNNING;
    }

    if (apNode->InSec.Driver.mEnabled)
    {
        return K2STAT_NO_ERROR;
    }

//    K2OSKERN_Debug("EXEC: Node(%08X): Enabling\n", apNode);

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
                    K2OSKERN_Debug("*** Memory allocation failure creating driver candidate\n");
                    pChild->InSec.mState = DevNodeState_Offline_NoDriver;
                }
                else
                {
                    K2MEM_Copy(pChild->InSec.mpDriverCandidate, pChild->InSec.mpPlatInfo, pChild->InSec.mPlatInfoBytes);
                    pChild->InSec.mpDriverCandidate[pChild->InSec.mPlatInfoBytes] = 0;
                    pChild->InSec.mState = DevNodeState_Offline_CheckForDriver;
                    DevMgr_QueueEvent(apNode, DevNodeEvent_ChildDriverSpecChanged, (UINT32)pChild);
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

void
Dev_Describe(
    DEVNODE *       apNode,
    UINT64 const *  apBusAddress,
    UINT32 *        apRetNodeName,
    char **         appOut,
    UINT32 *        apLeft
)
{
    UINT32    ate;
    UINT32    left;
    char *    pEnd;

    left = *apLeft;
    pEnd = *appOut;

    if (0 != (*apBusAddress))
    {
        ate = K2ASC_PrintfLen(pEnd, left, "BUSADDR(%08X,%08X);", (UINT32)((*apBusAddress) >> 32), (UINT32)((*apBusAddress) & 0xFFFFFFFFull));
        left -= ate;
        pEnd += ate;
    }

    if (NULL != apNode->InSec.mpAcpiNode)
    {
        K2MEM_Copy(apRetNodeName, apNode->InSec.mpAcpiNode->mName, sizeof(UINT32));
        ACPI_Describe(apNode->InSec.mpAcpiNode, &pEnd, &left);
    }
    else
    {
        switch (apNode->mBusType)
        {
        case K2OS_BUSTYPE_CPU:
            K2MEM_Copy(apRetNodeName, "CPU_", sizeof(UINT32));
            break;
        case K2OS_BUSTYPE_PCI:
            K2MEM_Copy(apRetNodeName, "PCI_", sizeof(UINT32));
            break;
        case K2OS_BUSTYPE_ISA:
            K2MEM_Copy(apRetNodeName, "ISA_", sizeof(UINT32));
            break;
        case K2OS_BUSTYPE_EISA:
            K2MEM_Copy(apRetNodeName, "EISA", sizeof(UINT32));
            break;
        case K2OS_BUSTYPE_PCMCIA:
            K2MEM_Copy(apRetNodeName, "PCMC", sizeof(UINT32));
            break;
        case K2OS_BUSTYPE_CARDBUS:
            K2MEM_Copy(apRetNodeName, "CARD", sizeof(UINT32));
            break;
        case K2OS_BUSTYPE_USB:
            K2MEM_Copy(apRetNodeName, "USB_", sizeof(UINT32));
            break;
        case K2OS_BUSTYPE_SERIAL:
            K2MEM_Copy(apRetNodeName, "SER_", sizeof(UINT32));
            break;
        case K2OS_BUSTYPE_SPI:
            K2MEM_Copy(apRetNodeName, "SPI_", sizeof(UINT32));
            break;
        case K2OS_BUSTYPE_I2C:
            K2MEM_Copy(apRetNodeName, "I2C_", sizeof(UINT32));
            break;
        case K2OS_BUSTYPE_CAN:
            K2MEM_Copy(apRetNodeName, "CAN_", sizeof(UINT32));
            break;
        case K2OS_BUSTYPE_HOST:
            K2MEM_Copy(apRetNodeName, "HOST", sizeof(UINT32));
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
    DEVNODE *               apParent,
    UINT32                  aMountFlagsAndChildBusType,
    UINT64 const *          apBusAddressOfChild,
    K2_DEVICE_IDENT const * apChildIdent,
    UINT32 *                apRetChildInstanceId
)
{
    char *      pIoBuf;
    DEVNODE *   pChildNode;
    char *      pOut;
    UINT32      ioBytes;
    UINT32      nodeName;
    K2STAT      stat;
    UINT32      left;

    if (DevNodeState_Online != apParent->InSec.mState)
    {
        K2OSKERN_Debug("*** MountChild for node that is not online\n");
        return K2STAT_ERROR_NOT_RUNNING;
    }

    if (apParent->InSec.Driver.mEnabled)
    {
        return K2STAT_NO_ERROR;
    }

    pIoBuf = (char *)K2OS_Heap_Alloc(K2OSPLAT_MOUNT_MAX_BYTES);
    if (NULL == pIoBuf)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    //
    // if bustype is different from parent then this is a bus bridge device
    //
    pChildNode = Dev_NodeLocked_CreateChildNode(apParent, apBusAddressOfChild, aMountFlagsAndChildBusType, apChildIdent);
    if (NULL == pChildNode)
    {
        stat = K2STAT_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        do {
            *apRetChildInstanceId = pChildNode->mParentsChildInstanceId = ++apParent->InSec.mLastChildInstanceId;
            stat = K2STAT_NO_ERROR;

            pOut = pIoBuf;
            *pOut = 0;
            left = K2OSPLAT_MOUNT_MAX_BYTES;
            if (NULL != apParent->InSec.mpAcpiNode)
            {
                if (ACPI_MatchAndAttachChild(apParent->InSec.mpAcpiNode, apBusAddressOfChild, pChildNode))
                {
                    K2_ASSERT(NULL != pChildNode->InSec.mpAcpiNode);
                }
            }

            Dev_Describe(pChildNode, apBusAddressOfChild, &nodeName, &pOut, &left);

            ioBytes = ((UINT32)(pOut - pIoBuf)) + 1;

            pChildNode->InSec.mpMountedInfo = (char *)K2OS_Heap_Alloc((ioBytes + 4) & ~3);
            if (NULL == pChildNode->InSec.mpMountedInfo)
            {
                K2OSKERN_Debug("*** Failed to allocate memory for mount info\n");
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
                break;
            }

            do {
                K2MEM_Copy(pChildNode->InSec.mpMountedInfo, pIoBuf, ioBytes);
                pChildNode->InSec.mMountedInfoBytes = ioBytes + 1;
                pChildNode->InSec.mpMountedInfo[ioBytes] = 0;

                pChildNode->InSec.mPlatDev = gPlat.DeviceCreate(
                    apParent->InSec.mPlatDev,
                    nodeName,
                    (UINT32)pChildNode,
                    &pChildNode->DeviceIdent,
                    (UINT8 *)pIoBuf,
                    K2OSPLAT_MOUNT_MAX_BYTES,
                    &ioBytes
                );

                if (NULL == pChildNode->InSec.mPlatDev)
                {
                    K2OSKERN_Debug("*** Mount of plat dev failed\n");
                    stat = K2STAT_ERROR_UNKNOWN;
                    break;
                }

                if (ioBytes > 0)
                {
                    K2_ASSERT(ioBytes <= K2OSPLAT_MOUNT_MAX_BYTES);

                    pChildNode->InSec.mpPlatInfo = (UINT8 *)K2OS_Heap_Alloc((ioBytes +4)&~3);
                    if (NULL == pChildNode->InSec.mpPlatInfo)
                    {
                        stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    }
                    else
                    {
                        K2MEM_Copy(pChildNode->InSec.mpPlatInfo, pIoBuf, ioBytes);
                        pChildNode->InSec.mpPlatInfo[ioBytes] = 0;
                        pChildNode->InSec.mPlatInfoBytes = ioBytes;
                        stat = K2STAT_NO_ERROR;
                    }
                }
                else
                {
                    stat = K2STAT_NO_ERROR;
                }

                if (K2STAT_IS_ERROR(stat))
                {
                    gPlat.DeviceRemove(pChildNode->InSec.mPlatDev);
                    pChildNode->InSec.mPlatDev = NULL;
                }

            } while (0);

            if (K2STAT_IS_ERROR(stat))
            {
                K2OS_Heap_Free(pChildNode->InSec.mpMountedInfo);
                pChildNode->InSec.mpMountedInfo = NULL;
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            Dev_ReleaseRef(&pChildNode->RefToSelf_OwnedByParent);
        }
    }

    K2OS_Heap_Free(pIoBuf);

    if (!K2STAT_IS_ERROR(stat))
    {
        if (NULL != pChildNode->InSec.mpAcpiNode)
        {
            ACPI_NodeLocked_PopulateRes(pChildNode);
        }

        DevMgr_QueueEvent(apParent, DevNodeEvent_NewChildMounted, (UINT32)pChildNode);
    }

    return stat;
}

K2STAT
Dev_NodeLocked_AddRes(
    DEVNODE *               apNode,
    K2OSDDK_RESDEF const *  apResDef,
    void *                  aAcpiContext
)
{
    DEV_RES *       pDevRes;
    UINT32 *        pu;
    DEV_RES *       pOtherDevRes;
    K2LIST_LINK *   pListLink;

    pDevRes = (DEV_RES *)K2OS_Heap_Alloc(sizeof(DEV_RES));
    if (NULL == pDevRes)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    K2MEM_Zero(pDevRes, sizeof(DEV_RES));

    K2MEM_Copy(&pDevRes->DdkRes.Def, apResDef, sizeof(K2OSDDK_RESDEF));

    pDevRes->mAcpiContext = (UINT32)aAcpiContext;

    switch (apResDef->mType)
    {
    case K2OS_RESTYPE_IO:
        pListLink = apNode->InSec.IoResList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pOtherDevRes = K2_GET_CONTAINER(DEV_RES, pListLink, ListLink);
                if ((pOtherDevRes->DdkRes.Def.Io.Range.mBasePort == pDevRes->DdkRes.Def.Io.Range.mBasePort) &&
                    (pOtherDevRes->DdkRes.Def.Io.Range.mSizeBytes == pDevRes->DdkRes.Def.Io.Range.mSizeBytes))
                {
                    //
                    // already exists (total match)
                    //
                    K2OS_Heap_Free(pDevRes);
                    return K2STAT_NO_ERROR;
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }
        pDevRes->mPlatRes = gPlat.DeviceAddRes(apNode->InSec.mPlatDev, K2OS_RESTYPE_IO, apResDef->Io.Range.mBasePort, apResDef->Io.Range.mSizeBytes);
        break;

    case K2OS_RESTYPE_PHYS:
        pListLink = apNode->InSec.PhysResList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pOtherDevRes = K2_GET_CONTAINER(DEV_RES, pListLink, ListLink);
                if ((pOtherDevRes->DdkRes.Def.Phys.Range.mBaseAddr == pDevRes->DdkRes.Def.Phys.Range.mBaseAddr) &&
                    (pOtherDevRes->DdkRes.Def.Phys.Range.mSizeBytes == pDevRes->DdkRes.Def.Phys.Range.mSizeBytes))
                {
                    //
                    // already exists (total match)
                    //
                    K2OS_Heap_Free(pDevRes);
                    return K2STAT_NO_ERROR;
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }
        pDevRes->DdkRes.Phys.mTokPageArray = gKernDdk.PageArray_CreateAt(
            pDevRes->DdkRes.Def.Phys.Range.mBaseAddr,
            pDevRes->DdkRes.Def.Phys.Range.mSizeBytes / K2_VA_MEMPAGE_BYTES
        );
        if (NULL == pDevRes->DdkRes.Phys.mTokPageArray)
        {
            K2OS_Heap_Free(pDevRes);
            return K2OS_Thread_GetLastStatus();
        }
        pDevRes->mPlatRes = gPlat.DeviceAddRes(apNode->InSec.mPlatDev, K2OS_RESTYPE_PHYS, apResDef->Phys.Range.mBaseAddr, apResDef->Phys.Range.mSizeBytes);
        if (NULL == pDevRes->mPlatRes)
        {
            K2OS_Token_Destroy(pDevRes->DdkRes.Phys.mTokPageArray);
            pDevRes->DdkRes.Phys.mTokPageArray = NULL;
        }
        break;

    case K2OS_RESTYPE_IRQ:
        pListLink = apNode->InSec.IrqResList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pOtherDevRes = K2_GET_CONTAINER(DEV_RES, pListLink, ListLink);
                if (pOtherDevRes->DdkRes.Def.Irq.Config.mSourceIrq == pDevRes->DdkRes.Def.Irq.Config.mSourceIrq)
                {
                    //
                    // already exists. check line properties for compatibility and warn
                    //
                    if ((0 != K2MEM_Compare(&pOtherDevRes->DdkRes.Def.Irq.Config.Line, &pDevRes->DdkRes.Def.Irq.Config.Line, sizeof(K2OS_IRQ_LINE_CONFIG))) ||
                        (pOtherDevRes->DdkRes.Def.Irq.Config.mDestCoreIx != pDevRes->DdkRes.Def.Irq.Config.mDestCoreIx))
                    {
                        K2OSKERN_Debug("\n*** IRQ %d config mistmatch\n    Was added with config %d,%d,%d,%d on core %d\n    But then attempted with %d,%d,%d,%d\n",
                            pOtherDevRes->DdkRes.Def.Irq.Config.Line.mIsActiveLow,
                            pOtherDevRes->DdkRes.Def.Irq.Config.Line.mIsEdgeTriggered,
                            pOtherDevRes->DdkRes.Def.Irq.Config.Line.mShareConfig,
                            pOtherDevRes->DdkRes.Def.Irq.Config.Line.mWakeConfig,
                            pDevRes->DdkRes.Def.Irq.Config.Line.mIsActiveLow,
                            pDevRes->DdkRes.Def.Irq.Config.Line.mIsEdgeTriggered,
                            pDevRes->DdkRes.Def.Irq.Config.Line.mShareConfig,
                            pDevRes->DdkRes.Def.Irq.Config.Line.mWakeConfig);
                    }
                    K2OS_Heap_Free(pDevRes);
                    return K2STAT_NO_ERROR;
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
        }
        if (!K2OSKERN_IrqDefine(&pDevRes->DdkRes.Def.Irq.Config))
        {
            return K2STAT_ERROR_REJECTED;
        }
        pDevRes->DdkRes.Irq.mTokInterrupt = K2OSKERN_IrqHook(
            pDevRes->DdkRes.Def.Irq.Config.mSourceIrq, 
            &apNode->Driver.mfIntrHookKey
        );
        if (NULL == pDevRes->DdkRes.Irq.mTokInterrupt)
        {
            K2OS_Heap_Free(pDevRes);
            return K2OS_Thread_GetLastStatus();
        }
        pu = (UINT32 *)&pDevRes->DdkRes.Def.Irq.Config;
        pDevRes->mPlatRes = gPlat.DeviceAddRes(apNode->InSec.mPlatDev, K2OS_RESTYPE_IRQ, pu[0], pu[1]);
        if (NULL == pDevRes->mPlatRes)
        {
            K2OS_Token_Destroy(pDevRes->DdkRes.Irq.mTokInterrupt);
            pDevRes->DdkRes.Irq.mTokInterrupt = NULL;
        }
        break;
    default:
        break;
    }

    if (NULL == pDevRes->mPlatRes)
    {
        K2OSKERN_Debug("*** Could not add plat res type %d\n", apResDef->mType);
        return K2STAT_ERROR_UNKNOWN;
    }

    switch (apResDef->mType)
    {
    case K2OS_RESTYPE_IO:
        K2LIST_AddAtTail(&apNode->InSec.IoResList, &pDevRes->ListLink);
        break;
    case K2OS_RESTYPE_PHYS:
        K2LIST_AddAtTail(&apNode->InSec.PhysResList, &pDevRes->ListLink);
        K2_ASSERT(NULL != pDevRes->DdkRes.Phys.mTokPageArray);
        break;
    case K2OS_RESTYPE_IRQ:
        K2LIST_AddAtTail(&apNode->InSec.IrqResList, &pDevRes->ListLink);
        K2_ASSERT(NULL != pDevRes->DdkRes.Irq.mTokInterrupt);
        break;
    default:
        K2_ASSERT(0);
        break;
    }

    return K2STAT_NO_ERROR;
}
