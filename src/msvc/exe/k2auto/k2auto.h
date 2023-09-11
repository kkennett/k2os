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
#ifndef __K2AUTO_H
#define __K2AUTO_H

#include "k2vfs.h"

//
//--------------------------------------------------------------------------------------------------
// 

#define ALL_ARCH_FLAGS ((1 << K2_ARCH_X32) | (1 << K2_ARCH_A32) | (1 << K2_ARCH_X64) | (1 << K2_ARCH_A64))
#define STRBUF_CHARS    4096

extern char const *         gpArchName[5];
extern UINT_PTR             gArch;
extern bool                 gDebugMode;
extern char const *         gpArchMode;
extern UINT_PTR const       gArchModeLen;
extern char const * const   gArchBoot[5];

extern char const * const   gpOsVer;

extern char const * const   gpBuildXmlFileName;

extern char *               gpStr_OptC;
extern char *               gpStr_OptCPP;
extern char *               gpStr_OptAsm;
extern char *               gpStr_OsInc;
extern char *               gpStr_LibGcc;
extern char *               gpStr_LdOpt;

extern Environ *            gpBaseEnviron;

extern char                 gStrBuf[STRBUF_CHARS];

extern K2LIST_ANCHOR        gSourceFileList;

//
//--------------------------------------------------------------------------------------------------
// 

bool        CreatePath(char *apPathZ, UINT_PTR aPathLen);
void        Spaces(UINT_PTR aCount);
void        DeDoubleDot(char *apPath);
UINT_PTR    GetXmlNodeArchFlags(K2XML_NODE const *apXmlNode);
bool        GetContentTarget(VfsFolder *apFolder, K2XML_NODE const *apXmlNode, char **appRetSrcPath);

//
//--------------------------------------------------------------------------------------------------
// 

enum BuildFileUserType 
{
    BuildFileUserType_Invalid = 0,

    BuildFileUserType_SrcXml,
    BuildFileUserType_SrcCode,
    BuildFileUserType_SrcInc,
    BuildFileUserType_SrcInf,

    BuildFileUserType_TmpObj,
    BuildFileUserType_TmpDep,
    BuildFileUserType_TmpElf,
    BuildFileUserType_TmpExpObj,

    BuildFileUserType_OutObj,
    BuildFileUserType_OutLib,
    BuildFileUserType_OutXdl,
    BuildFileUserType_OutXdlLib,

    BuildFileUserType_ImgDstXdl,
    BuildFileUserType_ImgSrcBoot,
    BuildFileUserType_ImgDstBoot,
    BuildFileUserType_Img,

    BuildFileUserType_Count
};

class BuildFileUser : public VfsUser
{
    bool mIsDamaged;
    bool mSomethingChangedSinceLastTryRepair;

public:
    BuildFileUser(VfsFile *apVfsFile, BuildFileUserType aFileType);
    ~BuildFileUser(void);

    bool const &    IsDamaged(void) { return mIsDamaged; }

    bool const &    GetSomethingChangedSinceLastTryRepair(void) const { return mSomethingChangedSinceLastTryRepair; }
    virtual void    SetSomethingChangedSinceLastTryRepair(void) { mSomethingChangedSinceLastTryRepair = true; }

    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);
    void OnFsDelete(char const *apFullPath);

    virtual void OnDamaged(void);
    virtual void OnRepaired(void);
    virtual bool TryRepair(void);

    BuildFileUserType const mFileType;
};

//
//--------------------------------------------------------------------------------------------------
// 

class BuildFileUser_SrcXml;
class BuildFileUser_SrcCode;
class BuildFileUser_SrcInc;
class BuildFileUser_SrcInf;
class BuildFileUser_TmpObj;
class BuildFileUser_TmpDep;
class BuildFileUser_TmpElf;
class BuildFileUser_TmpExpObj;
class BuildFileUser_OutObj;
class BuildFileUser_OutLib;
class BuildFileUser_OutXdl;
class BuildFileUser_OutXdlLib;
class BuildFileUser_ImgDstXdl;
class BuildFileUser_ImgSrcBoot;
class BuildFileUser_ImgDstBoot;
class BuildFileUser_Img;

class ProjDep;

enum ProjectType 
{
    ProjectType_Invalid = 0,

    ProjectType_Obj,
    ProjectType_Lib,
    ProjectType_Xdl,
    ProjectType_Img,

    ProjectType_Count
};

