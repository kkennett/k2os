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
    FAT_GENERIC_BOOTSECTOR *    pBootSec;
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

    pBootSec = (FAT_GENERIC_BOOTSECTOR *)apSector0;
    pPart = (FAT_MBR_PARTITION_ENTRY *)apSector0[446];
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
K2STOR_PART_DiscoverMBR(
    K2STOR_BLOCKIO const *  apBlockIo,
    K2STOR_MEDIA const *    apMedia,
    UINT_PTR *              apRetPartCount,
    K2STOR_PART **          appRetPartArray
)
{
    UINT8           diskSectorBuffer[K2STOR_SECTOR_BYTES * 2];
    UINT_PTR        align;
    UINT8 *         pSector0;
    K2STAT          stat;
    K2STOR_PART *   pRet;
    UINT64          blockIx;
    UINT_PTR        ioPartCount;

    align = (UINT_PTR)&diskSectorBuffer[0];
    align = ((align + (K2STOR_SECTOR_BYTES - 1)) / K2STOR_SECTOR_BYTES) * K2STOR_SECTOR_BYTES;
    pSector0 = (UINT8 *)align;

    blockIx = 0;
    stat = apBlockIo->Transfer(apBlockIo, &blockIx, 1, FALSE, (UINT_PTR)pSector0);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    ioPartCount = 0;
    stat = K2STOR_PART_DiscoverFromMBR(apMedia, pSector0, &ioPartCount, NULL);
    if ((!K2STAT_IS_ERROR(stat)) || (0 == ioPartCount))
    {
        *apRetPartCount = 0;
        *appRetPartArray = NULL;
        return stat;
    }

    pRet = (K2STOR_PART *)malloc(sizeof(K2STOR_PART) * ioPartCount);
    if (NULL == pRet)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    stat = K2STOR_PART_DiscoverFromMBR(apMedia, pSector0, &ioPartCount, pRet);
    if (K2STAT_IS_ERROR(stat))
    {
        free(pRet);
        return stat;
    }

    *apRetPartCount = ioPartCount;
    *appRetPartArray = pRet;

    return K2STAT_NO_ERROR;
}

