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
#include <lib/k2json.h>
#include <lib/k2mem.h>
#include <lib/k2asc.h>
#include <lib/k2parse.h>

void
K2JSON_EatWhitespace(
    char const **   appPars,
    UINT_PTR *      apLeft
)
{
    // this eats through an arbitrary # of EOLs
    char chk;
    do {
        K2PARSE_EatWhitespace(appPars, apLeft);
        if (0 == *apLeft)
            return;
        chk = **appPars;
        if ((chk != '\r') && (chk != '\n'))
            return;
        K2PARSE_EatEOL(appPars, apLeft);
    } while (0 != *apLeft);
}

void
K2JSON_Purge(
    K2JSON_pf_MemFree   afFree,
    K2JSON_PTR          aPtr
)
{
    UINT_PTR    ix;

    if (NULL == aPtr.mpAsAny)
        return;

    if (K2JSON_Obj == aPtr.mpAsAny->mType)
    {
        if (0 != aPtr.mpAsObj->mPairCount)
        {
            for (ix = 0; ix < aPtr.mpAsObj->mPairCount; ix++)
            {
                K2JSON_Purge(afFree, aPtr.mpAsObj->mpPairs[ix].Value);
            }
            afFree(aPtr.mpAsObj->mpPairs);
        }
    }
    else if (K2JSON_Array == aPtr.mpAsAny->mType)
    {
        if (0 != aPtr.mpAsArray->mElementCount)
        {
            for (ix = 0; ix < aPtr.mpAsArray->mElementCount; ix++)
            {
                K2JSON_Purge(afFree, aPtr.mpAsArray->mpElements[ix]);
            }
            afFree(aPtr.mpAsArray->mpElements);
        }
    }

    afFree(aPtr.mpAsAny);
}

K2STAT
K2JSON_ParseAsStr(
    K2JSON_PARSE *  apParse,
    K2JSON_STR *    apStr,
    char const **   appData,
    UINT_PTR *      apMaxDataLeft
)
{
    char const *pPars;
    UINT_PTR    left;
    char        c;
    BOOL        escaped;
    K2STAT      stat;

    left = *apMaxDataLeft;
    pPars = *appData;
    if (0 == left)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_END_OF_FILE;
    }

    K2MEM_Zero(apStr, sizeof(K2JSON_STR));
    apStr->Hdr.mType = K2JSON_Str;
    apStr->Hdr.mpRawChars = pPars;

    K2JSON_EatWhitespace(&pPars, &left);
    if (0 == left)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_END_OF_FILE;
    }

    c = *pPars;
    if ('\"' != c)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_BAD_FORMAT;
    }
    pPars++;
    left--;
    if (0 == left)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_END_OF_FILE;
    }

    stat = K2STAT_NO_ERROR;
    apStr->mpStr = pPars;
    
    escaped = FALSE;
    do {
        c = *pPars;
        if ('\"' == c)
        {
            if (!escaped)
            {
                apStr->mStrLen = (UINT_PTR)(pPars - apStr->mpStr);
                pPars++;
                left--;
                apStr->Hdr.mRawLen = (UINT_PTR)(pPars - apStr->Hdr.mpRawChars);
                break;
            }
        }
        if ('\\' == c)
        {
            if (!escaped)
            {
                escaped = TRUE;
            }
            else
            {
                // double backslash
                escaped = FALSE;
            }
        }
        else
        {
            escaped = FALSE;
        }
        pPars++;
        left--;
        if (0 == left)
        {
            apParse->mpErrorPoint = pPars;
            stat = K2STAT_ERROR_END_OF_FILE;
            break;
        }
    } while (1);

    if (!K2STAT_IS_ERROR(stat))
    {
        *appData = pPars;
        *apMaxDataLeft = left;
    }

    return stat;
}

