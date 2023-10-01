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
#include <acevents.h>

static ACPI_NODE sgAcpiTreeRoot;

void
Acpi_SystemNotifyHandler(
    ACPI_HANDLE                 Device,
    UINT32                      Value,
    void                        *Context)
{
    K2OSKERN_Debug("%s:%d\n", __FUNCTION__, __LINE__);
    K2_ASSERT(0);   // want to know
}

#if K2_TARGET_ARCH_IS_INTEL
void
Acpi_UseIOAPIC(
    void
)
{
    ACPI_STATUS         acpiStatus;
    ACPI_OBJECT_LIST    argList;
    ACPI_OBJECT         methodArg[1];
    ACPI_BUFFER         acpiResult;

    argList.Count = 1;
    argList.Pointer = methodArg;
    methodArg[0].Type = ACPI_TYPE_INTEGER;
    methodArg[0].Integer.Value = 1;   // IOAPIC mode
    acpiResult.Pointer = 0;
    acpiResult.Length = ACPI_ALLOCATE_BUFFER;

    acpiStatus = AcpiEvaluateObject(NULL, "\\_PIC", &argList, &acpiResult);
    if (acpiResult.Pointer != 0)
        K2OS_Heap_Free(acpiResult.Pointer);

    if (acpiStatus == AE_NOT_FOUND)
        return;

    K2_ASSERT(!ACPI_FAILURE(acpiStatus));
}
#endif

void
ACPI_Init(
    K2OSACPI_INIT *apInit
)
{
    ACPI_STATUS         acpiStatus;
    ACPI_TABLE_FADT *   pFADT;

    K2OSACPI_Init(apInit);

    acpiStatus = AcpiInitializeSubsystem();
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));
    acpiStatus = AcpiInitializeTables(NULL, 16, FALSE);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    //
    // ensure FADT can be found
    //
    acpiStatus = AcpiGetTable(ACPI_SIG_FADT, 0, (ACPI_TABLE_HEADER **)&pFADT);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));
}

void
ACPI_Enable(
    void
)
{
    ACPI_STATUS acpiStatus;

    acpiStatus = AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT,
        ACPI_SYSTEM_NOTIFY, Acpi_SystemNotifyHandler, NULL);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiEnableSubsystem(ACPI_NO_HANDLER_INIT);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

//    K2OSKERN_Debug("Loading and parsing ACPI...\n");
    acpiStatus = AcpiLoadTables();
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

#if K2_TARGET_ARCH_IS_INTEL
    Acpi_UseIOAPIC();
#endif
}

UINT32
ACPI_PowerButtonHandler(
    void *Context
)
{
    K2OSKERN_Debug("Acpi_PowerButtonHandler\n");
    AcpiClearEvent(ACPI_EVENT_POWER_BUTTON);
    return (AE_OK);
}

void
ACPI_Handlers_Init2(
    void
)
{
    ACPI_STATUS acpiStatus;

    acpiStatus = AcpiEvInstallXruptHandlers();
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON, ACPI_PowerButtonHandler, NULL);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiEnableEvent(ACPI_EVENT_POWER_BUTTON, 0);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));
}

void
ACPI_InitNode(
    ACPI_NODE * apAcpiNode,
    ACPI_HANDLE ahObject
);

void
ACPI_EnumCallback(
    ACPI_HANDLE ahParent,
    ACPI_HANDLE ahChild,
    UINT32      aContext
)
{
    ACPI_NODE * pParentAcpiNode;
    ACPI_NODE * pNewChildNode;

    pParentAcpiNode = (ACPI_NODE *)aContext;
    K2_ASSERT(pParentAcpiNode->mhObject == ahParent);

    pNewChildNode = (ACPI_NODE *)K2OS_Heap_Alloc(sizeof(ACPI_NODE));
    if (NULL == pNewChildNode)
    {
        K2OSKERN_Debug("***Memory alloc failure creating acpi node\n");
        return;
    }
    K2MEM_Zero(pNewChildNode, sizeof(ACPI_NODE));

    pNewChildNode->mpParentAcpiNode = pParentAcpiNode;
    K2LIST_AddAtTail(&pParentAcpiNode->ChildAcpiNodeList, &pNewChildNode->ParentChildListLink);

    ACPI_InitNode(pNewChildNode, ahChild);
}