K2STAT 
K2STOR_PART_ValidateGPT1(
    K2STOR_GPT_SECTOR const *   apSector1,
    K2STOR_MEDIA const *        apMedia
)
{
    UINT64  partitionTableSize;
    UINT32  crc;
    UINT32  zero;

    if ((NULL == apSector1) ||
        (NULL == apMedia))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((apSector1->Header.HeaderSize < sizeof(K2STOR_GPT_HEADER)) ||
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
        (sizeof(K2STOR_GPT_ENTRY) > apSector1->Header.SizeOfPartitionEntry) ||
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
    K2STOR_GPT_SECTOR const *   apSector1,
    K2STOR_GPT_SECTOR const *   apAltSector,
    K2STOR_MEDIA const *        apMedia
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
    K2STOR_GPT_SECTOR const *   apSector1,
    K2STOR_GPT_SECTOR const *   apAltSector,
    K2STOR_MEDIA const *        apMedia,
    UINT8 const *               apPartTab1,
    UINT8 const *               apPartTab2,
    UINT_PTR *                  apRetNonEmptyPartCount
)
{
    K2STAT              stat;
    UINT64              partitionTableSize;
    UINT32              alignEntry[(sizeof(K2STOR_GPT_ENTRY) + 3) / 4];
    K2STOR_GPT_ENTRY *  pEntry;
    UINT8 const *       pScan;
    UINT_PTR            partCount;
    UINT_PTR            ixPart;

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

    if (0 != K2MEM_Compare(apPartTab1, apPartTab2, partitionTableSize))
        return K2STAT_ERROR_CORRUPTED;

    if (apSector1->Header.PartitionEntryArrayCRC32 != 
        K2CRC_Calc32(0, apPartTab1, partitionTableSize))
        return K2STAT_ERROR_CORRUPTED;

    pEntry = (K2STOR_GPT_ENTRY *)&alignEntry[0];

    pScan = apPartTab1;
    partCount = 0;
    for (ixPart = 0; ixPart < apSector1->Header.NumberOfPartitionEntries; ixPart++)
    {
        K2MEM_Copy(pEntry, pScan, sizeof(K2STOR_GPT_ENTRY));
        pScan += apSector1->Header.SizeOfPartitionEntry;
        if (!K2MEM_VerifyZero(&pEntry->PartitionTypeGuid, sizeof(K2_GUID128)))
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

K2STAT
K2STOR_PART_DiscoverGPT(
    K2STOR_GPT_SECTOR const *   apSector1,
    K2STOR_BLOCKIO const *      apBlockIo,
    K2STOR_MEDIA const *        apMedia,
    UINT_PTR *                  apRetPartCount,
    K2STOR_PART **              appRetPartArray
)
{
    static K2_GUID128 const sBasicPartGuid = K2STOR_GPT_BASIC_DATA_PART_GUID;

    K2STAT              stat;
    UINT_PTR            bufBytes;
    UINT8 *             pBlockBuffer;
    UINT_PTR            align;
    K2STOR_GPT_SECTOR * pAltSector;
    UINT64              blockIx;
    UINT64              partitionTableSize;
    UINT8 *             pTableBuffer1;
    UINT8 *             pPartTab1;
    UINT8 *             pTableBuffer2;
    UINT8 *             pPartTab2;
    UINT_PTR            partCount;

    *apRetPartCount = 0;
    *appRetPartArray = NULL;

    stat = K2STOR_PART_ValidateGPT1(apSector1, apMedia);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    bufBytes = ((apBlockIo->mBlockSizeInBytes + (apBlockIo->mTransferAlignBytes - 1)) / apBlockIo->mTransferAlignBytes) * apBlockIo->mTransferAlignBytes;
    pBlockBuffer = (UINT8 *)malloc(bufBytes);
    if (NULL == pBlockBuffer)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    do {
        align = (UINT_PTR)pBlockBuffer;
        align = ((align + (apBlockIo->mTransferAlignBytes - 1)) / apBlockIo->mTransferAlignBytes) * apBlockIo->mTransferAlignBytes;
        pAltSector = (K2STOR_GPT_SECTOR *)align;

        blockIx = apSector1->Header.AlternateLBA;
        stat = apBlockIo->Transfer(apBlockIo, &blockIx, 1, FALSE, align);
        if (K2STAT_IS_ERROR(stat))
            break;

        stat = K2STOR_PART_ValidateGPTAlt(apSector1, pAltSector, apMedia);
        if (K2STAT_IS_ERROR(stat))
            break;

        partitionTableSize = apSector1->Header.SizeOfPartitionEntry * apSector1->Header.NumberOfPartitionEntries;
        partitionTableSize = (partitionTableSize + (apMedia->mBytesPerSector - 1)) / (apMedia->mBytesPerSector);

        bufBytes = (((apBlockIo->mBlockSizeInBytes * partitionTableSize) + (apBlockIo->mTransferAlignBytes - 1)) / apBlockIo->mTransferAlignBytes) * apBlockIo->mTransferAlignBytes;
        pTableBuffer1 = (UINT8 *)malloc(bufBytes);
        if (NULL == pTableBuffer1)
        {
            stat = K2STAT_ERROR_OUT_OF_MEMORY;
            break;
        }

        do {
            align = (UINT_PTR)pTableBuffer1;
            align = ((align + (apBlockIo->mTransferAlignBytes - 1)) / apBlockIo->mTransferAlignBytes) * apBlockIo->mTransferAlignBytes;
            pPartTab1 = (UINT8 *)align;

            blockIx = apSector1->Header.PartitionEntryLBA;
            stat = apBlockIo->Transfer(apBlockIo, &blockIx, (UINT_PTR)partitionTableSize, FALSE, align);
            if (K2STAT_IS_ERROR(stat))
                break;

            pTableBuffer2 = (UINT8 *)malloc(bufBytes);
            if (NULL == pTableBuffer2)
            {
                stat = K2STAT_ERROR_OUT_OF_MEMORY;
                break;
            }

            do {
                align = (UINT_PTR)pTableBuffer2;
                align = ((align + (apBlockIo->mTransferAlignBytes - 1)) / apBlockIo->mTransferAlignBytes) * apBlockIo->mTransferAlignBytes;
                pPartTab2 = (UINT8 *)align;

                blockIx = pAltSector->Header.PartitionEntryLBA;
                stat = apBlockIo->Transfer(apBlockIo, &blockIx, (UINT_PTR)partitionTableSize, FALSE, align);
                if (K2STAT_IS_ERROR(stat))
                    break;

                stat = K2STOR_PART_ValidateGPTPartitions(apSector1, pAltSector, apMedia, pPartTab2, pPartTab2, &partCount);
                if (K2STAT_IS_ERROR(stat))
                    break;

                //
                // usable partitions!
                //



            } while (0);

            free(pTableBuffer2);

        } while (0);

        free(pTableBuffer1);

    } while (0);

    free(pBlockBuffer);

    return stat;
}




K2STAT
K2STOR_PART_DiscoverGPT1(
    K2STOR_GPT_SECTOR const *   apSector1,
    K2STOR_BLOCKIO const *      apBlockIo,
    K2STOR_MEDIA const *        apMedia,
    UINT_PTR *                  apRetPartCount,
    K2STOR_PART **              appRetPartArray
)
{
    UINT64              partitionTableSize;
    UINT8               diskSectorBuffer[K2STOR_SECTOR_BYTES * 2];
    UINT_PTR            align;
    K2STOR_GPT_SECTOR * apAltSector;
    UINT64              blockIx;
    K2STAT              stat;
    UINT32              crc;
    UINT32              zero;
    K2STOR_GPT_ENTRY    entry;
    UINT8 *             pPartTab;
    UINT8 *             pScan;
    UINT_PTR            ixPart;
    UINT_PTR            partCount;
    K2STOR_PART *       pPartArray;
    K2STOR_PART *       pPart;

    *apRetPartCount = 0;
    *appRetPartArray = NULL;

    if ((apSector1->Header.HeaderSize < sizeof(K2STOR_GPT_HEADER)) ||
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
        (sizeof(K2STOR_GPT_ENTRY) > apSector1->Header.SizeOfPartitionEntry) ||
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

    align = (UINT_PTR)&diskSectorBuffer[0];
    align = ((align + (K2STOR_SECTOR_BYTES - 1)) / K2STOR_SECTOR_BYTES) * K2STOR_SECTOR_BYTES;
    apAltSector = (K2STOR_GPT_SECTOR *)align;

    blockIx = apSector1->Header.AlternateLBA;
    stat = apBlockIo->Transfer(apBlockIo, &blockIx, 1, FALSE, (UINT_PTR)apAltSector);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (apAltSector->Header.HeaderSize != apSector1->Header.HeaderSize)
        return K2STAT_ERROR_CORRUPTED;

    crc = K2CRC_Calc32(0, apAltSector, 16);
    crc = K2CRC_Calc32(crc, &zero, 4);
    crc = K2CRC_Calc32(crc, &apAltSector->Header.Reserved, apAltSector->Header.HeaderSize - 20);
    if (apAltSector->Header.HeaderCRC32 != crc)
        return K2STAT_ERROR_CORRUPTED;

    if ((apAltSector->Header.Revision != apSector1->Header.Revision) ||
        (apAltSector->Header.HeaderSize != apSector1->Header.HeaderSize) ||
        (apAltSector->Header.MyLBA != blockIx) ||
        (apAltSector->Header.AlternateLBA != 1) ||
        (apAltSector->Header.FirstUsableLBA != apSector1->Header.FirstUsableLBA) ||
        (apAltSector->Header.LastUsableLBA != apSector1->Header.LastUsableLBA) ||
        (0 != K2MEM_Compare(&apAltSector->Header.DiskGuid, &apSector1->Header.DiskGuid,sizeof(K2_GUID128))) ||
        (apAltSector->Header.NumberOfPartitionEntries != apSector1->Header.NumberOfPartitionEntries) ||
        (apAltSector->Header.SizeOfPartitionEntry != apSector1->Header.SizeOfPartitionEntry) ||
        (apAltSector->Header.PartitionEntryArrayCRC32 != apSector1->Header.PartitionEntryArrayCRC32) ||
        (apAltSector->Header.PartitionEntryLBA >= apMedia->mTotalSectorsCount))
        return K2STAT_ERROR_CORRUPTED;

    if ((apAltSector->Header.PartitionEntryLBA < 2) ||
        ((apAltSector->Header.PartitionEntryLBA >= apAltSector->Header.FirstUsableLBA) &&
         (apAltSector->Header.PartitionEntryLBA <= apAltSector->Header.LastUsableLBA)))
        return K2STAT_ERROR_CORRUPTED;

    if (apAltSector->Header.PartitionEntryLBA < apAltSector->Header.FirstUsableLBA)
    {
        if ((apAltSector->Header.FirstUsableLBA - apAltSector->Header.PartitionEntryLBA) < partitionTableSize)
            return K2STAT_ERROR_CORRUPTED;
    }
    else
    {
        if ((apMedia->mTotalSectorsCount - apAltSector->Header.PartitionEntryLBA) < partitionTableSize)
            return K2STAT_ERROR_CORRUPTED;
    }

    //
    // now load the partition table
    //
    pPartTab = (UINT8 *)malloc(((UINT_PTR)partitionTableSize) * apMedia->mBytesPerSector);
    if (NULL == pPartTab)
    {
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }

    blockIx = apSector1->Header.PartitionEntryLBA;
    stat = apBlockIo->Transfer(apBlockIo, &blockIx, (UINT_PTR)partitionTableSize, FALSE, (UINT_PTR)pPartTab);
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
        K2MEM_Copy(&entry, pScan, sizeof(K2STOR_GPT_ENTRY));
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
                K2MEM_Copy(&entry, pScan, sizeof(K2STOR_GPT_ENTRY));
                pScan += apSector1->Header.SizeOfPartitionEntry;
                if (!K2MEM_VerifyZero(&entry.PartitionTypeGuid, sizeof(K2_GUID128)))
                {
                    if ((entry.StartingLBA < apMedia->mTotalSectorsCount) &&
                        (entry.EndingLBA < apMedia->mTotalSectorsCount) &&
                        (entry.EndingLBA >= entry.StartingLBA))
                    {
                        pPart->mAttributes = entry.Attributes;
                        pPart->mFlagActive = ((entry.Attributes & K2STOR_GPT_ATTRIBUTE_LEGACY_BIOS_BOOTABLE) != 0) ? TRUE : FALSE;
                        if (0 == K2MEM_Compare(&sBasicPartGuid, &entry.PartitionTypeGuid, sizeof(K2_GUID128)))
                        {
                            pPart->mFlagReadOnly = ((entry.Attributes & K2STOR_GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY) != 0) ? TRUE : FALSE;
                        }
                        pPart->mMediaSectorsCount = (entry.EndingLBA - entry.StartingLBA) + 1;
                        pPart->mMediaStartSectorOffset = entry.StartingLBA;
                        pPart->mPartTableEntryIx = ixPart;
                        K2MEM_Copy(&pPart->mPartIdGuid, &entry.UniquePartitionGuid, sizeof(K2_GUID128));
                        K2MEM_Copy(&pPart->mPartTypeGuid, &entry.PartitionTypeGuid, sizeof(K2_GUID128));
                        pPart->mFlagEFI = TRUE;
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
    K2STOR_BLOCKIO const *  apBlockIo,
    K2STOR_MEDIA const *    apMedia,
    UINT_PTR *              apRetPartCount,
    K2STOR_PART **          appRetPartArray
)
{
    UINT8               diskSectorBuffer[K2STOR_SECTOR_BYTES * 2];
    UINT_PTR            align;
    K2STOR_GPT_SECTOR * pGptSector;
    UINT64              blockIx;
    K2STAT              stat;

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

    align = (UINT_PTR)&diskSectorBuffer[0];
    align = ((align + (K2STOR_SECTOR_BYTES - 1)) / K2STOR_SECTOR_BYTES) * K2STOR_SECTOR_BYTES;
    pGptSector = (K2STOR_GPT_SECTOR *)align;
    
    blockIx = 1;
    stat = apBlockIo->Transfer(apBlockIo, &blockIx, 1, FALSE, (UINT_PTR)pGptSector);
    
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (!K2ASC_CompLen((char *)&pGptSector->Header.Signature, "EFI PART", 8))
    {
        return K2STOR_PART_DiscoverGPT(pGptSector, apBlockIo, apMedia, apRetPartCount, appRetPartArray);
    }

    return K2STOR_PART_DiscoverMBR(apBlockIo, apMedia, apRetPartCount, appRetPartArray);
}

K2STAT
K2STOR_BLOCKDEV_DiscoverMediaPartitions(
    K2STOR_BLOCKDEV * apBlockDev
)
{
    K2STOR_MEDIA *  pMedia;

    if (NULL == apBlockDev)
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((NULL == apBlockDev->BlockIo.Transfer) ||
        (0 == apBlockDev->BlockIo.mBlockSizeInBytes))
        return K2STAT_ERROR_BAD_ARGUMENT;

    pMedia = apBlockDev->mpCurrentMedia;
    if (NULL == pMedia)
        return K2STAT_ERROR_NO_MEDIA;

    if ((NULL != apBlockDev->mpCurrentPartArray) ||
        (0 != apBlockDev->mCurrentPartCount))
        return K2STAT_ERROR_ALREADY_EXISTS;

    if ((apBlockDev->BlockIo.mBlockSizeInBytes != pMedia->mBytesPerSector) ||
        (0 == pMedia->mTotalSectorsCount))
        return K2STAT_ERROR_BAD_ARGUMENT;

    return K2STOR_PART_Discover(&apBlockDev->BlockIo, pMedia, &apBlockDev->mCurrentPartCount, &apBlockDev->mpCurrentPartArray);
}

