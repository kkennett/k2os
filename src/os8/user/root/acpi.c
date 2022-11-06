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
#include <acevents.h>

static ACPI_NODE sgAcpiTreeRoot;

void
Acpi_ReserveBuiltInIo(
    UINT16  aBase,
    UINT16  aCount
)
{
    K2OS_IOADDR_INFO    ioInfo;
    BOOL                ok;

    ioInfo.mPortBase = aBase;
    ioInfo.mPortCount = aCount;
    ok = Res_IoHeap_Reserve(&ioInfo, 1, TRUE);
    K2_ASSERT(ok);

    ok = K2OS_Root_AddIoRange(NULL, aBase, aCount);
    K2_ASSERT(ok);
}

K2OS_INTERRUPT_TOKEN
K2_CALLCONV_REGS
Acpi_HookIntr(
    K2OS_IRQ_CONFIG const *apConfig
)
{
    DEV_IRQ *   pDevIrq;
    UINT32 *    pu;

    //
    // acpi interrupts mount on the system bus root of the tree
    //

    pDevIrq = (DEV_IRQ *)K2OS_Heap_Alloc(sizeof(DEV_IRQ));
    if (NULL == pDevIrq)
    {
        return NULL;
    }

    K2MEM_Copy(&pDevIrq->DrvResIrq.Config, apConfig, sizeof(K2OS_IRQ_CONFIG));
    pDevIrq->Res.mAcpiContext = (UINT_PTR)gpDevTree->InSec.mpAcpiNode;
    pu = (UINT32 *)apConfig;
    pDevIrq->Res.mPlatContext = K2OS_Root_AddPlatResource(gpDevTree->InSec.mPlatContext, K2OS_PLATRES_IRQ, pu[0], pu[1]);
    if (0 == pDevIrq->Res.mPlatContext)
    {
        K2OS_Heap_Free(pDevIrq);
        return NULL;
    }

    K2LIST_AddAtTail(&gpDevTree->InSec.IrqResList, &pDevIrq->Res.ListLink);

    pDevIrq->DrvResIrq.mInterruptToken = K2OS_Root_HookPlatIntr(gpDevTree->InSec.mPlatContext, apConfig->mSourceIrq);
    K2_ASSERT(NULL != pDevIrq->DrvResIrq.mInterruptToken);

    return pDevIrq->DrvResIrq.mInterruptToken;
}

