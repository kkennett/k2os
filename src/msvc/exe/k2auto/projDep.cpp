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

ProjDep::ProjDep(
    BuildFileUser_SrcXml *  apOwner,
    K2LIST_ANCHOR *         apOwnerList,
    BuildFileUser_SrcXml *  apDependOn
)
    : mpOwner(apOwner), 
    mpOwnerList(apOwnerList), 
    mpDependOn(apDependOn)
{
    K2_ASSERT(NULL != mpDependOn);

    mpDependOn->AddRef();

    K2LIST_AddAtTail(&mpDependOn->ProjDepList, &ProjDepListLink);

    if (NULL != apOwnerList)
    {
        K2LIST_AddAtTail(apOwnerList, &OwnerListLink);
    }
    else
    {
        K2MEM_Zero(&OwnerListLink, sizeof(OwnerListLink));
    }

    mEventIter = 0;
    mpOwnerXmlNode = NULL;
}

ProjDep::~ProjDep(
    void
)
{
    mpOwnerXmlNode = NULL;

    if (NULL != mpOwnerList)
    {
        K2LIST_Remove(mpOwnerList, &OwnerListLink);
        mpOwnerList = NULL;
    }

    K2LIST_Remove(&mpDependOn->ProjDepList, &ProjDepListLink);

    mpDependOn->Release();
}

