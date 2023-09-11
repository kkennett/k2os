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

K2STAT vhdTransfer(UINT64 const * apBlockIx, UINT_PTR aBlockCount, BOOL aIsWrite, UINT_PTR aBufferAddr);

//
// ----------------------------------------------------------------------------------
//

StorMediaProp gProp;

class MyStorAdapter;

class MyStorMediaInstance : public IStorMediaInstance
{
public:
    MyStorMediaInstance(MyStorAdapter *apParent) : mpParent(apParent) 
    {
        mRefs = 1;
        mIsReadOnly = FALSE;
        K2MEM_Copy(&Prop, &gProp, sizeof(StorMediaProp));
    }
    virtual ~MyStorMediaInstance(void) {}

    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    UINT_PTR        GetInstanceId(void) { return 1; };
    BOOL            GetProperties(StorMediaProp *apRetProp) { K2MEM_Copy(apRetProp, &Prop, sizeof(StorMediaProp)); return TRUE; }
    BOOL            GetReadOnly(void) { return mIsReadOnly; }
    IStorAdapter *  GetHostStorAdapter(void);

    StorMediaProp           Prop;
    BOOL                    mIsReadOnly;
    UINT_PTR                mRefs;
    MyStorAdapter * const   mpParent;
};

class MyIoBuffer;

typedef union _MyBufferKey MyBufferKey;
union _MyBufferKey
{
    struct {
        UINT32  mMediaInstanceId;
        UINT32  mMediaPageOffset;
    };
    UINT64  mTreeKey;
};

int
MyBufferTreeCompare(
    UINT_PTR        aKey,
    K2TREE_NODE *   apNode
);

class MyStorBlockCache : public IStorBlockCache
{
public:
    MyStorBlockCache(MyStorAdapter *apParent) : mpParent(apParent)
    {
        mRefs = 1;
        K2LIST_Init(&IoBufferList);
        K2TREE_Init(&UseTree, MyBufferTreeCompare);
        mCurMediaInstanceId = 0;
        mCurMediaBlocksPerPage = 0;
        K2MEM_Zero(&CurMediaProp, sizeof(CurMediaProp));
    }
    virtual ~MyStorBlockCache(void);

    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    IStorAdapter *  GetStorAdapter(void);
    BOOL            Lock(UINT_PTR aMediaInstanceId, UINT_PTR aBlockNumber, void **appRetPtr);
    BOOL            Unlock(UINT_PTR aMediaInstanceId, UINT_PTR aBlockNumber, void *aPtr, BOOL aWriteThrough);
    BOOL            FlushWrites(void);

    UINT_PTR                mRefs;
    UINT_PTR                mCurMediaInstanceId;
    StorMediaProp           CurMediaProp;
    UINT_PTR                mCurMediaBlocksPerPage;
    K2LIST_ANCHOR           IoBufferList;
    K2TREE_ANCHOR           UseTree;
    MyStorAdapter * const   mpParent;
};

class MyIoBuffer
{
public:
    MyIoBuffer(MyStorBlockCache *apCache, UINT_PTR aPageAddr) : mpCache(apCache)
    {
        mIsDirty = FALSE;
        mLockCount = 0; 
        mMediaInstanceId = 0;   // nonzero means in use
        K2MEM_Zero(&UseTreeNode, sizeof(UseTreeNode));
        K2LIST_AddAtTail(&mpCache->IoBufferList, &ListLink);
    }

    ~MyIoBuffer(void)
    {
        K2LIST_Remove(&mpCache->IoBufferList, &ListLink);
    }

    K2TREE_NODE         UseTreeNode;
    BOOL                mIsDirty;
    UINT_PTR            mLockCount;
    MyStorBlockCache *  mpCache;
    K2LIST_LINK         ListLink;
};