void
Acpi_Init(
    void
)
{
    K2STAT                  stat;
    K2OSACPI_INIT           acpiInit;
    K2OS_FWINFO             fwInfo;
    UINT_PTR                ioBytes;
    K2OS_PAGEARRAY_TOKEN    tokPageArray;
    K2OS_MAP_TOKEN          tokMap;
    ACPI_STATUS             acpiStatus;
    ACPI_TABLE_FADT *       pFADT;

    ioBytes = sizeof(fwInfo);
    stat = K2OS_System_GetInfo(K2OS_SYSINFO_FWINFO, 0, &fwInfo, &ioBytes);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    acpiInit.mFwBasePhys = fwInfo.mFwBasePhys;
    acpiInit.mFwSizeBytes = fwInfo.mFwSizeBytes;
    acpiInit.mFacsPhys = fwInfo.mFacsPhys;
    acpiInit.mXFacsPhys = fwInfo.mXFacsPhys;

    K2_ASSERT(acpiInit.mFwBasePhys != 0);
    K2_ASSERT(acpiInit.mFwSizeBytes != 0);
    K2_ASSERT((acpiInit.mFacsPhys != 0) || (acpiInit.mXFacsPhys != 0));

    ioBytes = (acpiInit.mFwSizeBytes + (K2_VA_MEMPAGE_BYTES - 1)) / K2_VA_MEMPAGE_BYTES;

    tokPageArray = K2OS_Root_CreatePageArrayAt(acpiInit.mFwBasePhys, ioBytes);
    K2_ASSERT(NULL != tokPageArray);
    acpiInit.mFwBaseVirt = K2OS_Virt_Reserve(ioBytes);
    K2_ASSERT(0 != acpiInit.mFwBaseVirt);
    tokMap = K2OS_Map_Create(tokPageArray, 0, ioBytes, acpiInit.mFwBaseVirt, K2OS_MapType_MemMappedIo_ReadWrite);
    K2_ASSERT(NULL != tokMap);
    K2OS_Token_Destroy(tokPageArray);

    if (acpiInit.mFacsPhys != 0)
    {
        tokPageArray = K2OS_Root_CreatePageArrayAt(acpiInit.mFacsPhys, 1);
        K2_ASSERT(NULL != tokPageArray);
        acpiInit.mFacsVirt = K2OS_Virt_Reserve(1);
        K2_ASSERT(0 != acpiInit.mFacsVirt);
        tokMap = K2OS_Map_Create(tokPageArray, 0, 1, acpiInit.mFacsVirt, K2OS_MapType_MemMappedIo_ReadWrite);
        K2_ASSERT(NULL != tokMap);
        K2OS_Token_Destroy(tokPageArray);
    }
    else
    {
        K2_ASSERT(acpiInit.mXFacsPhys != 0);
        tokPageArray = K2OS_Root_CreatePageArrayAt(acpiInit.mXFacsPhys, 1);
        K2_ASSERT(NULL != tokPageArray);
        acpiInit.mXFacsVirt = K2OS_Virt_Reserve(1);
        K2_ASSERT(0 != acpiInit.mXFacsVirt);
        tokMap = K2OS_Map_Create(tokPageArray, 0, 1, acpiInit.mXFacsVirt, K2OS_MapType_MemMappedIo_ReadWrite);
        K2_ASSERT(NULL != tokMap);
        K2OS_Token_Destroy(tokPageArray);
    }

    acpiInit.mfHookIntr = Acpi_HookIntr;

    K2OSACPI_Init(&acpiInit);

    acpiStatus = AcpiInitializeSubsystem();
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiInitializeTables(NULL, 16, FALSE);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    //
    // find the acpi ports and grant ourselves access to them
    //
    acpiStatus = AcpiGetTable(ACPI_SIG_FADT, 0, (ACPI_TABLE_HEADER **)&pFADT);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));
    K2_ASSERT(NULL != pFADT);
    if (0 != pFADT->SmiCommand)
    {
        Acpi_ReserveBuiltInIo(pFADT->SmiCommand, sizeof(UINT32));
    }
    if ((0 != pFADT->Pm1aEventBlock) && (0 != pFADT->Pm1EventLength))
    {
        Acpi_ReserveBuiltInIo(pFADT->Pm1aEventBlock, pFADT->Pm1EventLength);
    }
    if ((0 != pFADT->Pm1aControlBlock) && (0 != pFADT->Pm1ControlLength))
    {
        Acpi_ReserveBuiltInIo(pFADT->Pm1aControlBlock, pFADT->Pm1ControlLength);
    }
    if ((0 != pFADT->Pm2ControlBlock) && (0 != pFADT->Pm2ControlLength))
    {
        Acpi_ReserveBuiltInIo(pFADT->Pm2ControlBlock, pFADT->Pm2ControlLength);
    }
    if ((0 != pFADT->PmTimerBlock) && (0 != pFADT->PmTimerLength))
    {
        Acpi_ReserveBuiltInIo(pFADT->PmTimerBlock, pFADT->PmTimerLength);
    }
    if ((0 != pFADT->Gpe0Block) && (0 != pFADT->Gpe0BlockLength))
    {
        Acpi_ReserveBuiltInIo(pFADT->Gpe0Block, pFADT->Gpe0BlockLength);
    }
    if ((0 != pFADT->Gpe1Block) && (0 != pFADT->Gpe1BlockLength))
    {
        Acpi_ReserveBuiltInIo(pFADT->Gpe1Block, pFADT->Gpe1BlockLength);
    }
}

void
Acpi_SystemNotifyHandler(
    ACPI_HANDLE                 Device,
    UINT32                      Value,
    void                        *Context)
{
    Debug_Printf("Acpi_NotifyHandler = Received a Notify %08X\n", Value);
    K2_ASSERT(0);   // want to know
}

ACPI_STATUS
Acpi_SysMemoryRegionInit(
    ACPI_HANDLE                 RegionHandle,
    UINT32                      Function,
    void                        *HandlerContext,
    void                        **RegionContext)
{
    Debug_Printf("Acpi_SysMemoryRegionInit = Received a region init for %08X\n", RegionHandle);
    K2_ASSERT(0);   // want to know
    if (Function == ACPI_REGION_DEACTIVATE)
    {
        *RegionContext = NULL;
    }
    else
    {
        *RegionContext = RegionHandle;
    }
    return (AE_OK);
}

