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

typedef struct _ROOTACPI_NODE ROOTACPI_NODE;
struct _ROOTACPI_NODE
{
    ACPI_HANDLE         mhObject;

    ROOTACPI_NODE *     mpParentAcpiNode;
    K2LIST_ANCHOR       ChildAcpiNodeList;
    K2LIST_LINK         ParentChildListLink;

    ROOTACPI_NODEINFO   Info;
    ROOTDEV_NODEREF     RefDevNode;

    ACPI_DEVICE_INFO *  mpDeviceInfo;

    ACPI_BUFFER         CurrentAcpiRes;
};

ROOTACPI_NODE   gAcpiTreeRoot;

void RootNodeAcpiDriver_InitNode(ROOTACPI_NODE *apAcpiNode, ACPI_HANDLE ahObject);

void
RootAcpiDriver_Describe(
    ROOTACPI_NODE * apAcpiNode,
    char **         appOut,
    UINT_PTR *      apLeft
)
{
    ACPI_DEVICE_INFO *  pInfo;
    char *              pEnd;
    UINT_PTR            left;
    UINT_PTR            ate;
    UINT_PTR            ix;

    pInfo = apAcpiNode->mpDeviceInfo;
    K2_ASSERT(NULL != pInfo);

    pEnd = *appOut;
    *pEnd = 0;
    left = *apLeft;
    if (pInfo->UniqueId.Length > 0)
    {
        ate = K2ASC_PrintfLen(pEnd, left, "U(%s);", pInfo->UniqueId.String);
        left -= ate;
        pEnd += ate;
    }
    if (pInfo->HardwareId.Length > 0)
    {
        ate = K2ASC_PrintfLen(pEnd, left, "H(%s);", pInfo->HardwareId.String);
        left -= ate;
        pEnd += ate;
    }
    if (pInfo->ClassCode.Length > 0)
    {
        ate = K2ASC_PrintfLen(pEnd, left, "CC(%s);", pInfo->ClassCode.String);
        left -= ate;
        pEnd += ate;
    }
    if (pInfo->Flags != 0)
    {
        ate = K2ASC_PrintfLen(pEnd, left, "FLAGS(%02X);", pInfo->Flags);
        left -= ate;
        pEnd += ate;
    }
    if (pInfo->CompatibleIdList.Count > 0)
    {
        ate = K2ASC_PrintfLen(pEnd, left, "CO(");
        left -= ate;
        pEnd += ate;
        for (ix = 0; ix < pInfo->CompatibleIdList.Count; ix++)
        {
            if (pInfo->CompatibleIdList.Ids[ix].Length > 0)
            {
                ate = K2ASC_PrintfLen(pEnd, left, "%s ", pInfo->CompatibleIdList.Ids[ix].String);
                left -= ate;
                pEnd += ate;
            }
        }
        ate = K2ASC_PrintfLen(pEnd, left, ")");
        left -= ate;
        pEnd += ate;
    }
    *appOut = pEnd;
    *apLeft = left;
}

void
RootAcpiDriver_DiscoveredIrq(
    ROOTDEV_NODE *  apNode,
    ACPI_RESOURCE * apRes
)
{
    ROOTDEV_IRQ *   pIrq;
    UINT32          ix;

    for (ix = 0; ix < apRes->Data.Irq.InterruptCount; ix++)
    {
        RootDebug_Printf("  [IRQ %d]\n", apRes->Data.Irq.Interrupts[ix]);

        pIrq = (ROOTDEV_IRQ *)K2OS_Heap_Alloc(sizeof(ROOTDEV_IRQ));
        if (NULL == pIrq)
            return;
        pIrq->DrvResIrq.Config.mSourceIrq = apRes->Data.Irq.Interrupts[ix];
        pIrq->DrvResIrq.Config.mDestCoreIx = 0;
        pIrq->DrvResIrq.Config.Line.mIsEdgeTriggered = apRes->Data.Irq.Triggering ? TRUE : FALSE;
        pIrq->DrvResIrq.Config.Line.mIsActiveLow = apRes->Data.Irq.Polarity ? TRUE : FALSE;
        pIrq->DrvResIrq.Config.Line.mShareConfig = apRes->Data.Irq.Shareable ? TRUE : FALSE;
        pIrq->DrvResIrq.Config.Line.mWakeConfig = apRes->Data.Irq.WakeCapable ? TRUE : FALSE;

        pIrq->Res.mAcpiContext = (UINT_PTR)apRes;
        K2LIST_AddAtTail(&apNode->InSec.IrqResList, &pIrq->Res.ListLink);
    }
}

