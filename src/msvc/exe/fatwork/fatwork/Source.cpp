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
#include <lib/k2win32.h>
#pragma warning(disable: 4477)

static
void
myAssert(
    char const *    apFile,
    int             aLineNum,
    char const *    apCondition
)
{
    DebugBreak();
}

extern "C" K2_pf_ASSERT K2_Assert = myAssert;

typedef struct _BLOCKIO BLOCKIO;
typedef K2STAT (*BLOCKIO_pf_Transfer)(BLOCKIO *apBlockIo, UINT_PTR aBlockIx, BOOL aIsWrite, void *apBuffer);
struct _BLOCKIO
{
    BLOCKIO_pf_Transfer Transfer;
    UINT_PTR            mBlockSizeBytes;
    UINT_PTR            mBlockCount;
    BOOL                mIsReadOnly;
};

typedef struct _STORPART STORPART;
struct _STORPART
{
    BLOCKIO *   mpBlockIo;
    UINT_PTR    mBlockOffset;
    UINT_PTR    mBlockCount;
    BOOL        mIsReadOnly;
};

K2STAT 
STORPART_Transfer(
    STORPART *  apStorPart,
    UINT_PTR    aBlockIx,
    BOOL        aIsWrite,
    void *      apBuffer
)
{
    BLOCKIO *   pBlockIo;

    if (NULL == apStorPart)
        return K2STAT_ERROR_BAD_ARGUMENT;

    pBlockIo = apStorPart->mpBlockIo;
    if ((NULL == pBlockIo) ||
        (NULL == pBlockIo->Transfer) ||
        (0 == pBlockIo->mBlockCount) ||
        (0 == pBlockIo->mBlockSizeBytes) ||
        (NULL == apBuffer))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if (apStorPart->mBlockOffset >= pBlockIo->mBlockCount)
        return K2STAT_ERROR_CORRUPTED;

    if ((pBlockIo->mBlockCount - apStorPart->mBlockOffset) < apStorPart->mBlockCount)
        return K2STAT_ERROR_CORRUPTED;

    if (aBlockIx >= apStorPart->mBlockCount)
        return K2STAT_ERROR_TOO_BIG;

    if (aIsWrite)
    {
        if ((apStorPart->mIsReadOnly) || (apStorPart->mpBlockIo->mIsReadOnly))
            return K2STAT_ERROR_READ_ONLY;
    }

    return pBlockIo->Transfer(apStorPart->mpBlockIo, aBlockIx + apStorPart->mBlockOffset, aIsWrite, apBuffer);
}

int main(int argc, char **argv)
{


    return 0;
}