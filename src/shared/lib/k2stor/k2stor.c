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

#include <lib/k2stor.h>

K2STAT
K2STOR_BLOCKDEV_Transfer(
    K2STOR_BLOCKDEV const * apBlockDev,
    UINT64 const *          apBlockIx,
    UINT_PTR                aBlockCount,
    BOOL                    aIsWrite,
    UINT_PTR                aBufferAddr
)
{
    K2STOR_MEDIA *  pMedia;

    if ((NULL == apBlockDev) ||
        (0 == apBlockDev->BlockIo.mBlockSizeInBytes) ||
        (0 == apBlockDev->BlockIo.mMaxBlocksOneTransfer) ||
        (0 == apBlockDev->BlockIo.mTransferAlignBytes))
        return K2STAT_ERROR_BAD_ARGUMENT;

    pMedia = apBlockDev->mpCurrentMedia;
    if (NULL == pMedia)
        return K2STAT_ERROR_NO_MEDIA;

    if ((apBlockDev->BlockIo.mBlockSizeInBytes != pMedia->mBytesPerSector) ||
        ((*apBlockIx) >= pMedia->mTotalSectorsCount) ||
        (aBlockCount > apBlockDev->BlockIo.mMaxBlocksOneTransfer) ||
        ((pMedia->mTotalSectorsCount - (*apBlockIx)) < aBlockCount) ||
        ((aBufferAddr % apBlockDev->BlockIo.mTransferAlignBytes) != 0))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((aIsWrite) && ((pMedia->mFlagReadOnly) || (apBlockDev->BlockIo.mIsReadOnlyDevice)))
        return K2STAT_ERROR_READ_ONLY;

    return apBlockDev->BlockIo.Transfer(&apBlockDev->BlockIo, apBlockIx, aBlockCount, aIsWrite, aBufferAddr);
}

K2STAT 
K2STOR_PART_Transfer(
    K2STOR_BLOCKDEV const *apBlockDev,
    UINT_PTR        aPartIx,
    UINT64 const *  apPartBlockOffset,
    UINT_PTR        aBlockCount,
    BOOL            aIsWrite,
    UINT_PTR        aBufferAddr
)
{
    UINT64                  trans;
    K2STOR_MEDIA const *    pMedia;
    K2STOR_PART const *     pPart;

    if (NULL == apBlockDev)
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((aIsWrite) && (apBlockDev->BlockIo.mIsReadOnlyDevice))
        return K2STAT_ERROR_READ_ONLY;

    pMedia = apBlockDev->mpCurrentMedia;
    if (NULL == pMedia)
        return K2STAT_ERROR_NO_MEDIA;

    if ((aPartIx > apBlockDev->mCurrentPartCount) ||
        (NULL == apBlockDev->mpCurrentPartArray))
        return K2STAT_ERROR_OUT_OF_BOUNDS;

    pPart = &apBlockDev->mpCurrentPartArray[aPartIx];

    if ((aIsWrite) && (pPart->mFlagReadOnly))
        return K2STAT_ERROR_READ_ONLY;

    if (((*apPartBlockOffset) >= pPart->mMediaSectorsCount) ||
        ((pPart->mMediaSectorsCount - (*apPartBlockOffset)) < aBlockCount))
        return K2STAT_ERROR_BAD_ARGUMENT;

    trans = (*apPartBlockOffset) + pPart->mMediaStartSectorOffset;

    return K2STOR_BLOCKDEV_Transfer(apBlockDev, &trans, aBlockCount, aIsWrite, aBufferAddr);
}