void
RootAcpiDriver_DiscoveredDMA(
    ROOTDEV_NODE *  apNode,
    ACPI_RESOURCE * apRes
)
{
#if 0
    if (apRes->Type == ACPI_RESOURCE_TYPE_DMA)
        RootDebug_Printf("  DMA(%d chan)\n", apRes->Data.Dma.ChannelCount);
    else
        RootDebug_Printf("  FIXED_DMA(%d chan)\n", apRes->Data.FixedDma.Channels);
#endif
}

void
RootAcpiDriver_DiscoveredIo(
    ROOTDEV_NODE *  apNode,
    ACPI_RESOURCE * apRes
)
{
    ROOTDEV_IO *    pIo;
    UINT16          basePort;
    UINT16          sizeBytes;

    if (apRes->Type == ACPI_RESOURCE_TYPE_IO)
    {
        basePort = apRes->Data.Io.Minimum;
        sizeBytes = apRes->Data.Io.AddressLength;
    }
    else
    {
        basePort = apRes->Data.FixedIo.Address;
        sizeBytes = apRes->Data.FixedIo.AddressLength;
    }
    RootDebug_Printf("  [IO %04X %04X]\n", basePort, sizeBytes);

    pIo = (ROOTDEV_IO *)K2OS_Heap_Alloc(sizeof(ROOTDEV_IO));
    if (NULL == pIo)
        return;
    pIo->DrvResIo.mBasePort = basePort;
    pIo->DrvResIo.mSizeBytes = sizeBytes;
    pIo->Res.mAcpiContext = (UINT_PTR)apRes;
    K2LIST_AddAtTail(&apNode->InSec.IoResList, &pIo->Res.ListLink);
}

void
RootAcpiDriver_DiscoveredMemory(
    ROOTDEV_NODE *  apNode,
    ACPI_RESOURCE * apRes
)
{
    ROOTDEV_PHYS *  pPhys;
    UINT32          baseAddr;
    UINT32          sizeBytes;

    if (apRes->Type == ACPI_RESOURCE_TYPE_MEMORY24)
    {
        baseAddr = apRes->Data.Memory24.Minimum;
        sizeBytes = apRes->Data.Memory24.AddressLength;
    }
    else if (apRes->Type == ACPI_RESOURCE_TYPE_MEMORY32)
    {
        baseAddr = apRes->Data.Memory32.Minimum;
        sizeBytes = apRes->Data.Memory32.AddressLength;
    }
    else
    {
        baseAddr = apRes->Data.FixedMemory32.Address;
        sizeBytes = apRes->Data.FixedMemory32.AddressLength;
    }
    RootDebug_Printf("  [PHYS %08X %08X]\n", baseAddr, sizeBytes);

    pPhys = (ROOTDEV_PHYS *)K2OS_Heap_Alloc(sizeof(ROOTDEV_PHYS));
    if (NULL == pPhys)
        return;
    pPhys->DrvResPhys.Range.mBaseAddr = baseAddr;
    pPhys->DrvResPhys.Range.mSizeBytes = sizeBytes;
    pPhys->Res.mAcpiContext = (UINT_PTR)apRes;
    K2LIST_AddAtTail(&apNode->InSec.PhysResList, &pPhys->Res.ListLink);
}

