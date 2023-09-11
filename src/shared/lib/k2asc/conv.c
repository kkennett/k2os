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
#include <lib/k2asc.h>

char         
K2ASC_ToUpper(
    char    aCh
    )
{
    if ((aCh >= 'a') && (aCh <= 'z'))
        aCh -= ('a' - 'A');
    return aCh;
}

char         
K2ASC_ToLower(
    char    aCh
    )
{
    if ((aCh >= 'A') && (aCh <= 'Z'))
        aCh += ('a' - 'A');
    return aCh;
}

K2_NUMTYPE 
K2ASC_NumTypeLen(
    char const *    apStr,
    UINT_PTR        aStrLen
    )
{
    BOOL        isIntNum;
    BOOL        isHex;
    UINT_PTR    numDigits;
    char        maxSaw;
    char        chk;

    if (aStrLen == 0)
        return K2_NUMTYPE_None;
    
    chk = *apStr;
    if (chk == 0)
        return K2_NUMTYPE_None;

    if (aStrLen==1)
    {
        if ((chk < '0') || (chk > '9'))
            return K2_NUMTYPE_None;
        return K2_NUMTYPE_Card;
    }

    isIntNum = FALSE;
    isHex = FALSE;
    numDigits = 0;
    maxSaw = '0';

    if (chk=='-')
    {
        /* starts with a minus - either integer or nan */
        isIntNum = TRUE;
        apStr++;
        aStrLen--;
    }
    else if (chk=='0')
    {
        apStr++;
        aStrLen--;
        chk = *apStr;
        if ((chk=='X') || (chk=='x'))
        {
            isHex = TRUE;
            apStr++;
            aStrLen--;
            if (aStrLen == 0)
                return K2_NUMTYPE_None;
        }
    }

    do
    {
        chk = *apStr;
        if (((chk >= 'a') && (chk <= 'f')) ||
            ((chk >= 'A') && (chk <= 'F')))
        {
            if (!isHex)
                return K2_NUMTYPE_None;
        }
        else if (!((chk >= '0') && (chk <= '9')))
            return K2_NUMTYPE_None;
        if (chk > maxSaw)
            maxSaw = chk;
        numDigits++;
        apStr++;
    } while (--aStrLen);

    if (numDigits == 0)
        return K2_NUMTYPE_None;

    /* we passed at least one digit before we hit a nondigit */
    if (aStrLen > 1)
        return K2_NUMTYPE_None; /* more than one char after number - can't be binary. */

    if (aStrLen == 1)
    {
        // if it had 0x at the beginning or a minus sign then it should
        // not have anything at the end of it
        if ((isHex) || (isIntNum))
            return K2_NUMTYPE_None;
        
        if ((*apStr=='b') || (*apStr=='B'))
        {
            if (maxSaw > '1')
            {
                /* number specified as binary had a digit bigger than one, hex designation, or a minus sign at the start of it. */
                return K2_NUMTYPE_None; 
            }
            return K2_NUMTYPE_Bin;
        }
        
        if ((*apStr=='k') || (*apStr=='K'))
            return K2_NUMTYPE_Kilo;
        
        if ((*apStr=='m') || (*apStr=='M'))
            return K2_NUMTYPE_Mega;
        
        /* stuff after number is unknown goop */
        return K2_NUMTYPE_None;
    }

    /* we made it to the end of the string without hitting a nondigit */
    if (isHex)
        return K2_NUMTYPE_Hex;

    if (isIntNum)
        return K2_NUMTYPE_Int;

    return K2_NUMTYPE_Card;
}

INT32       
K2ASC_NumValue32Len(
    char const *    apStr,
    UINT_PTR        aStrLen
    )
{
    char        chk;
    INT32       retVal;
    K2_NUMTYPE  got;
    
    got = K2ASC_NumTypeLen(apStr, aStrLen);

    /* now that we know what type of number it is we know how to scan it */

    if (got == K2_NUMTYPE_None)
        return 0;

    if (got == K2_NUMTYPE_Hex)
    {
        /* bypass 0x at beginning */
        apStr += 2;
        aStrLen -= 2;
    }
    else if (got == K2_NUMTYPE_Int)
    {
        /* bypass the - at the beginning */
        apStr++;
        aStrLen--;
    }

    retVal = 0;

    switch (got)
    {
    case K2_NUMTYPE_None:
        return 0;
    case K2_NUMTYPE_Kilo:
    case K2_NUMTYPE_Mega:
        aStrLen -= 1; /* dont parse the last character */
        /* fall thru */
    case K2_NUMTYPE_Card:
    case K2_NUMTYPE_Int:
        do {
            chk = *apStr;
            retVal = (retVal*10) + (chk-'0');
            apStr++;
        } while (--aStrLen);
        break;
    case K2_NUMTYPE_Hex:
        do {
            chk = *apStr;
            if (chk>'9')
            {
                if (chk>='a')
                    chk -= ('a'-'A');
                chk -= 'A';
                chk += 10;
            }
            else
                chk -= '0';
            retVal = (retVal * 16) + ((INT32)((UINT8)chk));
            apStr++;
        } while (--aStrLen);
        break;
    case K2_NUMTYPE_Bin:
        do {
            chk = *apStr;
            retVal = (retVal * 2) + (chk - '0');
            apStr++;
        } while (aStrLen>1);
        break;
    }
    if (got == K2_NUMTYPE_Int)
        return -retVal;
    if (got == K2_NUMTYPE_Kilo)
        return (INT32)(((UINT32)retVal)*1024);
    if (got == K2_NUMTYPE_Mega)
        return (INT32)(((UINT32)retVal)*1024*1024);
    return retVal;
}

