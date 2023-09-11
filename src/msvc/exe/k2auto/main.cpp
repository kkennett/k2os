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
#include "k2auto.h"

char const *        gpArchName[5] = { "none", "x32", "a32", "x64", "a64" };
UINT_PTR            gArch;
bool                gDebugMode;
static char         sgArchModeBuf[12];
char const *        gpArchMode = sgArchModeBuf;
UINT_PTR const      gArchModeLen = 9;
//char const * const  gpOsVer = "os8";
char const * const  gpOsVer = "os9";

char const * const gpBuildXmlFileName = "k2build.xml";

char *          gpStr_OptC;
char *          gpStr_OptCPP;
char *          gpStr_OptAsm;
char *          gpStr_OsInc;
char *          gpStr_LibGcc;
char *          gpStr_LdOpt;

Environ *       gpBaseEnviron;

K2LIST_ANCHOR   gSourceFileList;

char            gStrBuf[STRBUF_CHARS];

void 
Spaces(
    UINT_PTR aCount
)
{
    if (0 == aCount)
        return;
    do {
        printf(" ");
    } while (0 != --aCount);
}

static char const * const sgpOptBase = "-c -nostdinc -fno-common -Wall -I %s\\src\\shared\\inc -DK2_OS=%s -fdata-sections --function-sections";

void SetupStr(void)
{
    static char const *spArchA32 = "-march=armv7-a -mapcs-frame";
    static char const *spArchX32 = "-march=i686 -m32 -malign-double";
    static char const *spArchA64 = "-march=armv8-a";
    static char const *spArchX64 = "-march=x86-64 -m64 -malign-double";
    static char const *spDebug = "-ggdb -g2";
    static char const *spCpp = "-fno-rtti -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables";
    static char const *spOsInc = "-I %s\\src\\%s\\inc";
    static char const *spBinPath = "%s\\k2tools\\%s\\libgcc.a";
    static char const *spLinkOpt = "-q -nostdlib -static --no-undefined --script %s\\src\\build\\gcc_link32.l";

    UINT_PTR    len;
    UINT_PTR    addLen;
    char *      pTemp;

    gpStr_OptC = new char[(K2ASC_Len(sgpOptBase) - 2 + gVfsRootSpecLen + K2ASC_Len(gpOsVer) + 4) & ~3];
    K2_ASSERT(NULL != gpStr_OptC);
    K2ASC_Printf(gpStr_OptC, sgpOptBase, gpVfsRootSpec, gpOsVer);
    len = K2ASC_Len(gpStr_OptC);

    switch (gArch)
    {
    case K2_ARCH_X32:
        K2ASC_Copy(sgArchModeBuf, gpArchName[K2_ARCH_X32]);
        addLen = 1 + K2ASC_Len(spArchX32);
        pTemp = new char[(len + addLen + 4) & ~3];
        K2_ASSERT(NULL != pTemp);
        K2ASC_Printf(pTemp, "%s %s", gpStr_OptC, spArchX32);
        delete[] gpStr_OptC;
        gpStr_OptC = pTemp;
        len += addLen;
        break;
    case K2_ARCH_A32:
        K2ASC_Copy(sgArchModeBuf, gpArchName[K2_ARCH_A32]);
        addLen = 1 + K2ASC_Len(spArchA32);
        pTemp = new char[(len + addLen + 4) & ~3];
        K2_ASSERT(NULL != pTemp);
        K2ASC_Printf(pTemp, "%s %s", gpStr_OptC, spArchA32);
        delete[] gpStr_OptC;
        gpStr_OptC = pTemp;
        len += addLen;
        break;
    case K2_ARCH_X64:
        K2ASC_Copy(sgArchModeBuf, gpArchName[K2_ARCH_X64]);
        addLen = 1 + K2ASC_Len(spArchX64);
        pTemp = new char[(len + addLen + 4) & ~3];
        K2_ASSERT(NULL != pTemp);
        K2ASC_Printf(pTemp, "%s %s", gpStr_OptC, spArchX64);
        delete[] gpStr_OptC;
        gpStr_OptC = pTemp;
        len += addLen;
        break;
    case K2_ARCH_A64:
        K2ASC_Copy(sgArchModeBuf, gpArchName[K2_ARCH_A64]);
        addLen = 1 + K2ASC_Len(spArchA64);
        pTemp = new char[(len + addLen + 4) & ~3];
        K2_ASSERT(NULL != pTemp);
        K2ASC_Printf(pTemp, "%s %s", gpStr_OptC, spArchA64);
        delete[] gpStr_OptC;
        gpStr_OptC = pTemp;
        len += addLen;
        break;
    default:
        ExitProcess((UINT)-1);
    }

    if (gDebugMode)
    {
        K2ASC_Copy(&sgArchModeBuf[3], "\\debug");
        addLen = 1 + K2ASC_Len(spDebug);
        pTemp = new char[(len + addLen + 4) & ~3];
        K2_ASSERT(NULL != pTemp);
        K2ASC_Printf(pTemp, "%s %s", gpStr_OptC, spDebug);
        delete[] gpStr_OptC;
        gpStr_OptC = pTemp;
        len += addLen;
    }
    else
    {
        K2ASC_Copy(&sgArchModeBuf[3], "\\final");
        // LDOPT: --strip-debug
    }

    gpStr_OptCPP = new char[(len + 1 + K2ASC_Len(spCpp) + 4) & ~3];
    K2_ASSERT(NULL != gpStr_OptCPP);
    K2ASC_Printf(gpStr_OptCPP, "%s %s", gpStr_OptC, spCpp);

    len = K2ASC_Len(spOsInc);
    len += gVfsRootSpecLen;
    len += K2ASC_Len(gpOsVer);
    gpStr_OsInc = new char[(len + 4) & ~3];
    K2_ASSERT(NULL != gpStr_OsInc);
    K2ASC_Printf(gpStr_OsInc, spOsInc, gpVfsRootSpec, gpOsVer);

    len = K2ASC_Len(spBinPath);
    len += gVfsRootSpecLen;
    len += 3;
    gpStr_LibGcc = new char[(len + 4) & ~3];
    K2_ASSERT(NULL != gpStr_LibGcc);
    K2ASC_Printf(gpStr_LibGcc, spBinPath, gpVfsRootSpec, gpArchName[gArch]);


    len = K2ASC_Len(spLinkOpt);
    len += gVfsRootSpecLen;
    gpStr_LdOpt = new char[(len + 4) & ~3];
    K2_ASSERT(NULL != gpStr_LdOpt);
    K2ASC_Printf(gpStr_LdOpt, spLinkOpt, gpVfsRootSpec);
}

