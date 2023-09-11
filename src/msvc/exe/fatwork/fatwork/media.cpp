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

#include "fatwork.h"

#define K2OS_FILESYS_ID_DEFAULT                 1
#define K2OS_FILESYS_DESC_FRIENDLY_CHARCOUNT    32

#define K2OS_FOLDERPROP_NAME_CHARCOUNT          256
#define K2OS_FILEPROP_NAME_CHARCOUNT            256

typedef struct _K2OS_FILESYS_DESC K2OS_FILESYS_DESC;
struct _K2OS_FILESYS_DESC
{
    char        mFriendly[K2OS_FILESYS_DESC_FRIENDLY_CHARCOUNT];
    UINT_PTR    mId;
};

typedef enum _K2OSFS_Type K2OSFS_Type;
enum _K2OSFS_Type
{
    K2OSFs_Type_Invalid = 0,
    K2OSFs_Type_Builtin,
    K2OSFs_Type_FAT12,
    K2OSFs_Type_FAT16,
    K2OSFs_Type_FAT32,

    K2OSFs_Type_Count
};

#define K2OS_FILE_ATTR_READONLY                 1
#define K2OS_FILE_ATTR_HIDDEN                   2
#define K2OS_FILE_ATTR_SYSTEM                   4
#define K2OS_FILE_ATTR_RESERVED                 8
#define K2OS_FILE_ATTR_FOLDER                   16
#define K2OS_FILE_ATTR_ARCHIVE                  32

#define K2OS_FILE_ACCESS_READ                   PF_R
#define K2OS_FILE_ACCESS_WRITE                  PF_W
#define K2OS_FILE_ACCESS_EXEC                   PF_X
#define K2OS_FILE_ACCESS_NO_CACHE               0x80000000
#define K2OS_FILE_ACCESS_WRITE_THROUGH          0x40000000
#define K2OS_FILE_ACCESS_TRUNCATE               (0x20000000 | K2OS_FILE_ACCESS_WRITE)   // only allowed with Open()

#define K2OS_FILE_SHARE_READ                    PF_R
#define K2OS_FILE_SHARE_WRITE                   PF_W

#define K2OS_FOLDERPROP_NAME_CHARCOUNT          256
#define K2OS_FILEPROP_NAME_CHARCOUNT            256

typedef struct _K2OS_FILE_PROP K2OS_FILE_PROP;
struct _K2OS_FILE_PROP
{
    UINT64      mSizeBytes;
    UINT64      mTimeCreated;
    UINT64      mTimeLastModified;
    UINT32      mAttrib;
    UINT32      mNameLen;
    char        mName[K2OS_FILEPROP_NAME_CHARCOUNT];
};

typedef struct _K2OS_FOLDER_PROP K2OS_FOLDER_PROP;
struct _K2OS_FOLDER_PROP
{
    UINT64      mTimeCreated;
    UINT64      mTimeLastModified;
    UINT32      mAttrib;
    UINT32      mNameLen;
    char        mName[K2OS_FOLDERPROP_NAME_CHARCOUNT];
};

typedef enum _K2OS_FileSeekType K2OS_FileSeekType;
enum _K2OS_FileSeekType
{
    K2OS_FileSeek_Begin = 0,
    K2OS_FileSeek_CurPos,
    K2OS_FileSeek_End,

    K2OS_FileSeekType_Count
};

struct IFolder;
struct IFolderEnum;
struct IFile;

struct IFsItem : public IBase
{
    virtual K2STAT      OpenParent(IFolder **appRetParent) = 0;
    virtual UINT_PTR    GetName(char *apRetName, UINT_PTR aNameBufferChars) = 0;
};

struct IFolder : public IFsItem
{
    virtual K2STAT GetFolderProp(K2OS_FOLDER_PROP *apRetProp) = 0;

    virtual K2STAT SubCreate(char const *apFolderPath, UINT_PTR aAttr, IFolder **apRetSub) = 0;
    virtual K2STAT SubOpen(char const *apSubFolderPath, IFolder **apRetSub) = 0;
    virtual K2STAT SubDelete(char const *apFolderPath) = 0;

