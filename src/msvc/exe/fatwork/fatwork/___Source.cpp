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
#define INITGUID
#include "fatwork.h"
#include <lib/k2atomic.h>
#include <lib/k2spin.h>

K2STAT vhdTransfer(UINT64 const * apBlockIx, UINT_PTR aBlockCount, BOOL aIsWrite, UINT_PTR aBufferAddr);

//
// ----------------------------------------------------------------------------------
//

class MyStorMediaSession;

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

    BOOL AcquireSession(IStorMediaSession **appRetSession);
    BOOL EjectMedia(UINT_PTR aMediaSessionId);
    BOOL InsertMedia(StorMediaProp const *apMediaProp, BOOL aIsReadOnly);

    BOOL DataTransfer(BOOL aIsWrite, UINT_PTR aPhysAddr, UINT_PTR aStartBlock, UINT_PTR aBlockCount);

    UINT_PTR                mRefs;
    CRITICAL_SECTION        Sec;
    UINT_PTR                mNextSessionId;
    BOOL                    mIsRemovable;
    BOOL                    mIsReadOnly;
    MyStorMediaSession *    mpActiveSession;
};

class MyStorMediaSession : public IStorMediaSession
{
public:
    MyStorMediaSession(
        MyStorAdapter *         apParentAdapter, 
        UINT_PTR                aSessionId,
        StorMediaProp const *   apMediaProp,
        BOOL                    aIsReadOnly
    ) : mpParentAdapter(apParentAdapter), 
        mSessionId(aSessionId),
        mIsReadOnly(aIsReadOnly)
    {
        mRefs = 1;
        mpParentAdapter->AddRef();
        K2MEM_Copy(&MediaProp, apMediaProp, sizeof(StorMediaProp));
        InitializeCriticalSection(&Sec);
    }

    virtual ~MyStorMediaSession(void)
    {
        mpParentAdapter->Release();
        DeleteCriticalSection(&Sec);
    }

    BOOL SessionLocked_Read(UINT64 const *apByteOffset, UINT_PTR aByteCount, void *apBuffer);
    BOOL SessionLocked_Write(UINT64 const *apByteOffset, UINT_PTR aByteCount, void const *apBuffer, BOOL aWriteThrough);
    BOOL SessionLocked_FlushWrites(void);
    BOOL SessionLocked_PurgeCache(void);

    UINT8 * LockBlock(UINT_PTR aBlockIx);
    void    UnlockBlock(UINT_PTR aBlockIx, BOOL aWriteThrough);

    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    UINT_PTR GetId(void)
    {
        return mSessionId;
    }

    BOOL GetMediaProp(StorMediaProp *apRetMediaProp)
    {
        if (NULL == apRetMediaProp)
        {
            SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
            return FALSE;
        }

        K2MEM_Copy(apRetMediaProp, &MediaProp, sizeof(StorMediaProp));

        return TRUE;
    }

    BOOL GetReadOnly(void)
    {
        return mIsReadOnly;
    }
    
    IStorAdapter * GetAdapter(void)
    {
        return mpParentAdapter;
    }

    BOOL GetIsActive(void)
    {
        return mIsActive;
    }

    // these will fail if the session is not active
    BOOL Read(UINT64 const *apByteOffset, UINT_PTR aByteCount, void *apBuffer)
    {
        BOOL    result;
        UINT64  offset;

        if ((NULL == apBuffer) || (NULL == apByteOffset))
        {
            SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
            return FALSE;
        }

        if (!mIsActive)
        {
            SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
            return FALSE;
        }

        result = FALSE;

        EnterCriticalSection(&Sec);

        if (!mIsActive)
        {
            SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
        }
        else
        {
            offset = *apByteOffset;
            K2_CpuReadBarrier();
            if ((offset >= MediaProp.mTotalBytes) ||
                ((MediaProp.mTotalBytes - offset) < aByteCount))
            {
                SetLastError(K2STAT_ERROR_OUT_OF_BOUNDS);
            }
            else
            {
                if (0 == aByteCount)
                {
                    result = TRUE;
                }
                else
                {
                    result = SessionLocked_Read(&offset, aByteCount, apBuffer);
                }
            }
        }

        LeaveCriticalSection(&Sec);

        return result;
    }

