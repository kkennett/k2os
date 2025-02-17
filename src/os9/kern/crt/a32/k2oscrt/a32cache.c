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
#include "a32crtkern.h"

static UINT32           sgCacheInit = 0;
static A32_CACHEINFO    sgCacheInfo = { 0, };

void A32_DCacheFlushAll(void)
{
    UINT32  level;
    UINT32  nset;
    UINT32  nway;
    UINT32  ixset;
    UINT32  ixway;
    UINT32  setBit;
    UINT32  wayBit;
    UINT32  v;

    for(level=0;level<sgCacheInfo.mNumCacheLevels;level++)
    {
        if (0 == sgCacheInfo.mCacheConfig[level].mDSizeBytes)
            break;
        nset = sgCacheInfo.mCacheConfig[level].mDNumSets;
        nway = sgCacheInfo.mCacheConfig[level].mDNumWays;
        setBit = sgCacheInfo.mCacheConfig[level].mDSetBit;
        wayBit = sgCacheInfo.mCacheConfig[level].mDWayBit;
        ixset = 0;
        do {
            v = (setBit*ixset) | (level<<1);
            ixway = 0;
            do {
                A32_DCacheFlushSetWay(v);
                v += wayBit;
            } while (++ixway < nway);
        } while (++ixset < nset);
    }
    A32_DSB();
}

void A32_DCacheInvalidateAll(void)
{
    A32_REG_SCTRL sctrl;
    UINT32        level;
    UINT32        nset;
    UINT32        nway;
    UINT32        ixset;
    UINT32        ixway;
    UINT32        setBit;
    UINT32        wayBit;
    UINT32        v;

    /* don't do this when caches are on */
    sctrl.mAsUINT32 = A32_ReadSCTRL();
    while (sctrl.Bits.mC);

    for(level=0;level<sgCacheInfo.mNumCacheLevels;level++)
    {
        if (0 == sgCacheInfo.mCacheConfig[level].mDSizeBytes)
            break;
        nset = sgCacheInfo.mCacheConfig[level].mDNumSets;
        nway = sgCacheInfo.mCacheConfig[level].mDNumWays;
        setBit = sgCacheInfo.mCacheConfig[level].mDSetBit;
        wayBit = sgCacheInfo.mCacheConfig[level].mDWayBit;
        ixset = 0;
        do {
            v = (setBit*ixset) | (level<<1);
            ixway = 0;
            do {
                A32_DCacheInvalidateSetWay(v);
                v += wayBit;
            } while (++ixway < nway);
        } while (++ixset < nset);
    }
    A32_DSB();
}

void A32_DCacheFlushInvalidateAll(void)
{
    UINT32  level;
    UINT32  nset;
    UINT32  nway;
    UINT32  ixset;
    UINT32  ixway;
    UINT32  setBit;
    UINT32  wayBit;
    UINT32  v;

    for(level=0;level<sgCacheInfo.mNumCacheLevels;level++)
    {
        if (0 == sgCacheInfo.mCacheConfig[level].mDSizeBytes)
            break;
        nset = sgCacheInfo.mCacheConfig[level].mDNumSets;
        nway = sgCacheInfo.mCacheConfig[level].mDNumWays;
        setBit = sgCacheInfo.mCacheConfig[level].mDSetBit;
        wayBit = sgCacheInfo.mCacheConfig[level].mDWayBit;
        ixset = 0;
        do {
            v = (setBit*ixset) | (level<<1);
            ixway = 0;
            do {
                A32_DCacheFlushInvalidateSetWay(v);
                v += wayBit;
            } while (++ixway < nway);
        } while (++ixset < nset);
    }
    A32_DSB();
}

void A32_DCacheFlushRange(UINT32 aVirtAddr, UINT32 aBytes)
{
    UINT32 lineVal;
    UINT32 endAddr;

    lineVal = sgCacheInfo.mDMinLineBytes - 1;
    endAddr = (aVirtAddr + aBytes + lineVal) & ~lineVal;
    aVirtAddr &= ~lineVal;
    lineVal++;
    do {
        A32_DCacheFlushMVA_PoC(aVirtAddr);
        aVirtAddr += lineVal;
    } while (aVirtAddr != endAddr);
    A32_DSB();
}

void A32_DCacheFlushRange_PoU(UINT32 aVirtAddr, UINT32 aBytes)
{
    UINT32 lineVal;
    UINT32 endAddr;
    
    lineVal = sgCacheInfo.mDMinLineBytes - 1;
    endAddr = (aVirtAddr + aBytes + lineVal) & ~lineVal;
    aVirtAddr &= ~lineVal;
    lineVal++;
    do {
        A32_DCacheFlushMVA_PoU(aVirtAddr);
        aVirtAddr += lineVal;
    } while (aVirtAddr != endAddr);
    A32_DSB();
}

