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
#include <lib/k2mem.h>

UINT16 
K2MEM_ReadAsBytes_UINT16(
    void const *apData
)
{
    if (0 == (((UINT_PTR)apData) & 1))
    {
        return *((UINT16 *)apData);
    }
    return (((UINT16)K2_BYTEREAD(apData, 1)) << 8) | (((UINT16)K2_BYTEREAD(apData, 0)));
}

void 
K2MEM_WriteAsBytes_UINT16(
    void *  apData, 
    UINT16  aValue
)
{
    if (0 == (((UINT_PTR)apData) & 1))
    {
        *((UINT16 *)apData) = aValue;
    }
    else
    {
        K2_BYTEWRITE(apData, 0, aValue & 0xFF);
        K2_BYTEWRITE(apData, 1, (aValue >> 8) & 0xFF);
    }
}

UINT32 
K2MEM_ReadAsBytes_UINT32(
    void const *apData
)
{
    if (0 == (((UINT_PTR)apData) & 3))
    {
        return *((UINT32 *)apData);
    }
    return (((UINT32)K2_BYTEREAD(apData, 3)) << 24) |
        (((UINT32)K2_BYTEREAD(apData, 2)) << 16) |
        (((UINT32)K2_BYTEREAD(apData, 1)) << 8) |
        ((UINT32)K2_BYTEREAD(apData, 0));
}

void 
K2MEM_WriteAsBytes_UINT32(
    void *  apData, 
    UINT32  aValue
)
{
    if (0 == (((UINT_PTR)apData) & 3))
    {
        *((UINT32 *)apData) = aValue;
    }
    else
    {
        K2_BYTEWRITE(apData, 0, aValue & 0xFF);
        K2_BYTEWRITE(apData, 1, (aValue >> 8) & 0xFF);
        K2_BYTEWRITE(apData, 2, (aValue >> 16) & 0xFF);
        K2_BYTEWRITE(apData, 3, (aValue >> 24) & 0xFF);
    }
}

