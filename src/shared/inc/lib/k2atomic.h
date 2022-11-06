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
#ifndef __K2ATOMIC_H
#define __K2ATOMIC_H

#include <k2systype.h>

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

#if K2_TARGET_ARCH_IS_INTEL
#if K2_TOOLCHAIN_IS_MS

long
_InterlockedCompareExchange (
    long volatile *Destination,
    long ExChange,
    long Comperand
    );

INT64
_InterlockedCompareExchange64 (
    INT64 volatile *Destination,
    INT64 ExChange,
    INT64 Comperand
    );

#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedCompareExchange64)

#define InterlockedCompareExchange      _InterlockedCompareExchange
#define InterlockedCompareExchange64    _InterlockedCompareExchange64

#define K2ATOMIC_CompareExchange32 InterlockedCompareExchange
#define K2ATOMIC_CompareExchange64 InterlockedCompareExchange64
#endif
#endif

//
//------------------------------------------------------------------------
//

INT32  K2_CALLCONV_REGS K2ATOMIC_Add32        ( INT32 volatile * apDest, INT32  aValue);
INT32  K2_CALLCONV_REGS K2ATOMIC_AddExchange32( INT32 volatile * apDest, INT32  aValue);
UINT32 K2_CALLCONV_REGS K2ATOMIC_And32        (UINT32 volatile * apDest, UINT32 aValue);
UINT32 K2_CALLCONV_REGS K2ATOMIC_Or32         (UINT32 volatile * apDest, UINT32 aValue);
UINT32 K2_CALLCONV_REGS K2ATOMIC_Xor32        (UINT32 volatile * apDest, UINT32 aValue);
UINT32 K2_CALLCONV_REGS K2ATOMIC_Not32        (UINT32 volatile * apDest);
UINT32 K2_CALLCONV_REGS K2ATOMIC_Exchange32   (UINT32 volatile * apDest, UINT32 aValue);
#define K2ATOMIC_Inc32(apDest)        K2ATOMIC_Add32(apDest,1)
#define K2ATOMIC_Dec32(apDest)        K2ATOMIC_Add32(apDest,-1)

//
//------------------------------------------------------------------------
//

INT64  K2_CALLCONV_REGS K2ATOMIC_Add64        ( INT64 volatile * apDest, INT64  aValue);
INT64  K2_CALLCONV_REGS K2ATOMIC_AddExchange64( INT64 volatile * apDest, INT64  aValue);
UINT64 K2_CALLCONV_REGS K2ATOMIC_And64        (UINT64 volatile * apDest, UINT64 aValue);
UINT64 K2_CALLCONV_REGS K2ATOMIC_Or64         (UINT64 volatile * apDest, UINT64 aValue);
UINT64 K2_CALLCONV_REGS K2ATOMIC_Xor64        (UINT64 volatile * apDest, UINT64 aValue);
UINT64 K2_CALLCONV_REGS K2ATOMIC_Not64        (UINT64 volatile * apDest);
UINT64 K2_CALLCONV_REGS K2ATOMIC_Exchange64   (UINT64 volatile * apDest, UINT64 aValue);
#define K2ATOMIC_Inc64(apDest)        K2ATOMIC_Add64(apDest,1)
#define K2ATOMIC_Dec64(apDest)        K2ATOMIC_Add64(apDest,-1)

//
//------------------------------------------------------------------------
//

#if K2_TARGET_ARCH_IS_32BIT

#define K2ATOMIC_Add              K2ATOMIC_Add32        
#define K2ATOMIC_AddExchange      K2ATOMIC_AddExchange32
#define K2ATOMIC_And              K2ATOMIC_And32        
#define K2ATOMIC_Or               K2ATOMIC_Or32         
#define K2ATOMIC_Xor              K2ATOMIC_Xor32        
#define K2ATOMIC_Not              K2ATOMIC_Not32        
#define K2ATOMIC_Exchange         K2ATOMIC_Exchange32   
#define K2ATOMIC_Inc              K2ATOMIC_Inc32
#define K2ATOMIC_Dec              K2ATOMIC_Dec32
#define K2ATOMIC_CompareExchange  K2ATOMIC_CompareExchange32

#else

#define K2ATOMIC_Add              K2ATOMIC_Add64        
#define K2ATOMIC_AddExchange      K2ATOMIC_AddExchange64
#define K2ATOMIC_And              K2ATOMIC_And64        
#define K2ATOMIC_Or               K2ATOMIC_Or64         
#define K2ATOMIC_Xor              K2ATOMIC_Xor64        
#define K2ATOMIC_Not              K2ATOMIC_Not64        
#define K2ATOMIC_Exchange         K2ATOMIC_Exchange64   
#define K2ATOMIC_Inc              K2ATOMIC_Inc64
#define K2ATOMIC_Dec              K2ATOMIC_Dec64
#define K2ATOMIC_CompareExchange  K2ATOMIC_CompareExchange64

#endif

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif  // __K2ATOMIC_H

