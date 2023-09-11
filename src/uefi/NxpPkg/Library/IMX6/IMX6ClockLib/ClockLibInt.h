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

#ifndef __CLOCKLIBINT_H
#define __CLOCKLIBINT_H

#include <Library/IMX6/IMX6ClockLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>

#define CLOCKLIBINT_Check(x,size) \
typedef int Check ## x [(sizeof(x) == size) ? 1 : -1];

// -----------------------------------------------------------------------------

typedef union _CCM_CBCDR
{
    struct {
        UINT32  periph2_clk2_podf   : 3;
        UINT32  mmdc_ch1_axi_podf   : 3;
        UINT32  axi_sel             : 1;
        UINT32  axi_alt_sel         : 1;
        UINT32  ipg_podf            : 2;
        UINT32  ahb_podf            : 3;
        UINT32  rz13                : 3;
        UINT32  axi_podf            : 3;
        UINT32  mmdc_ch0_axi_podf   : 3;
        UINT32  rz22                : 3;
        UINT32  periph_clk_sel      : 1;
        UINT32  periph2_clk_sel     : 1;
        UINT32  periph_clk2_podf    : 3;
        UINT32  rz30                : 2;
    };
    UINT32 Raw;
} REG_CCM_CBCDR;
CLOCKLIBINT_Check(REG_CCM_CBCDR, sizeof(UINT32));

typedef union _CCM_CBCMR
{
    struct {
        UINT32 gpu2d_axi_clk_sel    : 1;
        UINT32 gpu3d_axi_clk_sel    : 1;
        UINT32 rz2                  : 2;
        UINT32 gpu3d_core_clk_sel   : 2;
        UINT32 rz6                  : 2;
        UINT32 gpu3d_shader_clk_sel : 2;
        UINT32 pcie_axi_clk_sel     : 1;
        UINT32 vdoaxi_clk_sel       : 1;
        UINT32 periph_clk2_sel      : 2;
        UINT32 vpu_axi_clk_sel      : 2;
        UINT32 gpu2d_core_clk_sel   : 2;
        UINT32 pre_periph_clk_sel   : 2;
        UINT32 periph2_clk2_sel     : 1;
        UINT32 pre_periph2_clk_sel  : 2;
        UINT32 gpu2d_core_clk_podf  : 3;
        UINT32 gpu3d_core_clk_podf  : 3;
        UINT32 gpu3d_shader_podf    : 3;
    };
    UINT32 Raw;
} REG_CCM_CBCMR;
CLOCKLIBINT_Check(REG_CCM_CBCMR, sizeof(UINT32));

typedef union _CCM_CSCMR1
{
    struct {
        UINT32  perclk_podf         : 6;
        UINT32  rz6                 : 4;
        UINT32  ssi1_clk_sel        : 2;
        UINT32  ssi2_clk_sel        : 2;
        UINT32  ssi3_clk_sel        : 2;
        UINT32  usdhc1_clk_sel      : 1;
        UINT32  usdhc2_clk_sel      : 1;
        UINT32  usdhc3_clk_sel      : 1;
        UINT32  usdhc4_clk_sel      : 1;
        UINT32  aclk_podf           : 3;
        UINT32  aclk_eim_slow_podf  : 3;
        UINT32  rz26                : 1;
        UINT32  aclk_sel            : 2;
        UINT32  aclk_eim_slow_sel   : 2;
        UINT32  rz31                : 1;
    };
    UINT32 Raw;
} REG_CCM_CSCMR1;
CLOCKLIBINT_Check(REG_CCM_CSCMR1, sizeof(UINT32));

#endif // __CLOCKLIBINT_H