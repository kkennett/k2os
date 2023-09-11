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

BuildFileUser_ImgDstXdl::BuildFileUser_ImgDstXdl(
    VfsFile *               apVfsFile,
    BuildFileUser_Img *     apParentImg,
    BuildFileUser_SrcXml *  apSrcProjSrcXml
) :
    BuildFileUser(apVfsFile, BuildFileUserType_ImgDstXdl),
    mpParentImg(apParentImg),
    mpSrcProjSrcXml(apSrcProjSrcXml)
{
    mpSrcProjSrcXml->AddRef();
    K2LIST_AddAtTail(&mpParentImg->ImgTargetList, &ImgTargetListLink);
    if (!CheckIfDamaged())
        OnRepaired();
}

BuildFileUser_ImgDstXdl::~BuildFileUser_ImgDstXdl(
    void
)
{
    mpSrcProjSrcXml->Release();
    K2LIST_Remove(&mpParentImg->ImgTargetList, &ImgTargetListLink);
}

void
BuildFileUser_ImgDstXdl::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    if (!CheckIfDamaged())
        OnRepaired();
}

void
BuildFileUser_ImgDstXdl::OnFsUpdate(
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
BuildFileUser_ImgDstXdl::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    mpParentImg->OnImgDstXdlDamaged(this);
}

void 
BuildFileUser_ImgDstXdl::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    mpParentImg->OnImgDstXdlRepaired(this);
}

bool
BuildFileUser_ImgDstXdl::TryRepair(
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

    if (mpSrcProjSrcXml->IsDamaged())
        return false;

//    if (mpVfsFile->IsOlderThan(mpSrcProjSrcXml->Proj.Xdl.mpOutXdl->mpVfsFile))
//        return false;

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
    pSrc = mpSrcProjSrcXml->Proj.Xdl.mpOutXdl->mpVfsFile->AllocFullPath();
    didSomething = (FALSE != CopyFile(pSrc, pDst, FALSE));
    printf("Copying xdl to img target [%s]\n", pDst);
    if (!didSomething)
    {
        printf("*** Failed to copy file:\n    %s\n to %s\n", pSrc, pDst);
    }

    delete[] pSrc;
    delete[] pDst;

    return didSomething;
}

void
BuildFileUser_ImgDstXdl::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    mpParentImg->SetSomethingChangedSinceLastTryRepair();
}

bool
BuildFileUser_ImgDstXdl::CheckIfDamaged(
    void
)
{
    if (!FileExists())
        return true;

    if (mpSrcProjSrcXml->IsDamaged())
        return true;

    if (mpVfsFile->IsOlderThan(mpSrcProjSrcXml->Proj.Xdl.mpOutXdl->mpVfsFile))
        return true;

    return false;
}

void
BuildFileUser_ImgDstXdl::Dump(
    bool aDamagedOnly
)
{
    if ((aDamagedOnly) && (!IsDamaged()))
        return;

    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("  IMGDSTXDL %s %s\n", IsDamaged() ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;
}

