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
BuildFileUser_SrcXml::Construct_Xdl(
    char const *apFullPath
)
{
    K2XML_NODE const *  pRootNode;
    UINT_PTR            subNodeCount;
    UINT_PTR            ixSubNode;
    K2XML_NODE const *  pSubNode;
    VfsFile *           pVfsFile;
    bool                ok;
    char *              pInfFilePath;
    char const *        pCheckPlat;
    char const *        pScan;
    UINT_PTR            infCount;
    UINT_PTR            subProjCount;
    VfsFile *           pInfVfsFile;
    bool                isInPlat;
    UINT_PTR            platLen;

    ok = false;
    isInPlat = false;

    pCheckPlat = apFullPath + gVfsRootSpecLen + 4 + 1 + K2ASC_Len(gpOsVer);
    isInPlat = (0 == K2ASC_CompInsLen(pCheckPlat, "\\plat\\", 6));
    if (isInPlat)
    {
        pScan = pCheckPlat + 6;
        do {
            if (*pScan == '\\')
                break;
            pScan++;
        } while ((*pScan) != 0);
        platLen = (UINT_PTR)(pScan - pCheckPlat);
    }

    Proj.Xdl.mIsCrt = (0 == K2ASC_CompIns(mpVfsFile->mpParentFolder->mpNameZ, "k2oscrt")) ? true : false;

    K2LIST_Init(&Proj.Xdl.ObjProjList);
    K2LIST_Init(&Proj.Xdl.LibProjList);
    K2LIST_Init(&Proj.Xdl.XdlProjList);

    infCount = subProjCount = 0;
    pInfVfsFile = NULL;

    do {
        pRootNode = XmlParse.mpRootNode;
        subNodeCount = pRootNode->mNumSubNodes;
        for (ixSubNode = 0; ixSubNode < subNodeCount; ixSubNode++)
        {
            pSubNode = pRootNode->mppSubNode[ixSubNode];

            if (0 == ((1 << gArch) & GetXmlNodeArchFlags(pSubNode)))
                continue;

            if (pSubNode->Name.mLen == 3)
            {
                if (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "img", 3))
                {
                    printf("*** XML for XDL project specifies an IMG which is not allowed [%s]\n", apFullPath);
                    break;
                }
                else if (0 == K2ASC_CompInsLen("inf", pSubNode->Name.mpChars, 3))
                {
                    //
                    // only count this <inf> if we match the arch and can find the node in the tree
                    //
                    if (0 != ((1 << gArch) & GetXmlNodeArchFlags(pSubNode)))
                    {
                        if (0 != infCount)
                        {
                            printf("*** XML has more than one INF for this architecture! [%s]\n", apFullPath);
                            break;
                        }

                        if (!GetContentTarget(mpVfsFile->mpParentFolder, pSubNode, &pInfFilePath))
                        {
                            printf("*** XML did not have a resolvable INF file via \"%.*s\" [%s]\n",
                                pSubNode->Content.mLen, pSubNode->Content.mpChars,
                                apFullPath);
                            break;
                        }

                        pInfVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(pInfFilePath + gVfsRootSpecLen + 1, false);
                        if (NULL == pInfVfsFile)
                        {
                            printf("*** XML INF file could not be acquired at \"%s\" [%s]\n", pInfFilePath, apFullPath);
                            delete[] pInfFilePath;
                            break;
                        }
                        delete[] pInfFilePath;

                        ++infCount;
                    }
                }
                else if (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "obj", 3))
                {
                    if (NULL == AddProject(apFullPath, pSubNode, ProjectType_Obj, &Proj.Xdl.ObjProjList))
                        break;
                }
                else if (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "lib", 3))
                {
                    if (NULL == AddProject(apFullPath, pSubNode, ProjectType_Lib, &Proj.Xdl.LibProjList))
                        break;
                }
                else if (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "xdl", 3))
                {
                    if (NULL == AddProject(apFullPath, pSubNode, ProjectType_Xdl, &Proj.Xdl.XdlProjList))
                        break;
                }
            }
        }
        if (ixSubNode != subNodeCount)
        {
            break;
        }

        if (NULL == pInfVfsFile)
        {
            printf("*** XML for XDL project did not have a usable INF file specified [%s]\n", apFullPath);
            break;
        }

        if ((0 == mSourcesCount) &&
            (0 == Proj.Xdl.ObjProjList.mNodeCount) &&
            (0 == Proj.Xdl.LibProjList.mNodeCount))
        {
            printf("*** XDL project does not have any source of its own or obj or lib subprojects [%s]\n", apFullPath);
            break;
        }

        if (!Proj.Xdl.mIsCrt)
        {
            //
            // add crtstub obj project
            //
            K2ASC_Printf(gStrBuf, "src\\%s\\crtstub", gpOsVer);
            if (!AddProjectByPath(apFullPath, gStrBuf, ProjectType_Obj, &Proj.Xdl.ObjProjList))
            {
                printf("*** Could not add crtstub as subproject for [%s]\n", apFullPath);
                break;
            }

            //
            // add k2oscrt xdl project
            //
            K2ASC_Printf(gStrBuf, "src\\%s%s\\crt\\%s\\k2oscrt",
                gpOsVer,
                mIsKernelTarget ? "\\kern" : "\\user",
                gpArchName[gArch]);

            if (!AddProjectByPath(apFullPath, gStrBuf, ProjectType_Xdl, &Proj.Xdl.XdlProjList))
            {
                printf("*** Could not add \"%s\" as subproject for [%s]\n", gStrBuf, apFullPath);
                break;
            }
        }

        K2ASC_Printf(gStrBuf, "bld\\out\\gcc\\xdl%s%.*s\\%s\\%s.xdl",
            mIsKernelTarget ? "\\kern" : "",
            isInPlat ? platLen : 0,
            isInPlat ? pCheckPlat : "",
            gpArchMode,
            mpVfsFile->mpParentFolder->mpNameZ);

        pVfsFile = (VfsFile *)VfsFolder::Root().AcquireOrCreateSub(gStrBuf, false);
        if (NULL == pVfsFile)
        {
            printf("*** could not acquire xdl object [%s] for [%s]\n", gStrBuf, apFullPath);
            break;
        }

        Proj.Xdl.mpOutXdl = BuildFileUser_OutXdl::Construct(pVfsFile, this, pInfVfsFile, apFullPath);
         
        pVfsFile->Release();

        if (NULL != Proj.Xdl.mpOutXdl)
        {

            if (!Proj.Xdl.mpOutXdl->CheckIfDamaged())
            {
                Proj.Xdl.mpOutXdl->OnRepaired();
            }
            else
            {
                Proj.Xdl.mpOutXdl->SetSomethingChangedSinceLastTryRepair();
            }

            ok = true;
        }

    } while (0);

    if (NULL != pInfVfsFile)
    {
        pInfVfsFile->Release();
    }

    if (!ok)
    {
        PurgeProjDepList(&Proj.Xdl.ObjProjList);
        PurgeProjDepList(&Proj.Xdl.LibProjList);
        PurgeProjDepList(&Proj.Xdl.XdlProjList);
        Proj.Xdl.mIsCrt = false;
    }

    return ok;
}

void
BuildFileUser_SrcXml::Destruct_Xdl(
    void
)
{
    K2_ASSERT(NULL != Proj.Xdl.mpOutXdl);
    delete Proj.Xdl.mpOutXdl;
    Proj.Xdl.mpOutXdl = NULL;
}

