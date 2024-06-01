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

#include <lib/k2dbus.h>
#include <lib/k2mem.h>
#include <lib/k2parse.h>

K2DBUS_SIG_TYPE const gK2DBUS_Types[K2DBUS_SigType_Count] =
{
    { 0, 0, 0, "Invalid" },
    { 'a', K2DBUS_Sig_ARRAY,    4, "ARRAY" },
    { 'b', K2DBUS_Sig_BOOLEAN,  4, "BOOLEAN" },
    { 'd', K2DBUS_Sig_DOUBLE,   8, "DOUBLE" },
    { 'e', K2DBUS_Sig_DICTENT,  8, "DICTENT" },
    { 'f', K2DBUS_Sig_FLOAT,    4, "FLOAT" },
    { 'g', K2DBUS_Sig_SIG,      1, "SIG" },
    { 'h', K2DBUS_Sig_TOKEN,    4, "TOKEN" },
    { 'i', K2DBUS_Sig_INT32,    4, "INT32" },
    { 'n', K2DBUS_Sig_INT16,    2, "INT16" },
    { 'o', K2DBUS_Sig_OBJPATH,  4, "OBJPATH" },
    { 'q', K2DBUS_Sig_UINT16,   2, "UINT16" },
    { 'r', K2DBUS_Sig_STRUCT,   8, "STRUCT" },
    { 's', K2DBUS_Sig_STRZ,     4, "STRZ" },
    { 't', K2DBUS_Sig_UINT64,   8, "UINT64" },
    { 'u', K2DBUS_Sig_UINT32,   4, "UINT32" },
    { 'v', K2DBUS_Sig_VARIANT,  1, "VARIANT" },
    { 'x', K2DBUS_Sig_INT64,    8, "INT64" },
    { 'y', K2DBUS_Sig_BYTE,     1, "BYTE" },
};

K2DBUS_SigType
K2DBUS_FindSigType(
    char aCh
)
{
    UINT32 ix;
    if (0 == aCh)
        return K2DBUS_SigType_Invalid;
    for (ix = 1; ix < K2DBUS_SigType_Count; ix++)
    {
        if (gK2DBUS_Types[ix].mChar == aCh)
            break;
    }
    if (ix == K2DBUS_SigType_Count)
        return K2DBUS_SigType_Invalid;
    return (K2DBUS_SigType)ix;
}

K2STAT
K2DBUS_GetOneSimpleTypeSigLength(
    char const *apSig,
    UINT32      aSigLen,
    UINT32 *    apRetSimpleLen,
    BOOL        aLastWasArray
);