    virtual K2STAT FileOpen(char const *apFilePath, BOOL aAlways, UINT_PTR aFileShareFlags, UINT_PTR aFileAccessFlags, IFile **appRetFile) = 0;
    virtual K2STAT FileCreate(char const *apFilePath, BOOL aAlways, UINT_PTR aFileAttr, UINT_PTR aFileShareFlags, UINT_PTR aFileAccessFlags, IFile **appRetFile) = 0;
    virtual K2STAT FileDelete(char const *apFilePath) = 0;
    virtual K2STAT FileGetAttr(char const *apFilePath, UINT_PTR *apRetAttrib) = 0;
    virtual K2STAT FileSetAttr(char const *apFilePath, UINT_PTR aAttrib) = 0;

    virtual K2STAT EnumCreate(char const *apMatchSpec, IFolderEnum **appRetEnum) = 0;
};

struct IFolderEnum : public IBase
{
    virtual K2STAT GetCount(UINT_PTR *apRetFolderCount, UINT_PTR *apRetFileCount) = 0;
    virtual K2STAT GetFileProp(UINT_PTR aFileIndex, K2OS_FILE_PROP *apRetFileProp) = 0;
    virtual K2STAT GetFolderProp(UINT_PTR aFolderIndex, K2OS_FOLDER_PROP *apRetFolderProp) = 0;
};

struct IFile : public IFsItem
{
    virtual K2STAT GetFileProp(K2OS_FILE_PROP *apRetProp) = 0;
    virtual K2STAT GetSharing(UINT_PTR *apRetShareFlags) = 0;
    virtual K2STAT GetAccess(UINT_PTR *apRetAccess) = 0;

    virtual K2STAT Seek(K2OS_FileSeekType aFromWhere, INT64 const *apRelativeOffset, UINT64 *apRetNewPos) = 0;
    virtual K2STAT GetPos(UINT64 *apRetCurrentPos) = 0;

    virtual K2STAT Read(void *apBuffer, UINT_PTR aBytesToRead) = 0;
    virtual K2STAT Write(void const *apBuffer, UINT_PTR aBytesToWrite) = 0;

    virtual K2STAT Flush(void) = 0;
};

BOOL        K2OSFS_GetDesc(UINT_PTR aEntryIx, K2OS_FILESYS_DESC *apRetDesc);
IFolder *   K2OSFS_Open(UINT_PTR aFileSysId);


//
// --------------------------------------------------------------------
//

struct MyFolder;

struct MyFileSys
{
    MyFileSys(void);
    virtual ~MyFileSys(void);

    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    BOOL                mIsActive;
    INT_PTR             mRefs;
    K2OS_FILESYS_DESC   Desc;
    MyFolder *          mpRootFolder;
    MyFileSys *         mpNext;
    K2OS_RWLOCK         RwLock;
    IStorVolume *       mpVolume;
};

struct MyFolder : public IFolder
{
    MyFolder(void);
    virtual ~MyFolder(void);

    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    K2STAT      OpenParent(IFolder **appRetParent);
    UINT_PTR    GetName(char *apRetName, UINT_PTR aNameBufferChars);

    K2STAT GetFolderProp(K2OS_FOLDER_PROP *apRetProp);
    K2STAT SubCreate(char const *apFolderPath, UINT_PTR aAttr, IFolder **apRetSub);
    K2STAT SubOpen(char const *apSubFolderPath, IFolder **apRetSub);
    K2STAT SubDelete(char const *apFolderPath);
    K2STAT FileOpen(char const *apFilePath, BOOL aAlways, UINT_PTR aFileShareFlags, UINT_PTR aFileAccessFlags, IFile **appRetFile);
    K2STAT FileCreate(char const *apFilePath, BOOL aAlways, UINT_PTR aFileAttr, UINT_PTR aFileShareFlags, UINT_PTR aFileAccessFlags, IFile **appRetFile);
    K2STAT FileDelete(char const *apFilePath);
    K2STAT FileGetAttr(char const *apFilePath, UINT_PTR *apRetAttrib);
    K2STAT FileSetAttr(char const *apFilePath, UINT_PTR aAttrib);
    K2STAT EnumCreate(char const *apMatchSpec, IFolderEnum **appRetEnum);

    INT_PTR             mRefs;
    MyFolder *          mpParent;
    MyFileSys *         mpFileSys;
    K2OS_FOLDER_PROP    FolderProp;
    K2LIST_LINK         FolderListLink;
    K2LIST_ANCHOR       SubFolderList;
    K2LIST_ANCHOR       FileList;
};

