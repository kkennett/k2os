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
    K2STOR_BLOCKDEV *   apBlockDev,
    UINT64 const *      apBlockIx,
    UINT_PTR            aBlockCount,
    BOOL                aIsWrite,
    UINT_PTR            aBufferAddr
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

    if ((aIsWrite) && (apStorPart->mFlagReadOnly))
        return K2STAT_ERROR_READ_ONLY;

    pMedia = apStorPart->mpMedia;
    if (NULL == pMedia)
        return K2STAT_ERROR_NO_MEDIA;

    if ((pMedia->mCurrentPartCount <= apStorPart->mPartTableEntryIx) ||
        (apStorPart != &pMedia->mpCurrentPartArray[apStorPart->mPartTableEntryIx]))
        return K2STAT_ERROR_CORRUPTED;

    pBlockDev = pMedia->mpCurrentMount;
    if ((NULL == pBlockDev) ||
        (pMedia != pBlockDev->mpCurrentMedia) ||
        ((*apBlockIx) >= apStorPart->mMediaSectorsCount) ||
        ((apStorPart->mMediaSectorsCount - (*apBlockIx)) < aBlockCount))
        return K2STAT_ERROR_BAD_ARGUMENT;

    trans = (*apBlockIx) + apStorPart->mMediaStartSectorOffset;

    return K2STOR_BLOCKDEV_Transfer(apStorPart->mpMedia->mpCurrentMount, &trans, aBlockCount, aIsWrite, aBufferAddr);
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
K2STOR_PART_DiscoverMBR(
    K2STOR_BLOCKIO const *  apBlockIo,
    K2STOR_MEDIA *          apMedia,
    UINT_PTR *              apRetPartCount,
    K2STOR_PART **          appRetPartArray
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
    K2STOR_PART *               pRet;
    UINT64                      blockIx;

    blockIx = 0;
    stat = apBlockIo->Transfer(apBlockIo, &blockIx, 1, FALSE, (UINT_PTR)&sector0);
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
            pRet[ix].mpMedia = apMedia;
        }
    }

    *apRetPartCount = partCount;
    *appRetPartArray = pRet;

    return K2STAT_NO_ERROR;
}

K2STAT
K2STOR_PART_DiscoverGPT(
    K2STOR_GPT_SECTOR const *   apSector1,
    K2STOR_BLOCKIO const *      apBlockIo,
    K2STOR_MEDIA  *             apMedia,
    UINT_PTR *                  apRetPartCount,
    K2STOR_PART **              appRetPartArray
)
{
    static K2_GUID128 const sBasicPartGuid = K2STOR_GPT_BASIC_DATA_PART_GUID;
    UINT64              partitionTableSize;
    K2STOR_GPT_SECTOR   gptSector2;
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

    blockIx = apSector1->Header.AlternateLBA;
    stat = apBlockIo->Transfer(apBlockIo, &blockIx, 1, FALSE, (UINT_PTR)&gptSector2);
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
                        pPart->mpMedia = apMedia;
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
    K2STOR_MEDIA  *         apMedia,
    UINT_PTR *              apRetPartCount,
    K2STOR_PART **          appRetPartArray
)
{
    K2STOR_GPT_SECTOR  gptSector;
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
    stat = apBlockIo->Transfer(apBlockIo, &blockIx, 1, FALSE, (UINT_PTR)&gptSector);
    
    if (K2STAT_IS_ERROR(stat))
        return stat;

    if (!K2ASC_CompLen((char *)&gptSector.Header.Signature, "EFI PART", 8))
    {
        return K2STOR_PART_DiscoverGPT(&gptSector, apBlockIo, apMedia, apRetPartCount, appRetPartArray);
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

    if ((NULL != pMedia->mpCurrentPartArray) ||
        (0 != pMedia->mCurrentPartCount))
        return K2STAT_ERROR_ALREADY_EXISTS;

    if ((apBlockDev->BlockIo.mBlockSizeInBytes != pMedia->mBytesPerSector) ||
        (0 == pMedia->mTotalSectorsCount))
        return K2STAT_ERROR_BAD_ARGUMENT;

    return K2STOR_PART_Discover(&apBlockDev->BlockIo, pMedia, &pMedia->mCurrentPartCount, &pMedia->mpCurrentPartArray);
}

