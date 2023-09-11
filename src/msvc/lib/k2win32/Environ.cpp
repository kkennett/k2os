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
#include <lib/k2win32.h>

EnvironEntry const 
Environ::smBlankEntry = 
{ 
    NULL, 0, 
    NULL, 0 
};

Environ::Environ(
    bool aInit
)
{
    mpBlob = NULL;
    mBlobLen = NULL;
    mChanged = false;
    mNumEntries = 0;
    mppEntry = NULL;
    if (!aInit)
        return;

    LPCH pBuf = GetEnvironmentStringsA();
    K2_ASSERT(NULL != pBuf);

    char const *    pCh;
    char const *    pHold;
    UINT_PTR        len;
    EnvironEntry *  pEntry;
    EnvironEntry ** ppList;

    pHold = pBuf;
    do {
        if ((*pHold) != '=')
            break;
        do {
            ++pHold;
        } while (*pHold != 0);
        pHold++;
    } while (*pHold != 0);
    K2_ASSERT(*pHold != 0);

    pCh = pHold;
    pHold = pCh;
    if (*pCh)
    {
        char ch;
        ch = *pCh;
        do {
            K2_ASSERT(0 != ch);
            if ('=' == ch)
            {
                len = (UINT_PTR)(pCh - pHold);
                K2_ASSERT(0 != len);
                pEntry = new EnvironEntry;
                K2_ASSERT(NULL != pEntry);
                pEntry->mNameLen = len;
                pEntry->mpName = new char[(len + 4) & ~3];
                K2_ASSERT(NULL != pEntry->mpName);
                strncpy_s(pEntry->mpName, len + 1, pHold, len);
                pCh++;
                pHold = pCh;
                ch = *pCh;
                K2_ASSERT(0 != ch);
                do {
                    if (0 == ch)
                    {
                        len = (UINT_PTR)(pCh - pHold);
                        K2_ASSERT(0 != len);
                        pEntry->mValueLen = len;
                        pEntry->mpValue = new char[(len + 4) & ~3];
                        K2_ASSERT(NULL != pEntry->mpValue);
                        strncpy_s(pEntry->mpValue, len + 1, pHold, len);

                        ppList = new EnvironEntry * [mNumEntries + 1];
                        K2_ASSERT(NULL != ppList);
                        if (0 != mNumEntries)
                        {
                            CopyMemory(ppList, mppEntry, sizeof(EnvironEntry *) * mNumEntries);
                            delete[] mppEntry;
                        }
                        mppEntry = ppList;
                        mppEntry[mNumEntries] = pEntry;
                        mNumEntries++;
                        pHold = pCh + 1;
                        break;
                    }
                    pCh++;
                    ch = *pCh;
                } while (1);
            }
            pCh++;
            ch = *pCh;
        } while (0 != ch);
        pCh++;
        mBlobLen = (UINT_PTR)(pCh - pBuf);
        mpBlob = new char[(mBlobLen + 4) & ~3];
        K2_ASSERT(NULL != mpBlob);
        CopyMemory(mpBlob, pBuf, mBlobLen);
    }
    FreeEnvironmentStringsA(pBuf);
}

Environ::Environ(
    Environ const &aCopyFrom
)
{
    UINT_PTR        ix;
    EnvironEntry *  pCopy;
    EnvironEntry *  pEntry;

    mpBlob = NULL;
    mBlobLen = 0;
    mChanged = false;
    mNumEntries = aCopyFrom.mNumEntries;
    mppEntry = new EnvironEntry * [mNumEntries];
    K2_ASSERT(NULL != mppEntry);

    for (ix = 0; ix < aCopyFrom.mNumEntries; ix++)
    {
        pCopy = mppEntry[ix];
        
        pEntry = new EnvironEntry;
        K2_ASSERT(NULL != pEntry);
        
        pEntry->mpName = new char[(pCopy->mNameLen + 4) & ~3];
        K2_ASSERT(NULL != pEntry->mpName);
        strncpy_s(pEntry->mpName, pCopy->mNameLen + 1, pCopy->mpName, pCopy->mNameLen);
        pEntry->mNameLen = pCopy->mNameLen;

        pEntry->mpValue = new char[(pCopy->mValueLen + 4) & ~3];
        K2_ASSERT(NULL != pEntry->mpValue);
        strncpy_s(pEntry->mpValue, pCopy->mValueLen + 1, pCopy->mpValue, pCopy->mValueLen);
        pEntry->mValueLen = pCopy->mValueLen;
    }
    if (mNumEntries > 0)
        mChanged = true;
}

