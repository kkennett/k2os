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

BuildFileUser::BuildFileUser(
    VfsFile *           apVfsFile, 
    BuildFileUserType   aFileType
) :
    VfsUser(apVfsFile),
    mFileType(aFileType)
{
    mIsDamaged = true;
    mSomethingChangedSinceLastTryRepair = true;
}

BuildFileUser::~BuildFileUser(void)
{

}

void 
BuildFileUser::OnDamaged(
    void
) 
{ 
    SetSomethingChangedSinceLastTryRepair();

    if (mIsDamaged)
        return;
#if 0
    char *pPath = mpVfsFile->AllocFullPath();
    printf("DAMAGED %s\n", pPath);
    delete[] pPath;
#endif

    mIsDamaged = true;
}

void 
BuildFileUser::OnRepaired(
    void
) 
{ 
    K2_ASSERT(FileExists()); // impossible for a nonexistent file to be repaired

    SetSomethingChangedSinceLastTryRepair();

    if (!mIsDamaged)
        return;

#if 0
    char *pPath = mpVfsFile->AllocFullPath();
    printf("REPAIRD %s\n", pPath);
    delete[] pPath;
#endif

    mIsDamaged = false;
}

void 
BuildFileUser::OnFsCreate(
    char const *apFullPath
) 
{ 
    SetSomethingChangedSinceLastTryRepair();
}

void 
BuildFileUser::OnFsUpdate(
    char const *apFullPath
) 
{ 
    OnDamaged(); 
}

void 
BuildFileUser::OnFsDelete(
    char const *apFullPath
) 
{ 
    OnDamaged(); 
}

bool 
BuildFileUser::TryRepair(
    void
)
{
    bool result;

    K2_ASSERT(mIsDamaged);

    result = mSomethingChangedSinceLastTryRepair;

    mSomethingChangedSinceLastTryRepair = false;

    return result;
}