    BOOL Write(UINT64 const *apByteOffset, UINT_PTR aByteCount, void const *apBuffer, BOOL aWriteThrough)
    {
        BOOL    result;
        UINT64  offset;

        if ((NULL == apBuffer) || (NULL == apByteOffset))
        {
            SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
            return FALSE;
        }

        if (mIsReadOnly)
        {
            SetLastError(K2STAT_ERROR_READ_ONLY);
            return FALSE;
        }

        if (!mIsActive)
        {
            SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
            return FALSE;
        }

        result = FALSE;

        EnterCriticalSection(&Sec);

        if (!mIsActive)
        {
            SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
        }
        else
        {
            offset = *apByteOffset;
            K2_CpuReadBarrier();
            if ((offset >= MediaProp.mTotalBytes) ||
                ((MediaProp.mTotalBytes - offset) < aByteCount))
            {
                SetLastError(K2STAT_ERROR_OUT_OF_BOUNDS);
            }
            else
            {
                if (0 == aByteCount)
                {
                    result = TRUE;
                }
                else
                {
                    result = SessionLocked_Write(&offset, aByteCount, apBuffer, aWriteThrough);
                }
            }
        }

        LeaveCriticalSection(&Sec);

        return result;
    }

    BOOL FlushWrites(void)
    {
        BOOL result;

        if (!mIsActive)
        {
            SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
            return FALSE;
        }

        if (mIsReadOnly)
        {
            SetLastError(K2STAT_ERROR_READ_ONLY);
            return FALSE;
        }

        result = FALSE;

        EnterCriticalSection(&Sec);

        if (!mIsActive)
        {
            SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
        }
        else
        {
            SessionLocked_FlushWrites();
            result = TRUE;
        }

        LeaveCriticalSection(&Sec);

        return result;
    }

    MyStorAdapter * const   mpParentAdapter;
    UINT_PTR const          mSessionId;
    BOOL const              mIsReadOnly;
    StorMediaProp           MediaProp;

    UINT_PTR                mRefs;
    CRITICAL_SECTION        Sec;
    BOOL                    mIsActive;
};

