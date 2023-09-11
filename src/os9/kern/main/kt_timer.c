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

#include "kern.h"

void    
K2OS_System_MsTickFromHfTick(
    UINT64 *        apRetMs,
    UINT64 const *  apHfTicks
)
{
    KernTimer_MsTickFromHfTick(apRetMs, apHfTicks);
}

void    
K2OS_System_HfTickFromMsTick(
    UINT64 *        apRetHfTicks,
    UINT64 const *  apMs
)
{
    return KernTimer_HfTickFromMsTick(apRetHfTicks, apMs);
}

UINT32  
K2OS_System_GetHfFreq(
    void
)
{
    return gData.Timer.mFreq;
}

void    
K2OS_System_GetHfTick(
    UINT64 *apRetHfTick
)
{
    if (NULL != apRetHfTick)
    {
        KernArch_GetHfTimerTick(apRetHfTick);
    }
}

void
K2OS_System_GetMsTick(
    UINT64 *apRetMsTick
)
{
    UINT64 v;

    K2OS_System_GetHfTick(&v);

    *apRetMsTick = (v * 1000ull) / ((UINT64)gData.Timer.mFreq);
}

UINT32
K2OS_System_GetMsTick32(
    void
)
{
    UINT64 v;
    K2OS_System_GetMsTick(&v);
    return (UINT32)v;
}

void
K2OS_System_HfTickFromMsTick32(
    UINT64 *apRetHfTick,
    UINT32 aMsTick32
)
{
    *apRetHfTick = (((UINT64)aMsTick32) * ((UINT64)gData.Timer.mFreq)) / 1000ull;
}

UINT32
K2OS_System_MsTick32FromHfTick(
    UINT64 const *apHfTick
)
{
    return (UINT32)(((*apHfTick) * 1000ull) / ((UINT64)gData.Timer.mFreq));
}
