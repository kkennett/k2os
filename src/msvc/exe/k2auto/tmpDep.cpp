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
#include "k2auto.h"

BuildFileUser_TmpDep *
BuildFileUser_TmpDep::Construct(
    char const *            apSrcCodePath,
    VfsFile *               apSrcCodeVfsFile,
    BuildFileUser_SrcXml *  apSrcXml,
    K2XML_NODE const *      apSrcXmlNode,
    char const *            apXmlFullPath
)
{
    BuildFileUser_TmpDep *  pResult;
    BuildFileUser_SrcCode * pSrcCode;
    char *                  pEnd;
    char                    ch;
    VfsFile *               pTmpDepVfsFile;

    pSrcCode = new BuildFileUser_SrcCode(apSrcCodeVfsFile, apSrcXmlNode);
    K2_ASSERT(NULL != pSrcCode);

    pEnd = gStrBuf + K2ASC_Printf(gStrBuf, "bld\\tmp\\gcc\\obj\\%s%s",
        gpArchMode,
        apSrcCodePath + gVfsRootSpecLen);
    do {
        --pEnd;
        ch = *pEnd;
        if ((ch == '.') || (ch == '\\'))
            break;
    } while (pEnd != gStrBuf);
    if (ch != '.')
    {
        printf("*** XML specifies <source> file \"%s\" with no extension [%s]\n", gStrBuf, apXmlFullPath);
        delete pSrcCode;
        return NULL;
    }
    pEnd++;
    *pEnd = 'd';
    pEnd++;
    *pEnd = 0;

    pTmpDepVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(gStrBuf, false);
    if (NULL == pTmpDepVfsFile)
    {
        printf("*** Failed to acquire a tmp dep file vfs node [%s]\n", gStrBuf);
        delete pSrcCode;
        return NULL;
    }

    pResult = new BuildFileUser_TmpDep(pTmpDepVfsFile, pSrcCode);

    pTmpDepVfsFile->Release();

    return pResult;
}


BuildFileUser_TmpDep::BuildFileUser_TmpDep(
    VfsFile *               apVfsFile, 
    BuildFileUser_SrcCode * apSrcCode
) :
    BuildFileUser(apVfsFile, BuildFileUserType_TmpDep),
    mpChildSrcCode(apSrcCode)
{
    mpChildSrcCode->mpParentTmpDep = this;
    mpParentTmpObj = NULL;
    mDepParseProblem = true;
    mInDepBuild = false;
    K2LIST_Init(&GeneratedSrcIncDepList);
}

BuildFileUser_TmpDep::~BuildFileUser_TmpDep(
    void
)
{
    K2LIST_LINK *           pListLink;
    BuildFileUser_SrcInc *  pSrcInc;

    delete mpChildSrcCode;
    mpParentTmpObj = NULL;

    pListLink = GeneratedSrcIncDepList.mpHead;
    if (NULL == pListLink)
        return;
    do {
        pSrcInc = K2_GET_CONTAINER(BuildFileUser_SrcInc, pListLink, ParentIncDepListLink);
        pListLink = pListLink->mpNext;
        delete pSrcInc;
    } while (NULL != pListLink);
}

void
BuildFileUser_TmpDep::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    BuildGeneratedDeps(apFullPath);
    if (!CheckIfDamaged())
    {
        OnRepaired();
    }
    else
    {
        SetSomethingChangedSinceLastTryRepair();
    }
}

void
BuildFileUser_TmpDep::OnFsUpdate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsUpdate(apFullPath);
    TearDownGeneratedDeps(apFullPath);
    BuildGeneratedDeps(apFullPath);
    if (!CheckIfDamaged())
    {
        OnRepaired();
    }
    else
    {
        SetSomethingChangedSinceLastTryRepair();
    }
}

void
BuildFileUser_TmpDep::OnFsDelete(
    char const *apFullPath
)
{
    BuildFileUser::OnFsDelete(apFullPath);
    TearDownGeneratedDeps(apFullPath);
}

void
BuildFileUser_TmpDep::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    mpParentTmpObj->SetSomethingChangedSinceLastTryRepair();
}

void
BuildFileUser_TmpDep::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    K2_ASSERT(NULL != mpParentTmpObj);
    mpParentTmpObj->OnDamaged();
}

void
BuildFileUser_TmpDep::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    K2_ASSERT(NULL != mpParentTmpObj);
    if (!mpParentTmpObj->CheckIfDamaged())
    {
        mpParentTmpObj->OnRepaired();
    }
    else
    {
        mpParentTmpObj->SetSomethingChangedSinceLastTryRepair();
    }
}