struct MyFolderEnum : public IBase
{
    MyFolderEnum(void);
    virtual ~MyFolderEnum(void);

    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    INT_PTR             mRefs;
    UINT_PTR            mFolderCount;
    UINT_PTR            mFileCount;
    K2OS_FOLDER_PROP *  mpFolderProps;
    K2OS_FILE_PROP *    mpFileProps;
};

struct MyFile : public IFile
{
    MyFile(void);
    virtual ~MyFile(void);

    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    K2STAT      OpenParent(IFolder **appRetParent);
    UINT_PTR    GetName(char *apRetName, UINT_PTR aNameBufferChars);

    K2STAT GetFileProp(K2OS_FILE_PROP *apRetProp);
    K2STAT GetSharing(UINT_PTR *apRetShareFlags);
    K2STAT GetAccess(UINT_PTR *apRetAccess);
    K2STAT Seek(K2OS_FileSeekType aFromWhere, INT64 const *apRelativeOffset, UINT64 *apRetNewPos);
    K2STAT GetPos(UINT64 *apRetCurrentPos);
    K2STAT Read(void *apBuffer, UINT_PTR aBytesToRead);
    K2STAT Write(void const *apBuffer, UINT_PTR aBytesToWrite);
    K2STAT Flush(void);

    INT_PTR         mRefs;
    MyFolder *      mpParent;
    K2OS_FILE_PROP  FileProp;
    K2LIST_LINK     FileListLink;
};

K2OS_RWLOCK     gFileSysListRwLock;
MyFileSys *     gpFileSysList;








class MyStorAdapter;

class MyStorMediaSession : public IStorMediaSession
{
public:
    MyStorMediaSession(MyStorAdapter *apAdapter, UINT_PTR aSessionId, StorMediaProp const *apMediaProp, BOOL aIsReadOnly, UINT8 *apPartial);
    virtual ~MyStorMediaSession(void);
    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    UINT_PTR GetId(void)
    {
        return mSessionId;
    }
    
    BOOL GetMediaProp(StorMediaProp *apRetMedia, BOOL *apRetReadOnly)
    {
        if ((NULL == apRetMedia) && (NULL == apRetReadOnly))
        {
            SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
            return FALSE;
        }

        if (NULL != apRetMedia)
        {
            K2MEM_Copy(apRetMedia, &MediaProp, sizeof(StorMediaProp));
        }

        if (NULL != apRetReadOnly)
        {
            *apRetReadOnly = mIsReadOnly;
        }

        return TRUE;
    }
    
    BOOL AcquireAdapter(IStorAdapter **apRetAdapter);

    BOOL GetIsActive(void)
    {
        return mIsActive;
    }

    // these will fail if the session is not active
    UINT_PTR ReadBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount, void *apAlignedBuffer);
    UINT_PTR WriteBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount, void const *apAlignedBuffer, BOOL aWriteThrough);
    BOOL     FlushDirtyBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount);

    BOOL Deactivate(void)
    {
        BOOL result;

        result = FALSE;

        K2OS_RwLock_WriteLock(&RwLock);

        if (mIsActive)
        {
            mIsActive = FALSE;
            result = TRUE;
        }

        K2OS_RwLock_WriteUnlock(&RwLock);

        if (!result)
        {
            SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
        }

        return result;
    }

    MyStorAdapter * const   mpAdapter;
    UINT_PTR const          mSessionId;
    BOOL const              mIsReadOnly;
    StorMediaProp const     MediaProp;
    UINT_PTR const          mBuffersPerPage;

    UINT_PTR                mRefs;
    BOOL                    mIsActive;
    K2OS_RWLOCK             RwLock;
    UINT8 * const           mpPartAlloc;
    UINT8 * const           mpPartial;
};

class MyStorAdapter : public IStorAdapter
{
public:
    MyStorAdapter(BOOL aRemovable, BOOL aIsReadOnly)
    {
        mRefs = 1;
        InitializeCriticalSection(&Sec);
        mpActiveSession = NULL;
        mNextSessionId = 1;
        mIsRemovable = aRemovable;
        mIsReadOnly = aIsReadOnly;
        K2MEM_Zero(&CurMediaProp, sizeof(CurMediaProp));
    }