class BuildFileUser_SrcXml : public BuildFileUser
{
    ~BuildFileUser_SrcXml(void);

    void OnFsCreate(char const *apFullPath) { return OnFsCreate(apFullPath, NULL); }
    void OnFsCreate(char const *apFullPath, bool *apRetArchMismatch);

    void OnFsUpdate(char const *apFullPath);
    void OnFsDelete(char const *apFullPath);

    void OnChange(bool aIsDamage);
    void Build(char const *apFullPath, bool *apRetArchMismatch);
    void Construct(char const *apFullPath, bool *apRetArchMismatch);
    void Destruct(void);

    bool Construct_Obj(char const *apFullPath);
    void Destruct_Obj(void);

    bool Construct_Lib(char const *apFullPath);
    void Destruct_Lib(void);

    bool Construct_Xdl(char const *apFullPath);
    void Destruct_Xdl(void);

    bool Construct_Img(char const *apFullPath);
    void Destruct_Img(void);

    bool EnumSources(char const *apFullPath);

    void PurgeProjDepList(K2LIST_ANCHOR *apList);
    ProjDep * AddProjectByPath(char const *apFullPath, char const *apTargetSubPath, ProjectType aProjectType, K2LIST_ANCHOR *apList);
    ProjDep * AddProject(char const *apFullPath, K2XML_NODE const *apXmlNode, ProjectType aProjectType, K2LIST_ANCHOR *apList);

    UINT_PTR mRefCount;

public:
    static void DiscoveredNew(char const *apSubFilePath, char const *apNameOnly);
    static bool Eval(void);

    static K2LIST_ANCHOR    sSrcXmlList;

    BuildFileUser_SrcXml(VfsFile *apVfsFile, char *apSubPath, UINT_PTR aSubPathLen);

    void OnDamaged(void);
    void OnRepaired(void);
    bool TryRepair(void);

    void AddRef(void);
    void Release(void);

    bool CheckIfDamaged(void);

    void OnDepDamaged(BuildFileUser_SrcXml *apDep);
    void OnDepRepaired(BuildFileUser_SrcXml *apDep);

    void OnOutputDamaged(void);
    void OnOutputRepaired(void);

    void Dump(bool aDamagedOnly);

    K2LIST_LINK     SrcXmlListLink;

    ProjectType     mProjectType;
    bool            mIsKernelTarget;
    UINT64          mEventIter;
    K2LIST_ANCHOR   ProjDepList;

    char * const    mpSubPath;
    UINT_PTR const  mSubPathLen;

    char *          mpFileData;
    UINT_PTR        mFileBytes;
    bool            mParseOk;
    K2XML_PARSE     XmlParse;

    UINT_PTR        mSourcesCount;

    union {
        struct {
            BuildFileUser_OutObj *  mpOutObj;
        } Obj;
        struct {
            BuildFileUser_OutLib *  mpOutLib;
            K2LIST_ANCHOR           ObjProjList;
        } Lib;
        struct {
            BuildFileUser_OutXdl *  mpOutXdl;
            K2LIST_ANCHOR           ObjProjList;
            K2LIST_ANCHOR           LibProjList;
            K2LIST_ANCHOR           XdlProjList;
            bool                    mIsCrt;
        } Xdl;
        struct {
            BuildFileUser_Img *     mpImg;
            ProjDep *               mpKernXdl;
            ProjDep *               mpPlatXdl;
            ProjDep *               mpKernCrtXdl;
            ProjDep *               mpKernAcpiXdl;
            ProjDep *               mpKernExecXdl;
            ProjDep *               mpUserCrtXdl;
            ProjDep *               mpUserSysProcXdl;
            K2LIST_ANCHOR           RawXdlProjList;
            K2LIST_ANCHOR           UserXdlProjList;
            K2LIST_ANCHOR           BuiltInXdlProjList;
            VfsFolder *             mpImgFolder;
        } Img;
    } Proj;
};

class ProjDep
{
public:
    ProjDep(BuildFileUser_SrcXml *apOwner, K2LIST_ANCHOR *apOwnerList, BuildFileUser_SrcXml *apDependOn);
    ~ProjDep(void);

    UINT64                          mEventIter;

    BuildFileUser_SrcXml *          mpOwner;
    K2LIST_ANCHOR *                 mpOwnerList;
    K2LIST_LINK                     OwnerListLink;
    K2XML_NODE const *              mpOwnerXmlNode;

