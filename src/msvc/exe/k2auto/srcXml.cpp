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

static bool sSrcXmlListInit = false;
K2LIST_ANCHOR BuildFileUser_SrcXml::sSrcXmlList;

static char const * const sgpProjectTypeStr[ProjectType_Count] =
{
    "Invalid", "Obj", "Lib", "Xdl", "Img"
};

BuildFileUser_SrcXml::BuildFileUser_SrcXml(
    VfsFile *   apVfsFile, 
    char *      apSubPath, 
    UINT_PTR    aSubPathLen
) :
    BuildFileUser(apVfsFile, BuildFileUserType_SrcXml),
    mpSubPath(apSubPath),
    mSubPathLen(aSubPathLen)
{
    if (!sSrcXmlListInit)
    {
        K2LIST_Init(&sSrcXmlList);
        sSrcXmlListInit = true;
    }
    mpFileData = NULL;
    mFileBytes = 0;
    mParseOk = false;
    mIsKernelTarget = false;
    mEventIter = 0;
    K2MEM_Zero(&XmlParse, sizeof(XmlParse));
    K2LIST_Init(&ProjDepList);
    K2MEM_Zero(&Proj, sizeof(Proj));
    mProjectType = ProjectType_Invalid;
    K2LIST_AddAtTail(&sSrcXmlList, &SrcXmlListLink);
    mRefCount = 1;
}

BuildFileUser_SrcXml::~BuildFileUser_SrcXml(void)
{
    K2_ASSERT(0 == mRefCount);
    Destruct();
    K2LIST_Remove(&sSrcXmlList, &SrcXmlListLink);
}

void
BuildFileUser_SrcXml::OnFsCreate(
    char const *apFullPath,
    bool *      apRetArchMismatch
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    Build(apFullPath, apRetArchMismatch);
}

void
BuildFileUser_SrcXml::OnFsUpdate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsUpdate(apFullPath);
    OnDamaged();
    Destruct();
    Build(apFullPath, NULL);
}

void
BuildFileUser_SrcXml::OnFsDelete(
    char const *apFullPath
)
{
    BuildFileUser::OnFsDelete(apFullPath);
    OnDamaged();
    Destruct();
}

void
BuildFileUser_SrcXml::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    OnChange(true);
}

void
BuildFileUser_SrcXml::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    OnChange(false);
}

bool
BuildFileUser_SrcXml::TryRepair(
    void
)
{
    if (!BuildFileUser::TryRepair())
    {
        printf("%s: no change\n", __FUNCTION__);
        return false;
    }

    if (ProjectType_Invalid == mProjectType)
    {
        printf("%s: XML did not load or did not parse correctly. \n", __FUNCTION__);
        return false;
    }

    switch (mProjectType)
    {
    case ProjectType_Obj:
        return Proj.Obj.mpOutObj->TryRepair();

    case ProjectType_Lib:
        return Proj.Lib.mpOutLib->TryRepair();

    case ProjectType_Xdl:
        return Proj.Xdl.mpOutXdl->TryRepair();

    case ProjectType_Img:
        return Proj.Img.mpImg->TryRepair();

    default:
        break;
    }

    return false;
}

void
BuildFileUser_SrcXml::AddRef(
    void
)
{
    K2ATOMIC_Inc((INT_PTR volatile *)&mRefCount);
}

void
BuildFileUser_SrcXml::Release(
    void
)
{
    UINT_PTR refs;

    refs = K2ATOMIC_Dec((INT_PTR volatile *)&mRefCount);

    if (0 != refs)
        return;

    delete this;
}

bool
BuildFileUser_SrcXml::CheckIfDamaged(
    void
)
{
    BuildFileUser * pOutputUser;

    if (ProjectType_Invalid == mProjectType)
        return true;

    if (!mpVfsFile->Exists())
        return true;

    pOutputUser = NULL;

    switch (mProjectType)
    {
    case ProjectType_Obj:
        pOutputUser = Proj.Obj.mpOutObj;
        break;
    case ProjectType_Lib:
        pOutputUser = Proj.Lib.mpOutLib;
        break;
    case ProjectType_Xdl:
        pOutputUser = Proj.Xdl.mpOutXdl;
        break;
    case ProjectType_Img:
        pOutputUser = Proj.Img.mpImg;
        break;
    default:
        return true;
    }

    if (!pOutputUser->mpVfsFile->Exists())
        return true;

    if (pOutputUser->IsDamaged())
        return true;

    if (mpVfsFile->IsNewerThan(pOutputUser->mpVfsFile))
        return true;

    return false;
}