void A32_DCacheInvalidateRange(UINT32 aVirtAddr, UINT32 aBytes)
{
    UINT32 lineVal;
    UINT32 endAddr;
    
    lineVal = sgCacheInfo.mDMinLineBytes - 1;
    endAddr = (aVirtAddr + aBytes + lineVal) & ~lineVal;
    aVirtAddr &= ~lineVal;
    lineVal++;
    do {
        A32_DCacheInvalidateMVA_PoC(aVirtAddr);
        aVirtAddr += lineVal;
    } while (aVirtAddr != endAddr);
    A32_ISB();
}

void A32_DCacheFlushInvalidateRange(UINT32 aVirtAddr, UINT32 aBytes)
{
    UINT32 lineVal;
    UINT32 endAddr;
    
    lineVal = sgCacheInfo.mDMinLineBytes - 1;
    endAddr = (aVirtAddr + aBytes + lineVal) & ~lineVal;
    aVirtAddr &= ~lineVal;
    lineVal++;
    do {
        A32_DCacheFlushInvalidateMVA_PoC(aVirtAddr);
        aVirtAddr += lineVal;
    } while (aVirtAddr != endAddr);
    A32_DSB();
}

void A32_ICacheInvalidateAll(void)
{
    if (A32_ReadMPIDR() & 0x80000000)
    {
        A32_ICacheInvalidateAll_MP();
    }
    else
    {
        A32_ICacheInvalidateAll_UP();
    }
}

void A32_ICacheInvalidateRange(UINT32 aVirtAddr, UINT32 aBytes)
{
    UINT32 lineVal;
    UINT32 endAddr;

    lineVal = sgCacheInfo.mIMinLineBytes - 1;
    endAddr = (aVirtAddr + aBytes + lineVal) & ~lineVal;
    aVirtAddr &= ~lineVal;
    lineVal++;
    do {
        A32_ICacheInvalidateMVA(aVirtAddr);
        aVirtAddr += lineVal;
    } while (aVirtAddr != endAddr);
    A32_ISB();
}

static UINT32 sLog2_Ceil(UINT32 v)
{
    register UINT32 c;
    register UINT32 b;

    if (!v)
        return 32;
    
    /* find highest bit set */
    c = v;
    if (v&0xFFFF0000)
    {
        b = 16;
        v >>= 16;
    }
    else 
        b = 0;
    if (v&0xFF00)
    {
        b += 8;
        v >>= 8;
    }
    if (v&0xF0)
    {
        b += 4;
        v >>= 4;
    }
    if (v&0xC)
    {
        b += 2;
        v >>= 2;
    }
    if (v&2)
        b++;

    /* b has bit position of highest bit set */
    if ((c & ~(1<<b))==0)
        return b;
    return b+1;
}

