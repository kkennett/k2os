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
    BuildFileUser_ImgDstBoot * pImgBoot;

    if (mSourcesCount != 0)
    {
        printf("*** XML for IMG project specifies a <source> which is not allowed [%s]\n", apFullPath);
        return false;
    }

    K2LIST_Init(&Proj.Img.UserXdlProjList);
    K2LIST_Init(&Proj.Img.RawXdlProjList);
    K2LIST_Init(&Proj.Img.BuiltInXdlProjList);
    K2LIST_Init(&Proj.Img.BuiltInKernXdlProjList);

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
            else if (pSubNode->Name.mLen == 12)
            {
                if (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "user_builtin", 12))
                {
                    pProjDep = AddProject(apFullPath, pSubNode, ProjectType_Xdl, &Proj.Img.BuiltInXdlProjList);
                    if (NULL == pProjDep)
                        break;
                }
                else if (0 == K2ASC_CompInsLen(pSubNode->Name.mpChars, "kern_builtin", 12))
                {
                    pProjDep = AddProject(apFullPath, pSubNode, ProjectType_Xdl, &Proj.Img.BuiltInKernXdlProjList);
                    if (NULL == pProjDep)
                        break;
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
            K2ASC_Printf(gStrBuf, "src\\%s\\kern\\crt\\%s\\k2oscrt", gpOsVer, gpArchName[gArch]);
            Proj.Img.mpKernCrtXdl = AddProjectByPath(apFullPath, gStrBuf, ProjectType_Xdl, &Proj.Img.RawXdlProjList);
            if (NULL == Proj.Img.mpKernCrtXdl)
                break;

            do {
                K2ASC_Printf(gStrBuf, "src\\%s\\kern\\main\\%s\\k2oskern", gpOsVer, gpArchName[gArch]);
                Proj.Img.mpKernXdl = AddProjectByPath(apFullPath, gStrBuf, ProjectType_Xdl, &Proj.Img.RawXdlProjList);
                if (NULL == Proj.Img.mpKernXdl)
                    break;

#if !IS_OSA
                do {
                    K2ASC_Printf(gStrBuf, "src\\%s\\kern\\k2osacpi", gpOsVer);
                    Proj.Img.mpKernAcpiXdl = AddProjectByPath(apFullPath, gStrBuf, ProjectType_Xdl, &Proj.Img.RawXdlProjList);
                    if (NULL == Proj.Img.mpKernAcpiXdl)
                        break;

                    do {
                        K2ASC_Printf(gStrBuf, "src\\%s\\kern\\k2osexec", gpOsVer);
                        Proj.Img.mpKernExecXdl = AddProjectByPath(apFullPath, gStrBuf, ProjectType_Xdl, &Proj.Img.RawXdlProjList);
                        if (NULL == Proj.Img.mpKernExecXdl)
                            break;
#endif
                        do {
                            K2ASC_Printf(gStrBuf, "src\\%s\\user\\crt\\%s\\k2oscrt", gpOsVer, gpArchName[gArch]);
                            Proj.Img.mpUserCrtXdl = AddProjectByPath(apFullPath, gStrBuf, ProjectType_Xdl, &Proj.Img.BuiltInXdlProjList);
                            if (NULL == Proj.Img.mpUserCrtXdl)
                                break;

                            do {
                                K2ASC_Printf(gStrBuf, "src\\%s\\user\\sysproc", gpOsVer);
                                Proj.Img.mpUserSysProcXdl = AddProjectByPath(apFullPath, gStrBuf, ProjectType_Xdl, &Proj.Img.BuiltInXdlProjList);
                                if (NULL == Proj.Img.mpUserSysProcXdl)
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
#if !IS_OSA
                                    if ((ProjectType_Invalid != Proj.Img.mpKernAcpiXdl->mpDependOn->mProjectType) &&
                                        (!Proj.Img.mpKernAcpiXdl->mpDependOn->mIsKernelTarget))
                                    {
                                        printf("*** XML for IMG specifies kern ACPI that is not targeted at kern mode [%s]\n", apFullPath);
                                        break;
                                    }

                                    if ((ProjectType_Invalid != Proj.Img.mpKernExecXdl->mpDependOn->mProjectType) &&
                                        (!Proj.Img.mpKernExecXdl->mpDependOn->mIsKernelTarget))
                                    {
                                        printf("*** XML for IMG specifies kern EXEC that is not targeted at kern mode [%s]\n", apFullPath);
                                        break;
                                    }
#endif
                                    if ((ProjectType_Invalid != Proj.Img.mpUserCrtXdl->mpDependOn->mProjectType) &&
                                        (Proj.Img.mpUserCrtXdl->mpDependOn->mIsKernelTarget))
                                    {
                                        printf("*** XML for IMG specifies user CRT that is not targeted at user mode [%s]\n", apFullPath);
                                        break;
                                    }

                                    if ((ProjectType_Invalid != Proj.Img.mpUserSysProcXdl->mpDependOn->mProjectType) &&
                                        (Proj.Img.mpUserSysProcXdl->mpDependOn->mIsKernelTarget))
                                    {
                                        printf("*** XML for IMG specifies system process that is not targeted at user mode [%s]\n", apFullPath);
                                        break;
                                    }

                                    K2ASC_Printf(gStrBuf, "bootdisk\\EFI\\BOOT\\boot%s.efi", gArchBoot[gArch]);
                                    pVfsFile = (VfsFile *)Proj.Img.mpImgFolder->AcquireOrCreateSub(gStrBuf, false);
                                    if (NULL == pVfsFile)
                                    {
                                        printf("*** Could not acquire boot disk target EFI bootloader [%s]\n", apFullPath);
                                        break;
                                    }
                                    pImgBoot = BuildFileUser_ImgDstBoot::Construct(pVfsFile, this, apFullPath);
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
                                    Proj.Img.mpUserSysProcXdl = NULL;

                            } while (0);

                            if (!ok)
                                Proj.Img.mpUserCrtXdl = NULL;

                        } while (0);

#if !IS_OSA

                        if (!ok)
                            Proj.Img.mpKernExecXdl = NULL;

                    } while (0);

                    if (!ok)
                        Proj.Img.mpKernAcpiXdl = NULL;

                } while (0);

#endif

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
        PurgeProjDepList(&Proj.Img.BuiltInKernXdlProjList);
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
    PurgeProjDepList(&Proj.Img.BuiltInKernXdlProjList);

    //
    // project dependencies will all be purged by srcxml generic code
    // we just wipe out the pointers to the deps here
    //

    Proj.Img.mpKernXdl = NULL;
    Proj.Img.mpPlatXdl = NULL;
    Proj.Img.mpKernCrtXdl = NULL;
#if !IS_OSA
    Proj.Img.mpKernAcpiXdl = NULL;
    Proj.Img.mpKernExecXdl = NULL;
#endif
    Proj.Img.mpUserCrtXdl = NULL;
    Proj.Img.mpUserSysProcXdl = NULL;
}