void
ACPI_Describe(
    ACPI_NODE * apAcpiNode,
    char **     appOut,
    UINT32 *  apLeft
)
{
    ACPI_DEVICE_INFO *  pInfo;
    char *              pEnd;
    UINT32              left;
    UINT32              ate;
    UINT32              ix;

    pInfo = apAcpiNode->mpDeviceInfo;
    K2_ASSERT(NULL != pInfo);

    pEnd = *appOut;
    *pEnd = 0;
    left = *apLeft;

    ate = K2ASC_PrintfLen(pEnd, left, "ACPI;");
    left -= ate;
    pEnd += ate;
    if (pInfo->Flags != 0)
    {
        ate = K2ASC_PrintfLen(pEnd, left, "FLAGS(%02X);", pInfo->Flags);
        left -= ate;
        pEnd += ate;

        if (0 != (pInfo->Flags & ACPI_PCI_ROOT_BRIDGE))
        {
            ate = K2ASC_PrintfLen(pEnd, left, "ROOT_BRIDGE;");
            left -= ate;
            pEnd += ate;
        }

        if ((0 != (pInfo->Valid & ACPI_VALID_UID)) && (pInfo->UniqueId.Length > 0))
        {
            ate = K2ASC_PrintfLen(pEnd, left, "U(%s);", pInfo->UniqueId.String);
            left -= ate;
            pEnd += ate;
        }

        if ((0 != (pInfo->Valid & ACPI_VALID_HID)) && (pInfo->HardwareId.Length > 0))
        {
            ate = K2ASC_PrintfLen(pEnd, left, "H(%s);", pInfo->HardwareId.String);
            left -= ate;
            pEnd += ate;
        }

        if ((0 != (pInfo->Valid & ACPI_VALID_CLS)) && (pInfo->ClassCode.Length > 0))
        {
            ate = K2ASC_PrintfLen(pEnd, left, "CC(%s);", pInfo->ClassCode.String);
            left -= ate;
            pEnd += ate;
        }

        if ((0 != (pInfo->Valid & ACPI_VALID_CID)) && (pInfo->CompatibleIdList.Count > 0))
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
            ate = K2ASC_PrintfLen(pEnd, left, ");");
            left -= ate;
            pEnd += ate;
        }
    }

    *appOut = pEnd;
    *apLeft = left;
}

static 
void
sIndent(
    UINT32 aCount
)
{
    if (0 != aCount)
    {
        do {
            K2OSKERN_Debug(" ");
        } while (--aCount);
    }
}

void
ACPI_DumpNode(
    UINT32          aIndent,
    ACPI_NODE *     apAcpiNode
)
{
    K2LIST_LINK *       pListLink;
    ACPI_NODE *         pChild;
    ACPI_DEVICE_INFO *  pDevInfo;
    UINT32              ix;

    sIndent(aIndent); K2OSKERN_Debug("ACPINODE(%08X) - %.4s\n", apAcpiNode, (char const *)&apAcpiNode->mName);
    pDevInfo = apAcpiNode->mpDeviceInfo;
    if (NULL != pDevInfo)
    {
        if (0 != pDevInfo->Flags)
        {
            sIndent(aIndent); K2OSKERN_Debug(" FLAGS(%02X)\n", pDevInfo->Flags);
            if (0 != (pDevInfo->Flags & ACPI_PCI_ROOT_BRIDGE))
            {
                sIndent(aIndent); K2OSKERN_Debug(" ROOT_BRIDGE\n");
            }
        }

        if ((0 != (pDevInfo->Valid & ACPI_VALID_ADR)) && (0 != pDevInfo->Address))
        {
            sIndent(aIndent); K2OSKERN_Debug(" ADR(%08X%08X)\n", (UINT32)(pDevInfo->Address >> 32), (UINT32)(pDevInfo->Address & 0xFFFFFFFF));
        }

        if ((0 != (pDevInfo->Valid & ACPI_VALID_CLS)) && (0 != pDevInfo->ClassCode.Length))
        {
            sIndent(aIndent); K2OSKERN_Debug(" CLS(%s)\n", pDevInfo->ClassCode.String);
        }

        if ((0 != (pDevInfo->Valid & ACPI_VALID_CID)) && (0 != pDevInfo->CompatibleIdList.Count))
        {
            for (ix = 0; ix < pDevInfo->CompatibleIdList.Count; ix++)
            {
                if (0 != pDevInfo->CompatibleIdList.Ids[ix].Length)
                {
                    sIndent(aIndent); K2OSKERN_Debug(" CID %2d/%2d: %s\n", ix, pDevInfo->CompatibleIdList.Count, pDevInfo->CompatibleIdList.Ids[ix].String);
                }
                else
                {
                    sIndent(aIndent); K2OSKERN_Debug(" CID %2d/%2d: <EMPTY>\n", ix, pDevInfo->CompatibleIdList.Count);
                }
            }
        }

        if ((0 != (pDevInfo->Valid & ACPI_VALID_HID)) && (0 != pDevInfo->HardwareId.Length))
        {
            sIndent(aIndent); K2OSKERN_Debug(" HID(%s)\n", pDevInfo->HardwareId.String);
        }

        if ((0 != (pDevInfo->Valid & ACPI_VALID_UID)) && (0 != pDevInfo->UniqueId.Length))
        {
            sIndent(aIndent); K2OSKERN_Debug(" UNIQUEID(%s)\n", pDevInfo->UniqueId.String);
        }
    }
    else
    {
        sIndent(aIndent);  K2OSKERN_Debug(" <NO DEVINFO>\n");
    }
    pListLink = apAcpiNode->ChildAcpiNodeList.mpHead;
    if (NULL != pListLink)
    {
        sIndent(aIndent); K2OSKERN_Debug(" CHILDREN:\n");
        do {
            pChild = K2_GET_CONTAINER(ACPI_NODE, pListLink, ParentChildListLink);
            ACPI_DumpNode(aIndent + 4, pChild);
            pListLink = pListLink->mpNext;
        } while (NULL != pListLink);
    }
}

