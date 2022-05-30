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
#include <lib/k2mem.h>

void 
K2MEM_Swap(
    void *      aPtr1,
    void *      aPtr2,
    UINT_PTR    aByteCount
    )
{
    UINT_PTR inc;

    if ((aByteCount == 0) || (aPtr2 == (void const*)aPtr1))
        return;

    if (((((UINT_PTR)aPtr1) & 7) == 0) && (aByteCount > 7))
    {
        if ((((UINT_PTR)aPtr2) & 7) == 0)
        {
            // 64-bit target, 64-bit src
            inc = aByteCount & ~7;
            K2MEM_Swap64(aPtr1, aPtr2, inc);
            if (inc == aByteCount)
                return;
            aPtr1 = (void *)(((UINT_PTR)aPtr1) + inc);
            aPtr2 = (void *)(((UINT_PTR)aPtr2) + inc);
            aByteCount &= 7;
        }
        else if ((((UINT_PTR)aPtr2) & 3) == 0)
        {
            // 64-bit target, 32-bit src
            inc = aByteCount & ~3;
            K2MEM_Swap32(aPtr1, aPtr2, inc);
            if (inc == aByteCount)
                return;
            aPtr1 = (void *)(((UINT_PTR)aPtr1) + inc);
            aPtr2 = (void *)(((UINT_PTR)aPtr2) + inc);
            aByteCount &= 3;
        }
        else if ((((UINT_PTR)aPtr2) & 1) == 0)
        {
            // 64-bit target, 16-bit src
            inc = aByteCount & ~1;
            K2MEM_Swap16(aPtr1, aPtr2, inc);
            if (inc == aByteCount)
                return;
            aPtr1 = (void *)(((UINT_PTR)aPtr1) + inc);
            aPtr2 = (void *)(((UINT_PTR)aPtr2) + inc);
            aByteCount &= 1;
        }
    }
    else if (((((UINT_PTR)aPtr1) & 3) == 0) && (aByteCount > 3))
    {
        if ((((UINT_PTR)aPtr2) & 3) == 0)
        {
            // 32-bit target, 32-bit src
            inc = aByteCount & ~3;
            K2MEM_Swap32(aPtr1, aPtr2, inc);
            if (inc == aByteCount)
                return;
            aPtr1 = (void *)(((UINT_PTR)aPtr1) + inc);
            aPtr2 = (void *)(((UINT_PTR)aPtr2) + inc);
            aByteCount &= 3;
        }
        else if ((((UINT_PTR)aPtr2) & 1) == 0)
        {
            // 32-bit target, 16-bit src
            inc = aByteCount & ~1;
            K2MEM_Swap16(aPtr1, aPtr2, inc);
            if (inc == aByteCount)
                return;
            aPtr1 = (void *)(((UINT_PTR)aPtr1) + inc);
            aPtr2 = (void *)(((UINT_PTR)aPtr2) + inc);
            aByteCount &= 1;
        }
    }
    else if (((((UINT_PTR)aPtr1) & 1) == 0) && (aByteCount > 1))
    {
        if ((((UINT_PTR)aPtr2) & 1) == 0)
        {
            // 16-bit target, 16-bit src
            inc = aByteCount & ~1;
            K2MEM_Swap16(aPtr1, aPtr2, inc);
            if (inc == aByteCount)
                return;
            aPtr1 = (void *)(((UINT_PTR)aPtr1) + inc);
            aPtr2 = (void *)(((UINT_PTR)aPtr2) + inc);
            aByteCount &= 1;
        }
    }

    // whatever is left copies at byte level
    K2MEM_Swap8(aPtr1, aPtr2, aByteCount);
}

