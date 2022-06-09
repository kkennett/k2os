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
#define INITGUID
#include <lib/k2win32.h>
#include <lib/k2asc.h>
#include <lib/k2mem.h>
#include <lib/k2crc.h>
#include <spec/fat.h>
#include <VirtDisk.h>
#pragma warning(disable: 4477)

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

#define K2STOR_SECTOR_BYTES   512

K2_PACKED_PUSH
typedef struct _K2STOR_MEDIA    K2STOR_MEDIA;
typedef struct _K2STOR_BLOCKIO  K2STOR_BLOCKIO;
typedef struct _K2STOR_PART     K2STOR_PART;
typedef struct _K2STOR_BLOCKDEV K2STOR_BLOCKDEV;

struct _K2STOR_MEDIA
{
    UINT8               mSerialNumber[31];
    UINT8               mFlagReadOnly;
    UINT16              mNumSectorsPerTrack;
    UINT16              mNumCylinders;
    UINT16              mNumHeads;
    UINT16              mBytesPerSector;
    UINT64              mTotalSectorsCount;
    UINT_PTR            mCurrentPartCount;
    K2STOR_PART *       mpCurrentPartArray;
    K2STOR_BLOCKDEV *   mpCurrentMount;
    UINT_PTR            mUserContext;
} K2_PACKED_ATTRIB;

struct _K2STOR_PART
{
    K2STOR_MEDIA *  mpMedia;
    UINT_PTR        mPartTableEntryIx;
    K2_GUID128      mPartTypeGuid;
    K2_GUID128      mPartIdGuid;
    UINT64          mAttributes;
    UINT64          mMediaStartSectorOffset;
    UINT64          mMediaSectorsCount;
    UINT_PTR        mUserContext;
    UINT8           mPartTypeByte;
    UINT8           mFlagReadOnly;
    UINT8           mFlagActive;
    UINT8           mFlagEFI;
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
    K2STOR_BLOCKIO  BlockIo;
    K2STOR_MEDIA *  mpCurrentMedia;
};

K2STAT
K2STOR_BLOCKDEV_Transfer(
    K2STOR_BLOCKDEV *   apBlockStor,
    UINT64 const *      apBlockIx,
    UINT_PTR            aBlockCount,
    BOOL                aIsWrite,
    UINT_PTR            aBufferAddr
)
{
    K2STOR_MEDIA *  pMedia;

    if ((NULL == apBlockStor) ||
        (0 == apBlockStor->BlockIo.mBlockSizeInBytes) ||
        (0 == apBlockStor->BlockIo.mMaxBlocksOneTransfer) ||
        (0 == apBlockStor->BlockIo.mTransferAlignBytes))
        return K2STAT_ERROR_BAD_ARGUMENT;

    pMedia = apBlockStor->mpCurrentMedia;
    if (NULL == pMedia)
        return K2STAT_ERROR_NO_MEDIA;

    if ((apBlockStor->BlockIo.mBlockSizeInBytes != pMedia->mBytesPerSector) ||
        ((*apBlockIx) >= pMedia->mTotalSectorsCount) ||
        (aBlockCount > apBlockStor->BlockIo.mMaxBlocksOneTransfer) ||
        ((pMedia->mTotalSectorsCount - (*apBlockIx)) < aBlockCount) ||
        ((aBufferAddr % apBlockStor->BlockIo.mTransferAlignBytes) != 0))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((aIsWrite) && ((pMedia->mFlagReadOnly) || (apBlockStor->BlockIo.mIsReadOnlyDevice)))
        return K2STAT_ERROR_READ_ONLY;

    return apBlockStor->BlockIo.Transfer(&apBlockStor->BlockIo, apBlockIx, aBlockCount, aIsWrite, aBufferAddr);
}

