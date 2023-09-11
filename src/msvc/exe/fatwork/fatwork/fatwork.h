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
#ifndef __FATWORK_H
#define __FATWORK_H

#include <lib/k2win32.h>

#include <k2systype.h>
#include <lib/k2mem.h>
#include <lib/k2asc.h>
#include <lib/k2crc.h>
#include <spec/fat.h>
#include <spec/gpt.h>

#include <lib/k2list.h>
#include <lib/k2tree.h>
#include <lib/k2atomic.h>
#include <VirtDisk.h>

//
//-------------------------------------------------------------------------------
//

typedef struct _K2OS_RWLOCK K2OS_RWLOCK;
struct _K2OS_RWLOCK
{
    CRITICAL_SECTION    CsReaderOuter;
    CRITICAL_SECTION    CsReaderChain;
    CRITICAL_SECTION    CsReaderFirst;
    CRITICAL_SECTION    CsWriter;
    CRITICAL_SECTION    CsWriterFirst;
    UINT_PTR volatile   mReadersCount;
    UINT_PTR volatile   mWritersCount;
};

BOOL K2OS_RwLock_Init(K2OS_RWLOCK *apRwLock);
BOOL K2OS_RwLock_ReadLock(K2OS_RWLOCK *apRwLock);
BOOL K2OS_RwLock_ReadUnlock(K2OS_RWLOCK *apRwLock);
BOOL K2OS_RwLock_WriteLock(K2OS_RWLOCK *apRwLock);
BOOL K2OS_RwLock_WriteUnlock(K2OS_RWLOCK *apRwLock);
BOOL K2OS_RwLock_Done(K2OS_RWLOCK *apRwLock);

//
//-------------------------------------------------------------------------------
//

#define K2STOR_MEDIA_FRIENDLY_BUFFER_CHARS 32

struct StorMediaProp
{
    UINT64  mUniqueId;
    UINT32  mBlockCount;
    UINT32  mBlockSizeBytes;
    UINT64  mTotalBytes;
    char    mFriendly[K2STOR_MEDIA_FRIENDLY_BUFFER_CHARS];
};

//
//-------------------------------------------------------------------------------
//

struct IBase
{
    virtual UINT_PTR AddRef(void) = 0;
    virtual UINT_PTR Release(void) = 0;
};

//
//-------------------------------------------------------------------------------
//


//
//-------------------------------------------------------------------------------
//

extern StorMediaProp gVhdProp;

K2STAT vhdTransfer(UINT64 const * apBlockIx, UINT_PTR aBlockCount, BOOL aIsWrite, UINT_PTR aBufferAddr);

struct IStorAdapter;

int RunTest(void);
int RunAdapter(IStorAdapter *apAdapter);

//
//-------------------------------------------------------------------------------
//

struct IStorMediaSession;

struct IStorAdapter : public IBase
{
    virtual UINT_PTR    GetFriendly(char *apBuffer, UINT_PTR aBufferLen) = 0;
    virtual BOOL        GetMediaIsRemovable(void) = 0;
    virtual BOOL        GetMediaProp(StorMediaProp *apRetProp, BOOL *apRetReadOnly) = 0;

    // this will fail if there is no media present in/attachedto the adapter or the media is nonremovable
    virtual BOOL        AcquireSession(IStorMediaSession **apRetSession) = 0;
    virtual BOOL        EjectMedia(UINT_PTR aMediaSessionId) = 0;
    virtual BOOL        TransferData(UINT_PTR aMediaSessionId, BOOL aIsWrite, UINT_PTR aPhysAddr, UINT_PTR aStartBlock, UINT_PTR aBlockCount) = 0;
};

//
//-------------------------------------------------------------------------------
//

struct IStorMediaSession : public IBase
{
    virtual UINT_PTR    GetId(void) = 0;
    virtual BOOL        GetMediaProp(StorMediaProp *apRetMedia, BOOL *apRetReadOnly) = 0;
    virtual BOOL        AcquireAdapter(IStorAdapter **apRetAdapter) = 0;
    virtual BOOL        GetIsActive(void) = 0;

    // these will fail if the session is not active
    virtual UINT_PTR    ReadBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount, void *apAlignedBuffer) = 0;
    virtual UINT_PTR    WriteBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount, void const *apAlignedBuffer, BOOL aWriteThrough) = 0;
    virtual BOOL        FlushDirtyBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount) = 0;
};

//
//-------------------------------------------------------------------------------
//

struct IStorPartition : public IBase
{
    virtual BOOL        AcquireMediaSession(IStorMediaSession **appRetSession) = 0;
    virtual BOOL        GetGuid(K2_GUID128 *apRetId) = 0;
    virtual BOOL        GetTypeGuid(K2_GUID128 *apRetTypeId) = 0;
    virtual UINT_PTR    GetLegacyType(void) = 0;
    virtual BOOL        GetAttributes(UINT64 *apRetAttributes) = 0;
    virtual UINT_PTR    GetStartBlock(void) = 0;
    virtual UINT_PTR    GetBlockCount(void) = 0;
    virtual void        GetSizeBytes(UINT64 *apRetSizeBytes) = 0;

    // these will fail if the media session is not active
    virtual UINT_PTR    ReadBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount, void *apAlignedBuffer) = 0;
    virtual UINT_PTR    WriteBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount, void const *apAlignedBuffer, BOOL aWriteThrough) = 0;
    virtual BOOL        FlushDirtyBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount) = 0;
};

//
//-------------------------------------------------------------------------------
//

struct StorExtent
{
    UINT64  mFirstByteOffset;
    UINT64  mLastByteOffset;
};

struct IStorPlex : public IBase
{
    virtual UINT_PTR    GetPartitionCount(void);
    virtual BOOL        GetExtent(UINT_PTR aIndex, UINT64 *apRetStorageMediaUniqueId, StorExtent *apRetExtent);

    virtual BOOL        SetPartitionCount(UINT_PTR aNewCount);
    virtual BOOL        SetExtent(UINT_PTR aIntex, UINT64 const *apStorageMediaUniqueId, StorExtent *apExtent);
    virtual BOOL        Online(void);

    virtual BOOL        GetIsOnline(void);

    virtual BOOL        Offline(void);
    virtual BOOL        AcquirePartition(UINT_PTR aIndex, IStorPartition **appRetPartition);
    virtual UINT_PTR    GetBlockCount(void);
    virtual BOOL        GetSizeBytes(UINT64 *apRetSizeBytes);

    // these will fail if the plex is not online
    virtual UINT_PTR    ReadBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount, void *apAlignedBuffer) = 0;
    virtual UINT_PTR    WriteBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount, void const *apAlignedBuffer, BOOL aWriteThrough) = 0;
    virtual BOOL        FlushDirtyBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount) = 0;
};

enum StorVolumeFormatType
{
    StorVolumeFormat_Raw = 0,
    StorVolumeFormat_Fat12,
    StorVolumeFormat_Fat16,
    StorVolumeFormat_Fat32,

    StorVolumeFormatType_Count
};

struct IStorVolume : public IBase
{
    virtual UINT_PTR                GetFriendly(char *apBuffer, UINT_PTR aBufferLen) = 0;
    virtual StorVolumeFormatType    GetFormat(void) = 0;
    virtual IStorPlex *             GetPlex(void) = 0;

    virtual BOOL Read(UINT64 const *apByteOffset, UINT_PTR aByteCount, void *apBuffer) = 0;
    virtual BOOL Write(UINT64 const *apByteOffset, UINT_PTR aByteCount, void const *apBuffer, BOOL aWriteThrough) = 0;
    virtual BOOL FlushWrites(void) = 0;
};

#endif