ACPI_STATUS
Acpi_SysMemoryRegionHandler(
    UINT32                      Function,
    ACPI_PHYSICAL_ADDRESS       Address,
    UINT32                      BitWidth,
    UINT64                      *Value,
    void                        *HandlerContext,
    void                        *RegionContext)
{
    Debug_Printf("Acpi_SysMemoryRegionHandler = Received a Region Access\n");
    K2_ASSERT(0);   // want to know
    return (AE_OK);
}

ACPI_STATUS
Acpi_SysIoRegionInit(
    ACPI_HANDLE                 RegionHandle,
    UINT32                      Function,
    void                        *HandlerContext,
    void                        **RegionContext)
{

    ACPI_OBJECT_REGION *    pRegion;

    pRegion = (ACPI_OBJECT_REGION *)RegionHandle;
    Debug_Printf("Acpi_SysIoRegionInit(%d) = Received a region init for %08X, %08X\n", 
            Function,
            (UINT32)pRegion->Address, pRegion->Length);

    if (Function == ACPI_REGION_DEACTIVATE)
    {
        *RegionContext = NULL;
    }
    else
    {
        K2OS_Root_AddIoRange(NULL, (UINT32)(pRegion->Address & 0xFFFFFFFF), pRegion->Length);
        *RegionContext = RegionHandle;
    }

    return (AE_OK);
}

ACPI_STATUS
AcpiOsReadPort(
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width);

ACPI_STATUS
AcpiOsWritePort(
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width);

ACPI_STATUS
Acpi_SysIoRegionHandler(
    UINT32                      Function,
    ACPI_PHYSICAL_ADDRESS       Address,
    UINT32                      BitWidth,
    UINT64                      *Value,
    void                        *HandlerContext,
    void                        *RegionContext)
{
    if (Function == ACPI_WRITE)
    {
//        Debug_Printf("Acpi.IoRegion.Write(%08X, %08X)\n", (UINT32)Address, (UINT32)((*Value) & 0xFFFFFFFF));
        AcpiOsWritePort(Address, (UINT32)((*Value) & 0xFFFFFFFF), BitWidth);
    }
    else
    {
//        Debug_Printf("Acpi.IoRegion.Read(%08X, %08X)\n", (UINT32)Address, (UINT32)((*Value) & 0xFFFFFFFF));
        *Value = 0;
        AcpiOsReadPort(Address, (UINT32 *)Value, BitWidth);
    }
    return (AE_OK);
}

void
Acpi_Handlers_Init1(void)
{
    ACPI_STATUS acpiStatus;

    acpiStatus = AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT,
        ACPI_SYSTEM_NOTIFY, Acpi_SystemNotifyHandler, NULL);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
        ACPI_ADR_SPACE_SYSTEM_MEMORY, Acpi_SysMemoryRegionHandler, Acpi_SysMemoryRegionInit, NULL);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
        ACPI_ADR_SPACE_SYSTEM_IO, Acpi_SysIoRegionHandler, Acpi_SysIoRegionInit, NULL);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));
}

UINT32
Acpi_PowerButtonHandler(
    void *Context
)
{
    Debug_Printf("Acpi_PowerButtonHandler\n");
    AcpiClearEvent(ACPI_EVENT_POWER_BUTTON);
    return (AE_OK);
}

void
Acpi_Handlers_Init2(
    void
)
{
    ACPI_STATUS acpiStatus;

    acpiStatus = AcpiEvInstallXruptHandlers();
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON, Acpi_PowerButtonHandler, NULL);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiEnableEvent(ACPI_EVENT_POWER_BUTTON, 0);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));
}

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

void 
Acpi_Enable(
    void
)
{
    ACPI_STATUS acpiStatus;

    Acpi_Handlers_Init1();

    acpiStatus = AcpiEnableSubsystem(ACPI_NO_HANDLER_INIT); //  (ACPI_FULL_INITIALIZATION);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiLoadTables();
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    Acpi_UseIOAPIC();
}

