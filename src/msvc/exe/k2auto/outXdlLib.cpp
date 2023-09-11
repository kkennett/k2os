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

BuildFileUser_OutXdlLib *
BuildFileUser_OutXdlLib::Construct(
    VfsFile *               apVfsFile, 
    BuildFileUser_SrcXml *  apSrcXml, 
    VfsFile *               apInfVfsFile,
    char const *            apXmlFullPath
)
{
    BuildFileUser_OutXdlLib *   pResult;
    BuildFileUser_TmpElf *      pTmpElf;
    VfsFile *                   pTmpElfVfsFile;

    K2ASC_Printf(gStrBuf, "bld\\out\\gcc\\xdl%s\\%s\\srcelf\\%s\\%s.elf",
        apSrcXml->mIsKernelTarget ? "\\kern" : "",
        gpArchMode,
        apSrcXml->mpSubPath,
        apSrcXml->mpVfsFile->mpParentFolder->mpNameZ);

    pTmpElfVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(gStrBuf, false);
    if (NULL == pTmpElfVfsFile)
    {
        printf("*** Could not acquire tmp elf \"%s\" for [%s]\n", gStrBuf, apXmlFullPath);
        return NULL;
    }

    pTmpElf = BuildFileUser_TmpElf::Construct(pTmpElfVfsFile, apSrcXml, apInfVfsFile, apXmlFullPath);

    pTmpElfVfsFile->Release();

    if (NULL != pTmpElf)
    {
        pResult = new BuildFileUser_OutXdlLib(apVfsFile, pTmpElf);
        K2_ASSERT(NULL != pResult);
    }
    else
    {
        pResult = NULL;
    }

    return pResult;
}

BuildFileUser_OutXdlLib::BuildFileUser_OutXdlLib(
    VfsFile *               apVfsFile,
    BuildFileUser_TmpElf *  apTmpElf
) : BuildFileUser(apVfsFile, BuildFileUserType_TmpElf),
    mpChildTmpElf(apTmpElf)
{
    mpChildTmpElf->mpParentOutXdlLib = this;
    mpParentOutXdl = NULL;
    if (!CheckIfDamaged())
        OnRepaired();
}

BuildFileUser_OutXdlLib::~BuildFileUser_OutXdlLib(
    void
)
{
    mpChildTmpElf->mpParentOutXdlLib = NULL;
    delete mpChildTmpElf;
}

void
BuildFileUser_OutXdlLib::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    if (!CheckIfDamaged())
        OnRepaired();
}

void 
BuildFileUser_OutXdlLib::OnFsUpdate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsUpdate(apFullPath);
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
BuildFileUser_OutXdlLib::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    K2_ASSERT(NULL != mpParentOutXdl);
    mpParentOutXdl->OnDamaged();
}

void 
BuildFileUser_OutXdlLib::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    if (!mpParentOutXdl->CheckIfDamaged())
    {
        mpParentOutXdl->OnRepaired();
    }
    else
    {
        mpParentOutXdl->SetSomethingChangedSinceLastTryRepair();
    }
}

void
BuildFileUser_OutXdlLib::OnChildDamaged(
    void
)
{
    OnDamaged();
}

void
BuildFileUser_OutXdlLib::OnChildRepaired(
    void
)
{
    if (!CheckIfDamaged())
    {
        OnRepaired();
    }
    else
    {
        SetSomethingChangedSinceLastTryRepair();
    }
}

