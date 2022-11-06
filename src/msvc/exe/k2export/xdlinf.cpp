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
#include "k2export.h"

enum InfSection
{
    ExpNone=0,
    ExpXDL,
    ExpCode,
    ExpData
};

static InfSection sgInfSection;

#define _K2PARSE_Token(p1,pn1,p2,pn2)     K2PARSE_Token((const char **)(p1), pn1, (const char **)(p2), pn2)
#define _K2PARSE_EatWhitespace(p1,pn1)    K2PARSE_EatWhitespace((const char **)(p1),pn1)
#define _K2PARSE_EatLine(p1,pn1,p2,pn2)   K2PARSE_EatLine((const char **)(p1), pn1, (const char **)(p2), pn2)

static
void
sInsertList(
    UINT_PTR        aLineNum,
    EXPORT_SPEC **  appList,
    EXPORT_SPEC *   apIns
    )
{
    int             comp;
    EXPORT_SPEC *   pPrev;
    EXPORT_SPEC *   pLook;

    apIns->mpNext = NULL;
    pLook = *appList;
    if (pLook == NULL)
    {
        *appList = apIns;
        return;
    }

    pPrev = NULL;
    do
    {
        comp = K2ASC_Comp(apIns->mpName, pLook->mpName);
        if (comp == 0)
        {
            printf("!!! Duplicated export on line %Id\n", aLineNum);
            return;  // duplicated symbol in export list
        }

        if (comp < 0)
        {
            apIns->mpNext = pLook;
            if (pPrev == NULL)
                *appList = apIns;
            else
                pPrev->mpNext = apIns;
            return;
        }

        pPrev = pLook;
        pLook = pLook->mpNext;

    } while (pLook != NULL);

    pPrev->mpNext = apIns;
}

static
bool
sSectionXDL(
    UINT_PTR    aLineNumber,
    char *      apLine,
    UINT_PTR    aLineLen
    )
{
    char *      pToken;
    UINT_PTR    tokLen;

    _K2PARSE_Token(&apLine, &aLineLen, &pToken, &tokLen);
    if (tokLen == 0)
    {
        printf("*** Syntax error in xdl inf file on line %d\n", aLineNumber);
        return false;
    }
    if (tokLen == 2)
    {
        if (0 == K2ASC_CompInsLen("ID", pToken, 2))
        {
            _K2PARSE_EatWhitespace(&apLine, &aLineLen);
            if ((aLineLen < 38) || (*apLine != '{') || (apLine[aLineLen-1] != '}'))
            {
                printf("*** Expected GUID but found other on line %d of xdl inf\n", aLineNumber);
                return false;
            }
            apLine ++;
            aLineLen -= 2;
            if (!K2PARSE_Guid128(apLine, aLineLen, &gOut.Id))
            {
                printf("*** Failed to parse GUID on line %d of xdl inf\n", aLineNumber);
                return false;
            }
            return true;
        }
    }

    printf("*** Unknown token \"%.*s\" in [XDL] section of xdl inf on line %d\n", tokLen, pToken, aLineNumber);

    return false;
}

static
bool
sSectionExport(
    UINT_PTR    aLineNumber,
    char *      apLine,
    UINT_PTR    aLineLen,
    bool        aIsCode
    )
{
    char *          pToken;
    UINT_PTR        tokLen;
    EXPORT_SPEC *   pSpec;

    _K2PARSE_Token(&apLine, &aLineLen, &pToken, &tokLen);
    if (tokLen == 0)
    {
        printf("*** Syntax error in xdl inf file on line %d\n", aLineNumber);
        return false;
    }

    _K2PARSE_EatWhitespace(&apLine, &aLineLen);
    if (aLineLen != 0)
    {
        printf("*** Garbage after export on line %d\n", aLineNumber);
        return false;
    }

    pToken[tokLen] = 0;

    pSpec = new EXPORT_SPEC;
    if (pSpec == NULL)
    {
        printf("*** Memory allocation failed\n");
        return false;
    }
    K2MEM_Zero(pSpec, sizeof(EXPORT_SPEC));

    pSpec->mpName = pToken;
    pSpec->mpNext = NULL;

    if (aIsCode)
    {
        sInsertList(aLineNumber, &gOut.mOutSeg[XDLProgData_Text].mpSpecList, pSpec);
        gOut.mOutSeg[XDLProgData_Text].mCount++;
    }
    else
    {
        sInsertList(aLineNumber, &gOut.mOutSeg[XDLProgData_Data].mpSpecList, pSpec);
        gOut.mOutSeg[XDLProgData_Data].mCount++;
    }

    gOut.mTotalExports++;

    return true;
}

