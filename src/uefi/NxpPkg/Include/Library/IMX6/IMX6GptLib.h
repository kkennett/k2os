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
#ifndef __IMX6GPTLIB_H__
#define __IMX6GPTLIB_H__

#include <Uefi.h>
#include "../../IMX6def.inc"
#include <Library/IMX6/IMX6ClockLib.h>

// -----------------------------------------------------------------------------

typedef struct _IMX6_GPT IMX6_GPT;
struct _IMX6_GPT
{
    UINT32  mRegs_GPT;
    UINT32  mRate;
};

VOID
EFIAPI
IMX6_GPT_Init(
    IMX6_GPT *  apGpt,
    UINT32      Regs_CCM,
    UINT32      Regs_GPT
);

UINT32
EFIAPI
IMX6_GPT_GetTickCount(
    IMX6_GPT *  apGpt
);

UINT32
EFIAPI
IMX6_GPT_TicksFromUs(
    IMX6_GPT *  apGpt,
    UINT32      aUs
);

void
IMX6_GPT_DelayUs(
    IMX6_GPT *  apGpt,
    UINT32      aUsDelay
);

// -----------------------------------------------------------------------------

#endif  // __IMX6GPTLIB_H__
