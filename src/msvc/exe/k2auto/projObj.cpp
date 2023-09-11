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

bool
BuildFileUser_SrcXml::Construct_Obj(
    char const *apFullPath
)
{
    K2XML_NODE const *      pRootNode;
    UINT_PTR                subNodeCount;
    UINT_PTR                ixSubNode;
    K2XML_NODE const *      pSubNode;
    VfsFile *               pVfsFile;

    if (mSourcesCount != 1)
    {
        printf("*** XML for OBJ project does not have exactly one <source> for selected arch [%s]\n", apFullPath);
        return false;
    }

    pRootNode = XmlParse.mpRootNode;
    subNodeCount = pRootNode->mNumSubNodes;
    for (ixSubNode = 0; ixSubNode < subNodeCount; ixSubNode++)
    {
        pSubNode = pRootNode->mppSubNode[ixSubNode];
        if (pSubNode->Name.mLen == 3)
        {
            if ((0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "obj", 3)) ||
                (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "lib", 3)) ||
                (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "xdl", 3)) ||
                (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "img", 3)))
            {
                printf("*** XML for OBJ project specifies a %.3s which is not allowed [%s]\n", pSubNode->Name.mpChars, apFullPath);
                return false;
            }
        }
    }

    K2ASC_Printf(gStrBuf, "bld\\out\\gcc\\obj\\%s\\%s\\%s.o",
        gpArchMode,
        mpSubPath,
        mpVfsFile->mpParentFolder->mpNameZ);

    pVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(gStrBuf, false);
    if (NULL == pVfsFile)
    {
        printf("*** Could not acquire output for OBJ project [%s]\n", apFullPath);
        return false;
    }

    Proj.Obj.mpOutObj = BuildFileUser_OutObj::Construct(pVfsFile, this, apFullPath);

    pVfsFile->Release();

    if (NULL == Proj.Obj.mpOutObj)
    {
        return false;
    }

    if (!Proj.Obj.mpOutObj->CheckIfDamaged())
    {
        Proj.Obj.mpOutObj->OnRepaired();
    }
    else
    {
        Proj.Obj.mpOutObj->SetSomethingChangedSinceLastTryRepair();
    }

    return true;
}

void
BuildFileUser_SrcXml::Destruct_Obj(
    void
)
{
    K2_ASSERT(NULL != Proj.Obj.mpOutObj);
    delete Proj.Obj.mpOutObj;
    Proj.Obj.mpOutObj = NULL;
}

