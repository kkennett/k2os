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

BuildFileUser_OutObj *
BuildFileUser_OutObj::Construct(
    VfsFile *               apVfsFile,
    BuildFileUser_SrcXml *  apSrcXml,
    char const *            apXmlFullPath
)
{
    BuildFileUser_OutObj *  pResult;
    BuildFileUser_TmpObj *  pTmpObj;
    K2XML_NODE const *      pXmlNode;
    UINT_PTR                nodeCount;
    UINT_PTR                nodeIx;
    char *                  pSrcCodePath;
    VfsFile *               pSrcCodeVfsFile;

    K2_ASSERT(1 == apSrcXml->mSourcesCount);
    K2_ASSERT(NULL != apSrcXml->XmlParse.mpRootNode);
    nodeCount = apSrcXml->XmlParse.mpRootNode->mNumSubNodes;
    pXmlNode = NULL;
    for (nodeIx = 0; nodeIx < nodeCount; nodeIx++)
    {
        pXmlNode = apSrcXml->XmlParse.mpRootNode->mppSubNode[nodeIx];
        if ((6 != pXmlNode->Name.mLen) ||
            (0 != K2ASC_CompInsLen("source", pXmlNode->Name.mpChars, 6)))
            continue;
        break;
    }

    K2_ASSERT(0 != ((1 << gArch) & GetXmlNodeArchFlags(pXmlNode)));

    pSrcCodePath = NULL;
    if (!GetContentTarget(apSrcXml->mpVfsFile->mpParentFolder, pXmlNode, &pSrcCodePath))
    {
        printf("*** Could not resolve <source> \"%.*s\" in %s [%s]\n",
            pXmlNode->Content.mLen, pXmlNode->Content.mpChars,
            gpBuildXmlFileName,
            apXmlFullPath);
        return NULL;
    }

    pSrcCodeVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(pSrcCodePath + gVfsRootSpecLen + 1, false);
    
    if (NULL == pSrcCodeVfsFile)
    {
        printf("*** Coult not acquire vfs file \"%s\" in %s [%s]\n",
            pSrcCodePath + gVfsRootSpecLen + 1, gpBuildXmlFileName, apXmlFullPath);
    }

    pTmpObj = BuildFileUser_TmpObj::Construct(pSrcCodePath, pSrcCodeVfsFile, apSrcXml, pXmlNode, apXmlFullPath);

    delete[] pSrcCodePath;

    pSrcCodeVfsFile->Release();

    if (NULL == pTmpObj)
        return NULL;

    pResult = new BuildFileUser_OutObj(apVfsFile, apSrcXml, pTmpObj);
    K2_ASSERT(NULL != pResult);

    return pResult;
}

BuildFileUser_OutObj::BuildFileUser_OutObj(
    VfsFile *               apVfsFile, 
    BuildFileUser_SrcXml *  apSrcXml, 
    BuildFileUser_TmpObj *  apTmpObj
) :
    BuildFileUser(apVfsFile, BuildFileUserType_OutObj),
    mpParentSrcXml(apSrcXml),
    mpChildTmpObj(apTmpObj)
{
    if (!CheckIfDamaged())
        OnRepaired();
}

BuildFileUser_OutObj::~BuildFileUser_OutObj(
    void
)
{
    delete mpChildTmpObj;
}

void
BuildFileUser_OutObj::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    if (!CheckIfDamaged())
        OnRepaired();
}

void
BuildFileUser_OutObj::OnFsUpdate(
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
BuildFileUser_OutObj::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    mpParentSrcXml->OnOutputDamaged();
}

void 
BuildFileUser_OutObj::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    mpParentSrcXml->OnOutputRepaired();
}

void
BuildFileUser_OutObj::OnChildDamaged(
    BuildFileUser_TmpObj *  apChild
)
{
    K2_ASSERT(mpChildTmpObj == apChild);
    OnDamaged();
}

void
BuildFileUser_OutObj::OnChildRepaired(
    BuildFileUser_TmpObj *  apChild
)
{
    K2_ASSERT(mpChildTmpObj == apChild);
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
BuildFileUser_OutObj::TryRepair(
    void
)
{
    VfsFolder * pParentFolder;
    char *      pSrc;
    char *      pDst;
    char *      pParentPath;
    DWORD       fAttr;
    bool        didSomething;

    if (!BuildFileUser::TryRepair())
        return false;

    didSomething = false;

    if (mpChildTmpObj->IsDamaged())
    {
        return mpChildTmpObj->TryRepair();
    }

    pParentFolder = mpVfsFile->mpParentFolder;
    pParentPath = pParentFolder->AllocFullPath();
    fAttr = GetFileAttributes(pParentPath);
    if (INVALID_FILE_ATTRIBUTES == fAttr)
    {
        if (!CreatePath(pParentPath, pParentFolder->GetFullPathLen()))
        {
            printf("*** Failed to create path for OutObj [%s]\n", pParentPath);
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

    pDst = mpVfsFile->AllocFullPath();
    pSrc = mpChildTmpObj->mpVfsFile->AllocFullPath();
    didSomething = (FALSE != CopyFile(pSrc, pDst, FALSE));
    printf("Copying obj to target [%s]\n", pDst);
    if (!didSomething)
    {
        printf("*** Failed to copy file:\n    %s\n to %s\n", pSrc, pDst);
    }

    delete[] pSrc;
    delete[] pDst;

    return didSomething;
}

void
BuildFileUser_OutObj::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    mpParentSrcXml->SetSomethingChangedSinceLastTryRepair();
}

bool
BuildFileUser_OutObj::CheckIfDamaged(
    void
)
{
    if (!FileExists())
        return true;

    K2_ASSERT(NULL != mpChildTmpObj);

    if (mpChildTmpObj->IsDamaged())
        return true;

    if (mpVfsFile->IsOlderThan(mpChildTmpObj->mpVfsFile))
        return true;

    return false;
}

void
BuildFileUser_OutObj::Dump(
    bool aDamagedOnly
)
{
    if ((aDamagedOnly) && (!IsDamaged()))
        return;

    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("  OUTOBJ %s %s\n", IsDamaged() ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;

    if (IsDamaged())
        mpChildTmpObj->Dump(aDamagedOnly);
}