K2STAT 
K2STOR_PART_Transfer(
    K2STOR_PART *   apStorPart,
    UINT64 const *  apBlockIx,
    UINT_PTR        aBlockCount,
    BOOL            aIsWrite,
    UINT_PTR        aBufferAddr
)
{
    UINT64              trans;
    K2STOR_MEDIA *      pMedia;
    K2STOR_BLOCKDEV *   pBlockDev;

    if (NULL == apStorPart)
        return K2STAT_ERROR_BAD_ARGUMENT;

    pMedia = apStorPart->mpMedia;
    if (NULL == pMedia)
        return K2STAT_ERROR_NO_MEDIA;



        (NULL == apStorPart->mpMedia) ||
        (NULL == apStorPart->mpMedia->mpCurrentMount) ||
        ((*apBlockIx) >= apStorPart->mMediaSectorsCount) ||
        ((apStorPart->mMediaSectorsCount - (*apBlockIx)) < aBlockCount))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((aIsWrite) && (apStorPart->mFlagReadOnly))
        return K2STAT_ERROR_READ_ONLY;

    trans = (*apBlockIx) + apStorPart->mMediaStartSectorOffset;

    return K2STOR_BLOCKDEV_Transfer(apStorPart->mpMedia->mpCurrentMount, &trans, aBlockCount, aIsWrite, apBuffer);
}

BOOL
K2STOR_PART_MBRPartIsValid(
    K2STOR_MEDIA const *               apMedia,
    FAT_MBR_PARTITION_ENTRY const * apEnt,
    UINT_PTR *                      apRetStartSector,
    UINT_PTR *                      apRetSectorCount
)
{
    UINT_PTR    space;
    UINT64      space64;

    if (0 == apEnt->mFileSystem)
        return FALSE;

    if ((0 == apEnt->mTotalSectors) &&
        (0 == apEnt->mStartSector))
    {
        UINT32 c;
        UINT32 h;
        UINT32 s;

        //
        // try CHS
        //
        c = apEnt->mFirstSector & 0xC0;
        c = (c << 2) | apEnt->mFirstTrack;
        h = apEnt->mFirstHead;
        s = apEnt->mFirstSector & 0x3F;

        if ((0 == s) ||
            (s >= apMedia->mNumSectorsPerTrack) ||
            (c >= apMedia->mNumCylinders) ||
            (h >= apMedia->mNumHeads))
            return FALSE;

        space = (c * apMedia->mNumHeads * apMedia->mNumSectorsPerTrack) +
            (h * apMedia->mNumSectorsPerTrack) +
            (s - 1);

        if (space >= apMedia->mTotalSectorsCount)
            return FALSE;

        *apRetStartSector = space;

        c = apEnt->mLastSector & 0xC0;
        c = (c << 2) | apEnt->mLastTrack;
        h = apEnt->mLastHead;
        s = apEnt->mLastSector & 0x3F;

        if ((0 == s) ||
            (s >= apMedia->mNumSectorsPerTrack) ||
            (c >= apMedia->mNumCylinders) ||
            (h >= apMedia->mNumHeads))
            return FALSE;

        space = (c * apMedia->mNumHeads * apMedia->mNumSectorsPerTrack) +
            (h * apMedia->mNumSectorsPerTrack) +
            (s - 1);

        if ((space < (*apRetStartSector)) ||
            (space >= apMedia->mTotalSectorsCount))
            return FALSE;

        *apRetSectorCount = space - (*apRetStartSector) + 1;

        return TRUE;
    }

    //
    // try to use LBA
    //
    if ((0 == apEnt->mTotalSectors) ||
        (apEnt->mStartSector < 1) ||
        (apEnt->mStartSector >= apMedia->mTotalSectorsCount))
        return FALSE;

    space64 = apMedia->mTotalSectorsCount - apEnt->mStartSector;
    if (space64 < (UINT64)apEnt->mTotalSectors)
        return FALSE;

    *apRetStartSector = apEnt->mStartSector;
    *apRetSectorCount = apEnt->mTotalSectors;

    return TRUE;
}

