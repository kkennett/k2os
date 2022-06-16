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
#ifndef __K2STOR_H
#define __K2STOR_H

#include <k2systype.h>
#include <lib/k2mem.h>
#include <lib/k2asc.h>
#include <lib/k2crc.h>
#include <spec/fat.h>

#ifdef __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------
//

#define K2STOR_SECTOR_BYTES   512

K2_PACKED_PUSH

typedef struct _K2STOR_MEDIA        K2STOR_MEDIA;
typedef struct _K2STOR_PART         K2STOR_PART;

typedef struct _K2STOR_GPT_HEADER   K2STOR_GPT_HEADER;
typedef struct _K2STOR_GPT_SECTOR   K2STOR_GPT_SECTOR;
typedef struct _K2STOR_GPT_ENTRY    K2STOR_GPT_ENTRY;

typedef struct _K2STOR_BLOCKIO      K2STOR_BLOCKIO;
typedef struct _K2STOR_BLOCKDEV     K2STOR_BLOCKDEV;

struct _K2STOR_MEDIA
{
    UINT8       mSerialNumber[31];
    UINT8       mFlagReadOnly;
    UINT16      mNumSectorsPerTrack;
    UINT16      mNumCylinders;
    UINT16      mNumHeads;
    UINT16      mBytesPerSector;
    UINT64      mTotalSectorsCount;
} K2_PACKED_ATTRIB;

struct _K2STOR_PART
{
    UINT_PTR    mPartTableEntryIx;
    K2_GUID128  mPartTypeGuid;
    K2_GUID128  mPartIdGuid;
    UINT64      mAttributes;
    UINT64      mMediaStartSectorOffset;
    UINT64      mMediaSectorsCount;
    UINT_PTR    mUserContext;
    UINT8       mPartTypeByte;
    UINT8       mFlagReadOnly;
    UINT8       mFlagActive;
    UINT8       mFlagEFI;
} K2_PACKED_ATTRIB;

struct _K2STOR_GPT_HEADER
{
    UINT8       Signature[8];       //  "EFI PART"
    UINT32      Revision;           //  0x00010000
    UINT32      HeaderSize;
    UINT32      HeaderCRC32;
    UINT32      Reserved;
    UINT64      MyLBA;
    UINT64      AlternateLBA;
    UINT64      FirstUsableLBA;
    UINT64      LastUsableLBA;
    K2_GUID128  DiskGuid;
    UINT64      PartitionEntryLBA;
    UINT32      NumberOfPartitionEntries;
    UINT32      SizeOfPartitionEntry;
    UINT32      PartitionEntryArrayCRC32;
} K2_PACKED_ATTRIB;
K2_STATIC_ASSERT(sizeof(K2STOR_GPT_HEADER) == 92);

struct _K2STOR_GPT_SECTOR
{
    K2STOR_GPT_HEADER   Header;
    UINT8               Reserved[K2STOR_SECTOR_BYTES - 92];
} K2_PACKED_ATTRIB;
K2_STATIC_ASSERT(sizeof(K2STOR_GPT_SECTOR) == K2STOR_SECTOR_BYTES);

struct _K2STOR_GPT_ENTRY
{
    K2_GUID128  PartitionTypeGuid;
    K2_GUID128  UniquePartitionGuid;
    UINT64      StartingLBA;
    UINT64      EndingLBA;
    UINT64      Attributes;
    UINT16      UnicodePartitionName[36];
} K2_PACKED_ATTRIB;
K2_STATIC_ASSERT(sizeof(K2STOR_GPT_ENTRY) == 128);

K2_PACKED_POP

typedef K2STAT (*K2STOR_BLOCKIO_pf_Transfer)(K2STOR_BLOCKIO const *apBlockIo, UINT64 const *apBlockStartIx, UINT_PTR aBlockCount, BOOL aIsWrite, UINT_PTR aBufferAddr);

struct _K2STOR_BLOCKIO
{
    K2STOR_BLOCKIO_pf_Transfer  Transfer;
    UINT_PTR                    mBlockSizeInBytes;
    UINT_PTR                    mMaxBlocksOneTransfer;
    UINT_PTR                    mTransferAlignBytes;
    BOOL                        mIsReadOnlyDevice;
};

struct _K2STOR_BLOCKDEV
{
    K2STOR_BLOCKIO      BlockIo;
    K2STOR_MEDIA *      mpCurrentMedia;
    UINT_PTR            mCurrentPartCount;
    K2STOR_PART *       mpCurrentPartArray;
};

#define K2STOR_GPT_ATTRIBUTE_LEGACY_BIOS_BOOTABLE   (0x0000000000000004)

#define K2STOR_GPT_BASIC_DATA_PART_GUID             { 0xEBD0A0A2, 0xB9E5, 0x4433, { 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 } }
#define K2STOR_GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY   (0x1000000000000000)

K2STAT K2STOR_BLOCKDEV_Transfer(K2STOR_BLOCKDEV const *apBlockDev, UINT64 const *apBlockIx, UINT_PTR aBlockCount, BOOL aIsWrite, UINT_PTR aBufferAddr);

K2STAT K2STOR_PART_Transfer(K2STOR_BLOCKDEV const *apBlockDev, UINT_PTR aPartIx, UINT64 const * apPartBlockOffset, UINT_PTR aBlockCount, BOOL aIsWrite, UINT_PTR aBufferAddr);

K2STAT K2STOR_PART_Discover(K2STOR_BLOCKIO const *apBlockIo, K2STOR_MEDIA const *apMedia, UINT_PTR *apRetPartCount,K2STOR_PART **appRetPartArray);

K2STAT K2STOR_BLOCKDEV_DiscoverMediaPartitions(K2STOR_BLOCKDEV * apBlockDev);

//
//------------------------------------------------------
//

#ifdef __cplusplus
}
#endif

#endif  // __K2STOR_H