A32_CACHEINFO const * A32CrtKern_InitCacheConfig(void)
{
    UINT32  val;
    UINT32  clidr;
    UINT32  ccsidr;
    UINT32  ix;
    UINT32  ctype[8];
    UINT32  A,L;

    if (sgCacheInit)
        return &sgCacheInfo;

    /* init and read cache configuration */
    K2MEM_Zero(&sgCacheInfo, sizeof(sgCacheInfo));

    val = A32_ReadCTR();
    sgCacheInfo.mIMinLineBytes = (1<<(val&0xF)) * 4;
    sgCacheInfo.mDMinLineBytes = (1<<((val>>16)&0xF)) * 4;
    sgCacheInfo.mICacheLargest = sgCacheInfo.mIMinLineBytes;
    sgCacheInfo.mDCacheLargest = sgCacheInfo.mDMinLineBytes;

    clidr = A32_ReadCLIDR();
    for(ix=0;ix<8;ix++)
    {
        ctype[ix] = (clidr & 0x7);
        clidr >>= 3;
        if (0 == ctype[ix])
            break;
    }
    sgCacheInfo.mNumCacheLevels = ix;

    for(ix=0;ix<sgCacheInfo.mNumCacheLevels;ix++)
    {
        if ((ctype[ix]==1) || (ctype[ix]==3))
        {
            A32_WriteCSSELR((ix<<1) | 1);
            ccsidr = A32_ReadCCSIDR();
            sgCacheInfo.mCacheConfig[ix].mIIDR = ccsidr;
            sgCacheInfo.mCacheConfig[ix].mILineBytes = 4*(1<<((ccsidr&0x7)+2));
            sgCacheInfo.mCacheConfig[ix].mINumSets = ((ccsidr>>13)&0x7FFF)+1;
            sgCacheInfo.mCacheConfig[ix].mINumWays = ((ccsidr>>3)&0x1F)+1;
            sgCacheInfo.mCacheConfig[ix].mISizeBytes = sgCacheInfo.mCacheConfig[ix].mINumWays*sgCacheInfo.mCacheConfig[ix].mINumSets*sgCacheInfo.mCacheConfig[ix].mILineBytes;
            if (sgCacheInfo.mICacheLargest < sgCacheInfo.mCacheConfig[ix].mISizeBytes)
                sgCacheInfo.mICacheLargest = sgCacheInfo.mCacheConfig[ix].mISizeBytes;
        }
        if (ctype[ix]!=1)
        {
            A32_WriteCSSELR((ix<<1));
            ccsidr = A32_ReadCCSIDR();
            sgCacheInfo.mCacheConfig[ix].mDIDR = ccsidr;
            sgCacheInfo.mCacheConfig[ix].mDLineBytes = 4*(1<<((ccsidr&0x7)+2));
            sgCacheInfo.mCacheConfig[ix].mDNumSets = ((ccsidr>>13)&0x7FFF)+1;
            sgCacheInfo.mCacheConfig[ix].mDNumWays = ((ccsidr>>3)&0x1F)+1;
            sgCacheInfo.mCacheConfig[ix].mDSizeBytes = sgCacheInfo.mCacheConfig[ix].mDNumWays*sgCacheInfo.mCacheConfig[ix].mDNumSets*sgCacheInfo.mCacheConfig[ix].mDLineBytes;
            if (sgCacheInfo.mDCacheLargest < sgCacheInfo.mCacheConfig[ix].mDSizeBytes)
                sgCacheInfo.mDCacheLargest = sgCacheInfo.mCacheConfig[ix].mDSizeBytes;
            /* calculate DSetBit and DWayBit - see TRM B3.12.31 */
            A = sLog2_Ceil(sgCacheInfo.mCacheConfig[ix].mDNumWays);
            L = sLog2_Ceil(sgCacheInfo.mCacheConfig[ix].mDLineBytes);
            sgCacheInfo.mCacheConfig[ix].mDSetBit = 1 << L;
            if (sgCacheInfo.mCacheConfig[ix].mDNumWays)
                sgCacheInfo.mCacheConfig[ix].mDWayBit = 1 << (32 - A);
        }
    }

    sgCacheInit = 1;

    A32_ICacheInvalidateAll_UP();
    A32_DSB();
    A32_ISB();

    return &sgCacheInfo;
}

void
K2OS_CacheOperation(
    K2OS_CACHEOP    aCacheOp,
    UINT32        aAddress,
    UINT32        aBytes
)
{
    A32_REG_SCTRL   sctrl;

    if (aCacheOp < K2OS_CACHEOP_FlushDataRange)
    {
        switch (aCacheOp)
        {
        case K2OS_CACHEOP_FlushDataAll:
            A32_DCacheFlushAll();
            break;
        case K2OS_CACHEOP_InvalidateDataAll:
            sctrl.mAsUINT32 = A32_ReadSCTRL();
            if (sctrl.Bits.mC == 0)
                A32_DCacheInvalidateAll();
            break;
        case K2OS_CACHEOP_FlushInvalidateDataAll:
            A32_DCacheFlushInvalidateAll();
            break;
        case K2OS_CACHEOP_InvalidateInstructionAll:
            A32_ICacheInvalidateAll();
            break;
        default:
            gShared.FuncTab.Panic("*** Unknown/Unsupported CacheOp\n");
            break;
        }
    }
    else if (aBytes > 0)
    {
        switch (aCacheOp)
        {
        case K2OS_CACHEOP_FlushDataRange:
            A32_DCacheFlushRange(aAddress, aBytes);
            break;
        case K2OS_CACHEOP_InvalidateDataRange:
            A32_DCacheInvalidateRange(aAddress, aBytes);
            break;
        case K2OS_CACHEOP_FlushInvalidateDataRange:
            A32_DCacheFlushInvalidateRange(aAddress, aBytes);
            break;
        case K2OS_CACHEOP_InvalidateInstructionRange:
            A32_ICacheInvalidateRange(aAddress, aBytes);
            break;
        default:
            gShared.FuncTab.Panic("*** Unknown/Unsupported CacheOp\n");
            break;
        }
    }
}