K2STAT
K2STOR_PART_DiscoverMBR(
    K2STOR_BLOCKIO const *     apBlockIo,
    K2STOR_MEDIA const *   apMedia,
    UINT_PTR *          apRetPartCount,
    K2STOR_PART **         appRetPartArray
)
{
    UINT8                       sector0[K2STOR_SECTOR_BYTES];
    K2STAT                      stat;
    UINT_PTR                    validMask;
    UINT_PTR                    ix;
    UINT_PTR                    partCount;
    FAT_MBR_PARTITION_ENTRY *   pPart;
    FAT_GENERIC_BOOTSECTOR *    pBootSec;
    UINT_PTR                    partStart[4];
    UINT_PTR                    partSectors[4];
    K2STOR_PART *                  pRet;
    UINT64                      blockIx;

    blockIx = 0;
    stat = apBlockIo->Transfer(apBlockIo, &blockIx, 1, FALSE, &sector0);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if ((sector0[510] != 0x55) ||
        (sector0[511] != 0xAA))
    {
        return K2STAT_ERROR_NOT_FOUND;
    }

    pBootSec = (FAT_GENERIC_BOOTSECTOR *)sector0;
    pPart = (FAT_MBR_PARTITION_ENTRY *)&sector0[446];
    validMask = 0;
    partCount = 0;
    for (ix = 0; ix < 4; ix++)
    {
        if (K2STOR_PART_MBRPartIsValid(apMedia, &pPart[ix], &partStart[ix], &partSectors[ix]))
        {
            partCount++;
            validMask |= (1 << ix);
        }
    }

    if (0 == partCount)
    {
        return K2STAT_ERROR_NOT_FOUND;
    }

    pRet = (K2STOR_PART *)malloc(partCount * sizeof(K2STOR_PART));
    if (NULL == pRet)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    K2MEM_Zero(pRet, partCount * sizeof(K2STOR_PART));

    for (ix = 0; ix < 4; ix++)
    {
        if (validMask & (1 << ix))
        {
            pRet[ix].mPartTableEntryIx = ix;
            pRet[ix].mMediaStartSectorOffset = partStart[ix];
            pRet[ix].mMediaSectorsCount = partSectors[ix];
            pRet[ix].mFlagReadOnly = apMedia->mFlagReadOnly;
            pRet[ix].mFlagActive = ((pPart[ix].mBootInd & 0x80) != 0) ? TRUE : FALSE;
            pRet[ix].mPartTypeByte = pPart[ix].mFileSystem;
        }
    }

    *apRetPartCount = partCount;
    *appRetPartArray = pRet;

    return K2STAT_NO_ERROR;
}

K2_PACKED_PUSH
typedef struct _GPT_HEADER GPT_HEADER;
struct _GPT_HEADER
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
K2_PACKED_POP
K2_STATIC_ASSERT(sizeof(GPT_HEADER) == 92);

