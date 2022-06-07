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
#include <Library/PrePiLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/VADebugAdapterLib.h>
#include <Library/BaseMemoryLib.h>

#define PHYS_REGS_ADDR VIRTARM_PHYSADDR_ADAPTER_REGS(FixedPcdGet32(PcdDebugAdapterSlotNumber))

#define DISTREG(x)  (*((UINT32 *)(VIRTARM_PHYSADDR_CORTEXA9MP_INTDIST + (x))))
#define PROCREG(x)  (*((UINT32 *)(VIRTARM_PHYSADDR_CORTEXA9MP_PERCPUINT + (x))))

EFI_STATUS
EFIAPI
PlatformPeim(
    VOID
)
{
//    UINT32 volatile * pu = (UINT32 volatile *)VIRTARM_PHYSADDR_CORTEXA9MP_INTDIST;

    UINT32  fileBytes;
    UINT32  baseAddr;
    UINT32  packetIx;
    UINT32  packetsLeft;
    UINT32  recvBytes;
    UINT32  packetBytes;
    UINT8 * pPacket;
    UINT8 * pOut;

    volatile VIRTARM_DEBUGADAPTER_REGS * pRegs = (volatile VIRTARM_DEBUGADAPTER_REGS *)PHYS_REGS_ADDR;

    UINT32 ix;

    /* disable IRQs on the distributor and the per-CPU interface */
    DISTREG(CORTEXA9MP_INTDIST_REGS_OFFSET_ICDDCR) = 0;
    PROCREG(CORTEXA9MP_PERCPUINT_REGS_OFFSET_ICCICR) = 0;

    for (ix = 0; ix < 8; ix++)
    {
        DISTREG(CORTEXA9MP_INTDIST_REGS_OFFSET_ICDICER_0_OF_8 + (4 * ix)) = 0xFFFFFFFF;
    }

    for (ix = 0; ix < 8; ix++)
    {
        DISTREG(CORTEXA9MP_INTDIST_REGS_OFFSET_ICDICPR_0_OF_8 + (4 * ix)) = 0xFFFFFFFF;
    }

    for (ix = 0; ix < 64; ix++)
    {
        DISTREG(CORTEXA9MP_INTDIST_REGS_OFFSET_ICDIPR_0_OF_64 + (4 * ix)) = 0;
    }
    
    for (ix = 0; ix < 16; ix++)
    {
        DISTREG(CORTEXA9MP_INTDIST_REGS_OFFSET_ICDICR_0_OF_16 + (4 * ix)) = 0;
    }

    for (ix = 4; ix < 64; ix++)
    {
        DISTREG(CORTEXA9MP_INTDIST_REGS_OFFSET_ICDIPTR_0_OF_64 + (4 * ix)) = 0x01010101;
    }

    ix = 3 | (3 << 8) | (3 << 16) | (3 << 24);
    DISTREG(CORTEXA9MP_INTDIST_REGS_OFFSET_ICDIPTR_0_OF_64 + 0) = ix;
    DISTREG(CORTEXA9MP_INTDIST_REGS_OFFSET_ICDIPTR_0_OF_64 + 4) = ix;
    DISTREG(CORTEXA9MP_INTDIST_REGS_OFFSET_ICDIPTR_0_OF_64 + 8) = ix;
    DISTREG(CORTEXA9MP_INTDIST_REGS_OFFSET_ICDIPTR_0_OF_64 + 12) = ix;

    for (ix = 0; ix < 8; ix++)
    {
        DISTREG(CORTEXA9MP_INTDIST_REGS_OFFSET_ICDISR_0_OF_8 + (4 * ix)) = 0xFFFFFFFF;
    }

    PROCREG(CORTEXA9MP_PERCPUINT_REGS_OFFSET_ICCPMR) = 0xFF;

    PROCREG(CORTEXA9MP_PERCPUINT_REGS_OFFSET_ICCICR) = 3;
    DISTREG(CORTEXA9MP_INTDIST_REGS_OFFSET_ICDDCR) = 3;




    //
    // FV that includes code executing now
    //
    BuildFvHob(PcdGet64(PcdFvBaseAddress), PcdGet32(PcdFvSize));

    ASSERT(VIRTARM_DEBUGADAPTER_VID == pRegs->mVID);
    ASSERT(VIRTARM_DEBUGADAPTER_PID == pRegs->mPID);

    pRegs->mIOCommand = VIRTARM_DEBUGADAPTER_IOCMD_GETAUXBYTES;
    fileBytes = pRegs->mIOArgument;
    ASSERT(0 != fileBytes);

    baseAddr = (UINT32)AllocatePages(EFI_SIZE_TO_PAGES(fileBytes));
    ASSERT(baseAddr != 0);
    pOut = (UINT8 *)baseAddr;

    packetsLeft = (fileBytes + (VIRTARM_DEBUGADAPTER_PACKET_MAX_BYTES - 1)) / VIRTARM_DEBUGADAPTER_PACKET_MAX_BYTES;
    packetIx = 0;
    do {
        pRegs->mIOArgument = packetIx++;
        pRegs->mIOCommand = VIRTARM_DEBUGADAPTER_IOCMD_PUSHAUXPACKET;
        recvBytes = pRegs->mIOArgument;
        ASSERT(0 != recvBytes);
        //
        // should be a packet available to receive
        //
        ASSERT(VIRTARMDEBUG_IsRecvPacketAvail(pRegs));

        packetBytes = 0;
        pPacket = VIRTARMDEBUG_GetRecvPacketBuffer(pRegs, &packetBytes);
        ASSERT(packetBytes == recvBytes);

        CopyMem(pOut, pPacket, packetBytes);
        pOut += packetBytes;

        recvBytes = VIRTARMDEBUG_ReleaseBuffer(pRegs, pPacket);
        ASSERT(0 == recvBytes);

    } while (--packetsLeft);

    //
    // Aux FV that includes compressed FV
    //
    BuildFvHob(baseAddr, fileBytes);

    return EFI_SUCCESS;
}
