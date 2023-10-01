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

BuildFileUser_Img *
BuildFileUser_Img::Construct(
    VfsFile *               apVfsFile,
    BuildFileUser_SrcXml *  apSrcXml,
    BuildFileUser_ImgDstBoot * apImgBoot,
    char const *            apXmlFullPath
)
{
    BuildFileUser_Img *         pResult;
    K2LIST_LINK *               pListLink;
    ProjDep *                   pProjDep;
    BuildFileUser_ImgDstXdl *   pImgDstXdl;
    bool                        ok;
    char *                      pEnd;
    VfsFile *                   pImgDstXdlVfsFile;

    pResult = new BuildFileUser_Img(apVfsFile, apSrcXml, apImgBoot);
    K2_ASSERT(NULL != pResult);

    if (NULL == pResult)
        return NULL;

    ok = true;

    do {
        pListLink = apSrcXml->Proj.Img.RawXdlProjList.mpHead;
        if (NULL != pListLink)
        {
            ok = false;
            pEnd = gStrBuf + K2ASC_Printf(gStrBuf, "bootdisk\\K2OS\\%s\\KERN\\", gpArchName[gArch]);
            do {
                pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
                K2ASC_Printf(pEnd, "%s.xdl", pProjDep->mpDependOn->mpVfsFile->mpParentFolder->mpNameZ);
                pImgDstXdlVfsFile = (VfsFile *)apSrcXml->Proj.Img.mpImgFolder->AcquireOrCreateSub(gStrBuf, false);
                if (NULL == pImgDstXdlVfsFile)
                {
                    break;
                }
                pImgDstXdl = new BuildFileUser_ImgDstXdl(pImgDstXdlVfsFile, pResult, pProjDep->mpDependOn);
                pImgDstXdlVfsFile->Release();
                if (NULL == pImgDstXdl)
                {
                    break;
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
            if (NULL == pListLink)
            {
                ok = true;
            }
        }
        if (!ok)
            break;

        pListLink = apSrcXml->Proj.Img.UserXdlProjList.mpHead;
        if (NULL != pListLink)
        {
            ok = false;
            pEnd = gStrBuf + K2ASC_Printf(gStrBuf, "bootdisk\\K2OS\\%s\\", gpArchName[gArch]);
            do {
                pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
                K2ASC_Printf(pEnd, "%s.xdl", pProjDep->mpDependOn->mpVfsFile->mpParentFolder->mpNameZ);
                pImgDstXdlVfsFile = (VfsFile *)apSrcXml->Proj.Img.mpImgFolder->AcquireOrCreateSub(gStrBuf, false);
                if (NULL == pImgDstXdlVfsFile)
                {
                    break;
                }
                pImgDstXdl = new BuildFileUser_ImgDstXdl(pImgDstXdlVfsFile, pResult, pProjDep->mpDependOn);
                pImgDstXdlVfsFile->Release();
                if (NULL == pImgDstXdl)
                {
                    break;
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
            if (NULL == pListLink)
            {
                ok = true;
            }
        }
        if (!ok)
            break;

        pListLink = apSrcXml->Proj.Img.BuiltInXdlProjList.mpHead;
        if (NULL != pListLink)
        {
            ok = false;
            pEnd = gStrBuf + K2ASC_Copy(gStrBuf, "builtin\\");
            do {
                pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
                K2ASC_Printf(pEnd, "%s.xdl", pProjDep->mpDependOn->mpVfsFile->mpParentFolder->mpNameZ);
                pImgDstXdlVfsFile = (VfsFile *)apSrcXml->Proj.Img.mpImgFolder->AcquireOrCreateSub(gStrBuf, false);
                if (NULL == pImgDstXdlVfsFile)
                {
                    break;
                }
                pImgDstXdl = new BuildFileUser_ImgDstXdl(pImgDstXdlVfsFile, pResult, pProjDep->mpDependOn);
                pImgDstXdlVfsFile->Release();
                if (NULL == pImgDstXdl)
                {
                    break;
                }
                pListLink = pListLink->mpNext;
            } while (NULL != pListLink);
            if (NULL == pListLink)
            {
                ok = true;
            }
        }
        if (!ok)
            break;

    } while (0);

    if (!ok)
    {
        delete pResult;
        pResult = NULL;
    }

    return pResult;
}

BuildFileUser_Img::BuildFileUser_Img(
    VfsFile *               apVfsFile, 
    BuildFileUser_SrcXml *  apSrcXml, 
    BuildFileUser_ImgDstBoot * apImgBoot
) :
    BuildFileUser(apVfsFile, BuildFileUserType_OutObj),
    mpParentSrcXml(apSrcXml),
    mpChildImgBoot(apImgBoot)
{
    mpChildImgBoot->mpParentImg = this;
    K2LIST_Init(&ImgTargetList);
    if (!CheckIfDamaged())
        OnRepaired();
}

BuildFileUser_Img::~BuildFileUser_Img(
    void
)
{
    K2LIST_LINK *               pListLink;
    BuildFileUser_ImgDstXdl *   pImgDstXdl;
    
    pListLink = ImgTargetList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pImgDstXdl = K2_GET_CONTAINER(BuildFileUser_ImgDstXdl, pListLink, ImgTargetListLink);
            pListLink = pListLink->mpNext;
            delete pImgDstXdl;
        } while (NULL != pListLink);
    }

    delete mpChildImgBoot;
}

void
BuildFileUser_Img::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    if (!CheckIfDamaged())
        OnRepaired();
}

void
BuildFileUser_Img::OnFsUpdate(
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
BuildFileUser_Img::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    mpParentSrcXml->OnOutputDamaged();
}

void 
BuildFileUser_Img::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    mpParentSrcXml->OnOutputRepaired();
}

