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

#ifndef __K2ARCHA32_H
#define __K2ARCHA32_H

/* --------------------------------------------------------------------------------- */

#include <k2systype.h>

#if K2_TOOLCHAIN_IS_GCC
#if K2_TARGET_ARCH_IS_ARM

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------------*/

static __inline__
__attribute__((always_inline))
UINT32 A32_ReadFramePointer(void)
{
    register UINT32 result;
    __asm__("mov %[output], r11\n" : [output] "=r" (result));
    return result;
}

static __inline__
__attribute__((always_inline))
UINT32 A32_ReadStackPointer(void)
{
    register UINT32 result;
    __asm__("mov %[output], r13\n" : [output] "=r" (result));
    return result;
}

/*-------------------------------------------------------------------------------*/

UINT32  A32_SetCoreInterruptMask(UINT32 aMaskBits);
UINT32  A32_GetCoreInterruptMask(void);

void A32_ResetStackAndJump(UINT32 aNewStackPtr, UINT32 aJumpTarget);

UINT32  A32_ReadMIDR(void);

UINT32  A32_ReadMPIDR(void);

UINT32  A32_ReadIDPFR1(void);

UINT32  A32_ReadSCR(void);
void    A32_WriteSCR(UINT32 aVal);

UINT32  A32_ReadSCTRL(void);
void    A32_WriteSCTRL(UINT32 aVal);

UINT32  A32_ReadAUXCTRL(void);
void    A32_WriteAUXCTRL(UINT32 aVal);

UINT32  A32_ReadDACR(void);
void    A32_WriteDACR(UINT32 aVal);

UINT32  A32_ReadCPACR(void);
void    A32_WriteCPACR(UINT32 aVal);

UINT32  A32_ReadCBAR(void);

/*-------------------------------------------------------------------------------*/

UINT32  A32_ReadCONTEXT(void);
void    A32_WriteCONTEXT(UINT32 aVal);

UINT32  A32_ReadTPIDRPRW(void);
void    A32_WriteTPIDRPRW(UINT32 aVal);

UINT32  A32_ReadTPIDRURO(void);
void    A32_WriteTPIDRURO(UINT32 aVal);

UINT32  A32_ReadTPIDRURW(void);
void    A32_WriteTPIDRURW(UINT32 aVal);

/*-------------------------------------------------------------------------------*/

UINT32  A32_ReadDFAR(void);

UINT32  A32_ReadDFSR(void);

UINT32  A32_ReadIFAR(void);

UINT32  A32_ReadIFSR(void);

/*-------------------------------------------------------------------------------*/

UINT32  A32_ReadTTBR0(void);
void    A32_WriteTTBR0(UINT32 aVal);

UINT32  A32_ReadTTBR1(void);
void    A32_WriteTTBR1(UINT32 aVal);

UINT32  A32_ReadTTBCR(void);
void    A32_WriteTTBCR(UINT32 aVal);

UINT32  A32_TranslateVirtPrivRead(UINT32 aVal);

UINT32  A32_TranslateVirtPrivWrite(UINT32 aVal);

/*-------------------------------------------------------------------------------*/

UINT32  A32_ReadCPSR(void);

UINT32  A32_ReadCTR(void);

UINT32  A32_ReadCCSIDR(void);

UINT32  A32_ReadCLIDR(void);

UINT32  A32_ReadCSSELR(void);
void    A32_WriteCSSELR(UINT32 aVal);

void    A32_BPInvalidateAll_UP(void);
void    A32_BPInvalidateAll_MP(void);
void    A32_BPInvalidateMVA_UP(UINT32 aMVA);

void    A32_DCacheFlushInvalidateMVA_PoC(UINT32 aMVA);
void    A32_DCacheFlushInvalidateSetWay(UINT32 aSetWay);

void    A32_DCacheFlushMVA_PoC(UINT32 aMVA);
void    A32_DCacheFlushMVA_PoU(UINT32 aMVA);

void    A32_DCacheFlushSetWay(UINT32 aSetWay);

void    A32_DCacheInvalidateMVA_PoC(UINT32 aMVA);
void    A32_DCacheInvalidateSetWay(UINT32 aSetWay);

void    A32_ICacheInvalidateAll_UP(void);
void    A32_ICacheInvalidateAll_MP(void);
void    A32_ICacheInvalidateMVA(UINT32 aMVA);

/*-------------------------------------------------------------------------------*/

void    A32_TLBInvalidateAll_UP(void);
void    A32_TLBInvalidateMVA_UP_OneASID(UINT32 aMVA, UINT32 aASID);
void    A32_TLBInvalidateASID_UP(UINT32 aASID);
void    A32_TLBInvalidateMVA_UP_AllASID(UINT32 aMVA);

void    A32_TLBInvalidateAll_MP(void);
void    A32_TLBInvalidateMVA_MP_OneASID(UINT32 aMVA, UINT32 aASID);
void    A32_TLBInvalidateASID_MP(UINT32 aASID);
void    A32_TLBInvalidateMVA_MP_AllASID(UINT32 aMVA);

/*-------------------------------------------------------------------------------*/

static __inline__ 
__attribute__((always_inline))
void A32_ISB(void)
{
    __asm__("isb\n");
}

static __inline__ 
__attribute__((always_inline))
void A32_DSB(void)
{
    __asm__("dsb\n");
}

static __inline__ 
__attribute__((always_inline))
void A32_DMB(void)
{
    __asm__("dmb\n");
}

static __inline__ 
__attribute__((always_inline))
void A32_SEV(void)
{
    __asm__("sev\n");
}

static __inline__ 
__attribute__((always_inline))
void A32_WFE(void)
{
    __asm__("wfe\n");
}

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // K2_TARGET_ARCH_IS_ARM
#endif  // K2_TOOLCHAIN_IS_GCC

/* --------------------------------------------------------------------------------- */

#endif // __K2ARCHA32_H
