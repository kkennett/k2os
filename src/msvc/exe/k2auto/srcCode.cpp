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

BuildFileUser_SrcCode::BuildFileUser_SrcCode(
    VfsFile *           apVfsFile,
    K2XML_NODE const *  apSrcXmlNode
) :
    BuildFileUser(apVfsFile, BuildFileUserType_SrcCode),
    mpParentSrcXmlNode(apSrcXmlNode)
{
    mpParentTmpDep = NULL;
    mLastBuildResult = -1;
    if (FileExists())
        OnRepaired();
    K2LIST_AddAtTail(&gSourceFileList, &SrcFileListLink);
}

BuildFileUser_SrcCode::~BuildFileUser_SrcCode(
    void
)
{
    mpParentTmpDep = NULL;
    K2LIST_Remove(&gSourceFileList, &SrcFileListLink);
}

void 
BuildFileUser_SrcCode::OnFsCreate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsCreate(apFullPath);
    OnRepaired();
    mpParentTmpDep->OnSrcCodeRepaired();
}

void 
BuildFileUser_SrcCode::OnFsUpdate(
    char const *apFullPath
)
{
    BuildFileUser::OnFsUpdate(apFullPath);
    OnDamaged();
    OnRepaired();
    mpParentTmpDep->OnSrcCodeRepaired();
}

void
BuildFileUser_SrcCode::OnDamaged(
    void
)
{
    BuildFileUser::OnDamaged();
    mpParentTmpDep->OnSrcCodeDamaged();
    OnRepaired();
    mpParentTmpDep->OnSrcCodeRepaired();
}

void 
BuildFileUser_SrcCode::SetSomethingChangedSinceLastTryRepair(
    void
)
{
    BuildFileUser::SetSomethingChangedSinceLastTryRepair();
    if (NULL != mpParentTmpDep)
    {
        mpParentTmpDep->SetSomethingChangedSinceLastTryRepair();
    }
}

void 
BuildFileUser_SrcCode::Dump(
    void
)
{
    char *pFullPath = mpVfsFile->AllocFullPath();
    printf("        %s %s\n", IsDamaged() ? "DAMG" : "GOOD", pFullPath + gVfsRootSpecLen + 5);
    delete[] pFullPath;
}

bool
BuildFileUser_SrcCode::Build(
    void
)
{
    char *                  pTmpDepFilePath;
    char *                  pTmpObjFilePath;
    bool                    didSomething;
    char *                  pEnd;
    BuildFileUser_SrcXml *  pSrcXml;

    pTmpDepFilePath = mpParentTmpDep->mpVfsFile->AllocFullPath();
    pTmpObjFilePath = mpParentTmpDep->mpParentTmpObj->mpVfsFile->AllocFullPath();

    pEnd = mpVfsFile->mpNameZ + mpVfsFile->GetExtOffset();
    pSrcXml = mpParentTmpDep->mpParentTmpObj->mpParentSrcXml;
    if (0 == K2ASC_CompIns(pEnd, "c"))
    {
        didSomething = Launch_C_Compile(pSrcXml, pTmpDepFilePath, pTmpObjFilePath);
    }
    else if (0 == K2ASC_CompIns(pEnd, "cpp"))
    {
        didSomething = Launch_CPP_Compile(pSrcXml, pTmpDepFilePath, pTmpObjFilePath);
    }
    else if (0 == K2ASC_CompIns(pEnd, "s"))
    {
        didSomething = Launch_Assembler(pSrcXml, pTmpDepFilePath, pTmpObjFilePath);
    }
    else
    {
        pEnd = mpVfsFile->AllocFullPath();
        printf("*** No idea how to build [%s]\n", pEnd);
        delete[] pEnd;
        didSomething = false;
    }

    delete[] pTmpDepFilePath;
    delete[] pTmpObjFilePath;

    return didSomething;
}

