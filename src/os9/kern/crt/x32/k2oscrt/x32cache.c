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
#include "x32crtkern.h"

static UINT32           sgCacheInit = 0;
static X32_CPUID        sgCPUID1;
static X32_CACHEINFO    sgCacheInfo = { 0, };

typedef struct _rawCacheInfo
{
    UINT8   mId;
    UINT8   mWays; 
    UINT16  mFlags;
    UINT32  mSizeBytes;
    UINT32  mLineBytes;
} rawCacheInfo;

#define CIF_L1          0x01
#define CIF_L2          0x02
#define CIF_L3          0x03
#define CIF_LEVELMASK   0x03

#define CIF_DATA        0x80
#define CIF_INST        0x40

static rawCacheInfo const sgRawCacheInfo[] = 
{
    { 0x06,  4, CIF_L1 | CIF_INST, 8*1024, 32 },
    { 0x08,  4, CIF_L1 | CIF_INST, 16*1024, 32 },
    { 0x09,  4, CIF_L1 | CIF_INST, 32*1024, 64 },
    { 0x0A,  2, CIF_L1 | CIF_DATA, 8*1024, 32 },
    { 0x0C,  4, CIF_L1 | CIF_DATA, 16*1024, 32 },
    { 0x0D,  4, CIF_L1 | CIF_DATA, 16*1024, 64 },
    { 0x0E,  6, CIF_L1 | CIF_DATA, 24*1024, 64 },
    { 0x21,  8, CIF_L2           , 256*1024, 64 },
    { 0x22,  4, CIF_L3           , 512*1024, 64 },
    { 0x23,  8, CIF_L3           , 1024*1024, 64 },
    { 0x25,  8, CIF_L3           , 2*1024*1024, 64 },
    { 0x29,  8, CIF_L3           , 4*1024*1024, 64 },
    { 0x2C,  8, CIF_L1 | CIF_DATA, 32*1024, 64 },
    { 0x30,  8, CIF_L1 | CIF_INST, 32*1024, 64 },
    { 0x41,  4, CIF_L2           , 128*1024, 32 },
    { 0x42,  4, CIF_L2           , 256*1024, 32 },
    { 0x43,  4, CIF_L2           , 512*1024, 32 },
    { 0x44,  4, CIF_L2           , 1024*1024, 32 },
    { 0x45,  4, CIF_L2           , 2*1024*1024, 32 },
    { 0x46,  4, CIF_L3           , 4*1024*1024, 64 },
    { 0x47,  8, CIF_L3           , 8*1024*1024, 64 },
    { 0x48, 12, CIF_L2           , 3*1024*1024, 64 },
    { 0x4A, 12, CIF_L3           , 6*1024*1024, 64 },
    { 0x4B, 16, CIF_L3           , 8*1024*1024, 64 },
    { 0x4C, 12, CIF_L3           , 12*1024*1024, 64 },
    { 0x4D, 16, CIF_L3           , 16*1024*1024, 64 },
    { 0x4E, 24, CIF_L2           , 6*1024*1024, 64 },
    { 0x60,  8, CIF_L1 | CIF_DATA, 16*1024, 64 },
    { 0x66,  4, CIF_L1 | CIF_DATA, 8*1024, 64 },
    { 0x67,  4, CIF_L1 | CIF_DATA, 16*1024, 64 },
    { 0x68,  4, CIF_L1 | CIF_DATA, 32*1024, 64 },
    { 0x78,  4, CIF_L2           , 1*1024*1024, 64 },
    { 0x79,  8, CIF_L2           , 128*1024, 64 },
    { 0x7A,  8, CIF_L2           , 256*1024, 64 },
    { 0x7B,  8, CIF_L2           , 512*1024, 64 },
    { 0x7C,  8, CIF_L2           , 1*1024*1024, 64 },
    { 0x7D,  8, CIF_L2           , 2*1024*1024, 64 },
    { 0x7F,  2, CIF_L2           , 512*1024, 64 },
    { 0x80,  8, CIF_L2           , 512*1024, 64 },
    { 0x82,  8, CIF_L2           , 256*1024, 32 },
    { 0x83,  8, CIF_L2           , 512*1024, 32 },
    { 0x84,  8, CIF_L2           , 1024*1024, 32 },
    { 0x85,  8, CIF_L2           , 2*1024*1024, 32 },
    { 0x86,  4, CIF_L2           , 512*1024, 64 },
    { 0x87,  8, CIF_L2           , 1024*1024, 64 },
    { 0xD0,  4, CIF_L3           , 512*1024, 64 },
    { 0xD1,  4, CIF_L3           , 1024*1024, 64 },
    { 0xD2,  4, CIF_L3           , 2*1024*1024, 64 },
    { 0xD6,  8, CIF_L3           , 1024*1024, 64 },
    { 0xD7,  8, CIF_L3           , 2*1024*1024, 64 },
    { 0xD8,  8, CIF_L3           , 4*1024*1024, 64 },
    { 0xDC, 12, CIF_L3           , 1536*1024, 64 },
    { 0xDD, 12, CIF_L3           , 3*1024*1024, 64 },
    { 0xDE, 12, CIF_L3           , 6*1024*1024, 64 },
    { 0xE2, 16, CIF_L3           , 2*1024*1024, 64 },
    { 0xE3, 16, CIF_L3           , 4*1024*1024, 64 },
    { 0xE4, 16, CIF_L3           , 8*1024*1024, 64 },
    { 0xEA, 24, CIF_L3           , 12*1024*1024, 64 },
    { 0xEB, 24, CIF_L3           , 18*1024*1024, 64 },
    { 0xEC, 24, CIF_L3           , 24*1024*1024, 64 }
};
#define NUM_CACHEINFO (sizeof(sgRawCacheInfo)/sizeof(rawCacheInfo))