class MyStorAdapter : public IStorAdapter
{
public:
    MyStorAdapter(void)
    {
        mRefs = 1;
        mpCache = new MyStorBlockCache(this);
        mpMedia = new MyStorMediaInstance(this);
    }
    virtual ~MyStorAdapter(void)
    {
        mpCache->Release();
        mpMedia->Release();
    }

    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    UINT_PTR            GetFriendly(char *apBuffer, UINT_PTR aBufferLen) { return K2ASC_CopyLen(apBuffer, "VHDHost", aBufferLen); }
    BOOL                GetMediaIsRemovable(void) { return FALSE; }
    IStorBlockCache *   GetCache(void) { return mpCache; }
    BOOL                AcquireMedia(IStorMediaInstance **appRetMedia) { mpMedia->AddRef(); *appRetMedia = mpMedia; return TRUE; }
    BOOL                EjectMedia(UINT_PTR aMediaInstanceId) { return FALSE; }
    BOOL                ReadMedia(UINT_PTR aMediaInstanceId, UINT_PTR aPhysAddr, UINT_PTR aStartBlock, UINT_PTR aBlockCount);
    BOOL                WriteMedia(UINT_PTR aMediaInstanceId, UINT_PTR aPhysAddr, UINT_PTR aStartBlock, UINT_PTR aBlockCount);

    UINT_PTR                mRefs;
    MyStorBlockCache *      mpCache;
    MyStorMediaInstance *   mpMedia;
};

IStorAdapter *  
MyStorMediaInstance::GetHostStorAdapter(void) 
{ 
    return mpParent; 
}

MyStorBlockCache::~MyStorBlockCache(void)
{
    K2LIST_LINK *   pLink;
    MyIoBuffer *    pIoBuffer;

    pLink = IoBufferList.mpHead;
    if (NULL != pLink)
    {
        do {
            pIoBuffer = K2_GET_CONTAINER(MyIoBuffer, pLink, ListLink);
            pLink = pLink->mpNext;
            if (pIoBuffer->mIsDirty)
            {
                DebugBreak();
            }
            VirtualFree((LPVOID)pIoBuffer->mPageAddr, 0, MEM_RELEASE);
            K2LIST_Remove(&IoBufferList, &pIoBuffer->ListLink);
        } while (NULL != pLink);
    }
}

IStorAdapter *  
MyStorBlockCache::GetStorAdapter(void)
{
    return mpParent;
}

int
MyBufferTreeCompare(
    UINT_PTR        aKey,
    K2TREE_NODE *   apNode
)
{
    UINT64 *        pVal;
    MyIoBuffer *    pIoBuffer;

    pVal = (UINT64 *)aKey;
    pIoBuffer = K2_GET_CONTAINER(MyIoBuffer, apNode, UseTreeNode);


}


BOOL            
MyStorBlockCache::Lock(
    UINT_PTR    aMediaInstanceId,
    UINT_PTR    aBlockNumber,
    void **     appRetPtr
)
{
    IStorMediaInstance *pMedia;
    UINT_PTR            workVal;
    K2TREE_NODE *       pTreeNode;
    MyIoBuffer *        pIoBuffer;

    if ((0 == aMediaInstanceId) || 
        (NULL == appRetPtr))
    {
        SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
        return FALSE;
    }

    if (0 != mCurMediaInstanceId)
    {
        if (aMediaInstanceId != mCurMediaInstanceId)
        {
            SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
            return FALSE;
        }
    }

    *appRetPtr = NULL;

    if (!mpParent->AcquireMedia(&pMedia))
        return FALSE;

    if (0 != mCurMediaInstanceId)
    {
        if (pMedia->GetInstanceId() != mCurMediaInstanceId)
        {
            pMedia->Release();
            SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
            return FALSE;
        }
    }
    else
    {
        if (!pMedia->GetProperties(&CurMediaProp))
        {
            pMedia->Release();
            return FALSE;
        }
        mCurMediaBlocksPerPage = K2_VA_MEMPAGE_BYTES / CurMediaProp.mBlockSizeBytes;
        mCurMediaInstanceId = pMedia->GetInstanceId();
    }

    workVal = aBlockNumber / mCurMediaBlocksPerPage;

    pIoBuffer = NULL;

    pTreeNode = K2TREE_Find(&UseTree, workVal);
    if (NULL == pTreeNode)
    {
        //
        // page not in memory. go get it.
        //

    }
    else
    {
        pIoBuffer = K2_GET_CONTAINER(MyIoBuffer, pTreeNode, UseTreeNode);
    }

    K2_ASSERT(NULL != pIoBuffer);

    K2_ASSERT(pIoBuffer->mMediaInstanceId == aMediaInstanceId);
    pIoBuffer->mLockCount++;
    workVal = (aBlockNumber % mCurMediaBlocksPerPage) * CurMediaProp.mBlockSizeBytes;
    *appRetPtr = (void *)(pIoBuffer->mPageAddr + workVal);

    return TRUE;
}