K2_PACKED_PUSH
typedef struct _GPT_SECTOR GPT_SECTOR;
struct _GPT_SECTOR
{
    GPT_HEADER  Header;
    UINT8       Reserved[K2STOR_SECTOR_BYTES - 92];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
K2_STATIC_ASSERT(sizeof(GPT_SECTOR) == K2STOR_SECTOR_BYTES);

K2_PACKED_PUSH
typedef struct _GPT_ENTRY GPT_ENTRY;
struct _GPT_ENTRY
{
    K2_GUID128  PartitionTypeGuid;
    K2_GUID128  UniquePartitionGuid;
    UINT64      StartingLBA;
    UINT64      EndingLBA;
    UINT64      Attributes;
    UINT16      UnicodePartitionName[36];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
K2_STATIC_ASSERT(sizeof(GPT_ENTRY) == 128);

#define GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY          (0x1000000000000000)

K2STAT
K2STOR_PART_DiscoverGPT(
    GPT_SECTOR const *  apSector1,
    K2STOR_BLOCKIO const *     apBlockIo,
    K2STOR_MEDIA const *   apMedia,
    UINT_PTR *          apRetPartCount,
    K2STOR_PART **         appRetPartArray
)
{
    static K2_GUID128 const sBasicPartGuid = { 0xEBD0A0A2, 0xB9E5, 0x4433, { 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 } };
    UINT64      partitionTableSize;
    GPT_SECTOR  gptSector2;
    UINT64      blockIx;
    K2STAT      stat;
    UINT32      crc;
    UINT32      zero;
    GPT_ENTRY   entry;
    UINT8 *     pPartTab;
    UINT8 *     pScan;
    UINT_PTR    ixPart;
    UINT_PTR    partCount;
    K2STOR_PART *  pPartArray;
    K2STOR_PART *  pPart;

    *apRetPartCount = 0;
    *appRetPartArray = NULL;

    if ((apSector1->Header.HeaderSize < sizeof(GPT_HEADER)) ||
        (apSector1->Header.HeaderSize >= apMedia->mBytesPerSector))
        return K2STAT_ERROR_CORRUPTED;

    zero = 0;
    crc = K2CRC_Calc32(0, apSector1, 16);
    crc = K2CRC_Calc32(crc, &zero, 4);
    crc = K2CRC_Calc32(crc, &apSector1->Header.Reserved, apSector1->Header.HeaderSize - 20);
    if (apSector1->Header.HeaderCRC32 != crc)
        return K2STAT_ERROR_CORRUPTED;

    if ((apSector1->Header.MyLBA != 1) ||
        (apSector1->Header.FirstUsableLBA < 2) ||
        (apSector1->Header.AlternateLBA >= apMedia->mTotalSectorsCount) ||
        (apSector1->Header.LastUsableLBA >= apMedia->mTotalSectorsCount) ||
        (apSector1->Header.FirstUsableLBA >= apSector1->Header.LastUsableLBA))
        return K2STAT_ERROR_CORRUPTED;

    if ((apSector1->Header.PartitionEntryLBA < 2) ||
        ((apSector1->Header.PartitionEntryLBA >= apSector1->Header.FirstUsableLBA) &&
         (apSector1->Header.PartitionEntryLBA <= apSector1->Header.LastUsableLBA)) ||
        (sizeof(GPT_ENTRY) > apSector1->Header.SizeOfPartitionEntry) ||
        (apSector1->Header.NumberOfPartitionEntries == 0))
        return K2STAT_ERROR_CORRUPTED;

    partitionTableSize = apSector1->Header.SizeOfPartitionEntry * apSector1->Header.NumberOfPartitionEntries;
    partitionTableSize = (partitionTableSize + (apMedia->mBytesPerSector - 1)) / (apMedia->mBytesPerSector);
    if (apSector1->Header.PartitionEntryLBA < apSector1->Header.FirstUsableLBA)
    {
        if ((apSector1->Header.FirstUsableLBA - apSector1->Header.PartitionEntryLBA) < partitionTableSize)
            return K2STAT_ERROR_CORRUPTED;
    }
    else
    {
        if ((apMedia->mTotalSectorsCount - apSector1->Header.PartitionEntryLBA) < partitionTableSize)
            return K2STAT_ERROR_CORRUPTED;
    }

    blockIx = apSector1->Header.AlternateLBA;
    stat = apBlockIo->Transfer(apBlockIo, &blockIx, 1, FALSE, &gptSector2);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (gptSector2.Header.HeaderSize != apSector1->Header.HeaderSize)
        return K2STAT_ERROR_CORRUPTED;

    crc = K2CRC_Calc32(0, &gptSector2, 16);
    crc = K2CRC_Calc32(crc, &zero, 4);
    crc = K2CRC_Calc32(crc, &gptSector2.Header.Reserved, gptSector2.Header.HeaderSize - 20);
    if (gptSector2.Header.HeaderCRC32 != crc)
        return K2STAT_ERROR_CORRUPTED;

    if ((gptSector2.Header.Revision != apSector1->Header.Revision) ||
        (gptSector2.Header.HeaderSize != apSector1->Header.HeaderSize) ||
        (gptSector2.Header.MyLBA != blockIx) ||
        (gptSector2.Header.AlternateLBA != 1) ||
        (gptSector2.Header.FirstUsableLBA != apSector1->Header.FirstUsableLBA) ||
        (gptSector2.Header.LastUsableLBA != apSector1->Header.LastUsableLBA) ||
        (0 != K2MEM_Compare(&gptSector2.Header.DiskGuid, &apSector1->Header.DiskGuid,sizeof(K2_GUID128))) ||
        (gptSector2.Header.NumberOfPartitionEntries != apSector1->Header.NumberOfPartitionEntries) ||
        (gptSector2.Header.SizeOfPartitionEntry != apSector1->Header.SizeOfPartitionEntry) ||
        (gptSector2.Header.PartitionEntryArrayCRC32 != apSector1->Header.PartitionEntryArrayCRC32) ||
        (gptSector2.Header.PartitionEntryLBA >= apMedia->mTotalSectorsCount))
        return K2STAT_ERROR_CORRUPTED;

    if ((gptSector2.Header.PartitionEntryLBA < 2) ||
        ((gptSector2.Header.PartitionEntryLBA >= gptSector2.Header.FirstUsableLBA) &&
         (gptSector2.Header.PartitionEntryLBA <= gptSector2.Header.LastUsableLBA)))
        return K2STAT_ERROR_CORRUPTED;

    if (gptSector2.Header.PartitionEntryLBA < gptSector2.Header.FirstUsableLBA)
    {
        if ((gptSector2.Header.FirstUsableLBA - gptSector2.Header.PartitionEntryLBA) < partitionTableSize)
            return K2STAT_ERROR_CORRUPTED;
    }
    else
    {
        if ((apMedia->mTotalSectorsCount - gptSector2.Header.PartitionEntryLBA) < partitionTableSize)
            return K2STAT_ERROR_CORRUPTED;
    }

    //
    // now load the partition table
    //
    pPartTab = (UINT8 *)malloc(((size_t)partitionTableSize) * apMedia->mBytesPerSector);
    if (NULL == pPartTab)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    blockIx = apSector1->Header.PartitionEntryLBA;
    stat = apBlockIo->Transfer(apBlockIo, &blockIx, (UINT_PTR)partitionTableSize, FALSE, pPartTab);
    if (K2STAT_IS_ERROR(stat))
    {
        free(pPartTab);
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    crc = K2CRC_Calc32(0, pPartTab, apSector1->Header.NumberOfPartitionEntries * apSector1->Header.SizeOfPartitionEntry);
    if (crc != apSector1->Header.PartitionEntryArrayCRC32)
    {
        free(pPartTab);
        return K2STAT_ERROR_CORRUPTED;
    }

    stat = K2STAT_NO_ERROR;
    pScan = pPartTab;
    partCount = 0;
    pPartArray = NULL;
    for (ixPart = 0; ixPart < apSector1->Header.NumberOfPartitionEntries; ixPart++)
    {
        K2MEM_Copy(&entry, pScan, sizeof(GPT_ENTRY));
        pScan += apSector1->Header.SizeOfPartitionEntry;
        if (!K2MEM_VerifyZero(&entry.PartitionTypeGuid, sizeof(K2_GUID128)))
        {
            if ((entry.StartingLBA < apMedia->mTotalSectorsCount) &&
                (entry.EndingLBA < apMedia->mTotalSectorsCount) &&
                (entry.EndingLBA >= entry.StartingLBA))
            {
                //
                // valid partition
                //
                partCount++;
            }
        }
    }

    if (0 != partCount)
    {
        pPartArray = (K2STOR_PART *)malloc(sizeof(K2STOR_PART) * partCount);
        if (NULL == pPartArray)
        {
            partCount = 0;
        }
        else
        {
            K2MEM_Zero(pPartArray, sizeof(K2STOR_PART) * partCount);
            pScan = pPartTab;
            pPart = pPartArray;
            for (ixPart = 0; ixPart < apSector1->Header.NumberOfPartitionEntries; ixPart++)
            {
                K2MEM_Copy(&entry, pScan, sizeof(GPT_ENTRY));
                pScan += apSector1->Header.SizeOfPartitionEntry;
                if (!K2MEM_VerifyZero(&entry.PartitionTypeGuid, sizeof(K2_GUID128)))
                {
                    if ((entry.StartingLBA < apMedia->mTotalSectorsCount) &&
                        (entry.EndingLBA < apMedia->mTotalSectorsCount) &&
                        (entry.EndingLBA >= entry.StartingLBA))
                    {
                        pPart->mAttributes = entry.Attributes;
                        pPart->mFlagActive = ((entry.Attributes & GPT_ATTRIBUTE_LEGACY_BIOS_BOOTABLE) != 0) ? TRUE : FALSE;
                        if (0 == K2MEM_Compare(&sBasicPartGuid, &entry.PartitionTypeGuid, sizeof(K2_GUID128)))
                        {
                            pPart->mFlagReadOnly = ((entry.Attributes & GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY) != 0) ? TRUE : FALSE;
                        }
                        pPart->mMediaSectorsCount = (entry.EndingLBA - entry.StartingLBA) + 1;
                        pPart->mMediaStartSectorOffset = entry.StartingLBA;
                        pPart->mPartTableEntryIx = ixPart;
                        K2MEM_Copy(&pPart->mPartIdGuid, &entry.UniquePartitionGuid, sizeof(K2_GUID128));
                        K2MEM_Copy(&pPart->mPartTypeGuid, &entry.PartitionTypeGuid, sizeof(K2_GUID128));
                        pPart++;
                    }
                }
            }
        }
    }

    free(pPartTab);

    if (!K2STAT_IS_ERROR(stat))
    {
        *apRetPartCount = partCount;
        *appRetPartArray = pPartArray;
    }

    return stat;
}

K2STAT 
K2STOR_PART_Discover(
    K2STOR_BLOCKIO const *     apBlockIo,
    K2STOR_MEDIA const *   apMedia,
    UINT_PTR *          apRetPartCount,
    K2STOR_PART **         appRetPartArray
)
{
    GPT_SECTOR  gptSector;
    UINT64      blockIx;
    K2STAT      stat;

    if ((NULL == apBlockIo) ||
        (NULL == apBlockIo->Transfer) ||
        (0 == apBlockIo->mBlockSizeInBytes))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((NULL == apMedia) || 
        (0 == apMedia->mBytesPerSector) ||
        (0 == apMedia->mTotalSectorsCount))
        return K2STAT_ERROR_BAD_ARGUMENT;

    *apRetPartCount = 0;
    *appRetPartArray = NULL;

    K2_ASSERT(K2STOR_SECTOR_BYTES == apBlockIo->mBlockSizeInBytes);
    
    blockIx = 1;
    stat = apBlockIo->Transfer(apBlockIo, &blockIx, 1, FALSE, &gptSector);
    
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (!K2ASC_CompLen((char *)&gptSector.Header.Signature, "EFI PART", 8))
    {
        return K2STOR_PART_DiscoverGPT(&gptSector, apBlockIo, apMedia, apRetPartCount, appRetPartArray);
    }

    return K2STOR_PART_DiscoverMBR(apBlockIo, apMedia, apRetPartCount, appRetPartArray);
}

K2STAT
K2STOR_BLOCKDEV_Discover(
    K2STOR_BLOCKDEV * apBlockStor
)
{
    K2STAT      stat;
    UINT_PTR    ix;

    if (NULL == apBlockStor)
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((NULL == apBlockStor->BlockIo.Transfer) ||
        (0 == apBlockStor->BlockIo.mBlockSizeInBytes))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((apBlockStor->BlockIo.mBlockSizeInBytes != apBlockStor->Media.mBytesPerSector) ||
        (0 == apBlockStor->Media.mTotalSectorsCount))
        return K2STAT_ERROR_BAD_ARGUMENT;

    stat = K2STOR_PART_Discover(&apBlockStor->BlockIo, &apBlockStor->Media, &apBlockStor->mPartCount, &apBlockStor->mpPartArray);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (!K2STAT_IS_ERROR(stat))
    {
        for (ix = 0; ix < apBlockStor->mPartCount; ix++)
        {
            apBlockStor->mpPartArray[ix].mpBlockStor = apBlockStor;
        }
    }

    return stat;
}


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
    K2STOR_BLOCKDEV   BlockStor;
};

K2STAT
vhdTransfer(
    K2STOR_BLOCKIO const * apBlockIo,
    UINT64 const *  apBlockIx,
    UINT_PTR        aBlockCount,
    BOOL            aIsWrite,
    void *          apBuffer
)
{
    VHD *       pVhd;

    if ((NULL == apBlockIo) ||
        (vhdTransfer != apBlockIo->Transfer) ||
        (0 == apBlockIo->mBlockSizeInBytes) ||
        (NULL == apBuffer))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if (0 == aBlockCount)
        return K2STAT_NO_ERROR;

    pVhd = K2_GET_CONTAINER(VHD, apBlockIo, BlockStor.BlockIo);

    LARGE_INTEGER li;
    li.QuadPart = pVhd->BlockStor.Media.mBytesPerSector;
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
        ok = WriteFile(pVhd->mHandle, apBuffer, pVhd->BlockStor.Media.mBytesPerSector * aBlockCount, &bytesIo, &overlapped);
    }
    else
    {
        ok = ReadFile(pVhd->mHandle, apBuffer, pVhd->BlockStor.Media.mBytesPerSector * aBlockCount, &bytesIo, &overlapped);
    }
    if (!ok)
    {
        if (ERROR_IO_PENDING == GetLastError())
        {
            if (!GetOverlappedResult(pVhd->mHandle, &overlapped, &bytesIo, TRUE))
            {
                return (K2STAT)(HRESULT_FROM_WIN32(GetLastError()));
            }
        }
    }