static rawCacheInfo const sgByte49_1 = { 0x49, 16, CIF_L3, 4*1024*1024, 64 };
static rawCacheInfo const sgByte49_2 = { 0x49, 16, CIF_L2, 4*1024*1024, 64 };

static void sParseCacheInfoByte(UINT32 aByte)
{
    UINT32                ix;
    rawCacheInfo const *  pInfo;
    
    if ((!aByte) || (aByte==0xFF))
        return;

    if (aByte==0x49)
    {
        if ((((sgCPUID1.EAX & X32_CPUID1_MASK_FAMILY) >> X32_CPUID1_SHFT_FAMILY)==0x0F) &&
            (((sgCPUID1.EAX & X32_CPUID1_MASK_MODEL) >> X32_CPUID1_SHFT_MODEL)==0x06))
            pInfo = &sgByte49_1;
        else
            pInfo = &sgByte49_2;
    }
    else
    {
        pInfo = sgRawCacheInfo;
        for(ix=0;ix<NUM_CACHEINFO;ix++)
        {
            if (pInfo->mId == aByte)
                break;
            pInfo++;
        }
        if (ix==NUM_CACHEINFO)
            return;
    }

    /* we have some information */
    ix = pInfo->mFlags & CIF_LEVELMASK;
    if (ix == CIF_L1)
    {
        if ((!sgCacheInfo.mCacheLineBytes) || (pInfo->mLineBytes < sgCacheInfo.mCacheLineBytes))
            sgCacheInfo.mCacheLineBytes = pInfo->mLineBytes;

        if (pInfo->mFlags & CIF_DATA)
        {
            /* L1 data cache info */
            if (pInfo->mSizeBytes > sgCacheInfo.mL1CacheSizeBytesD)
                sgCacheInfo.mL1CacheSizeBytesD = pInfo->mSizeBytes;
        }
        else if (pInfo->mFlags & CIF_INST)
        {
            /* L1 instruction cache info */
            if (pInfo->mSizeBytes > sgCacheInfo.mL1CacheSizeBytesI)
                sgCacheInfo.mL1CacheSizeBytesI = pInfo->mSizeBytes;
        }
    }
    else if (ix == CIF_L2)
    {
        /* L2 cache info */
        if (sgCacheInfo.mL2CacheSizeBytes < pInfo->mSizeBytes)
            sgCacheInfo.mL2CacheSizeBytes = pInfo->mSizeBytes;
    }
    else if (ix == CIF_L3)
    {
        /* L3 cache info */
        if (sgCacheInfo.mL3CacheSizeBytes < pInfo->mSizeBytes)
            sgCacheInfo.mL3CacheSizeBytes = pInfo->mSizeBytes;
    }
}

