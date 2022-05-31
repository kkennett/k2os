//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2020, Kurt Kennett
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

#include "kern.h"

UINT32 
KernBit_LowestSet_Index(
    UINT32 x
)
{
    UINT32 bitNum;

    if (0 == x)
        return (UINT32)-1;

    if (0 == (x & 0x0000FFFF))
    {
        x >>= 16;
        bitNum = 16;
    }
    else
        bitNum = 0;
    if (0 == (x & 0x00FF))
    {
        x >>= 8;
        bitNum += 8;
    }
    if (0 == (x & 0x0F))
    {
        x >>= 4;
        bitNum += 4;
    }
    if (0 == (x & 0x3))
    {
        x >>= 2;
        bitNum += 2;
    }
    if (0 == (x & 1))
        bitNum++;
    return bitNum;
}

UINT32 
KernBit_ExtractLowestSet(
    UINT32 x
)
{
    UINT32 v;
    
    if (0 == x)
        return (UINT32)-1;

    v = x - 1;

    return v ^ (x | v);
}

UINT32 
KernBit_HighestSet_Index(
    UINT32 x
)
{
    UINT32 bitNum;

    if (0 == x)
        return (UINT32)-1;
    if (x & 0xFFFF0000)
    {
        bitNum = 16;
        x >>= 16;
    }
    else
        bitNum = 0;
    if (x & 0xFF00)
    {
        bitNum += 8;
        x >>= 8;
    }
    if (x & 0xF0)
    {
        bitNum += 4;
        x >>= 4;
    }
    if (x & 0xC)
    {
        bitNum += 2;
        x >>= 2;
    }
    if (x & 2)
        bitNum++;
    return bitNum;
}

UINT32 
KernBit_ExtractHighestSet(
    UINT32 x
)
{
    if (0 == x)
        return (UINT32)-1;
    return 1 << KernBit_HighestSet_Index(x);
}

BOOL 
KernBit_IsPowerOfTwo(
    UINT32 x
)
{
    return (x != 0) && (0 == (x & (x - 1)));
}

UINT32 
KernBit_LowestSet_Index64(
    UINT64 *px
)
{
    UINT64 v;
    UINT32 ret;

    v = *px;
    K2_ASSERT(0 != v);

    // get lowest bit set by itself
    v = (v | (v - 1)) ^ (v - 1);

    // now calculate the bit's index using arch-independent code
    if (v & 0xFFFFFFFF00000000)
    {
        ret = 32;
        v >>= 32;
    }
    else
        ret = 0;

    if (v & 0xFFFF0000)
    {
        ret += 16;
        v >>= 16;
    }

    if (v & 0xFF00)
    {
        ret += 8;
        v >>= 8;
    }

    if (v & 0xF0)
    {
        ret += 4;
        v >>= 4;
    }

    if (v & 0xC)
    {
        ret += 2;
        v >>= 4;
    }

    if (v & 2)
        ret++;

    return ret;
}
