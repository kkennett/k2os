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
#include <k2asma32.inc>

/*-------------------------------------------------------------------------------*/
// UINT32 K2ATOMIC_CompareExchange32(UINT32 volatile *apMem, UINT32 aNewVal, UINT32 aComparand);
BEGIN_A32_PROC(K2ATOMIC_CompareExchange32)
_Restart32:
    ldrex r3, [r0]
    cmp r3, r2
    beq _TryStore32
    clrex
    mov r0, r3
    bx lr
_TryStore32:
    strex r12, r1, [r0]
    cmp r12, #0
    bne _Restart32
    mov r0, r3
    bx lr
END_A32_PROC(K2ATOMIC_CompareExchange32)

/*-------------------------------------------------------------------------------*/
// UINT64 K2ATOMIC_CompareExchange64(UINT64 volatile *apMem, UINT64 aNewVal, UINT64 aComparand);
BEGIN_A32_PROC(K2ATOMIC_CompareExchange64)
_Restart64:
    ldrex r3, [r0]
    cmp r3, r2
    beq _TryStore64
    clrex
    mov r0, r3
    bx lr
_TryStore64:
    strex r12, r1, [r0]
    cmp r12, #0
    bne _Restart64
    mov r0, r3
    bx lr
END_A32_PROC(K2ATOMIC_CompareExchange64)
  
/*-------------------------------------------------------------------------------*/

    .end
