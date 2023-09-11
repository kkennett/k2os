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
#include <lib/k2mem.h>

void
K2BIT_InitField(
    K2BIT_FIELD *   apField,
    UINT_PTR        aNumWords,
    UINT32 *        apWords
    )
{
    K2_ASSERT(apField != NULL);
    K2_ASSERT(aNumWords > 0);
    K2_ASSERT(apWords != NULL);
    apField->mpWords = apWords;
    apField->mNumWords = aNumWords;
    apField->mFirstFreeBitWordIndex = 0;
    K2MEM_Zero(apWords, aNumWords * sizeof(UINT32));
}

static
UINT_PTR
sFindFreeBits(
    UINT32      b, 
    UINT_PTR *  apRetStart, 
    UINT32 *    apRetMask
    )
{
    UINT_PTR ix, ixe;

    b = ~b;
    if (b == 0)
        return 0;
    K2BIT_GetLowestPos32(b, &ix);
    *apRetStart = ix;
    b >>= ix + 1;
    K2BIT_GetLowestPos32(~b, &ixe);
    *apRetMask = ((((1 << ixe) - 1) << 1) | 1) << ix;
    return ixe + 1;
}

BOOL
K2BIT_AllocFromField(
    K2BIT_FIELD * apField,
    UINT_PTR      aNumBits,
    UINT_PTR *    apRetFirstBitIndex
    )
{
    UINT_PTR    startWordIx;
    UINT32 *    pStart;
    UINT_PTR    startFirstBitIx;
    UINT32      startMaskBits;
    UINT_PTR    countStartBits;
    UINT32      startValue;
    UINT32      checkMask;
    UINT32 *    pWork;
    UINT_PTR    workWordIx;
    UINT_PTR    bitsLeft;
    BOOL        gotFirst;

    K2_ASSERT(apField != NULL);
    K2_ASSERT(aNumBits > 0);

    if (apField->mFirstFreeBitWordIndex == apField->mNumWords)
        return FALSE;

    startWordIx = apField->mFirstFreeBitWordIndex;
    pStart = apField->mpWords + startWordIx;
    gotFirst = FALSE;

    startValue = *pStart;

    do {
        do {
            do {
                countStartBits = sFindFreeBits(startValue, &startFirstBitIx, &startMaskBits);

                if (countStartBits >= aNumBits)
                {
                    /* done - alloc fits entirely into one DWORD */
                    checkMask = (((((1 << (aNumBits - 1)) - 1) << 1) | 1) << startFirstBitIx);
                    *pStart |= checkMask;
                    if (apRetFirstBitIndex != NULL)
                        *apRetFirstBitIndex = (startWordIx * 32) + startFirstBitIx;
                    if (!gotFirst)
                    {
                        if ((startValue | checkMask) == 0xFFFFFFFF)
                            apField->mFirstFreeBitWordIndex = startWordIx + 1;
                    }
                    return TRUE;
                }

                if (startFirstBitIx + countStartBits == 32)
                {
                    if (startWordIx + 1 == apField->mNumWords)
                        return FALSE;
                    if (((aNumBits - countStartBits) / 32) > (apField->mNumWords - (startWordIx + 1)))
                        return FALSE;
                    break;
                }

                if (!gotFirst)
                {
                    apField->mFirstFreeBitWordIndex = startWordIx;
                    gotFirst = TRUE;
                }

                startValue |= startMaskBits;

            } while (startValue != 0xFFFFFFFF);

            if (startValue != 0xFFFFFFFF)
                break;

            do { 
                pStart++;
                if (++startWordIx == apField->mNumWords)
                    break;
                startValue = *pStart;
            } while (startValue == 0xFFFFFFFF);

        } while (startWordIx < apField->mNumWords);

        if (startWordIx == apField->mNumWords)
            return FALSE;

        /* if we get here we found some bits at the end of a dword but not enough
           to satisfy the alloc so we need to keep looking from this point */

        /* due to check above we know there is at least one dword after the starting one
           that we can check, and it is also possible for enough bits to be left to fulfill
           the allocation */
        if (apRetFirstBitIndex != NULL)
            *apRetFirstBitIndex = (startWordIx * 32) + startFirstBitIx;
        bitsLeft = aNumBits - countStartBits;
        pWork = pStart + 1;
        workWordIx = startWordIx + 1;
        while (bitsLeft)
        {
            if (bitsLeft > 32)
            {
                /* more than 32 bits needed. if next word is not zero then impossible */
                if ((*pWork) || (workWordIx == (apField->mNumWords - 1)))
                    break;
                workWordIx++;
                pWork++;
                bitsLeft -= 32;
            }
            else
            {
                /* 32 or fewer bits needed */
                checkMask = ((((1 << (bitsLeft - 1)) - 1) << 1) | 1);
                if (((*pWork) & checkMask) == 0)
                {
                    /* that's all we need */
                    *pStart |= startMaskBits;
                    pStart++;
                    while (pStart != pWork)
                    {
                        *pStart = 0xFFFFFFFF;
                        pStart++;
                    }
                    *pWork |= checkMask;

                    if ((startWordIx == apField->mFirstFreeBitWordIndex) && (!gotFirst))
                    {
                        /* set new first free bit index */
                        apField->mFirstFreeBitWordIndex = workWordIx;
                        if (*pWork==0xFFFFFFFF)
                            apField->mFirstFreeBitWordIndex++;
                    }
                    return TRUE;
                }
                break;
            }
        }

        /* if we get here we failed to allocate at pStart */
        pStart = pWork;
        startWordIx = workWordIx;
    } while (startWordIx != (apField->mNumWords - 1));

    return FALSE;
}

void
K2BIT_FreeToField(
    K2BIT_FIELD *   apField,
    UINT_PTR        aNumBits,
    UINT_PTR        aFirstBitIndex
    )
{
    UINT_PTR    clearIx;
    UINT32 *    pClear;
    UINT_PTR    clearFirstCount;

    K2_ASSERT(apField != NULL);

    clearIx = (aFirstBitIndex / 32);
    if (apField->mFirstFreeBitWordIndex > clearIx)
        apField->mFirstFreeBitWordIndex = clearIx;
    pClear = apField->mpWords + clearIx;
    aFirstBitIndex &= 31;

    /* clear first word */
    clearFirstCount = 32 - aFirstBitIndex;
    if (clearFirstCount > aNumBits)
        clearFirstCount = aNumBits;
    *pClear &= ~(((((1 << (clearFirstCount - 1)) - 1) << 1) | 1) << aFirstBitIndex);
    aNumBits -= clearFirstCount;
    if (aNumBits == 0)
        return;
    pClear++;

    /* clear words in the middle */
    if (aNumBits >= 32)
    {
        do {
            *pClear = 0;
            pClear++;
            aNumBits -= 32;
        } while (aNumBits >= 32);
        if (aNumBits == 0)
            return;
    }

    /* clear bits at the end */
    *pClear &= ~((((1 << (aNumBits - 1)) - 1) << 1) | 1);
}


