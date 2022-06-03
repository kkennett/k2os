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

#include <Uefi.h>
#include <Library/TimerLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/IMX6/IMX6GptLib.h>

UINTN
EFIAPI
MicroSecondDelay(
    IN  UINTN   MicroSeconds
)
{
    UINTN CurCount;
    UINTN TargetCount;

    CurCount = GetPerformanceCounter();
    TargetCount = CurCount + MicroSeconds;
    if (TargetCount < CurCount)
    {
        while (GetPerformanceCounter() >= CurCount);
    }
    while (GetPerformanceCounter() <= TargetCount);

    return MicroSeconds;
}

UINTN
EFIAPI
NanoSecondDelay(
    IN  UINTN   NanoSeconds
)
{
    if (NanoSeconds < (0xFFFFFFFF - 999))
        NanoSeconds += 999;
    MicroSecondDelay(NanoSeconds / 1000);
    return NanoSeconds;
}

UINT64
EFIAPI
GetPerformanceCounter(
    VOID
)
{
    return MmioRead32(IMX6_PHYSADDR_GPT_CNT);
}

UINT64
EFIAPI
GetPerformanceCounterProperties(
    OUT UINT64 *    StartValue,
    OUT UINT64 *    EndValue   
)
{
    if (StartValue != NULL) {
        *StartValue = 0x0;
    }

    if (EndValue != NULL) {
        *EndValue = 0xFFFFFFFF;
    }

    return 1000000;
}

UINT64
EFIAPI
GetTimeInNanoSecond(
    IN  UINT64  Ticks
)
{
    return Ticks * 1000;
}