void
Acpi_Describe(
    ACPI_NODE * apAcpiNode,
    char **     appOut,
    UINT_PTR *  apLeft
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

    ate = K2ASC_PrintfLen(pEnd, left, "ACPI;");
    left -= ate;
    pEnd += ate;

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
        ate = K2ASC_PrintfLen(pEnd, left, ");");
        left -= ate;
        pEnd += ate;
    }
    *appOut = pEnd;
    *apLeft = left;
}

void
Acpi_DiscoveredIrq(
    DEVNODE *       apNode,
    ACPI_RESOURCE * apRes
)
{
    DEV_IRQ *   pIrq;
    UINT32 *    pu;
    UINT32      ix;

    for (ix = 0; ix < apRes->Data.Irq.InterruptCount; ix++)
    {
        Debug_Printf("  [IRQ %d]\n", apRes->Data.Irq.Interrupts[ix]);

        pIrq = (DEV_IRQ *)K2OS_Heap_Alloc(sizeof(DEV_IRQ));
        if (NULL == pIrq)
            return;

        pIrq->DrvResIrq.Config.mSourceIrq = apRes->Data.Irq.Interrupts[ix];
        pIrq->DrvResIrq.Config.mDestCoreIx = 0;
        pIrq->DrvResIrq.Config.Line.mIsEdgeTriggered = apRes->Data.Irq.Triggering ? TRUE : FALSE;
        pIrq->DrvResIrq.Config.Line.mIsActiveLow = apRes->Data.Irq.Polarity ? TRUE : FALSE;
        pIrq->DrvResIrq.Config.Line.mShareConfig = apRes->Data.Irq.Shareable ? TRUE : FALSE;
        pIrq->DrvResIrq.Config.Line.mWakeConfig = apRes->Data.Irq.WakeCapable ? TRUE : FALSE;

        pu = (UINT32 *)&pIrq->DrvResIrq.Config;

        pIrq->Res.mPlatContext = K2OS_Root_AddPlatResource(apNode->InSec.mPlatContext, K2OSDRV_RESTYPE_IRQ, pu[0], pu[1]);
        if (0 == pIrq->Res.mPlatContext)
        {
            Debug_Printf("  [IRQ %d] IGNORED AddPlatResource failed (%08X)\n", apRes->Data.Irq.Interrupts[ix], K2OS_Thread_GetLastStatus());
            K2OS_Heap_Free(pIrq);
        }
        else
        {
            pIrq->Res.mAcpiContext = (UINT_PTR)apRes;
            K2LIST_AddAtTail(&apNode->InSec.IrqResList, &pIrq->Res.ListLink);
        }
    }
}

void
Acpi_DiscoveredDMA(
    DEVNODE *       apNode,
    ACPI_RESOURCE * apRes
)
{
#if 0
    if (apRes->Type == ACPI_RESOURCE_TYPE_DMA)
        Debug_Printf("  DMA(%d chan)\n", apRes->Data.Dma.ChannelCount);
    else
        Debug_Printf("  FIXED_DMA(%d chan)\n", apRes->Data.FixedDma.Channels);
#endif
}

void
Acpi_DiscoveredIo(
    DEVNODE *       apNode,
    ACPI_RESOURCE * apRes
)
{
    DEV_IO *    pIo;
    UINT16      basePort;
    UINT16      sizeBytes;

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
    Debug_Printf("  [IO %04X %04X]\n", basePort, sizeBytes);

    pIo = (DEV_IO *)K2OS_Heap_Alloc(sizeof(DEV_IO));
    if (NULL == pIo)
        return;

    pIo->Res.mPlatContext = K2OS_Root_AddPlatResource(apNode->InSec.mPlatContext, K2OS_PLATRES_IO, basePort, sizeBytes);
    if (0 == pIo->Res.mPlatContext)
    {
        Debug_Printf("  [IO %04X %04X] IGNORED AddPlatResource failed (%08X)\n", basePort, sizeBytes, K2OS_Thread_GetLastStatus());
        K2OS_Heap_Free(pIo);
        return;
    }

    pIo->DrvResIo.mBasePort = basePort;
    pIo->DrvResIo.mSizeBytes = sizeBytes;
    pIo->Res.mAcpiContext = (UINT_PTR)apRes;

    K2LIST_AddAtTail(&apNode->InSec.IoResList, &pIo->Res.ListLink);
}