void
BuildFileUser_SrcXml::OnDepDamaged(
    BuildFileUser_SrcXml * apDamagedDep
)
{
    switch (mProjectType)
    {
    case ProjectType_Obj:
        // this should never have any project dependencies
        K2_ASSERT(0);
        break;

    case ProjectType_Lib:
        K2_ASSERT(ProjectType_Obj == apDamagedDep->mProjectType);
        Proj.Lib.mpOutLib->OnDamaged();
        break;

    case ProjectType_Xdl:
        Proj.Xdl.mpOutXdl->mpChildOutXdlLib->mpChildTmpElf->mpChildTmpExpObj->OnDamaged();
        break;

    case ProjectType_Img:
        Proj.Img.mpImg->OnDepDamaged(apDamagedDep);
        break;

    default:
        K2_ASSERT(0);
        break;
    }
}

void
BuildFileUser_SrcXml::OnDepRepaired(
    BuildFileUser_SrcXml * apRepairedDep
)
{
    if (!CheckIfDamaged())
    {
        OnRepaired();
    }
    else
    {
        switch (mProjectType)
        {
        case ProjectType_Obj:
            // this should never have any project dependencies
            K2_ASSERT(0);
            break;

        case ProjectType_Lib:
            K2_ASSERT(ProjectType_Obj == apRepairedDep->mProjectType);
            Proj.Lib.mpOutLib->SetSomethingChangedSinceLastTryRepair();
            break;

        case ProjectType_Xdl:
            Proj.Xdl.mpOutXdl->mpChildOutXdlLib->mpChildTmpElf->mpChildTmpExpObj->SetSomethingChangedSinceLastTryRepair();
            break;

        case ProjectType_Img:
            Proj.Img.mpImg->mpChildImgBoot->SetSomethingChangedSinceLastTryRepair();
            break;

        default:
            K2_ASSERT(0);
            break;
        }
    }
}

void 
BuildFileUser_SrcXml::OnOutputDamaged(
    void
)
{
    OnDamaged();
}

void 
BuildFileUser_SrcXml::OnOutputRepaired(
    void
)
{
    OnRepaired();
}