K2STAT
K2JSON_ParseAsNum(
    K2JSON_PARSE *  apParse,
    K2JSON_NUM *    apNum,
    char const **   appData,
    UINT_PTR *      apMaxDataLeft
)
{
    char const *pPars;
    UINT_PTR    left;
    char        c;

    left = *apMaxDataLeft;
    pPars = *appData;
    if (0 == left)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_END_OF_FILE;
    }

    K2MEM_Zero(apNum, sizeof(K2JSON_NUM));
    apNum->Hdr.mType = K2JSON_Num;
    apNum->Hdr.mpRawChars = pPars;

    K2JSON_EatWhitespace(&pPars, &left);
    if (0 == left)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_END_OF_FILE;
    }

    c = *pPars;
    if ('-' == c)
    {
        apNum->mIsNeg = TRUE;
        pPars++;
        left--;
        if (0 == left)
        {
            apParse->mpErrorPoint = pPars;
            return K2STAT_ERROR_END_OF_FILE;
        }
        c = *pPars;
    }

    //
    // at zero check
    // next must be a 0 or a digit 1-9
    //
    if ('0' != c)
    {
        if (('1' > c) || (c > '9'))
        {
            apParse->mpErrorPoint = pPars;
            return K2STAT_ERROR_BAD_FORMAT;
        }
        // eat until something that is not a digit
        do {
            pPars++;
            left--;
            if (0 == left)
            {
                apParse->mpErrorPoint = pPars;
                return K2STAT_ERROR_END_OF_FILE;
            }
            c = *pPars;
        } while (('0' <= c) && (c <= '9'));
    }
    else
    {
        pPars++;
        left--;
        if (0 == left)
        {
            apParse->mpErrorPoint = pPars;
            return K2STAT_ERROR_END_OF_FILE;
        }
        c = *pPars;
    }

    //
    // at fraction check
    //
    if ('.' == c)
    {
        apNum->mIsFrac = TRUE;
        pPars++;
        left--;
        if (0 == left)
        {
            apParse->mpErrorPoint = pPars;
            return K2STAT_ERROR_END_OF_FILE;
        }
        c = *pPars;
        if (('0' > c) || (c > '9'))
        {
            apParse->mpErrorPoint = pPars;
            return K2STAT_ERROR_BAD_FORMAT;
        }
        // eat until something that is not a digit
        do {
            pPars++;
            left--;
            if (0 == left)
            {
                apParse->mpErrorPoint = pPars;
                return K2STAT_ERROR_END_OF_FILE;
            }
            c = *pPars;
        } while (('0' <= c) && (c <= '9'));
    }

    if (('e' == c) || ('E' == c))
    {
        apNum->mHasExp = TRUE;
        pPars++;
        left--;
        if (0 == left)
        {
            apParse->mpErrorPoint = pPars;
            return K2STAT_ERROR_END_OF_FILE;
        }
        c = *pPars;
        if (('-' == c) || ('+' == c))
        {
            if ('-' == c)
            {
                apNum->mExpIsNeg = TRUE;
            }
            pPars++;
            left--;
            if (0 == left)
            {
                apParse->mpErrorPoint = pPars;
                return K2STAT_ERROR_END_OF_FILE;
            }
            c = *pPars;
        }
        if (('0' > c) || (c > '9'))
        {
            apParse->mpErrorPoint = pPars;
            return K2STAT_ERROR_BAD_FORMAT;
        }
        // eat until something that is not a digit
        do {
            pPars++;
            left--;
            if (0 == left)
            {
                apParse->mpErrorPoint = pPars;
                return K2STAT_ERROR_END_OF_FILE;
            }
            c = *pPars;
        } while (('0' <= c) && (c <= '9'));
    }

    apNum->Hdr.mRawLen = (UINT_PTR)(pPars - apNum->Hdr.mpRawChars);

    *appData = pPars;
    *apMaxDataLeft = left;

    return K2STAT_NO_ERROR;
}

