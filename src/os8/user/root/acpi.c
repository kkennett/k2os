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

void
RootAcpi_ReserveBuiltInIo(
    UINT16  aBase,
    UINT16  aCount
)
{
    K2OS_IOADDR_INFO    ioInfo;
    BOOL                ok;

    ioInfo.mPortBase = aBase;
    ioInfo.mPortCount = aCount;
    ok = RootRes_IoHeap_Reserve(&ioInfo, 1, TRUE);
    K2_ASSERT(ok);

    ok = K2OS_Root_AddIoRange(NULL, aBase, aCount);
    K2_ASSERT(ok);
}

K2OS_INTERRUPT_TOKEN
K2_CALLCONV_REGS
RootAcpi_HookIntr(
    K2OS_IRQ_CONFIG const *apConfig
)
{
    UINT32 *    pu;

    //
    // acpi interrupts mount on the root of the tree
    //
    pu = (UINT32 *)apConfig;
    if (!K2OS_Root_AddPlatResource(gRootDevNode.InSec.mPlatContext, K2OS_PLATRES_IRQ, pu[0], pu[1]))
    {
        return NULL;
    }

    return K2OS_Root_HookPlatIntr(gRootDevNode.InSec.mPlatContext, apConfig->mSourceIrq);
}

void
RootAcpi_Init(
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

    acpiInit.mfHookIntr = RootAcpi_HookIntr;

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
        RootAcpi_ReserveBuiltInIo(pFADT->SmiCommand, sizeof(UINT32));
    }
    if ((0 != pFADT->Pm1aEventBlock) && (0 != pFADT->Pm1EventLength))
    {
        RootAcpi_ReserveBuiltInIo(pFADT->Pm1aEventBlock, pFADT->Pm1EventLength);
    }
    if ((0 != pFADT->Pm1aControlBlock) && (0 != pFADT->Pm1ControlLength))
    {
        RootAcpi_ReserveBuiltInIo(pFADT->Pm1aControlBlock, pFADT->Pm1ControlLength);
    }
    if ((0 != pFADT->Pm2ControlBlock) && (0 != pFADT->Pm2ControlLength))
    {
        RootAcpi_ReserveBuiltInIo(pFADT->Pm2ControlBlock, pFADT->Pm2ControlLength);
    }
    if ((0 != pFADT->PmTimerBlock) && (0 != pFADT->PmTimerLength))
    {
        RootAcpi_ReserveBuiltInIo(pFADT->PmTimerBlock, pFADT->PmTimerLength);
    }
    if ((0 != pFADT->Gpe0Block) && (0 != pFADT->Gpe0BlockLength))
    {
        RootAcpi_ReserveBuiltInIo(pFADT->Gpe0Block, pFADT->Gpe0BlockLength);
    }
    if ((0 != pFADT->Gpe1Block) && (0 != pFADT->Gpe1BlockLength))
    {
        RootAcpi_ReserveBuiltInIo(pFADT->Gpe1Block, pFADT->Gpe1BlockLength);
    }
}

void
RootAcpi_SystemNotifyHandler(
    ACPI_HANDLE                 Device,
    UINT32                      Value,
    void                        *Context)
{
    RootDebug_Printf("RootAcpi_NotifyHandler = Received a Notify %08X\n", Value);
    K2_ASSERT(0);   // want to know
}

ACPI_STATUS
RootAcpi_SysMemoryRegionInit(
    ACPI_HANDLE                 RegionHandle,
    UINT32                      Function,
    void                        *HandlerContext,
    void                        **RegionContext)
{
    RootDebug_Printf("RootAcpi_SysMemoryRegionInit = Received a region init for %08X\n", RegionHandle);
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
RootAcpi_SysMemoryRegionHandler(
    UINT32                      Function,
    ACPI_PHYSICAL_ADDRESS       Address,
    UINT32                      BitWidth,
    UINT64                      *Value,
    void                        *HandlerContext,
    void                        *RegionContext)
{
    RootDebug_Printf("RootAcpi_SysMemoryRegionHandler = Received a Region Access\n");
    K2_ASSERT(0);   // want to know
    return (AE_OK);
}

ACPI_STATUS
RootAcpi_SysIoRegionInit(
    ACPI_HANDLE                 RegionHandle,
    UINT32                      Function,
    void                        *HandlerContext,
    void                        **RegionContext)
{

    ACPI_OBJECT_REGION *    pRegion;

    pRegion = (ACPI_OBJECT_REGION *)RegionHandle;
//    RootDebug_Printf("RootAcpi_SysIoRegionInit(%d) = Received a region init for %08X, %08X\n", 
//            Function,
//            (UINT32)pRegion->Address, pRegion->Length);

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
RootAcpi_SysIoRegionHandler(
    UINT32                      Function,
    ACPI_PHYSICAL_ADDRESS       Address,
    UINT32                      BitWidth,
    UINT64                      *Value,
    void                        *HandlerContext,
    void                        *RegionContext)
{
    if (Function == ACPI_WRITE)
    {
//        RootDebug_Printf("Acpi.IoRegion.Write(%08X, %08X)\n", (UINT32)Address, (UINT32)((*Value) & 0xFFFFFFFF));
        AcpiOsWritePort(Address, (UINT32)((*Value) & 0xFFFFFFFF), BitWidth);
    }
    else
    {
//        RootDebug_Printf("Acpi.IoRegion.Read(%08X, %08X)\n", (UINT32)Address, (UINT32)((*Value) & 0xFFFFFFFF));
        *Value = 0;
        AcpiOsReadPort(Address, (UINT32 *)Value, BitWidth);
    }
    return (AE_OK);
}

void
RootAcpi_Handlers_Init1(void)
{
    ACPI_STATUS acpiStatus;

    acpiStatus = AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT,
        ACPI_SYSTEM_NOTIFY, RootAcpi_SystemNotifyHandler, NULL);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
        ACPI_ADR_SPACE_SYSTEM_MEMORY, RootAcpi_SysMemoryRegionHandler, RootAcpi_SysMemoryRegionInit, NULL);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
        ACPI_ADR_SPACE_SYSTEM_IO, RootAcpi_SysIoRegionHandler, RootAcpi_SysIoRegionInit, NULL);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));
}

UINT32
RootAcpi_PowerButtonHandler(
    void *Context
)
{
    RootDebug_Printf("RootAcpi_PowerButtonHandler\n");
    AcpiClearEvent(ACPI_EVENT_POWER_BUTTON);
    return (AE_OK);
}

void
RootAcpi_Handlers_Init2(
    void
)
{
    ACPI_STATUS acpiStatus;

    acpiStatus = AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON, RootAcpi_PowerButtonHandler, NULL);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiEnableEvent(ACPI_EVENT_POWER_BUTTON, 0);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));
}

void
RootAcpi_UseIOAPIC(
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
RootAcpi_Enable(
    void
)
{
    ACPI_STATUS acpiStatus;

    RootAcpi_Handlers_Init1();

    acpiStatus = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiLoadTables();
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    acpiStatus = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    K2_ASSERT(!ACPI_FAILURE(acpiStatus));

    RootAcpi_UseIOAPIC();

    RootAcpi_Handlers_Init2();
}