void
BuildFileUser_SrcXml::Dump(
    bool aDamagedOnly
)
{
    K2LIST_LINK *           pListLink;
    ProjDep *               pProjDep;
    BuildFileUser_SrcXml *  pSubProj;

    if ((aDamagedOnly) && (!IsDamaged()))
        return;

    if (!aDamagedOnly)
    {
        printf("Project [%s]\n", mpSubPath);
    }

    if (mProjectType == ProjectType_Obj)
    {
        Proj.Obj.mpOutObj->Dump(aDamagedOnly);
        return;
    }
    if (mProjectType == ProjectType_Lib)
    {
        pListLink = Proj.Lib.ObjProjList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
                pListLink = pListLink->mpNext;
                pSubProj = pProjDep->mpDependOn;
                if ((!aDamagedOnly) || (pProjDep->mpDependOn->IsDamaged()))
                {
                    printf("  SUB OBJ %s %s\n", pProjDep->mpDependOn->IsDamaged() ? "DAMG" : "GOOD", pProjDep->mpDependOn->mpSubPath);
                }
            } while (NULL != pListLink);
        }
        Proj.Lib.mpOutLib->Dump(aDamagedOnly);
        return;
    }
    if (mProjectType == ProjectType_Xdl)
    {
        pListLink = Proj.Xdl.ObjProjList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
                pListLink = pListLink->mpNext;
                pSubProj = pProjDep->mpDependOn;
                if ((!aDamagedOnly) || (pProjDep->mpDependOn->IsDamaged()))
                {
                    printf("  SUB OBJ %s %s\n", pProjDep->mpDependOn->IsDamaged() ? "DAMG" : "GOOD", pProjDep->mpDependOn->mpSubPath);
                }
            } while (NULL != pListLink);
        }
        pListLink = Proj.Xdl.LibProjList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
                pListLink = pListLink->mpNext;
                pSubProj = pProjDep->mpDependOn;
                if ((!aDamagedOnly) || (pProjDep->mpDependOn->IsDamaged()))
                {
                    printf("  SUB LIB %s %s\n", pProjDep->mpDependOn->IsDamaged() ? "DAMG" : "GOOD", pProjDep->mpDependOn->mpSubPath);
                }
            } while (NULL != pListLink);
        }
        pListLink = Proj.Xdl.XdlProjList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
                pListLink = pListLink->mpNext;
                pSubProj = pProjDep->mpDependOn;
                if ((!aDamagedOnly) || (pProjDep->mpDependOn->IsDamaged()))
                {
                    printf("  SUB XDL %s %s\n", pProjDep->mpDependOn->IsDamaged() ? "DAMG" : "GOOD", pProjDep->mpDependOn->mpSubPath);
                }
            } while (NULL != pListLink);
        }
        Proj.Xdl.mpOutXdl->Dump(aDamagedOnly);
        return;
    }
    else if (ProjectType_Invalid != mProjectType)
    {
        pListLink = Proj.Img.RawXdlProjList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
                pListLink = pListLink->mpNext;
                pSubProj = pProjDep->mpDependOn;
                if ((!aDamagedOnly) || (pProjDep->mpDependOn->IsDamaged()))
                {
                    printf("  SUB RAW XDL %s %s\n", pProjDep->mpDependOn->IsDamaged() ? "DAMG" : "GOOD", pProjDep->mpDependOn->mpSubPath);
                }
            } while (NULL != pListLink);
        }
        pListLink = Proj.Img.BuiltInXdlProjList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
                pListLink = pListLink->mpNext;
                pSubProj = pProjDep->mpDependOn;
                if ((!aDamagedOnly) || (pProjDep->mpDependOn->IsDamaged()))
                {
                    printf("  SUB BIN XDL %s %s\n", pProjDep->mpDependOn->IsDamaged() ? "DAMG" : "GOOD", pProjDep->mpDependOn->mpSubPath);
                }
            } while (NULL != pListLink);
        }
        pListLink = Proj.Img.UserXdlProjList.mpHead;
        if (NULL != pListLink)
        {
            do {
                pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
                pListLink = pListLink->mpNext;
                pSubProj = pProjDep->mpDependOn;
                if ((!aDamagedOnly) || (pProjDep->mpDependOn->IsDamaged()))
                {
                    printf("  SUB USR XDL %s %s\n", pProjDep->mpDependOn->IsDamaged() ? "DAMG" : "GOOD", pProjDep->mpDependOn->mpSubPath);
                }
            } while (NULL != pListLink);
        }
        Proj.Img.mpImg->Dump(aDamagedOnly);
    }
}

void
BuildFileUser_SrcXml::OnChange(
    bool aDamaged
)
{
    BuildFileUser_SrcXml *  pSrcXml;
    K2LIST_LINK *           pListLink;
    ProjDep *               pProjDep;

    SetSomethingChangedSinceLastTryRepair();

    pListLink = ProjDepList.mpHead;
    if (NULL == pListLink)
        return;

    ++mEventIter;

    do {
        pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, ProjDepListLink);
        pProjDep->mEventIter = mEventIter;
        pListLink = pListLink->mpNext;
    } while (NULL != pListLink);

    do {
        pListLink = ProjDepList.mpHead;
        if (NULL == pListLink)
            break;
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, ProjDepListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mEventIter == mEventIter)
            {
                pSrcXml = pProjDep->mpOwner;
                if (aDamaged)
                {
                    pSrcXml->OnDepDamaged(this);
                }
                else
                {
                    pSrcXml->OnDepRepaired(this);
                }
                pProjDep->mEventIter = 0;
                break;
            }
        } while (NULL != pListLink);
    } while (NULL != pListLink);
}

