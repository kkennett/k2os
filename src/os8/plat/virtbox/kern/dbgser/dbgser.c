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

#include "dbgser.h"

#define TX_EMPTY_BITS (PC_SERIAL_LSR_ALLTX_EMPTY | PC_SERIAL_LSR_THR_EMPTY)

void
PC_DBGSER_Init(
    void
)
{
    X32_IoWrite8(0, PC_SERIAL1_IER);
    X32_IoWrite8(PC_SERIAL_LCR_DLAB, PC_SERIAL1_LCR);
    X32_IoWrite8(PC_SERIAL_DIV_BAUD115200, PC_SERIAL1_DLAB1_LOWDIV);
    X32_IoWrite8(0, PC_SERIAL1_DLAB1_HIDIV);
    X32_IoWrite8(PC_SERIAL_LCR_DATABITS_8 |
        PC_SERIAL_LCR_PARITY_NONE |
        PC_SERIAL_LCR_STOPBITS_1, PC_SERIAL1_LCR);
    X32_IoWrite8(PC_SERIAL_FCR_LCR_BYTE1 |
        PC_SERIAL_FCR_CLEAR_TX |
        PC_SERIAL_FCR_CLEAR_RX, PC_SERIAL1_IIR_FIFOCR);
    X32_IoWrite8(PC_SERIAL_MCR_AUX2 |
        PC_SERIAL_MCR_RTS |
        PC_SERIAL_MCR_DTR, PC_SERIAL1_MCR);
}

void
PC_DBGSER_OutByte(
    UINT8 aByte
)
{
    while ((X32_IoRead8(PC_SERIAL1_LSR) & TX_EMPTY_BITS) != TX_EMPTY_BITS);
    X32_IoWrite8(aByte, PC_SERIAL1_THR_RHR);
}

BOOL
PC_DBGSER_InAvail(
    void
)
{
    if (0 == (X32_IoRead8(PC_SERIAL1_LSR) & PC_SERIAL_LSR_RHR_FULL))
        return FALSE;
    return TRUE;
}

BOOL
PC_DBGSER_InByte(
    UINT8 *apRetByte
)
{
    if (!PC_DBGSER_InAvail())
        return FALSE;
    *apRetByte = X32_IoRead8(PC_SERIAL1_THR_RHR);
    return TRUE;
}