void
Acpi_AddPhys(
    DEVNODE *   apNode,
    UINT32      aBaseAddr,
    UINT32      aSizeBytes,
    UINT_PTR    aAcpiContext
)
{
    DEV_PHYS *  pPhys;

    Debug_Printf("  [PHYS %08X %08X]\n", aBaseAddr, aSizeBytes);

    if ((0 != (aBaseAddr & K2_VA_MEMPAGE_OFFSET_MASK)) ||
        (0 == aSizeBytes) ||
        (0 != (aSizeBytes & K2_VA_MEMPAGE_OFFSET_MASK)))
    {
        Debug_Printf("  [PHYS %08X %08X] IGNORED BAD ALIGN OR SIZE\n", aBaseAddr, aSizeBytes);
        return;
    }

    pPhys = (DEV_PHYS *)K2OS_Heap_Alloc(sizeof(DEV_PHYS));
    if (NULL == pPhys)
        return;

    pPhys->DrvResPhys.mPageArrayToken = K2OS_Root_CreatePageArrayAt(
        aBaseAddr,
        aSizeBytes / K2_VA_MEMPAGE_BYTES
    );
    if (NULL == pPhys->DrvResPhys.mPageArrayToken)
    {
        Debug_Printf("  [PHYS %08X %08X] IGNORED CreatePageArray failed (%08X)\n", aBaseAddr, aSizeBytes, K2OS_Thread_GetLastStatus());
        K2OS_Heap_Free(pPhys);
        return;
    }

    pPhys->Res.mPlatContext = K2OS_Root_AddPlatResource(apNode->InSec.mPlatContext, K2OSDRV_RESTYPE_PHYS, aBaseAddr, aSizeBytes);
    if (0 == pPhys->Res.mPlatContext)
    {
        Debug_Printf("  [PHYS %08X %08X] IGNORED AddPlatResource failed (%08X)\n", aBaseAddr, aSizeBytes, K2OS_Thread_GetLastStatus());
        K2OS_Token_Destroy(pPhys->DrvResPhys.mPageArrayToken);
        K2OS_Heap_Free(pPhys);
        return;
    }

    pPhys->DrvResPhys.Range.mBaseAddr = aBaseAddr;
    pPhys->DrvResPhys.Range.mSizeBytes = aSizeBytes;
    pPhys->Res.mAcpiContext = aAcpiContext;

    K2LIST_AddAtTail(&apNode->InSec.PhysResList, &pPhys->Res.ListLink);
}

void
Acpi_DiscoveredMemory(
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

    Acpi_AddPhys(apNode, baseAddr, sizeBytes, (UINT_PTR)apRes);
}