void
BuildFileUser_SrcXml::Construct(
    char const *apFullPath,
    bool *      apRetArchMismatch
)
{
    K2ReadOnlyMappedFile *  pFile;
    K2STAT                  stat;
    UINT_PTR                ixType;
    K2XML_STR *             pStr;
    char const *            pPars;
    UINT_PTR                left;
    UINT_PTR                ixSubNode;
    K2XML_NODE const *      pSubNode;

    K2_ASSERT(ProjectType_Invalid == mProjectType);
    K2_ASSERT(NULL == XmlParse.mpRootNode);
    K2_ASSERT(NULL == mpFileData);
    K2_ASSERT(0 == mFileBytes);
    K2_ASSERT(!mParseOk);
    K2_ASSERT(K2MEM_VerifyZero(&Proj, sizeof(Proj)));
    K2_ASSERT(0 == mSourcesCount);

    if (NULL != apRetArchMismatch)
    {
        *apRetArchMismatch = false;
    }

    pFile = K2ReadOnlyMappedFile::Create(apFullPath);
    if (NULL == pFile)
    {
        printf("*** XML File open failure: [%s]\n", apFullPath);
        return;
    }

    mFileBytes = (UINT_PTR)pFile->FileBytes();
    if (0 != mFileBytes)
    {
        mpFileData = new char[(mFileBytes + 4) & ~3];
        K2_ASSERT(NULL != mpFileData);
        K2MEM_Copy(mpFileData, pFile->DataPtr(), mFileBytes);
        mpFileData[mFileBytes] = 0;
    }

    delete pFile;

    if (NULL == mpFileData)
        return;

    do {
        stat = K2XML_Parse(malloc, free, mpFileData, mFileBytes, &XmlParse);
        if (K2STAT_IS_ERROR(stat))
        {
            printf("*** XML Parse Failure (%08X): [%s]\n", stat, apFullPath);
            break;
        }

        do {
            if (NULL == XmlParse.mpRootNode)
                break;

            if ((XmlParse.mpRootNode->Name.mLen != 7) ||
                (0 != K2ASC_CompInsLen(XmlParse.mpRootNode->Name.mpChars, "k2build", 7)))
            {
                printf("*** XML file has wrong root node name [%s]\n", apFullPath);
                break;
            }

            if (0 == ((1 << gArch) & GetXmlNodeArchFlags(XmlParse.mpRootNode)))
            {
                if (NULL != apRetArchMismatch)
                {
                    *apRetArchMismatch = true;
                }
                break;
            }

            if (!K2XML_FindAttrib(XmlParse.mpRootNode, "type", &ixType))
            {
                printf("*** XML <k2build> root node has no 'type' attribute [%s]\n", apFullPath);
                break;
            }

            pStr = &XmlParse.mpRootNode->mppAttrib[ixType]->Value;
            pPars = pStr->mpChars;
            left = pStr->mLen;
            K2PARSE_EatWhitespace(&pPars, &left);
            K2PARSE_EatWhitespaceAtEnd(pPars, &left);

            mProjectType = ProjectType_Invalid;

            if (left == 3)
            {
                if (0 == K2ASC_CompInsLen(pPars, sgpProjectTypeStr[ProjectType_Obj], 3))
                {
                    mProjectType = ProjectType_Obj;
                }
                else if (0 == K2ASC_CompInsLen(pPars, sgpProjectTypeStr[ProjectType_Lib], 3))
                {
                    mProjectType = ProjectType_Lib;
                }
                else if (0 == K2ASC_CompInsLen(pPars, sgpProjectTypeStr[ProjectType_Xdl], 3))
                {
                    mProjectType = ProjectType_Xdl;
                }
                else if (0 == K2ASC_CompInsLen(pPars, sgpProjectTypeStr[ProjectType_Img], 3))
                {
                    mProjectType = ProjectType_Img;
                }
            }

            if (ProjectType_Invalid == mProjectType)
            {
                printf("*** XML has no valid type attribute [%s]\n", apFullPath);
                break;
            }

            mIsKernelTarget = false;
            for (ixSubNode = 0; ixSubNode < XmlParse.mpRootNode->mNumSubNodes; ixSubNode++)
            {
                pSubNode = XmlParse.mpRootNode->mppSubNode[ixSubNode];
                if ((pSubNode->Name.mLen == 6) &&
                    (0 == K2ASC_CompInsLen("kernel", pSubNode->Name.mpChars, 6)))
                {
                    mIsKernelTarget = true;
                    break;
                }
            }

            //
            // any project can have sources
            //
            if (!EnumSources(apFullPath))
                break;

            switch (mProjectType)
            {
            case ProjectType_Obj:
                mParseOk = Construct_Obj(apFullPath);
                break;
            case ProjectType_Lib:
                mParseOk = Construct_Lib(apFullPath);
                break;
            case ProjectType_Xdl:
                mParseOk = Construct_Xdl(apFullPath);
                break;
            case ProjectType_Img:
                mParseOk = Construct_Img(apFullPath);
                break;
            }

        } while (0);

        if (!mParseOk)
        {
            K2XML_Purge(&XmlParse);
        }

    } while (0);

    if (!mParseOk)
    {
        delete[] mpFileData;
        mpFileData = NULL;
        mFileBytes = 0;
        mProjectType = ProjectType_Invalid;
        K2_ASSERT(K2MEM_VerifyZero(&Proj, sizeof(Proj)));
        BuildFileUser::TryRepair(); // dont try again until something changes
    }
    else
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
}

