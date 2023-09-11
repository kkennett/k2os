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

BuildFileUser_TmpExpObj * 
BuildFileUser_TmpExpObj::Construct(
    VfsFile *               apVfsFile,
    BuildFileUser_SrcXml *  apSrcXml,
    VfsFile *               apInfVfsFile,
    char const *            apXmlFullPath
)
{
    BuildFileUser_TmpExpObj *   pResult;
    BuildFileUser_TmpObj *      pTmpObj;
    K2XML_NODE const *          pXmlNode;
    UINT_PTR                    nodeCount;
    UINT_PTR                    nodeIx;
    char *                      pSrcCodePath;
    VfsFile *                   pSrcCodeVfsFile;
    bool                        ok;
    K2LIST_ANCHOR               tmpObjList;
    BuildFileUser_SrcInf *      pSrcInf;

    pSrcInf = new BuildFileUser_SrcInf(apInfVfsFile);
    K2_ASSERT(NULL != pSrcInf);

    ok = true;
    pXmlNode = NULL;
    pResult = NULL;
    K2LIST_Init(&tmpObjList);
    if (0 != apSrcXml->mSourcesCount)
    {
        nodeCount = apSrcXml->XmlParse.mpRootNode->mNumSubNodes;

        for (nodeIx = 0; nodeIx < nodeCount; nodeIx++)
        {
            pXmlNode = apSrcXml->XmlParse.mpRootNode->mppSubNode[nodeIx];
            if ((6 != pXmlNode->Name.mLen) ||
                (0 != K2ASC_CompInsLen("source", pXmlNode->Name.mpChars, 6)))
                continue;

            if (0 == ((1 << gArch) & GetXmlNodeArchFlags(pXmlNode)))
                continue;

            pSrcCodePath = NULL;
            ok = false;

            if (!GetContentTarget(apSrcXml->mpVfsFile->mpParentFolder, pXmlNode, &pSrcCodePath))
            {
                printf("*** Could not resolve <source> \"%.*s\" in %s [%s]\n",
                    pXmlNode->Content.mLen, pXmlNode->Content.mpChars,
                    gpBuildXmlFileName,
                    apXmlFullPath);
                break;
            }

            pSrcCodeVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(pSrcCodePath + gVfsRootSpecLen + 1, false);

            if (NULL == pSrcCodeVfsFile)
            {
                printf("*** Coult not acquire vfs file \"%s\" in %s [%s]\n",
                    pSrcCodePath + gVfsRootSpecLen + 1, gpBuildXmlFileName, apXmlFullPath);
                break;
            }

            pTmpObj = BuildFileUser_TmpObj::Construct(pSrcCodePath, pSrcCodeVfsFile, apSrcXml, pXmlNode, apXmlFullPath);

            delete[] pSrcCodePath;

            pSrcCodeVfsFile->Release();

            if (NULL == pTmpObj)
                break;

            K2LIST_AddAtTail(&tmpObjList, &pTmpObj->SrcXmlProjOutputTmpObjListLink);

            ok = true;
        }
    }

    if (ok)
    {
        pResult = new BuildFileUser_TmpExpObj(apVfsFile, pSrcInf, &tmpObjList);
        K2_ASSERT(NULL != pResult);
    }
    else
    {
        pResult = NULL;
        if (NULL != tmpObjList.mpHead)
        {
            do {
                pTmpObj = K2_GET_CONTAINER(BuildFileUser_TmpObj, tmpObjList.mpHead, SrcXmlProjOutputTmpObjListLink);
                K2LIST_Remove(&tmpObjList, &pTmpObj->SrcXmlProjOutputTmpObjListLink);
                delete pTmpObj;
            } while (NULL != tmpObjList.mpHead);
        }

        delete pSrcInf;
    }

    return pResult;
}

BuildFileUser_TmpExpObj::BuildFileUser_TmpExpObj(
    VfsFile *               apVfsFile,
    BuildFileUser_SrcInf *  apSrcInf,
    K2LIST_ANCHOR *         apTmpObjList
) : BuildFileUser(apVfsFile, BuildFileUserType_TmpExpObj),
    mpChildSrcInf(apSrcInf)
{
    mpChildSrcInf->mpParentTmpExpObj = this;
    mpParentTmpElf = NULL;
    K2MEM_Copy(&TmpObjList, apTmpObjList, sizeof(K2LIST_ANCHOR));
    if (!CheckIfDamaged())
        OnRepaired();
}

