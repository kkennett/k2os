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
#include "crt32.h"

void
K2_CALLCONV_REGS
K2OS_System_GetHfTick(
    UINT64 *apRetHfTick
)
{
    UINT32 lo32;
    UINT32 hi32;
    UINT32 chk;

    chk = *((UINT32 *)(gTimerAddr + sizeof(UINT32)));
    lo32 = *((UINT32 *)gTimerAddr);
    hi32 = *((UINT32 *)(gTimerAddr + sizeof(UINT32)));
    if (hi32 != chk)
    {
        lo32 = *((UINT32 *)gTimerAddr);
    }

    *apRetHfTick = (((UINT64)hi32) << 32) | ((UINT64)lo32);
}

void     
K2_CALLCONV_REGS   
K2OS_System_GetMsTick(
    UINT64 *apRetMsTick
)
{
    UINT64 v;

    K2OS_System_GetHfTick(&v);

    *apRetMsTick = (v * 1000ull) / ((UINT64)gTimerFreq);
}

UINT32   
K2_CALLCONV_REGS   
K2OS_System_GetMsTick32(
    void
)
{
    UINT64 v;

    v = *((UINT32 *)gTimerAddr);
    
    return (UINT32)((v * 1000ull) / ((UINT64)gTimerFreq));
}

UINT32
K2_CALLCONV_REGS
K2OS_System_GetHfFreq(
    void
)
{
    return gTimerFreq;
}

void     
K2_CALLCONV_REGS   
K2OS_System_HfTickFromMsTick(
    UINT64 *        apRetHfTick,
    UINT64 const *  apMsTick
)
{
    *apRetHfTick = ((*apMsTick) * ((UINT64)gTimerFreq)) / 1000ull;
}

void     
K2_CALLCONV_REGS   
K2OS_System_MsTickFromHfTick(
    UINT64 *        apRetMsTick,
    UINT64 const *  apHfTick
)
{
    *apRetMsTick = (((*apHfTick) * 1000ull) / ((UINT64)gTimerFreq));
}

void     
K2_CALLCONV_REGS   
K2OS_System_HfTickFromMsTick32(
    UINT64 *apRetHfTick,
    UINT32 aMsTick32
)
{
    *apRetHfTick = (((UINT64)aMsTick32) * ((UINT64)gTimerFreq)) / 1000ull;
}

UINT32   
K2_CALLCONV_REGS   
K2OS_System_MsTick32FromHfTick(
    UINT64 const *apHfTick
)
{
    return (UINT32)(((*apHfTick) * 1000ull) / ((UINT64)gTimerFreq));
}
