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

K2STAT 
K2MEM_Scatter(
    UINT8 const *           apBuffer, 
    UINT_PTR *              apBufferBytes, 
    UINT_PTR                aVectorCount, 
    K2MEM_BUFVECTOR const * apVectorArray
)
{
    UINT_PTR ixVec;
    UINT_PTR copyBytes;
    UINT_PTR vecSpace;
    UINT_PTR thisVec;

    if (NULL == apBufferBytes)
        return K2STAT_ERROR_BAD_ARGUMENT;

    copyBytes = *apBufferBytes;
    *apBufferBytes = 0;

    if ((NULL == apBuffer) ||
        (0 == aVectorCount) ||
        (NULL == apVectorArray))
        return K2STAT_ERROR_BAD_ARGUMENT;

    //
    // clamp transfer to available space
    //
    vecSpace = 0;
    for (ixVec = 0; ixVec < aVectorCount; ixVec++)
    {
        vecSpace += apVectorArray[ixVec].mByteCount;
        if (vecSpace >= copyBytes)
        {
            vecSpace = copyBytes;
            break;
        }
    }
    copyBytes = vecSpace;

    for (ixVec = 0; ixVec < aVectorCount; ixVec++)
    {
        thisVec = apVectorArray->mByteCount;
        if (thisVec > vecSpace)
            thisVec = vecSpace;
        K2MEM_Copy(apVectorArray->mpBuffer, apBuffer, thisVec);
        vecSpace -= thisVec;
        if (0 == vecSpace)
            break;
    }

    *apBufferBytes = copyBytes;

    return K2STAT_NO_ERROR;
}

