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
#ifndef __SPEC_ASCII_H
#define __SPEC_ASCII_H

//
// --------------------------------------------------------------------------------- 
//

#include <k2basetype.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ASCII_CODE
{
    ASCII_NUL,       //  0 == 0x00 - null character
    ASCII_SOH,       //  1 == 0x01 - start of header
    ASCII_STX,       //  2 == 0x02 - start of text
    ASCII_ETX,       //  3 == 0x03 - end of text
    ASCII_EOT,       //  4 == 0x04 - end of transmit
    ASCII_ENQ,       //  5 == 0x05 - enquiry
    ASCII_ACK,       //  6 == 0x06 - acknowledge
    ASCII_BEL,       //  7 == 0x07 - alert bell     '\a'
    ASCII_BS,        //  8 == 0x08 - backspace      '\b'
    ASCII_TAB,       //  9 == 0x09 - horizontal tab '\t'
    ASCII_LF,        // 10 == 0x0A - line feed      '\r'
    ASCII_VT,        // 11 == 0x0B - vertical tab   '\v'
    ASCII_FF,        // 12 == 0x0C - form feed      '\f'
    ASCII_CR,        // 13 == 0x0D - carrage return '\n'
    ASCII_SO,        // 14 == 0x0E - shift out
    ASCII_SI,        // 15 == 0x0F - shift in
    ASCII_DLE,       // 16 == 0x10 - device link escape
    ASCII_DC1,       // 17 == 0x11 - device code 1 (XON)
    ASCII_DC2,       // 18 == 0x12 - device code 2
    ASCII_DC3,       // 19 == 0x13 - device code 3 (XOFF)
    ASCII_DC4,       // 20 == 0x14 - device code 4
    ASCII_NAK,       // 21 == 0x15 - negative acknowledge
    ASCII_SYN,       // 22 == 0x16 - synchronous idle
    ASCII_ETB,       // 23 == 0x17 - end transmission block
    ASCII_CAN,       // 24 == 0x18 - cancel
    ASCII_EM,        // 25 == 0x19 - end medium
    ASCII_SUB,       // 26 == 0x1A - substitute
    ASCII_ESC,       // 27 == 0x1B - escape
    ASCII_FS,        // 28 == 0x1C - field seperator
    ASCII_GS,        // 29 == 0x1D - group separator
    ASCII_RS,        // 30 == 0x1E - record separator
    ASCII_US,        // 31 == 0x1F - unit separator
    ASCII_SPC,       // 32 == 0x20 - space
} ASCII_CODE;


#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif  // __SPEC_ASCII_H

