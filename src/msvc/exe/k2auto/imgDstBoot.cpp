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

BuildFileUser_ImgDstBoot *
BuildFileUser_ImgDstBoot::Construct(
    VfsFile *               apVfsFile,
    BuildFileUser_SrcXml *  apSrcXml,
    char const *            apXmlFullPath
)
{
    BuildFileUser_ImgDstBoot *  pResult;
    BuildFileUser_ImgSrcBoot *  pSrcBoot;
    VfsFile *                   pSrcVfsFile;

    K2ASC_Printf(gStrBuf, "src\\%s\\boot\\boot%s.efi", gpOsVer, gArchBoot[gArch]);

    pSrcVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(gStrBuf, false);
    if (NULL == pSrcVfsFile)
    {
        printf("*** Could not acquire vfs file of bootloader [%s]\n", gStrBuf);
        return NULL;
    }

    pSrcBoot = new BuildFileUser_ImgSrcBoot(pSrcVfsFile, apSrcXml);
    K2_ASSERT(NULL != pSrcBoot);

    pSrcVfsFile->Release();

    pResult = new BuildFileUser_ImgDstBoot(apVfsFile, apSrcXml, pSrcBoot);
    K2_ASSERT(NULL != pResult);

    return pResult;
}

BuildFileUser_ImgDstBoot::BuildFileUser_ImgDstBoot(
    VfsFile *                   apVfsFile,
    BuildFileUser_SrcXml *      apSrcXml,
    BuildFileUser_ImgSrcBoot *  apSrcBoot
) :
    BuildFileUser(apVfsFile, BuildFileUserType_ImgDstBoot),
    mpSrcBoot(apSrcBoot)
{
    mpParentImg = NULL;
    mpSrcBoot->mpDstBoot = this;
    if (!CheckIfDamaged())
        OnRepaired();
}

BuildFileUser_ImgDstBoot::~BuildFileUser_ImgDstBoot(
    void
)
{
    delete mpSrcBoot;
    mpParentImg = NULL;
}

void
BuildFileUser_ImgDstBoot::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    if (!CheckIfDamaged())
        OnRepaired();
}

void
BuildFileUser_ImgDstBoot::OnFsUpdate(
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
BuildFileUser_ImgDstBoot::OnDamaged(
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
BuildFileUser_ImgDstBoot::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    if (NULL != mpParentImg)
    {
        mpParentImg->OnBootRepaired();
    }
}

void 
BuildFileUser_ImgDstBoot::OnSrcDamaged(
    void
)
{
    OnDamaged();
}

void 
BuildFileUser_ImgDstBoot::OnSrcRepaired(
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
BuildFileUser_ImgDstBoot::TryRepair(
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

    pSrc = mpSrcBoot->mpVfsFile->AllocFullPath();
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
BuildFileUser_ImgDstBoot::SetSomethingChangedSinceLastTryRepair(
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
BuildFileUser_ImgDstBoot::CheckIfDamaged(
    void
)
{
    if (!FileExists())
        return true;

    K2_ASSERT(NULL != mpSrcBoot);

    if (!mpSrcBoot->FileExists())
        return true;

    if (mpVfsFile->IsOlderThan(mpSrcBoot->mpVfsFile))
        return true;

    return false;
}

void
BuildFileUser_ImgDstBoot::Dump(
    bool aDamagedOnly
)
{
    if ((aDamagedOnly) && (!IsDamaged()))
        return;

    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("  BOOTDST %s %s\n", IsDamaged() ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;

    if ((aDamagedOnly) && (mpSrcBoot->FileExists()))
        return;

    pFullPath = mpSrcBoot->mpVfsFile->AllocFullPath();
    printf("    BOOTSRC GOOD %s\n", pFullPath + gVfsRootSpecLen);
    delete[] pFullPath;
}

