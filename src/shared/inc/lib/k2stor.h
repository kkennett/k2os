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
#ifndef __K2STOR_H
#define __K2STOR_H

#include <k2systype.h>
#include <lib/k2mem.h>
#include <lib/k2asc.h>
#include <lib/k2crc.h>
#include <spec/fat.h>
#include <spec/gpt.h>

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

K2STAT
K2STOR_BLOCKDEV_Transfer(
    K2STOR_BLOCKDEV const * apBlockDev,
    UINT64 const *          apBlockIx,
    UINT_PTR                aBlockCount,
    BOOL                    aIsWrite,
    UINT_PTR                aBufferAddr
);

K2STAT
K2STOR_PART_Transfer(
    K2STOR_BLOCKDEV const *apBlockDev,
    UINT_PTR        aPartIx,
    UINT64 const *  apPartBlockOffset,
    UINT_PTR        aBlockCount,
    BOOL            aIsWrite,
    UINT_PTR        aBufferAddr
);

K2STAT
K2STOR_PART_DiscoverFromMBR(
    K2STOR_MEDIA const *    apMedia,
    UINT8 const *           apSector0,
    UINT_PTR *              apIoPartCount,
    K2STOR_PART *           apRetPartEntries
);

K2STAT
K2STOR_PART_ValidateGPT1(
    GPT_SECTOR const *      apSector1,
    K2STOR_MEDIA const *    apMedia
);

K2STAT
K2STOR_PART_ValidateGPTAlt(
    GPT_SECTOR const *      apSector1,
    GPT_SECTOR const *      apAltSector,
    K2STOR_MEDIA const *    apMedia
);

K2STAT
K2STOR_PART_ValidateGPTPartitions(
    GPT_SECTOR const *   apSector1,
    GPT_SECTOR const *   apAltSector,
    K2STOR_MEDIA const * apMedia,
    UINT8 const *        apPartTab1,
    UINT8 const *        apPartTab2,
    UINT_PTR *           apRetNonEmptyPartCount
);

//
//------------------------------------------------------
//

#ifdef __cplusplus
}
#endif

#endif  // __K2STOR_H