void
BuildFileUser_SrcXml::Build(
    char const *    apFullPath,
    bool *          apRetArchMismatch
)
{
    bool wasDamaged;

    wasDamaged = IsDamaged();

    Construct(apFullPath, apRetArchMismatch);

    if (CheckIfDamaged() != wasDamaged)
    {
        if (wasDamaged)
        {
            OnRepaired();
        }
        else
        {
            OnDamaged();
        }
    }
}

void
BuildFileUser_SrcXml::PurgeProjDepList(
    K2LIST_ANCHOR *apList
)
{
    K2LIST_LINK *   pListLink;
    ProjDep *       pProjDep;

    pListLink = apList->mpHead;
    if (NULL == pListLink)
        return;
    do {
        pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
        pListLink = pListLink->mpNext;
        delete pProjDep;
    } while (NULL != pListLink);
}

void 
BuildFileUser_SrcXml::Destruct(
    void
)
{
    switch (mProjectType)
    {
    case ProjectType_Obj:
        Destruct_Obj();
        break;

    case ProjectType_Lib:
        Destruct_Lib();

        break;
    case ProjectType_Xdl:
        Destruct_Xdl();
        break;

    case ProjectType_Img:
        Destruct_Img();
        break;
    }

    K2XML_Purge(&XmlParse);

    if (NULL != mpFileData)
    {
        K2_ASSERT(0 != mFileBytes);
        delete[] mpFileData;
        mpFileData = NULL;
        mFileBytes = 0;
    }
    else
    {
        K2_ASSERT(0 == mFileBytes);
    }

    mIsKernelTarget = false;
    mEventIter = 0;
    mParseOk = false;
    SetSomethingChangedSinceLastTryRepair();
    mProjectType = ProjectType_Invalid;
    mSourcesCount = 0;
}

bool
BuildFileUser_SrcXml::EnumSources(
    char const *apFullPath
)
{
    UINT_PTR                ixSubNode;
    UINT_PTR                nodeCount;
    K2XML_NODE const *      pSubNode;
    char *                  pSrcPath;
    VfsFile *               pSrcCodeVfsFile;

    nodeCount = XmlParse.mpRootNode->mNumSubNodes;
    for (ixSubNode = 0; ixSubNode < nodeCount; ixSubNode++)
    {
        pSubNode = XmlParse.mpRootNode->mppSubNode[ixSubNode];
        if ((pSubNode->Name.mLen != 6) ||
            (0 != K2ASC_CompInsLen("source", pSubNode->Name.mpChars, 6)))
            continue;

        if (0 == ((1 << gArch) & GetXmlNodeArchFlags(pSubNode)))
            continue;

        if (!GetContentTarget(mpVfsFile->mpParentFolder, pSubNode, &pSrcPath))
        {
            printf("*** XML has unresolvable <source> \"%.*s\" in [%s]\n", pSubNode->Content.mLen, pSubNode->Content.mpChars, apFullPath);
            break;
        }

        pSrcCodeVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(pSrcPath + gVfsRootSpecLen + 1, false);
        if (NULL == pSrcCodeVfsFile)
        {
            printf("*** XML specifies <source> file \"%s\" that could not be acquired [%s]\n", gStrBuf, apFullPath);
            delete[] pSrcPath;
            break;
        }

        ++mSourcesCount;

        pSrcCodeVfsFile->Release();
    }

    if (ixSubNode != nodeCount)
        return false;

    return true;
}

