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

#include "ClockLibInt.h"

static const UINT32 sgAnalog_Offset[6] = 
{
    0,
    0x4000, // PLL1 (800 TYP) ARM
    0x4030, // PLL2 (528) USB1
    0x4010, // PLL3 (480) SYS
    0x4070, // PLL4 (630) AUDIO
    0x40A0  // PLL5 (630) VIDEO
};

static const UINT32 sgAnalog_PFD_Offset[4] =
{
    0,
    0,
    0x4100,  // PLL2 (528)
    0x40F0   // PLL3 (480)
};

UINT32
EFIAPI
IMX6_CLOCK_GetRate_IPG(
    UINT32 Regs_CCM
)
{
    REG_CCM_CBCDR CBCDR;

    CBCDR.Raw = MmioRead32(Regs_CCM + IMX6_CCM_OFFSET_CBCDR);

    return IMX6_CLOCK_GetRate_AHB(Regs_CCM) / (CBCDR.ipg_podf + 1);
}

UINT32
EFIAPI
IMX6_CLOCK_GetRate_USDHC_SRC(
    UINT32 Regs_CCM,
    UINT32 UnitNum
)
{
    REG_CCM_CSCMR1 CSCMR1;

    ASSERT((UnitNum >= 1) && (UnitNum <= 4));

    CSCMR1.Raw = MmioRead32(Regs_CCM + IMX6_CCM_OFFSET_CSCMR1);

    if ((CSCMR1.Raw >> (15+UnitNum)) & 1)
        return IMX6_CLOCK_GetRate_PLL2_PFD(Regs_CCM, 0);

    return IMX6_CLOCK_GetRate_PLL2_PFD(Regs_CCM, 2);
}

UINT32
EFIAPI
IMX6_CLOCK_GetRate_PERCLK_CLK_ROOT(
    UINT32 Regs_CCM
)
{
    REG_CCM_CSCMR1 CSCMR1;

    CSCMR1.Raw = MmioRead32(Regs_CCM + IMX6_CCM_OFFSET_CSCMR1);

    return IMX6_CLOCK_GetRate_IPG(Regs_CCM) / (1 + CSCMR1.perclk_podf);
}

UINT32
EFIAPI
IMX6_CLOCK_GetRate_AHB(
    UINT32 Regs_CCM
)
{
    REG_CCM_CBCDR   CBCDR;
    REG_CCM_CBCMR   CBCMR;
    UINT32          srcClk;

    CBCDR.Raw = MmioRead32(Regs_CCM + IMX6_CCM_OFFSET_CBCDR);
    CBCMR.Raw = MmioRead32(Regs_CCM + IMX6_CCM_OFFSET_CBCMR);

    if (CBCDR.periph_clk_sel != 0)
    {
        // periph_clk2_clk
        if (CBCMR.periph_clk2_sel == 0)
        {
            // pll_sw_clk
            srcClk = IMX6_CLOCK_GetRate_PLL3_MAIN(Regs_CCM);
        }
        else
        {
            srcClk = IMX6_HIGH_OSC_CLOCK_FREQ;
        }
        srcClk = srcClk / (CBCDR.periph2_clk2_podf + 1);
    }
    else
    {
        // pll2_main_clk
        switch (CBCMR.pre_periph_clk_sel)
        {
        case 0:
            srcClk = IMX6_CLOCK_GetRate_PLL2_MAIN(Regs_CCM);
            break;
        case 1:
            srcClk = IMX6_CLOCK_GetRate_PLL2_PFD(Regs_CCM, 2);
            break;
        case 2:
            srcClk = IMX6_CLOCK_GetRate_PLL2_PFD(Regs_CCM, 0);
            break;
        case 3:
            srcClk = IMX6_CLOCK_GetRate_PLL2_PFD(Regs_CCM, 2) >> 1;
            break;
        default:
            ASSERT(0);
        }
    }

    ASSERT(srcClk != 0);

    return srcClk / (CBCDR.ahb_podf + 1);
}

UINT32
EFIAPI
IMX6_CLOCK_GetRate_PLL3_MAIN(
    UINT32 Regs_CCM
)
{
    UINT32 analog;

    analog = MmioRead32(Regs_CCM + sgAnalog_Offset[3]);

    return IMX6_HIGH_OSC_CLOCK_FREQ * (20 + ((analog & 1) << 1));
}

UINT32
EFIAPI
IMX6_CLOCK_GetRate_PLL3_PFD(
    UINT32 Regs_CCM,
    UINT32 SelPFD
)
{
    UINT32 analog;
    UINT64 clkRate;

    ASSERT(SelPFD < 4);

    analog = MmioRead32(Regs_CCM + sgAnalog_PFD_Offset[3]);
    analog = (analog >> (8 * SelPFD)) & 0x3F;
    ASSERT(analog != 0);

    clkRate = (UINT64)IMX6_CLOCK_GetRate_PLL3_MAIN(Regs_CCM);
    clkRate *= 18ULL;
    clkRate /= (UINT64)analog;

    return (UINT32)clkRate;
}

UINT32
EFIAPI
IMX6_CLOCK_GetRate_PLL2_MAIN(
    UINT32 Regs_CCM
)
{
    UINT32 analog;

    analog = MmioRead32(Regs_CCM + sgAnalog_Offset[2]);

    return IMX6_HIGH_OSC_CLOCK_FREQ * (20 + ((analog & 1) << 1));
}

UINT32
EFIAPI
IMX6_CLOCK_GetRate_PLL2_PFD(
    UINT32 Regs_CCM,
    UINT32 SelPFD
)
{
    UINT32 analog;
    UINT64 clkRate;

    ASSERT(SelPFD < 4);

    analog = MmioRead32(Regs_CCM + sgAnalog_PFD_Offset[2]);
    analog = (analog >> (8 * SelPFD)) & 0x3F;
    ASSERT(analog != 0);

    clkRate = (UINT64)IMX6_CLOCK_GetRate_PLL2_MAIN(Regs_CCM);
    clkRate *= 18ULL;
    clkRate /= (UINT64)analog;

    return (UINT32)clkRate;
}