bool
BuildFileUser_SrcCode::Launch_C_Compile(
    BuildFileUser_SrcXml *  apSrcXml,
    char const *            apDepFilePath,
    char const *            apObjFilePath
)
{
    //    gcc -x c $(GCCOPT)  -MT '$(K2_OBJECT_PATH)/$(notdir $@)' -MD -MF $(K2_OBJECT_PATH)/$(basename $<).d -o $(K2_OBJECT_PATH)/$(basename $<).o $<
    static char const *spBuildSpec = "gcc -x c %s %s%s-MT '%s' -MD -MF %s -o %s %s";
    char *              pCmdLine;
    char *              pTargetOnly;
    UINT_PTR            blobLen;
    char *              pEnv;
    char *              pOutput;
    UINT_PTR            outLen;
    bool                isInOsPath;
    K2XML_NODE const *  pRootNode;
    K2XML_NODE const *  pSubNode;
    UINT_PTR            numSubNodes;
    UINT_PTR            ixSubNode;
    char *              pWork;
    char *              pTemp;
    char *              pIncPath;

    pTargetOnly = mpVfsFile->AllocFullPath();

    K2ASC_Printf(gStrBuf, "src\\%s\\", gpOsVer);
    isInOsPath = (0 == K2ASC_CompInsLen(pTargetOnly + gVfsRootSpecLen + 1, gStrBuf, 5 + K2ASC_Len(gpOsVer))) ? true : false;

    outLen = 1;
    pTemp = new char[(outLen + 4) & ~3];
    K2_ASSERT(NULL != pTemp);
    pTemp[0] = ' ';
    pTemp[1] = 0;

    //
    // look for include and copts
    //
    pRootNode = apSrcXml->XmlParse.mpRootNode;
    numSubNodes = pRootNode->mNumSubNodes;
    for (ixSubNode = 0; ixSubNode < numSubNodes; ixSubNode++)
    {
        pSubNode = pRootNode->mppSubNode[ixSubNode];
        if (0 != ((1 << gArch) & GetXmlNodeArchFlags(pSubNode)))
        {
            if (pSubNode->Name.mLen == 5)
            {
                if (0 == K2ASC_CompInsLen("copts", pSubNode->Name.mpChars, 5))
                {
                    pWork = new char[(outLen + pSubNode->Content.mLen + 1) + 4 & ~3];
                    K2_ASSERT(NULL != pWork);
                    outLen = K2ASC_Printf(pWork, "%s%.*s ", pTemp, pSubNode->Content.mLen, pSubNode->Content.mpChars);
                    delete[] pTemp;
                    pTemp = pWork;
                }
            }
            else if (pSubNode->Name.mLen == 7)
            {
                if (0 == K2ASC_CompInsLen("include", pSubNode->Name.mpChars, 7))
                {
                    if (!GetContentTarget(apSrcXml->mpVfsFile->mpParentFolder, pSubNode, &pIncPath))
                    {
                        printf("*** could not resolve include target to vfs path [%.*s]\n",
                            pSubNode->Content.mLen, pSubNode->Content.mpChars);
                    }
                    else
                    {
                        pWork = new char[(outLen + 3 + K2ASC_Len(pIncPath) + 1) + 4 & ~3];
                        K2_ASSERT(NULL != pWork);
                        outLen = K2ASC_Printf(pWork, "%s-I %s ", pTemp, pIncPath);
                        delete[] pTemp;
                        pTemp = pWork;
                    }
                }
            }
        }
    }

    outLen = K2ASC_Printf(gStrBuf,
        spBuildSpec,
        gpStr_OptC,
        isInOsPath ? gpStr_OsInc : "",
        pTemp, 
        apObjFilePath,
        apDepFilePath,
        apObjFilePath,
        pTargetOnly);

    delete[] pTemp;
    delete[] pTargetOnly;

    pCmdLine = new char[(outLen + 4) & ~3];
    K2_ASSERT(NULL != pCmdLine);
    K2ASC_CopyLen(pCmdLine, gStrBuf, outLen);
    pCmdLine[outLen] = 0;

    pEnv = gpBaseEnviron->CopyBlob(&blobLen);
    K2_ASSERT(NULL != pEnv);

    outLen = 0;
    pOutput = NULL;
//    printf("%s\n", pCmdLine);
    printf("Compile C [%s]\n", mpVfsFile->mpNameZ);
    mLastBuildResult = RunProgram(gpVfsRootSpec, pCmdLine, pEnv, &pOutput, &outLen);
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

    if (0 != mLastBuildResult)
    {
        printf("error %d\n", mLastBuildResult);
        return false;
    }

    return true;
}