bool
BuildFileUser_TmpDep::TryRepair(
    bool aForce
)
{
    VfsFolder * pParentFolder;
    char *      pParentPath;
    DWORD       fAttr;

    if (!aForce)
    {
        if (!BuildFileUser::TryRepair())
            return false;
    }

    pParentFolder = mpVfsFile->mpParentFolder;
    pParentPath = pParentFolder->AllocFullPath();
    fAttr = GetFileAttributes(pParentPath);
    if (INVALID_FILE_ATTRIBUTES == fAttr)
    {
        if (!CreatePath(pParentPath, pParentFolder->GetFullPathLen()))
        {
            printf("*** Failed to create path for TmpObj [%s]\n", pParentPath);
            delete[] pParentPath;
            return false;
        }
    }
    else if (0 == (fAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        printf("*** Target directory name exists but is not a directory! [%s]\n", pParentPath);
        delete[] pParentPath;
        return false;
    }
    delete[] pParentPath;

    return mpChildSrcCode->Build();
}

bool
BuildFileUser_TmpDep::CheckIfDamaged(
    void
)
{
    K2LIST_LINK *           pListLink;
    BuildFileUser_SrcInc *  pSrcInc;

    if (!mpVfsFile->Exists())
        return true;

    if (mDepParseProblem)
        return true;

    if (!mpChildSrcCode->mpVfsFile->Exists())
        return true;
    if (mpVfsFile->IsOlderThan(mpChildSrcCode->mpVfsFile))
        return true;

    pListLink = GeneratedSrcIncDepList.mpHead;
    if (NULL == pListLink)
        return false;

    do {
        pSrcInc = K2_GET_CONTAINER(BuildFileUser_SrcInc, pListLink, ParentIncDepListLink);
        pListLink = pListLink->mpNext;
        if (!pSrcInc->mpVfsFile->Exists())
            return true;
        if (mpVfsFile->IsOlderThan(pSrcInc->mpVfsFile))
            return true;
    } while (NULL != pListLink);

    return false;
}

void 
BuildFileUser_TmpDep::Dump(
    bool aDamagedOnly
)
{
    K2LIST_LINK *           pListLink;
    BuildFileUser_SrcInc *  pSrcInc;

    if ((aDamagedOnly) && (!IsDamaged()))
        return;

    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("      %s (%s) %s\n", IsDamaged() ? "DAMG" : "GOOD", mDepParseProblem ? "!!!" : "Dok", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;

    if (!IsDamaged())
        return;

    mpChildSrcCode->Dump();

    pListLink = GeneratedSrcIncDepList.mpHead;
    if (NULL == pListLink)
        return;
    do {
        pSrcInc = K2_GET_CONTAINER(BuildFileUser_SrcInc, pListLink, ParentIncDepListLink);
        pListLink = pListLink->mpNext;
        pSrcInc->Dump();
    } while (NULL != pListLink);
}

bool
BuildFileUser_TmpDep::ParseLine(
    char const *            apFullPathOfDepFile,
    BuildFileUser_SrcXml *  apSrcXml,
    BuildFileUser_SrcCode * apSrcCode,
    char const *            apSrcCodePath,
    char const *            apLine,
    UINT_PTR                aLineLen
)
{
    char const *            pHold;
    char                    ch;
    UINT_PTR                entLen;
    char *                  pEnt;
    char *                  pFix;
    bool                    append;
    char *                  pDepFileSpec;
    VfsFile *               pDepVfsFile;
    BuildFileUser_SrcInc *  pInclude;
    bool                    problem;

    problem = false;

    pHold = apLine;
    do {
        ch = *apLine;
        if ((ch == ' ') ||
            (ch == '\t') ||
            (ch == '\r') ||
            (ch == '\n'))
        {
            entLen = (UINT_PTR)(apLine - pHold);
            if (entLen == 1)
            {
                if ((*pHold) == '\\')
                    entLen = 0;
            }
            if (entLen > 0)
            {
                pEnt = new char[(entLen + 4) & ~3];
                K2_ASSERT(NULL != pEnt);
                K2ASC_CopyLen(pEnt, pHold, entLen);
                pEnt[entLen] = 0;
                pFix = pEnt;
                do {
                    ch = *pFix;
                    if (ch == '/')
                        *pFix = '\\';
                    pFix++;
                } while (0 != ch);

                append = true;
                pDepFileSpec = NULL;
                if (entLen > gVfsRootSpecLen)
                {
                    if ((0 == K2ASC_CompInsLen(gpVfsRootSpec, pEnt, gVfsRootSpecLen)) &&
                        (pEnt[gVfsRootSpecLen] == '\\'))
                    {
                        pDepFileSpec = pEnt + gVfsRootSpecLen + 1;
                        append = false;
                    }
                }
                if (append)
                {
                    UINT_PTR newLen;
                    char* pParentFolderPath = apSrcXml->mpVfsFile->mpParentFolder->AllocFullPath();
                    newLen = 4 + (apSrcXml->mpVfsFile->mpParentFolder->GetFullPathLen() - (gVfsRootSpecLen + 5)) + 1 + entLen;
                    pDepFileSpec = new char[(newLen + 4) & ~3];
                    K2_ASSERT(NULL != pDepFileSpec);
                    K2ASC_Printf(pDepFileSpec, "src\\%s\\%s", pParentFolderPath + gVfsRootSpecLen + 5, pEnt);
                    delete[] pParentFolderPath;
                }
                K2_ASSERT(NULL != pDepFileSpec);
                DeDoubleDot(pDepFileSpec);

                pDepVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(pDepFileSpec, false);
                if (NULL != pDepVfsFile)
                {
                    if (0 != K2ASC_CompIns(apSrcCodePath, pDepFileSpec))
                    {
                        pInclude = new BuildFileUser_SrcInc(pDepVfsFile, this);
                        K2_ASSERT(NULL != pInclude);
                    }
                    pDepVfsFile->Release();
                }
                else
                {
                    printf("*** Failure to attach to generated dependency \"%s\" found in [%s]\n", pDepFileSpec, apFullPathOfDepFile);
                    problem = true;
                }

                if (append)
                {
                    delete[] pDepFileSpec;
                }

                delete[] pEnt;
            }
            K2PARSE_EatWhitespace(&apLine, &aLineLen);
            pHold = apLine;
        }
        else
        {
            apLine++;
            --aLineLen;
        }
    } while (0 != aLineLen);

    return !problem;
}

void 
BuildFileUser_TmpDep::BuildGeneratedDeps(
    char const *apFullPath
)
{
    BuildFileUser_SrcCode * pSrcCode;
    BuildFileUser_SrcXml *  pSrcXml;
    K2ReadOnlyMappedFile *  pFile;
    char *                  pSrcFullPath;
    char const *            pLine;
    char const *            pHold;
    UINT_PTR                lineLen;
    char                    ch;
    bool                    quoted;
    char const *            pFileData;
    UINT_PTR                fileBytes;

    K2_ASSERT(0 == GeneratedSrcIncDepList.mNodeCount);

    K2_ASSERT(mpVfsFile->Exists());

    mDepParseProblem = true;

    pFile = K2ReadOnlyMappedFile::Create(apFullPath);
    if (NULL == pFile)
    {
//        printf("*** Could not access dependency file [%s]\n", apFullPath);
        return;
    }

    pSrcCode = mpChildSrcCode;
    K2_ASSERT(NULL != mpParentTmpObj);

    pSrcXml = mpParentTmpObj->mpParentSrcXml;
    K2_ASSERT(NULL != pSrcXml);
    K2_ASSERT(pSrcXml->mpVfsFile->Exists());
    K2_ASSERT(NULL != pSrcXml->XmlParse.mpRootNode);

    mInDepBuild = true;

    do {
        //
        // find first nonempty line
        //
        pFileData = (char const *)pFile->DataPtr();
        fileBytes = (UINT_PTR)pFile->FileBytes();
        pLine = pFileData;
        do {
            K2PARSE_EatWhitespace(&pFileData, &fileBytes);
            if (0 == fileBytes)
                break;
            pLine = pFileData;
            K2PARSE_EatToEOL(&pFileData, &fileBytes);
            lineLen = (UINT_PTR)(pFileData - pLine);
            K2PARSE_EatEOL(&pFileData, &fileBytes);
            K2PARSE_EatWhitespaceAtEnd(pLine, &lineLen);
        } while (0 == lineLen);

        if (0 == fileBytes)
            break;

        //
        // first line should have a freestanding colon on it
        // find the first whitespace
        //
        if (*pLine == '\'')
        {
            pLine++;
            --lineLen;
            quoted = true;
        }
        else
        {
            quoted = false;
        }
        pHold = pLine;
        do {
            ch = *pLine;
            if ((ch == ' ') ||
                (ch == '\t'))
                break;
            pLine++;
        } while (0 != --lineLen);

        UINT_PTR specLen = (UINT_PTR)(pLine - pHold);
        --pLine;
        ch = *pLine;
        ++pLine;
        if (ch == ':')
        {
            --specLen;
        }
        else
        {
            K2PARSE_EatWhitespace(&pLine, &lineLen);
            if (0 == lineLen)
                break;
            ch = *pLine;
            if (ch != ':')
                break;
            pLine++;
            lineLen--;
        }
        if (quoted)
        {
            if (pHold[specLen - 1] != '\'')
            {
                break;
            }
            --specLen;
        }

        //
        // past the colon on the first line
        //
        char *pSpec = new char[(specLen + 4) & ~3];
        K2_ASSERT(NULL != pSpec);
        K2ASC_CopyLen(pSpec, pHold, specLen);
        pSpec[specLen] = 0;

        char *pFix = pSpec;
        do {
            ch = *pFix;
            if (ch == '/')
            {
                *pFix = '\\';
            }
            pFix++;
        } while (0 != ch);

        if (0 != K2ASC_CompInsLen(pSpec, gpVfsRootSpec, gVfsRootSpecLen))
        {
            UINT_PTR hostDirLen = pSrcXml->mpVfsFile->mpParentFolder->GetFullPathLen();
            char *pSpec2 = pSrcXml->mpVfsFile->mpParentFolder->AllocFullPath(specLen + 1);
            pSpec2[hostDirLen] = '\\';
            K2ASC_Copy(pSpec2 + hostDirLen + 1, pSpec);
            specLen += hostDirLen + 1;
            delete[] pSpec;
            pSpec = pSpec2;
        }

        delete[] pSpec;

        pSrcFullPath = pSrcCode->mpVfsFile->AllocFullPath();

        //
        // start eating files
        //
        K2PARSE_EatWhitespace(&pLine, &lineLen);
        if (0 != lineLen)
        {
            mDepParseProblem = !ParseLine(apFullPath, pSrcXml, pSrcCode, pSrcFullPath + gVfsRootSpecLen + 1, pLine, lineLen);
            if (mDepParseProblem)
                break;
        }
        do {
            K2PARSE_EatWhitespace(&pFileData, &fileBytes);
            if (0 == fileBytes)
                break;
            pLine = pFileData;
            K2PARSE_EatToEOL(&pFileData, &fileBytes);
            lineLen = (UINT_PTR)(pFileData - pLine);
            K2PARSE_EatEOL(&pFileData, &fileBytes);
            if (0 != lineLen)
            {
                K2PARSE_EatWhitespaceAtEnd(pLine, &lineLen);
                if (0 != lineLen)
                {
                    mDepParseProblem = !ParseLine(apFullPath, pSrcXml, pSrcCode, pSrcFullPath + gVfsRootSpecLen + 1, pLine, lineLen);
                    if (mDepParseProblem)
                        break;
                }
            }
        } while (0 != fileBytes);

    } while (0);

    delete pFile;

    if (mDepParseProblem)
    {
        TearDownGeneratedDeps(apFullPath);
    }

    mInDepBuild = false;
}

void 
BuildFileUser_TmpDep::TearDownGeneratedDeps(
    char const *apFullPath
)
{
    K2LIST_LINK *           pListLink;
    BuildFileUser_SrcInc *  pDerivedDep;

    pListLink = GeneratedSrcIncDepList.mpHead;
    if (NULL == pListLink)
        return;

    do {
        pDerivedDep = K2_GET_CONTAINER(BuildFileUser_SrcInc, pListLink, ParentIncDepListLink);
        pListLink = pListLink->mpNext;
        K2_ASSERT(pDerivedDep->mpParentTmpDep == this);
        delete pDerivedDep;
    } while (NULL != pListLink);
}

void 
BuildFileUser_TmpDep::OnSrcCodeDamaged(
    void
)
{
    OnDamaged();
}

void 
BuildFileUser_TmpDep::OnSrcCodeRepaired(
    void
)
{
    if (!CheckIfDamaged())
        OnRepaired();
}

void
BuildFileUser_TmpDep::OnSrcIncDamaged(
    BuildFileUser_SrcInc *apSrcCode
)
{
    mpChildSrcCode->OnDamaged();
}

void
BuildFileUser_TmpDep::OnSrcIncRepaired(
    BuildFileUser_SrcInc *apSrcCode
)
{
    if (!mInDepBuild)
    {
        if (!CheckIfDamaged())
            OnRepaired();
    }
}