static void sParseCacheInfo(X32_CPUID *pId)
{
    if (!(pId->EAX & 0x80000000))
    {
        sParseCacheInfoByte((pId->EAX >> 8) & 0xFF);
        sParseCacheInfoByte((pId->EAX >> 16) & 0xFF);
        sParseCacheInfoByte((pId->EAX >> 24) & 0xFF);
    }
    if (!(pId->EBX & 0x80000000))
    {
        sParseCacheInfoByte(pId->EBX & 0xFF);
        sParseCacheInfoByte((pId->EBX >> 8) & 0xFF);
        sParseCacheInfoByte((pId->EBX >> 16) & 0xFF);
        sParseCacheInfoByte((pId->EBX >> 24) & 0xFF);
    }
    if (!(pId->ECX & 0x80000000))
    {
        sParseCacheInfoByte(pId->ECX & 0xFF);
        sParseCacheInfoByte((pId->ECX >> 8) & 0xFF);
        sParseCacheInfoByte((pId->ECX >> 16) & 0xFF);
        sParseCacheInfoByte((pId->ECX >> 24) & 0xFF);
    }
    if (!(pId->EDX & 0x80000000))
    {
        sParseCacheInfoByte(pId->EDX & 0xFF);
        sParseCacheInfoByte((pId->EDX >> 8) & 0xFF);
        sParseCacheInfoByte((pId->EDX >> 16) & 0xFF);
        sParseCacheInfoByte((pId->EDX >> 24) & 0xFF);
    }
}

#define X32_MAX_CPUID2  16

X32_CACHEINFO const * X32CrtKern_InitCacheConfig(void)
{
    X32_CPUID   io;
    int         ix;
    X32_CPUID   cpuid2[X32_MAX_CPUID2];
    int         maxType2;

    if (sgCacheInit)
        return &sgCacheInfo;

    /* get cache/tlb info so that CacheOp will work */
    io.EAX = 0;
    X32_CallCPUID(&io);
    if (io.EAX < 2)
        return NULL;
    sgCPUID1.EAX = 1;
    X32_CallCPUID(&sgCPUID1);
    if (sgCPUID1.EDX & (1<<19))
        sgCacheInfo.mCacheLineBytes = ((sgCPUID1.EBX >> 8) & 0xFF) * 8;
    else
        sgCacheInfo.mCacheLineBytes = 0;
    sgCacheInfo.mL1CacheSizeBytesI = 0;
    sgCacheInfo.mL1CacheSizeBytesD = 0;
    sgCacheInfo.mL2CacheSizeBytes = 0;
    sgCacheInfo.mL3CacheSizeBytes = 0;

    cpuid2[0].EAX = 2;
    X32_CallCPUID(&cpuid2[0]);
    sParseCacheInfo(&cpuid2[0]);

    maxType2 = (cpuid2[0].EAX&0xFF);
    if (maxType2 > X32_MAX_CPUID2)
    {
        cpuid2[0].EAX = (cpuid2[0].EAX & 0xFFFFFF00) | X32_MAX_CPUID2;
        maxType2 = X32_MAX_CPUID2;
    }

    ix = 1;
    while (ix < maxType2)
    {
        cpuid2[ix].EAX = 2;
        X32_CallCPUID(&cpuid2[ix]);
        sParseCacheInfo(&cpuid2[ix]);
        ix++;
    }

    if (!sgCacheInfo.mCacheLineBytes)
        sgCacheInfo.mCacheLineBytes = 32;
    if (!sgCacheInfo.mL1CacheSizeBytesD)
    {
        if (!sgCacheInfo.mL1CacheSizeBytesI)
            sgCacheInfo.mL1CacheSizeBytesD = 32*1024;
    }
    if (!sgCacheInfo.mL1CacheSizeBytesI)
         sgCacheInfo.mL1CacheSizeBytesI = sgCacheInfo.mL1CacheSizeBytesD;

    sgCacheInit = 1;

    return &sgCacheInfo;
}

void
K2OS_CacheOperation(
    K2OS_CACHEOP  aCacheOp,
    UINT32      aAddress,
    UINT32      aBytes
)
{
    UINT32 lineVal;
    UINT32 endAddr;

    if ((aCacheOp == K2OS_CACHEOP_FlushDataAll) ||
        (aCacheOp == K2OS_CACHEOP_InvalidateDataAll) ||
        (aCacheOp == K2OS_CACHEOP_FlushInvalidateDataAll) ||
        (aCacheOp == K2OS_CACHEOP_InvalidateInstructionAll))
    {
        X32_CacheFlushAll();
    }
    else if (aBytes != 0)
    {
        lineVal = sgCacheInfo.mCacheLineBytes - 1;
        endAddr = (aAddress + aBytes + lineVal) & ~lineVal;
        aAddress &= ~lineVal;
        lineVal++;
        do {
            X32_CacheFlushLine(aAddress);
            aAddress += lineVal;
        } while (aAddress != endAddr);

        K2_CpuFullBarrier();
    }
}
