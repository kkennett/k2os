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
#include <lib/k2asc.h>

char * 
K2ASC_FindCharLen(
    char        aCh,
    char *      apStr,
    UINT_PTR    aMaxLen
    )
{
    char ch;

    if (aMaxLen == 0)
        return NULL;

    do {
        ch = *apStr;

        if (ch == 0)
            return NULL;

        if (ch == aCh)
            return apStr;

        apStr++;

    } while (--aMaxLen);

    return NULL;
}

char * 
K2ASC_FindCharInsLen(
    char        aCh,
    char *      apStr,
    UINT_PTR    aMaxLen
    )
{
    char ch;

    if (aMaxLen == 0)
        return NULL;

    do {
        ch = *apStr;

        if (K2ASC_ToUpper(ch) == K2ASC_ToUpper(aCh))
            return apStr;

        if (ch == 0)
            break;

        apStr++;

    } while (--aMaxLen);

    return NULL;
}

char *
K2ASC_FindAnyCharLen(
    char const *    apChk,
    char *          apStr,
    UINT_PTR        aMaxLen
    )
{
    char nextCh;
    char *pRet;

    if (aMaxLen == 0)
        return NULL;
    
    nextCh = *apChk;

    if (nextCh == 0)
        return NULL;
    
    do {
        pRet = K2ASC_FindCharLen(nextCh, apStr, aMaxLen);

        if (pRet != NULL)
            return pRet;

        apChk++;

        nextCh = *apChk;

    } while (nextCh != 0);

    return NULL;
}

char *
K2ASC_FindAnyCharInsLen(
    char const *    apChk,
    char *          apStr,
    UINT_PTR        aMaxLen
    )
{
    char nextCh;
    char *pRet;

    if (aMaxLen == 0)
        return NULL;
    
    nextCh = *apChk;

    if (nextCh == 0)
        return NULL;
    
    do {
        pRet = K2ASC_FindCharInsLen(nextCh, apStr, aMaxLen);

        if (pRet != NULL)
            return pRet;

        apChk++;

        nextCh = *apChk;

    } while (nextCh != 0);

    return NULL;
}

char const * 
K2ASC_FindCharConstLen(
    char            aCh,
    char const *    apStr,
    UINT_PTR        aMaxLen
    )
{
    return (char const *)K2ASC_FindCharLen(aCh, (char *)apStr, aMaxLen);
}

char const * 
K2ASC_FindCharConstInsLen(
    char            aCh,
    char const *    apStr,
    UINT_PTR        aMaxLen
    )
{
    return (char const *)K2ASC_FindCharInsLen(aCh, (char *)apStr, aMaxLen);
}

char const *
K2ASC_FindAnyCharConstLen(
    char const *    apChk,
    char const *    apStr,
    UINT_PTR        aMaxLen
    )
{
    return (char const *)K2ASC_FindAnyCharLen(apChk, (char *)apStr, aMaxLen);
}

char const *
K2ASC_FindAnyCharConstInsLen(
    char const *    apChk,
    char const *    apStr,
    UINT_PTR        aMaxLen
    )
{
    return (char const *)K2ASC_FindAnyCharInsLen(apChk, (char *)apStr, aMaxLen);
}