void
SetupBaseEnv(
    void
)
{
    char const * const spExtendPath = "%s;%s\\k2tools;%s\\k2tools\\%s";
    static Environ env(true);

    UINT_PTR    srcIx;
    UINT_PTR    len;
    char *      pSet;

    srcIx = env.Find("path");
    if (((UINT_PTR)-1) == srcIx)
    {
        ExitProcess(-2);
        return;
    }

    len = env.Entry(srcIx).mValueLen;
    len += K2ASC_Len(spExtendPath);
    len += 2 * gVfsRootSpecLen;
    len += 3;
    pSet = new char[(len + 4) & ~3];
    K2_ASSERT(NULL != pSet);

    K2ASC_Printf(pSet, spExtendPath, env.Entry(srcIx).mpValue, gpVfsRootSpec, gpVfsRootSpec, gpArchName[gArch]);

    if (!SetEnvironmentVariable("path", pSet))
    {
        ExitProcess(-3);
        return;
    }

    env.Set("path", pSet);

    delete[] pSet;
 
    gpBaseEnviron = &env;
}

bool
CreatePath(
    char *      apPathZ,
    UINT_PTR    aPathLen
)
{
    DWORD   fAttr;
    char *  pEnd;
    
    K2_ASSERT(0 != aPathLen);
    pEnd = apPathZ + aPathLen;
    K2_ASSERT(0 == *pEnd);
    do {
        --pEnd;
        if (*pEnd == '\\')
        {
            *pEnd = 0;
            fAttr = GetFileAttributes(apPathZ);
            if (fAttr == INVALID_FILE_ATTRIBUTES)
            {
                bool ok = CreatePath(apPathZ, ((UINT_PTR)(pEnd - apPathZ)));
                *pEnd = '\\';
                if (!ok)
                    return false;
            }
            else if (0 == (fAttr & FILE_ATTRIBUTE_DIRECTORY))
            {
                printf("*** [%s] exists but is not a directory!\n", apPathZ);
                *pEnd = '\\';
                return false;
            }
            else
            {
                *pEnd = '\\';
            }
            return (FALSE != CreateDirectory(apPathZ, NULL));
        }
    } while (pEnd != apPathZ);

    return false;
}

void
DeDoubleDot(
    char *apPath
)
{
    char *  pEnd;
    char *  pScan;
    char    ch;

    //
    // scan for .. path modifier
    //
    pScan = apPath;
    do {
        ch = *pScan;
        if (ch == 0)
            break;
        if (ch == '.')
        {
            pScan++;
            ch = *pScan;
            if (0 == ch)
                break;
            if (ch == '.')
            {
                pScan++;
                ch = *pScan;
                if (0 == ch)
                    break;
                if ((ch == '/') ||
                    (ch == '\\'))
                {
                    //
                    // double period - need to go back a directory
                    //
                    pEnd = pScan;
                    pScan -= 3;
                    K2_ASSERT(pScan != apPath);
                    do {
                        --pScan;
                        ch = *pScan;
                        if ((ch == '/') ||
                            (ch == '\\'))
                        {
                            break;
                        }
                    } while (pScan != apPath);
                    K2ASC_Copy(pScan, pEnd);
                }
            }
        }
        pScan++;
    } while (1);
}

