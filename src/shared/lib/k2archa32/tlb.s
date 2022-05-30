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
#include <k2asma32.inc>

/* --------------------------------------------------------------------------------- */

// void A32_TLBInvalidateAll_UP(void);
BEGIN_A32_PROC(A32_TLBInvalidateAll_UP)
    mcr p15, 0, r12, c8, c7, 0
    isb
    bx lr
END_A32_PROC(A32_TLBInvalidateAll_UP)

// void A32_TLBInvalidateMVA_UP_OneASID(UINT32 aMVA, UINT32 aASID);
BEGIN_A32_PROC(A32_TLBInvalidateMVA_UP_OneASID)
    bic r0, r0, #0xFF
    bic r0, r0, #0xF00
    and r1, r1, #0xFF
    orr r0, r0, r1
    mcr p15, 0, r0, c8, c7, 1
    isb
    bx lr
END_A32_PROC(A32_TLBInvalidateMVA_UP_OneASID)

// void A32_TLBInvalidateASID_UP(UINT32 aASID);
BEGIN_A32_PROC(A32_TLBInvalidateASID_UP)
    and r0, r0, #0xFF
    mcr p15, 0, r0, c8, c7, 2
    isb
    bx lr
END_A32_PROC(A32_TLBInvalidateASID_UP)

// void A32_TLBInvalidateMVA_UP_AllASID(UINT32 aMVA);
BEGIN_A32_PROC(A32_TLBInvalidateMVA_UP_AllASID)
    mcr p15, 0, r0, c8, c7, 3
    isb
    bx lr
END_A32_PROC(A32_TLBInvalidateMVA_UP_AllASID)

/* --------------------------------------------------------------------------------- */

// void A32_TLBInvalidateAll_MP(void);
BEGIN_A32_PROC(A32_TLBInvalidateAll_MP)
    mcr p15, 0, r12, c8, c3, 0
    isb
    bx lr
END_A32_PROC(A32_TLBInvalidateAll_MP)

// void A32_TLBInvalidateMVA_MP_OneASID(UINT32 aMVA, UINT32 aASID);
BEGIN_A32_PROC(A32_TLBInvalidateMVA_MP_OneASID)
    bic r0, r0, #0xFF
    bic r0, r0, #0xF00
    and r1, r1, #0xFF
    orr r0, r0, r1
    mcr p15, 0, r0, c8, c3, 1
    isb
    bx lr
END_A32_PROC(A32_TLBInvalidateMVA_MP_OneASID)

// void A32_TLBInvalidateASID_MP(UINT32 aASID);
BEGIN_A32_PROC(A32_TLBInvalidateASID_MP)
    and r0, r0, #0xFF
    mcr p15, 0, r0, c8, c3, 2
    isb
    bx lr
END_A32_PROC(A32_TLBInvalidateASID_MP)

// void A32_TLBInvalidateMVA_MP_AllASID(UINT32 aMVA);
BEGIN_A32_PROC(A32_TLBInvalidateMVA_MP_AllASID)
    mcr p15, 0, r0, c8, c3, 3
    isb
    bx lr
END_A32_PROC(A32_TLBInvalidateMVA_MP_AllASID)

/* --------------------------------------------------------------------------------- */

    .end

