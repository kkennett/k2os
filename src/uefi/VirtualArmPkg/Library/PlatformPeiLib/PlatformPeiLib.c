/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
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

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/VADiskAdapterLib.h>

#define PHYS_REGS_ADDR VIRTARM_PHYSADDR_ADAPTER_REGS(FixedPcdGet32(PcdDiskAdapterSlotNumber))

EFI_STATUS
EFIAPI
PlatformPeim(
    VOID
)
{
    volatile VIRTARM_DISKADAPTER_REGS * pRegs = (volatile VIRTARM_DISKADAPTER_REGS *)PHYS_REGS_ADDR;

    DebugPrint(0xFFFFFFFF, "+PlatformPeim()\n");

    VIRTARMDISK_Init(pRegs);

    ASSERT(0x45E == pRegs->mVID);
    ASSERT(0x606 == pRegs->mPID);
    ASSERT(0 != VIRTARMDISK_GetMaxNumSectors(pRegs));

    BuildFvHob(PcdGet64(PcdFvBaseAddress), PcdGet32(PcdFvSize));

    DebugPrint(0xFFFFFFFF, "-PlatformPeim()\n");

    return EFI_SUCCESS;
}