BOOL            
MyStorBlockCache::Unlock(
    UINT_PTR    aMediaInstanceId,
    UINT_PTR    aBlockNumber,
    void *      aPtr,
    BOOL        aWriteThrough
)
{




    return FALSE;
}

BOOL            
MyStorBlockCache::FlushWrites(
    void
)
{
    return FALSE;
}

BOOL                
MyStorAdapter::ReadMedia(
    UINT_PTR aMediaInstanceId,
    UINT_PTR aPhysAddr,
    UINT_PTR aStartBlock,
    UINT_PTR aBlockCount
)
{
    K2STAT stat;
    UINT64 blockIx;

    if (NULL == mpMedia)
    {
        SetLastError(K2STAT_ERROR_NO_MEDIA);
        return FALSE;
    }

    if (aMediaInstanceId != mpMedia->GetInstanceId())
    {
        SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
        return FALSE;
    }

    if ((aStartBlock > mpMedia->Prop.mBlockCount) ||
        ((mpMedia->Prop.mBlockCount - aStartBlock) < aBlockCount))
    {
        SetLastError(K2STAT_ERROR_OUT_OF_BOUNDS);
        return FALSE;
    }

    blockIx = aStartBlock;
    stat = vhdTransfer(&blockIx, aBlockCount, FALSE, aPhysAddr);
    if (K2STAT_IS_ERROR(stat))
    {
        SetLastError(stat);
        return FALSE;
    }

    return TRUE;
}

BOOL                
MyStorAdapter::WriteMedia(
    UINT_PTR aMediaInstanceId,
    UINT_PTR aPhysAddr,
    UINT_PTR aStartBlock,
    UINT_PTR aBlockCount
)
{
    K2STAT stat;
    UINT64 blockIx;

    if (NULL == mpMedia)
    {
        SetLastError(K2STAT_ERROR_NO_MEDIA);
        return FALSE;
    }

    if (aMediaInstanceId != mpMedia->GetInstanceId())
    {
        SetLastError(K2STAT_ERROR_MEDIA_CHANGED);
        return FALSE;
    }

    if (mpMedia->mIsReadOnly)
    {
        SetLastError(K2STAT_ERROR_READ_ONLY);
        return FALSE;
    }

    if ((aStartBlock > mpMedia->Prop.mBlockCount) ||
        ((mpMedia->Prop.mBlockCount - aStartBlock) < aBlockCount))
    {
        SetLastError(K2STAT_ERROR_OUT_OF_BOUNDS);
        return FALSE;
    }

    blockIx = aStartBlock;
    stat = vhdTransfer(&blockIx, aBlockCount, TRUE, aPhysAddr);
    if (K2STAT_IS_ERROR(stat))
    {
        SetLastError(stat);
        return FALSE;
    }

    return TRUE;
}


int RunTest(void)
{
    IStorAdapter * pAdapter;

    pAdapter = new MyStorAdapter;

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