static
bool
sParseXdlInfLine(
    UINT_PTR    aLineNumber,
    char *      apLine,
    UINT_PTR    aLineLen
    )
{
    char *      pToken;
    UINT_PTR    tokLen;

    if (apLine[0] == '[')
    {
        apLine++;
        aLineLen--;
        _K2PARSE_Token(&apLine, &aLineLen, &pToken, &tokLen);
        if (tokLen == 0)
        {
            printf("*** Cannot parse section type in xdl inf file at line %d\n", aLineNumber);
            return false;
        }
        _K2PARSE_EatWhitespace(&apLine, &aLineLen);
        if ((aLineLen == 0) || (*apLine != ']'))
        {
            printf("*** Unterminated section type in xdl inf file at line %d\n", aLineNumber);
            return false;
        }
        apLine++;
        aLineLen--;
        _K2PARSE_EatWhitespace(&apLine, &aLineLen);
        if (aLineLen != 0)
        {
            printf("*** Export def file has garbage at end of line %d\n", aLineNumber);
            return false;
        }
        if ((tokLen == 3) && (0 == K2ASC_CompInsLen(pToken, "XDL", 3)))
        {
            sgInfSection = ExpXDL;
            return true;
        }
        if (tokLen == 4)
        {
            if (0 == K2ASC_CompInsLen(pToken, "CODE", 4))
            {
                sgInfSection = ExpCode;
                return true;
            }
            else if (0 == K2ASC_CompInsLen(pToken, "DATA", 4))
            {
                sgInfSection = ExpData;
                return true;
            }
        }
        printf("*** Unknown inf section type \"%.*s\" at line %d\n", tokLen, pToken, aLineNumber);
        return false;
    }

    if (sgInfSection == ExpNone)
    {
        printf("*** No inf section at start of xdl inf file\n");
        return false;
    }

    switch (sgInfSection)
    {
    case ExpXDL:
        return sSectionXDL(aLineNumber, apLine, aLineLen);
    case ExpCode:
        return sSectionExport(aLineNumber, apLine, aLineLen, true);
    case ExpData:
        return sSectionExport(aLineNumber, apLine, aLineLen, false);
    default:
        break;
    }
    return false;
}

static
K2STAT
sParseXdlInf(
    void
    )
{
    char *      pPars;
    UINT_PTR    left;
    char *      pLine;
    UINT_PTR    lineLen;
    UINT_PTR    lineNumber;

    pPars = (char *)gOut.mpMappedXdlInf->DataPtr();
    left = (UINT_PTR)gOut.mpMappedXdlInf->FileBytes();

    if (left == 0)
        return K2STAT_NO_ERROR;

    sgInfSection = ExpNone;
    lineNumber = 1;
    do 
    {
        _K2PARSE_EatLine(&pPars, &left, &pLine, &lineLen);
        _K2PARSE_EatWhitespace(&pLine, &lineLen);
        K2PARSE_EatWhitespaceAtEnd(pLine, &lineLen);
        if ((lineLen != 0) && (*pLine != '#'))
        {
            if (!sParseXdlInfLine(lineNumber, pLine, lineLen))
                return K2STAT_ERROR_BAD_FORMAT;
        }
        lineNumber++;
    } while (left != 0);

//    printf("%d Code Exports\n", gOut.mOutSeg[XDLProgData_Text].mCount);
//    printf("%d Data Exports\n", gOut.mOutSeg[XDLProgData_Data].mCount);

    return K2STAT_NO_ERROR;
}

K2STAT
LoadXdlInfFile(
    char const *apArgument
    )
{
    DWORD   outPathLen;
    char *  pXdlInfFilePath;
    
    outPathLen = GetFullPathName(apArgument, 0, NULL, NULL);
    if (outPathLen == 0)
    {
        printf("*** Could not parse xdl inf file argument \"%s\"\n", apArgument);
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    pXdlInfFilePath = new char[(outPathLen + 4)&~3];

    if (pXdlInfFilePath == NULL)
    {
        printf("*** Memory allocation failed.\n");
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    if (0 == GetFullPathName(apArgument, outPathLen + 1, pXdlInfFilePath, NULL))
    {
        // weird
        delete[] pXdlInfFilePath;
        printf("*** Error creating xdl inf file path.\n");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    gOut.mpMappedXdlInf = K2ReadWriteMappedFile::Create(pXdlInfFilePath);
    if (gOut.mpMappedXdlInf == NULL)
    {
        printf("*** Error mapping in xdl inf file \"%s\"\n", pXdlInfFilePath);
        delete[] pXdlInfFilePath;
        return K2STAT_ERROR_UNKNOWN;
    }

    delete[] pXdlInfFilePath;

    return sParseXdlInf();
}