bool
BuildFileUser_OutXdlLib::TryRepair(
    bool aForce
)
{
    char const * const spConvCmd = "k2elf2xdl%s%s%.*s -s %d -i %s -o %s -l %s";

    BuildFileUser_SrcXml *  pSrcXml;
    VfsFolder *             pParentFolder;
    char *                  pParentPath;
    DWORD                   fAttr;
    K2XML_NODE const *      pRootNode;
    UINT_PTR                stackPages;
    char const *            pPlacement;
    UINT_PTR                placementLen;
    UINT_PTR                ixNode;
    K2XML_NODE const *      pSubNode;
    char const *            pPars;
    UINT_PTR                len;
    char *                  pTmpElfPath;
    char *                  pOutputPath;
    char *                  pOutLibPath;
    char *                  pCmdLine;
    char *                  pEnv;
    UINT_PTR                blobLen;
    UINT_PTR                outLen;
    char *                  pOutput;
    int                     cmdResult;

    if (!aForce)
    {
        if (!BuildFileUser::TryRepair())
            return false;
    }

    if (mpChildTmpElf->IsDamaged())
    {
        return mpChildTmpElf->TryRepair();
    }

    pSrcXml = mpParentOutXdl->mpParentSrcXml;

    stackPages = 0;
    pPlacement = NULL;
    placementLen = 0;
    pRootNode = pSrcXml->XmlParse.mpRootNode;
    for (ixNode = 0; ixNode < pRootNode->mNumSubNodes; ixNode++)
    {
        pSubNode = pRootNode->mppSubNode[ixNode];
        if ((pSubNode->Name.mLen == 5) &&
            (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "stack", 5)))
        {
            //
            // found stack tag
            //
            pPars = pSubNode->Content.mpChars;
            len = pSubNode->Content.mLen;
            K2PARSE_EatWhitespace(&pPars, &len);
            K2PARSE_EatWhitespaceAtEnd(pPars, &len);
            stackPages = K2ASC_NumValue32Len(pPars, len);
        }
        else if ((pSubNode->Name.mLen == 9) &&
            (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "placement", 9)))
        {
            //
            // found placement tag
            //
            pPlacement = pSubNode->Content.mpChars;
            placementLen = pSubNode->Content.mLen;
            K2PARSE_EatWhitespace(&pPlacement, &placementLen);
            K2PARSE_EatWhitespaceAtEnd(pPlacement, &placementLen);
        }
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

    pTmpElfPath = mpChildTmpElf->mpVfsFile->AllocFullPath();
    pOutputPath = mpParentOutXdl->mpVfsFile->AllocFullPath();
    pOutLibPath = mpVfsFile->AllocFullPath();
    len = K2ASC_Printf(gStrBuf, spConvCmd, 
        pSrcXml->mIsKernelTarget ? " -k" : "",
        (placementLen > 0) ? " -p " : "",
        placementLen,
        (placementLen > 0) ? pPlacement : "",
        stackPages,
        pTmpElfPath,
        pOutputPath,
        pOutLibPath);
    delete[] pTmpElfPath;
    delete[] pOutputPath;
    delete[] pOutLibPath;

    pCmdLine = new char[(len + 4) & ~3];
    K2_ASSERT(NULL != pCmdLine);
    K2ASC_CopyLen(pCmdLine, gStrBuf, len);
    pCmdLine[len] = 0;

    //
    // we have our command line
    //
    pEnv = gpBaseEnviron->CopyBlob(&blobLen);
    K2_ASSERT(NULL != pEnv);

    outLen = 0;
    pOutput = NULL;
//    printf("%s\n", pCmdLine);
    printf("Create XDL [%s] and XDL import Lib [%s]\n", mpParentOutXdl->mpVfsFile->mpNameZ, mpVfsFile->mpNameZ);
    cmdResult = RunProgram(gpVfsRootSpec, pCmdLine, pEnv, &pOutput, &outLen);
    if (0 != outLen)
    {
        K2_ASSERT(NULL != pOutput);
        printf("%.*s\n", outLen, pOutput);
        delete[] pOutput;
    }
    else
    {
        K2_ASSERT(NULL == pOutput);
    }

    delete[] pEnv;
    delete[] pCmdLine;

    if (0 != cmdResult)
    {
        printf("error %d\n", cmdResult);
        return false;
    }

    return true;
}

void
BuildFileUser_OutXdlLib::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    mpParentOutXdl->SetSomethingChangedSinceLastTryRepair();
}

bool 
BuildFileUser_OutXdlLib::CheckIfDamaged(
    void
)
{
    if (!FileExists())
        return true;

    K2_ASSERT(NULL != mpChildTmpElf);

    if (mpChildTmpElf->IsDamaged())
        return true;

    if (mpVfsFile->IsOlderThan(mpChildTmpElf->mpVfsFile))
        return true;

    return false;
}

void 
BuildFileUser_OutXdlLib::Dump(
    bool aDamagedOnly
)
{
    if ((aDamagedOnly) && (!IsDamaged()))
        return;

    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("    OUTXDLLIB %s %s\n", IsDamaged() ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;

    if (IsDamaged())
        mpChildTmpElf->Dump(aDamagedOnly);
}