void
RootAcpiDriver_DiscoveredAddress(
    ROOTDEV_NODE *  apNode,
    ACPI_RESOURCE * apRes
)
{
    //
    // don't really care about addresses.  UEFI might but at OS level we don't
    //
#if 0
    ACPI_STATUS             status;
    ACPI_RESOURCE_ADDRESS64 addr64;
    K2MEM_Zero(&addr64, sizeof(addr64));
    status = AcpiResourceToAddress64(apRes, &addr64);
    if (ACPI_FAILURE(status))
        return;
    RootDebug_Printf("ADDRESS(%d, %d)\n", apRes->Type, addr64.ResourceType);
    RootDebug_Printf("  ProducerConsumer %02X\n", addr64.ProducerConsumer);
    RootDebug_Printf("  Decode           %02X\n", addr64.Decode);
    RootDebug_Printf("  MinAddressFixed  %02X\n", addr64.MinAddressFixed);
    RootDebug_Printf("  MaxAddressFixed  %02X\n", addr64.MaxAddressFixed);
    RootDebug_Printf("  Address.Granularity         %08X%08X\n", (UINT32)((addr64.Address.Granularity >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.Granularity & 0xFFFFFFFFull));
    RootDebug_Printf("  Address.Minimum             %08X%08X\n", (UINT32)((addr64.Address.Minimum >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.Minimum & 0xFFFFFFFFull));
    RootDebug_Printf("  Address.Maximum             %08X%08X\n", (UINT32)((addr64.Address.Maximum >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.Maximum & 0xFFFFFFFFull));
    RootDebug_Printf("  Address.TranslationOffset   %08X%08X\n", (UINT32)((addr64.Address.TranslationOffset >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.TranslationOffset & 0xFFFFFFFFull));
    RootDebug_Printf("  Address.AddressLength       %08X%08X\n", (UINT32)((addr64.Address.AddressLength >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.AddressLength & 0xFFFFFFFFull));
#endif
}

ACPI_STATUS
RootAcpiDriver_ResEnumCallback(
    ACPI_RESOURCE * Resource,
    void *          Context
)
{
    ROOTDEV_NODE *  pDevNode;

    pDevNode = (ROOTDEV_NODE *)Context;

    switch (Resource->Type)
    {
    case ACPI_RESOURCE_TYPE_IRQ:
        RootAcpiDriver_DiscoveredIrq(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_DMA:
    case ACPI_RESOURCE_TYPE_FIXED_DMA:
        RootAcpiDriver_DiscoveredDMA(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_IO:
    case ACPI_RESOURCE_TYPE_FIXED_IO:
        RootAcpiDriver_DiscoveredIo(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_MEMORY24:
    case ACPI_RESOURCE_TYPE_MEMORY32:
    case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
        RootAcpiDriver_DiscoveredMemory(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_ADDRESS16:
    case ACPI_RESOURCE_TYPE_ADDRESS32:
    case ACPI_RESOURCE_TYPE_ADDRESS64:
    case ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64:
        RootAcpiDriver_DiscoveredAddress(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_END_TAG:
        // ignore
        break;

    default:
        RootDebug_Printf("!!!!RootAcpiDriver_ResEnumCallback - %d\n", Resource->Type);
        //
        // dont know what to do with this
        //
        K2_ASSERT(0);
    }

    return AE_OK;
}

void
RootNodeAcpiDriver_OnEvent_NewChild(
    ROOTDEV_DRIVER_NEWCHILD_EVENT * apEvent
)
{
    ROOTACPI_NODE * pAcpiNode;
    char *          pIoBuf;
    char *          pOut;
    UINT_PTR        left;
    UINT_PTR        ioBytes;
    ACPI_STATUS     acpiStatus;
    K2LIST_LINK *   pListLink;
    ROOTDEV_IO *    pIoRes;
    ROOTDEV_EVENT * pEvent;
    ROOTDEV_NODEREF refNewNode;
    ROOTDEV_NODE *  pNewDevNode;

    refNewNode.mpRefNode = NULL;
    RootDevNode_CreateRef(&refNewNode, apEvent->RefNewDevNode.mpRefNode);
    RootDevNode_ReleaseRef(&apEvent->RefNewDevNode);

    K2_ASSERT(NULL != refNewNode.mpRefNode->InSec.mpAcpiNodeInfo);
    pAcpiNode = K2_GET_CONTAINER(ROOTACPI_NODE, refNewNode.mpRefNode->InSec.mpAcpiNodeInfo, Info);

    pIoBuf = (char *)K2OS_Heap_Alloc(ROOTPLAT_MOUNT_MAX_BYTES);
    if (NULL == pIoBuf)
    {
        RootDebug_Printf("*** Could not allocate Io buffer for device mount to kernel\n");
        RootDevNode_ReleaseRef(&refNewNode);
        return;
    }
    pOut = pIoBuf;
    *pOut = 0;
    left = ROOTPLAT_MOUNT_MAX_BYTES;

    RootAcpiDriver_Describe(pAcpiNode, &pOut, &left);

    ioBytes = ((UINT_PTR)(pOut - pIoBuf)) + 1;
    refNewNode.mpRefNode->InSec.mpMountedInfo = (char *)K2OS_Heap_Alloc((ioBytes + 4) & ~3);
    if (NULL == refNewNode.mpRefNode->InSec.mpMountedInfo)
    {
        RootDebug_Printf("*** Failed to allocate memory for mount info\n");
        K2OS_Heap_Free(pIoBuf);
        RootDevNode_ReleaseRef(&refNewNode);
        return;
    }

    K2MEM_Copy(refNewNode.mpRefNode->InSec.mpMountedInfo, pIoBuf, ioBytes);
    refNewNode.mpRefNode->InSec.mMountedInfoBytes = ioBytes + 1;
    refNewNode.mpRefNode->InSec.mpMountedInfo[ioBytes] = 0;

    refNewNode.mpRefNode->InSec.mPlatContext = K2OS_Root_CreatePlatNode(
        gRootDevNode.InSec.mPlatContext,
        pAcpiNode->mpDeviceInfo->Name,
        (UINT_PTR)refNewNode.mpRefNode,
        pIoBuf,
        ROOTPLAT_MOUNT_MAX_BYTES,
        &ioBytes);

    if (0 == refNewNode.mpRefNode->InSec.mPlatContext)
    {
        RootDebug_Printf("*** Mount of plat node failed (last status 0x%08X)\n", K2OS_Thread_GetLastStatus());
        K2OS_Heap_Free(refNewNode.mpRefNode->InSec.mpMountedInfo);
        K2OS_Heap_Free(pIoBuf);
        RootDevNode_ReleaseRef(&refNewNode);
        return;
    }

    if (ioBytes > 0)
    {
        K2_ASSERT(ioBytes <= ROOTPLAT_MOUNT_MAX_BYTES);
        refNewNode.mpRefNode->InSec.mpPlatInfo = (UINT8 *)K2OS_Heap_Alloc((ioBytes + 4) & ~3);
        K2_ASSERT(NULL != refNewNode.mpRefNode->InSec.mpPlatInfo);
        K2MEM_Copy(refNewNode.mpRefNode->InSec.mpPlatInfo, pIoBuf, ioBytes);
        refNewNode.mpRefNode->InSec.mpPlatInfo[ioBytes] = 0;
        refNewNode.mpRefNode->InSec.mPlatInfoBytes = ioBytes;
    }

    K2OS_Heap_Free(pIoBuf);

    RootDevNode_CreateRef(&pAcpiNode->RefDevNode, refNewNode.mpRefNode);
    pNewDevNode = pAcpiNode->RefDevNode.mpRefNode;

    pAcpiNode->CurrentAcpiRes.Pointer = NULL;
    pAcpiNode->CurrentAcpiRes.Length = ACPI_ALLOCATE_BUFFER;
    acpiStatus = AcpiGetCurrentResources(pAcpiNode->mhObject, &pAcpiNode->CurrentAcpiRes);
    if (ACPI_FAILURE(acpiStatus))
    {
        pAcpiNode->CurrentAcpiRes.Pointer = NULL;
        pAcpiNode->CurrentAcpiRes.Length = 0;
    }
    if (pAcpiNode->CurrentAcpiRes.Length > 0)
    {
        AcpiWalkResourceBuffer(&pAcpiNode->CurrentAcpiRes, RootAcpiDriver_ResEnumCallback, (void *)pNewDevNode);
    }

    if (pAcpiNode->mpDeviceInfo->Flags & ACPI_PCI_ROOT_BRIDGE)
    {
        //
        // permit root process access to io ports for the bridge to facilitate ACPI PCI accesses
        // 
        if (0 < pNewDevNode->InSec.IoResList.mNodeCount)
        {
            pListLink = pNewDevNode->InSec.IoResList.mpHead;
            do {
                pIoRes = K2_GET_CONTAINER(ROOTDEV_IO, pListLink, Res.ListLink);
                pListLink = pListLink->mpNext;
                K2OS_Root_AddIoRange(NULL, pIoRes->DrvResIo.mBasePort, pIoRes->DrvResIo.mSizeBytes);
            } while (NULL != pListLink);
        }
    }

    K2OS_CritSec_Enter(&gRootDevNode.Sec);
    RootDevNode_CreateRef(&pNewDevNode->RefParent, &gRootDevNode);
    K2LIST_AddAtTail(&gRootDevNode.InSec.ChildList, &pNewDevNode->ParentChildListLink);
    K2OS_CritSec_Leave(&gRootDevNode.Sec);

    //
    // check for output from plat for forced driver
    // if there is one set the candidate driver and fire the candidate driver changed event
    // for the new node
    //
    K2OS_CritSec_Enter(&pNewDevNode->Sec);

    if (RootDevNodeState_NoDriver == pNewDevNode->InSec.mState)
    {
        if (0 != pNewDevNode->InSec.mPlatInfoBytes)
        {
            //
            // set driver candidate as output from plat
            //
            pNewDevNode->InSec.mpDriverCandidate = (char *)K2OS_Heap_Alloc((pNewDevNode->InSec.mPlatInfoBytes + 4) & ~3);
            if (NULL == pNewDevNode->InSec.mpDriverCandidate)
            {
                RootDebug_Printf("*** Memory allocation failure creating driver candidate\n");
            }
            else
            {
                K2MEM_Copy(pNewDevNode->InSec.mpDriverCandidate, pNewDevNode->InSec.mpPlatInfo, pNewDevNode->InSec.mPlatInfoBytes);
                pNewDevNode->InSec.mpDriverCandidate[pNewDevNode->InSec.mPlatInfoBytes] = 0;
                pNewDevNode->InSec.mState = RootDevNodeState_DriverSpec;
                pEvent = RootDevMgr_AllocEvent(0, pNewDevNode);
                K2_ASSERT(NULL != pEvent);
                pEvent->mType = RootEvent_DriverEvent;
                pEvent->mDriverType = RootDriverEvent_CandidateChanged;
                RootDevMgr_EventOccurred(pEvent);
            }
        }
    }

    K2OS_CritSec_Leave(&pNewDevNode->Sec);

    RootDevNode_ReleaseRef(&refNewNode);
}

void
RootNodeAcpiDriver_CreateChild(
    ROOTACPI_NODE * apChildNode
)
{
    ROOTDEV_NODE *                  pDevNodeMem;
    ROOTDEV_NODEREF                 newNodeRef;
    ROOTDEV_DRIVER_NEWCHILD_EVENT * pNewChildEvent;

    pDevNodeMem = (ROOTDEV_NODE *)K2OS_Heap_Alloc(sizeof(ROOTDEV_NODE));
    if (NULL == pDevNodeMem)
    {
        RootDebug_Printf("*** Memory allocation failed creating child node for root acpi\n");
        return;
    }

    newNodeRef.mpRefNode = NULL;
    if (!RootDevNode_Init(pDevNodeMem, &newNodeRef))
    {
        RootDebug_Printf("*** Failed to initialize dev node\n");
        K2OS_Heap_Free(newNodeRef.mpRefNode);
        return;
    }

    pNewChildEvent = (ROOTDEV_DRIVER_NEWCHILD_EVENT *)RootDevMgr_AllocEvent(sizeof(ROOTDEV_DRIVER_NEWCHILD_EVENT), &gRootDevNode);
    if (NULL == pNewChildEvent)
    {
        RootDevNode_ReleaseRef(&newNodeRef);
        RootDebug_Printf("*** Memory allocation failed creating new child node event\n");
        return;
    }

    pNewChildEvent->DevEvent.mType = RootEvent_DriverEvent;
    pNewChildEvent->DevEvent.mDriverType = RootDriverEvent_NewChild;
    pNewChildEvent->mAddress = apChildNode->mpDeviceInfo->Address;
    RootDevNode_CreateRef(&pNewChildEvent->RefNewDevNode, newNodeRef.mpRefNode);

    newNodeRef.mpRefNode->InSec.mpAcpiNodeInfo = &apChildNode->Info;

    RootDevNode_ReleaseRef(&newNodeRef);

    RootDevMgr_EventOccurred(&pNewChildEvent->DevEvent);
}

void
RootNodeAcpiDriver_EnumCallback(
    ACPI_HANDLE ahParent,
    ACPI_HANDLE ahChild,
    UINT_PTR    aContext
)
{
    ROOTACPI_NODE * pParentAcpiNode;
    ROOTACPI_NODE * pNewChildNode;

    pParentAcpiNode = (ROOTACPI_NODE *)aContext;
    K2_ASSERT(pParentAcpiNode->mhObject == ahParent);

    pNewChildNode = (ROOTACPI_NODE *)K2OS_Heap_Alloc(sizeof(ROOTACPI_NODE));
    if (NULL == pNewChildNode)
    {
        RootDebug_Printf("***Memory alloc failure creating acpi node\n");
        return;
    }
    K2MEM_Zero(pNewChildNode, sizeof(ROOTACPI_NODE));

    pNewChildNode->mpParentAcpiNode = pParentAcpiNode;
    K2LIST_AddAtTail(&pParentAcpiNode->ChildAcpiNodeList, &pNewChildNode->ParentChildListLink);

    RootNodeAcpiDriver_InitNode(pNewChildNode, ahChild);
}

void
RootNodeAcpiDriver_InitNode(
    ROOTACPI_NODE * apAcpiNode,
    ACPI_HANDLE     ahObject
)
{
    ACPI_DEVICE_INFO *  pAcpiDevInfo;
    ACPI_STATUS         acpiStatus;

    K2LIST_Init(&apAcpiNode->ChildAcpiNodeList);

    apAcpiNode->mhObject = ahObject;
    acpiStatus = AcpiGetObjectInfo(ahObject, &pAcpiDevInfo);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));
    apAcpiNode->mpDeviceInfo = pAcpiDevInfo;

    K2MEM_Copy(&apAcpiNode->Info.mName, &pAcpiDevInfo->Name, sizeof(UINT32));
    apAcpiNode->Info.mpHardwareId = (pAcpiDevInfo->HardwareId.Length > 0) ? pAcpiDevInfo->HardwareId.String : "";

    K2OSACPI_EnumChildren(ahObject, RootNodeAcpiDriver_EnumCallback, (UINT_PTR)apAcpiNode);
}

void
RootNodeAcpiDriver_SystemBus_AcquireIo(
    ROOTACPI_NODE * apAcpiNode,
    ACPI_RESOURCE * apRes
)
{
    K2OS_IOADDR_INFO    ioInfo;
    BOOL                ok;

    if (apRes->Type == ACPI_RESOURCE_TYPE_IO)
    {
        ioInfo.mPortBase = apRes->Data.Io.Minimum;
        ioInfo.mPortCount = apRes->Data.Io.AddressLength;
    }
    else
    {
        ioInfo.mPortBase = apRes->Data.FixedIo.Address;
        ioInfo.mPortCount = apRes->Data.FixedIo.AddressLength;
    }
    //    RootDebug_Printf("  [IO %04X %04X]\n", ioInfo.mPortBase, ioInfo.mPortCount);
    ok = RootRes_IoHeap_Reserve(&ioInfo, (UINT_PTR)apAcpiNode, FALSE);
    K2_ASSERT(ok);
    ok = K2OS_Root_AddIoRange(NULL, ioInfo.mPortBase, ioInfo.mPortCount);
    K2_ASSERT(ok);
}

void
RootNodeAcpiDriver_SystemBus_AcquireMemory(
    ROOTACPI_NODE * apAcpiNode,
    ACPI_RESOURCE * apRes
)
{
    K2OS_PHYSADDR_INFO  physInfo;
    BOOL                ok;

    if (apRes->Type == ACPI_RESOURCE_TYPE_MEMORY24)
    {
        physInfo.mBaseAddr = apRes->Data.Memory24.Minimum;
        physInfo.mSizeBytes = apRes->Data.Memory24.AddressLength;
    }
    else if (apRes->Type == ACPI_RESOURCE_TYPE_MEMORY32)
    {
        physInfo.mBaseAddr = apRes->Data.Memory32.Minimum;
        physInfo.mSizeBytes = apRes->Data.Memory32.AddressLength;
    }
    else
    {
        physInfo.mBaseAddr = apRes->Data.FixedMemory32.Address;
        physInfo.mSizeBytes = apRes->Data.FixedMemory32.AddressLength;
    }
    //    RootDebug_Printf("  [PHYS %08X %08X]\n", physInfo.mBaseAddr, physInfo.mSizeBytes);
    ok = RootRes_PhysHeap_Reserve(&physInfo, (UINT_PTR)apAcpiNode, FALSE);
    K2_ASSERT(ok);
}

ACPI_STATUS
RootNodeAcpiDriver_SystemBus_AcquireResources(
    ACPI_RESOURCE * Resource,
    void *          Context
)
{
    ROOTACPI_NODE * pAcpiNode;

    pAcpiNode = (ROOTACPI_NODE *)Context;

    switch (Resource->Type)
    {
    case ACPI_RESOURCE_TYPE_IO:
    case ACPI_RESOURCE_TYPE_FIXED_IO:
        RootNodeAcpiDriver_SystemBus_AcquireIo(pAcpiNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_MEMORY24:
    case ACPI_RESOURCE_TYPE_MEMORY32:
    case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
        RootNodeAcpiDriver_SystemBus_AcquireMemory(pAcpiNode, Resource);
        break;

    default:
        // ignore
        break;
    }

    return AE_OK;
}

void
RootNodeAcpiDriver_OnEvent_SetRunning(
    void
)
{
    ROOTACPI_NODE * pAcpiNode;
    K2LIST_LINK *   pListLink;
    ACPI_STATUS     acpiStatus;
    ROOTACPI_NODE * pChildNode;
    UINT32          staFlags;

    K2_ASSERT(RootDevNodeState_Running == gRootDevNode.InSec.mState);

    //
    // parse the ACPI tree. do not invoke any methods
    //
    RootNodeAcpiDriver_InitNode(&gAcpiTreeRoot, ACPI_ROOT_OBJECT);

    //
    // find the system bus _SB_
    //
    pAcpiNode = NULL;
    pListLink = gAcpiTreeRoot.ChildAcpiNodeList.mpHead;
    K2_ASSERT(NULL != pListLink);
    do {
        pAcpiNode = K2_GET_CONTAINER(ROOTACPI_NODE, pListLink, ParentChildListLink);
        if (0 == K2ASC_Comp(pAcpiNode->Info.mName, "_SB_"))
            break;
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);
    K2_ASSERT(NULL != pAcpiNode);

    pAcpiNode->CurrentAcpiRes.Pointer = NULL;
    pAcpiNode->CurrentAcpiRes.Length = ACPI_ALLOCATE_BUFFER;
    acpiStatus = AcpiGetCurrentResources(pAcpiNode->mhObject, &pAcpiNode->CurrentAcpiRes);
    if (ACPI_FAILURE(acpiStatus))
    {
        pAcpiNode->CurrentAcpiRes.Pointer = NULL;
        pAcpiNode->CurrentAcpiRes.Length = 0;
    }
    if (pAcpiNode->CurrentAcpiRes.Length > 0)
    {
        AcpiWalkResourceBuffer(&pAcpiNode->CurrentAcpiRes, RootNodeAcpiDriver_SystemBus_AcquireResources, (void *)pAcpiNode);
    }

    pListLink = pAcpiNode->ChildAcpiNodeList.mpHead;
    K2_ASSERT(NULL != pListLink);
    do {
        pChildNode = K2_GET_CONTAINER(ROOTACPI_NODE, pListLink, ParentChildListLink);
        //
        // ignore legacy interrupt routing as IOAPIC is required and is being used
        //
        if (0 != K2ASC_Comp(pChildNode->Info.mpHardwareId, "PNP0C0F"))
        {
            staFlags = K2OSACPI_RunSTA(pChildNode->mhObject);
            if ((0 != (staFlags & ACPI_STA_DEVICE_PRESENT)) ||
                (0 != (staFlags & ACPI_STA_DEVICE_FUNCTIONING)))
            {
                RootNodeAcpiDriver_CreateChild(pChildNode);
            }
        }
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);
}

void
RootNodeAcpiDriver_OnEvent(
    ROOTDEV_NODE *  apDevNode,
    ROOTDEV_EVENT * apEvent
)
{
    K2_ASSERT(apDevNode == &gRootDevNode);
    K2_ASSERT(apEvent->RefDevNode.mpRefNode == &gRootDevNode);
    K2_ASSERT(gRootDevNode.RefParent.mpRefNode == NULL);

    if (apEvent->mType == RootEvent_DriverEvent)
    {
        switch (apEvent->mDriverType)
        {
        case RootDriverEvent_SetRunning:
            RootNodeAcpiDriver_OnEvent_SetRunning();
            break;

        case RootDriverEvent_NewChild:
            RootNodeAcpiDriver_OnEvent_NewChild((ROOTDEV_DRIVER_NEWCHILD_EVENT *)apEvent);
            break;

        default:
            K2_ASSERT(0);
            break;
        }
    }
    else
    {
        K2_ASSERT(0);
    }
}

void
RootNodeAcpiDriver_Start(
    void
)
{
    ROOTDEV_EVENT *  pDevEvent;

    K2_ASSERT(0 != gRootDevNode.InSec.mPlatContext);

    //
    // attach the rootacpidriver to the dev node and generate the attach event
    //
    gRootDevNode.mfOnEvent = RootNodeAcpiDriver_OnEvent;
    gRootDevNode.InSec.mState = RootDevNodeState_Running;
    pDevEvent = RootDevMgr_AllocEvent(0, &gRootDevNode);
    K2_ASSERT(NULL != pDevEvent);
    pDevEvent->mType = RootEvent_DriverEvent;
    pDevEvent->mDriverType = RootDriverEvent_SetRunning;
    RootDevMgr_EventOccurred(pDevEvent);
}