BOOL
K2STOR_PART_MBRPartIsValid(
    K2STOR_MEDIA const *            apMedia,
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
K2STOR_PART_DiscoverFromMBR(
    K2STOR_MEDIA const *    apMedia,
    UINT8 const *           apSector0,
    UINT_PTR *              apIoPartCount,
    K2STOR_PART *           apRetPartEntries
)
{
    UINT_PTR                    validMask;
    UINT_PTR                    ix;
    UINT_PTR                    partCount;
    FAT_MBR_PARTITION_ENTRY *   pPart;
    UINT_PTR                    partStart[4];
    UINT_PTR                    partSectors[4];
    UINT_PTR                    maxCount;

    if ((NULL == apSector0) ||
        (NULL == apIoPartCount))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if (NULL == apRetPartEntries)
    {
        *apIoPartCount = 0;
    }

    maxCount = *apIoPartCount;

    if ((apSector0[510] != 0x55) ||
        (apSector0[511] != 0xAA))
    {
        return K2STAT_ERROR_NOT_FOUND;
    }

    pPart = (FAT_MBR_PARTITION_ENTRY *)&apSector0[446];
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

    *apIoPartCount = partCount;

    if (0 == partCount)
    {
        return K2STAT_NO_ERROR;
    }

    if (partCount > maxCount)
    {
        return K2STAT_ERROR_OUTBUF_TOO_SMALL;
    }

    if (NULL == apRetPartEntries)
    {
        return K2STAT_ERROR_NULL_POINTER;
    }

    K2MEM_Zero(apRetPartEntries, partCount * sizeof(K2STOR_PART));

    for (ix = 0; ix < 4; ix++)
    {
        if (validMask & (1 << ix))
        {
            apRetPartEntries[ix].mPartTableEntryIx = ix;
            apRetPartEntries[ix].mMediaStartSectorOffset = partStart[ix];
            apRetPartEntries[ix].mMediaSectorsCount = partSectors[ix];
            apRetPartEntries[ix].mFlagReadOnly = apMedia->mFlagReadOnly;
            apRetPartEntries[ix].mFlagActive = ((pPart[ix].mBootInd & 0x80) != 0) ? TRUE : FALSE;
            apRetPartEntries[ix].mPartTypeByte = pPart[ix].mFileSystem;
        }
    }

    return K2STAT_NO_ERROR;
}

K2STAT 
K2STOR_PART_ValidateGPT1(
    GPT_SECTOR const *   apSector1,
    K2STOR_MEDIA const * apMedia
)
{
    UINT64  partitionTableSize;
    UINT32  crc;
    UINT32  zero;

    if ((NULL == apSector1) ||
        (NULL == apMedia))
        return K2STAT_ERROR_BAD_ARGUMENT;

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

    return K2STAT_NO_ERROR;
}

K2STAT
K2STOR_PART_ValidateGPTAlt(
    GPT_SECTOR const *      apSector1,
    GPT_SECTOR const *      apAltSector,
    K2STOR_MEDIA const *    apMedia
)
{
    UINT64  partitionTable1Size;
    UINT64  partitionTable2Size;
    UINT32  crc;
    UINT32  zero;
    K2STAT  stat;

    if ((NULL == apSector1) ||
        (NULL == apAltSector) ||
        (NULL == apMedia))
        return K2STAT_ERROR_BAD_ARGUMENT;

    stat = K2STOR_PART_ValidateGPT1(apSector1, apMedia);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    partitionTable1Size = apSector1->Header.SizeOfPartitionEntry * apSector1->Header.NumberOfPartitionEntries;
    partitionTable1Size = (partitionTable1Size + (apMedia->mBytesPerSector - 1)) / (apMedia->mBytesPerSector);

    if ((apAltSector->Header.HeaderSize != apSector1->Header.HeaderSize) ||
        (apSector1->Header.AlternateLBA != apAltSector->Header.MyLBA))
        return K2STAT_ERROR_CORRUPTED;

    zero = 0;
    crc = K2CRC_Calc32(0, apAltSector, 16);
    crc = K2CRC_Calc32(crc, &zero, 4);
    crc = K2CRC_Calc32(crc, &apAltSector->Header.Reserved, apAltSector->Header.HeaderSize - 20);
    if (apAltSector->Header.HeaderCRC32 != crc)
        return K2STAT_ERROR_CORRUPTED;

    if ((apAltSector->Header.Revision != apSector1->Header.Revision) ||
        (apAltSector->Header.HeaderSize != apSector1->Header.HeaderSize) ||
        (apAltSector->Header.MyLBA != apSector1->Header.AlternateLBA) ||
        (apAltSector->Header.AlternateLBA != apSector1->Header.MyLBA) ||
        (apAltSector->Header.FirstUsableLBA != apSector1->Header.FirstUsableLBA) ||
        (apAltSector->Header.LastUsableLBA != apSector1->Header.LastUsableLBA) ||
        (0 != K2MEM_Compare(&apAltSector->Header.DiskGuid, &apSector1->Header.DiskGuid, sizeof(K2_GUID128))) ||
        (apAltSector->Header.NumberOfPartitionEntries != apSector1->Header.NumberOfPartitionEntries) ||
        (apAltSector->Header.SizeOfPartitionEntry != apSector1->Header.SizeOfPartitionEntry) ||
        (apAltSector->Header.PartitionEntryArrayCRC32 != apSector1->Header.PartitionEntryArrayCRC32) ||
        (apAltSector->Header.PartitionEntryLBA >= apMedia->mTotalSectorsCount))
        return K2STAT_ERROR_CORRUPTED;

    if ((apAltSector->Header.PartitionEntryLBA < 2) ||
        ((apAltSector->Header.PartitionEntryLBA >= apAltSector->Header.FirstUsableLBA) &&
            (apAltSector->Header.PartitionEntryLBA <= apAltSector->Header.LastUsableLBA)))
        return K2STAT_ERROR_CORRUPTED;

    partitionTable2Size = apSector1->Header.SizeOfPartitionEntry * apSector1->Header.NumberOfPartitionEntries;
    partitionTable2Size = (partitionTable2Size + (apMedia->mBytesPerSector - 1)) / (apMedia->mBytesPerSector);
    if (partitionTable1Size != partitionTable2Size)
        return K2STAT_ERROR_CORRUPTED;

    if (apAltSector->Header.PartitionEntryLBA < apAltSector->Header.FirstUsableLBA)
    {
        if ((apAltSector->Header.FirstUsableLBA - apAltSector->Header.PartitionEntryLBA) < partitionTable1Size)
            return K2STAT_ERROR_CORRUPTED;
    }
    else
    {
        if ((apMedia->mTotalSectorsCount - apAltSector->Header.PartitionEntryLBA) < partitionTable1Size)
            return K2STAT_ERROR_CORRUPTED;
    }

    return K2STAT_NO_ERROR;
}

K2STAT
K2STOR_PART_ValidateGPTPartitions(
    GPT_SECTOR const *      apSector1,
    GPT_SECTOR const *      apAltSector,
    K2STOR_MEDIA const *    apMedia,
    UINT8 const *           apPartTab1,
    UINT8 const *           apPartTab2,
    UINT_PTR *              apRetNonEmptyPartCount
)
{
    K2STAT          stat;
    UINT64          partitionTableSize;
    UINT32          alignEntry[(sizeof(GPT_ENTRY) + 3) / 4];
    GPT_ENTRY *     pEntry;
    UINT8 const *   pScan;
    UINT_PTR        partCount;
    UINT_PTR        ixPart;

    if ((NULL == apSector1) ||
        (NULL == apAltSector) ||
        (NULL == apMedia) ||
        (NULL == apPartTab1) ||
        (NULL == apPartTab2))
        return K2STAT_ERROR_BAD_ARGUMENT;

    stat = K2STOR_PART_ValidateGPTAlt(apSector1, apAltSector, apMedia);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    partitionTableSize = apSector1->Header.SizeOfPartitionEntry * apSector1->Header.NumberOfPartitionEntries;

    if (0 != K2MEM_Compare(apPartTab1, apPartTab2, (UINT_PTR)partitionTableSize))
        return K2STAT_ERROR_CORRUPTED;

    if (apSector1->Header.PartitionEntryArrayCRC32 != 
        K2CRC_Calc32(0, apPartTab1, (UINT_PTR)partitionTableSize))
        return K2STAT_ERROR_CORRUPTED;

    pEntry = (GPT_ENTRY *)&alignEntry[0];

    pScan = apPartTab1;
    partCount = 0;
    for (ixPart = 0; ixPart < apSector1->Header.NumberOfPartitionEntries; ixPart++)
    {
        K2MEM_Copy(pEntry, pScan, sizeof(GPT_ENTRY));
        pScan += apSector1->Header.SizeOfPartitionEntry;
        if (0 == K2MEM_VerifyZero(&pEntry->PartitionTypeGuid, sizeof(K2_GUID128)))
        {
            if ((pEntry->StartingLBA >= apMedia->mTotalSectorsCount) ||
                (pEntry->EndingLBA >= apMedia->mTotalSectorsCount) ||
                (pEntry->EndingLBA < pEntry->StartingLBA))
            {
                //
                // invalid partition
                //
                return K2STAT_ERROR_CORRUPTED;
            }

            partCount++;
        }
    }

    if (NULL != apRetNonEmptyPartCount)
        *apRetNonEmptyPartCount = partCount;

    return K2STAT_NO_ERROR;
}

