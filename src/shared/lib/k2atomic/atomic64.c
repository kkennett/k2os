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
#include <lib/k2atomic.h>

INT64 
K2_CALLCONV_REGS
K2ATOMIC_Add64(
    INT64 volatile *    apDest,
    INT64               aValue
    )
{
    K2_ASSERT(apDest != NULL);
    return aValue + K2ATOMIC_AddExchange64(apDest, aValue);
}

INT64 
K2_CALLCONV_REGS
K2ATOMIC_AddExchange64(
    INT64 volatile *    apDest,
    INT64               aValue
    )
{
    INT64 val;

    K2_ASSERT(apDest != NULL);

    do {
        val = *apDest;
    } while (val != K2ATOMIC_CompareExchange64((UINT64 volatile *)apDest, val + aValue, val));
    
    return val;
}

UINT64 
K2_CALLCONV_REGS
K2ATOMIC_And64(
    UINT64 volatile *   apDest,
    UINT64              aValue
    )
{
    UINT64 val;

    K2_ASSERT(apDest != NULL);

    do {
        val = *apDest;
    } while (val != K2ATOMIC_CompareExchange64(apDest, val & aValue, val));

    return val & aValue;
}

UINT64 
K2_CALLCONV_REGS
K2ATOMIC_Or64(
    UINT64 volatile *   apDest,
    UINT64              aValue
    )
{
    UINT64 val;

    K2_ASSERT(apDest != NULL);

    do {
        val = *apDest;
    } while (val != K2ATOMIC_CompareExchange64(apDest, val | aValue, val));

    return val | aValue;
}

UINT64 
K2_CALLCONV_REGS
K2ATOMIC_Xor64(
    UINT64 volatile *   apDest,
    UINT64              aValue
    )
{
    UINT64 val;

    K2_ASSERT(apDest != NULL);

    do {
        val = *apDest;
    } while (val != K2ATOMIC_CompareExchange64(apDest, val ^ aValue, val));

    return val ^ aValue;
}

UINT64 
K2_CALLCONV_REGS
K2ATOMIC_Not64(
    UINT64 volatile *   apDest
    )
{
    UINT64 val;

    K2_ASSERT(apDest != NULL);

    do {
        val = *apDest;
    } while (val != K2ATOMIC_CompareExchange64(apDest, ~val, val));

    return ~val;
}

UINT64 
K2_CALLCONV_REGS
K2ATOMIC_Exchange64(
    UINT64 volatile *   apDest,
    UINT64              aValue
    )
{
    UINT64 val;

    K2_ASSERT(apDest != NULL);

    do {
        val = *apDest;
    } while (val != K2ATOMIC_CompareExchange64(apDest, aValue, val));

    return val;
}

