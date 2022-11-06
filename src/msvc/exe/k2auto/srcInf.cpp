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

BuildFileUser_SrcInf::BuildFileUser_SrcInf(
    VfsFile * apVfsFile
) :
    BuildFileUser(apVfsFile, BuildFileUserType_SrcInf)
{
    mpParentTmpExpObj = NULL;
    if (mpVfsFile->Exists())
        OnRepaired();
}

BuildFileUser_SrcInf::~BuildFileUser_SrcInf(
    void
)
{
    mpParentTmpExpObj = NULL;
}

void 
BuildFileUser_SrcInf::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    OnRepaired();
    mpParentTmpExpObj->OnSrcInfRepaired();
}

void 
BuildFileUser_SrcInf::OnFsUpdate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsUpdate(apFullPath);
    mpParentTmpExpObj->OnSrcInfDamaged();
    OnRepaired();
    mpParentTmpExpObj->OnSrcInfRepaired();
}

void 
BuildFileUser_SrcInf::OnFsDelete(
    char const *apFullPath
)
{
    BuildFileUser::OnFsDelete(apFullPath);
    mpParentTmpExpObj->OnSrcInfDamaged();
}

void
BuildFileUser_SrcInf::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    if (NULL != mpParentTmpExpObj)
    {
        mpParentTmpExpObj->SetSomethingChangedSinceLastTryRepair();
    }
}

void 
BuildFileUser_SrcInf::Dump(
    void
)
{
    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("          %s %s\n", IsDamaged() ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;
}