UINT_PTR    
GetXmlNodeArchFlags(
    K2XML_NODE const *apXmlNode
)
{
    UINT_PTR                archFlags;
    UINT_PTR                ixAttribArch;
    K2XML_ATTRIB const *    pAttrib;
    char const *            pPars;
    UINT_PTR                left;

    if (!K2XML_FindAttrib(apXmlNode, "arch", &ixAttribArch))
        return ALL_ARCH_FLAGS;

    pAttrib = apXmlNode->mppAttrib[ixAttribArch];
    pPars = pAttrib->Value.mpChars;
    left = pAttrib->Value.mLen;

    archFlags = 0;

    K2PARSE_EatWhitespace(&pPars, &left);
    K2PARSE_EatWhitespaceAtEnd(pPars, &left);

    char const *pToken;
    UINT_PTR tokLen;
    do {
        K2PARSE_Token(&pPars, &left, &pToken, &tokLen);
        if ((0 == tokLen) || (tokLen != 3))
            break;

        if (0 == K2ASC_CompInsLen(pToken, "x32", 3))
        {
            archFlags |= (1 << K2_ARCH_X32);
        }
        else if (0 == K2ASC_CompInsLen(pToken, "a32", 3))
        {
            archFlags |= (1 << K2_ARCH_A32);
        }
        else if (0 == K2ASC_CompInsLen(pToken, "x64", 3))
        {
            archFlags |= (1 << K2_ARCH_X64);
        }
        else if (0 == K2ASC_CompInsLen(pToken, "a64", 3))
        {
            archFlags |= (1 << K2_ARCH_A64);
        }
        else
        {
            break;
        }

        K2PARSE_EatWhitespace(&pPars, &left);
        if (0 == left)
            break;
        if ((*pPars != ',') && (*pPars != ';'))
            break;
        pPars++;
        left--;
        K2PARSE_EatWhitespace(&pPars, &left);

    } while (0 != left);

    return archFlags;
}

bool
GetContentTarget(
    VfsFolder *         apFolder,
    K2XML_NODE const *  apXmlNode,
    char **             appRetSrcPath
)
{
    UINT_PTR                ixAttribArch;
    char const *            pPars;
    UINT_PTR                left;
    char *                  pSrcPath;
    char *                  pEnd;
    char                    ch;

    pPars = apXmlNode->Content.mpChars;
    left = apXmlNode->Content.mLen;
    K2PARSE_EatWhitespace(&pPars, &left);
    K2PARSE_EatWhitespaceAtEnd(pPars, &left);
    if (0 == left)
        return false;

    if (*pPars == '@')
    {
        left = K2ASC_Printf(gStrBuf, "%s\\src\\%.*s", gpVfsRootSpec, left - 1, pPars + 1);
        pSrcPath = new char[(left + 4) & ~3];
        K2_ASSERT(NULL != pSrcPath);
        K2ASC_Copy(pSrcPath, gStrBuf);
        pEnd = pSrcPath + gVfsRootSpecLen + 1 + 4;
    }
    else if (*pPars == '~')
    {
        left = K2ASC_Printf(gStrBuf, "%s\\src\\%s\\%.*s", gpVfsRootSpec, gpOsVer, left - 1, pPars + 1);
        pSrcPath = new char[(left + 4) & ~3];
        K2_ASSERT(NULL != pSrcPath);
        K2ASC_Copy(pSrcPath, gStrBuf);
        pEnd = pSrcPath + gVfsRootSpecLen + 1 + 8;
    }
    else
    {
        pSrcPath = apFolder->AllocFullPath(9 + left); //+8 for other extensions than what is there
        pEnd = pSrcPath + apFolder->GetFullPathLen();
        *pEnd = '\\';
        pEnd++;
        K2ASC_CopyLen(pEnd, pPars, left);
        pEnd[left] = 0;
    }

    for (ixAttribArch = 0; ixAttribArch < left; ixAttribArch++)
    {
        ch = *pEnd;
        if (ch == '/')
            *pEnd = '\\';
        if (0 == ch)
            break;
        pEnd++;
    }
    if (0 != ch)
        *pEnd = 0;

    DeDoubleDot(pSrcPath + gVfsRootSpecLen + 1);

    //
    // must be under src dir
    //
    if (0 != K2ASC_CompInsLen(pSrcPath + gVfsRootSpecLen, "\\src\\", 5))
    {
        delete[] pSrcPath;
        return false;
    }

    *appRetSrcPath = pSrcPath;

    return true;
}

int main(int argc, char **argv)
{
//    gArch = K2_ARCH_A32;
    gArch = K2_ARCH_X32;
    gDebugMode = true;

    gpVfsRootSpec = "C:\\repo\\k2os";
    gVfsRootSpecLen = K2ASC_Len(gpVfsRootSpec);

    K2LIST_Init(&gSourceFileList);

    SetupBaseEnv();

    SetupStr();

    Vfs_Run(BuildFileUser_SrcXml::Eval, BuildFileUser_SrcXml::DiscoveredNew);

    return 0;
}

