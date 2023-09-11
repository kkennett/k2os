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

/* --------------------------------------------------------------------------------- */

// void A32_DCacheFlushInvalidateMVA_PoC(UINT32 aMVA);
BEGIN_A32_PROC(A32_DCacheFlushInvalidateMVA_PoC)
    mcr p15, 0, r0, c7, c14, 1  // DCCIMVAC 
    bx lr
END_A32_PROC(A32_DCacheFlushInvalidateMVA_PoC)

// void A32_DCacheFlushInvalidateSetWay(UINT32 aSetWay);
BEGIN_A32_PROC(A32_DCacheFlushInvalidateSetWay)
    mcr p15, 0, r0, c7, c14, 2  // DCCISW
    bx lr
END_A32_PROC(A32_DCacheFlushInvalidateSetWay)

// void A32_DCacheFlushMVA_PoC(UINT32 aMVA);
BEGIN_A32_PROC(A32_DCacheFlushMVA_PoC)
    mcr p15, 0, r0, c7, c10, 1  // DCCMVAC
    bx lr
END_A32_PROC(A32_DCacheFlushMVA_PoC)

// void A32_DCacheFlushMVA_PoU(UINT32 aMVA);
BEGIN_A32_PROC(A32_DCacheFlushMVA_PoU)
    mcr p15, 0, r0, c7, c11, 1  // DCCMVAU
    bx lr
END_A32_PROC(A32_DCacheFlushMVA_PoU)

// void A32_DCacheFlushSetWay(UINT32 aSetWay);
BEGIN_A32_PROC(A32_DCacheFlushSetWay)
    mcr p15, 0, r0, c7, c10, 2  // DCCSW
    bx lr
END_A32_PROC(A32_DCacheFlushSetWay)

// void A32_DCacheInvalidateMVA_PoC(UINT32 aMVA);
BEGIN_A32_PROC(A32_DCacheInvalidateMVA_PoC)
    mcr p15, 0, r0, c7, c6, 1   // DCIMVAC
    bx lr
END_A32_PROC(A32_DCacheInvalidateMVA_PoC)

// void A32_DCacheInvalidateSetWay(UINT32 aSetWay);
BEGIN_A32_PROC(A32_DCacheInvalidateSetWay)
    mcr p15, 0, r0, c7, c6, 2   //DCISW
    bx lr
END_A32_PROC(A32_DCacheInvalidateSetWay)

/* --------------------------------------------------------------------------------- */

    .end

