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
K2MEM_Copy64(
    void *          apTarget,
    void const *    apSrc,
    UINT_PTR        aByteCount
    )
{
    K2_ASSERT((((UINT_PTR)apTarget) & 7) == 0);
    K2_ASSERT((((UINT_PTR)apSrc) & 7) == 0);
    K2_ASSERT((((UINT_PTR)aByteCount) & 7) == 0);
    if (aByteCount == 0)
        return;
    K2_ASSERT((((UINT_PTR)apSrc) + aByteCount - 1) >= ((UINT_PTR)apSrc));
    K2_ASSERT((((UINT_PTR)apTarget) + aByteCount - 1) >= ((UINT_PTR)apTarget));
    if (apSrc == apTarget)
        return;
    if ((((UINT_PTR)apSrc) + aByteCount) > ((UINT_PTR)apTarget))
    {
        // src and target overlap, copy high to low
        apTarget = ((UINT8 *)apTarget) + aByteCount;
        apSrc = ((UINT8 const *)apSrc) + aByteCount;
        do {
            apTarget = (void *)(((UINT_PTR)apTarget) - sizeof(UINT64));
            apSrc = (void const *)(((UINT_PTR)apSrc) - sizeof(UINT64));
            *((UINT64 *)apTarget) = *((UINT64 const *)apSrc);
            aByteCount -= sizeof(UINT64);
        } while (aByteCount != 0);
    }
    else
    {
        do {
            *((UINT64 *)apTarget) = *((UINT64 const *)apSrc);
            apTarget = (void *)(((UINT_PTR)apTarget) + sizeof(UINT64));
            apSrc = (void const *)(((UINT_PTR)apSrc) + sizeof(UINT64));
            aByteCount -= sizeof(UINT64);
        } while (aByteCount != 0);
    }
}