ProjDep *
BuildFileUser_SrcXml::AddProjectByPath(
    char const *        apFullPath,
    char const *        apTargetSubPath,
    ProjectType         aProjectType,
    K2LIST_ANCHOR *     apList
)
{
    VfsFolder *             pVfsFolder;
    VfsFile *               pVfsFile;
    ProjDep *               pProjDep;
    BuildFileUser_SrcXml *  pSrcXml;
    UINT_PTR                userCount;

    pVfsFolder = (VfsFolder *)VfsFolder::Root().AcquireOrCreateSub(apTargetSubPath, true);
    if (NULL == pVfsFolder)
    {
        printf("*** Could not resolve folder \"%s\" for project [%s]\n", apTargetSubPath, apFullPath);
        return NULL;
    }

    pVfsFile = (VfsFile *)pVfsFolder->AcquireOrCreateSub(gpBuildXmlFileName, false);
    if (NULL == pVfsFile)
    {
        printf("*** Could not resolve vfs %s inside \"%s\" for project [%s]\n", gpBuildXmlFileName, apTargetSubPath, apFullPath);
        pVfsFolder->Release();
        return NULL;
    }

    userCount = pVfsFile->GetUserCount();
    if (0 == userCount)
    {
        UINT_PTR subPathLen = pVfsFolder->GetFullPathLen() - (gVfsRootSpecLen + 1);
        char *pSubPath = new char[(subPathLen + 4) & ~3];
        K2_ASSERT(NULL != pSubPath);
        K2ASC_CopyLen(pSubPath, apTargetSubPath, subPathLen);
        pSubPath[subPathLen] = 0;
        pSrcXml = new BuildFileUser_SrcXml(pVfsFile, pSubPath, subPathLen);
        K2_ASSERT(NULL != pSrcXml);
    }
    else
    {
        K2_ASSERT(1 == userCount);
        pSrcXml = (BuildFileUser_SrcXml *)pVfsFile->GetUserByIndex(0);
        K2_ASSERT(BuildFileUserType_SrcXml == pSrcXml->mFileType);

        if (pSrcXml->mProjectType != ProjectType_Invalid)
        {
            if (pSrcXml->mProjectType != aProjectType)
            {
                printf("*** XML specified subproject \"%s\" as %s but it is %s [%s]\n",
                    apTargetSubPath,
                    sgpProjectTypeStr[aProjectType], sgpProjectTypeStr[pSrcXml->mProjectType],
                    apFullPath);
                pSrcXml = NULL;
            }
        }

        if (NULL != pSrcXml)
        {
            pSrcXml->AddRef();
        }
    }

    pVfsFile->Release();

    pVfsFolder->Release();

    if (NULL == pSrcXml)
        return NULL;

    pProjDep = new ProjDep(this, apList, pSrcXml);
    K2_ASSERT(NULL != pProjDep);

    pSrcXml->Release();

    return pProjDep;
}

ProjDep *
BuildFileUser_SrcXml::AddProject(
    char const *        apFullPath,
    K2XML_NODE const *  apXmlNode,
    ProjectType         aProjectType,
    K2LIST_ANCHOR *     apList
)
{
    ProjDep *   pResult;
    char *      pSrcFolderPath;

    if (!GetContentTarget(mpVfsFile->mpParentFolder, apXmlNode, &pSrcFolderPath))
    {
        printf("*** XML specified bad subproject target \"%.*s\" [%s]\n",
            apXmlNode->Content.mLen, apXmlNode->Content.mpChars, apFullPath);
        return NULL;
    }

    pResult = AddProjectByPath(apFullPath, pSrcFolderPath + gVfsRootSpecLen + 1, aProjectType, apList);

    delete[] pSrcFolderPath;

    if (NULL != pResult)
    {
        pResult->mpOwnerXmlNode = apXmlNode;
    }

    return pResult;
}