    virtual ~MyStorAdapter(void)
    {
        K2_ASSERT(NULL == mpActiveSession);
        DeleteCriticalSection(&Sec);
    }

    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    UINT_PTR GetFriendly(char *apBuffer, UINT_PTR aBufferLen)
    {
        if (NULL == apBuffer)
        {
            SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
            return 0;
        }
        return K2ASC_CopyLen(apBuffer, "VhdAdapter", aBufferLen);
    }

    BOOL GetMediaIsRemovable(void)
    {
        return mIsRemovable;
    }

    BOOL GetMediaProp(StorMediaProp *apRetProp, BOOL *apRetReadOnly)
    {
        BOOL result;

        EnterCriticalSection(&Sec);

        if (NULL != mpActiveSession)
        {
            if (NULL != apRetProp)
            {
                K2MEM_Copy(apRetProp, &CurMediaProp, sizeof(StorMediaProp));
            }
            if (NULL != apRetReadOnly)
            {
                *apRetReadOnly = mpActiveSession->mIsReadOnly;
            }
            result = TRUE;
        }
        else
        {
            result = FALSE;
        }

        LeaveCriticalSection(&Sec);

        if (!result)
        {
            SetLastError(K2STAT_ERROR_NO_MEDIA);
        }

        return result;
    }

    BOOL AcquireSession(IStorMediaSession **appRetSession);
    BOOL EjectMedia(UINT_PTR aMediaSessionId);
    BOOL InsertMedia(StorMediaProp const *apMediaProp, BOOL aIsReadOnly);

    BOOL TransferData(UINT_PTR aMediaSessionId, BOOL aIsWrite, UINT_PTR aPhysAddr, UINT_PTR aStartBlock, UINT_PTR aBlockCount);

    UINT_PTR                mRefs;
    CRITICAL_SECTION        Sec;
    UINT_PTR                mNextSessionId;
    BOOL                    mIsRemovable;
    BOOL                    mIsReadOnly;
    StorMediaProp           CurMediaProp;
    MyStorMediaSession *    mpActiveSession;
};

