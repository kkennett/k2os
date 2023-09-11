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
#ifndef __K2VFS_H
#define __K2VFS_H

#include <stdio.h>
#include <Windows.h>
#include <lib/k2atomic.h>
#include <lib/k2win32.h>
#include <lib/k2xml.h>
#include <lib/k2asc.h>
#include <lib/k2xml.h>
#include <lib/k2mem.h>
#include <lib/k2list.h>
#include <lib/k2tree.h>
#include <lib/k2parse.h>

typedef bool (*Vfs_pf_Eval)(void);
typedef void (*Vfs_pf_OnDiscoverNewFile)(char const *apRootSubFilePath, char const *apNameOnly);

class VfsNode;
class VfsFolder;
class VfsFile;
class VfsUser;

enum VfsFileEvent
{
    VfsFileEvent_Invalid = 0,

    VfsFileEvent_Discovered,
    VfsFileEvent_Created,
    VfsFileEvent_Updated,
    VfsFileEvent_Deleted,

    VfsFileEvent_Count,
};

class VfsNode
{
    friend class VfsFolder;
    friend class VfsFile;
    friend class VfsUser;
    friend void Vfs_Run(Vfs_pf_Eval aEvalFunc, Vfs_pf_OnDiscoverNewFile aOnDiscoverNew);

    static K2TREE_ANCHOR WorkTree;

    UINT_PTR    mRefCount;
    UINT64      mVersion;
    bool        mExistsInFs;
    UINT_PTR    mFullPathLen;
    UINT_PTR    mExtOffset;
    K2TREE_NODE ParentsChildTreeNode;
    UINT64      mLastIter;

    static int sTimeCompare(UINT_PTR aKey, K2TREE_NODE *apNode);
    static int sNameCompare(UINT_PTR aKey, K2TREE_NODE *apTreeNode);

    VfsNode(VfsFolder *apParentFolder, char * apNameZ, UINT_PTR aNameLen, bool aExistsInFs, bool aIsFolder);
    virtual ~VfsNode(void);

public:
    virtual void        AddRef(void);
    virtual void        Release(void);

    UINT_PTR            GenName(char *apFullPathBuf) const;
    char *              AllocFullPath(UINT_PTR aExtraChars = 0, UINT_PTR *apRetLen = NULL) const;

    UINT64 const &      GetVersion(void) const { return mVersion; }
    UINT_PTR const &    GetFullPathLen(void) const { return mFullPathLen; }
    UINT_PTR const &    GetExtOffset(void) const { return mExtOffset; }

    VfsFolder * const   mpParentFolder;
    bool const          mIsFolder;
    char * const        mpNameZ;
    UINT_PTR const      mNameLen;
    UINT_PTR const      mDepth;
};

class VfsFolder : public VfsNode
{
    friend class VfsNode;
    friend class VfsFile;
    friend class VfsUser;
    friend void Vfs_Run(Vfs_pf_Eval aEvalFunc, Vfs_pf_OnDiscoverNewFile aOnDiscoverNew);

    static bool         sgAfterFirstScan;
    static UINT64       sgCurIter;
    static VfsFolder *  sgpRoot;

    VfsFolder(VfsFolder *apParentFolder, char * apNameZ, UINT_PTR aNameLen, bool aExistsInFs);
    ~VfsFolder(void);

    void SubScan(char *apBuf, char *apEnd, FILETIME *apDeleteFileTime);
    void FsFolderTreeDeleted(char *apBuf, char *apEnd, FILETIME const *apFileTime);
    void FsFileTreeDeleted(char *apBuf, char *apEnd, FILETIME const *apFileTime);

    K2TREE_ANCHOR FolderChildTree;
    K2TREE_ANCHOR FileChildTree;

public:
    static VfsFolder & Root(void) { return *sgpRoot; }

    VfsNode * AcquireOrCreateSub(char const *apNameZ, UINT_PTR aNameLen, bool aIsFolder);
    VfsNode * AcquireOrCreateSub(char const *apNameZ, bool aIsFolder)
    {
        K2_ASSERT(NULL != apNameZ);
        return AcquireOrCreateSub(apNameZ, K2ASC_Len(apNameZ), aIsFolder);
    }
};

enum VfsFileState 
{
    VfsFileState_DoesNotExist = 0,
    VfsFileState_DiscoverPending,
    VfsFileState_Inert,
    VfsFileState_CreatePending,
    VfsFileState_UpdatePending,
    VfsFileState_DeletePending,

    VfsFileState_Count
};

class VfsFile : public VfsNode
{
    friend class VfsNode;
    friend class VfsFolder;
    friend class VfsUser;
    friend void Vfs_Run(Vfs_pf_Eval aEvalFunc, Vfs_pf_OnDiscoverNewFile aOnDiscoverNew);

    VfsFile(VfsFolder *apParentFolder, char *apNameZ, UINT_PTR aNameLen, bool aExistsInFs);
    ~VfsFile(void);

    void OnFsEvent(VfsFileEvent aEvent);
    void OnFsExec(Vfs_pf_OnDiscoverNewFile aOnDiscoverNew);

    bool Exists(void) const;    // only VfsUser should call this; outside a usage, the file has no state

    K2LIST_ANCHOR   UserList;
    FILETIME        mTimeStamp;
    UINT64          mEventIter;
    VfsFileState    mFileState;
    bool            mInWorkTree;    // sanity
    K2TREE_NODE     WorkTreeNode;

public:
    void AddRef(void);
    void Release(void);

    void                DeleteInFs(void);
    FILETIME const &    GetTimeStamp(void) const { return mTimeStamp; }
    UINT_PTR const &    GetUserCount(void) const { return UserList.mNodeCount; }
    VfsUser *           GetUserByIndex(UINT_PTR aIndex) const;
    bool                IsNewerThan(VfsFile const *apOtherFile) const;
    bool                IsOlderThan(VfsFile const *apOtherFile) const;
};

class VfsUser
{
    friend class VfsFile;

    K2LIST_LINK UserListLink;
    UINT64      mEventIter;

public:
    VfsUser(VfsFile *apVfsFile);
    virtual ~VfsUser(void);

    virtual void OnFsCreate(char const *apFullPath) {}
    virtual void OnFsUpdate(char const *apFullPath) {}
    virtual void OnFsDelete(char const *apFullPath) {}

    bool FileExists(void) { return mpVfsFile->Exists(); }

    VfsFile * const mpVfsFile;
};

void Vfs_Run(Vfs_pf_Eval aEvalFunc, Vfs_pf_OnDiscoverNewFile aOnDiscoverNew);

extern char const *    gpVfsRootSpec;
extern UINT_PTR        gVfsRootSpecLen;

#endif // __K2VFS_H
