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

char const * const gArchBoot[5] = { "none", "ia32", "arm", "ia64", "aa64" };

bool
BuildFileUser_SrcXml::Construct_Img(
    char const *apFullPath
)
{
    K2XML_NODE const *      pRootNode;
    UINT_PTR                subNodeCount;
    UINT_PTR                ixSubNode;
    K2XML_NODE const *      pSubNode;
    bool                    ok;
    ProjDep *               pProjDep;
    VfsFile *               pVfsFile;
    BuildFileUser_ImgBoot * pImgBoot;

    if (mSourcesCount != 0)
    {
        printf("*** XML for IMG project specifies a <source> which is not allowed [%s]\n", apFullPath);
        return false;
    }

    K2LIST_Init(&Proj.Img.UserXdlProjList);
    K2LIST_Init(&Proj.Img.RawXdlProjList);
    K2LIST_Init(&Proj.Img.BuiltInXdlProjList);

    ok = false;

    do {
        pRootNode = XmlParse.mpRootNode;
        subNodeCount = pRootNode->mNumSubNodes;
        for (ixSubNode = 0; ixSubNode < subNodeCount; ixSubNode++)
        {
            pSubNode = pRootNode->mppSubNode[ixSubNode];
            if (pSubNode->Name.mLen == 3)
            {
                if ((0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "obj", 3)) ||
                    (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "lib", 3)) ||
                    (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "img", 3)))
                {
                    printf("*** XML for IMG project specifies a %.3s which is not allowed [%s]\n", pSubNode->Name.mpChars, apFullPath);
                    return false;
                }
                else if (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "xdl", 3))
                {
                    pProjDep = AddProject(apFullPath, pSubNode, ProjectType_Xdl, &Proj.Img.UserXdlProjList);
                    if (NULL == pProjDep)
                        break;
                    if ((ProjectType_Invalid != pProjDep->mpDependOn->mProjectType) &&
                        (pProjDep->mpDependOn->mIsKernelTarget))
                    {
                        printf("*** XML for IMG specifies supplemental user XDL that is not targeted at user mode [%s]\n", apFullPath);
                        break;
                    }
                }
            }
            else if (pSubNode->Name.mLen == 4)
            {
                if (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "plat", 4))
                {
                    if (NULL != Proj.Img.mpPlatXdl)
                    {
                        printf("*** XML for IMG has more than one <plat> specified [%s]\n", apFullPath);
                        break;
                    }
                    Proj.Img.mpPlatXdl = AddProject(apFullPath, pSubNode, ProjectType_Xdl, &Proj.Img.RawXdlProjList);
                    if (NULL == Proj.Img.mpPlatXdl)
                        break;
                }
            }
            else if (pSubNode->Name.mLen == 7)
            {
                if (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "builtin", 7))
                {
                    pProjDep = AddProject(apFullPath, pSubNode, ProjectType_Xdl, &Proj.Img.BuiltInXdlProjList);
                    if (NULL == pProjDep)
                        break;
                    if ((ProjectType_Invalid != pProjDep->mpDependOn->mProjectType) &&
                        (pProjDep->mpDependOn->mIsKernelTarget))
                    {
                        printf("*** XML for IMG specifies supplemental user XDL that is not targeted at user mode [%s]\n", apFullPath);
                        break;
                    }
                }
            }
        }
        if (ixSubNode != subNodeCount)
        {
            break;
        }

        if (NULL == Proj.Img.mpPlatXdl)
        {
            printf("*** XML for IMG has no <plat> node specified [%s]\n", apFullPath);
            break;
        }

        K2ASC_Printf(gStrBuf, "img\\%s\\%s",
            gpArchMode,
            mpSubPath + 4);   // move past "src\"

        Proj.Img.mpImgFolder = (VfsFolder *)VfsFolder::Root().AcquireOrCreateSub(gStrBuf, true);
        if (NULL == Proj.Img.mpImgFolder)
        {
            printf("*** Could not acquire output for IMG project [%s]\n", apFullPath);
            break;
        }

        do {
            K2ASC_Printf(gStrBuf, "src\\os8\\kern\\crt\\bits32\\%s\\k2oscrt", gpArchName[gArch]);
            Proj.Img.mpKernCrtXdl = AddProjectByPath(apFullPath, gStrBuf, ProjectType_Xdl, &Proj.Img.RawXdlProjList);
            if (NULL == Proj.Img.mpKernCrtXdl)
                break;

            do {
                K2ASC_Printf(gStrBuf, "src\\os8\\kern\\main\\bits32\\%s\\k2oskern", gpArchName[gArch]);
                Proj.Img.mpKernXdl = AddProjectByPath(apFullPath, gStrBuf, ProjectType_Xdl, &Proj.Img.RawXdlProjList);
                if (NULL == Proj.Img.mpKernXdl)
                    break;

                do {
                    K2ASC_Printf(gStrBuf, "src\\os8\\user\\crt\\bits32\\%s\\k2oscrt", gpArchName[gArch]);
                    Proj.Img.mpUserCrtXdl = AddProjectByPath(apFullPath, gStrBuf, ProjectType_Xdl, &Proj.Img.BuiltInXdlProjList);
                    if (NULL == Proj.Img.mpUserCrtXdl)
                        break;

                    do {
                        Proj.Img.mpUserRootXdl = AddProjectByPath(apFullPath, "src\\os8\\user\\root", ProjectType_Xdl, &Proj.Img.BuiltInXdlProjList);
                        if (NULL == Proj.Img.mpUserRootXdl)
                            break;

                        do {
                            Proj.Img.mpUserAcpiXdl = AddProjectByPath(apFullPath, "src\\os8\\user\\k2osacpi", ProjectType_Xdl, &Proj.Img.BuiltInXdlProjList);
                            if (NULL == Proj.Img.mpUserAcpiXdl)
                                break;

                            do {
                                Proj.Img.mpUserRpcXdl = AddProjectByPath(apFullPath, "src\\os8\\user\\k2osrpc", ProjectType_Xdl, &Proj.Img.BuiltInXdlProjList);
                                if (NULL == Proj.Img.mpUserRpcXdl)
                                    break;

                                do {
                                    Proj.Img.mpUserDrvXdl = AddProjectByPath(apFullPath, "src\\os8\\user\\k2osdrv", ProjectType_Xdl, &Proj.Img.BuiltInXdlProjList);
                                    if (NULL == Proj.Img.mpUserDrvXdl)
                                        break;

                                    do {
                                        if ((ProjectType_Invalid != Proj.Img.mpKernCrtXdl->mpDependOn->mProjectType) &&
                                            (!Proj.Img.mpKernCrtXdl->mpDependOn->mIsKernelTarget))
                                        {
                                            printf("*** XML for IMG specifies kernel CRT that is not targeted at kernel mode [%s]\n", apFullPath);
                                            break;
                                        }
                                        if ((ProjectType_Invalid != Proj.Img.mpKernXdl->mpDependOn->mProjectType) &&
                                            (!Proj.Img.mpKernXdl->mpDependOn->mIsKernelTarget))
                                        {
                                            printf("*** XML for IMG specifies kernel that is not targeted at kernel mode [%s]\n", apFullPath);
                                            break;
                                        }

                                        if ((ProjectType_Invalid != Proj.Img.mpUserCrtXdl->mpDependOn->mProjectType) &&
                                            (Proj.Img.mpUserCrtXdl->mpDependOn->mIsKernelTarget))
                                        {
                                            printf("*** XML for IMG specifies user CRT that is not targeted at user mode [%s]\n", apFullPath);
                                            break;
                                        }

                                        if ((ProjectType_Invalid != Proj.Img.mpUserRootXdl->mpDependOn->mProjectType) &&
                                            (Proj.Img.mpUserRootXdl->mpDependOn->mIsKernelTarget))
                                        {
                                            printf("*** XML for IMG specifies root process that is not targeted at user mode [%s]\n", apFullPath);
                                            break;
                                        }
                                        if ((ProjectType_Invalid != Proj.Img.mpUserAcpiXdl->mpDependOn->mProjectType) &&
                                            (Proj.Img.mpUserAcpiXdl->mpDependOn->mIsKernelTarget))
                                        {
                                            printf("*** XML for IMG specifies user ACPI that is not targeted at user mode [%s]\n", apFullPath);
                                            break;
                                        }

                                        if ((ProjectType_Invalid != Proj.Img.mpUserRpcXdl->mpDependOn->mProjectType) &&
                                            (Proj.Img.mpUserRpcXdl->mpDependOn->mIsKernelTarget))
                                        {
                                            printf("*** XML for IMG specifies rpc xdl that is not targeted at user mode [%s]\n", apFullPath);
                                            break;
                                        }
                                        if ((ProjectType_Invalid != Proj.Img.mpUserDrvXdl->mpDependOn->mProjectType) &&
                                            (Proj.Img.mpUserDrvXdl->mpDependOn->mIsKernelTarget))
                                        {
                                            printf("*** XML for IMG specifies driver host xdl that is not targeted at user mode [%s]\n", apFullPath);
                                            break;
                                        }

                                        K2ASC_Printf(gStrBuf, "bootdisk\\EFI\\BOOT\\BOOT%s.EFI", gArchBoot[gArch]);
                                        pVfsFile = (VfsFile *)Proj.Img.mpImgFolder->AcquireOrCreateSub(gStrBuf, false);
                                        if (NULL == pVfsFile)
                                        {
                                            printf("*** Could not acquire boot disk target EFI bootloader [%s]\n", apFullPath);
                                            break;
                                        }
                                        pImgBoot = BuildFileUser_ImgBoot::Construct(pVfsFile, this, apFullPath);
                                        pVfsFile->Release();
                                        if (NULL == pImgBoot)
                                        {
                                            printf("*** Could not construct bootloader output file in image folder [%s]\n", apFullPath);
                                            break;
                                        }

                                        K2ASC_Printf(gStrBuf, "bootdisk\\K2OS\\%s\\KERN\\k2oskern.img", gpArchName[gArch]);
                                        pVfsFile = (VfsFile *)Proj.Img.mpImgFolder->AcquireOrCreateSub(gStrBuf, false);
                                        if (NULL == pVfsFile)
                                        {
                                            printf("*** Could not acquire output image file [%s]\n", apFullPath);
                                            delete pImgBoot;
                                            break;
                                        }
                                        Proj.Img.mpImg = BuildFileUser_Img::Construct(pVfsFile, this, pImgBoot, apFullPath);
                                        pVfsFile->Release();
                                        if (NULL == Proj.Img.mpImg)
                                        {
                                            delete pImgBoot;
                                        }
                                        else
                                        {
                                            ok = true;
                                        }

                                    } while (0);

                                    if (!ok)
                                        Proj.Img.mpUserDrvXdl = NULL;

                                } while (0);

                                if (!ok)
                                    Proj.Img.mpUserRpcXdl = NULL;

                            } while (0);

                            if (!ok)
                                Proj.Img.mpUserAcpiXdl = NULL;

                        } while (0);

                        if (!ok)
                            Proj.Img.mpUserRootXdl = NULL;

                    } while (0);

                    if (!ok)
                        Proj.Img.mpUserCrtXdl = NULL;

                } while (0);

                if (!ok)
                    Proj.Img.mpKernXdl = NULL;

            } while (0);

            if (!ok)
                Proj.Img.mpKernCrtXdl = NULL;

        } while (0);

        if (!ok)
        {
            Proj.Img.mpImgFolder->Release();
            Proj.Img.mpImgFolder = NULL;
        }

    } while (0);

    if (!ok)
    {
        PurgeProjDepList(&Proj.Img.RawXdlProjList);
        PurgeProjDepList(&Proj.Img.UserXdlProjList);
        PurgeProjDepList(&Proj.Img.BuiltInXdlProjList);
        Proj.Img.mpPlatXdl = NULL;
    }

    return ok;
}

void
BuildFileUser_SrcXml::Destruct_Img(
    void
)
{
    delete Proj.Img.mpImg;

    PurgeProjDepList(&Proj.Img.RawXdlProjList);
    PurgeProjDepList(&Proj.Img.UserXdlProjList);
    PurgeProjDepList(&Proj.Img.BuiltInXdlProjList);

    //
    // project dependencies will all be purged by srcxml generic code
    // we just wipe out the pointers to the deps here
    //

    Proj.Img.mpKernXdl = NULL;
    Proj.Img.mpPlatXdl = NULL;
    Proj.Img.mpKernCrtXdl = NULL;
    Proj.Img.mpUserCrtXdl = NULL;
    Proj.Img.mpUserAcpiXdl = NULL;
    Proj.Img.mpUserRootXdl = NULL;
    Proj.Img.mpUserRpcXdl = NULL;
    Proj.Img.mpUserDrvXdl = NULL;
}

