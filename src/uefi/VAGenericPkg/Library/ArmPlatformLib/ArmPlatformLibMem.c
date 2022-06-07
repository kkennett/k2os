/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/PrePiLib.h>
#include <VAGeneric.h>

#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  (5 + (32*2) + 1)

VOID
ArmPlatformGetVirtualMemoryMap(
    IN ARM_MEMORY_REGION_DESCRIPTOR** VirtualMemoryMap
)
{
    //
    // PLATFORM IS IN PHYSICAL MODE WITH MMU OFF
    //
    UINTN                           Index = 0;
    UINT32                          workAddr;
    UINTN                           ixAdapter;
    ARM_MEMORY_REGION_DESCRIPTOR  * VirtualMemoryTable;

    ASSERT(VirtualMemoryMap != NULL);

    VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR*)AllocatePages(EFI_SIZE_TO_PAGES(sizeof(ARM_MEMORY_REGION_DESCRIPTOR) * MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS));
    if (VirtualMemoryTable == NULL)
        return;

    // BOOT ROM
    VirtualMemoryTable[Index].PhysicalBase = VIRTARM_PHYSADDR_BOOTROM;
    VirtualMemoryTable[Index].VirtualBase = VIRTARM_PHYSADDR_BOOTROM;
    VirtualMemoryTable[Index].Length = VIRTARM_PHYSSIZE_BOOTROM;
    VirtualMemoryTable[Index].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED;
    ++Index;

    // Cortext A9 PERIPHBASE
    VirtualMemoryTable[Index].PhysicalBase = VIRTARM_PHYSADDR_PERIPHBASE;
    VirtualMemoryTable[Index].VirtualBase = VIRTARM_PHYSADDR_PERIPHBASE;
    VirtualMemoryTable[Index].Length = VIRTATM_PHYSSIZE_PERIPHBASE;
    VirtualMemoryTable[Index].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    ++Index;

    // Program Counters
    VirtualMemoryTable[Index].PhysicalBase = VIRTARM_PHYSADDR_PROGCOUNTERS;
    VirtualMemoryTable[Index].VirtualBase = VIRTARM_PHYSADDR_PROGCOUNTERS;
    VirtualMemoryTable[Index].Length = VIRTARM_PHYSSIZE_ALL_PROGCOUNTERS;
    VirtualMemoryTable[Index].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    ++Index;

    // SP804 Timers
    VirtualMemoryTable[Index].PhysicalBase = VIRTARM_PHYSADDR_SP804_TIMERS;
    VirtualMemoryTable[Index].VirtualBase = VIRTARM_PHYSADDR_SP804_TIMERS;
    VirtualMemoryTable[Index].Length = VIRTARM_PHYSSIZE_ALL_SP804_TIMERS;
    VirtualMemoryTable[Index].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    ++Index;

    // DRAM
    VirtualMemoryTable[Index].PhysicalBase = VIRTARM_PHYSADDR_RAMBANK;
    VirtualMemoryTable[Index].VirtualBase = VIRTARM_PHYSADDR_RAMBANK;
    VirtualMemoryTable[Index].Length = PcdGet32(PcdRamPartSize);
    VirtualMemoryTable[Index].Attributes = DDR_ATTRIBUTES_CACHED;
    ++Index;

    // Adapter Slots
    workAddr = VIRTARM_PHYSADDR_ADAPTERSLOTS;
    for (ixAdapter = 0; ixAdapter < 32; ixAdapter++)
    {
        //
        // registers
        //
        VirtualMemoryTable[Index].PhysicalBase = workAddr + VIRTARM_OFFSET_ADAPTER_REGS;
        VirtualMemoryTable[Index].VirtualBase = VirtualMemoryTable[Index].PhysicalBase;
        VirtualMemoryTable[Index].Length = VIRTARM_PHYSSIZE_ADAPTER_REGS;
        VirtualMemoryTable[Index].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED;
        ++Index;

        //
        // sram
        //
        VirtualMemoryTable[Index].PhysicalBase = workAddr + VIRTARM_OFFSET_ADAPTER_SRAM;
        VirtualMemoryTable[Index].VirtualBase = VirtualMemoryTable[Index].PhysicalBase;
        VirtualMemoryTable[Index].Length = VIRTARM_PHYSSIZE_ADAPTER_SRAM;
        VirtualMemoryTable[Index].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED;
        ++Index;

        workAddr += VIRTARM_PHYSSIZE_ONE_ADAPTERSLOT;
    }

    // End of Table
    VirtualMemoryTable[Index].PhysicalBase = 0;
    VirtualMemoryTable[Index].VirtualBase = 0;
    VirtualMemoryTable[Index].Length = 0;
    VirtualMemoryTable[Index].Attributes = (ARM_MEMORY_REGION_ATTRIBUTES)0;
    ++Index;

    ASSERT(Index == MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);

    *VirtualMemoryMap = VirtualMemoryTable;
}
