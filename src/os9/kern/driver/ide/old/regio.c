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

UINT8 IDE_Read8_DATA(UINT32 aChannel)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        return K2MMIO_Read8(gChannel[aChannel].RegAddr.RW_DATA);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead8(gChannel[aChannel].RegAddr.RW_DATA);
#endif
}

void IDE_Write8_DATA(UINT32 aChannel, UINT8 aValue)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        K2MMIO_Write8(gChannel[aChannel].RegAddr.RW_DATA, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite8(aValue, gChannel[aChannel].RegAddr.RW_DATA);
#endif
}

UINT16 IDE_Read16_DATA(UINT32 aChannel)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        return K2MMIO_Read16(gChannel[aChannel].RegAddr.RW_DATA);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead16(gChannel[aChannel].RegAddr.RW_DATA);
#endif
}

void IDE_Write16_DATA(UINT32 aChannel, UINT16 aValue)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        K2MMIO_Write16(gChannel[aChannel].RegAddr.RW_DATA, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite16(aValue, gChannel[aChannel].RegAddr.RW_DATA);
#endif
}

UINT32 IDE_Read32_DATA(UINT32 aChannel)
{
    UINT32 result;

#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        result = K2MMIO_Read16(gChannel[aChannel].RegAddr.RW_DATA);
        result |= ((UINT32)K2MMIO_Read16(gChannel[aChannel].RegAddr.RW_DATA)) << 16;
        return result;
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead32(gChannel[aChannel].RegAddr.RW_DATA);
#endif
}

void IDE_Write32_DATA(UINT32 aChannel, UINT32 aValue)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        K2MMIO_Write16(gChannel[aChannel].RegAddr.RW_DATA, (UINT16)(aValue & 0xFFFF));
        K2MMIO_Write16(gChannel[aChannel].RegAddr.RW_DATA, (UINT16)((aValue >> 16) & 0xFFFF));
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite32(aValue, gChannel[aChannel].RegAddr.RW_DATA);
#endif
}

UINT8 IDE_Read8_ERROR(UINT32 aChannel)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        return K2MMIO_Read8(gChannel[aChannel].RegAddr.R_ERROR);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead8(gChannel[aChannel].RegAddr.R_ERROR);
#endif
}

void IDE_Write8_FEATURES(UINT32 aChannel, UINT8 aValue)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        K2MMIO_Write8(gChannel[aChannel].RegAddr.W_FEATURES, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite8(aValue, gChannel[aChannel].RegAddr.W_FEATURES);
#endif
}

UINT8 IDE_Read8_SECCOUNT0(UINT32 aChannel)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        return K2MMIO_Read8(gChannel[aChannel].RegAddr.RW_SECCOUNT0);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead8(gChannel[aChannel].RegAddr.RW_SECCOUNT0);
#endif
}

void IDE_Write8_SECCOUNT0(UINT32 aChannel, UINT8 aValue)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        K2MMIO_Write8(gChannel[aChannel].RegAddr.RW_SECCOUNT0, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite8(aValue, gChannel[aChannel].RegAddr.RW_SECCOUNT0);
#endif
}

UINT8 IDE_Read8_LBA_LOW(UINT32 aChannel)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        return K2MMIO_Read8(gChannel[aChannel].RegAddr.RW_LBA_LOW);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead8(gChannel[aChannel].RegAddr.RW_LBA_LOW);
#endif
}

void IDE_Write8_LBA_LOW(UINT32 aChannel, UINT8 aValue)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        K2MMIO_Write8(gChannel[aChannel].RegAddr.RW_LBA_LOW, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite8(aValue, gChannel[aChannel].RegAddr.RW_LBA_LOW);
#endif
}

UINT8 IDE_Read8_LBA_MID(UINT32 aChannel)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        return K2MMIO_Read8(gChannel[aChannel].RegAddr.RW_LBA_MID);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead8(gChannel[aChannel].RegAddr.RW_LBA_MID);
#endif
}

void IDE_Write8_LBA_MID(UINT32 aChannel, UINT8 aValue)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        K2MMIO_Write8(gChannel[aChannel].RegAddr.RW_LBA_MID, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite8(aValue, gChannel[aChannel].RegAddr.RW_LBA_MID);
#endif
}

UINT8 IDE_Read8_LBA_HI(UINT32 aChannel)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        return K2MMIO_Read8(gChannel[aChannel].RegAddr.RW_LBA_HI);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead8(gChannel[aChannel].RegAddr.RW_LBA_HI);
#endif
}

void IDE_Write8_LBA_HI(UINT32 aChannel, UINT8 aValue)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        K2MMIO_Write8(gChannel[aChannel].RegAddr.RW_LBA_HI, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite8(aValue, gChannel[aChannel].RegAddr.RW_LBA_HI);
#endif
}

UINT8 IDE_Read8_HDDEVSEL(UINT32 aChannel)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        return K2MMIO_Read8(gChannel[aChannel].RegAddr.RW_HDDEVSEL);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead8(gChannel[aChannel].RegAddr.RW_HDDEVSEL);
#endif
}

void IDE_Write8_HDDEVSEL(UINT32 aChannel, UINT8 aValue)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        K2MMIO_Write8(gChannel[aChannel].RegAddr.RW_HDDEVSEL, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite8(aValue, gChannel[aChannel].RegAddr.RW_HDDEVSEL);
#endif
}

UINT8 IDE_Read8_STATUS(UINT32 aChannel)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        return K2MMIO_Read8(gChannel[aChannel].RegAddr.R_STATUS);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead8(gChannel[aChannel].RegAddr.R_STATUS);
#endif
}

void IDE_Write8_COMMAND(UINT32 aChannel, UINT8 aValue)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        K2MMIO_Write8(gChannel[aChannel].RegAddr.W_COMMAND, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite8(aValue, gChannel[aChannel].RegAddr.W_COMMAND);
#endif
}

UINT8 IDE_Read8_ASR(UINT32 aChannel)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        return K2MMIO_Read8(gChannel[aChannel].RegAddr.R_ASR);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return X32_IoRead8(gChannel[aChannel].RegAddr.R_ASR);
#endif
}

void IDE_Write8_DCR(UINT32 aChannel, UINT8 aValue)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (gChannel[aChannel].Regs.mIsPhys)
    {
#endif
        K2MMIO_Write8(gChannel[aChannel].RegAddr.W_DCR, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    X32_IoWrite8(aValue, gChannel[aChannel].RegAddr.W_DCR);
#endif
}