void
Acpi_DiscoveredAddress(
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
    Debug_Printf("ADDRESS(%d, %d)\n", apRes->Type, addr64.ResourceType);
    Debug_Printf("  ProducerConsumer %02X\n", addr64.ProducerConsumer);
    Debug_Printf("  Decode           %02X\n", addr64.Decode);
    Debug_Printf("  MinAddressFixed  %02X\n", addr64.MinAddressFixed);
    Debug_Printf("  MaxAddressFixed  %02X\n", addr64.MaxAddressFixed);
    Debug_Printf("  Address.Granularity         %08X%08X\n", (UINT32)((addr64.Address.Granularity >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.Granularity & 0xFFFFFFFFull));
    Debug_Printf("  Address.Minimum             %08X%08X\n", (UINT32)((addr64.Address.Minimum >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.Minimum & 0xFFFFFFFFull));
    Debug_Printf("  Address.Maximum             %08X%08X\n", (UINT32)((addr64.Address.Maximum >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.Maximum & 0xFFFFFFFFull));
    Debug_Printf("  Address.TranslationOffset   %08X%08X\n", (UINT32)((addr64.Address.TranslationOffset >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.TranslationOffset & 0xFFFFFFFFull));
    Debug_Printf("  Address.AddressLength       %08X%08X\n", (UINT32)((addr64.Address.AddressLength >> 32) & 0xFFFFFFFFull), (UINT32)(addr64.Address.AddressLength & 0xFFFFFFFFull));
#endif
}

ACPI_STATUS
Acpi_ResCreateCallback(
    ACPI_RESOURCE * Resource,
    void *          Context
)
{
    DEVNODE *   pDevNode;

    pDevNode = (DEVNODE *)Context;

    switch (Resource->Type)
    {
    case ACPI_RESOURCE_TYPE_IRQ:
        Acpi_DiscoveredIrq(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_DMA:
    case ACPI_RESOURCE_TYPE_FIXED_DMA:
        Acpi_DiscoveredDMA(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_IO:
    case ACPI_RESOURCE_TYPE_FIXED_IO:
        Acpi_DiscoveredIo(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_MEMORY24:
    case ACPI_RESOURCE_TYPE_MEMORY32:
    case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
        Acpi_DiscoveredMemory(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_ADDRESS16:
    case ACPI_RESOURCE_TYPE_ADDRESS32:
    case ACPI_RESOURCE_TYPE_ADDRESS64:
    case ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64:
        Acpi_DiscoveredAddress(pDevNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_END_TAG:
        // ignore
        break;

    default:
        Debug_Printf("!!!!RootAcpiDriver_ResEnumCallback - %d\n", Resource->Type);
        //
        // dont know what to do with this
        //
        K2_ASSERT(0);
    }

    return AE_OK;
}

void
Acpi_NodeLocked_PopulateRes(
    DEVNODE *apNode
)
{
    ACPI_NODE *             pAcpiNode;
    ACPI_STATUS             acpiStatus;
    UINT64                  seg;
    UINT64                  bbn;
    UINT_PTR                sizeEnt;
    UINT_PTR                entCount;
    ACPI_TABLE_MCFG *       pMCFG;
    ACPI_MCFG_ALLOCATION *  pAlloc;
    K2LIST_LINK *           pListLink;
    DEV_IO *                pDevIo;

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
        AcpiWalkResourceBuffer(&pAcpiNode->CurrentAcpiRes, Acpi_ResCreateCallback, (void *)apNode);
    }

    if (pAcpiNode->mpDeviceInfo->Flags & ACPI_PCI_ROOT_BRIDGE)
    {
        //
        // This is a PCI root bridge. Go see if there are PCI CFG physical ranges that need to be added
        // to this node
        //
        apNode->mBusType = K2OSDRV_BUSTYPE_PCI;

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
                    Acpi_AddPhys(apNode, (UINT_PTR)bbn, 0x100000, 0);
                }
            }
        }
    }
    else
    {
        apNode->mBusType = apNode->RefToParent.mpDevNode->mBusType;
    }

    // 
    // permit root process access to resources for the bridge to facilitate ACPI PCI accesses
    // 
    if (0 < apNode->InSec.IoResList.mNodeCount)
    {
        pListLink = apNode->InSec.IoResList.mpHead;
        do {
            pDevIo = K2_GET_CONTAINER(DEV_IO, pListLink, Res.ListLink);
            pListLink = pListLink->mpNext;
            K2OS_Root_AddIoRange(NULL, pDevIo->DrvResIo.mBasePort, pDevIo->DrvResIo.mSizeBytes);
        } while (NULL != pListLink);
    }
}

void
Acpi_DiscoveredSystemBusChild(
    ACPI_NODE * apChildNode
)
{
    DEVNODE *       pChild;
    K2_DEVICE_IDENT ident;
    char *          pIoBuf;
    char *          pOut;
    UINT_PTR        ioBytes;
    UINT_PTR        left;
    BOOL            ok;

    K2_ASSERT(apChildNode->mpParentAcpiNode == gpDevTree->InSec.mpAcpiNode);
    K2_ASSERT(NULL != apChildNode->mpDeviceInfo);
    K2MEM_Zero(&ident, sizeof(ident));

    pChild = Dev_NodeLocked_CreateChildNode(gpDevTree, &ident);
    if (NULL == pChild)
        return;

    ok = FALSE;

    K2OS_CritSec_Enter(&pChild->Sec);

    do {
        pIoBuf = (char *)K2OS_Heap_Alloc(ROOTPLAT_MOUNT_MAX_BYTES);
        if (NULL == pIoBuf)
        {
            Debug_Printf("*** Could not allocate Io buffer for device mount to kernel\n");
            break;
        }
        do {
            pChild->InSec.mpAcpiNode = apChildNode;
            pOut = pIoBuf;
            *pOut = 0;
            left = ROOTPLAT_MOUNT_MAX_BYTES;
            Acpi_Describe(apChildNode, &pOut, &left);

            ioBytes = ((UINT_PTR)(pOut - pIoBuf)) + 1;

            pChild->InSec.mpMountedInfo = (char *)K2OS_Heap_Alloc((ioBytes + 4) & ~3);
            if (NULL == pChild->InSec.mpMountedInfo)
            {
                Debug_Printf("*** Failed to allocate memory for mount info\n");
                break;
            }

            do {
                K2MEM_Copy(pChild->InSec.mpMountedInfo, pIoBuf, ioBytes);
                pChild->InSec.mMountedInfoBytes = ioBytes + 1;
                pChild->InSec.mpMountedInfo[ioBytes] = 0;

                pChild->InSec.mPlatContext = K2OS_Root_CreatePlatNode(
                    gpDevTree->InSec.mPlatContext,
                    apChildNode->mpDeviceInfo->Name,
                    (UINT_PTR)pChild,
                    pIoBuf,
                    ROOTPLAT_MOUNT_MAX_BYTES,
                    &ioBytes);

                if (0 == pChild->InSec.mPlatContext)
                {
                    Debug_Printf("*** Mount of plat node failed (last status 0x%08X)\n", K2OS_Thread_GetLastStatus());
                    break;
                }

                if (ioBytes > 0)
                {
                    K2_ASSERT(ioBytes <= ROOTPLAT_MOUNT_MAX_BYTES);

                    pChild->InSec.mpPlatInfo = (UINT8 *)K2OS_Heap_Alloc((ioBytes + 4) & ~3);
                    if (NULL == pChild->InSec.mpPlatInfo)
                    {
                        Debug_Printf("*** Could not allocate memory to hold node plat output\n");
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
                    K2OS_Root_DeletePlatNode(pChild->InSec.mPlatContext);
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

    Acpi_NodeLocked_PopulateRes(pChild);

    K2OS_CritSec_Leave(&pChild->Sec);
}

void
Acpi_InitNode(
    ACPI_NODE * apAcpiNode,
    ACPI_HANDLE ahObject
);

void
Acpi_EnumCallback(
    ACPI_HANDLE ahParent,
    ACPI_HANDLE ahChild,
    UINT_PTR    aContext
)
{
    ACPI_NODE * pParentAcpiNode;
    ACPI_NODE * pNewChildNode;

    pParentAcpiNode = (ACPI_NODE *)aContext;
    K2_ASSERT(pParentAcpiNode->mhObject == ahParent);

    pNewChildNode = (ACPI_NODE *)K2OS_Heap_Alloc(sizeof(ACPI_NODE));
    if (NULL == pNewChildNode)
    {
        Debug_Printf("***Memory alloc failure creating acpi node\n");
        return;
    }
    K2MEM_Zero(pNewChildNode, sizeof(ACPI_NODE));

    pNewChildNode->mpParentAcpiNode = pParentAcpiNode;
    K2LIST_AddAtTail(&pParentAcpiNode->ChildAcpiNodeList, &pNewChildNode->ParentChildListLink);

    Acpi_InitNode(pNewChildNode, ahChild);
}

void
Acpi_InitNode(
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

    K2OSACPI_EnumChildren(ahObject, Acpi_EnumCallback, (UINT_PTR)apAcpiNode);
}

void
Acpi_SystemBus_AcquireIo(
    ACPI_NODE *     apAcpiNode,
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
    ok = Res_IoHeap_Reserve(&ioInfo, (UINT_PTR)apAcpiNode, FALSE);
    K2_ASSERT(ok);
    ok = K2OS_Root_AddIoRange(NULL, ioInfo.mPortBase, ioInfo.mPortCount);
    K2_ASSERT(ok);
}

void
Acpi_SystemBus_AcquireMemory(
    ACPI_NODE *     apAcpiNode,
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
    ok = Res_PhysHeap_Reserve(&physInfo, (UINT_PTR)apAcpiNode, FALSE);
    K2_ASSERT(ok);
}

ACPI_STATUS
Acpi_SystemBus_AcquireResources(
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
        Acpi_SystemBus_AcquireIo(pAcpiNode, Resource);
        break;

    case ACPI_RESOURCE_TYPE_MEMORY24:
    case ACPI_RESOURCE_TYPE_MEMORY32:
    case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
        Acpi_SystemBus_AcquireMemory(pAcpiNode, Resource);
        break;

    default:
        // ignore
        break;
    }

    return AE_OK;
}

void 
Acpi_StartDriver(
    void
)
{
    K2STAT          stat;
    UINT_PTR        nodeName;
    ACPI_NODE *     pAcpiNode;
    K2LIST_LINK *   pListLink;
    ACPI_STATUS     acpiStatus;
    ACPI_NODE *     pChildAcpiNode;
    UINT_PTR        staFlags;

    //
    // parse the ACPI tree. do not invoke any methods
    //
    Acpi_InitNode(&sgAcpiTreeRoot, ACPI_ROOT_OBJECT);

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
    // now create the root plat node in the kernel
    //
    K2MEM_Copy(&nodeName, "SYS:", 4);
    gpDevTree->mBusType = K2OSDRV_BUSTYPE_CPU;
    gpDevTree->InSec.mPlatContext = K2OS_Root_CreatePlatNode(0, nodeName, (UINT_PTR)gpDevTree, NULL, 0, NULL);
    K2_ASSERT(0 != gpDevTree->InSec.mPlatContext);

    //
    // this will call HookIntr above to install the irq.
    // So the root dev node need to be ready to accept it
    // and have the plat object(s) added, which is why we did that above
    //
    Acpi_Handlers_Init2();

    //
    // enumerate _SB_ resources, adding them to plat and giving root access to them
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
        AcpiWalkResourceBuffer(&pAcpiNode->CurrentAcpiRes, Acpi_SystemBus_AcquireResources, (void *)pAcpiNode);
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
        if (0 != K2ASC_Comp(pChildAcpiNode->mpHardwareId, "PNP0C0F"))
        {
            staFlags = K2OSACPI_RunSTA(pChildAcpiNode->mhObject);
            if ((0 != (staFlags & ACPI_STA_DEVICE_PRESENT)) ||
                (0 != (staFlags & ACPI_STA_DEVICE_FUNCTIONING)))
            {
                Acpi_DiscoveredSystemBusChild(pChildAcpiNode);
            }
        }
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);

    //
    // everything is enumerated/added.  enable the node now
    //
    stat = Dev_NodeLocked_Enable(gpDevTree);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    K2OS_CritSec_Leave(&gpDevTree->Sec);
}

BOOL
Acpi_MatchAndAttachChild(
    ACPI_NODE *     apParentAcpiNode,
    UINT64 const *  apBusSpecificAddress,
    DEVNODE *       apChildNode
)
{
    K2LIST_LINK *   pListLink;
    ACPI_NODE *     pChild;

    K2_ASSERT(NULL != apParentAcpiNode);
    K2_ASSERT(NULL != apBusSpecificAddress);
    K2_ASSERT(NULL != apChildNode);
    K2_ASSERT(NULL == apChildNode->InSec.mpAcpiNode);

    pListLink = apParentAcpiNode->ChildAcpiNodeList.mpHead;
    if (NULL == pListLink)
        return FALSE;

    do {
        pChild = K2_GET_CONTAINER(ACPI_NODE, pListLink, ParentChildListLink);
        pListLink = pListLink->mpNext;

        K2_ASSERT(NULL != pChild->mpDeviceInfo);

        if (pChild->mpDeviceInfo->Address == (*apBusSpecificAddress))
        {
            K2OS_CritSec_Enter(&apChildNode->Sec);

            apChildNode->InSec.mpAcpiNode = pChild;

            K2OS_CritSec_Leave(&apChildNode->Sec);

            return TRUE;
        }

    } while (NULL != pListLink);

    return FALSE;
}

K2STAT
Acpi_RunMethod(
    ACPI_NODE *     apAcpiNode,
    char const *    apMethod,
    BOOL            aHasInput,
    UINT32          aInput,
    UINT32 *        apRetOutput
)
{
    ACPI_STATUS     acpiStatus;
    UINT64          valueOut;

    acpiStatus = K2OSACPI_RunDeviceNumericMethod(
        apAcpiNode->mhObject,
        apMethod,
        &valueOut);

    if (!ACPI_FAILURE(acpiStatus))
    {
        *apRetOutput = (UINT32)valueOut;
        return K2STAT_NO_ERROR;
    }

    return K2STAT_ERROR_NOT_FOUND;
}