BOOL MyStorAdapter::InsertMedia(StorMediaProp const *apMediaProp, BOOL aIsReadOnly)
{
    BOOL    result;
    UINT64  check;
    UINT8 * pPartial;

    if (NULL == apMediaProp)
    {
        SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    check = ((UINT64)apMediaProp->mBlockCount) * ((UINT64)apMediaProp->mBlockSizeBytes);

    if ((0 == apMediaProp->mBlockCount) ||
        (0 == apMediaProp->mBlockSizeBytes) ||
        (0 != (apMediaProp->mBlockSizeBytes & (apMediaProp->mBlockSizeBytes - 1))) ||
        (check != apMediaProp->mTotalBytes))
    {
        SetLastError(K2STAT_ERROR_INVALID_MEDIA);
        return FALSE;
    }

    result = FALSE;

    pPartial = new UINT8[apMediaProp->mBlockSizeBytes * 2];
    if (NULL == pPartial)
    {
        SetLastError(K2STAT_ERROR_OUT_OF_MEMORY);
        return FALSE;
    }

    EnterCriticalSection(&Sec);

    if (NULL != mpActiveSession)
    {
        SetLastError(K2STAT_ERROR_MEDIA_PRESENT);
    }
    else
    {
        if (mIsReadOnly)
            aIsReadOnly = TRUE;
        mpActiveSession = new MyStorMediaSession(this, mNextSessionId++, apMediaProp, aIsReadOnly, pPartial);
        result = (NULL == mpActiveSession) ? FALSE : TRUE;
        if (result)
        {
            K2MEM_Copy(&CurMediaProp, apMediaProp, sizeof(StorMediaProp));
        }
    }

    LeaveCriticalSection(&Sec);

    if (!result)
    {
        delete[] pPartial;
    }

    return result;
}

BOOL MyStorAdapter::AcquireSession(IStorMediaSession **appRetSession)
{
    if (NULL == appRetSession)
    {
        SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    *appRetSession = NULL;

    EnterCriticalSection(&Sec);

    if (NULL == mpActiveSession)
    {
        LeaveCriticalSection(&Sec);
        SetLastError(K2STAT_ERROR_NO_MEDIA);
        return FALSE;
    }

    mpActiveSession->AddRef();
    *appRetSession = mpActiveSession;

    LeaveCriticalSection(&Sec);

    return TRUE;
}

BOOL MyStorAdapter::EjectMedia(UINT_PTR aMediaSessionId)
{
    BOOL                    result;
    MyStorMediaSession *    pSession;

    if (!mIsRemovable)
    {
        SetLastError(K2STAT_ERROR_MEDIA_NOT_REMOVABLE);
        return FALSE;
    }

    result = FALSE;

    EnterCriticalSection(&Sec);

    if (NULL != mpActiveSession)
    {
        if (aMediaSessionId == mpActiveSession->mSessionId)
        {
            result = mpActiveSession->Deactivate();
            if (result)
            {
                pSession = mpActiveSession;
                mpActiveSession = NULL;
                K2MEM_Zero(&CurMediaProp, sizeof(StorMediaProp));
                pSession->Release();
            }
        }
        else
        {
            SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
        }
    }
    else
    {
        SetLastError(K2STAT_ERROR_NO_MEDIA);
    }

    LeaveCriticalSection(&Sec);

    return result;
}

BOOL MyStorAdapter::TransferData(UINT_PTR aMediaSessionId, BOOL aIsWrite, UINT_PTR aPhysAddr, UINT_PTR aStartBlock, UINT_PTR aBlockCount)
{
    K2STAT  stat;
    UINT64  blockIx;
    BOOL    result;

    result = FALSE;

    EnterCriticalSection(&Sec);

    if (NULL == mpActiveSession)
    {
        SetLastError(K2STAT_ERROR_NO_MEDIA);
    }
    else
    {
        if (mpActiveSession->mSessionId != aMediaSessionId)
        {
            SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
        }
        else
        {
            if (0 != aPhysAddr % CurMediaProp.mBlockSizeBytes)
            {
                SetLastError(K2STAT_ERROR_BAD_ALIGNMENT);
            }
            else
            {
                if ((aStartBlock > mpActiveSession->MediaProp.mBlockCount) ||
                    ((mpActiveSession->MediaProp.mBlockCount - aStartBlock) < aBlockCount))
                {
                    SetLastError(K2STAT_ERROR_OUT_OF_BOUNDS);
                }
                else
                {
                    if (0 != (aPhysAddr % mpActiveSession->MediaProp.mBlockSizeBytes))
                    {
                        SetLastError(K2STAT_ERROR_BAD_ALIGNMENT);
                    }
                    else
                    {
                        blockIx = aStartBlock;
                        stat = vhdTransfer(&blockIx, aBlockCount, aIsWrite, aPhysAddr);
                        if (K2STAT_IS_ERROR(stat))
                        {
                            SetLastError(stat);
                        }
                        else
                        {
                            result = TRUE;
                        }
                    }
                }
            }
        }
    }

    LeaveCriticalSection(&Sec);

    return result;
}

MyStorMediaSession::MyStorMediaSession(
    MyStorAdapter *         apAdapter, 
    UINT_PTR                aSessionId, 
    StorMediaProp const *   apMediaProp, 
    BOOL                    aIsReadOnly,
    UINT8 *                 apPartial
) :
    mpAdapter(apAdapter),
    mSessionId(aSessionId),
    mIsReadOnly(aIsReadOnly),
    MediaProp(*apMediaProp),
    mBuffersPerPage(K2_VA_MEMPAGE_BYTES / apMediaProp->mBlockSizeBytes),
    mpPartAlloc(apPartial),
    mpPartial((UINT8 *)((UINT_PTR)apPartial) + (apMediaProp->mBlockSizeBytes - (((UINT_PTR)apPartial) % apMediaProp->mBlockSizeBytes)))

{
    mRefs = 1;
    mIsActive = TRUE;
    mpAdapter->AddRef();
    K2OS_RwLock_Init(&RwLock);
}

MyStorMediaSession::~MyStorMediaSession(
    void
)
{
    K2_ASSERT(0 == mRefs);
    mpAdapter->Release();
    K2OS_RwLock_Done(&RwLock);
    delete[] mpPartAlloc;
}

BOOL MyStorMediaSession::AcquireAdapter(IStorAdapter **apRetAdapter)
{
    if (NULL == apRetAdapter)
    {
        SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }
    mpAdapter->AddRef();
    *apRetAdapter = mpAdapter;
    return TRUE;
}

UINT_PTR 
MyStorMediaSession::ReadBlocks(
    UINT_PTR    aStartBlock,
    UINT_PTR    aBlockCount,
    void *      apAlignedBuffer
)
{
    BOOL        ok;
    UINT_PTR    result;
    UINT_PTR    doNow;

    if ((aStartBlock >= MediaProp.mBlockCount) ||
        ((MediaProp.mBlockCount - aStartBlock) < aBlockCount))
    {
        SetLastError(K2STAT_ERROR_OUT_OF_BOUNDS);
        return 0;
    }

    if (0 != (((UINT_PTR)apAlignedBuffer) % MediaProp.mBlockSizeBytes))
    {
        SetLastError(K2STAT_ERROR_BAD_ALIGNMENT);
        return 0;
    }

    result = 0;

    do {
        K2OS_RwLock_ReadLock(&RwLock);

        ok = mIsActive;

        if (ok)
        {
            if (aBlockCount > 8)
            {
                doNow = 8;
            }
            else
            {
                doNow = aBlockCount;
            }

            ok = mpAdapter->TransferData(mSessionId, FALSE, (UINT_PTR)apAlignedBuffer, aStartBlock, doNow);

            if (ok)
            {
                result += doNow;
                aBlockCount -= doNow;
                apAlignedBuffer = ((UINT8 *)apAlignedBuffer) + (doNow * MediaProp.mBlockSizeBytes);
            }
        }

        K2OS_RwLock_ReadUnlock(&RwLock);

        if (!ok)
            break;

    } while (0 != aBlockCount);

    return result;
}

UINT_PTR 
MyStorMediaSession::WriteBlocks(
    UINT_PTR    aStartBlock,
    UINT_PTR    aBlockCount,
    void const *apAlignedBuffer,
    BOOL        aWriteThrough
)
{
    BOOL        ok;
    UINT_PTR    result;
    UINT_PTR    doNow;

    if ((aStartBlock >= MediaProp.mBlockCount) ||
        ((MediaProp.mBlockCount - aStartBlock) < aBlockCount))
    {
        SetLastError(K2STAT_ERROR_OUT_OF_BOUNDS);
        return 0;
    }

    if (0 != (((UINT_PTR)apAlignedBuffer) % MediaProp.mBlockSizeBytes))
    {
        SetLastError(K2STAT_ERROR_BAD_ALIGNMENT);
        return 0;
    }

    result = 0;

    do {
        K2OS_RwLock_WriteLock(&RwLock);

        ok = mIsActive;

        if (ok)
        {
            if (aBlockCount > 8)
            {
                doNow = 8;
            }
            else
            {
                doNow = aBlockCount;
            }

            ok = mpAdapter->TransferData(mSessionId, TRUE, (UINT_PTR)apAlignedBuffer, aStartBlock, doNow);

            if (ok)
            {
                result += doNow;
                aBlockCount -= doNow;
                apAlignedBuffer = ((UINT8 *)apAlignedBuffer) + (doNow * MediaProp.mBlockSizeBytes);
            }
        }

        K2OS_RwLock_WriteUnlock(&RwLock);

        if (!ok)
            break;

    } while (0 != aBlockCount);

    return result;
}

BOOL     
MyStorMediaSession::FlushDirtyBlocks(
    UINT_PTR aStartBlock,
    UINT_PTR aBlockCount
)
{
    BOOL ok;

    K2OS_RwLock_WriteLock(&RwLock);

    ok = mIsActive;

    K2OS_RwLock_WriteUnlock(&RwLock);

    if (!ok)
    {
        SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
    }

    return ok;
}

//
// ------------------------------------------------------------------------
//

int RunTest(void)
{
    MyStorAdapter * pAdapter;
    int             result;

    pAdapter = new MyStorAdapter(TRUE, FALSE);

    if (!pAdapter->InsertMedia(&gVhdProp, FALSE))
        return GetLastError();

    result = RunAdapter(pAdapter);

    pAdapter->Release();

    return result;
}

