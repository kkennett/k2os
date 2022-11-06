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

BuildFileUser_TmpElf *
BuildFileUser_TmpElf::Construct(
    VfsFile *               apVfsFile, 
    BuildFileUser_SrcXml *  apSrcXml, 
    VfsFile *               apInfVfsFile,
    char const *            apXmlFullPath
)
{
    BuildFileUser_TmpElf *      pResult;
    BuildFileUser_TmpExpObj *   pTmpExpObj;
    VfsFile *                   pTmpExpObjVfsFile;

    K2ASC_Printf(gStrBuf, "bld\\tmp\\gcc\\obj\\%s\\%s\\exp_%s.o",
        gpArchMode,
        apSrcXml->mpSubPath,
        apSrcXml->mpVfsFile->mpParentFolder->mpNameZ);

    pTmpExpObjVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(gStrBuf, false);
    if (NULL == pTmpExpObjVfsFile)
    {
        printf("*** Could not acquire tmp export object \"%s\" for [%s]\n", gStrBuf, apXmlFullPath);
        return NULL;
    }

    pTmpExpObj = BuildFileUser_TmpExpObj::Construct(pTmpExpObjVfsFile, apSrcXml, apInfVfsFile, apXmlFullPath);

    pTmpExpObjVfsFile->Release();

    if (NULL != pTmpExpObj)
    {
        pResult = new BuildFileUser_TmpElf(apVfsFile, pTmpExpObj);
        K2_ASSERT(NULL != pResult);
    }
    else
    {
        pResult = NULL;
    }

    return pResult;
}

BuildFileUser_TmpElf::BuildFileUser_TmpElf(
    VfsFile *                   apVfsFile,
    BuildFileUser_TmpExpObj *   apTmpExpObj
) : BuildFileUser(apVfsFile, BuildFileUserType_TmpElf),
    mpChildTmpExpObj(apTmpExpObj)
{
    mpChildTmpExpObj->mpParentTmpElf = this;
    mpParentOutXdlLib = NULL;
    if (!CheckIfDamaged())
        OnRepaired();
}

BuildFileUser_TmpElf::~BuildFileUser_TmpElf(
    void
)
{
    delete mpChildTmpExpObj;
    mpParentOutXdlLib = NULL;
}

void
BuildFileUser_TmpElf::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    if (!CheckIfDamaged())
        OnRepaired();
}

void 
BuildFileUser_TmpElf::OnFsUpdate(
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
BuildFileUser_TmpElf::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    K2_ASSERT(NULL != mpParentOutXdlLib);
    mpParentOutXdlLib->OnDamaged();
}

void 
BuildFileUser_TmpElf::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    K2_ASSERT(NULL != mpParentOutXdlLib);
    if (!mpParentOutXdlLib->CheckIfDamaged())
    {
        mpParentOutXdlLib->OnRepaired();
    }
    else
    {
        mpParentOutXdlLib->SetSomethingChangedSinceLastTryRepair();
    }
}

void
BuildFileUser_TmpElf::OnChildDamaged(
    BuildFileUser_TmpObj *  apChild
)
{
    OnDamaged();
}

