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

#include "Sec.h"

UINT32
SecondaryTrustedMonitorInit(
    UINT32 StackPointer,
    UINT32 SVC_SPSR,
    UINT32 MON_SPSR,
    UINT32 MON_CPSR
)
{
    //
    // Transfer all interrupts to Non-secure World
    //
    ArmGicSetupNonSecure(ArmReadMpidr() & 3, (INTN)PcdGet64(PcdGicDistributorBase), (INTN)PcdGet64(PcdGicInterruptInterfaceBase));

    //
    // This is where to go when you exit secure mode
    //
    return (UINT32)PcdGet64(PcdFvBaseAddress);
}

UINT32
VirtualArmSecondaryCoreSecStart(
    UINT32 aMpCoreId,
    UINT32 aInitStackPointer
)
{
    UINT32 BaseAddress;

    //
    // Enable SWP instructions in secure state
    //
    ArmEnableSWPInstruction();

    //
    // Get Snoop Control Unit base address and enable it
    // Allow NS access to SCU register
    // Allow NS access to Private Peripherals
    //
    BaseAddress = ArmGetScuBaseAddress();
    MmioOr32(BaseAddress + A9_SCU_SACR_OFFSET, 0xf);
    MmioOr32(BaseAddress + A9_SCU_SSACR_OFFSET, 0xfff);

    //
    // Enable this cpu's interrupt interface
    //
    ArmGicEnableInterruptInterface((INTN)PcdGet64(PcdGicInterruptInterfaceBase));

    //
    // Enable Full Access to CoProcessors
    //
    ArmWriteCpacr(CPACR_CP_FULL_ACCESS);

    // Enter Trusted Monitor for setup there
    return (UINT32)SecondaryTrustedMonitorInit;
}