bool
BuildFileUser_SrcCode::Launch_CPP_Compile(
    BuildFileUser_SrcXml *  apSrcXml,
    char const *            apDepFilePath,
    char const *            apObjFilePath
)
{
    static char const *spBuildSpec = "gcc -x c++ %s %s%s-MT '%s' -MD -MF %s -o %s %s";
    char *              pCmdLine;
    char *              pTargetOnly;
    UINT_PTR            blobLen;
    char *              pEnv;
    char *              pOutput;
    UINT_PTR            outLen;
    bool                isInOsPath;
    K2XML_NODE const *  pRootNode;
    K2XML_NODE const *  pSubNode;
    UINT_PTR            numSubNodes;
    UINT_PTR            ixSubNode;
    char *              pWork;
    char *              pTemp;
    char *              pIncPath;

    pTargetOnly = mpVfsFile->AllocFullPath();

    K2ASC_Printf(gStrBuf, "src\\%s\\", gpOsVer);
    isInOsPath = (0 == K2ASC_CompInsLen(pTargetOnly + gVfsRootSpecLen + 1, gStrBuf, 5 + K2ASC_Len(gpOsVer))) ? true : false;
 
    outLen = 1;
    pTemp = new char[(outLen + 4) & ~3];
    K2_ASSERT(NULL != pTemp);
    pTemp[0] = ' ';
    pTemp[1] = 0;

    //
    // look for include and copts
    //
    pRootNode = apSrcXml->XmlParse.mpRootNode;
    numSubNodes = pRootNode->mNumSubNodes;
    for (ixSubNode = 0; ixSubNode < numSubNodes; ixSubNode++)
    {
        pSubNode = pRootNode->mppSubNode[ixSubNode];
        if (0 != ((1 << gArch) & GetXmlNodeArchFlags(pSubNode)))
        {
            if (pSubNode->Name.mLen == 5)
            {
                if (0 == K2ASC_CompInsLen("copts", pSubNode->Name.mpChars, 5))
                {
                    pWork = new char[(outLen + pSubNode->Content.mLen + 1) + 4 & ~3];
                    K2_ASSERT(NULL != pWork);
                    outLen = K2ASC_Printf(pWork, "%s%.*s ", pTemp, pSubNode->Content.mLen, pSubNode->Content.mpChars);
                    delete[] pTemp;
                    pTemp = pWork;
                }
            }
            else if (pSubNode->Name.mLen == 7)
            {
                if (0 == K2ASC_CompInsLen("include", pSubNode->Name.mpChars, 7))
                {
                    if (!GetContentTarget(apSrcXml->mpVfsFile->mpParentFolder, pSubNode, &pIncPath))
                    {
                        printf("*** could not resolve include target to vfs path [%.*s]\n",
                            pSubNode->Content.mLen, pSubNode->Content.mpChars);
                    }
                    else
                    {
                        pWork = new char[(outLen + 3 + K2ASC_Len(pIncPath) + 1) + 4 & ~3];
                        K2_ASSERT(NULL != pWork);
                        outLen = K2ASC_Printf(pWork, "%s-I %s ", pTemp, pIncPath);
                        delete[] pTemp;
                        pTemp = pWork;
                    }
                }
                else if (0 == K2ASC_CompInsLen("cppopts", pSubNode->Name.mpChars, 7))
                {
                    pWork = new char[(outLen + pSubNode->Content.mLen + 1) + 4 & ~3];
                    K2_ASSERT(NULL != pWork);
                    outLen = K2ASC_Printf(pWork, "%s%.*s ", pTemp, pSubNode->Content.mLen, pSubNode->Content.mpChars);
                    delete[] pTemp;
                    pTemp = pWork;
                }
            }
        }
    }

    outLen = K2ASC_Printf(gStrBuf,
        spBuildSpec,
        gpStr_OptCPP,
        isInOsPath ? gpStr_OsInc : "",
        pTemp,
        apObjFilePath,
        apDepFilePath,
        apObjFilePath,
        pTargetOnly);

    delete[] pTemp;
    delete[] pTargetOnly;

    pCmdLine = new char[(outLen + 4) & ~3];
    K2_ASSERT(NULL != pCmdLine);
    K2ASC_CopyLen(pCmdLine, gStrBuf, outLen);
    pCmdLine[outLen] = 0;

    pEnv = gpBaseEnviron->CopyBlob(&blobLen);
    K2_ASSERT(NULL != pEnv);

    outLen = 0;
    pOutput = NULL;
//    printf("%s\n", pCmdLine);
    printf("Compile CPP [%s]\n", mpVfsFile->mpNameZ);
    mLastBuildResult = RunProgram(gpVfsRootSpec, pCmdLine, pEnv, &pOutput, &outLen);
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

    if (0 != mLastBuildResult)
    {
        printf("error %d\n", mLastBuildResult);
        return false;
    }

    return true;
}

