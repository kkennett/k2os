/*   
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
*/
#ifndef __K2ASMX32_INC
#define __K2ASMX32_INC

/* --------------------------------------------------------------------------------- */

.intel_syntax
.code32

#include "k2basedefs.inc"

#define X32_DATA \
        .section ".data","aw"

#define X32_TEXT \
        .section ".text","ax"

#define X32_READ \
        .section ".text","a"

#define X32_STARTUPTEXT \
        .section ".text.startup","x","progbits"

#define BEGIN_X32_PROC(symbol) \
        .text; \
        .balign 4; \
        .global symbol; \
        .type   symbol,"function"; \
symbol:

#define BEGIN_X32_STATIC_PROC(symbol) \
        .text; \
        .balign 4; \
        .type   symbol,"function"; \
symbol:

#define END_X32_PROC(symbol) \
        .size   symbol,.-symbol;

/* --------------------------------------------------------------------------------- */

#endif // __K2ASMX32_INC

