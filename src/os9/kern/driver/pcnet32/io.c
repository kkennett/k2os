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

#include "pcnet32.h"

void    
PCNET32_ReadAPROM(
    PCNET32_DEVICE *apDevice,
    UINT32          aOffset,
    UINT32          aLength,
    UINT8 *         apBuffer
)
{
    UINT32 max;

    if ((0 == aLength) ||
        (aOffset >= PCNET32_APROM_LENGTH))
        return;

    max = PCNET32_APROM_LENGTH - aOffset;
    if (aLength > max)
        aLength = max;

    K2MEM_Copy(apBuffer, &apDevice->mCacheAPROM[aOffset], aLength);
}

UINT16  
PCNET32_ReadRDP(
    PCNET32_DEVICE *apDevice
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apDevice->mMappedIo)
    {
#endif
        return (UINT16)(K2MMIO_Read32(apDevice->mRegsVirtAddr + PCNET32_REG32_RDP_OFFSET) & 0x0000FFFF);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return (UINT16)(X32_IoRead32(apDevice->Res.Def.Io.Range.mBasePort + PCNET32_REG32_RDP_OFFSET) & 0x0000FFFF);
#endif
}

void    
PCNET32_WriteRDP(
    PCNET32_DEVICE *apDevice,
    UINT16          aValue
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apDevice->mMappedIo)
    {
#endif
        K2MMIO_Write32(apDevice->mRegsVirtAddr + PCNET32_REG32_RDP_OFFSET, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    else
    {
        X32_IoWrite32(aValue, apDevice->Res.Def.Io.Range.mBasePort + PCNET32_REG32_RDP_OFFSET);
    }
#endif
}

UINT16
PCNET32_ReadRAP(
    PCNET32_DEVICE *apDevice
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apDevice->mMappedIo)
    {
#endif
        return (UINT16)(K2MMIO_Read32(apDevice->mRegsVirtAddr + PCNET32_REG32_RAP_OFFSET) & 0x0000FFFF);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return (UINT16)(X32_IoRead32(apDevice->Res.Def.Io.Range.mBasePort + PCNET32_REG32_RAP_OFFSET) & 0x0000FFFF);
#endif
}

void
PCNET32_WriteRAP(
    PCNET32_DEVICE *apDevice,
    UINT16          aValue
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apDevice->mMappedIo)
    {
#endif
        K2MMIO_Write32(apDevice->mRegsVirtAddr + PCNET32_REG32_RAP_OFFSET, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    else
    {
        X32_IoWrite32(aValue, apDevice->Res.Def.Io.Range.mBasePort + PCNET32_REG32_RAP_OFFSET);
    }
#endif
}

UINT16
PCNET32_ReadRESET(
    PCNET32_DEVICE *apDevice
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apDevice->mMappedIo)
    {
#endif
        return (UINT16)(K2MMIO_Read32(apDevice->mRegsVirtAddr + PCNET32_REG32_RESET_OFFSET) & 0x0000FFFF);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return (UINT16)(X32_IoRead32(apDevice->Res.Def.Io.Range.mBasePort + PCNET32_REG32_RESET_OFFSET) & 0x0000FFFF);
#endif
}

void
PCNET32_WriteRESET(
    PCNET32_DEVICE *apDevice,
    UINT16          aValue
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apDevice->mMappedIo)
    {
#endif
        K2MMIO_Write32(apDevice->mRegsVirtAddr + PCNET32_REG32_RESET_OFFSET, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    else
    {
        X32_IoWrite32(aValue, apDevice->Res.Def.Io.Range.mBasePort + PCNET32_REG32_RESET_OFFSET);
    }
#endif
}

UINT16
PCNET32_ReadBDP(
    PCNET32_DEVICE *apDevice
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apDevice->mMappedIo)
    {
#endif
        return (UINT16)(K2MMIO_Read32(apDevice->mRegsVirtAddr + PCNET32_REG32_BDP_OFFSET) & 0x0000FFFF);
#if K2_TARGET_ARCH_IS_INTEL
    }
    return (UINT16)(X32_IoRead32(apDevice->Res.Def.Io.Range.mBasePort + PCNET32_REG32_BDP_OFFSET) & 0x0000FFFF);
#endif
}

void
PCNET32_WriteBDP(
    PCNET32_DEVICE *apDevice,
    UINT16          aValue
)
{
#if K2_TARGET_ARCH_IS_INTEL
    if (apDevice->mMappedIo)
    {
#endif
        K2MMIO_Write32(apDevice->mRegsVirtAddr + PCNET32_REG32_BDP_OFFSET, aValue);
#if K2_TARGET_ARCH_IS_INTEL
    }
    else
    {
        X32_IoWrite32(aValue, apDevice->Res.Def.Io.Range.mBasePort + PCNET32_REG32_BDP_OFFSET);
    }
#endif
}

void    
PCNET32_WriteCSR(
    PCNET32_DEVICE *apDevice,
    UINT32          aIndex,
    UINT16          aValue
)
{
    PCNET32_WriteRAP(apDevice, (UINT16)aIndex);
    PCNET32_WriteRDP(apDevice, aValue);
}

UINT16  
PCNET32_ReadCSR(
    PCNET32_DEVICE *apDevice,
    UINT32          aIndex
)
{
    PCNET32_WriteRAP(apDevice, (UINT16)aIndex);
    return PCNET32_ReadRDP(apDevice);
}

void    
PCNET32_WriteBCR(
    PCNET32_DEVICE *apDevice,
    UINT32          aIndex,
    UINT16          aValue
)
{
    PCNET32_WriteRAP(apDevice, (UINT16)aIndex);
    PCNET32_WriteBDP(apDevice, aValue);
}

UINT16  
PCNET32_ReadBCR(
    PCNET32_DEVICE *apDevice,
    UINT32          aIndex
)
{
    PCNET32_WriteRAP(apDevice, (UINT16)aIndex);
    return PCNET32_ReadBDP(apDevice);
}

void    
PCNET32_WriteANR(
    PCNET32_DEVICE *apDevice,
    UINT32          aIndex,
    UINT16          aValue
)
{
    PCNET32_WriteBCR(apDevice, 33, PCNET32_PHYAD_INTERNAL | (aIndex & 0x1F));
    PCNET32_WriteBCR(apDevice, 34, aValue);
}

UINT16  
PCNET32_ReadANR(
    PCNET32_DEVICE *apDevice,
    UINT32          aIndex
)
{
    PCNET32_WriteBCR(apDevice, 33, PCNET32_PHYAD_INTERNAL | (aIndex & 0x1F));
    return PCNET32_ReadBCR(apDevice, 34);
}

void    
PCNET32_WritePciCfg(
    PCNET32_DEVICE *apDevice,
    UINT32          aOffset,
    UINT32          aWidth,
    UINT32          aValue
)
{
    K2OS_PCIBUS_CFG_WRITE_IN    writeIn;
    K2OS_RPC_CALLARGS           callArgs;
    UINT32                      rpcOut;
    K2STAT                      stat;

    K2MEM_Zero(&writeIn, sizeof(writeIn));
    if (aWidth == 32)
    {
        writeIn.mValue = aValue;
    }
    else if (aWidth == 16)
    {
        writeIn.mValue = (UINT16)(aValue & 0x0000FFFF);
    }
    else if (aWidth == 8)
    {
        writeIn.mValue = (UINT8)(aValue & 0x000000FF);
    }
    else
    {
        K2OSKERN_Debug("PCNET32(%08X): Bad width on PciCfg Write (%d)\n", apDevice, aWidth);
        return;
    }
    writeIn.Loc.mOffset = aOffset;
    writeIn.Loc.mWidth = aWidth;
    writeIn.mBusChildId = apDevice->InstInfo.mBusChildId;

    K2MEM_Zero(&callArgs, sizeof(callArgs));
    callArgs.mMethodId = K2OS_PCIBUS_METHOD_CFG_WRITE;
    callArgs.mpInBuf = (UINT8 const *)&writeIn;
    callArgs.mInBufByteCount = sizeof(writeIn);
    rpcOut = 0;
    stat = K2OS_Rpc_Call(apDevice->mhBusRpc, &callArgs, &rpcOut);
    if (K2STAT_IS_ERROR(stat))
    {
        K2OSKERN_Debug("PCNET32(%08X): PciCfg%d Write(0x%04X) failed(%08X)\n", apDevice, aWidth, aOffset, stat);
    }
}

UINT32   
PCNET32_ReadPciCfg(
    PCNET32_DEVICE *apDevice,
    UINT32          aOffset,
    UINT32          aWidth
)
{
    K2OS_PCIBUS_CFG_READ_IN readIn;
    K2OS_RPC_CALLARGS       callArgs;
    UINT32                  rpcOut;
    K2STAT                  stat;
    UINT32                  result;

    K2MEM_Zero(&readIn, sizeof(readIn));
    if ((aWidth != 32) &&
        (aWidth != 16) &&
        (aWidth != 8))
    {
        K2OSKERN_Debug("PCNET32(%08X): Bad width on PciCfg Read (%d)\n", apDevice, aWidth);
        return 0;
    }
    K2MEM_Zero(&callArgs, sizeof(callArgs));
    readIn.Loc.mOffset = aOffset;
    readIn.Loc.mWidth = aWidth;
    readIn.mBusChildId = apDevice->InstInfo.mBusChildId;
    callArgs.mMethodId = K2OS_PCIBUS_METHOD_CFG_READ;
    callArgs.mpInBuf = (UINT8 const *)&readIn;
    callArgs.mInBufByteCount = sizeof(readIn);
    callArgs.mpOutBuf = (UINT8 *)&result;
    callArgs.mOutBufByteCount = sizeof(result);
    result = rpcOut = 0;
    stat = K2OS_Rpc_Call(apDevice->mhBusRpc, &callArgs, &rpcOut);
    if ((K2STAT_IS_ERROR(stat)) || (rpcOut != sizeof(UINT32)))
    {
        K2OSKERN_Debug("PCNET32(%08X): PciCfg%d Read(0x%04X) failed(%08X)\n", apDevice, aWidth, aOffset, stat);
    }
    return result;
}