BOOL MyStorAdapter::InsertMedia(StorMediaProp const *apMediaProp, BOOL aIsReadOnly)
{
    BOOL result;

    if (NULL == apMediaProp)
    {
        SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    result = FALSE;

    EnterCriticalSection(&Sec);

    if (NULL != mpActiveSession)
    {
        SetLastError(K2STAT_ERROR_MEDIA_PRESENT);
    }
    else
    {
        if (mIsReadOnly)
            aIsReadOnly = TRUE;
        mpActiveSession = new MyStorMediaSession(this, mNextSessionId++, apMediaProp, aIsReadOnly);
        result = (NULL == mpActiveSession) ? FALSE : TRUE;
    }

    LeaveCriticalSection(&Sec);

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

    if (!AcquireSession((IStorMediaSession **)&pSession))
        return FALSE;

    if (pSession->GetId() != aMediaSessionId)
    {
        pSession->Release();
        SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
        return FALSE;
    }

    result = FALSE;

    EnterCriticalSection(&pSession->Sec);

    if (!pSession->mIsActive)
    {
        SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
    }
    else
    {
        K2_ASSERT(pSession == mpActiveSession);

        pSession->mIsActive = FALSE;

        pSession->SessionLocked_PurgeCache();

        result = TRUE;
    }

    LeaveCriticalSection(&pSession->Sec);

    pSession->Release();

    return result;
}

BOOL MyStorAdapter::DataTransfer(BOOL aIsWrite, UINT_PTR aPhysAddr, UINT_PTR aStartBlock, UINT_PTR aBlockCount)
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
        if ((aStartBlock > mpActiveSession->MediaProp.mBlockCount) ||
            ((mpActiveSession->MediaProp.mBlockCount - aStartBlock) < aBlockCount))
        {
            SetLastError(K2STAT_ERROR_OUT_OF_BOUNDS);
        }
        else
        {
            blockIx = aStartBlock;
            stat = vhdTransfer(&blockIx, aBlockCount, FALSE, aPhysAddr);
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

    LeaveCriticalSection(&Sec);

    return result;
}

BOOL MyStorMediaSession::SessionLocked_Read(UINT64 const *apByteOffset, UINT_PTR aByteCount, void *apBuffer)
{
    UINT64      offset;
    UINT_PTR    blockIx;
    UINT_PTR    blockOffset;
    UINT_PTR    bytesLeft;
    UINT8 *     pBlock;

    // parameters already validated

    offset = *apByteOffset;
    blockIx = (UINT_PTR)(offset / (UINT64)MediaProp.mBlockSizeBytes);
    blockOffset = (UINT_PTR)(offset - (((UINT64)MediaProp.mBlockSizeBytes) * ((UINT64)blockIx)));

    if (0 != blockOffset)
    {
        pBlock = LockBlock(blockIx);
        if (NULL == pBlock)
            return FALSE;
        bytesLeft = MediaProp.mBlockSizeBytes - blockOffset;
        if (aByteCount < bytesLeft)
            bytesLeft = aByteCount;
        K2MEM_Copy(apBuffer, pBlock + blockOffset, bytesLeft);
        UnlockBlock(blockIx, FALSE);
        aByteCount -= bytesLeft;
        apBuffer = ((UINT8 *)apBuffer) + bytesLeft;
        blockIx++;
    }

    if (aByteCount >= MediaProp.mBlockSizeBytes)
    {
        do {
            pBlock = LockBlock(blockIx);
            if (NULL == pBlock)
                return FALSE;
            K2MEM_Copy(apBuffer, pBlock, MediaProp.mBlockSizeBytes);
            UnlockBlock(blockIx, FALSE);
            aByteCount -= MediaProp.mBlockSizeBytes;
            apBuffer = ((UINT8 *)apBuffer) + MediaProp.mBlockSizeBytes;
            blockIx++;
        } while (aByteCount >= MediaProp.mBlockSizeBytes);
    }

    if (0 != aByteCount)
    {
        pBlock = LockBlock(blockIx);
        if (NULL == pBlock)
            return FALSE;
        K2MEM_Copy(apBuffer, pBlock, aByteCount);
        UnlockBlock(blockIx, FALSE);
    }

    return TRUE;
}

BOOL MyStorMediaSession::SessionLocked_Write(UINT64 const *apByteOffset, UINT_PTR aByteCount, void const *apBuffer, BOOL aWriteThrough)
{
    UINT64      offset;
    UINT_PTR    blockIx;
    UINT_PTR    blockOffset;
    UINT_PTR    bytesLeft;
    UINT8 *     pBlock;

    // parameters already validated

    offset = *apByteOffset;
    blockIx = (UINT_PTR)(offset / (UINT64)MediaProp.mBlockSizeBytes);
    blockOffset = (UINT_PTR)(offset - (((UINT64)MediaProp.mBlockSizeBytes) * ((UINT64)blockIx)));

    if (0 != blockOffset)
    {
        pBlock = LockBlock(blockIx);
        if (NULL == pBlock)
            return FALSE;
        bytesLeft = MediaProp.mBlockSizeBytes - blockOffset;
        if (aByteCount < bytesLeft)
            bytesLeft = aByteCount;
        K2MEM_Copy(pBlock + blockOffset, apBuffer, bytesLeft);
        UnlockBlock(blockIx, aWriteThrough);
        aByteCount -= bytesLeft;
        apBuffer = ((UINT8 const *)apBuffer) + bytesLeft;
        blockIx++;
    }

    if (aByteCount >= MediaProp.mBlockSizeBytes)
    {
        do {
            pBlock = LockBlock(blockIx);
            if (NULL == pBlock)
                return FALSE;
            K2MEM_Copy(pBlock, apBuffer, MediaProp.mBlockSizeBytes);
            UnlockBlock(blockIx, aWriteThrough);
            aByteCount -= MediaProp.mBlockSizeBytes;
            apBuffer = ((UINT8 const *)apBuffer) + MediaProp.mBlockSizeBytes;
            blockIx++;
        } while (aByteCount >= MediaProp.mBlockSizeBytes);
    }

    if (0 != aByteCount)
    {
        pBlock = LockBlock(blockIx);
        if (NULL == pBlock)
            return FALSE;
        K2MEM_Copy(pBlock, apBuffer, aByteCount);
        UnlockBlock(blockIx, aWriteThrough);
    }

    return TRUE;
}

BOOL MyStorMediaSession::SessionLocked_FlushWrites(void)
{

    // parameters already validated

    return FALSE;
}

BOOL MyStorMediaSession::SessionLocked_PurgeCache(void)
{

    // parameters already validated

    return FALSE;
}

UINT8 * MyStorMediaSession::LockBlock(UINT_PTR aBlockIx)
{
    return NULL;
}

void MyStorMediaSession::UnlockBlock(UINT_PTR aBlockIx, BOOL aWriteThrough)
{
    

}

StorMediaProp gProp;

int RunTest(void)
{
    MyStorAdapter *     pAdapter;
    IStorMediaSession * pSession;
    UINT8               buffer[512];
    UINT64              offset;

    pAdapter = new MyStorAdapter(TRUE, FALSE);
    pAdapter->InsertMedia(&gProp, FALSE);
    pAdapter->AcquireSession(&pSession);

    offset = 0;
    pSession->Read(&offset, 1, &buffer);

    return 0;
}

//
// ----------------------------------------------------------------------------------
//

static
void
myAssert(
    char const *    apFile,
    int             aLineNum,
    char const *    apCondition
)
{
    DebugBreak();
}

extern "C" K2_pf_ASSERT K2_Assert = myAssert;

K2_PACKED_PUSH
typedef struct _VHD_FOOTER VHD_FOOTER;
struct _VHD_FOOTER
{
    UINT8   mCookie[8];
    UINT32  mFeatures;
    UINT32  mFormatVersion;
    UINT64  mDataOffset;
    UINT32  mTimeStamp;
    UINT32  mCreatorApp;
    UINT32  mCreatorVersion;
    UINT32  mCreatorHostOS;
    UINT64  mOriginalSize;
    UINT64  mCurrentSize;
    UINT16  mDiskGeo_NumCyl;
    UINT8   mDiskGeo_NumHeads;
    UINT8   mDiskGeo_NumSectorsPerTrack;
    UINT32  mDiskType;
    UINT32  mCheckSum;
    UINT8   mUniqueId[16];
    UINT8   mSavedState;
    UINT8   mReserved[427];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
K2_STATIC_ASSERT(K2STOR_SECTOR_BYTES == sizeof(VHD_FOOTER));
VHD_FOOTER vhdFooter;

typedef struct _VHD VHD;
struct _VHD
{
    HANDLE      mHandle;
    VHD_FOOTER  VhdFooter;
};

VHD gVHD;


K2STAT
vhdTransfer(
    UINT64 const *  apBlockIx,
    UINT_PTR        aBlockCount,
    BOOL            aIsWrite,
    UINT_PTR        aBufferAddr
)
{
    if (0 == aBlockCount)
        return K2STAT_NO_ERROR;

    LARGE_INTEGER li;
    li.QuadPart = gProp.mBlockSizeBytes;
    li.QuadPart *= *apBlockIx;
    OVERLAPPED overlapped;
    ZeroMemory(&overlapped, sizeof(overlapped));
    DWORD bytesIo;
    bytesIo = 0;
    BOOL ok;
    overlapped.Offset = li.LowPart;
    overlapped.OffsetHigh = li.HighPart;
    if (aIsWrite)
    {
        ok = WriteFile(gVHD.mHandle, (void *)aBufferAddr, gProp.mBlockSizeBytes * aBlockCount, &bytesIo, &overlapped);
    }
    else
    {
        ok = ReadFile(gVHD.mHandle, (void *)aBufferAddr, gProp.mBlockSizeBytes * aBlockCount, &bytesIo, &overlapped);
    }
    if (!ok)
    {
        if (ERROR_IO_PENDING == GetLastError())
        {
            if (!GetOverlappedResult(gVHD.mHandle, &overlapped, &bytesIo, TRUE))
            {
                return (K2STAT)(HRESULT_FROM_WIN32(GetLastError()));
            }
        }
    }

    return K2STAT_NO_ERROR;
}


int main(int argc, char **argv)
{
    static WCHAR const *spDiskFileName = L"C:\\users\\kurt_\\desktop\\Test.VHD";

    ZeroMemory(&gVHD, sizeof(gVHD));

    HANDLE hFile;
    hFile = CreateFileW(spDiskFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (INVALID_HANDLE_VALUE == hFile)
        return -1;
    ULONG fileBytes = GetFileSize(hFile, NULL);
    if (INVALID_FILE_SIZE == fileBytes)
        return -2;
    if (0 != (fileBytes % K2STOR_SECTOR_BYTES))
        return -3;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, -K2STOR_SECTOR_BYTES, NULL, FILE_END))
        return -4;
    DWORD bytesIo = 0;
    if (!ReadFile(hFile, &gVHD.VhdFooter, K2STOR_SECTOR_BYTES, &bytesIo, NULL))
    {
        return -5;
    }
    CloseHandle(hFile);

    OPEN_VIRTUAL_DISK_PARAMETERS openParameters;

    ZeroMemory(&openParameters, sizeof(openParameters));
    openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_2;

    ATTACH_VIRTUAL_DISK_PARAMETERS attachParameters;
    attachParameters.Version = ATTACH_VIRTUAL_DISK_VERSION_1;

    VIRTUAL_STORAGE_TYPE storageType;
    storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    if (OpenVirtualDisk(&storageType, spDiskFileName,
        VIRTUAL_DISK_ACCESS_NONE, OPEN_VIRTUAL_DISK_FLAG_NONE,
        &openParameters, &gVHD.mHandle) != ERROR_SUCCESS) {
        // If return value of OpenVirtualDisk isn't ERROR_SUCCESS, there was a problem opening the VHD
        return -1;
    }

    // Warning: AttachVirtualDisk requires elevation
    if (AttachVirtualDisk(gVHD.mHandle, 0, ATTACH_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST,
        0, &attachParameters, 0) != ERROR_SUCCESS) {
        // If return value of AttachVirtualDisk isn't ERROR_SUCCESS, there was a problem attach the disk
        return -1;
    }

    GET_VIRTUAL_DISK_INFO info;
    ZeroMemory(&info, sizeof(info));
    ULONG infoSize = sizeof(info);
    ULONG sizeUsed = 0;
    info.Version = GET_VIRTUAL_DISK_INFO_SIZE;
    if (GetVirtualDiskInformation(gVHD.mHandle, &infoSize, &info, &sizeUsed) != ERROR_SUCCESS)
    {
        return -1;
    }

    gProp.mBlockSizeBytes = info.Size.SectorSize;
    gProp.mBlockCount = (UINT32)(info.Size.VirtualSize / info.Size.SectorSize);
    K2ASC_Copy(gProp.mFriendly, "VHD");
    gProp.mUniqueId = K2CRC_Calc32(0, &vhdFooter.mUniqueId, 8);
    gProp.mUniqueId |= ((UINT64)(K2CRC_Calc32(0, &vhdFooter.mUniqueId[8], 8))) << 32;
    gProp.mTotalBytes = ((UINT64)gProp.mBlockCount) * ((UINT64)gProp.mBlockSizeBytes);

    return RunTest();
}