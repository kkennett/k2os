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

BuildFileUser_TmpObj *
BuildFileUser_TmpObj::Construct(
    char const *            apSrcCodePath,
    VfsFile *               apSrcCodeVfsFile,
    BuildFileUser_SrcXml *  apSrcXml,
    K2XML_NODE const *      apSrcXmlNode,
    char const *            apXmlFullPath
)
{
    BuildFileUser_TmpObj *  pResult;
    BuildFileUser_TmpDep *  pTmpDep;
    char *                  pEnd;
    char                    ch;
    VfsFile *               pTmpObjVfsFile;

    pTmpDep = BuildFileUser_TmpDep::Construct(apSrcCodePath, apSrcCodeVfsFile, apSrcXml, apSrcXmlNode, apXmlFullPath);
    if (NULL == pTmpDep)
        return NULL;

    pEnd = gStrBuf + K2ASC_Printf(gStrBuf, "bld\\tmp\\gcc\\obj\\%s%s",
        gpArchMode,
        apSrcCodePath + gVfsRootSpecLen);
    do {
        --pEnd;
        ch = *pEnd;
        if ((ch == '.') || (ch == '\\'))
            break;
    } while (pEnd != gStrBuf);
    if (ch != '.')
    {
        printf("*** XML specifies <source> file \"%s\" with no extension [%s]\n", gStrBuf, apXmlFullPath);
        delete pTmpDep;
        return NULL;
    }
    pEnd++;
    *pEnd = 'o';
    pEnd++;
    *pEnd = 0;

    pTmpObjVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(gStrBuf, false);
    if (NULL == pTmpObjVfsFile)
    {
        printf("*** Failed to acquire a tmp obj file vfs node [%s]\n", gStrBuf);
        delete pTmpDep;
        return NULL;
    }

    pResult = new BuildFileUser_TmpObj(pTmpObjVfsFile, pTmpDep, apSrcXml);

    pTmpObjVfsFile->Release();

    return pResult;
}

BuildFileUser_TmpObj::BuildFileUser_TmpObj(
    VfsFile *               apVfsFile, 
    BuildFileUser_TmpDep *  apTmpDep, 
    BuildFileUser_SrcXml *  apSrcXml
) :
    BuildFileUser(apVfsFile, BuildFileUserType_TmpObj),
    mpParentSrcXml(apSrcXml),
    mpChildTmpDep(apTmpDep)
{
    mpChildTmpDep->mpParentTmpObj = this;
    K2MEM_Zero(&SrcXmlProjOutputTmpObjListLink, sizeof(SrcXmlProjOutputTmpObjListLink));
}

BuildFileUser_TmpObj::~BuildFileUser_TmpObj(
    void
)
{
    delete mpChildTmpDep;
}

void 
BuildFileUser_TmpObj::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
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
BuildFileUser_TmpObj::OnFsUpdate(
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
BuildFileUser_TmpObj::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    switch (mpParentSrcXml->mProjectType)
    {
    case ProjectType_Obj:
        mpParentSrcXml->Proj.Obj.mpOutObj->SetSomethingChangedSinceLastTryRepair();
        break;
    case ProjectType_Lib:
        mpParentSrcXml->Proj.Lib.mpOutLib->SetSomethingChangedSinceLastTryRepair();
        break;
    case ProjectType_Xdl:
        mpParentSrcXml->Proj.Xdl.mpOutXdl->SetSomethingChangedSinceLastTryRepair();
        break;
    default:
        K2_ASSERT(0);
        break;
    }
}

void 
BuildFileUser_TmpObj::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    switch (mpParentSrcXml->mProjectType)
    {
    case ProjectType_Obj:
        mpParentSrcXml->Proj.Obj.mpOutObj->OnChildDamaged(this);
        break;
    case ProjectType_Lib:
        mpParentSrcXml->Proj.Lib.mpOutLib->OnChildDamaged(this);
        break;
    case ProjectType_Xdl:
        mpParentSrcXml->Proj.Xdl.mpOutXdl->OnChildDamaged(this);
        break;
    default:
        K2_ASSERT(0);
        break;
    }
}

void 
BuildFileUser_TmpObj::OnRepaired(
    void
)
{
    BuildFileUser::OnRepaired();
    switch (mpParentSrcXml->mProjectType)
    {
    case ProjectType_Obj:
        mpParentSrcXml->Proj.Obj.mpOutObj->OnChildRepaired(this);
        break;
    case ProjectType_Lib:
        mpParentSrcXml->Proj.Lib.mpOutLib->OnChildRepaired(this);
        break;
    case ProjectType_Xdl:
        mpParentSrcXml->Proj.Xdl.mpOutXdl->OnChildRepaired(this);
        break;
    default:
        K2_ASSERT(0);
        break;
    }
}

bool 
BuildFileUser_TmpObj::TryRepair(void
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

    return mpChildTmpDep->TryRepair(true);
}

bool 
BuildFileUser_TmpObj::CheckIfDamaged(
    void
)
{
    if (!FileExists())
        return true;
    if (mpChildTmpDep->IsDamaged())
        return true;
    if (mpVfsFile->IsOlderThan(mpChildTmpDep->mpVfsFile))
        return true;
    return false;
}

void 
BuildFileUser_TmpObj::Dump(
    bool aDamagedOnly
)
{
    if ((aDamagedOnly) && (!IsDamaged()))
        return;

    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("    %s %s\n", IsDamaged() ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;

    mpChildTmpDep->Dump(aDamagedOnly);
}
