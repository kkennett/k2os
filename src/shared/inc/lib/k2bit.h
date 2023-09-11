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

#ifndef __K2BIT_H
#define __K2BIT_H

/* --------------------------------------------------------------------------------- */

#include <k2systype.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL
K2BIT_GetHighestPos32(
    UINT32      aValue,
    UINT_PTR *  apRetHighestBitSet
    );

UINT32
K2BIT_GetLowestOnly32(
    UINT32  aValue
    );

BOOL
K2BIT_IsOnlyOneSet32(
    UINT32 aValue
    );

BOOL
K2BIT_GetLowestPos32(
    UINT32      aValue,
    UINT_PTR *  apRetLowestBitSet
    );

UINT_PTR
K2BIT_CountNumberSet32(
    UINT32 aValue
    );

BOOL
K2BIT_GetHighestPos64(
    UINT64      aValue,
    UINT_PTR *  apRetHighestBitSet
    );

UINT64
K2BIT_GetLowestOnly64(
    UINT64  aValue
    );

BOOL
K2BIT_IsOnlyOneSet64(
    UINT64 aValue
    );

BOOL
K2BIT_GetLowestPos64(
    UINT64      aValue,
    UINT_PTR *  apRetLowestBitSet
    );

UINT_PTR
K2BIT_CountNumberSet64(
    UINT64 aValue
    );

#if K2_TARGET_ARCH_IS_32BIT
#define K2BIT_GetHighestPosN  K2BIT_GetHighestPos32
#define K2BIT_GetLowestOnlyN  K2BIT_GetLowestOnly32
#define K2BIT_IsOnlyOneSetN   K2BIT_IsOnlyOneSet32
#define K2BIT_GetLowestPosN   K2BIT_GetLowestPos32
#define K2BIT_CountNumberSetN K2BIT_CountNumberSet32
#else
#define K2BIT_GetHighestPosN  K2BIT_GetHighestPos64
#define K2BIT_GetLowestOnlyN  K2BIT_GetLowestOnly64
#define K2BIT_IsOnlyOneSetN   K2BIT_IsOnlyOneSet64
#define K2BIT_GetLowestPosN   K2BIT_GetLowestPos64
#define K2BIT_CountNumberSetN K2BIT_CountNumberSet64
#endif

/* --------------------------------------------------------------------------------- */

#define K2BIT_FIELD_WORDS_TO_HOLD_BITS(numBits)  (((numBits)+31)/32)

struct _K2BIT_FIELD
{
    UINT32  *   mpWords;
    UINT_PTR    mNumWords;
    UINT_PTR    mFirstFreeBitWordIndex;
};
typedef struct _K2BIT_FIELD K2BIT_FIELD;

void
K2BIT_InitField(
    K2BIT_FIELD *   apField,
    UINT_PTR        aNumWords,
    UINT32 *        apWords
    );

BOOL
K2BIT_AllocFromField(
    K2BIT_FIELD *   apField,
    UINT_PTR        aNumBits,
    UINT_PTR *      apRetFirstBitIndex
    );

void
K2BIT_FreeToField(
    K2BIT_FIELD *   apField,
    UINT_PTR        aNumBits,
    UINT_PTR        aFirstBitIndex
    );

#ifdef __cplusplus
};  // extern "C"
#endif

/* --------------------------------------------------------------------------------- */

#endif // __K2BIT_H
