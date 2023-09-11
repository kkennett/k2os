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
#include <lib/k2atomic.h>

INT32 
K2_CALLCONV_REGS
K2ATOMIC_Add32(
    INT32 volatile *    apDest,
    INT32               aValue
    )
{
    K2_ASSERT(apDest != NULL);

    return aValue + K2ATOMIC_AddExchange32(apDest, aValue);
}

INT32 
K2_CALLCONV_REGS
K2ATOMIC_AddExchange32(
    INT32 volatile *    apDest,
    INT32               aValue
    )
{
    INT32 val;

    K2_ASSERT(apDest != NULL);

    do {
        val = *apDest;
    } while (val != K2ATOMIC_CompareExchange32((UINT32 volatile *)apDest, val + aValue, val));
    
    return val;
}

UINT32 
K2_CALLCONV_REGS
K2ATOMIC_And32(
    UINT32 volatile *   apDest,
    UINT32              aValue
    )
{
    UINT32 val;

    K2_ASSERT(apDest != NULL);

    do {
        val = *apDest;
    } while (val != K2ATOMIC_CompareExchange32(apDest, val & aValue, val));

    return val & aValue;
}

UINT32 
K2_CALLCONV_REGS
K2ATOMIC_Or32(
    UINT32 volatile *   apDest,
    UINT32              aValue
    )
{
    UINT32 val;

    K2_ASSERT(apDest != NULL);

    do {
        val = *apDest;
    } while (val != K2ATOMIC_CompareExchange32(apDest, val | aValue, val));

    return val | aValue;
}

UINT32 
K2_CALLCONV_REGS
K2ATOMIC_Xor32(
    UINT32 volatile *   apDest,
    UINT32              aValue
    )
{
    UINT32 val;

    K2_ASSERT(apDest != NULL);

    do {
        val = *apDest;
    } while (val != K2ATOMIC_CompareExchange32(apDest, val ^ aValue, val));

    return val ^ aValue;
}

UINT32 
K2_CALLCONV_REGS
K2ATOMIC_Not32(
    UINT32 volatile *   apDest
    )
{
    UINT32 val;

    K2_ASSERT(apDest != NULL);

    do {
        val = *apDest;
    } while (val != K2ATOMIC_CompareExchange32(apDest, ~val, val));

    return ~val;
}

UINT32 
K2_CALLCONV_REGS
K2ATOMIC_Exchange32(
    UINT32 volatile *   apDest,
    UINT32              aValue
    )
{
    UINT32 val;

    K2_ASSERT(apDest != NULL);

    do {
        val = *apDest;
    } while (val != K2ATOMIC_CompareExchange32(apDest, aValue, val));

    return val;
}