    return K2STAT_NO_ERROR;
}

VHD gVHD;

int main(int argc, char **argv)
{
    static WCHAR const *spDiskFileName = L"C:\\users\\kurt_\\desktop\\Test.VHD";

    ZeroMemory(&gVHD, sizeof(gVHD));

    HANDLE hFile;
    hFile = CreateFile(spDiskFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
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

    gVHD.BlockStor.BlockIo.Transfer = vhdTransfer;
    gVHD.BlockStor.BlockIo.mBlockSizeInBytes = info.Size.SectorSize;
    gVHD.BlockStor.BlockIo.mIsReadOnlyDevice = FALSE;

    _snprintf_s((char *)&gVHD.BlockStor.Media.mSerialNumber, 31, 17, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
        vhdFooter.mUniqueId[0], vhdFooter.mUniqueId[1], vhdFooter.mUniqueId[2], vhdFooter.mUniqueId[3],
        vhdFooter.mUniqueId[4], vhdFooter.mUniqueId[5], vhdFooter.mUniqueId[6], vhdFooter.mUniqueId[7],
        vhdFooter.mUniqueId[8], vhdFooter.mUniqueId[9], vhdFooter.mUniqueId[10], vhdFooter.mUniqueId[11],
        vhdFooter.mUniqueId[12], vhdFooter.mUniqueId[13], vhdFooter.mUniqueId[14], vhdFooter.mUniqueId[15]);
    gVHD.BlockStor.Media.mNumSectorsPerTrack = vhdFooter.mDiskGeo_NumSectorsPerTrack;
    gVHD.BlockStor.Media.mNumCylinders = vhdFooter.mDiskGeo_NumCyl;
    gVHD.BlockStor.Media.mNumHeads = vhdFooter.mDiskGeo_NumHeads;
    gVHD.BlockStor.Media.mBytesPerSector = (UINT16)info.Size.SectorSize;
    gVHD.BlockStor.Media.mTotalSectorsCount = info.Size.VirtualSize / info.Size.SectorSize;
 
    K2STAT stat;
    stat = K2STOR_BLOCKDEV_Discover(&gVHD.BlockStor);
    if (K2STAT_IS_ERROR(stat))
    {
        return -1;
    }

    UINT64 blockIx;
    FAT_GENERIC_BOOTSECTOR bootSector;

    blockIx = 0;
    stat = K2STOR_PART_Transfer(&gVHD.BlockStor.mpPartArray[0], &blockIx, 1, FALSE, &bootSector);
    if (K2STAT_IS_ERROR(stat))
    {
        return -1;
    }




    return 0;
}