void
ACPI_DiscoveredIrq(
    DEVNODE *       apNode,
    ACPI_RESOURCE * apRes
)
{
    UINT32          ix;
    K2STAT          stat;
    K2OSDDK_RESDEF  resDef;

    K2OS_CritSec_Enter(&apNode->Sec);

    for (ix = 0; ix < apRes->Data.Irq.InterruptCount; ix++)
    {
        K2OSKERN_Debug("  [IRQ %d]\n", apRes->Data.Irq.Interrupts[ix]);

        K2MEM_Zero(&resDef, sizeof(resDef));

        resDef.mType = K2OS_RESTYPE_IRQ;
        resDef.mId = apNode->InSec.IrqResList.mNodeCount;
        resDef.Irq.Config.mSourceIrq = apRes->Data.Irq.Interrupts[ix];
        resDef.Irq.Config.mDestCoreIx = 0;
        resDef.Irq.Config.Line.mIsEdgeTriggered = apRes->Data.Irq.Triggering ? TRUE : FALSE;
        resDef.Irq.Config.Line.mIsActiveLow = apRes->Data.Irq.Polarity ? TRUE : FALSE;
        resDef.Irq.Config.Line.mShareConfig = apRes->Data.Irq.Shareable ? TRUE : FALSE;
        resDef.Irq.Config.Line.mWakeConfig = apRes->Data.Irq.WakeCapable ? TRUE : FALSE;

        stat = Dev_NodeLocked_AddRes(apNode, &resDef, apRes);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("  [IRQ %d] failed to be added\n", apRes->Data.Irq.Interrupts[ix]);
        }
    }

    K2OS_CritSec_Leave(&apNode->Sec);
}

void
ACPI_DiscoveredExtIrq(
    DEVNODE *       apNode,
    ACPI_RESOURCE * apRes
)
{
    UINT32          ix;
    K2STAT          stat;
    K2OSDDK_RESDEF  resDef;

    for (ix = 0; ix < apRes->Data.ExtendedIrq.InterruptCount; ix++)
    {
//        K2OSKERN_Debug("  [EXTIRQ %d]\n", apRes->Data.ExtendedIrq.Interrupts[ix]);

        K2MEM_Zero(&resDef, sizeof(resDef));

        resDef.mType = K2OS_RESTYPE_IRQ;
        resDef.mId = 0x100 + apNode->InSec.IrqResList.mNodeCount;
        resDef.Irq.Config.mSourceIrq = apRes->Data.ExtendedIrq.Interrupts[ix];
        resDef.Irq.Config.mDestCoreIx = 0;
        resDef.Irq.Config.Line.mIsEdgeTriggered = apRes->Data.ExtendedIrq.Triggering ? TRUE : FALSE;
        resDef.Irq.Config.Line.mIsActiveLow = apRes->Data.ExtendedIrq.Polarity ? TRUE : FALSE;
        resDef.Irq.Config.Line.mShareConfig = apRes->Data.ExtendedIrq.Shareable ? TRUE : FALSE;
        resDef.Irq.Config.Line.mWakeConfig = apRes->Data.ExtendedIrq.WakeCapable ? TRUE : FALSE;

        stat = Dev_NodeLocked_AddRes(apNode, &resDef, apRes);
        if (K2STAT_IS_ERROR(stat))
        {
            K2OSKERN_Debug("  [EXTIRQ %d] failed to be added\n", apRes->Data.ExtendedIrq.Interrupts[ix]);
        }
    }
}

void
ACPI_DiscoveredDMA(
    DEVNODE *       apNode,
    ACPI_RESOURCE * apRes
)
{
#if 1
    if (apRes->Type == ACPI_RESOURCE_TYPE_DMA)
        K2OSKERN_Debug("  DMA(%d chan)\n", apRes->Data.Dma.ChannelCount);
    else
        K2OSKERN_Debug("  FIXED_DMA(%d chan)\n", apRes->Data.FixedDma.Channels);
    K2_ASSERT(0);
#endif
}

void
ACPI_DiscoveredIo(
    DEVNODE *       apNode,
    ACPI_RESOURCE * apRes
)
{
    K2OSDDK_RESDEF  resDef;
    UINT16          basePort;
    UINT16          sizeBytes;
    K2STAT          stat;

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
//    K2OSKERN_Debug("  [IO %04X %04X]\n", basePort, sizeBytes);

    K2MEM_Zero(&resDef, sizeof(resDef));

    resDef.mType = K2OS_RESTYPE_IO;
    resDef.mId = apNode->InSec.IoResList.mNodeCount;
    resDef.Io.Range.mBasePort = basePort;
    resDef.Io.Range.mSizeBytes = sizeBytes;

    stat = Dev_NodeLocked_AddRes(apNode, &resDef, apRes);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("  [IO %d] failed to be added\n", resDef.mId);
    }
}

