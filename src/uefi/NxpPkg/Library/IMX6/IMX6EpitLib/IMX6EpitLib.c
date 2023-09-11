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

#include <Library/IMX6/IMX6EpitLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>

#define EPITLIB_Check(x,size) \
typedef int Check ## x [(sizeof(x) == size) ? 1 : -1];

// -----------------------------------------------------------------------------

typedef union _EPIT_CR
{
    struct {
        UINT32  EN          : 1;
        UINT32  ENMOD       : 1;
        UINT32  OCIEN       : 1;
        UINT32  RLD         : 1;
        UINT32  PRESCALAR   : 12;
        UINT32  SWR         : 1;
        UINT32  IOVW        : 1;
        UINT32  DBGEN       : 1;
        UINT32  WAITEN      : 1;
        UINT32  rz20        : 1;
        UINT32  STOPEN      : 1;
        UINT32  OM          : 2;
        UINT32  CLKSRC      : 2;
        UINT32  rz26        : 6;
    };
    UINT32 Raw;
} REG_EPIT_CR;
EPITLIB_Check(REG_EPIT_CR, sizeof(UINT32));

UINT32
EFIAPI
IMX6_EPIT_GetRate(
    UINT32 Regs_CCM,
    UINT32 Regs_EPIT
)
{
    REG_EPIT_CR EPIT_CR;
    UINT32 clkRate;

    EPIT_CR.Raw = MmioRead32(Regs_EPIT + IMX6_EPIT_OFFSET_CR);

    switch (EPIT_CR.CLKSRC)
    {
    case 0:
        // clock is off
        return 0;
    case 1:
        // ipg_clk
        clkRate = IMX6_CLOCK_GetRate_IPG(Regs_CCM);
        break;
    case 2:
        // ipg_clk_highfreq
        clkRate = IMX6_CLOCK_GetRate_PERCLK_CLK_ROOT(Regs_CCM);
        break;
    case 3:
        // ipg_clk_32k
        clkRate = IMX6_LOW_OSC_CLOCK_FREQ;
        break;
    }

    return clkRate;
}

