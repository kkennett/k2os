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

#include <lib/k2bit.h>

BOOL
K2BIT_GetHighestPos32(
    UINT32      aValue,
    UINT_PTR *  apRetHighestBitSet
    )
{
    UINT_PTR ret;

    if (aValue == 0)
    {
        if (apRetHighestBitSet != NULL)
            *apRetHighestBitSet = (UINT_PTR)-1;
        return FALSE;
    }
    else if (apRetHighestBitSet == NULL)
        return TRUE;

    if (aValue & 0xFFFF0000)
    {
        ret = 16;
        aValue >>= 16;
    }
    else
        ret = 0;

    if (aValue & 0xFF00)
    {
        ret += 8;
        aValue >>= 8;
    }

    if (aValue & 0xF0)
    {
        ret += 4;
        aValue >>= 4;
    }

    if (aValue & 0xC)
    {
        ret += 2;
        aValue >>= 2;
    }

    if (aValue & 2)
        ret++;

     *apRetHighestBitSet = ret;

     return TRUE;
}

UINT32
K2BIT_GetLowestOnly32(
    UINT32  aValue
    )
{
    UINT32 v;
    v = aValue - 1;
    return (aValue | v) ^ v;
}

BOOL
K2BIT_IsOnlyOneSet32(
    UINT32 aValue
    )
{
    if (aValue == 0)
        return FALSE;
    return ((aValue & (aValue - 1)) == 0);
}

BOOL
K2BIT_GetLowestPos32(
    UINT32      aValue,
    UINT_PTR *  apRetLowestBitSet
    )
{
    if (aValue == 0)
    {
        if (apRetLowestBitSet != NULL)
            *apRetLowestBitSet = (UINT_PTR)-1;
        return FALSE;
    }

    return K2BIT_GetHighestPos32(K2BIT_GetLowestOnly32(aValue), apRetLowestBitSet);
}

UINT_PTR
K2BIT_CountNumberSet32(
    UINT32   aValue
    )
{
    aValue = (aValue & 0x55555555) + ((aValue >>  1) & 0x55555555);
    aValue = (aValue & 0x33333333) + ((aValue >>  2) & 0x33333333);
    aValue = (aValue & 0x0F0F0F0F) + ((aValue >>  4) & 0x0F0F0F0F);
    aValue = (aValue & 0x00FF00FF) + ((aValue >>  8) & 0x00FF00FF);
    return   (aValue & 0x0000FFFF) + ((aValue >> 16) & 0x0000FFFF);
}

