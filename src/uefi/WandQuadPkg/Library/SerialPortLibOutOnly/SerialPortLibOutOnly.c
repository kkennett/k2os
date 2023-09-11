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

#include <WandQuad.h>
#include <Library/SerialPortLib.h>
#include <Library/IMX6/IMX6UartLib.h>

RETURN_STATUS
EFIAPI
SerialPortInitialize(
    VOID
)
{
    //
    // pad configuration and intiialization of UART is done elsewhere,
    // usually in SEC when the platform first executes code.
    // this library is used by multiple modules and reinitializing for
    // each one is dumb
    //
    return EFI_SUCCESS;
}

UINTN
EFIAPI
SerialPortWrite(
    IN  UINT8 * Buffer,
    IN  UINTN   NumberOfBytes
)
{
    return IMX6_UART_SyncWrite(Buffer, NumberOfBytes, WANDQUAD_DEBUG_UART);
}

UINTN
EFIAPI
SerialPortRead(
    OUT UINT8 * Buffer,
    IN  UINTN   NumberOfBytes
)
{
    return 0;
}

BOOLEAN
EFIAPI
SerialPortPoll(
    VOID
)
{
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
