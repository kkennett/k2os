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
#ifndef __IMX6UARTLIB_H__
#define __IMX6UARTLIB_H__

#include <Uefi.h>
#include "../../IMX6def.inc"

// -----------------------------------------------------------------------------

VOID
EFIAPI
IMX6_UART_SyncInitForDebug(
    IN  UINT32  UartBaseAddress,
    IN  UINT32  UartRFDIV,
    IN  UINT32  UartBIR,
    IN  UINT32  UartBMR
);

UINTN
EFIAPI
IMX6_UART_SyncWrite(
    IN  UINT8 * Buffer,
    IN  UINTN   NumberOfBytes,
    IN  UINT32  UartBaseAddress
);

UINTN
EFIAPI
IMX6_UART_SyncRead(
    OUT UINT8 * Buffer,
    IN  UINTN   NumberOfBytes,
    IN  UINT32  UartBaseAddress
);

BOOLEAN
EFIAPI
IMX6_UART_Poll(
    IN  UINT32  UartBaseAddress
);

// -----------------------------------------------------------------------------

#endif  // __IMX6UARTLIB_H__