void
ACPI_AddPhys(
    DEVNODE *   apNode,
    UINT32      aBaseAddr,
    UINT32      aSizeBytes,
    UINT32      aAcpiContext
)
{
    K2OSDDK_RESDEF  resDef;
    K2STAT          stat;

//    K2OSKERN_Debug("  [PHYS %08X %08X]\n", aBaseAddr, aSizeBytes);

    if ((0 != (aBaseAddr & K2_VA_MEMPAGE_OFFSET_MASK)) ||
        (0 == aSizeBytes) ||
        (0 != (aSizeBytes & K2_VA_MEMPAGE_OFFSET_MASK)))
    {
        K2OSKERN_Debug("  [PHYS %08X %08X] IGNORED BAD ALIGN OR SIZE\n", aBaseAddr, aSizeBytes);
        return;
    }

    K2MEM_Zero(&resDef, sizeof(resDef));

    resDef.mType = K2OS_RESTYPE_PHYS;
    resDef.mId = apNode->InSec.PhysResList.mNodeCount;
    resDef.Phys.Range.mBaseAddr = aBaseAddr;
    resDef.Phys.Range.mSizeBytes = aSizeBytes;

    stat = Dev_NodeLocked_AddRes(apNode, &resDef, (void *)aAcpiContext);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("  [IO %d] failed to be added\n", resDef.mId);
    }
}

void
ACPI_DiscoveredMemory(
    DEVNODE *       apNode,
    ACPI_RESOURCE * apRes
)
{
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

    ACPI_AddPhys(apNode, baseAddr, sizeBytes, (UINT32)apRes);
}

