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

BuildFileUser_OutLib *
BuildFileUser_OutLib::Construct(
    VfsFile *               apVfsFile,
    BuildFileUser_SrcXml *  apSrcXml,
    char const *            apXmlFullPath
)
{
    BuildFileUser_OutLib *  pResult;
    BuildFileUser_TmpObj *  pTmpObj;
    K2XML_NODE const *      pXmlNode;
    UINT_PTR                nodeCount;
    UINT_PTR                nodeIx;
    char *                  pSrcCodePath;
    VfsFile *               pSrcCodeVfsFile;
    bool                    ok;
    K2LIST_ANCHOR           tmpObjList;

    K2_ASSERT(0 != apSrcXml->mSourcesCount);
    K2_ASSERT(NULL != apSrcXml->XmlParse.mpRootNode);

    nodeCount = apSrcXml->XmlParse.mpRootNode->mNumSubNodes;
    pXmlNode = NULL;
    ok = true;
    K2LIST_Init(&tmpObjList);

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

    if (ok)
    {
        pResult = new BuildFileUser_OutLib(apVfsFile, apSrcXml, &tmpObjList);
        K2_ASSERT(NULL != pResult);
    }
    else
    {
        if (NULL != tmpObjList.mpHead)
        {
            do {
                pTmpObj = K2_GET_CONTAINER(BuildFileUser_TmpObj, tmpObjList.mpHead, SrcXmlProjOutputTmpObjListLink);
                K2LIST_Remove(&tmpObjList, &pTmpObj->SrcXmlProjOutputTmpObjListLink);
                delete pTmpObj;
            } while (NULL != tmpObjList.mpHead);
        }

        return NULL;
    }

    return pResult;
}


BuildFileUser_OutLib::BuildFileUser_OutLib(
    VfsFile *               apVfsFile,
    BuildFileUser_SrcXml *  apSrcXml,
    K2LIST_ANCHOR *         apTmpObjList
) :
    BuildFileUser(apVfsFile, BuildFileUserType_OutLib),
    mpParentSrcXml(apSrcXml)
{
    K2MEM_Copy(&TmpObjList, apTmpObjList, sizeof(K2LIST_ANCHOR));
    if (!CheckIfDamaged())
        OnRepaired();
}

BuildFileUser_OutLib::~BuildFileUser_OutLib(
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
}

void
BuildFileUser_OutLib::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    if (!CheckIfDamaged())
        OnRepaired();
}

void
BuildFileUser_OutLib::OnFsUpdate(
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
BuildFileUser_OutLib::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    mpParentSrcXml->OnOutputDamaged();
}

void 
BuildFileUser_OutLib::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    mpParentSrcXml->OnOutputRepaired();
}

void
BuildFileUser_OutLib::OnChildDamaged(
    BuildFileUser_TmpObj *  apChild
)
{
    OnDamaged();
}

void
BuildFileUser_OutLib::OnChildRepaired(
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
BuildFileUser_OutLib::TryRepair(
    void
)
{
    static char const * const spCmdSpec = "ar rco %s";

    VfsFolder *             pParentFolder;
    char *                  pDst;
    char *                  pParentPath;
    DWORD                   fAttr;
    K2LIST_LINK *           pListLink;
    BuildFileUser_TmpObj *  pTmpObj;
    char *                  pFullPath;
    char *                  pTemp;
    char *                  pNext;
    UINT_PTR                curLen;
    char *                  pEnv;
    UINT_PTR                blobLen;
    UINT_PTR                outLen;
    char *                  pOutput;
    int                     cmdResult;
    ProjDep *               pProjDep;
    BuildFileUser_OutObj *  pOutObj;
    bool                    atLeastOneObjDidRepair;
    bool                    atLeastOneObjDidNotRepair;

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

    pListLink = mpParentSrcXml->Proj.Lib.ObjProjList.mpHead;
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
            printf("*** Failed to create path for OutLib [%s]\n", pParentPath);
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

    pDst = mpVfsFile->AllocFullPath();

    pFullPath = mpVfsFile->AllocFullPath(K2ASC_Len(spCmdSpec));
    pTemp = new char[(mpVfsFile->GetFullPathLen() + 4) & ~3];
    K2_ASSERT(NULL != pTemp);
    K2ASC_Copy(pTemp, pFullPath);
    curLen = K2ASC_Printf(pFullPath, spCmdSpec, pTemp);
    delete[] pTemp;

    //
    // options for making lib (if there are any) are in the build xml file
    //

    //
    // add all the dependency objects to the command line at the end
    //
    pListLink = TmpObjList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pTmpObj = K2_GET_CONTAINER(BuildFileUser_TmpObj, pListLink, SrcXmlProjOutputTmpObjListLink);
            pListLink = pListLink->mpNext;

            pTemp = pTmpObj->mpVfsFile->AllocFullPath(0);
            curLen += 1 + pTmpObj->mpVfsFile->GetFullPathLen();
            pNext = new char[(curLen + 4) & ~3];
            K2_ASSERT(NULL != pNext);
            K2ASC_Printf(pNext, "%s %s", pFullPath, pTemp);
            delete[] pTemp;
            delete[] pFullPath;
            pFullPath = pNext;
        } while (NULL != pListLink);
    }

    pListLink = mpParentSrcXml->Proj.Lib.ObjProjList.mpHead;
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
            K2ASC_Printf(pNext, "%s %s", pFullPath, pTemp);
            delete[] pTemp;
            delete[] pFullPath;
            pFullPath = pNext;
        } while (NULL != pListLink);
    }

    //
    // we have our command line
    //
    pEnv = gpBaseEnviron->CopyBlob(&blobLen);
    K2_ASSERT(NULL != pEnv);

    outLen = 0;
    pOutput = NULL;
//    printf("%s\n", pFullPath);
    printf("Create Library [%s]\n", mpVfsFile->mpNameZ);
    cmdResult = RunProgram(gpVfsRootSpec, pFullPath, pEnv, &pOutput, &outLen);
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

    //
    // appearance of lib in file system will switch this to not damaged
    //

    delete[] pEnv;
    delete[] pFullPath;

    if (0 != cmdResult)
    {
        printf("error %d\n", cmdResult);
        return atLeastOneObjDidRepair;
    }

    return true;

}

void
BuildFileUser_OutLib::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    mpParentSrcXml->SetSomethingChangedSinceLastTryRepair();
}


bool
BuildFileUser_OutLib::CheckIfDamaged(
    void
)
{
    K2LIST_LINK *           pListLink;
    BuildFileUser_TmpObj *  pTmpObj;
    ProjDep *               pProjDep;

    if (!FileExists())
        return true;

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

    pListLink = mpParentSrcXml->Proj.Lib.ObjProjList.mpHead;
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

    return false;
}

void
BuildFileUser_OutLib::Dump(
    bool aDamagedOnly
)
{
    K2LIST_LINK *           pListLink;
    BuildFileUser_TmpObj *  pTmpObj;

    if ((aDamagedOnly) && (!IsDamaged()))
        return;

    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("  OUTLIB %s %s\n", IsDamaged() ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;

    if (IsDamaged())
    {
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
}

