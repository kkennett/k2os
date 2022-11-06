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

#include <Library/IMX6/IMX6GptLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>

#define GPTLIB_Check(x,size) \
typedef int Check ## x [(sizeof(x) == size) ? 1 : -1];

// -----------------------------------------------------------------------------

typedef union _GPT_CR
{
    struct {
        UINT32  EN              : 1;
        UINT32  ENMOD           : 1;
        UINT32  DBGEN           : 1;
        UINT32  WAITEN          : 1;
        UINT32  DOZEEN          : 1;
        UINT32  STOPEN          : 1;
        UINT32  CLKSRC          : 3;
        UINT32  FRR             : 1;
        UINT32  Reserved_0      : 5;
        UINT32  SWR             : 1;
        UINT32  IM1             : 2;
        UINT32  IM2             : 2;
        UINT32  OM1             : 3;
        UINT32  OM2             : 3;
        UINT32  OM3             : 3;
        UINT32  FO1             : 1;
        UINT32  FO2             : 1;
        UINT32  FO3             : 1;
    };
    UINT32 Raw;
} REG_GPT_CR;
GPTLIB_Check(REG_GPT_CR, sizeof(UINT32));

typedef union _GPT_PR
{
    struct {
        UINT32  PRESCALER       : 12;
        UINT32  Reserved_0      : 20;
    };
    UINT32 Raw;
} REG_GPT_PR;
GPTLIB_Check(REG_GPT_PR, sizeof(UINT32));

// -----------------------------------------------------------------------------

UINT32
EFIAPI
IMX6_GPT_GetRate(
    UINT32 Regs_CCM,
    UINT32 Regs_GPT
)
{
    REG_GPT_CR GPT_CR;
    REG_GPT_PR GPT_PR;
    UINT64     Calc64;

    GPT_CR.Raw = MmioRead32(Regs_GPT + IMX6_GPT_OFFSET_CR);
    GPT_PR.Raw = MmioRead32(Regs_GPT + IMX6_GPT_OFFSET_PR);

    Calc64 = 0;

    switch (GPT_CR.CLKSRC)
    {
    case 1:
        // Peripheral Clock (ipg_clk)
        Calc64 = IMX6_CLOCK_GetRate_IPG(Regs_CCM);
        break;
    case 2:
        // High Frequency Reference Clock (ipg_clk_highfreq)
        Calc64 = IMX6_CLOCK_GetRate_PERCLK_CLK_ROOT(Regs_CCM);
        break;
    case 4:
        // Low Frequency Reference Clock (ipg_clk_32k)
        Calc64 = IMX6_LOW_OSC_CLOCK_FREQ;
        break;
    case 5:
        Calc64 = IMX6_HIGH_OSC_CLOCK_FREQ / 8;
        break;
    case 7:
        Calc64 = IMX6_HIGH_OSC_CLOCK_FREQ;
        break;

    default:
        break;
    }

    Calc64 /= (UINT64)(GPT_PR.PRESCALER + 1);

    ASSERT(Calc64 != 0);

    return (UINT32)Calc64;
}

UINT32
EFIAPI
IMX6_GPT_TicksFromUs(
    UINT32 GPTRate,
    UINT32 uSeconds
)
{
    UINT64 Calc64;

    if (uSeconds == 0)
        return 0;

    ASSERT(GPTRate != 0);

    Calc64 = (((UINT64)GPTRate) * ((UINT64)uSeconds)) / (UINT64)1000000;

    if (Calc64 & 0xFFFFFFFF00000000)
        return 0xFFFFFFFF;

    return (UINT32)Calc64;
}

void
IMX6_GPT_DelayUs(
    UINT32 Regs_GPT,
    UINT32 GPTRate,
    UINT32 usDelay
)
{
    UINT32  Cur;
    UINT32  End;

    if (usDelay == 0)
        return;

    ASSERT(GPTRate != 0);

    Cur = MmioRead32(Regs_GPT + IMX6_GPT_OFFSET_CNT);

    End = Cur + IMX6_GPT_TicksFromUs(GPTRate, usDelay);

    if (End < Cur)
    {
        // going to wrap
        while (MmioRead32(Regs_GPT + IMX6_GPT_OFFSET_CNT) >= Cur);
    }

    while (MmioRead32(Regs_GPT + IMX6_GPT_OFFSET_CNT) < End);
}