K2STAT
K2JSON_ParseAsObj(
    K2JSON_PARSE *  apParse,
    K2JSON_OBJ *    apObj,
    char const **   appData,
    UINT_PTR *      apMaxDataLeft
);

K2STAT
K2JSON_ParseAsArray(
    K2JSON_PARSE *  apParse,
    K2JSON_ARRAY *  apArray,
    char const **   appData,
    UINT_PTR *      apMaxDataLeft
)
{
    char const *    pPars;
    UINT_PTR        left;
    char            c;
    K2STAT          stat;
    K2JSON_PTR      elem;
    UINT_PTR        ix;
    K2JSON_PTR *    pElems;

    left = *apMaxDataLeft;
    pPars = *appData;
    if (0 == left)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_END_OF_FILE;
    }

    K2MEM_Zero(apArray, sizeof(K2JSON_ARRAY));
    apArray->Hdr.mType = K2JSON_Array;
    apArray->Hdr.mpRawChars = pPars;

    K2JSON_EatWhitespace(&pPars, &left);
    if (0 == left)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_END_OF_FILE;
    }

    c = *pPars;
    if ('[' != c)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_BAD_FORMAT;
    }
    pPars++;
    left--;

    K2JSON_EatWhitespace(&pPars, &left);
    if (0 == left)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_END_OF_FILE;
    }

    c = *pPars;
    if (']' == c)
    {
        // empty array
        pPars++;
        left--;
        apArray->Hdr.mRawLen = (UINT_PTR)(pPars - apArray->Hdr.mpRawChars);
        stat = K2STAT_NO_ERROR;
    }
    else
    {
        // not an empty array
        stat = K2STAT_ERROR_BAD_FORMAT;
        elem.mpAsAny = NULL;

        do {
            K2JSON_EatWhitespace(&pPars, &left);
            if (0 == left)
            {
                apParse->mpErrorPoint = pPars;
                stat = K2STAT_ERROR_END_OF_FILE;
                break;
            }

            c = *pPars;
            if ('[' == c)
            {
                elem.mpAsArray = apParse->mfAlloc(sizeof(K2JSON_ARRAY));
                if (NULL == elem.mpAsArray)
                {
                    apParse->mpErrorPoint = pPars;
                    stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    break;
                }
                stat = K2JSON_ParseAsArray(apParse, elem.mpAsArray, &pPars, &left);
            }
            else if ('{' == c)
            {
                elem.mpAsObj = apParse->mfAlloc(sizeof(K2JSON_OBJ));
                if (NULL == elem.mpAsObj)
                {
                    apParse->mpErrorPoint = pPars;
                    stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    break;
                }
                stat = K2JSON_ParseAsObj(apParse, elem.mpAsObj, &pPars, &left);
            }
            else if ('\"' == c)
            {
                elem.mpAsStr = apParse->mfAlloc(sizeof(K2JSON_STR));
                if (NULL == elem.mpAsStr)
                {
                    apParse->mpErrorPoint = pPars;
                    stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    break;
                }
                stat = K2JSON_ParseAsStr(apParse, elem.mpAsStr, &pPars, &left);
            }
            else
            {
                if (('n' == c) || ('t' == c) || ('f' == c))
                {
                    if ('n' == c)
                    {
                        // parse as null
                        if (left < 5)
                        {
                            apParse->mpErrorPoint = pPars;
                            break;  // bad format
                        }
                        if (0 != K2ASC_CompLen(pPars, "null", 4))
                        {
                            apParse->mpErrorPoint = pPars;
                            break;
                        }
                        // leave elem.mpAsAny as NULL;
                        pPars += 4;
                        left -= 4;
                    }
                    else if ('t' == c)
                    {
                        // parse as bool-true
                        if (left < 5)
                        {
                            apParse->mpErrorPoint = pPars;
                            break;
                        }
                        if (0 != K2ASC_CompLen(pPars, "true", 4))
                        {
                            apParse->mpErrorPoint = pPars;
                            break;
                        }
                        elem.mpAsBool = apParse->mfAlloc(sizeof(K2JSON_BOOL));
                        if (NULL == elem.mpAsBool)
                        {
                            apParse->mpErrorPoint = pPars;
                            stat = K2STAT_ERROR_OUT_OF_MEMORY;
                            break;
                        }
                        elem.mpAsBool->Hdr.mType = K2JSON_Bool;
                        elem.mpAsBool->Hdr.mpRawChars = pPars;
                        elem.mpAsBool->Hdr.mRawLen = 4;
                        elem.mpAsBool->mBool = TRUE;
                        pPars += 4;
                        left -= 4;
                    }
                    else // ('f' == c)
                    {
                        // parse as bool-false
                        if (left < 6)
                        {
                            apParse->mpErrorPoint = pPars;
                            break;
                        }
                        if (0 != K2ASC_CompLen(pPars, "false", 5))
                        {
                            apParse->mpErrorPoint = pPars;
                            break;
                        }
                        elem.mpAsBool = apParse->mfAlloc(sizeof(K2JSON_BOOL));
                        if (NULL == elem.mpAsBool)
                        {
                            apParse->mpErrorPoint = pPars;
                            stat = K2STAT_ERROR_OUT_OF_MEMORY;
                            break;
                        }
                        elem.mpAsBool->Hdr.mType = K2JSON_Bool;
                        elem.mpAsBool->Hdr.mpRawChars = pPars;
                        elem.mpAsBool->Hdr.mRawLen = 5;
                        elem.mpAsBool->mBool = FALSE;
                        pPars += 5;
                        left -= 5;
                    }

                    // must be followed by whitespace, a closing bracket, or a comma
                    c = *pPars;
                    if ((c != ' ') && (c != '\t') && (c != '\r') && (c != '\n') && (c != ']') && (c != ','))
                    {
                        apParse->mpErrorPoint = pPars;
                        break;  // bad format
                    }
                }
                else
                {
                    // anything else must parse as number
                    elem.mpAsNum = apParse->mfAlloc(sizeof(K2JSON_NUM));
                    if (NULL == elem.mpAsNum)
                    {
                        apParse->mpErrorPoint = pPars;
                        stat = K2STAT_ERROR_OUT_OF_MEMORY;
                        break;
                    }
                    stat = K2JSON_ParseAsNum(apParse, elem.mpAsNum, &pPars, &left);
                }
            }
            if (K2STAT_IS_ERROR(stat))
            {
                apParse->mfFree(elem.mpAsAny);
                break;
            }

            //
            // elem is ok
            //

            K2JSON_EatWhitespace(&pPars, &left);
            if (0 == left)
            {
                // no closing bracket or comma
                apParse->mpErrorPoint = pPars;
                stat = K2STAT_ERROR_END_OF_FILE;
                break;
            }

            //
            // expand array elements to include new element
            //
            pElems = (K2JSON_PTR *)apParse->mfAlloc(sizeof(K2JSON_PTR) * (apArray->mElementCount + 1));
            if (NULL == pElems)
            {
                apParse->mpErrorPoint = pPars;
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
                break;
            }
            if (apArray->mElementCount > 0)
            {
                K2MEM_Copy(pElems, apArray->mpElements, sizeof(K2JSON_PTR) * apArray->mElementCount);
                apParse->mfFree(apArray->mpElements);
            }
            pElems[apArray->mElementCount].mpAsAny = elem.mpAsAny;
            apArray->mpElements = pElems;
            apArray->mElementCount++;
            elem.mpAsAny = NULL;

            c = *pPars;
            if (']' == c)
            {
                pPars++;
                left--;
                apArray->Hdr.mRawLen = (UINT_PTR)(pPars - apArray->Hdr.mpRawChars);
                stat = K2STAT_NO_ERROR;
                break;
            }

            if (',' != c)
            {
                apParse->mpErrorPoint = pPars;
                break;  // bad format
            }
            pPars++;
            left--;

            K2JSON_EatWhitespace(&pPars, &left);

            if (0 == left)
            {
                apParse->mpErrorPoint = pPars;
                stat = K2STAT_ERROR_END_OF_FILE;
                break;
            }
            c = *pPars;

        } while (1);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        // purge all elements in array
        if (0 != apArray->mElementCount)
        {
            for (ix = 0; ix < apArray->mElementCount; ix++)
            {
                K2JSON_Purge(apParse->mfFree, apArray->mpElements[ix]);
            }
            apParse->mfFree(apArray->mpElements);
            apArray->mElementCount = 0;
            apArray->mpElements = NULL;
        }
    }
    else
    {
        // parse of array successful
        *appData = pPars;
        *apMaxDataLeft = left;
    }

    return stat;
}

