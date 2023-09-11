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

#ifndef __K2ARCHX32_H
#define __K2ARCHX32_H

#include <k2systype.h>

#if K2_TOOLCHAIN_IS_GCC
#if K2_TARGET_ARCH_IS_INTEL

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------------*/

UINT32 K2_CALLCONV_REGS X32_ReadESP(void);
void   K2_CALLCONV_REGS X32_WriteESP(UINT32 aVal);

UINT32 K2_CALLCONV_REGS X32_ReadEBP(void);
void   K2_CALLCONV_REGS X32_WriteEBP(UINT32 aVal);

void   K2_CALLCONV_REGS X32_ResetStackAndJump(UINT32 aNewStackPtr, UINT32 aJumpTarget);

UINT32 K2_CALLCONV_REGS X32_ReadCR0(void);
UINT32 K2_CALLCONV_REGS X32_ReadCR2(void);
UINT32 K2_CALLCONV_REGS X32_ReadCR3(void);

UINT32 K2_CALLCONV_REGS X32_ReadEFLAGS(void);

void   K2_CALLCONV_REGS X32_LoadTR(UINT32 aSelector);

void   K2_CALLCONV_REGS X32_LoadLDT(UINT32 aSelector);
UINT32 K2_CALLCONV_REGS X32_StoreLDT(void);

void   K2_CALLCONV_REGS X32_SetFS(UINT32 aSelector);
UINT32 K2_CALLCONV_REGS X32_GetFSData(UINT32 aOffset);

void   K2_CALLCONV_REGS X32_LoadCR3(UINT32 aPageDirPhysAddr);

void   K2_CALLCONV_REGS X32_CallCPUID(X32_CPUID *apIo);

void                    X32_CacheFlushAll(void);
void   K2_CALLCONV_REGS X32_CacheFlushLine(UINT32 aVirtAddr);

void   K2_CALLCONV_REGS X32_TLBInvalidateAll(void);

void   K2_CALLCONV_REGS X32_TLBInvalidatePage(UINT32 aVirtAddr);

UINT32 K2_CALLCONV_REGS X32_SetCoreInterruptMask(UINT32 aMaskBits);
UINT32 K2_CALLCONV_REGS X32_GetCoreInterruptMask(void);

void   K2_CALLCONV_REGS X32_IoWait(void);
void   K2_CALLCONV_REGS X32_IoWrite8(UINT8 aValue, UINT16 aPort);
void   K2_CALLCONV_REGS X32_IoWrite16(UINT16 aValue, UINT16 aPort);
void   K2_CALLCONV_REGS X32_IoWrite32(UINT32 aValue, UINT16 aPort);
UINT8  K2_CALLCONV_REGS X32_IoRead8(UINT16 aPort);
UINT16 K2_CALLCONV_REGS X32_IoRead16(UINT16 aPort);
UINT32 K2_CALLCONV_REGS X32_IoRead32(UINT16 aPort);

UINT32 K2_CALLCONV_REGS X32_MSR_Read32(UINT32 aRegNum);
void   K2_CALLCONV_REGS X32_MSR_Write32(UINT32 aRegNum, UINT32 aValue);

/*-------------------------------------------------------------------------------*/

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // K2_TARGET_ARCH_IS_INTEL
#endif  // K2_TOOLCHAIN_IS_GCC

#endif // __K2ARCHX32_H