void
BuildFileUser_Img::OnBootDamaged(
    void
)
{
    OnDamaged();
}

void
BuildFileUser_Img::OnBootRepaired(
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

void 
BuildFileUser_Img::OnDepDamaged(
    BuildFileUser_SrcXml *apDep
)
{
    K2LIST_LINK *               pListLink;
    BuildFileUser_ImgDstXdl *   pDstXdl;

    pListLink = ImgTargetList.mpHead;
    if (NULL == pListLink)
        return;

    do {
        pDstXdl = K2_GET_CONTAINER(BuildFileUser_ImgDstXdl, pListLink, ImgTargetListLink);
        if (pDstXdl->mpSrcProjSrcXml == apDep)
        {
            pDstXdl->OnDamaged();
            return;
        }
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);
}

void
BuildFileUser_Img::OnImgDstXdlDamaged(
    BuildFileUser_ImgDstXdl *  apImgXdlDamaged
)
{
    OnDamaged();
}

void
BuildFileUser_Img::OnImgDstXdlRepaired(
    BuildFileUser_ImgDstXdl *  apImgXdlRepaired
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
BuildFileUser_Img::TryRepair(
    void
)
{
    VfsFolder *                 pParentFolder;
    char *                      pParentPath;
    DWORD                       fAttr;
    K2LIST_LINK *               pListLink;
    BuildFileUser_ImgDstXdl *   pImgDstXdl;
    bool                        somethingChanged;
    bool                        anyRepairFailed;
    VfsFolder *                 pRofsSrcFolder;
    char *                      pCmdLine;
    char *                      pEnv;
    UINT_PTR                    blobLen;
    UINT_PTR                    outLen;
    char *                      pOutput;
    int                         cmdResult;
    ProjDep *                   pProjDep;

    if (!BuildFileUser::TryRepair())
        return false;

    if (mpChildImgBoot->IsDamaged())
    {
        return mpChildImgBoot->TryRepair();
    }

    pListLink = mpParentSrcXml->Proj.Img.BuiltInXdlProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mpDependOn->IsDamaged())
                return false;
        } while (NULL != pListLink);
    }
    pListLink = mpParentSrcXml->Proj.Img.RawXdlProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mpDependOn->IsDamaged())
                return false;
        } while (NULL != pListLink);
    }
    pListLink = mpParentSrcXml->Proj.Img.UserXdlProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mpDependOn->IsDamaged())
                return false;
        } while (NULL != pListLink);
    }

    anyRepairFailed = false;
    somethingChanged = false;
    pListLink = ImgTargetList.mpHead;
    do {
        pImgDstXdl = K2_GET_CONTAINER(BuildFileUser_ImgDstXdl, pListLink, ImgTargetListLink);
        pListLink = pListLink->mpNext;
        if (pImgDstXdl->IsDamaged())
        {
            if (!pImgDstXdl->TryRepair())
            {
                anyRepairFailed = true;
            }
            else
            {
                somethingChanged = true;
            }
        }
    } while (NULL != pListLink);

    if (anyRepairFailed)
        return somethingChanged;

    pParentFolder = mpVfsFile->mpParentFolder;
    pParentPath = pParentFolder->AllocFullPath();
    fAttr = GetFileAttributes(pParentPath);
    if (INVALID_FILE_ATTRIBUTES == fAttr)
    {
        if (!CreatePath(pParentPath, pParentFolder->GetFullPathLen()))
        {
            printf("*** Failed to create path for Img [%s]\n", pParentPath);
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

    pRofsSrcFolder = (VfsFolder *)mpParentSrcXml->Proj.Img.mpImgFolder->AcquireOrCreateSub("builtin", true);
    K2_ASSERT(NULL != pRofsSrcFolder);
    
    char *pSrcPath = pRofsSrcFolder->AllocFullPath();
    char *pDstFile = mpVfsFile->AllocFullPath();

    outLen = K2ASC_Printf(gStrBuf, "k2rofs %s %s", pSrcPath, pDstFile);
    pCmdLine = new char[(outLen + 4) & ~3];
    K2_ASSERT(NULL != pCmdLine);
    K2ASC_CopyLen(pCmdLine, gStrBuf, outLen);
    pCmdLine[outLen] = 0;

    //
    // we have our command line
    //
    pEnv = gpBaseEnviron->CopyBlob(&blobLen);
    K2_ASSERT(NULL != pEnv);

    outLen = 0;
    pOutput = NULL;
    //    printf("%s\n", pCmdLine);
    printf("Create Image ROFS [%s]\n", mpParentSrcXml->mpVfsFile->mpParentFolder->mpNameZ);
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
BuildFileUser_Img::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    mpParentSrcXml->SetSomethingChangedSinceLastTryRepair();
}

bool
BuildFileUser_Img::CheckIfDamaged(
    void
)
{
    K2LIST_LINK *               pListLink;
    BuildFileUser_ImgDstXdl *   pImgDstXdl;
    ProjDep *                   pProjDep;

    if (!FileExists())
        return true;

    K2_ASSERT(NULL != mpChildImgBoot);
    if (mpChildImgBoot->IsDamaged())
        return true;
    if (mpVfsFile->IsOlderThan(mpChildImgBoot->mpVfsFile))
        return true;

    pListLink = mpParentSrcXml->Proj.Img.BuiltInXdlProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mpDependOn->IsDamaged())
                return true;
            if (mpVfsFile->IsOlderThan(pProjDep->mpDependOn->Proj.Xdl.mpOutXdl->mpVfsFile))
                return true;
        } while (NULL != pListLink);
    }
    pListLink = mpParentSrcXml->Proj.Img.RawXdlProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mpDependOn->IsDamaged())
                return true;
            if (mpVfsFile->IsOlderThan(pProjDep->mpDependOn->Proj.Xdl.mpOutXdl->mpVfsFile))
                return true;
        } while (NULL != pListLink);
    }
    pListLink = mpParentSrcXml->Proj.Img.UserXdlProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mpDependOn->IsDamaged())
                return true;
            if (mpVfsFile->IsOlderThan(pProjDep->mpDependOn->Proj.Xdl.mpOutXdl->mpVfsFile))
                return true;
        } while (NULL != pListLink);
    }

    pListLink = ImgTargetList.mpHead;
    if (NULL == pListLink)
        return true;    // nothing on the target list (yet)
    do {
        pImgDstXdl = K2_GET_CONTAINER(BuildFileUser_ImgDstXdl, pListLink, ImgTargetListLink);
        pListLink = pListLink->mpNext;
        if (pImgDstXdl->IsDamaged())
            return true;
        if (mpVfsFile->IsOlderThan(pImgDstXdl->mpVfsFile))
            return true;
    } while (NULL != pListLink);

    return false;
}

void
BuildFileUser_Img::Dump(
    bool aDamagedOnly
)
{
    bool isDamaged = IsDamaged();

    if ((aDamagedOnly) && (!isDamaged))
        return;

    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("  IMG %s %s\n", isDamaged ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;

    if (isDamaged)
        mpChildImgBoot->Dump(aDamagedOnly);
}