K2STAT 
K2DBUS_SigIsWellFormedInternal(
    char const *    apSig,
    UINT32          aMaxLen,
    UINT32 *        apRetSigLen,
    BOOL            aInsideStruct
)
{
    UINT32  sigLen;
    UINT32  simpleLen;
    K2STAT  stat;
    char    c;

    if (NULL != apRetSigLen)
        *apRetSigLen = 0;

    if (0 == aMaxLen)
        return K2STAT_NO_ERROR;

    sigLen = 0;
    do {
        c = *apSig;

        if (aInsideStruct)
        {
            if (0 == c)
                return K2STAT_ERROR_BAD_FORMAT;

            if (')' == c)
                break;
        }
        else
        {
            if (0 == c)
                break;
        }

        stat = K2DBUS_GetOneSimpleTypeSigLength(apSig, aMaxLen, &simpleLen, FALSE);
        if (K2STAT_IS_ERROR(stat))
            return stat;

        sigLen += simpleLen;
        apSig += simpleLen;
        aMaxLen -= simpleLen;

    } while (0 != aMaxLen);

    if (NULL != apRetSigLen)
        *apRetSigLen = sigLen;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_GetOneStructSigLength(
    char const *apSig,
    UINT32      aSigLen,
    UINT32 *    apRetSimpleLen
    )
{
    char    c;
    UINT32  subLen;
    K2STAT  stat;

    if (NULL != apRetSimpleLen)
        *apRetSimpleLen = 0;

    if (0 == aSigLen)
        return K2STAT_ERROR_EMPTY;

    // should be at opening bracket
    c = *apSig;
    K2_ASSERT('(' == c);

    // eat bracket
    apSig++;
    aSigLen--;

    if (0 == aSigLen)
        return K2STAT_ERROR_BAD_FORMAT;

    stat = K2DBUS_SigIsWellFormedInternal(apSig, aSigLen, &subLen, TRUE);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (NULL != apRetSimpleLen)
    {
        // open bracket, sub, close bracket
        (*apRetSimpleLen) = 1 + subLen + 1;
    }

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_GetOneSimpleTypeSigLength(
    char const *apSig,
    UINT32      aSigLen,
    UINT32 *    apRetSimpleLen,
    BOOL        aLastWasArray
)
{
    char            c;
    K2DBUS_SigType  sigType;
    UINT32          subLen;
    K2STAT          stat;

    if (NULL != apRetSimpleLen)
        *apRetSimpleLen = 0;

    if (0 == aSigLen)
        return K2STAT_ERROR_EMPTY;

    c = *apSig;

    if ('(' == c)
    {
        return K2DBUS_GetOneStructSigLength(apSig, aSigLen, apRetSimpleLen);
    }

    if ('{' == c)
    {
        // dictent can only appear as array type
        // must be at least brace,two chars,brace 
        if ((!aLastWasArray) || (4 > aSigLen))  
            return K2STAT_ERROR_BAD_FORMAT;

        // eat brace
        apSig++;
        aSigLen--;

        stat = K2DBUS_GetOneSimpleTypeSigLength(apSig, aSigLen, &subLen, FALSE);
        if (K2STAT_IS_ERROR(stat))
            return stat;

        // first simple type must be a basic type
        sigType = K2DBUS_FindSigType(*apSig);
        if ((K2DBUS_SigType_Invalid == sigType) ||
            (K2DBUS_Sig_STRUCT == sigType) ||
            (K2DBUS_Sig_DICTENT == sigType) ||
            (K2DBUS_Sig_ARRAY == sigType) ||
            (K2DBUS_Sig_VARIANT == sigType))
            return K2STAT_ERROR_BAD_FORMAT;
        K2_ASSERT(1 == subLen);

        // eat the basic type
        apSig++;
        aSigLen--;

        // this can be anything
        stat = K2DBUS_GetOneSimpleTypeSigLength(apSig, aSigLen, &subLen, FALSE);
        if (K2STAT_IS_ERROR(stat))
            return stat;

        // eat the type
        apSig += subLen;
        aSigLen -= subLen;
        if (0 == subLen)
            return K2STAT_ERROR_BAD_FORMAT;

        // that's all that is allowed
        if ('}' != *apSig)
            return K2STAT_ERROR_BAD_FORMAT;

        // eat the closing brace
        apSig++;
        aSigLen--;

        // brace, basic type, second type, brace
        if (NULL != apRetSimpleLen)
            (*apRetSimpleLen) = (1 + 1 + subLen + 1);

        return K2STAT_NO_ERROR;
    }

    sigType = K2DBUS_FindSigType(c);
    if ((K2DBUS_SigType_Invalid == sigType) ||
        (K2DBUS_Sig_STRUCT == sigType) ||
        (K2DBUS_Sig_DICTENT == sigType))
    {
        // invalid char
        return K2STAT_ERROR_BAD_FORMAT;
    }

    if (sigType == K2DBUS_Sig_ARRAY)
    {
        // eat the array type
        apSig++;
        aSigLen--;

        stat = K2DBUS_GetOneSimpleTypeSigLength(apSig, aSigLen, &subLen, TRUE);
        if (K2STAT_IS_ERROR(stat))
            return stat;

        // eat the element type
        apSig += subLen;
        aSigLen -= subLen;

        // array type + element type
        if (NULL != apRetSimpleLen)
            (*apRetSimpleLen) = 1 + subLen;

        return K2STAT_NO_ERROR;
    }

    // not struct, not array, not dictent
    // so type code is one char thats it
    if (NULL != apRetSimpleLen)
        (*apRetSimpleLen) = 1;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_SigIsWellFormed(
    char const *    apSig,
    UINT32          aMaxLen,
    UINT32 *        apRetSigLen
)
{
    return K2DBUS_SigIsWellFormedInternal(apSig, aMaxLen, apRetSigLen, FALSE);
}

BOOL
K2DBUS_AlignUp(
    UINT32 *apAddr,
    UINT32 *apLeft,
    UINT32 aAlignTo
)
{
    UINT32 v;
    UINT32 left;

    K2_ASSERT(0 != aAlignTo);

    left = *apLeft;
    if (0 == left)
        return FALSE;

    v = *apAddr;
    if (0 == (v % aAlignTo))
        return TRUE;

    // not aligned
    do {
        if (0 == left)
            return FALSE;   // no more data

        if (0 != *((UINT8 const *)v))
            return FALSE;   // padding is not zero

        v++;
        left--;
    } while (0 != (v % aAlignTo));

    *apAddr = v;
    *apLeft = left;

    return TRUE;
}

BOOL 
K2DBUS_StringIsValidObjPath(
    char const *apStr
)
{
    char    c;
    UINT32  elemLen;

    //
    // The path may be of any length.
    // The path must begin with an ASCII '/' (integer 47) character, and must consist of elements separated by slash characters.
    // Each element must only contain the ASCII characters "[A-Z][a-z][0-9]_"
    // No element may be the empty string.
    // Multiple '/' characters cannot occur in sequence.
    // A trailing '/' character is not allowed unless the path is the root path(a single '/' character).
    // 

    if (*apStr != '/')
        return FALSE;
    apStr++;

    // A trailing '/' character is not allowed unless the path is the root path(a single '/' character).
    c = *apStr;
    if (0 == c)
        return TRUE;

    elemLen = 0;

    do {
        if (c == 0)
        {
            if (0 == elemLen)
                return FALSE;
            break;
        }

        if (c == '/')
        {
            if (0 == elemLen)
                return FALSE;
            elemLen = 0;
        }
        else
        {
            if (!
                (
                 ((c >= '0') && (c <= '9')) ||
                 ((c >= 'a') && (c <= 'z')) ||
                 ((c >= 'A') && (c <= 'Z')) ||
                 (c == '_')
                )
            )
                return FALSE;
            elemLen++;
        }
        apStr++;
        c = *apStr;
    } while (1);

    return TRUE;
}

BOOL 
K2DBUS_StringIsValidInterface(
    char const *apStr
)
{
    char    c;
    UINT32  elemLen;
    UINT32  maxLeft;
    BOOL    hasTwoElem;

    //
    // Interface names are composed of 2 or more elements separated by a period('.') character.
    // All elements must contain at least one character.
    // Each element must only contain the ASCII characters "[A-Z][a-z][0-9]_" and must not begin with a digit.
    // Interface names must not exceed the maximum name length.
    // 

    elemLen = 0;
    maxLeft = K2DBUS_NAME_MAX_LEN;
    hasTwoElem = FALSE;

    c = *apStr;

    do {
        if (c == 0)
        {
            if (0 == elemLen)
                return FALSE;
            break;
        }

        if (0 == --maxLeft)
            return FALSE;

        if (c == '.')
        {
            if (0 == elemLen)
                return FALSE;
            hasTwoElem = TRUE;
            elemLen = 0;
        }
        else
        {
            if (0 == elemLen)
            {
                if ((c >= '0') && (c <= '9'))
                    return FALSE;
            }

            if (!
                (
                    ((c >= '0') && (c <= '9')) ||
                    ((c >= 'a') && (c <= 'z')) ||
                    ((c >= 'A') && (c <= 'Z')) ||
                    (c == '_')
                    )
                )
                return FALSE;
            elemLen++;
        }

        apStr++;
        c = *apStr;

    } while (1);

    if (!hasTwoElem)
        return FALSE;

    return TRUE;
}

BOOL 
K2DBUS_StringIsValidMemberName(
    char const *apStr
)
{
    char    c;
    UINT32  maxLeft;

    //
    // Must only contain the ASCII characters "[A-Z][a-z][0-9]_" and may not begin with a digit.
    // Must not exceed the maximum name length.
    // Must be at least 1 byte in length.
    //

    c = *apStr;
    if ((0 == c) ||
        ((c >= '0') && (c <= '9')))
        return FALSE;

    maxLeft = K2DBUS_NAME_MAX_LEN - 1;

    do {
        if (0 == --maxLeft)
            return FALSE;

        if (!
            (
                ((c >= '0') && (c <= '9')) ||
                ((c >= 'a') && (c <= 'z')) ||
                ((c >= 'A') && (c <= 'Z')) ||
                (c == '_')
                )
            )
            return FALSE;

        apStr++;
        c = *apStr;

    } while (0 != c);

    return TRUE;
}

BOOL 
K2DBUS_StringIsValidErrorName(
    char const *apStr
)
{
    // Error names have the same restrictions as interface names.
    return K2DBUS_StringIsValidInterface(apStr);
}

BOOL 
K2DBUS_StringIsValidBusName(
    char const *apStr
)
{
    char    c;
    UINT32  elemLen;
    UINT32  maxLeft;
    BOOL    hasTwoElem;
    BOOL    isUniqueConn;

    //
    // Bus names that start with a colon(':') character are unique connection names.Other bus names are called well - known bus names.
    // Bus names are composed of 1 or more elements separated by a period('.') character. 
    // All elements must contain at least one character.
    // Each element must only contain the ASCII characters "[A-Z][a-z][0-9]_-", with "-" discouraged in new bus names.
    // Only elements that are part of a unique connection name may begin with a digit, elements in other bus names must not begin with a digit.
    // Bus names must contain at least one '.' (period)character(and thus at least two elements).
    // Bus names must not begin with a '.' (period)character.
    // Bus names must not exceed the maximum name length.
    // 

    elemLen = 0;
    maxLeft = K2DBUS_NAME_MAX_LEN;
    hasTwoElem = FALSE;

    c = *apStr;

    if ((0 == c) || ('.' == c))
        return FALSE;

    if (':' == c)
    {
        isUniqueConn = TRUE;
        apStr++;
        maxLeft--;
        c = *apStr;
    }
    else
    {
        isUniqueConn = FALSE;
    }

    do {
        if (0 == elemLen)
        {
            if (0 == c)
                return FALSE;

            if (!isUniqueConn)
            {
                if ((c >= '0') && (c <= '9'))
                    return FALSE;
            }

            if (c == '.')
                return FALSE;
        }

        if (0 == c)
            break;

        if (0 == --maxLeft)
            return FALSE;

        if (c == '.')
        {
            hasTwoElem = TRUE;
            elemLen = 0;
        }
        else
        {
            if (!
                (
                    ((c >= '0') && (c <= '9')) ||
                    ((c >= 'a') && (c <= 'z')) ||
                    ((c >= 'A') && (c <= 'Z')) ||
                    (c == '_') ||
                    (c == '-')
                    )
                )
                return FALSE;
            elemLen++;
        }

        apStr++;
        c = *apStr;

    } while (1);

    if (!hasTwoElem)
        return FALSE;

    return TRUE;
}

K2STAT
K2DBUS_Eat_AlignedBYTE(
    K2DBUS_PARSE *  apParse,
    UINT8 *         apRetByte
)
{
    K2_ASSERT((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_BYTE].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;

    if (NULL != apRetByte)
        *apRetByte = *((UINT8 *)apParse->mMemAddr);

    apParse->mMemAddr++;
    apParse->mMemLeft--;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_Eat_AlignedBOOLEAN(
    K2DBUS_PARSE *  apParse,
    BOOL *          apRetBool
)
{
    UINT32 align;

    K2_ASSERT(*apParse->mpSig == gK2DBUS_Types[K2DBUS_Sig_BOOLEAN].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;
        
    align = gK2DBUS_Types[K2DBUS_Sig_BOOLEAN].mAlign;
    K2_ASSERT(0 == (apParse->mMemAddr % align));
    if (align > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    if (NULL != apRetBool)
        *apRetBool = ((*((UINT32 *)apParse->mMemAddr)) == 0) ? 0 : 1;

    apParse->mMemAddr += align;
    apParse->mMemLeft -= align;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_Eat_AlignedDOUBLE(
    K2DBUS_PARSE *  apParse,
    double *        apRetDouble
)
{
    UINT32 align;

    K2_ASSERT((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_DOUBLE].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;

    align = gK2DBUS_Types[K2DBUS_Sig_DOUBLE].mAlign;
    K2_ASSERT(0 == (apParse->mMemAddr % align));
    if (align > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    K2_ASSERT(0 == (apParse->mMemAddr % align));

    if (NULL != apRetDouble)
        *apRetDouble = *((double *)apParse->mMemAddr);

    apParse->mMemAddr += align;
    apParse->mMemLeft -= align;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_Eat_AlignedFLOAT(
    K2DBUS_PARSE *  apParse,
    float *         apRetFloat
)
{
    UINT32 align;

    K2_ASSERT((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_FLOAT].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;

    align = gK2DBUS_Types[K2DBUS_Sig_FLOAT].mAlign;
    K2_ASSERT(0 == (apParse->mMemAddr % align));
    if (align > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    K2_ASSERT(0 == (apParse->mMemAddr % align));

    if (NULL != apRetFloat)
        *apRetFloat = *((float *)apParse->mMemAddr);

    apParse->mMemAddr += align;
    apParse->mMemLeft -= align;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_Eat_AlignedTOKEN(
    K2DBUS_PARSE *  apParse,
    UINT32 *        apRetToken
)
{
    UINT32 align;

    K2_ASSERT((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_TOKEN].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;

    align = gK2DBUS_Types[K2DBUS_Sig_TOKEN].mAlign;
    K2_ASSERT(0 == (apParse->mMemAddr % align));
    if (align > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    K2_ASSERT(0 == (apParse->mMemAddr % align));

    if (NULL != apRetToken)
        *apRetToken = *((UINT32 *)apParse->mMemAddr);

    apParse->mMemAddr += align;
    apParse->mMemLeft -= align;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_Eat_AlignedINT32(
    K2DBUS_PARSE *  apParse,
    INT32 *         apRetInt32
)
{
    UINT32 align;

    K2_ASSERT((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_INT32].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;

    align = gK2DBUS_Types[K2DBUS_Sig_INT32].mAlign;
    K2_ASSERT(0 == (apParse->mMemAddr % align));
    if (align > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    K2_ASSERT(0 == (apParse->mMemAddr % align));

    if (NULL != apRetInt32)
        *apRetInt32 = *((INT32 *)apParse->mMemAddr);

    apParse->mMemAddr += align;
    apParse->mMemLeft -= align;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_Eat_AlignedINT16(
    K2DBUS_PARSE *  apParse,
    INT16 *         apRetInt16
)
{
    UINT32 align;

    K2_ASSERT((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_INT16].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;

    align = gK2DBUS_Types[K2DBUS_Sig_INT16].mAlign;
    K2_ASSERT(0 == (apParse->mMemAddr % align));
    if (align > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    K2_ASSERT(0 == (apParse->mMemAddr % align));

    if (NULL != apRetInt16)
        *apRetInt16 = *((INT16 *)apParse->mMemAddr);

    apParse->mMemAddr += align;
    apParse->mMemLeft -= align;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_Eat_AlignedUINT16(
    K2DBUS_PARSE *  apParse,
    UINT16 *        apRetUint16
)
{
    UINT32 align;

    K2_ASSERT((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_UINT16].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;

    align = gK2DBUS_Types[K2DBUS_Sig_UINT16].mAlign;
    K2_ASSERT(0 == (apParse->mMemAddr % align));
    if (align > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    K2_ASSERT(0 == (apParse->mMemAddr % align));

    if (NULL != apRetUint16)
        *apRetUint16 = *((UINT16 *)apParse->mMemAddr);

    apParse->mMemAddr += align;
    apParse->mMemLeft -= align;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_Eat_AlignedUINT64(
    K2DBUS_PARSE *  apParse,
    UINT64 *        apRetUint64
)
{
    UINT32 align;

    K2_ASSERT((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_UINT64].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;

    align = gK2DBUS_Types[K2DBUS_Sig_UINT64].mAlign;
    K2_ASSERT(0 == (apParse->mMemAddr % align));
    if (align > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    K2_ASSERT(0 == (apParse->mMemAddr % align));

    if (NULL != apRetUint64)
        *apRetUint64 = *((UINT64 *)apParse->mMemAddr);

    apParse->mMemAddr += align;
    apParse->mMemLeft -= align;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_Eat_AlignedUINT32(
    K2DBUS_PARSE *  apParse,
    UINT32 *        apRetUint32
)
{
    UINT32 align;

    K2_ASSERT((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_UINT32].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;

    align = gK2DBUS_Types[K2DBUS_Sig_UINT32].mAlign;
    K2_ASSERT(0 == (apParse->mMemAddr % align));
    if (align > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    K2_ASSERT(0 == (apParse->mMemAddr % align));

    if (NULL != apRetUint32)
        *apRetUint32 = *((UINT32 *)apParse->mMemAddr);

    apParse->mMemAddr += align;
    apParse->mMemLeft -= align;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_Eat_AlignedINT64(
    K2DBUS_PARSE *  apParse,
    INT64 *         apRetInt64
)
{
    UINT32 align;

    K2_ASSERT((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_INT64].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;

    align = gK2DBUS_Types[K2DBUS_Sig_INT64].mAlign;
    K2_ASSERT(0 == (apParse->mMemAddr % align));
    if (align > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    K2_ASSERT(0 == (apParse->mMemAddr % align));

    if (NULL != apRetInt64)
        *apRetInt64 = *((INT64 *)apParse->mMemAddr);

    apParse->mMemAddr += align;
    apParse->mMemLeft -= align;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_Eat_AlignedSTRZ(
    K2DBUS_PARSE *  apParse,
    char const **   apRetStr,
    UINT32 *        apRetStrLen
)
{
    UINT32          align;
    UINT32          strLen;
    UINT32          strLeft;
    char const *    pStr;
    char            c;

    K2_ASSERT(((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_STRZ].mChar) ||
        ((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_OBJPATH].mChar));
    apParse->mpSig++;
    apParse->mSigLen--;

    align = gK2DBUS_Types[K2DBUS_Sig_STRZ].mAlign;
    K2_ASSERT(0 == (apParse->mMemAddr % align));
    if (align > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    strLeft = strLen = *((UINT32 const *)apParse->mMemAddr);
    apParse->mMemAddr += align;
    apParse->mMemLeft -= align;
    if (strLen > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    pStr = (char const *)apParse->mMemAddr;
    if (NULL != apRetStr)
        *apRetStr = pStr;
    if (NULL != apRetStrLen)
        *apRetStrLen = strLen;

    if (0 != strLeft)
    {
        do {
            c = *pStr;
            if (0 == c)
                return K2STAT_ERROR_BAD_FORMAT;
            pStr++;
        } while (--strLeft);
    }

    if (0 != *pStr)
        return K2STAT_ERROR_BAD_FORMAT;

    apParse->mMemAddr += strLen + 1;
    apParse->mMemLeft -= strLen + 1;

    return K2STAT_NO_ERROR;
}

K2STAT
K2DBUS_Eat_SIG(
    K2DBUS_PARSE *  apParse,
    char const **   apRetSig,
    UINT32 *        apRetSigLen
)
{
    UINT32      sigLen;
    UINT32      sigLeft;
    char const *pSig;
    UINT32      simpleLen;
    K2STAT      stat;

    K2_ASSERT((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_SIG].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;

    if (0 == apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    sigLeft = sigLen = *((UINT8 const *)apParse->mMemAddr);
    apParse->mMemAddr++;
    apParse->mMemLeft--;
    if (sigLen > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    pSig = (char const *)apParse->mMemAddr;
    if (NULL != apRetSig)
        *apRetSig = pSig;
    if (NULL != apRetSigLen)
        *apRetSigLen = sigLen;

    do {
        stat = K2DBUS_GetOneSimpleTypeSigLength(pSig, sigLeft, &simpleLen, FALSE);
        if (K2STAT_IS_ERROR(stat))
            return stat;
        pSig += simpleLen;
        sigLeft -= simpleLen;
    } while (0 != sigLeft);

    if (0 != *((UINT8 const *)pSig))
        return K2STAT_ERROR_BAD_FORMAT;

    apParse->mMemAddr += sigLen + 1;
    apParse->mMemLeft -= sigLen + 1;

    return K2STAT_NO_ERROR;
}

K2STAT K2DBUS_ValidateInternal(K2DBUS_PARSE *apParse, BOOL aInStruct, BOOL aDoSingle, BOOL aPartOfArray);

K2STAT 
K2DBUS_Eat_AlignedARRAY(
    K2DBUS_PARSE *  apParse,
    UINT32 *        apRetCount,
    char const **   apRetElemSig,
    UINT32 *        apRetElemSigLen,
    UINT8 const **  apRetElemsStart,
    UINT32 *        apRetElemsLen
)
{
    UINT32          align;
    K2STAT          stat;
    UINT32          arrayBytes;
    K2DBUS_PARSE    parse;
    char const *    pElemSig;
    UINT32          elemSigLen;

    K2_ASSERT((*apParse->mpSig) == gK2DBUS_Types[K2DBUS_Sig_ARRAY].mChar);
    apParse->mpSig++;
    apParse->mSigLen--;

    align = gK2DBUS_Types[K2DBUS_Sig_ARRAY].mAlign;
    K2_ASSERT(0 == (apParse->mMemAddr % align));
    if (align > apParse->mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    K2_ASSERT(0 == (apParse->mMemAddr % align));

    arrayBytes = *((UINT32 *)apParse->mMemAddr);
    apParse->mMemAddr += sizeof(UINT32);
    apParse->mMemLeft -= sizeof(UINT32);
    if (NULL != apRetCount)
        *apRetCount = 0;
    if (NULL != apRetElemsStart)
        *apRetElemsStart = (UINT8 const *)apParse->mMemAddr;
    if (NULL != apRetElemsLen)
        *apRetElemsLen = arrayBytes;

    pElemSig = apParse->mpSig;
    if (NULL != apRetElemSig)
        *apRetElemSig = pElemSig;
    elemSigLen = 0;
    stat = K2DBUS_GetOneSimpleTypeSigLength(apParse->mpSig, apParse->mSigLen, &elemSigLen, TRUE);
    if (K2STAT_IS_ERROR(stat))
        return stat;
    if (NULL != apRetElemSigLen)
        *apRetElemSigLen = elemSigLen;

    apParse->mpSig += elemSigLen;
    apParse->mSigLen -= elemSigLen;

    if (0 == arrayBytes)
    {
        // no data
        return K2STAT_NO_ERROR;
    }

    parse.mMemAddr = apParse->mMemAddr;
    parse.mMemLeft = arrayBytes;
    do {
        parse.mpSig = pElemSig;
        parse.mSigLen = elemSigLen;
        stat = K2DBUS_ValidateInternal(&parse, FALSE, FALSE, TRUE);
        if (K2STAT_IS_ERROR(stat))
            return stat;
        if (NULL != apRetCount)
            (*apRetCount)++;
    } while (parse.mMemLeft > 0);

    apParse->mMemAddr = parse.mMemAddr;
    apParse->mMemLeft = apParse->mMemLeft - arrayBytes;

    return K2STAT_NO_ERROR;
}

K2STAT 
K2DBUS_Eat_AlignedDICTENT(
    K2DBUS_PARSE *  apParse,
    char const **   apRetElemSig,
    UINT32 *        apRetElemSigLen,
    UINT32 *        apRetElemLen
)
{
    char            c;
    K2DBUS_PARSE    parse;
    UINT32          subLen;
    K2STAT          stat;
    K2DBUS_SigType  sigType;
    char const *    pStr;

    K2MEM_Copy(&parse, apParse, sizeof(parse));

    c = (*parse.mpSig);
    if ('{' != c)
        return K2STAT_ERROR_BAD_FORMAT;
    parse.mpSig++;
    parse.mSigLen--;
    if (0 == parse.mSigLen)
        return K2STAT_ERROR_BAD_FORMAT;

    if (NULL != apRetElemSig)
        *apRetElemSig = parse.mpSig;
    if (NULL != apRetElemSigLen)
        *apRetElemSigLen = 0;
    if (NULL != apRetElemLen)
        *apRetElemLen = 0;

    stat = K2DBUS_GetOneSimpleTypeSigLength(parse.mpSig, parse.mSigLen, &subLen, FALSE);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    // first simple type must be a basic type
    sigType = K2DBUS_FindSigType(*parse.mpSig);
    if ((K2DBUS_SigType_Invalid == sigType) ||
        (K2DBUS_Sig_STRUCT == sigType) ||
        (K2DBUS_Sig_DICTENT == sigType) ||
        (K2DBUS_Sig_ARRAY == sigType) ||
        (K2DBUS_Sig_VARIANT == sigType))
        return K2STAT_ERROR_BAD_FORMAT;
    K2_ASSERT(1 == subLen);

    // eat the basic type and its content
    if (!K2DBUS_AlignUp(&parse.mMemAddr, &parse.mMemLeft, gK2DBUS_Types[sigType].mAlign))
        return K2STAT_ERROR_BAD_FORMAT;
    switch (sigType)
    {
    case K2DBUS_Sig_BOOLEAN:
        stat = K2DBUS_Eat_AlignedBOOLEAN(&parse, NULL);
        break;
    case K2DBUS_Sig_DOUBLE:
        stat = K2DBUS_Eat_AlignedDOUBLE(&parse, NULL);
        break;
    case K2DBUS_Sig_FLOAT:
        stat = K2DBUS_Eat_AlignedFLOAT(&parse, NULL);
        break;
    case K2DBUS_Sig_SIG:
        stat = K2DBUS_Eat_SIG(&parse, NULL, NULL);
        break;
    case K2DBUS_Sig_TOKEN:
        stat = K2DBUS_Eat_AlignedTOKEN(&parse, NULL);
        break;
    case K2DBUS_Sig_INT32:
        stat = K2DBUS_Eat_AlignedINT32(&parse, NULL);
        break;
    case K2DBUS_Sig_INT16:
        stat = K2DBUS_Eat_AlignedINT16(&parse, NULL);
        break;
    case K2DBUS_Sig_OBJPATH:
        stat = K2DBUS_Eat_AlignedSTRZ(&parse, &pStr, NULL);
        if (!K2STAT_IS_ERROR(stat))
        {
            if (!K2DBUS_StringIsValidObjPath(pStr))
                stat = K2STAT_ERROR_BAD_FORMAT;
        }
        break;
    case K2DBUS_Sig_UINT16:
        stat = K2DBUS_Eat_AlignedUINT16(&parse, NULL);
        break;
    case K2DBUS_Sig_STRZ:
        stat = K2DBUS_Eat_AlignedSTRZ(&parse, NULL, NULL);
        break;
    case K2DBUS_Sig_UINT64:
        stat = K2DBUS_Eat_AlignedUINT64(&parse, NULL);
        break;
    case K2DBUS_Sig_UINT32:
        stat = K2DBUS_Eat_AlignedUINT32(&parse, NULL);
        break;
    case K2DBUS_Sig_INT64:
        stat = K2DBUS_Eat_AlignedINT64(&parse, NULL);
        break;
    case K2DBUS_Sig_BYTE:
        stat = K2DBUS_Eat_AlignedBYTE(&parse, NULL);
        break;
    default:
        K2_ASSERT(0);
        stat = K2STAT_ERROR_BAD_FORMAT;
        break;
    }
    if (K2STAT_IS_ERROR(stat))
        return stat;

    //
    // next thing can be anything but there can be only one of it
    //
    stat = K2DBUS_GetOneSimpleTypeSigLength(parse.mpSig, parse.mSigLen, &subLen, FALSE);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    // must be a closing brace after the second simple type
    if (0 == (parse.mMemLeft - subLen))
        return K2STAT_ERROR_BAD_FORMAT;

    if ('}' != parse.mpSig[subLen])
        return K2STAT_ERROR_BAD_FORMAT;

    //
    // ok try to eat the second thing now
    //
    stat = K2DBUS_ValidateInternal(&parse, FALSE, TRUE, FALSE);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    // eat the closing brace
    parse.mpSig++;
    parse.mSigLen--;

    if (NULL != apRetElemSigLen)
        *apRetElemSigLen = subLen + 1;

    if (NULL != apRetElemLen)
        *apRetElemLen = parse.mMemAddr - apParse->mMemAddr;

    K2MEM_Copy(apParse, &parse, sizeof(parse));

    return K2STAT_NO_ERROR;
}

K2STAT 
K2DBUS_Eat_AlignedSTRUCT(
    K2DBUS_PARSE *  apParse,
    char const **   apRetStructSig,
    UINT32 *        apRetStructSigLen,
    UINT32 *        apRetStructDataLen
)
{
    K2DBUS_PARSE    parse;
    char const *    pSigStart;
    UINT32          memStart;
    K2STAT          stat;

    K2MEM_Copy(&parse, apParse, sizeof(K2DBUS_PARSE));

    //
    // sig points to opening bracket
    //
    parse.mpSig++;
    parse.mSigLen--;
    if (0 == parse.mSigLen)
        return K2STAT_ERROR_BAD_FORMAT;
    pSigStart = parse.mpSig;
    memStart = parse.mMemAddr;

    stat = K2DBUS_ValidateInternal(&parse, TRUE, FALSE, FALSE);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (NULL != apRetStructSig)
        *apRetStructSig = pSigStart;
    if (NULL != apRetStructSigLen)
        *apRetStructSigLen = (UINT32)(parse.mpSig - (pSigStart + 1));
    if (NULL != apRetStructSigLen)
        *apRetStructSigLen = parse.mMemAddr - memStart;

    K2MEM_Copy(apParse, &parse, sizeof(parse));

    return K2STAT_NO_ERROR;
}

K2STAT 
K2DBUS_Eat_VARIANT(
    K2DBUS_PARSE *  apParse,
    char const **   apRetSig,
    UINT32 *        apRetSigLen,
    UINT8 const **  apRetVarData,
    UINT32 *        apRetVarLen
)
{
    char const * const sigSig = "g";

    K2STAT          stat;
    K2DBUS_PARSE    parse;
    char const *    pVarSig;
    UINT32          varSigLen;
    UINT32          varDataStart;

    //
    // eat signature
    //
    parse.mpSig = sigSig;
    parse.mSigLen = 1;
    parse.mMemAddr = apParse->mMemAddr;
    parse.mMemLeft = apParse->mMemLeft;

    stat = K2DBUS_Eat_SIG(&parse, &pVarSig, &varSigLen);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    parse.mpSig = pVarSig;
    parse.mSigLen = varSigLen;

    if (NULL != apRetSig)
        *apRetSig = pVarSig;
    if (NULL != apRetSigLen)
        *apRetSigLen = varSigLen;
    if (NULL != apRetVarData)
        *apRetVarData = (UINT8 const *)parse.mMemAddr;

    varDataStart = parse.mMemAddr;
    stat = K2DBUS_ValidateInternal(&parse, FALSE, FALSE, FALSE);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (NULL != apRetVarLen)
        *apRetVarLen = (parse.mMemAddr - varDataStart);

    // eat the v
    apParse->mpSig++;
    apParse->mSigLen--;

    apParse->mMemAddr = parse.mMemAddr;
    apParse->mMemLeft = parse.mMemLeft;

    return K2STAT_NO_ERROR;
}

K2STAT 
K2DBUS_ValidateInternal(
    K2DBUS_PARSE *  apParse,
    BOOL            aInStruct,
    BOOL            aDoSingle,
    BOOL            aPartOfArray
)
{
    char            c;
    K2DBUS_SigType  sigType;
    K2STAT          stat;
    K2DBUS_PARSE    parse;
    char const *    pStr;

    K2MEM_Copy(&parse, apParse, sizeof(parse));

    K2_ASSERT(NULL != parse.mpSig);
    do {
        if (0 == parse.mSigLen)
            break;
        c = *parse.mpSig;

        // parse one single complete type

        if ('(' == c)
        {
            if (!K2DBUS_AlignUp(&parse.mMemAddr, &parse.mMemLeft, gK2DBUS_Types[K2DBUS_Sig_STRUCT].mAlign))
                stat = K2STAT_ERROR_BAD_FORMAT;
            else
                stat = K2DBUS_Eat_AlignedSTRUCT(&parse, NULL, NULL, NULL);
        }
        else if ('{' == c)
        {
            if (!aPartOfArray)
                return K2STAT_ERROR_BAD_FORMAT;

            if (!K2DBUS_AlignUp(&parse.mMemAddr, &parse.mMemLeft, gK2DBUS_Types[K2DBUS_Sig_DICTENT].mAlign))
                stat = K2STAT_ERROR_BAD_FORMAT;
            else
                stat = K2DBUS_Eat_AlignedDICTENT(&parse, NULL, NULL, NULL);
        }
        else if ((')' == c) && (aInStruct))
        {
            parse.mpSig++;
            parse.mSigLen--;
            break;
        }
        else
        {
            sigType = K2DBUS_FindSigType(c);

            if ((K2DBUS_SigType_Invalid == sigType) ||
                (K2DBUS_Sig_STRUCT == sigType) ||
                (K2DBUS_Sig_DICTENT == sigType))
            {
                stat = K2STAT_ERROR_BAD_FORMAT;
            }
            else
            {
                if (!K2DBUS_AlignUp(&parse.mMemAddr, &parse.mMemLeft, gK2DBUS_Types[sigType].mAlign))
                    stat = K2STAT_ERROR_BAD_FORMAT;
                else
                {
                    switch (sigType)
                    {
                    case K2DBUS_Sig_ARRAY:
                        stat = K2DBUS_Eat_AlignedARRAY(&parse, NULL, NULL, NULL, NULL, NULL);
                        break;
                    case K2DBUS_Sig_BOOLEAN:
                        stat = K2DBUS_Eat_AlignedBOOLEAN(&parse, NULL);
                        break;
                    case K2DBUS_Sig_DOUBLE:
                        stat = K2DBUS_Eat_AlignedDOUBLE(&parse, NULL);
                        break;
                    case K2DBUS_Sig_FLOAT:
                        stat = K2DBUS_Eat_AlignedFLOAT(&parse,NULL);
                        break;
                    case K2DBUS_Sig_SIG:
                        stat = K2DBUS_Eat_SIG(&parse,NULL, NULL);
                        break;
                    case K2DBUS_Sig_TOKEN:
                        stat = K2DBUS_Eat_AlignedTOKEN(&parse,NULL);
                        break;
                    case K2DBUS_Sig_INT32:
                        stat = K2DBUS_Eat_AlignedINT32(&parse,NULL);
                        break;
                    case K2DBUS_Sig_INT16:
                        stat = K2DBUS_Eat_AlignedINT16(&parse,NULL);
                        break;
                    case K2DBUS_Sig_OBJPATH:
                        stat = K2DBUS_Eat_AlignedSTRZ(&parse, &pStr, NULL);
                        if (!K2STAT_IS_ERROR(stat))
                        {
                            if (!K2DBUS_StringIsValidObjPath(pStr))
                                stat = K2STAT_ERROR_BAD_FORMAT;
                        }
                        break;
                    case K2DBUS_Sig_UINT16:
                        stat = K2DBUS_Eat_AlignedUINT16(&parse,NULL);
                        break;
                    case K2DBUS_Sig_STRZ:
                        stat = K2DBUS_Eat_AlignedSTRZ(&parse,NULL, NULL);
                        break;
                    case K2DBUS_Sig_UINT64:
                        stat = K2DBUS_Eat_AlignedUINT64(&parse,NULL);
                        break;
                    case K2DBUS_Sig_UINT32:
                        stat = K2DBUS_Eat_AlignedUINT32(&parse,NULL);
                        break;
                    case K2DBUS_Sig_VARIANT:
                        stat = K2DBUS_Eat_VARIANT(&parse,NULL, NULL, NULL, NULL);
                        break;
                    case K2DBUS_Sig_INT64:
                        stat = K2DBUS_Eat_AlignedINT64(&parse,NULL);
                        break;
                    case K2DBUS_Sig_BYTE:
                        stat = K2DBUS_Eat_AlignedBYTE(&parse,NULL);
                        break;
                    default:
                        K2_ASSERT(0);
                        stat = K2STAT_ERROR_BAD_FORMAT;
                        break;
                    }
                }
            }
        }
        if (K2STAT_IS_ERROR(stat))
            return stat;
    } while (!aDoSingle);

    K2MEM_Copy(apParse, &parse, sizeof(parse));

    return K2STAT_NO_ERROR;
}

K2STAT 
K2DBUS_Validate(
    K2DBUS_PARSE *apParse
)
{
    return K2DBUS_ValidateInternal(apParse, FALSE, FALSE, FALSE);
}

K2STAT
K2DBUS_EatHeader(
    UINT8 const **      apStream,
    UINT32 *            apBytesLen,
    K2DBUS_PARSED_HDR * apRetParseHdr
)
{
    UINT32                  startAddr;
    K2STAT                  stat;
    K2DBUS_PARSE            parse;
    K2DBUS_MSGHDR const *   pHdr;
    UINT32                  arrayLeft;
    K2DBUS_MsgHdrFieldType  hdrFieldType;
    char const *            pVarSig;
    UINT32                  varSigLen;
    UINT32                  varLen;
    UINT8 const *           pVarData;
    K2DBUS_PARSE            varParse;

    if (NULL == apRetParseHdr)
        return K2STAT_ERROR_BAD_ARGUMENT;

    K2MEM_Zero(apRetParseHdr, sizeof(K2DBUS_PARSED_HDR));

    if ((NULL == apStream) || (NULL == apBytesLen))
        return K2STAT_ERROR_BAD_ARGUMENT;

    parse.mMemAddr = startAddr = (UINT32)(*apStream);
    parse.mMemLeft = (*apBytesLen);

    if (!K2DBUS_ALIGNED(startAddr, K2DBUS_STRUCT_ALIGN))
        return K2STAT_ERROR_BAD_ALIGNMENT;

    if (parse.mMemLeft < sizeof(K2DBUS_MSGHDR))
        return K2STAT_ERROR_TOO_SMALL;

    if (K2DBUS_MSG_LITTLE_ENDIAN != (*(UINT8 const *)startAddr))
    {
        if (K2DBUS_MSG_BIG_ENDIAN != (*(UINT8 const *)startAddr))
            return K2STAT_ERROR_BAD_FORMAT;
        return K2STAT_ERROR_NOT_SUPPORTED;
    }

    pHdr = apRetParseHdr->mpRaw = (K2DBUS_MSGHDR const *)startAddr;

    if ((K2DBUS_MsgType_Invalid == pHdr->mMsgType) ||
        (K2DBUS_MsgType_Count <= pHdr->mMsgType))
        return K2STAT_ERROR_BAD_FORMAT;

    if (K2DBUS_PROTO_VERSION != pHdr->mProtoVer)
        return K2STAT_ERROR_NOT_SUPPORTED;

    // eat the fixed part of the header
    parse.mMemAddr += sizeof(K2DBUS_MSGHDR);
    parse.mMemLeft -= sizeof(K2DBUS_MSGHDR);

    // rest of header is an array of (yv) structs

    if (sizeof(UINT32) > parse.mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    arrayLeft = *(UINT32 const *)parse.mMemAddr;
    parse.mMemAddr += sizeof(UINT32);

    if (arrayLeft > parse.mMemLeft)
        return K2STAT_ERROR_BAD_FORMAT;

    parse.mMemLeft = arrayLeft;

    do {
        parse.mpSig = "yv";
        parse.mSigLen = 2;

        hdrFieldType = 0;

        stat = K2DBUS_AlignUp(&parse.mMemAddr, &parse.mMemLeft, K2DBUS_STRUCT_ALIGN);
        if (K2STAT_IS_ERROR(stat))
            return stat;

        stat = K2DBUS_Eat_AlignedBYTE(&parse, (UINT8 *)&hdrFieldType);
        if (K2STAT_IS_ERROR(stat))
            return stat;

        stat = K2DBUS_Eat_VARIANT(&parse, &pVarSig, &varSigLen, &pVarData, &varLen);
        if (K2STAT_IS_ERROR(stat))
            return stat;

        // all header types are basic types - object path, string, unsigned int32
        if (1 != varSigLen)
            return K2STAT_ERROR_BAD_FORMAT;

        varParse.mpSig = pVarSig;
        varParse.mSigLen = varSigLen;
        varParse.mMemAddr = (UINT32)pVarData;
        varParse.mMemLeft = varLen;

        switch (hdrFieldType)
        {
        case K2DBUS_MsgHdrField_PATH:            // K2DBUS_Sig_OBJPATH   is conn + object;  target for METHOD_CALL, source for SIGNAL
            if (gK2DBUS_Types[K2DBUS_Sig_OBJPATH].mChar != (*pVarSig))
                return K2STAT_ERROR_BAD_FORMAT;
            stat = K2DBUS_AlignUp(&varParse.mMemAddr, &varParse.mMemLeft, gK2DBUS_Types[K2DBUS_Sig_OBJPATH].mAlign);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            stat = K2DBUS_Eat_AlignedSTRZ(&varParse, &apRetParseHdr->mpObjPath, &apRetParseHdr->mObjPathLen);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            if (!K2DBUS_StringIsValidObjPath(apRetParseHdr->mpObjPath))
                return K2STAT_ERROR_BAD_FORMAT;
            break;

        case K2DBUS_MsgHdrField_INTERFACE:
            if (gK2DBUS_Types[K2DBUS_Sig_STRZ].mChar != (*pVarSig))
                return K2STAT_ERROR_BAD_FORMAT;
            stat = K2DBUS_AlignUp(&varParse.mMemAddr, &varParse.mMemLeft, gK2DBUS_Types[K2DBUS_Sig_STRZ].mAlign);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            stat = K2DBUS_Eat_AlignedSTRZ(&varParse, &apRetParseHdr->mpInterface, &apRetParseHdr->mInterfaceLen);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            if (!K2DBUS_StringIsValidInterface(apRetParseHdr->mpInterface))
                return K2STAT_ERROR_BAD_FORMAT;
            break;

        case K2DBUS_MsgHdrField_MEMBER:
            if (gK2DBUS_Types[K2DBUS_Sig_STRZ].mChar != (*pVarSig))
                return K2STAT_ERROR_BAD_FORMAT;
            stat = K2DBUS_AlignUp(&varParse.mMemAddr, &varParse.mMemLeft, gK2DBUS_Types[K2DBUS_Sig_STRZ].mAlign);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            stat = K2DBUS_Eat_AlignedSTRZ(&varParse, &apRetParseHdr->mpMember, &apRetParseHdr->mMemberLen);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            if (!K2DBUS_StringIsValidMemberName(apRetParseHdr->mpMember))
                return K2STAT_ERROR_BAD_FORMAT;
            break;

        case K2DBUS_MsgHdrField_ERROR_NAME:
            if (gK2DBUS_Types[K2DBUS_Sig_STRZ].mChar != (*pVarSig))
                return K2STAT_ERROR_BAD_FORMAT;
            stat = K2DBUS_AlignUp(&varParse.mMemAddr, &varParse.mMemLeft, gK2DBUS_Types[K2DBUS_Sig_STRZ].mAlign);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            stat = K2DBUS_Eat_AlignedSTRZ(&varParse, &apRetParseHdr->mpErrorName, &apRetParseHdr->mErrorNameLen);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            if (!K2DBUS_StringIsValidErrorName(apRetParseHdr->mpErrorName))
                return K2STAT_ERROR_BAD_FORMAT;
            break;

        case K2DBUS_MsgHdrField_DESTINATION:
            if (gK2DBUS_Types[K2DBUS_Sig_STRZ].mChar != (*pVarSig))
                return K2STAT_ERROR_BAD_FORMAT;
            stat = K2DBUS_AlignUp(&varParse.mMemAddr, &varParse.mMemLeft, gK2DBUS_Types[K2DBUS_Sig_STRZ].mAlign);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            stat = K2DBUS_Eat_AlignedSTRZ(&varParse, &apRetParseHdr->mpDestination, &apRetParseHdr->mDestinationLen);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            if (!K2DBUS_StringIsValidBusName(apRetParseHdr->mpDestination))
                return K2STAT_ERROR_BAD_FORMAT;
            break;

        case K2DBUS_MsgHdrField_SENDER:
            if (gK2DBUS_Types[K2DBUS_Sig_STRZ].mChar != (*pVarSig))
                return K2STAT_ERROR_BAD_FORMAT;
            stat = K2DBUS_AlignUp(&varParse.mMemAddr, &varParse.mMemLeft, gK2DBUS_Types[K2DBUS_Sig_STRZ].mAlign);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            stat = K2DBUS_Eat_AlignedSTRZ(&varParse, &apRetParseHdr->mpSender, &apRetParseHdr->mSenderLen);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            if (!K2DBUS_StringIsValidBusName(apRetParseHdr->mpSender))
                return K2STAT_ERROR_BAD_FORMAT;
            break;

        case K2DBUS_MsgHdrField_SIGNATURE:
            if (gK2DBUS_Types[K2DBUS_Sig_SIG].mChar != (*pVarSig))
                return K2STAT_ERROR_BAD_FORMAT;
            stat = K2DBUS_Eat_SIG(&varParse, &apRetParseHdr->mpSignature, &apRetParseHdr->mSignatureLen);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            break;

        case K2DBUS_MsgHdrField_REPLY_SERIAL:
            if (gK2DBUS_Types[K2DBUS_Sig_UINT32].mChar != (*pVarSig))
                return K2STAT_ERROR_BAD_FORMAT;
            stat = K2DBUS_AlignUp(&varParse.mMemAddr, &varParse.mMemLeft, gK2DBUS_Types[K2DBUS_Sig_UINT32].mAlign);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            stat = K2DBUS_Eat_AlignedUINT32(&varParse, &apRetParseHdr->mReplySerial);
            if (K2STAT_IS_ERROR(stat))
                return stat;
            apRetParseHdr->mHasReplySerial = TRUE;
            break;

        default:
            // unknown header type.  bypass the content of the variant
            return K2STAT_ERROR_NOT_SUPPORTED;
        }

    } while (0 != parse.mMemLeft);

    *apStream = (UINT8 const *)parse.mMemAddr;
    *apBytesLen -= (parse.mMemAddr - startAddr);

    return K2STAT_NO_ERROR;
}

