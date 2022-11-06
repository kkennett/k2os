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
#ifndef __K2OSPLAT_H
#define __K2OSPLAT_H

#include "k2oskern.h"

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

K2_PACKED_PUSH
typedef struct _K2OS_PLATINIT_MAP K2OS_PLATINIT_MAP;
struct _K2OS_PLATINIT_MAP
{
    UINT_PTR    mPhysAddr;  // specified by k2osplat in array returned by K2OSPLAT_Init
    UINT_PTR    mPageCount; // specified by k2osplat in array returned by K2OSPLAT_Init
    UINT_PTR    mMapAttr;   // specified by k2osplat in array returned by K2OSPLAT_Init
    UINT_PTR    mVirtAddr;  // filled in by kernel in place after array returned by K2OSPLAT_Init
} K2_PACKED_ATTRIB;
K2_PACKED_POP

typedef K2OS_PLATINIT_MAP * (K2_CALLCONV_REGS *K2OSPLAT_pf_Init)(K2OS_PLATINIT_MAP const *apMapped);
K2OS_PLATINIT_MAP * K2_CALLCONV_REGS K2OSPLAT_Init(K2OS_PLATINIT_MAP const *apMapped);

typedef void (K2_CALLCONV_REGS *K2OSPLAT_pf_DebugOut)(UINT8 aByte);
void K2_CALLCONV_REGS K2OSPLAT_DebugOut(UINT8 aByte);

typedef BOOL (K2_CALLCONV_REGS *K2OSPLAT_pf_DebugIn)(UINT8 *apRetData);
BOOL K2_CALLCONV_REGS K2OSPLAT_DebugIn(UINT8 *apRetData);

typedef UINT_PTR (*K2OSPLAT_pf_ForcedDriverQuery)(K2OSKERN_PLATNODE const *apNode, char *apIoBuf, UINT_PTR aIoBufSizeBytes, UINT_PTR aIoBufInCount);
UINT_PTR K2OSPLAT_ForcedDriverQuery(K2OSKERN_PLATNODE const *apNode, char *apIoBuf, UINT_PTR aIoBufSizeBytes, UINT_PTR aIoBufInCount);

typedef K2OSKERN_PLATINTR * (K2_CALLCONV_REGS *K2OSPLAT_pf_OnIrq)(K2OSKERN_PLATIRQ const *apIrq);
K2OSKERN_PLATINTR * K2_CALLCONV_REGS K2OSPLAT_OnIrq(K2OSKERN_PLATIRQ const *apIrq);

typedef void (*K2OSPLAT_pf_GetResTable)(UINT32 aTableIx, UINT32 *apRetCount, void const **appRetTable);
void K2OSPLAT_GetResTable(UINT32 aTableIx, UINT32 *apRetCount, void const **appRetTable);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif


#endif // __K2OSHAL_H
