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

#include <virtarm.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SerialPortLib.h>
#include <Library/VADebugAdapterLib.h>

#define USE_MSG_BUFFER  1

#define PHYS_REGS_ADDR      VIRTARM_PHYSADDR_ADAPTER_REGS(FixedPcdGet32(PcdDebugAdapterSlotNumber))

RETURN_STATUS
EFIAPI
SerialPortInitialize(
    VOID
)
{
    VIRTARMDEBUG_Init((volatile VIRTARM_DEBUGADAPTER_REGS *)PHYS_REGS_ADDR, 
#if USE_MSG_BUFFER
        TRUE,
#else
        FALSE,
#endif
        FALSE);
    return EFI_SUCCESS;
}

#if USE_MSG_BUFFER
UINTN
EFIAPI
SerialPortWrite(
    IN  UINT8 * Buffer,
    IN  UINTN   NumberOfBytes
)
{
    volatile VIRTARM_DEBUGADAPTER_REGS *pRegs;
    UINT8 *                             pTarget;

    if (0 == NumberOfBytes)
        return 0;

    pRegs = (volatile VIRTARM_DEBUGADAPTER_REGS *)PHYS_REGS_ADDR;

    if (!(pRegs->mControlAndStatus & VIRTARM_DEBUGADAPTER_CTRLSTAT_CONSOLE_PRESENT))
        return 0;

    if (NumberOfBytes > 2047)
        NumberOfBytes = 2047;

    pTarget = (UINT8 *)pRegs;
    pTarget -= VIRTARM_OFFSET_ADAPTER_REGS;
    pTarget += VIRTARM_OFFSET_ADAPTER_SRAM;
    pTarget += VIRTARM_DEBUGADAPTER_SRAM_OFFSET_MESSAGEBUFFER;

    CopyMem(pTarget, Buffer, NumberOfBytes);

    pRegs->mDebugOut = NumberOfBytes;

    return NumberOfBytes;
}

#else
UINTN
EFIAPI
SerialPortWrite(
    IN  UINT8 * Buffer,
    IN  UINTN   NumberOfBytes
)
{
    CHAR16 char16[2];
    UINT32 left;

    if (0 == NumberOfBytes)
        return 0;

    left = NumberOfBytes;
    char16[1] = 0;
    do {
        char16[0] = *Buffer;
        VIRTARMDEBUG_OutputString((volatile VIRTARM_DEBUGADAPTER_REGS *)PHYS_REGS_ADDR, char16, 1);
        Buffer++;
    } while (--left);

    return NumberOfBytes;
}
#endif

UINTN
EFIAPI
SerialPortRead(
    OUT UINT8 * Buffer,
    IN  UINTN   NumberOfBytes
)
{
    UINT32 inCount;
    UINT32 left;

    if (0 == NumberOfBytes)
        return 0;

    inCount = ((volatile VIRTARM_DEBUGADAPTER_REGS *)PHYS_REGS_ADDR)->mDebugInCount;
    if (0 == inCount)
        return 0;

    left = inCount;
    do {
        *Buffer = (UINT8)(((volatile VIRTARM_DEBUGADAPTER_REGS *)PHYS_REGS_ADDR)->mDebugIn);
        Buffer++;
    } while (--left);

    return inCount;
}

BOOLEAN
EFIAPI
SerialPortPoll(
    VOID
)
{
    if (((volatile VIRTARM_DEBUGADAPTER_REGS *)PHYS_REGS_ADDR)->mDebugInCount)
        return TRUE;
    return FALSE;
}

RETURN_STATUS
EFIAPI
SerialPortSetControl(
    IN UINT32 Control
)
{
    return RETURN_UNSUPPORTED;
}

RETURN_STATUS
EFIAPI
SerialPortGetControl(
    OUT UINT32 *Control
)
{
    return RETURN_UNSUPPORTED;
}

RETURN_STATUS
EFIAPI
SerialPortSetAttributes(
    IN OUT UINT64             *BaudRate,
    IN OUT UINT32             *ReceiveFifoDepth,
    IN OUT UINT32             *Timeout,
    IN OUT EFI_PARITY_TYPE    *Parity,
    IN OUT UINT8              *DataBits,
    IN OUT EFI_STOP_BITS_TYPE *StopBits
)
{
    // 
    // ignore this call but dont fail it.
    //
    return EFI_SUCCESS;
}