BuildFileUser_TmpExpObj::~BuildFileUser_TmpExpObj(
    void
)
{
    BuildFileUser_TmpObj * pTmpObj;

    if (NULL == TmpObjList.mpHead)
        return;
    do {
        pTmpObj = K2_GET_CONTAINER(BuildFileUser_TmpObj, TmpObjList.mpHead, SrcXmlProjOutputTmpObjListLink);
        K2LIST_Remove(&TmpObjList, &pTmpObj->SrcXmlProjOutputTmpObjListLink);
        delete pTmpObj;
    } while (NULL != TmpObjList.mpHead);

    delete mpChildSrcInf;

    mpParentTmpElf = NULL;
}

void
BuildFileUser_TmpExpObj::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    if (!CheckIfDamaged())
        OnRepaired();
}

void 
BuildFileUser_TmpExpObj::OnFsUpdate(
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
BuildFileUser_TmpExpObj::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    K2_ASSERT(NULL != mpParentTmpElf);
    mpParentTmpElf->OnDamaged();
}

void 
BuildFileUser_TmpExpObj::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    K2_ASSERT(NULL != mpParentTmpElf);
    if (!mpParentTmpElf->CheckIfDamaged())
    {
        mpParentTmpElf->OnRepaired();
    }
    else
    {
        mpParentTmpElf->SetSomethingChangedSinceLastTryRepair();
    }
}

void 
BuildFileUser_TmpExpObj::OnSrcInfDamaged(
    void
)
{
    OnDamaged();
}

void 
BuildFileUser_TmpExpObj::OnSrcInfRepaired(
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
BuildFileUser_TmpExpObj::TryRepair(
    void
)
{
    static char const * const spExportCmd = "k2export -i %s -o %s %s";

    BuildFileUser_SrcXml *  pSrcXml;
    ProjDep *               pProjDep;
    VfsFolder *             pParentFolder;
    char *                  pParentPath;
    DWORD                   fAttr;
    K2LIST_LINK *           pListLink;
    BuildFileUser_TmpObj *  pTmpObj;
    char *                  pInfFile;
    char *                  pOutPath;
    UINT_PTR                curLen;
    char *                  pCmdLine;
    char *                  pTemp;
    char *                  pNext;
    BuildFileUser_OutObj *  pOutObj;
    BuildFileUser_OutLib *  pOutLib;
    BuildFileUser_OutXdl *  pOutXdl;
    char *                  pEnv;
    UINT_PTR                blobLen;
    UINT_PTR                outLen;
    char *                  pOutput;
    int                     cmdResult;
    bool                    atLeastOneObjDidNotRepair;
    bool                    atLeastOneObjDidRepair;

    if (!BuildFileUser::TryRepair())
        return false;

    pListLink = TmpObjList.mpHead;
    K2_ASSERT(NULL != pListLink);

    atLeastOneObjDidRepair = atLeastOneObjDidNotRepair = false;

    do {
        pTmpObj = K2_GET_CONTAINER(BuildFileUser_TmpObj, pListLink, SrcXmlProjOutputTmpObjListLink);
        pListLink = pListLink->mpNext;
        if (pTmpObj->IsDamaged())
        {
            if (!pTmpObj->TryRepair())
            {
                atLeastOneObjDidNotRepair = true;
            }
            else
            {
                atLeastOneObjDidRepair = true;
            }
        }
    } while (NULL != pListLink);

    if (atLeastOneObjDidNotRepair)
        return atLeastOneObjDidRepair;

    pSrcXml = mpParentTmpElf->mpParentOutXdlLib->mpParentOutXdl->mpParentSrcXml;

    pListLink = pSrcXml->Proj.Xdl.ObjProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mpDependOn->IsDamaged())
                return atLeastOneObjDidRepair;
        } while (NULL != pListLink);
    }
    pListLink = pSrcXml->Proj.Xdl.LibProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mpDependOn->IsDamaged())
                return atLeastOneObjDidRepair;
        } while (NULL != pListLink);
    }
    pListLink = pSrcXml->Proj.Xdl.XdlProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mpDependOn->IsDamaged())
                return atLeastOneObjDidRepair;
        } while (NULL != pListLink);
    }

    pParentFolder = mpVfsFile->mpParentFolder;
    pParentPath = pParentFolder->AllocFullPath();
    fAttr = GetFileAttributes(pParentPath);
    if (INVALID_FILE_ATTRIBUTES == fAttr)
    {
        if (!CreatePath(pParentPath, pParentFolder->GetFullPathLen()))
        {
            printf("*** Failed to create path for TmpExpObj [%s]\n", pParentPath);
            delete[] pParentPath;
            return atLeastOneObjDidRepair;
        }
    }
    else if (0 == (fAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        printf("*** Target directory name exists but is not a directory! [%s]\n", pParentPath);
        delete[] pParentPath;
        return atLeastOneObjDidRepair;
    }
    delete[] pParentPath;

    //
    // options for making xdl (if there are any) are in the build xml file
    //

    pInfFile = mpChildSrcInf->mpVfsFile->AllocFullPath();
    pOutPath = mpVfsFile->AllocFullPath(0);
    curLen = K2ASC_Printf(gStrBuf, spExportCmd, pInfFile, pOutPath, gpStr_LibGcc);
    delete[] pInfFile;
    delete[] pOutPath;
    pCmdLine = new char[(curLen + 4) & ~3];
    K2_ASSERT(NULL != pCmdLine);
    K2ASC_CopyLen(pCmdLine, gStrBuf, curLen);
    pCmdLine[curLen] = 0;

    //
    // object files
    //
    pListLink = TmpObjList.mpHead;
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
    // we have our command line
    //
    pEnv = gpBaseEnviron->CopyBlob(&blobLen);
    K2_ASSERT(NULL != pEnv);

    outLen = 0;
    pOutput = NULL;
//    printf("%s\n", pCmdLine);
    printf("Create Export Object [%s]\n", mpVfsFile->mpNameZ);
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
        return atLeastOneObjDidRepair;
    }

    return true;
}