UINT_PTR 
Environ::Find(
    char const *apName
) const
{
    if ((NULL == apName) || (0 == *apName))
        return (UINT_PTR)-1;

    UINT_PTR len = strlen(apName);
    
    UINT_PTR ix;

    for (ix = 0; ix < mNumEntries; ix++)
    {
        if (len != mppEntry[ix]->mNameLen)
            continue;
        if (0 == _strnicmp(apName, mppEntry[ix]->mpName, len))
        {
            return ix;
        }
    }

    return (UINT_PTR)-1;
}

bool 
Environ::Set(
    char const *apName, 
    char const *apValue
)
{
    UINT_PTR        ix;
    UINT_PTR        len;
    EnvironEntry *  pEntry;
    EnvironEntry ** ppList;

    ix = Find(apName);

    if (((UINT_PTR)-1) == ix)
    {
        //
        // new variable
        //
        if ((NULL == apName) || (0 == *apName))
            return false;
        if ((NULL == apValue) || (0 == *apValue))
            return true;

        pEntry = new EnvironEntry;
        K2_ASSERT(NULL != pEntry);

        len = strlen(apName);
        pEntry->mpName = new char[(len + 4) & ~3];
        K2_ASSERT(NULL != pEntry->mpName);
        strncpy_s(pEntry->mpName, len + 1, apName, len);
        pEntry->mNameLen = len;

        len = strlen(apValue);
        pEntry->mpValue = new char[(len + 4) & ~3];
        K2_ASSERT(NULL != pEntry->mpValue);
        strncpy_s(pEntry->mpValue, len + 1, apValue, len);
        pEntry->mValueLen = len;

        ppList = new EnvironEntry * [mNumEntries + 1];
        K2_ASSERT(NULL != ppList);

        if (0 != mNumEntries)
        {
            CopyMemory(ppList, mppEntry, sizeof(EnvironEntry *) * mNumEntries);
            delete[] mppEntry;
        }
        mppEntry = ppList;
        mppEntry[mNumEntries] = pEntry;
        mNumEntries++;

        return true;
    }

    pEntry = mppEntry[ix];

    if ((NULL == apValue) || (0 == (*apValue)))
    {
        //
        // clear variable
        //
        delete[] pEntry->mpValue;
        delete[] pEntry->mpName;
        delete pEntry;
        mppEntry[ix] = NULL;

        if (ix != (mNumEntries - 1))
        {
            CopyMemory(&mppEntry[ix], &mppEntry[ix + 1], ((mNumEntries - ix) - 1) * sizeof(EnvironEntry *));
        }
        mNumEntries--;
        mChanged = true;
        return true;
    }

    //
    // change existing variable
    //
    delete[] pEntry->mpValue;

    len = strlen(apValue);
    pEntry->mpValue = new char[(len + 4) & ~3];
    K2_ASSERT(NULL != pEntry->mpValue);
    strncpy_s(pEntry->mpValue, len + 1, apValue, len);
    pEntry->mValueLen = len;

    mChanged = true;

    return true;
}

char * 
Environ::CopyBlob(
    UINT_PTR *apRetLen
)
{
    char *      pResult;
    UINT_PTR    ix;
    UINT_PTR    len;

    if (NULL == mpBlob)
    {
        if (!mChanged)
        {
            if (NULL != apRetLen)
                *apRetLen = 0;
            return NULL;
        }
    }

    if (mChanged)
    {
        if (0 != mBlobLen)
        {
            delete[] mpBlob;
            mpBlob = NULL;
            mBlobLen = 0;
        }
        if (mNumEntries > 0)
        {
            //
            // regenerate blob
            //
            len = 1;    // trailing zero
            for (ix = 0; ix < mNumEntries; ix++)
            {
                len += mppEntry[ix]->mNameLen;
                len++;
                len += mppEntry[ix]->mValueLen;
                len++;
            }

            pResult = new char[(len + 4) & ~3];
            K2_ASSERT(NULL != pResult);

            mpBlob = pResult;
            mBlobLen = len;
            for (ix = 0; ix < mNumEntries; ix++)
            {
                strncpy_s(pResult, mppEntry[ix]->mNameLen + 1, mppEntry[ix]->mpName, mppEntry[ix]->mNameLen);
                pResult += mppEntry[ix]->mNameLen;
                *pResult = '=';
                pResult++;
                strncpy_s(pResult, mppEntry[ix]->mValueLen + 1, mppEntry[ix]->mpValue, mppEntry[ix]->mValueLen);
                pResult += mppEntry[ix]->mValueLen + 1;
            }
            K2_ASSERT(pResult == &mpBlob[len - 1]);
            mpBlob[len - 1] = 0;
        }
    }

    if (0 == mBlobLen)
    {
        pResult = NULL;
    }
    else
    {
        K2_ASSERT(NULL != mpBlob);
        pResult = new char[mBlobLen];
        K2_ASSERT(NULL != pResult);
        CopyMemory(pResult, mpBlob, mBlobLen);
    }

    if (NULL != apRetLen)
        *apRetLen = mBlobLen;

    return pResult;
}


