/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
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
#include <Ppi/ArmMpCoreInfo.h>

ARM_CORE_INFO mArmPlatformNullMpCoreInfoTable[] = {
  {
    // Cluster 0, Core 0
    0x0, 0x0,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)(VIRTARM_PHYSADDR_RAMBANK + 0x3FF0),
    (EFI_PHYSICAL_ADDRESS)(VIRTARM_PHYSADDR_RAMBANK + 0x3FF0),
    (EFI_PHYSICAL_ADDRESS)(VIRTARM_PHYSADDR_RAMBANK + 0x3FF0),
    (UINT64)0
  },
  {
    // Cluster 0, Core 1
    0x0, 0x1,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)(VIRTARM_PHYSADDR_RAMBANK + 0x3FF8),
    (EFI_PHYSICAL_ADDRESS)(VIRTARM_PHYSADDR_RAMBANK + 0x3FF8),
    (EFI_PHYSICAL_ADDRESS)(VIRTARM_PHYSADDR_RAMBANK + 0x3FF8),
    (UINT64)0
  }
};

/**
  Return the current Boot Mode

  This function returns the boot reason on the platform

**/
EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  )
{
  return BOOT_WITH_FULL_CONFIGURATION;
}

/**
  Initialize controllers that must setup in the normal world

  This function is called by the ArmPlatformPkg/PrePi or ArmPlatformPkg/PlatformPei
  in the PEI phase.

**/
RETURN_STATUS
ArmPlatformInitialize(
    IN UINTN MpId
)
{
    volatile CORTEXA9MP_SCU_REGS * pScu = (volatile CORTEXA9MP_SCU_REGS *)VIRTARM_PHYSADDR_CORTEXA9MP_SCU;
    UINT32 r;

    //
    // invalidate and turn on SCU
    //
    pScu->mInvalidateSecure = 0xFFFFFFFF;
    pScu->mControl = 1;
    pScu->mInvalidateSecure = 0xFFFFFFFF;

    //
    // set SMP mode
    //
    r = ArmReadCpuActlr();
    r |= 0x41;
    ArmWriteCpuActlr(r);
    r = ArmReadCpuActlr();

    if (!ArmPlatformIsPrimaryCore(MpId))
    {
        return RETURN_SUCCESS;
    }

    //
    // should not be needed but we do it anyway
    //
    pScu->mAccessControl = 0xF;
    pScu->mNonSecureAccessControl = 0xFFF;

    return RETURN_SUCCESS;
}

EFI_STATUS
PrePeiCoreGetMpCoreInfo (
  OUT UINTN                   *CoreCount,
  OUT ARM_CORE_INFO           **ArmCoreTable
  )
{
  if (ArmIsMpCore()) {
    *CoreCount    = sizeof(mArmPlatformNullMpCoreInfoTable) / sizeof(ARM_CORE_INFO);
    *ArmCoreTable = mArmPlatformNullMpCoreInfoTable;
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
}

ARM_MP_CORE_INFO_PPI mMpCoreInfoPpi = { PrePeiCoreGetMpCoreInfo };

EFI_PEI_PPI_DESCRIPTOR      gPlatformPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gArmMpCoreInfoPpiGuid,
    &mMpCoreInfoPpi
  }
};

VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  if (ArmIsMpCore()) {
    *PpiListSize = sizeof(gPlatformPpiTable);
    *PpiList = gPlatformPpiTable;
  } else {
    *PpiListSize = 0;
    *PpiList = NULL;
  }
}
