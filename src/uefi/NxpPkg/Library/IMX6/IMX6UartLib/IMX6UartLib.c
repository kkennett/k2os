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

#include <Library/IMX6/IMX6UartLib.h>
#include <Library/IoLib.h>

VOID
EFIAPI
IMX6_UART_SyncInitForDebug(
    IN  UINT32  UartBaseAddress,
    IN  UINT32  UartRFDIV,
    IN  UINT32  UartBIR,
    IN  UINT32  UartBMR
)
{
    //
    // reset, wait for reset to clear
    //
    MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UCR1, 0);
    MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UCR2, 0);
    while (0 == (MmioRead32(UartBaseAddress + IMX6_UART_OFFSET_UCR2) & IMX6_UART_UCR2_SRST));
    while (0 != (MmioRead32(UartBaseAddress + IMX6_UART_OFFSET_UTS) & IMX6_UART_UTS_SOFTRST));

    //
    // configure default dsr, cdc, ri
    //
    MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UCR3,
        IMX6_UART_UCR3_DSR |
        IMX6_UART_UCR3_DCD |
        IMX6_UART_UCR3_RI |
        IMX6_UART_UCR3_ADNIMP |
        IMX6_UART_UCR3_RXDMUXSEL);

    //
    // line parameters and baud rate from clock configuration
    //
    MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UCR4, 0x8000);
    MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UESC, 0x2B);
    MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UTIM, 0);
    MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UTS, 0);
    MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UFCR, (MmioRead32(UartBaseAddress + IMX6_UART_OFFSET_UFCR) & ~IMX6_UART_UFCR_RFDIV_MASK) | (UartRFDIV & IMX6_UART_UFCR_RFDIV_MASK));
    MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UBIR, UartBIR);
    MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UBMR, UartBMR);

    // 
    // configure for enable tx,rx
    //
    MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UCR2,
        IMX6_UART_UCR2_IRTS |
        IMX6_UART_UCR2_WS |
        IMX6_UART_UCR2_TXEN |
        IMX6_UART_UCR2_RXEN |
        IMX6_UART_UCR2_SRST);

    //
    // enable the UART
    //
    MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UCR1, IMX6_UART_UCR1_UARTEN);
}

UINTN
EFIAPI
IMX6_UART_SyncWrite(
    IN  UINT8 * Buffer,
    IN  UINTN   NumberOfBytes,
    IN  UINT32  UartBaseAddress
)
{
    UINTN Count;

    for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
        while (0 == (MmioRead32(UartBaseAddress + IMX6_UART_OFFSET_UTS) & IMX6_UART_UTS_TXEMPTY));
        MmioWrite32(UartBaseAddress + IMX6_UART_OFFSET_UTXD, *Buffer);
    }

    return NumberOfBytes;
}

UINTN
EFIAPI
IMX6_UART_SyncRead(
    OUT UINT8 * Buffer,
    IN  UINTN   NumberOfBytes,
    IN  UINT32  UartBaseAddress
)
{
    UINTN Count;

    for (Count = 0; Count < NumberOfBytes; Count++)
    {
        while (0 == (MmioRead32(UartBaseAddress + IMX6_UART_OFFSET_USR2) & IMX6_UART_USR2_RDR));
        *Buffer = (UINT8)(MmioRead32(UartBaseAddress + IMX6_UART_OFFSET_URXD) & 0xFF);
        Buffer++;
    }

    return NumberOfBytes;
}

BOOLEAN
EFIAPI
IMX6_UART_Poll(
    IN  UINT32  UartBaseAddress
)
{
    return (0 != (MmioRead32(UartBaseAddress + IMX6_UART_OFFSET_USR2) & IMX6_UART_USR2_RDR));
}