bool
BuildFileUser_SrcXml::Eval(
    void
)
{
    static bool informedOfDamage = false;
    K2LIST_LINK *           pListLink;
    BuildFileUser_SrcXml *  pSrcXml;
    bool                    result;
    bool                    triedRepair;
    bool                    atLeastOneDamaged;
    BuildFileUser_SrcCode * pSrcCode;

    printf("\n*\n");
    if (!sSrcXmlListInit)
        return false;

    //
    // set something changed in all source files that do not have an
    // existing dep file.  This ensures that source code files that are broken
    // and have no dep file generated get retried on each file system change
    //
    pListLink = gSourceFileList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pSrcCode = K2_GET_CONTAINER(BuildFileUser_SrcCode, pListLink, SrcFileListLink);
            pListLink = pListLink->mpNext;
            if (!pSrcCode->mpParentTmpDep->mpVfsFile->Exists())
                pSrcCode->SetSomethingChangedSinceLastTryRepair();
        } while (NULL != pListLink);
    }

    pListLink = sSrcXmlList.mpHead;
    if (NULL == pListLink)
        return false;

    atLeastOneDamaged = triedRepair = result = false;

    do {
        pSrcXml = K2_GET_CONTAINER(BuildFileUser_SrcXml, pListLink, SrcXmlListLink);
        if (pSrcXml->IsDamaged())
        {
            atLeastOneDamaged = true;
            informedOfDamage = false;
            if (pSrcXml->GetSomethingChangedSinceLastTryRepair())
            {
                if (ProjectType_Invalid == pSrcXml->mProjectType)
                {
                    printf("Project is Invalid [%s]\n", pSrcXml->mpSubPath);
                }
                else
                {
                    triedRepair = true;
                    printf("Try Repair [%s]\n", pSrcXml->mpSubPath);
//                    pSrcXml->Dump(false);
                    if (pSrcXml->TryRepair())
                    {
                        result = true;
                        break;
                    }
                }
            }
        }
        pListLink = pListLink->mpNext;

    } while (NULL != pListLink);

    if (NULL == pListLink)
    {
        //
        // hit the end of the list without doing anything
        //
        if (atLeastOneDamaged)
        {
            if (!triedRepair)
            {
                printf("\nAt least one project is damaged but no action could be taken\n");
            }
            else
            {
                printf("\nAt least one project is damaged and could not be repaired.\n");
            }
            pListLink = sSrcXmlList.mpHead;
            do {
                pSrcXml = K2_GET_CONTAINER(BuildFileUser_SrcXml, pListLink, SrcXmlListLink);
                pListLink = pListLink->mpNext;
                if (pSrcXml->IsDamaged())
                {
                    printf("%s DAMG: %s\n", sgpProjectTypeStr[pSrcXml->mProjectType], pSrcXml->mpSubPath);
                    pSrcXml->Dump(true);
                }
            } while (NULL != pListLink);
        }
        else
        {
            if (!informedOfDamage)
            {
                printf("\n ALL PROJECTS OK\n");
                informedOfDamage = true;
            }
        }
    }

    return result;
}

void
BuildFileUser_SrcXml::DiscoveredNew(
    char const *    apFullPath,
    char const *    apNameOnly
)
{
    VfsFile *               pVfsFile;
    BuildFileUser_SrcXml *  pSrcXml;
    char *                  pSubPath;
    UINT_PTR                subPathLen;
    UINT_PTR                userCount;
    bool                    archMismatch;

    if (0 != K2ASC_CompIns(apNameOnly, "k2build.xml"))
        return;

    pVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(apFullPath + gVfsRootSpecLen + 1, false);
    if (NULL == pVfsFile)
    {
        printf("*** Discovered file but could not acquire it! [%s]\n", apFullPath);
        return;
    }

//    printf("Discover[%s]\n", apFullPath + gVfsRootSpecLen + 1);

    userCount = pVfsFile->GetUserCount();
    if (0 == userCount)
    {
        subPathLen = pVfsFile->mpParentFolder->GetFullPathLen() - (gVfsRootSpecLen + 1);
        pSubPath = new char[(subPathLen + 4) & ~3];
        K2_ASSERT(NULL != pSubPath);
        K2ASC_CopyLen(pSubPath, apFullPath + gVfsRootSpecLen + 1, subPathLen);
        pSubPath[subPathLen] = 0;

        pSrcXml = new BuildFileUser_SrcXml(pVfsFile, pSubPath, subPathLen);
        K2_ASSERT(NULL != pSrcXml);
    }
    else
    {
        K2_ASSERT(1 == userCount);
        pSrcXml = (BuildFileUser_SrcXml *)pVfsFile->GetUserByIndex(0);
    }

    archMismatch = false;
    pSrcXml->OnFsCreate(apFullPath, &archMismatch);
    if (archMismatch)
    {
        pSrcXml->Release();
    }

    pVfsFile->Release();
}