void
BuildFileUser_TmpExpObj::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    mpParentTmpElf->SetSomethingChangedSinceLastTryRepair();
}


bool 
BuildFileUser_TmpExpObj::CheckIfDamaged(
    void
)
{
    BuildFileUser_SrcXml *  pSrcXml;
    K2LIST_LINK *           pListLink;
    BuildFileUser_TmpObj *  pTmpObj;
    ProjDep *               pProjDep;

    if (!FileExists())
        return true;

    if (mpVfsFile->IsOlderThan(mpChildSrcInf->mpVfsFile))
        return true;

    if (NULL == mpParentTmpElf)
        return true;

    pSrcXml = mpParentTmpElf->mpParentOutXdlLib->mpParentOutXdl->mpParentSrcXml;

    pListLink = TmpObjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pTmpObj = K2_GET_CONTAINER(BuildFileUser_TmpObj, pListLink, SrcXmlProjOutputTmpObjListLink);
            pListLink = pListLink->mpNext;
            if (pTmpObj->IsDamaged())
                return true;
            if (mpVfsFile->IsOlderThan(pTmpObj->mpVfsFile))
                return true;
        } while (NULL != pListLink);
    }

    pListLink = pSrcXml->Proj.Xdl.ObjProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mpDependOn->IsDamaged())
                return true;
            if (mpVfsFile->IsOlderThan(pProjDep->mpDependOn->Proj.Obj.mpOutObj->mpVfsFile))
                return true;
        } while (NULL != pListLink);
    }

    pListLink = pSrcXml->Proj.Xdl.LibProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mpDependOn->IsDamaged())
                return true;
            if (mpVfsFile->IsOlderThan(pProjDep->mpDependOn->Proj.Lib.mpOutLib->mpVfsFile))
                return true;
        } while (NULL != pListLink);
    }

    pListLink = pSrcXml->Proj.Xdl.XdlProjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pProjDep = K2_GET_CONTAINER(ProjDep, pListLink, OwnerListLink);
            pListLink = pListLink->mpNext;
            if (pProjDep->mpDependOn->IsDamaged())
                return true;
            if (mpVfsFile->IsOlderThan(pProjDep->mpDependOn->Proj.Xdl.mpOutXdl->mpChildOutXdlLib->mpVfsFile))
                return true;
        } while (NULL != pListLink);
    }

    return false;
}

void 
BuildFileUser_TmpExpObj::Dump(
    bool aDamagedOnly
)
{
    K2LIST_LINK *           pListLink;
    BuildFileUser_TmpObj *  pTmpObj;

    if ((aDamagedOnly) && (!IsDamaged()))
        return;

    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("        TMPEXPOBJ %s %s\n", IsDamaged() ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;

    pListLink = TmpObjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pTmpObj = K2_GET_CONTAINER(BuildFileUser_TmpObj, pListLink, SrcXmlProjOutputTmpObjListLink);
            pListLink = pListLink->mpNext;
            pTmpObj->Dump(aDamagedOnly);
        } while (NULL != pListLink);
    }
}