    BuildFileUser_SrcXml * const    mpDependOn;
    K2LIST_LINK                     ProjDepListLink;
};

//
//--------------------------------------------------------------------------------------------------
// 

class BuildFileUser_SrcInc : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);
    void OnFsDelete(char const *apFullPath);

public:
    BuildFileUser_SrcInc(VfsFile *apVfsFile, BuildFileUser_TmpDep *apParentTmpDep);
    ~BuildFileUser_SrcInc(void);

    void SetSomethingChangedSinceLastTryRepair(void);

    void Dump(void);

    BuildFileUser_TmpDep * const    mpParentTmpDep;
    K2LIST_LINK                     ParentIncDepListLink;
};

class BuildFileUser_SrcCode : public BuildFileUser
{
    bool Launch_C_Compile(BuildFileUser_SrcXml *apSrcXml, char const *apDepFilePath, char const *apObjFilePath);
    bool Launch_CPP_Compile(BuildFileUser_SrcXml *apSrcXml, char const *apDepFilePath, char const *apObjFilePath);
    bool Launch_Assembler(BuildFileUser_SrcXml *apSrcXml, char const *apDepFilePath, char const *apObjFilePath);

    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);

public:
    BuildFileUser_SrcCode(VfsFile *apVfsFile, K2XML_NODE const *apSrcXmlNode);
    ~BuildFileUser_SrcCode(void);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);

    void Dump(void);

    bool Build(void);

    int                         mLastBuildResult;
    K2XML_NODE const * const    mpParentSrcXmlNode;
    BuildFileUser_TmpDep *      mpParentTmpDep;

    K2LIST_LINK                 SrcFileListLink;    // on gSourceFileList
};

class BuildFileUser_TmpDep : public BuildFileUser
{
    friend class BuildFileUser_SrcXml;

    bool ParseLine(char const * apFullPathOfDepFile, BuildFileUser_SrcXml *apSrcXml, BuildFileUser_SrcCode *apSrcCode, char const *apSrcCodePath, char const *apLine, UINT_PTR aLineLen);
    void BuildGeneratedDeps(char const *apFullPath);
    void TearDownGeneratedDeps(char const *apFullPath);

    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);
    void OnFsDelete(char const *apFullPath);

public:
    BuildFileUser_TmpDep(VfsFile *apVfsFile, BuildFileUser_SrcCode *apSrcCode);
    ~BuildFileUser_TmpDep(void);

    static BuildFileUser_TmpDep * Construct(char const *apSrcCodePath, VfsFile *apSrcCodeVfsFile, BuildFileUser_SrcXml *apSrcXml, K2XML_NODE const *apSrcXmlNode, char const *apXmlFullPath);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);
    void OnRepaired(void);
    bool TryRepair(void) { return TryRepair(false); }
    bool TryRepair(bool aForce);

    bool CheckIfDamaged(void);

    void OnSrcCodeDamaged(void);
    void OnSrcCodeRepaired(void);
    
    void OnSrcIncDamaged(BuildFileUser_SrcInc *apSrcInc);
    void OnSrcIncRepaired(BuildFileUser_SrcInc *apSrcInc);

    void Dump(bool aDamagedOnly);

    bool                            mInDepBuild;
    BuildFileUser_SrcCode * const   mpChildSrcCode;
    BuildFileUser_TmpObj *          mpParentTmpObj;
    bool                            mDepParseProblem;
    K2LIST_ANCHOR                   GeneratedSrcIncDepList;
};

class BuildFileUser_TmpObj : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);

public:
    BuildFileUser_TmpObj(VfsFile *apVfsFile, BuildFileUser_TmpDep *apTmpDep, BuildFileUser_SrcXml *apSrcXml);
    ~BuildFileUser_TmpObj(void);

    static BuildFileUser_TmpObj * Construct(char const *apSrcCodePath, VfsFile *apSrcCodeVfsFile, BuildFileUser_SrcXml *apSrcXml, K2XML_NODE const *apSrcXmlNode, char const *apXmlFullPath);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);
    void OnRepaired(void);
    bool TryRepair(void);

    bool CheckIfDamaged(void);

    void Dump(bool aDamagedOnly);

    BuildFileUser_SrcXml * const    mpParentSrcXml;
    BuildFileUser_TmpDep * const    mpChildTmpDep;
    K2LIST_LINK                     SrcXmlProjOutputTmpObjListLink;
};

