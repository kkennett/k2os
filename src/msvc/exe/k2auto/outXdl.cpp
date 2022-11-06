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

BuildFileUser_OutXdl *
BuildFileUser_OutXdl::Construct(
    VfsFile *               apVfsFile,
    BuildFileUser_SrcXml *  apParentSrcXml,
    VfsFile *               apInfVfsFile,
    char const *            apXmlFullPath
)
{
    BuildFileUser_OutXdl *      pResult;
    BuildFileUser_OutXdlLib *   pOutXdlLib;
    VfsFile *                   pOutXdlLibVfsFile;

    K2ASC_Printf(gStrBuf, "bld\\out\\gcc\\xdllib%s\\%s\\%s\\%s.lib",
        apParentSrcXml->mIsKernelTarget ? "\\kern" : "",
        gpArchMode,
        apParentSrcXml->mpSubPath,
        apParentSrcXml->mpVfsFile->mpParentFolder->mpNameZ);

    pOutXdlLibVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(gStrBuf, false);
    if (NULL == pOutXdlLibVfsFile)
    {
        printf("*** Could not acquire xdl library output file \"%s\" for [%s]\n", gStrBuf, apXmlFullPath);
        return NULL;
    }

    pOutXdlLib = BuildFileUser_OutXdlLib::Construct(pOutXdlLibVfsFile, apParentSrcXml, apInfVfsFile, apXmlFullPath);

    pOutXdlLibVfsFile->Release();

    if (NULL != pOutXdlLib)
    {
        pResult = new BuildFileUser_OutXdl(apVfsFile, apParentSrcXml, pOutXdlLib);
        K2_ASSERT(NULL != pResult);
    }
    else
    {
        pResult = NULL;
    }

    return pResult;
}

BuildFileUser_OutXdl::BuildFileUser_OutXdl(
    VfsFile *                   apVfsFile, 
    BuildFileUser_SrcXml *      apSrcXml,
    BuildFileUser_OutXdlLib *   apChildXdlLib
) :
    BuildFileUser(apVfsFile, BuildFileUserType_OutXdl),
    mpParentSrcXml(apSrcXml),
    mpChildOutXdlLib(apChildXdlLib)
{
    mpChildOutXdlLib->mpParentOutXdl = this;
    if (!CheckIfDamaged())
        OnRepaired();
}

BuildFileUser_OutXdl::~BuildFileUser_OutXdl(
    void
)
{
    mpChildOutXdlLib->mpParentOutXdl = NULL;
    delete mpChildOutXdlLib;
}

void
BuildFileUser_OutXdl::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    if (!CheckIfDamaged())
        OnRepaired();
}

void
BuildFileUser_OutXdl::OnFsUpdate(
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
BuildFileUser_OutXdl::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    mpParentSrcXml->OnOutputDamaged();
}

void 
BuildFileUser_OutXdl::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    mpParentSrcXml->OnOutputRepaired();
}

void
BuildFileUser_OutXdl::OnChildDamaged(
    BuildFileUser_TmpObj *  apChild
)
{
    mpChildOutXdlLib->mpChildTmpElf->mpChildTmpExpObj->OnDamaged();
}

void
BuildFileUser_OutXdl::OnChildRepaired(
    BuildFileUser_TmpObj * apTmpObjRepaired
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
BuildFileUser_OutXdl::TryRepair(
    void
)
{
    VfsFolder * pParentFolder;
    char *      pParentPath;
    DWORD       fAttr;

    if (!BuildFileUser::TryRepair())
        return false;

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

    return mpChildOutXdlLib->TryRepair(true);
}

void
BuildFileUser_OutXdl::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    mpParentSrcXml->SetSomethingChangedSinceLastTryRepair();
}

bool 
BuildFileUser_OutXdl::CheckIfDamaged(void) 
{ 
    if (!mpVfsFile->Exists())
        return true;

    K2_ASSERT(NULL != mpChildOutXdlLib);

    if (mpChildOutXdlLib->IsDamaged())
        return true;

    if (mpVfsFile->IsOlderThan(mpChildOutXdlLib->mpVfsFile))
        return true;

    return false; 
}

void 
BuildFileUser_OutXdl::Dump(
    bool aDamagedOnly
)
{
    if ((aDamagedOnly) && (!IsDamaged()))
        return;

    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("  OUTXDL %s %s\n", IsDamaged() ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;

    mpChildOutXdlLib->Dump(aDamagedOnly);
}

