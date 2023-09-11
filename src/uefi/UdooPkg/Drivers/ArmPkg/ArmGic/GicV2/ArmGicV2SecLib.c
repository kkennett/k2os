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

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/ArmGicLib.h>

UINTN
ArmPlatformIsPrimaryCore(
    IN UINTN MpId
);

/*
 * This function configures the all interrupts to be Non-secure.
 *
 */
VOID
EFIAPI
ArmGicV2SetupNonSecure (
  IN  UINTN         MpId,
  IN  INTN          GicDistributorBase,
  IN  INTN          GicInterruptInterfaceBase
  )
{
  UINTN InterruptId;
  UINTN CachedPriorityMask;
  UINTN Index;
  UINTN MaxInterrupts;

  CachedPriorityMask = MmioRead32 (GicInterruptInterfaceBase + ARM_GIC_ICCPMR);

  // Set priority Mask so that no interrupts get through to CPU
  MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCPMR, 0);

  InterruptId   = MmioRead32 (GicInterruptInterfaceBase + ARM_GIC_ICCIAR);
  MaxInterrupts = ArmGicGetMaxNumInterrupts (GicDistributorBase);

  // Only try to clear valid interrupts. Ignore spurious interrupts.
  while ((InterruptId & 0x3FF) < MaxInterrupts) {
    // Some of the SGI's are still pending, read Ack register and send End of Interrupt Signal
    ArmGicEndOfInterrupt (GicInterruptInterfaceBase, InterruptId);

    // Next
    InterruptId = MmioRead32 (GicInterruptInterfaceBase + ARM_GIC_ICCIAR);
  }

  // Only the primary core should set the Non Secure bit to the SPIs (Shared Peripheral Interrupt).
  if (ArmPlatformIsPrimaryCore (MpId)) {
    // Ensure all GIC interrupts are Non-Secure
    for (Index = 0; Index < (MaxInterrupts / 32); Index++) {
      MmioWrite32 (GicDistributorBase + ARM_GIC_ICDISR + (Index * 4), 0xffffffff);
    }
  } else {
    // The secondary cores only set the Non Secure bit to their banked PPIs
    MmioWrite32 (GicDistributorBase + ARM_GIC_ICDISR, 0xffffffff);
  }

  // Ensure all interrupts can get through the priority mask
  MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCPMR, CachedPriorityMask);
}

VOID
EFIAPI
ArmGicV2EnableInterruptInterface (
  IN  INTN          GicInterruptInterfaceBase
  )
{
  // Set Priority Mask to allow interrupts
  MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCPMR, 0x000000FF);

  // Enable CPU interface in Secure world
  // Enable CPU interface in Non-secure World
  // Signal Secure Interrupts to CPU using FIQ line *
  MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCICR,
      ARM_GIC_ICCICR_ENABLE_SECURE |
      ARM_GIC_ICCICR_ENABLE_NS |
      ARM_GIC_ICCICR_SIGNAL_SECURE_TO_FIQ);
}

VOID
EFIAPI
ArmGicV2DisableInterruptInterface (
  IN  INTN          GicInterruptInterfaceBase
  )
{
  UINT32    ControlValue;

  // Disable CPU interface in Secure world and Non-secure World
  ControlValue = MmioRead32 (GicInterruptInterfaceBase + ARM_GIC_ICCICR);
  MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCICR, ControlValue & ~(ARM_GIC_ICCICR_ENABLE_SECURE | ARM_GIC_ICCICR_ENABLE_NS));
}