//
//--------------------------------------------------------------------------------------------------
// 

class BuildFileUser_OutObj : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);

public:
    BuildFileUser_OutObj(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml, BuildFileUser_TmpObj *apTmpObj);
    ~BuildFileUser_OutObj(void);

    static BuildFileUser_OutObj * Construct(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml, char const *apXmlFullPath);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);
    void OnRepaired(void);
    bool TryRepair(void);

    void OnChildDamaged(BuildFileUser_TmpObj *apChildTmpObj);
    void OnChildRepaired(BuildFileUser_TmpObj *apChildTmpObj);

    bool CheckIfDamaged(void);

    void Dump(bool aDamagedOnly);

    BuildFileUser_SrcXml * const mpParentSrcXml;
    BuildFileUser_TmpObj * const mpChildTmpObj;
};

//
//--------------------------------------------------------------------------------------------------
// 

class BuildFileUser_OutLib : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);

public:
    BuildFileUser_OutLib(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml, K2LIST_ANCHOR *apTmpObjList);
    ~BuildFileUser_OutLib(void);

    static BuildFileUser_OutLib * Construct(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml, char const *apXmlFullPath);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);
    void OnRepaired(void);
    bool TryRepair(void);

    void OnChildDamaged(BuildFileUser_TmpObj *apChildTmpObj);
    void OnChildRepaired(BuildFileUser_TmpObj *apChildTmpObj);

    bool CheckIfDamaged(void);

    void Dump(bool aDamagedOnly);

    BuildFileUser_SrcXml * const    mpParentSrcXml;
    K2LIST_ANCHOR                   TmpObjList;
};

//
//--------------------------------------------------------------------------------------------------
// 

class BuildFileUser_SrcInf : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);
    void OnFsDelete(char const *apFullPath);

public:
    BuildFileUser_SrcInf(VfsFile *apVfsFile);
    ~BuildFileUser_SrcInf(void);

    void SetSomethingChangedSinceLastTryRepair(void);

    void Dump(void);

    BuildFileUser_TmpExpObj * mpParentTmpExpObj;
};

class BuildFileUser_TmpExpObj : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);

public:
    BuildFileUser_TmpExpObj(VfsFile *apVfsFile, BuildFileUser_SrcInf *apSrcInf, K2LIST_ANCHOR * apTmpObjList);
    ~BuildFileUser_TmpExpObj(void);

    static BuildFileUser_TmpExpObj * Construct(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml, VfsFile *apInfVfsFile, char const *apXmlFullPath);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);
    void OnRepaired(void);
    bool TryRepair(void);

    void OnSrcInfDamaged(void);
    void OnSrcInfRepaired(void);

    bool CheckIfDamaged(void);

    void Dump(bool aDamagedOnly);

    BuildFileUser_TmpElf *  mpParentTmpElf;
    BuildFileUser_SrcInf *  mpChildSrcInf;
    K2LIST_ANCHOR           TmpObjList;
};

class BuildFileUser_TmpElf : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);

public:

    BuildFileUser_TmpElf(VfsFile *apVfsFile, BuildFileUser_TmpExpObj *apTmpExpObj);
    ~BuildFileUser_TmpElf(void);

    static BuildFileUser_TmpElf * Construct(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml, VfsFile *apInfVfsFile, char const *apXmlFullPath);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);
    void OnRepaired(void);
    bool TryRepair(void);

    bool CheckIfDamaged(void);

    void Dump(bool aDamagedOnly);

    void OnChildDamaged(BuildFileUser_TmpObj *apChildTmpObj);
    void OnChildRepaired(BuildFileUser_TmpObj *apChildTmpObj);

    BuildFileUser_OutXdlLib *       mpParentOutXdlLib;
    BuildFileUser_TmpExpObj * const mpChildTmpExpObj;
};

class BuildFileUser_OutXdlLib : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);

public:
    BuildFileUser_OutXdlLib(VfsFile *apVfsFile, BuildFileUser_TmpElf *apTmpElf);
    ~BuildFileUser_OutXdlLib(void);

    static BuildFileUser_OutXdlLib * Construct(VfsFile *apVfsFile, BuildFileUser_SrcXml *apParentSrcXml, VfsFile *apInfVfsFile, char const *apXmlFullPath);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);
    void OnRepaired(void);
    bool TryRepair(void) { return TryRepair(false); }
    bool TryRepair(bool aForce);

    bool CheckIfDamaged(void);

    void OnChildDamaged(void);
    void OnChildRepaired(void);

    void Dump(bool aDamagedOnly);

    BuildFileUser_OutXdl *          mpParentOutXdl;
    BuildFileUser_TmpElf * const    mpChildTmpElf;
};