K2STAT
K2JSON_ParseAsObj(
    K2JSON_PARSE *  apParse,
    K2JSON_OBJ *    apObj,
    char const **   appData,
    UINT_PTR *      apMaxDataLeft
)
{
    char const *    pPars;
    UINT_PTR        left;
    char            c;
    K2STAT          stat;
    K2JSON_PAIR     pair;
    UINT_PTR        ix;
    K2JSON_PAIR *   pPairs;

    left = *apMaxDataLeft;
    pPars = *appData;
    if (0 == left)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_END_OF_FILE;
    }

    K2MEM_Zero(apObj, sizeof(K2JSON_OBJ));
    apObj->Hdr.mType = K2JSON_Obj;
    apObj->Hdr.mpRawChars = pPars;

    K2JSON_EatWhitespace(&pPars, &left);
    if (0 == left)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_END_OF_FILE;
    }

    c = *pPars;
    if ('{' != c)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_BAD_FORMAT;
    }
    pPars++;
    left--;

    K2JSON_EatWhitespace(&pPars, &left);
    if (0 == left)
    {
        apParse->mpErrorPoint = pPars;
        return K2STAT_ERROR_END_OF_FILE;
    }

    c = *pPars;
    if ('}' == c)
    {
        // empty object
        pPars++;
        left--;
        apObj->Hdr.mRawLen = (UINT_PTR)(pPars - apObj->Hdr.mpRawChars);
        stat = K2STAT_NO_ERROR;
    }
    else
    {
        // not an empty object
        stat = K2STAT_ERROR_BAD_FORMAT;
        K2MEM_Zero(&pair, sizeof(pair));

        do {
            //
            // whitespace has been eaten and there is 
            // something left to parse.
            //

            if ('\"' != c)
            {
                apParse->mpErrorPoint = pPars;
                break;  // bad format
            }

            stat = K2JSON_ParseAsStr(apParse, &pair.Name, &pPars, &left);
            if (K2STAT_IS_ERROR(stat))
                break;

            //
            // eat colon
            //
            K2JSON_EatWhitespace(&pPars, &left);
            if (0 == left)
            {
                apParse->mpErrorPoint = pPars;
                stat = K2STAT_ERROR_END_OF_FILE;
                break;
            }
            c = *pPars;
            if (':' != c)
            {
                apParse->mpErrorPoint = pPars;
                break;  // bad format
            }
            pPars++;
            left--;
            K2JSON_EatWhitespace(&pPars, &left);
            if (0 == left)
            {
                apParse->mpErrorPoint = pPars;
                stat = K2STAT_ERROR_END_OF_FILE;
                break;
            }

            //
            // eat value for the pair
            // can be one of 
            //      [               array
            //      {               object
            //      "               string
            //      null            null
            //      true false      bool
            //      <digit>         number
            //
            c = *pPars;
            if ('[' == c)
            {
                pair.Value.mpAsArray = apParse->mfAlloc(sizeof(K2JSON_ARRAY));
                if (NULL == pair.Value.mpAsArray)
                {
                    apParse->mpErrorPoint = pPars;
                    stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    break;
                }
                stat = K2JSON_ParseAsArray(apParse, pair.Value.mpAsArray, &pPars, &left);
            }
            else if ('{' == c)
            {
                pair.Value.mpAsObj = apParse->mfAlloc(sizeof(K2JSON_OBJ));
                if (NULL == pair.Value.mpAsObj)
                {
                    apParse->mpErrorPoint = pPars;
                    stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    break;
                }
                stat = K2JSON_ParseAsObj(apParse, pair.Value.mpAsObj, &pPars, &left);
            }
            else if ('\"' == c)
            {
                pair.Value.mpAsStr = apParse->mfAlloc(sizeof(K2JSON_STR));
                if (NULL == pair.Value.mpAsStr)
                {
                    apParse->mpErrorPoint = pPars;
                    stat = K2STAT_ERROR_OUT_OF_MEMORY;
                    break;
                }
                stat = K2JSON_ParseAsStr(apParse, pair.Value.mpAsStr, &pPars, &left);
            }
            else
            {
                if (('n' == c) || ('t' == c) || ('f' == c))
                {
                    if ('n' == c)
                    {
                        // parse as null
                        if (left < 5)
                        {
                            apParse->mpErrorPoint = pPars;
                            break;  // bad format
                        }
                        if (0 != K2ASC_CompLen(pPars, "null", 4))
                        {
                            apParse->mpErrorPoint = pPars;
                            break;
                        }
                        // leave Value.mpAsAny as NULL;
                        pPars += 4;
                        left -= 4;
                    }
                    else if ('t' == c)
                    {
                        // parse as bool-true
                        if (left < 5)
                        {
                            apParse->mpErrorPoint = pPars;
                            break;
                        }
                        if (0 != K2ASC_CompLen(pPars, "true", 4))
                        {
                            apParse->mpErrorPoint = pPars;
                            break;
                        }
                        pair.Value.mpAsBool = apParse->mfAlloc(sizeof(K2JSON_BOOL));
                        if (NULL == pair.Value.mpAsBool)
                        {
                            apParse->mpErrorPoint = pPars;
                            stat = K2STAT_ERROR_OUT_OF_MEMORY;
                            break;
                        }
                        pair.Value.mpAsBool->Hdr.mType = K2JSON_Bool;
                        pair.Value.mpAsBool->Hdr.mpRawChars = pPars;
                        pair.Value.mpAsBool->Hdr.mRawLen = 4;
                        pair.Value.mpAsBool->mBool = TRUE;
                        pPars += 4;
                        left -= 4;
                    }
                    else // ('f' == c)
                    {
                        // parse as bool-false
                        if (left < 6)
                        {
                            apParse->mpErrorPoint = pPars;
                            break;
                        }
                        if (0 != K2ASC_CompLen(pPars, "false", 5))
                        {
                            apParse->mpErrorPoint = pPars;
                            break;
                        }
                        pair.Value.mpAsBool = apParse->mfAlloc(sizeof(K2JSON_BOOL));
                        if (NULL == pair.Value.mpAsBool)
                        {
                            apParse->mpErrorPoint = pPars;
                            stat = K2STAT_ERROR_OUT_OF_MEMORY;
                            break;
                        }
                        pair.Value.mpAsBool->Hdr.mType = K2JSON_Bool;
                        pair.Value.mpAsBool->Hdr.mpRawChars = pPars;
                        pair.Value.mpAsBool->Hdr.mRawLen = 5;
                        pair.Value.mpAsBool->mBool = FALSE;
                        pPars += 5;
                        left -= 5;
                    }

                    // must be followed by whitespace, a closing brace, or a comma
                    c = *pPars;
                    if ((c != ' ') && (c != '\t') && (c != '\r') && (c != '\n') && (c != ']') && (c != ','))
                    {
                        apParse->mpErrorPoint = pPars;
                        break;  // bad format
                    }
                }
                else
                {
                    // anything else must parse as number
                    pair.Value.mpAsNum = apParse->mfAlloc(sizeof(K2JSON_NUM));
                    if (NULL == pair.Value.mpAsNum)
                    {
                        apParse->mpErrorPoint = pPars;
                        stat = K2STAT_ERROR_OUT_OF_MEMORY;
                        break;
                    }
                    stat = K2JSON_ParseAsNum(apParse, pair.Value.mpAsNum, &pPars, &left);
                }
            }
            if (K2STAT_IS_ERROR(stat))
            {
                apParse->mfFree(pair.Value.mpAsAny);
                break;
            }

            //
            // pair is ok
            //

            K2JSON_EatWhitespace(&pPars, &left);
            if (0 == left)
            {
                // no closing brace or comma
                apParse->mpErrorPoint = pPars;
                stat = K2STAT_ERROR_END_OF_FILE;
                break;
            }

            //
            // expand object pairs to include new pair
            //
            pPairs = (K2JSON_PAIR *)apParse->mfAlloc(sizeof(K2JSON_PAIR) * (apObj->mPairCount + 1));
            if (NULL == pPairs)
            {
                apParse->mpErrorPoint = pPars;
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
                break;
            }
            if (apObj->mPairCount > 0)
            {
                K2MEM_Copy(pPairs, apObj->mpPairs, sizeof(K2JSON_PAIR) * apObj->mPairCount);
                apParse->mfFree(apObj->mpPairs);
            }
            K2MEM_Copy(&pPairs[apObj->mPairCount], &pair, sizeof(K2JSON_PAIR));
            apObj->mpPairs = pPairs;
            apObj->mPairCount++;
            K2MEM_Zero(&pair, sizeof(pair));

            c = *pPars;
            if ('}' == c)
            {
                pPars++;
                left--;
                apObj->Hdr.mRawLen = (UINT_PTR)(pPars - apObj->Hdr.mpRawChars);
                stat = K2STAT_NO_ERROR;
                break;
            }

            if (',' != c)
            {
                apParse->mpErrorPoint = pPars;
                break;  // bad format
            }
            pPars++;
            left--;

            K2JSON_EatWhitespace(&pPars, &left);

            if (0 == left)
            {
                apParse->mpErrorPoint = pPars;
                stat = K2STAT_ERROR_END_OF_FILE;
                break;
            }
            c = *pPars;

        } while (1);
    }

    if (K2STAT_IS_ERROR(stat))
    {
        // purge all pairs in object
        if (0 != apObj->mPairCount)
        {
            for (ix = 0; ix < apObj->mPairCount; ix++)
            {
                K2JSON_Purge(apParse->mfFree, apObj->mpPairs[ix].Value);
            }
            apParse->mfFree(apObj->mpPairs);
            apObj->mpPairs = NULL;
            apObj->mPairCount = 0;
        }
    }
    else
    {
        // parse of object successful
        *appData = pPars;
        *apMaxDataLeft = left;
    }

    return stat;
}

K2STAT
K2JSON_Parse(
    K2JSON_pf_MemAlloc  afAlloc,
    K2JSON_pf_MemFree   afFree,
    char const *        apData,
    UINT_PTR            aMaxDataLen,
    K2JSON_PARSE *      apRetParse
)
{
    if ((NULL == afAlloc) ||
        (NULL == afFree) ||
        (NULL == apData) ||
        (NULL == apRetParse))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }
    K2MEM_Zero(apRetParse, sizeof(K2JSON_PARSE));
    apRetParse->mfAlloc = afAlloc;
    apRetParse->mfFree = afFree;
    if (0 == aMaxDataLen)
    {
        apRetParse->mpErrorPoint = apData;
        return K2STAT_ERROR_END_OF_FILE;
    }
    return K2JSON_ParseAsObj(apRetParse, &apRetParse->RootObj, &apData, &aMaxDataLen);
}