bool
BuildFileUser_SrcCode::Launch_Assembler(
    BuildFileUser_SrcXml *  apSrcXml,
    char const *            apDepFilePath,
    char const *            apObjFilePath
)
{
    // gcc -x assembler-with-cpp -E $(GCCOPT) -D_ASSEMBLER -MT '$(K2_OBJECT_PATH)/$(notdir $@)' -MD -MF $(K2_OBJECT_PATH)/$(basename $<).d -o $(K2_OBJECT_PATH)/$(basename $<).s $<
    // gcc $(GCCOPT) -o $(K2_OBJECT_PATH)/$(basename $<).o $(K2_OBJECT_PATH)/$(basename $<).s

    static char const *spPreSpec = "gcc -x assembler-with-cpp -E %s %s -D_ASSEMBLER -MT %s -MD -MF %s -o %s %s";
    static char const *spBuildSpec = "gcc %s %s -o %s %s";
    UINT_PTR    objLen;
    char *      pCmdLine;
    char *      pTargetOnly;
    UINT_PTR    blobLen;
    char *      pEnv;
    char *      pOutput;
    UINT_PTR    outLen;
    char *      pEnd;
    char        ch;
    bool        isInOsPath;

    objLen = K2ASC_Copy(gStrBuf, apObjFilePath);
    pEnd = gStrBuf + objLen;
    do {
        --pEnd;
        ch = *pEnd;
        if (ch == '.')
            break;
    } while (pEnd != gStrBuf);
    K2_ASSERT(pEnd != gStrBuf);
    pEnd++;
    *pEnd = 's';
    pEnd++;
    *pEnd = 0;

    char *pTweenS = new char[(objLen + 8) & ~3];
    K2_ASSERT(NULL != pTweenS);
    K2ASC_Copy(pTweenS, gStrBuf);

    pTargetOnly = mpVfsFile->AllocFullPath();

    K2ASC_Printf(gStrBuf, "src\\%s\\", gpOsVer);
    isInOsPath = (0 == K2ASC_CompInsLen(pTargetOnly + gVfsRootSpecLen + 1, gStrBuf, 5 + K2ASC_Len(gpOsVer))) ? true : false;

    outLen = K2ASC_Printf(gStrBuf,
        spPreSpec,
        gpStr_OptC,
        isInOsPath ? gpStr_OsInc : "",
        apObjFilePath,
        apDepFilePath,
        pTweenS,
        pTargetOnly);

    delete[] pTargetOnly;

    pCmdLine = new char[(outLen + 4) & ~3];
    K2_ASSERT(NULL != pCmdLine);
    K2ASC_CopyLen(pCmdLine, gStrBuf, outLen);
    pCmdLine[outLen] = 0;

    pEnv = gpBaseEnviron->CopyBlob(&blobLen);
    K2_ASSERT(NULL != pEnv);

    outLen = 0;
    pOutput = NULL;
//    printf("%s\n", pCmdLine);
    printf("Preprocess ASM [%s]\n", mpVfsFile->mpNameZ);
    mLastBuildResult = RunProgram(gpVfsRootSpec, pCmdLine, pEnv, &pOutput, &outLen);
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

    delete[] pCmdLine;

    if (0 != mLastBuildResult)
    {
        delete[] pEnv;
        delete[] pTweenS;
        printf("error %d\n", mLastBuildResult);
        return false;
    }

    //
    // preprocessed assembly file successfully generated to 'pTweenS'
    //

    outLen = K2ASC_Printf(gStrBuf,
        spBuildSpec,
        gpStr_OptC,
        isInOsPath ? gpStr_OsInc : "",
        apObjFilePath,
        pTweenS);

    pCmdLine = new char[(outLen + 4) & ~3];
    K2_ASSERT(NULL != pCmdLine);
    K2ASC_CopyLen(pCmdLine, gStrBuf, outLen);
    pCmdLine[outLen] = 0;

    outLen = 0;
    pOutput = NULL;
    //    printf("%s\n", pCmdLine);
    printf("Assemble ASM [%s]\n", mpVfsFile->mpNameZ);
    mLastBuildResult = RunProgram(gpVfsRootSpec, pCmdLine, pEnv, &pOutput, &outLen);
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
    delete[] pTweenS;

    if (0 != mLastBuildResult)
    {
        printf("error %d\n", mLastBuildResult);
        return false;
    }

    return true;
}

