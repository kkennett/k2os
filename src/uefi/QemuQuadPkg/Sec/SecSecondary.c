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

#include "Sec.h"

typedef void (*pfEntryFunc)(void);

void
SecondaryCoreInNonSecure(
    void
)
{
    UINT32      coreIx;
    UINT32      corePark;
    pfEntryFunc entryFunc;
    UINTN       intrId;
    UINTN       ackIntr;

    // Data Cache should be off but we set it disable and invalidate it anyway
    ArmDisableDataCache();
    ArmInvalidateDataCache();

    // Instruction cache should be off but we invalidate it anyway then enable it
    ArmInvalidateInstructionCache();
    ArmEnableInstructionCache();
    // Enable program flow prediction, if supported.
    ArmEnableBranchPrediction();

    // Enable SWP instructions in NonSecure State
    ArmEnableSWPInstruction();

    // Enable Full Access to CoProcessors
    ArmWriteCpacr(CPACR_CP_FULL_ACCESS);

    coreIx = ArmReadMpidr() & 0x3;
    corePark = (QEMUQUAD_PHYSADDR_PERCPU_PAGES + (0x1000 * coreIx));

//    DebugPrint(0xFFFFFFFF, "SecondaryCoreInNonSecure(%d)\n", coreIx);
//    DebugPrint(0xFFFFFFFF, "Secondary SCTLR  %08X\n", ArmReadSctlr());
//    DebugPrint(0xFFFFFFFF, "Secondary ACTLR  %08X\n", ArmReadAuxCr());
//    DebugPrint(0xFFFFFFFF, "Secondary NSACR  %08X\n", ArmReadNsacr());
//    DebugPrint(0xFFFFFFFF, "Secondary CPACR  %08X\n", ArmReadCpacr());

    MmioWrite32(corePark, 0);

    do {
        ArmCallWFI();

        // Read the Mailbox
        entryFunc = (pfEntryFunc)MmioRead32(corePark);

        // Acknowledge the interrupt and send End of Interrupt signal.
        ackIntr = ArmGicAcknowledgeInterrupt(PcdGet64(PcdGicInterruptInterfaceBase), &intrId);

        // Check if it is a valid interrupt ID
        if (intrId < ArmGicGetMaxNumInterrupts(PcdGet64(PcdGicDistributorBase))) 
        {
            // Got a valid SGI number hence signal End of Interrupt
            ArmGicEndOfInterrupt(PcdGet64(PcdGicInterruptInterfaceBase), ackIntr);
        }
    } while (NULL == entryFunc);

    // Jump to entry point.
    entryFunc();

    // Should never return from that
//    ASSERT(FALSE);

    while (1);
}

UINT32
SecondaryTrustedMonitorInit(
    UINT32 StackPointer,
    UINT32 SVC_SPSR,
    UINT32 MON_SPSR,
    UINT32 MON_CPSR
)
{
#if SLOW_SECONDARY_CORE_STARTUP
    UINT32 ScuAddr;

    // slow down starting of secondar cores to see these messages
    // without having one core stomp on another's debug prints.
    // we can't use exclusives to sync because caches are off
//    DebugPrint(0xFFFFFFFF, "SCTLR = 0x%08X\n", ArmReadSctlr());
//    DebugPrint(0xFFFFFFFF, "%08X = %08X\n", IMX6_PHYSADDR_PL310, MmioRead32(IMX6_PHYSADDR_PL310));
    //DebugPrint(0xFFFFFFFF, "%08X = %08X\n", IMX6_PHYSADDR_PL310 + 0x100, MmioRead32(IMX6_PHYSADDR_PL310 + 0x100));
    DebugPrint(0xFFFFFFFF, "\nSecondary Trusted Monitor Init:\n");
    DebugPrint(0xFFFFFFFF, "Secondary SCTLR        0x%08X\n", ArmReadSctlr());
    DebugPrint(0xFFFFFFFF, "Secondary StackPointer 0x%08X\n", StackPointer);
    DebugPrint(0xFFFFFFFF, "Secondary SVC_SPSR     0x%08X\n", SVC_SPSR);
    DebugPrint(0xFFFFFFFF, "Secondary MON_SPSR     0x%08X\n", MON_SPSR);
    DebugPrint(0xFFFFFFFF, "Secondary MON_CPSR     0x%08X\n", MON_CPSR);
    DebugPrint(0xFFFFFFFF, "Secondary ACTLR  %08X\n", ArmReadAuxCr());
    DebugPrint(0xFFFFFFFF, "Secondary SCR    %08X\n", ArmReadScr());
    DebugPrint(0xFFFFFFFF, "Secondary NSACR  %08X\n", ArmReadNsacr());

    ScuAddr = ArmGetScuBaseAddress();

    DebugPrint(0xFFFFFFFF, "SCU.CONTROL %08X\n", MmioRead32(ScuAddr + A9_SCU_CONTROL_OFFSET));
    DebugPrint(0xFFFFFFFF, "SCU.CONFIG  %08X\n", MmioRead32(ScuAddr + A9_SCU_CONFIG_OFFSET));
    DebugPrint(0xFFFFFFFF, "SCU.PWRSTAT %08X\n", MmioRead32(ScuAddr + A9_SCU_POWERSTAT_OFFSET));
    DebugPrint(0xFFFFFFFF, "SCU.FILT_S  %08X\n", MmioRead32(ScuAddr + A9_SCU_FILT_START_OFFSET));
    DebugPrint(0xFFFFFFFF, "SCU.FILT_E  %08X\n", MmioRead32(ScuAddr + A9_SCU_FILT_END_OFFSET));
    DebugPrint(0xFFFFFFFF, "SCU.SACR    %08X\n", MmioRead32(ScuAddr + A9_SCU_SACR_OFFSET));
    DebugPrint(0xFFFFFFFF, "SCU.SSACR   %08X\n", MmioRead32(ScuAddr + A9_SCU_SSACR_OFFSET));
#endif

    //
    // Transfer all interrupts to this CPU to the Non-secure World
    //
    ArmGicSetupNonSecure(ArmReadMpidr() & 3, (INTN)PcdGet64(PcdGicDistributorBase), (INTN)PcdGet64(PcdGicInterruptInterfaceBase));

    //
    // return where to go when you exit secure mode
    //
    return (UINT32)SecondaryCoreInNonSecure;
}

UINT32
QemuQuadSecondaryCoreSecStart(
    UINT32 aMpCoreId,
    UINT32 aInitStackPointer
)
{
    UINT32 BaseAddress;

#if SLOW_SECONDARY_CORE_STARTUP
	DebugPrint(0xFFFFFFFF, "Secure SecondaryCoreStart(%d, %08X)\n", aMpCoreId, aInitStackPointer);
#endif

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

