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

BOOL
K2MEM_Verify(
    void const *    apTarget,
    UINT8           aValue,
    UINT_PTR        aByteCount
    )
{
    UINT_PTR   ptr;
    UINT_PTR   inc;
    UINT64      mask;

    if (aByteCount == 0)
        return TRUE;

    ptr = (UINT_PTR)apTarget;

    // unknown alignment

    mask = ((UINT64)aValue);
    mask |= mask << 8;
    mask |= mask << 16;
    mask |= mask << 32;

    if ((ptr & 1) != 0)
    {
        if (*((UINT8 const *)ptr) != aValue)
            return FALSE;
        ptr += sizeof(UINT8);
        aByteCount -= sizeof(UINT8);
        if (aByteCount == 0)
            return TRUE;
    }

    if (aByteCount >= sizeof(UINT16))
    {
        if ((ptr & 3)!=0)
        {
            if (*((UINT16 const *)ptr) != (UINT16)mask)
                return FALSE;
            ptr += sizeof(UINT16);
            aByteCount -= sizeof(UINT16);
        }

        if (aByteCount >= sizeof(UINT32))
        {
            if ((ptr & 7)!=0)
            {
                if (*((UINT32 const *)ptr) != (UINT32)mask)
                    return FALSE;
                ptr += sizeof(UINT32);
                aByteCount -= sizeof(UINT32);
                if (aByteCount == 0)
                    return TRUE;
            }

            if (aByteCount > 7)
            {
                inc = aByteCount & ~7;
                if (!K2MEM_Verify64((void const *)ptr, mask, inc))
                    return FALSE;
                ptr += inc;
                aByteCount -= inc;
                if (aByteCount == 0)
                    return TRUE;
            }

            if (aByteCount > 3)
            {
                inc = aByteCount & ~3;
                if (!K2MEM_Verify32((void const *)ptr, (UINT32)mask, inc))
                    return FALSE;
                ptr += inc;
                aByteCount -= inc;
                if (aByteCount == 0)
                    return TRUE;
            }
        }

        if (aByteCount > 1)
        {
            inc = aByteCount & ~1;
            if (!K2MEM_Verify16((void const *)ptr, (UINT16)mask, inc))
                return FALSE;
            ptr += inc;
            aByteCount -= inc;
            if (aByteCount == 0)
                return TRUE;
        }
    }

    // at least 16-bit aligned, 1 byte left
    return (*((UINT8 const *)ptr) == aValue);
}