void
ACPI_DiscoveredAddress(
    DEVNODE *       apNode,
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
    K2OSKERN_Debug("ADDRESS(%d, %d)\n", apRes->Type, addr64.ResourceType);
    K2OSKERN_Debug("  ProducerConsumer %02X\n", addr64.ProducerConsumer);
    K2OSKERN_Debug("  Decode           %02X\n", addr64.Decode);
    K2OSKERN_Debug("  MinAddressFixed  %02X\n", addr64.MinAddressFixed);
    K2OSKERN_Debug("  MaxAddressFixed  %02X\n", addr64.MaxAddressFixed);
    K2OSKERN_Debug("  Address.Granularity         %08X%08X\n", (UINT32)((addr64.Address.Granularity >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.Granularity & 0xFFFFFFFFull));
    K2OSKERN_Debug("  Address.Minimum             %08X%08X\n", (UINT32)((addr64.Address.Minimum >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.Minimum & 0xFFFFFFFFull));
    K2OSKERN_Debug("  Address.Maximum             %08X%08X\n", (UINT32)((addr64.Address.Maximum >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.Maximum & 0xFFFFFFFFull));
    K2OSKERN_Debug("  Address.TranslationOffset   %08X%08X\n", (UINT32)((addr64.Address.TranslationOffset >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.TranslationOffset & 0xFFFFFFFFull));
    K2OSKERN_Debug("  Address.AddressLength       %08X%08X\n", (UINT32)((addr64.Address.AddressLength >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.AddressLength & 0xFFFFFFFFull));
#endif
}

ACPI_STATUS
ACPI_ResCreateCallback(
    ACPI_RESOURCE * Resource,
    void *          Context
)
{
    DEVNODE *   pDevNode;

    pDevNode = (DEVNODE *)Context;

    switch (Resource->Type)
    {
    case ACPI_RESOURCE_TYPE_IRQ:
        ACPI_DiscoveredIrq(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
        ACPI_DiscoveredExtIrq(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_DMA:
    case ACPI_RESOURCE_TYPE_FIXED_DMA:
        ACPI_DiscoveredDMA(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_IO:
    case ACPI_RESOURCE_TYPE_FIXED_IO:
        ACPI_DiscoveredIo(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_MEMORY24:
    case ACPI_RESOURCE_TYPE_MEMORY32:
    case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
        ACPI_DiscoveredMemory(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_ADDRESS16:
    case ACPI_RESOURCE_TYPE_ADDRESS32:
    case ACPI_RESOURCE_TYPE_ADDRESS64:
    case ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64:
        ACPI_DiscoveredAddress(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_END_TAG:
        // ignore
        break;

    default:
        K2OSKERN_Debug("!!!!SysProcAcpiDriver_ResEnumCallback - %d\n", Resource->Type);
        //
        // dont know what to do with this
        //
        K2_ASSERT(0);
    }

    return AE_OK;
}

void
ACPI_NodeLocked_PopulateRes(
    DEVNODE *apNode
)
{
    ACPI_NODE *             pAcpiNode;
    ACPI_STATUS             acpiStatus;
    UINT64                  seg;
    UINT64                  bbn;
    UINT32                  sizeEnt;
    UINT32                  entCount;
    ACPI_TABLE_MCFG *       pMCFG;
    ACPI_MCFG_ALLOCATION *  pAlloc;

    pAcpiNode = apNode->InSec.mpAcpiNode;
    K2_ASSERT(NULL != pAcpiNode);

    //
    // parent and child both still locked.
    // populate child node with resources
    //
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
        AcpiWalkResourceBuffer(&pAcpiNode->CurrentAcpiRes, ACPI_ResCreateCallback, (void *)apNode);
    }

    if (pAcpiNode->mpDeviceInfo->Flags & ACPI_PCI_ROOT_BRIDGE)
    {
        //
        // This is a PCI root bridge. Go see if there are PCI CFG physical ranges that need to be added
        // to this node
        //
        apNode->mBusType = K2OS_BUSTYPE_PCI;

        //
        // get segment via "_SEG"
        //
        acpiStatus = K2OSACPI_RunDeviceNumericMethod(
            pAcpiNode->mhObject,
            "_SEG",
            &seg);
        if (ACPI_FAILURE(acpiStatus))
        {
            seg = 0;
        }
        else
        {
            seg &= 0xFF;
        }

        //
        // get bridge bus number via "_BBN"
        //
        acpiStatus = K2OSACPI_RunDeviceNumericMethod(
            pAcpiNode->mhObject,
            "_BBN",
            &bbn);
        if (!ACPI_FAILURE(acpiStatus))
        {
            //
            // try to find the bridge in the MCFG table
            //
            pMCFG = NULL;
            acpiStatus = AcpiGetTable(ACPI_SIG_MCFG, 0, (ACPI_TABLE_HEADER **)&pMCFG);
            if (!ACPI_FAILURE(acpiStatus))
            {
                K2_ASSERT(pMCFG != NULL);
                K2_ASSERT(pMCFG->Header.Length > sizeof(ACPI_TABLE_MCFG));
                sizeEnt = pMCFG->Header.Length - sizeof(ACPI_TABLE_MCFG);
                entCount = sizeEnt / sizeof(ACPI_MCFG_ALLOCATION);
                K2_ASSERT(0 < entCount);
                pAlloc = (ACPI_MCFG_ALLOCATION *)(((UINT8 *)pMCFG) + sizeof(ACPI_TABLE_MCFG));
                do {
                    if ((pAlloc->PciSegment == seg) &&
                        (pAlloc->StartBusNumber <= bbn) &&
                        (pAlloc->EndBusNumber >= bbn))
                    {
                        break;
                    }
                    pAlloc = (ACPI_MCFG_ALLOCATION *)(((UINT8 *)pAlloc) + sizeof(ACPI_MCFG_ALLOCATION));
                } while (--entCount);
                if (0 != entCount)
                {
                    //
                    // add the bus physical range to the resources available for the device
                    // 256 pages per bus
                    //
                    bbn = (bbn * 0x100000) + (pAlloc->Address & 0xFFFFFFFFull);
                    ACPI_AddPhys(apNode, (UINT32)bbn, 0x100000, 0);
                }
            }
        }
    }
    else
    {
        apNode->mBusType = apNode->RefToParent.mpDevNode->mBusType;
    }
}


void
ACPI_InitNode(
    ACPI_NODE * apAcpiNode,
    ACPI_HANDLE ahObject
)
{
    ACPI_DEVICE_INFO *  pAcpiDevInfo;
    ACPI_STATUS         acpiStatus;

    K2LIST_Init(&apAcpiNode->ChildAcpiNodeList);

    apAcpiNode->mhObject = ahObject;
    acpiStatus = AcpiGetObjectInfo(ahObject, &pAcpiDevInfo);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));
    apAcpiNode->mpDeviceInfo = pAcpiDevInfo;

    K2MEM_Copy(&apAcpiNode->mName, &pAcpiDevInfo->Name, sizeof(UINT32));
    apAcpiNode->mpHardwareId = (pAcpiDevInfo->HardwareId.Length > 0) ? pAcpiDevInfo->HardwareId.String : "";

    if ((pAcpiDevInfo->UniqueId.Length > 0) &&
        (pAcpiDevInfo->UniqueId.String != NULL))
    {
        apAcpiNode->mHasUniqueId = TRUE;
        apAcpiNode->mUniqueId = K2ASC_NumValue32(pAcpiDevInfo->UniqueId.String);
    }

    K2OSACPI_EnumChildren(ahObject, ACPI_EnumCallback, (UINT32)apAcpiNode);
}

void
ACPI_SystemBus_AcquireIo(
    ACPI_NODE *     apAcpiNode,
    ACPI_RESOURCE * apRes
)
{
    K2OS_IOPORT_RANGE   ioRange;

    if (apRes->Type == ACPI_RESOURCE_TYPE_IO)
    {
        ioRange.mBasePort = apRes->Data.Io.Minimum;
        ioRange.mSizeBytes = apRes->Data.Io.AddressLength;
    }
    else
    {
        ioRange.mBasePort = apRes->Data.FixedIo.Address;
        ioRange.mSizeBytes = apRes->Data.FixedIo.AddressLength;
    }
    K2OSKERN_Debug("_SB_ io resource %04X for %04X\n", ioRange.mBasePort, ioRange.mSizeBytes);
}

void
ACPI_SystemBus_AcquireMemory(
    ACPI_NODE *     apAcpiNode,
    ACPI_RESOURCE * apRes
)
{
    K2OS_PHYSADDR_RANGE physRange;

    if (apRes->Type == ACPI_RESOURCE_TYPE_MEMORY24)
    {
        physRange.mBaseAddr = apRes->Data.Memory24.Minimum;
        physRange.mSizeBytes = apRes->Data.Memory24.AddressLength;
    }
    else if (apRes->Type == ACPI_RESOURCE_TYPE_MEMORY32)
    {
        physRange.mBaseAddr = apRes->Data.Memory32.Minimum;
        physRange.mSizeBytes = apRes->Data.Memory32.AddressLength;
    }
    else
    {
        physRange.mBaseAddr = apRes->Data.FixedMemory32.Address;
        physRange.mSizeBytes = apRes->Data.FixedMemory32.AddressLength;
    }

    K2OSKERN_Debug("_SB_ phys resource %08X for %08X\n", physRange.mBaseAddr, physRange.mSizeBytes);
}

ACPI_STATUS
ACPI_SystemBus_AcquireResources(
    ACPI_RESOURCE * Resource,
    void *          Context
)
{
    ACPI_NODE * pAcpiNode;

    pAcpiNode = (ACPI_NODE *)Context;

    switch (Resource->Type)
    {
    case ACPI_RESOURCE_TYPE_IO:
    case ACPI_RESOURCE_TYPE_FIXED_IO:
        ACPI_SystemBus_AcquireIo(pAcpiNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_MEMORY24:
    case ACPI_RESOURCE_TYPE_MEMORY32:
    case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
        ACPI_SystemBus_AcquireMemory(pAcpiNode, Resource);
        break;

    default:
        // ignore
        K2OSKERN_Panic("*** Unhandled ACPI resource type on root SB : %d\n", Resource->Type);
        break;
    }

    return AE_OK;
}

void
ACPI_DiscoveredSystemBusChild(
    ACPI_NODE *     apChildAcpiNode,
    UINT32          aChildBusType
)
{
    DEVNODE *           pChild;
    K2_DEVICE_IDENT     ident;
    char *              pIoBuf;
    char *              pOut;
    UINT32              ioBytes;
    UINT32              left;
    BOOL                ok;
    ACPI_DEVICE_INFO *  pDevInfo;

    pDevInfo = apChildAcpiNode->mpDeviceInfo;
    K2_ASSERT(NULL != pDevInfo);
    
    K2_ASSERT(apChildAcpiNode->mpParentAcpiNode == gpDevTree->InSec.mpAcpiNode);
    K2_ASSERT(NULL != apChildAcpiNode->mpDeviceInfo);
    K2MEM_Zero(&ident, sizeof(ident));
    ident.mClassCode = PCI_CLASS_BRIDGE;
    ident.mSubClassCode = PCI_BRIDGE_SUBCLASS_PCI;

    pChild = Dev_NodeLocked_CreateChildNode(gpDevTree, &pDevInfo->Address, aChildBusType, &ident);
    if (NULL == pChild)
        return;

    ok = FALSE;

    K2OS_CritSec_Enter(&pChild->Sec);

    do {
        pIoBuf = (char *)K2OS_Heap_Alloc(K2OSPLAT_MOUNT_MAX_BYTES);
        if (NULL == pIoBuf)
        {
            K2OSKERN_Debug("*** Could not allocate Io buffer for device mount to kernel\n");
            break;
        }
        do {
            pChild->InSec.mpAcpiNode = apChildAcpiNode;
            pOut = pIoBuf;
            *pOut = 0;
            left = K2OSPLAT_MOUNT_MAX_BYTES;
            ACPI_Describe(apChildAcpiNode, &pOut, &left);

            ioBytes = ((UINT32)(pOut - pIoBuf)) + 1;

            pChild->InSec.mpMountedInfo = (char *)K2OS_Heap_Alloc((ioBytes + 4) & ~3);
            if (NULL == pChild->InSec.mpMountedInfo)
            {
                K2OSKERN_Debug("*** Failed to allocate memory for mount info\n");
                break;
            }

            do {
                K2MEM_Copy(pChild->InSec.mpMountedInfo, pIoBuf, ioBytes);
                pChild->InSec.mMountedInfoBytes = ioBytes + 1;
                pChild->InSec.mpMountedInfo[ioBytes] = 0;

                pChild->InSec.mPlatDev = gPlat.DeviceCreate(
                    gpDevTree->InSec.mPlatDev,
                    apChildAcpiNode->mpDeviceInfo->Name,
                    (UINT32)pChild,
                    &pChild->DeviceIdent,
                    (UINT8 *)pIoBuf,
                    K2OSPLAT_MOUNT_MAX_BYTES,
                    &ioBytes);

                if (0 == pChild->InSec.mPlatDev)
                {
                    K2OSKERN_Debug("*** Mount of plat node failed (last status 0x%08X)\n", K2OS_Thread_GetLastStatus());
                    break;
                }

                if (ioBytes > 0)
                {
                    K2_ASSERT(ioBytes <= K2OSPLAT_MOUNT_MAX_BYTES);

                    pChild->InSec.mpPlatInfo = (UINT8 *)K2OS_Heap_Alloc((ioBytes + 4) & ~3);
                    if (NULL == pChild->InSec.mpPlatInfo)
                    {
                        K2OSKERN_Debug("*** Could not allocate memory to hold node plat output\n");
                    }
                    else
                    {
                        K2MEM_Copy(pChild->InSec.mpPlatInfo, pIoBuf, ioBytes);
                        pChild->InSec.mpPlatInfo[ioBytes] = 0;
                        pChild->InSec.mPlatInfoBytes = ioBytes;
                        ok = TRUE;
                    }
                }
                else
                {
                    ok = TRUE;
                }

                if (!ok)
                {
                    gPlat.DeviceRemove(pChild->InSec.mPlatDev);
                }

            } while (0);

            if (!ok)
            {
                K2OS_Heap_Free(pChild->InSec.mpMountedInfo);
                pChild->InSec.mpMountedInfo = NULL;
            }

        } while (0);

        K2OS_Heap_Free(pIoBuf);

    } while (0);

    if (!ok)
    {
        K2OS_CritSec_Leave(&pChild->Sec);
        Dev_NodeLocked_DeleteChildNode(gpDevTree, pChild);
        return;
    }

    ACPI_NodeLocked_PopulateRes(pChild);

    K2OS_CritSec_Leave(&pChild->Sec);
}

void
ACPI_AddRamDisk(
    void
)
{
    DEVNODE *           pChild;
    char *              pIoBuf;
    char *              pOut;
    UINT32              ioBytes;
    BOOL                ok;
    UINT64              addr;
    K2_DEVICE_IDENT     ident;
    UINT32              name;

    K2MEM_Zero(&ident, sizeof(ident));
    addr = 0;
    ident.mClassCode = 0xFE;
    ident.mSubClassCode = 1;
    ident.mVendorId = 0xFEED;
    ident.mDeviceId = 0xF00D;

    pChild = Dev_NodeLocked_CreateChildNode(gpDevTree, &addr, K2OS_BUSTYPE_CPU, &ident);
    if (NULL == pChild)
        return;

    K2OS_CritSec_Enter(&pChild->Sec);

    do {
        pIoBuf = (char *)K2OS_Heap_Alloc(K2OSPLAT_MOUNT_MAX_BYTES);
        if (NULL == pIoBuf)
        {
            K2OSKERN_Debug("*** Could not allocate Io buffer for device mount to kernel\n");
            break;
        }
        do {
            pOut = pIoBuf;
            pOut += K2ASC_PrintfLen(pOut, K2OSPLAT_MOUNT_MAX_BYTES, "RAMDISK;");
            ioBytes = ((UINT32)(pOut - pIoBuf)) + 1;

            pChild->InSec.mpMountedInfo = (char *)K2OS_Heap_Alloc((ioBytes + 4) & ~3);
            if (NULL == pChild->InSec.mpMountedInfo)
            {
                K2OSKERN_Debug("*** Failed to allocate memory for mount info\n");
                break;
            }

            do {
                K2MEM_Copy(pChild->InSec.mpMountedInfo, pIoBuf, ioBytes);
                pChild->InSec.mMountedInfoBytes = ioBytes + 1;
                pChild->InSec.mpMountedInfo[ioBytes] = 0;
                K2MEM_Copy(&name, "RAMD", 4);

                pChild->InSec.mPlatDev = gPlat.DeviceCreate(
                    gpDevTree->InSec.mPlatDev,
                    name,
                    (UINT32)pChild,
                    &pChild->DeviceIdent,
                    (UINT8 *)pIoBuf,
                    K2OSPLAT_MOUNT_MAX_BYTES,
                    &ioBytes);

                if (0 == pChild->InSec.mPlatDev)
                {
                    K2OSKERN_Debug("*** Mount of plat node failed (last status 0x%08X)\n", K2OS_Thread_GetLastStatus());
                    break;
                }

                if (ioBytes > 0)
                {
                    K2_ASSERT(ioBytes <= K2OSPLAT_MOUNT_MAX_BYTES);

                    pChild->InSec.mpPlatInfo = (UINT8 *)K2OS_Heap_Alloc((ioBytes + 4) & ~3);
                    if (NULL == pChild->InSec.mpPlatInfo)
                    {
                        K2OSKERN_Debug("*** Could not allocate memory to hold node plat output\n");
                    }
                    else
                    {
                        K2MEM_Copy(pChild->InSec.mpPlatInfo, pIoBuf, ioBytes);
                        pChild->InSec.mpPlatInfo[ioBytes] = 0;
                        pChild->InSec.mPlatInfoBytes = ioBytes;
                        ok = TRUE;
                    }
                }
                else
                {
                    ok = TRUE;
                }

                if (!ok)
                {
                    gPlat.DeviceRemove(pChild->InSec.mPlatDev);
                }

            } while (0);

            if (!ok)
            {
                K2OS_Heap_Free(pChild->InSec.mpMountedInfo);
                pChild->InSec.mpMountedInfo = NULL;
            }

        } while (0);

        K2OS_Heap_Free(pIoBuf);

    } while (0);

    if (!ok)
    {
        K2OS_CritSec_Leave(&pChild->Sec);
        Dev_NodeLocked_DeleteChildNode(gpDevTree, pChild);
        return;
    }

    K2OS_CritSec_Leave(&pChild->Sec);
}

void
ACPI_StartSystemBusDriver(
    void
)
{
    ACPI_NODE *     pAcpiNode;
    K2LIST_LINK *   pListLink;
    UINT32          nodeName;
    ACPI_STATUS     acpiStatus;
    ACPI_NODE *     pChildAcpiNode;
    K2STAT          stat;
    UINT32          staFlags;

    //
    // parse the ACPI tree top level. do not invoke any methods
    //
    ACPI_InitNode(&sgAcpiTreeRoot, ACPI_ROOT_OBJECT);
//    ACPI_DumpNode(0, &sgAcpiTreeRoot);

    //
    // find the system bus _SB_ off the root
    //
    pAcpiNode = NULL;
    pListLink = sgAcpiTreeRoot.ChildAcpiNodeList.mpHead;
    K2_ASSERT(NULL != pListLink);
    do {
        pAcpiNode = K2_GET_CONTAINER(ACPI_NODE, pListLink, ParentChildListLink);
        if (0 == K2ASC_Comp(pAcpiNode->mName, "_SB_"))
            break;
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);
    K2_ASSERT(NULL != pAcpiNode);

    //
    // attach the Acpi _SB_ node to the root dev node
    //
    gpDevTree->InSec.mpAcpiNode = pAcpiNode;


    //
    // now create the plat node
    //
    K2MEM_Copy(&nodeName, "SYS:", 4);
    gpDevTree->InSec.mPlatDev = gPlat.DeviceCreate(NULL, nodeName, (UINT32)gpDevTree, &gpDevTree->DeviceIdent, NULL, 0, NULL);
    K2_ASSERT(NULL != gpDevTree->InSec.mPlatDev);

    //
    // this will hook the SCI interrupt handler
    //
    ACPI_Handlers_Init2();

    //
    // enumerate _SB_ resources
    //
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
        AcpiWalkResourceBuffer(&pAcpiNode->CurrentAcpiRes, ACPI_SystemBus_AcquireResources, (void *)pAcpiNode);
    }

    K2OS_CritSec_Enter(&gpDevTree->Sec);

    //
    // jump root bus node to online state
    //
    K2_ASSERT(gpDevTree->InSec.mState == DevNodeState_Offline_NoDriver);
    gpDevTree->InSec.mState = DevNodeState_Online;

    //
    // discover children
    //
    pListLink = pAcpiNode->ChildAcpiNodeList.mpHead;
    K2_ASSERT(NULL != pListLink);
    do {
        pChildAcpiNode = K2_GET_CONTAINER(ACPI_NODE, pListLink, ParentChildListLink);

        //
        // ignore legacy interrupt routing as IOAPIC is required and is being used
        //
        K2_ASSERT(NULL != pChildAcpiNode->mpHardwareId);

        //
        // only bus type we understand off the system bus at present the PCI bus
        //
        if (0 != K2ASC_Comp(pChildAcpiNode->mpHardwareId, "PNP0C0F"))
        {
            staFlags = K2OSACPI_RunSTA(pChildAcpiNode->mhObject);
            if ((0 != (staFlags & ACPI_STA_DEVICE_PRESENT)) ||
                (0 != (staFlags & ACPI_STA_DEVICE_FUNCTIONING)))
            {
                ACPI_DiscoveredSystemBusChild(pChildAcpiNode, K2OS_BUSTYPE_PCI);
            }
        }
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);

    //
    // add ramdisk node
    //
    ACPI_AddRamDisk();

    //
    // everything is enumerated/added.  enable the node now
    //
    stat = Dev_NodeLocked_Enable(gpDevTree);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    K2OS_CritSec_Leave(&gpDevTree->Sec);
}

BOOL 
ACPI_MatchAndAttachChild(
    ACPI_NODE *     apParentAcpiNode,
    UINT64 const *  apBusAddrOfChild,
    DEVNODE *       apChildNode
)
{
    K2LIST_LINK *   pListLink;
    ACPI_NODE *     pChild;

    K2_ASSERT(NULL != apParentAcpiNode);
    K2_ASSERT(NULL != apBusAddrOfChild);
    K2_ASSERT(NULL != apChildNode);
    K2_ASSERT(NULL == apChildNode->InSec.mpAcpiNode);

    pListLink = apParentAcpiNode->ChildAcpiNodeList.mpHead;
    if (NULL == pListLink)
        return FALSE;

    do {
        pChild = K2_GET_CONTAINER(ACPI_NODE, pListLink, ParentChildListLink);
        pListLink = pListLink->mpNext;

        K2_ASSERT(NULL != pChild->mpDeviceInfo);

        if (pChild->mpDeviceInfo->Address == (*apBusAddrOfChild))
        {
            K2OS_CritSec_Enter(&apChildNode->Sec);

            apChildNode->InSec.mpAcpiNode = pChild;

            K2OS_CritSec_Leave(&apChildNode->Sec);

            return TRUE;
        }

    } while (NULL != pListLink);

    return FALSE;
}

