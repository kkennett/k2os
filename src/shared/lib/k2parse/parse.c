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
#include <lib/k2parse.h>
#include <lib/k2asc.h>

void 
K2PARSE_EatToEOL(
    char const **   appPars, 
    UINT_PTR *      apLeft
    )
{
    char chk;

    K2_ASSERT(appPars != NULL);
    K2_ASSERT(apLeft != NULL);

    if ((*apLeft) == 0)
        return;
    do {
        chk = **appPars;
        if ((chk=='\r') || (chk=='\n') || (chk==0))
            break;
        (*appPars)++;
    } while (--(*apLeft));
}

void 
K2PARSE_EatEOL(
    char const **   appPars, 
    UINT_PTR *      apLeft
    )
{
    char chk;

    K2_ASSERT(appPars != NULL);
    K2_ASSERT(apLeft != NULL);

    if ((*apLeft) == 0)
        return;
    do {
        chk = **appPars;
        if ((chk!='\r') && (chk!='\n'))
            break;
        (*appPars)++;
    } while (--(*apLeft));
}

void 
K2PARSE_EatWhitespace(
    char const **   appPars, 
    UINT_PTR *      apLeft
    )
{
    char chk;

    K2_ASSERT(appPars != NULL);
    K2_ASSERT(apLeft != NULL);

    if ((*apLeft) == 0)
        return;
    do {
        chk = **appPars;
        if ((chk!=' ') && (chk!='\t'))
            break;
        (*appPars)++;
    } while (--(*apLeft));
}

void 
K2PARSE_EatWhitespaceAtEnd(
    char const *    apPars, 
    UINT_PTR *      apLineLen
    )
{
    char chk;
    char const *pStart;

    K2_ASSERT(apLineLen != NULL);

    if ((*apLineLen) == 0)
        return;
    pStart = apPars;
    apPars += (*apLineLen);
    do {
        apPars--;
        chk = *apPars;
        if ((chk!=' ') && (chk!='\t'))
            break;
        (*apLineLen)--;
    } while (apPars != pStart);
}

void 
K2PARSE_EatLine(
    char const **   appPars, 
    UINT_PTR *      apLeft, 
    char const **   apRetLine, 
    UINT_PTR *      apRetLineLen
    )
{
    K2_ASSERT(appPars != NULL);
    K2_ASSERT(apLeft != NULL);
    K2_ASSERT(apRetLine != NULL);
    K2_ASSERT(apRetLineLen != NULL);
    *apRetLine = *appPars;
    *apRetLineLen = 0;
    if ((*apLeft) == 0)
        return;
    K2PARSE_EatToEOL(appPars, apLeft);
    *apRetLineLen = (UINT_PTR)((*appPars)-(*apRetLine));
    K2PARSE_EatEOL(appPars, apLeft);
}

void 
K2PARSE_EatWord(
    char const **   appPars, 
    UINT_PTR *      apLeft, 
    char const **   apRetToken, 
    UINT_PTR *      apRetTokLen
    )
{
    char chk;

    K2_ASSERT(appPars != NULL);
    K2_ASSERT(apLeft != NULL);
    K2_ASSERT(apRetToken != NULL);
    K2_ASSERT(apRetTokLen != NULL);

    K2PARSE_EatWhitespace(appPars, apLeft);

    *apRetTokLen = 0;
    *apRetToken = *appPars;

    if ((*apLeft) == 0)
        return;

    chk = **appPars;
    if ((chk=='\r') || (chk=='\n') || (chk==0))
        return;
    do {
        chk = **appPars;
        if ((chk==' ') || (chk=='\t') || (chk=='\r') || (chk=='\n') || (chk==0))
            break;
        (*appPars)++;
    } while (--(*apLeft));

    *apRetTokLen = (UINT_PTR)((*appPars) - (*apRetToken));
}

void
K2PARSE_EatToChar(
    char const **   appPars,
    UINT_PTR *      apLeft,
    char            aCh
    )
{
    char chk;

    K2_ASSERT(appPars != NULL);
    K2_ASSERT(apLeft != NULL);

    if ((*apLeft) == 0)
        return;
    do {
        chk = **appPars;
        if (chk==aCh)
            break;
        (*appPars)++;
    } while (--(*apLeft));
}

void
K2PARSE_Token(
    char const **   appPars,
    UINT_PTR *      apLeft,
    char const **   apRetToken,
    UINT_PTR *      apRetTokLen
    )
{
    char chk;

    K2_ASSERT(appPars != NULL);
    K2_ASSERT(apLeft != NULL);
    K2_ASSERT(apRetToken != NULL);
    K2_ASSERT(apRetTokLen != NULL);

    K2PARSE_EatWhitespace(appPars, apLeft);

    *apRetTokLen = 0;
    *apRetToken = *appPars;

    if ((*apLeft) == 0)
        return;

    chk = K2ASC_ToUpper(**appPars);
    if (!((chk == '_') ||
        ((chk >= 'A') && (chk <= 'Z'))))
        return;

    *apRetTokLen = 1;
    (*appPars)++;
    (*apLeft)--;

    if ((*apLeft) == 0)
        return;

    do
    {
        chk = K2ASC_ToUpper(**appPars);
        if (!((chk == '_') ||
            ((chk >= 'A') && (chk <= 'Z')) ||
            ((chk >= '0') && (chk <= '9'))
            ))
            break;
        (*apRetTokLen)++;
        (*appPars)++;
        (*apLeft)--;
    } while ((*apLeft) != 0);
}

static
BOOL
sTextToHex(
    char const *    apText,
    UINT_PTR        aCharCount,
    UINT_PTR *      apRetVal
    )
{
    UINT_PTR val;
    UINT_PTR ix;
    char  ch;

    val = 0;

    for (ix = 0;ix < aCharCount;ix++)
    {
        ch = K2ASC_ToUpper(apText[ix]);
        if ((ch >= 'A') && (ch <= 'F'))
        {
            ch = 10 + (ch - 'A');
        }
        else if ((ch >= '0') && (ch <= '9'))
        {
            ch -= '0';
        }
        else
            return FALSE;
        val = (val << 4) + ch;
    }

    *apRetVal = val;

    return TRUE;
}

BOOL
K2PARSE_Guid128(
    char const *    apText,
    UINT_PTR        aLen,
    K2_GUID128 *    apRetGuid
    )
{
    UINT_PTR  ix;
    UINT_PTR  val;

    K2_ASSERT(aLen != 0);
    K2_ASSERT(apRetGuid != NULL);

    if ((aLen != 36) ||
        (apText[8] != '-') ||
        (apText[13] != '-') ||
        (apText[18] != '-') ||
        (apText[23] != '-'))
        return FALSE;

    if (!sTextToHex(apText, 8, &val))
        return FALSE;
    apRetGuid->mData1 = (UINT32)val;

    if (!sTextToHex(&apText[9], 4, &val))
        return FALSE;
    apRetGuid->mData2 = (UINT16)val;

    if (!sTextToHex(&apText[14], 4, &val))
        return FALSE;
    apRetGuid->mData3 = (UINT16)val;

    if (!sTextToHex(&apText[19], 4, &val))
        return FALSE;
    apRetGuid->mData4[0] = (UINT8)((val >> 8) & 0xFF);
    apRetGuid->mData4[1] = (UINT8)(val & 0xFF);

    apText += 24;
    for (ix = 2;ix < 8;ix++)
    {
        if (!sTextToHex(apText, 2, &val))
            return FALSE;
        apRetGuid->mData4[ix] = (UINT8)(val & 0xFF);
        apText++;
    }

    return TRUE;
}