class BuildFileUser_OutXdl : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);

public:
    BuildFileUser_OutXdl(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml, BuildFileUser_OutXdlLib *apChildXdlLib);
    ~BuildFileUser_OutXdl(void);

    static BuildFileUser_OutXdl * Construct(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml, VfsFile *apInfVfsFile, char const *apXmlFullPath);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);
    void OnRepaired(void);
    bool TryRepair(void);

    void OnChildDamaged(BuildFileUser_TmpObj *apChildTmpObj);
    void OnChildRepaired(BuildFileUser_TmpObj *apChildTmpObj);

    bool CheckIfDamaged(void);

    void Dump(bool aDamagedOnly);

    BuildFileUser_SrcXml * const    mpParentSrcXml;
    BuildFileUser_OutXdlLib *       mpChildOutXdlLib;
};

//
//--------------------------------------------------------------------------------------------------
// 

class BuildFileUser_ImgDstXdl : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);

public:
    BuildFileUser_ImgDstXdl(VfsFile *apVfsFile, BuildFileUser_Img *apParentImg, BuildFileUser_SrcXml *apSrcProjSrcXml);
    ~BuildFileUser_ImgDstXdl(void);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);
    void OnRepaired(void);
    bool TryRepair(void);

    bool CheckIfDamaged(void);

    void Dump(bool aDamagedOnly);

    BuildFileUser_Img * const       mpParentImg;
    BuildFileUser_SrcXml * const    mpSrcProjSrcXml;
    K2LIST_LINK                     ImgTargetListLink;
};

class BuildFileUser_ImgSrcBoot : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);

public:
    BuildFileUser_ImgSrcBoot(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml);
    ~BuildFileUser_ImgSrcBoot(void);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);
    void OnRepaired(void);

    bool CheckIfDamaged(void);

    void Dump(bool aDamagedOnly);

    BuildFileUser_ImgDstBoot * mpDstBoot;
};

class BuildFileUser_ImgDstBoot : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);

public:
    BuildFileUser_ImgDstBoot(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml, BuildFileUser_ImgSrcBoot *apSrcBoot);
    ~BuildFileUser_ImgDstBoot(void);

    static BuildFileUser_ImgDstBoot * Construct(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml, char const *apXmlFullPath);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);
    void OnRepaired(void);
    bool TryRepair(void);

    void OnSrcDamaged(void);
    void OnSrcRepaired(void);

    bool CheckIfDamaged(void);

    void Dump(bool aDamagedOnly);

    BuildFileUser_Img *         mpParentImg;
    BuildFileUser_ImgSrcBoot *  mpSrcBoot;
};

class BuildFileUser_Img : public BuildFileUser
{
    void OnFsCreate(char const *apFullPath);
    void OnFsUpdate(char const *apFullPath);

public:
    BuildFileUser_Img(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml, BuildFileUser_ImgDstBoot *apImgRofs);
    ~BuildFileUser_Img(void);

    static BuildFileUser_Img * Construct(VfsFile *apVfsFile, BuildFileUser_SrcXml *apSrcXml, BuildFileUser_ImgDstBoot *apImgBoot, char const *apXmlFullPath);

    void SetSomethingChangedSinceLastTryRepair(void);

    void OnDamaged(void);
    void OnRepaired(void);
    bool TryRepair(void);

    void OnBootDamaged(void);
    void OnBootRepaired(void);

    void OnDepDamaged(BuildFileUser_SrcXml *apDep);

    void OnImgDstXdlDamaged(BuildFileUser_ImgDstXdl *apImgXdl);
    void OnImgDstXdlRepaired(BuildFileUser_ImgDstXdl *apImgXdl);

    bool CheckIfDamaged(void);

    void Dump(bool aDamagedOnly);

    BuildFileUser_SrcXml *  mpParentSrcXml;
    BuildFileUser_ImgDstBoot * mpChildImgBoot;
    K2LIST_ANCHOR           ImgTargetList;
};

//
//--------------------------------------------------------------------------------------------------
// 

#endif // __K2AUTO_H