void
BuildFileUser_TmpElf::OnChildRepaired(
    BuildFileUser_TmpObj *  apChild
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
BuildFileUser_TmpElf::TryRepair(
    void
)
{
    static char const* const spLinkCmd = "ld %s -e %s -o %s -( %s";
    static char const* const spKernCrtEntry = "__k2oscrt_kern_entry";
    static char const* const spUserCrtEntry = "__k2oscrt_user_entry";
    static char const* const spNormalEntry = "__K2OS_xdl_crt";

    BuildFileUser_SrcXml *  pSrcXml;
    ProjDep *               pProjDep;
    VfsFolder *             pParentFolder;
    char *                  pParentPath;
    DWORD                   fAttr;
    char *                  pOutPath;
    char *                  pExportObjPath;
    char const *            pUseEntry;
    UINT_PTR                curLen;
    char *                  pCmdLine;
    K2LIST_LINK *           pListLink;
    BuildFileUser_TmpObj *  pTmpObj;
    BuildFileUser_OutObj *  pOutObj;
    BuildFileUser_OutLib *  pOutLib;
    BuildFileUser_OutXdl *  pOutXdl;
    char *                  pTemp;
    char *                  pNext;
    char *                  pEnv;
    UINT_PTR                blobLen;
    UINT_PTR                outLen;
    char *                  pOutput;
    int                     cmdResult;

    if (!BuildFileUser::TryRepair())
        return false;

    if (mpChildTmpExpObj->IsDamaged())
    {
        return mpChildTmpExpObj->TryRepair();
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

    pSrcXml = mpParentOutXdlLib->mpParentOutXdl->mpParentSrcXml;

    if (pSrcXml->Proj.Xdl.mIsCrt)
    {
        if (pSrcXml->mIsKernelTarget)
        {
            pUseEntry = spKernCrtEntry;
        }
        else
        {
            pUseEntry = spUserCrtEntry;
        }
    }
    else
    {
        pUseEntry = spNormalEntry;
    }

    pOutPath = mpVfsFile->AllocFullPath();
    curLen = K2ASC_Printf(gStrBuf, spLinkCmd, gpStr_LdOpt, pUseEntry, pOutPath, gpStr_LibGcc);
    delete[] pOutPath;
    pCmdLine = new char[(curLen + 4) & ~3];
    K2_ASSERT(NULL != pCmdLine);
    K2ASC_CopyLen(pCmdLine, gStrBuf, curLen);
    pCmdLine[curLen] = 0;

    //
    // object files
    //
    pListLink = mpChildTmpExpObj->TmpObjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pTmpObj = K2_GET_CONTAINER(BuildFileUser_TmpObj, pListLink, SrcXmlProjOutputTmpObjListLink);
            pListLink = pListLink->mpNext;

            pTemp = pTmpObj->mpVfsFile->AllocFullPath();
            curLen += 1 + pTmpObj->mpVfsFile->GetFullPathLen();
            pNext = new char[(curLen + 4) & ~3];
            K2_ASSERT(NULL != pNext);
            K2ASC_Printf(pNext, "%s %s", pCmdLine, pTemp);
            delete[] pTemp;
            delete[] pCmdLine;
            pCmdLine = pNext;
        } while (NULL != pListLink);
    }

    //
    // object sub projects
    //
    pListLink = pSrcXml->Proj.Xdl.ObjProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            pOutObj = pProjDep->mpDependOn->Proj.Obj.mpOutObj;

            pTemp = pOutObj->mpVfsFile->AllocFullPath(0);
            curLen += 1 + pOutObj->mpVfsFile->GetFullPathLen();
            pNext = new char[(curLen + 4) & ~3];
            K2_ASSERT(NULL != pNext);
            K2ASC_Printf(pNext, "%s %s", pCmdLine, pTemp);
            delete[] pTemp;
            delete[] pCmdLine;
            pCmdLine = pNext;
        } while (NULL != pListLink);
    }

    //
    // library sub projects
    //
    pListLink = pSrcXml->Proj.Xdl.LibProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            pOutLib = pProjDep->mpDependOn->Proj.Lib.mpOutLib;

            pTemp = pOutLib->mpVfsFile->AllocFullPath(0);
            curLen += 1 + pOutLib->mpVfsFile->GetFullPathLen();
            pNext = new char[(curLen + 4) & ~3];
            K2_ASSERT(NULL != pNext);
            K2ASC_Printf(pNext, "%s %s", pCmdLine, pTemp);
            delete[] pTemp;
            delete[] pCmdLine;
            pCmdLine = pNext;
        } while (NULL != pListLink);
    }

    //
    // xdl sub projects
    //
    pListLink = pSrcXml->Proj.Xdl.XdlProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            pOutXdl = pProjDep->mpDependOn->Proj.Xdl.mpOutXdl;

            pTemp = pOutXdl->mpChildOutXdlLib->mpVfsFile->AllocFullPath(0);
            curLen += 1 + pOutXdl->mpChildOutXdlLib->mpVfsFile->GetFullPathLen();
            pNext = new char[(curLen + 4) & ~3];
            K2_ASSERT(NULL != pNext);
            K2ASC_Printf(pNext, "%s %s", pCmdLine, pTemp);
            delete[] pTemp;
            delete[] pCmdLine;
            pCmdLine = pNext;
        } while (NULL != pListLink);
    }

    //
    // place export obj last
    //
    pExportObjPath = mpChildTmpExpObj->mpVfsFile->AllocFullPath();
    curLen += 1 + mpChildTmpExpObj->mpVfsFile->GetFullPathLen() + 1 + 2;
    pNext = new char[(curLen + 4) & ~3];
    K2_ASSERT(NULL != pNext);
    K2ASC_Printf(pNext, "%s %s -)", pCmdLine, pExportObjPath);
    delete[] pCmdLine;
    delete[] pExportObjPath;
    pCmdLine = pNext;

    //
    // we have our command line
    //
    pEnv = gpBaseEnviron->CopyBlob(&blobLen);
    K2_ASSERT(NULL != pEnv);

    outLen = 0;
    pOutput = NULL;
//    printf("%s\n", pCmdLine);
    printf("Link ELF [%s]\n", mpVfsFile->mpNameZ);
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
BuildFileUser_TmpElf::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    mpParentOutXdlLib->SetSomethingChangedSinceLastTryRepair();
}

bool 
BuildFileUser_TmpElf::CheckIfDamaged(
    void
)
{
    if (!mpVfsFile->Exists())
        return true;

    K2_ASSERT(NULL != mpChildTmpExpObj);

    if (mpChildTmpExpObj->IsDamaged())
        return true;

    if (mpVfsFile->IsOlderThan(mpChildTmpExpObj->mpVfsFile))
        return true;

    return false;
}

void 
BuildFileUser_TmpElf::Dump(
    bool aDamagedOnly
)
{
    if ((aDamagedOnly) && (!IsDamaged()))
        return;

    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("      TMPELF %s %s\n", IsDamaged() ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;

    mpChildTmpExpObj->Dump(aDamagedOnly);
}

