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

EFI_STATUS
EFIAPI
PlatformPeim(
    VOID
)
{
    UINT32  fileBytes;
    UINT32  baseAddr;
    UINT32  packetIx;
    UINT32  packetsLeft;
    UINT32  recvBytes;
    UINT32  packetBytes;
    UINT8 * pPacket;
    UINT8 * pOut;

    volatile VIRTARM_DEBUGADAPTER_REGS * pRegs = (volatile VIRTARM_DEBUGADAPTER_REGS *)PHYS_REGS_ADDR;

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
