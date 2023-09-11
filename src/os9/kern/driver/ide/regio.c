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

#include "ide.h"

UINT8  
IDE_Read8(
    IDE_CHANNEL *   apChannel,
    IdeChanRegType  aReg
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apChannel->ChanRegs.mIsPhys)
    {
#endif
        return K2MMIO_Read8(apChannel->ChanRegs.mRegAddr[aReg]);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead8(apChannel->ChanRegs.mRegAddr[aReg]);
#endif
}

void   
IDE_Write8(
    IDE_CHANNEL *   apChannel,
    IdeChanRegType  aReg,
    UINT8           aValue
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apChannel->ChanRegs.mIsPhys)
    {
#endif
        K2MMIO_Write8(apChannel->ChanRegs.mRegAddr[aReg], aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite8(aValue, apChannel->ChanRegs.mRegAddr[aReg]);
#endif
}

UINT16 
IDE_Read16(
    IDE_CHANNEL *   apChannel,
    IdeChanRegType  aReg
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apChannel->ChanRegs.mIsPhys)
    {
#endif
        return K2MMIO_Read16(apChannel->ChanRegs.mRegAddr[aReg]);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead16(apChannel->ChanRegs.mRegAddr[aReg]);
#endif
}

void   
IDE_Write16(
    IDE_CHANNEL *   apChannel,
    IdeChanRegType  aReg,
    UINT16          aValue
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apChannel->ChanRegs.mIsPhys)
    {
#endif
        K2MMIO_Write16(apChannel->ChanRegs.mRegAddr[aReg], aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite16(aValue, apChannel->ChanRegs.mRegAddr[aReg]);
#endif
}

UINT32 
IDE_Read32(
    IDE_CHANNEL *   apChannel,
    IdeChanRegType  aReg
)
{
    UINT32 result;
#if K2_TARGET_ARCH_IS_INTEL
    if (apChannel->ChanRegs.mIsPhys)
    {
#endif
        result = K2MMIO_Read16(apChannel->ChanRegs.mRegAddr[aReg]);
        result |= ((UINT32)K2MMIO_Read16(apChannel->ChanRegs.mRegAddr[aReg])) << 16;
        return result;
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead32(apChannel->ChanRegs.mRegAddr[aReg]);
#endif
}

void   
IDE_Write32(
    IDE_CHANNEL *   apChannel,
    IdeChanRegType  aReg,
    UINT32          aValue
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apChannel->ChanRegs.mIsPhys)
    {
#endif
        K2MMIO_Write16(apChannel->ChanRegs.mRegAddr[aReg], (UINT16)(aValue & 0xFFFF));
        K2MMIO_Write16(apChannel->ChanRegs.mRegAddr[aReg], (UINT16)((aValue >> 16) & 0xFFFF));
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite32(aValue, apChannel->ChanRegs.mRegAddr[aReg]);
#endif
}

