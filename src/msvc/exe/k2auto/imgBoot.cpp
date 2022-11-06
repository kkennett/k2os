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

BuildFileUser_ImgBoot *
BuildFileUser_ImgBoot::Construct(
    VfsFile *               apVfsFile,
    BuildFileUser_SrcXml *  apSrcXml,
    char const *            apXmlFullPath
)
{
    BuildFileUser_ImgBoot * pResult;
    VfsFile *               pSrcVfsFile;

    K2ASC_Printf(gStrBuf, "src\\os8\\boot\\boot%s.efi", gArchBoot[gArch]);

    pSrcVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(gStrBuf, false);
    if (NULL == pSrcVfsFile)
    {
        printf("*** Could not acquire vfs file of bootloader [%s]\n", gStrBuf);
        return NULL;
    }

    pResult = new BuildFileUser_ImgBoot(apVfsFile, apSrcXml, pSrcVfsFile);
    K2_ASSERT(NULL != pResult);
    pSrcVfsFile->Release();

    return pResult;
}

BuildFileUser_ImgBoot::BuildFileUser_ImgBoot(
    VfsFile *               apVfsFile,
    BuildFileUser_SrcXml *  apSrcXml,
    VfsFile *               apSrcFile
) :
    BuildFileUser(apVfsFile, BuildFileUserType_ImgBoot),
    mpSrcFile(apSrcFile)
{
    mpParentImg = NULL;
    if (!CheckIfDamaged())
        OnRepaired();
}

BuildFileUser_ImgBoot::~BuildFileUser_ImgBoot(
    void
)
{
    mpSrcFile->Release();
    mpParentImg = NULL;
}

void
BuildFileUser_ImgBoot::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    if (!CheckIfDamaged())
        OnRepaired();
}

void
BuildFileUser_ImgBoot::OnFsUpdate(
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
BuildFileUser_ImgBoot::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    if (NULL != mpParentImg)
    {
        mpParentImg->OnBootDamaged();
    }
}

void 
BuildFileUser_ImgBoot::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    if (NULL != mpParentImg)
    {
        mpParentImg->OnBootRepaired();
    }
}

bool
BuildFileUser_ImgBoot::TryRepair(
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

    pSrc = mpSrcFile->AllocFullPath();
    pDst = mpVfsFile->AllocFullPath();

    didSomething = (FALSE != CopyFile(pSrc, pDst, FALSE));
    printf("Copying bootloader efi to target [%s]\n", pDst);
    if (!didSomething)
    {
        printf("*** Failed to copy file:\n    %s\n to %s\n", pSrc, pDst);
    }

    delete[] pSrc;
    delete[] pDst;

    return didSomething;
}

void
BuildFileUser_ImgBoot::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    if (NULL != mpParentImg)
    {
        mpParentImg->SetSomethingChangedSinceLastTryRepair();
    }
}

bool
BuildFileUser_ImgBoot::CheckIfDamaged(
    void
)
{
    if (!mpVfsFile->Exists())
        return true;

    K2_ASSERT(NULL != mpSrcFile);

    if (!mpSrcFile->Exists())
        return true;

    if (mpVfsFile->IsOlderThan(mpSrcFile))
        return true;

    return false;
}

void
BuildFileUser_ImgBoot::Dump(
    bool aDamagedOnly
)
{
    if ((aDamagedOnly) && (!IsDamaged()))
        return;

    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("  BOOTEFI %s %s\n", IsDamaged() ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;

    if ((aDamagedOnly) && (mpSrcFile->Exists()))
        return;

    pFullPath = mpSrcFile->AllocFullPath();
    printf("    BOOTSRC GOOD %s\n", pFullPath + gVfsRootSpecLen);
    delete[] pFullPath;
}

