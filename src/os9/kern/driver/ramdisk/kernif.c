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

#include "ramdisk.h"
#include <spec/gpt.h>

#define RAMDISK_PAGES       (256 * 4)  // 4MB
#define RAMDISK_BLOCKSIZE   512
#define RAMDISK_BLOCKCOUNT  ((RAMDISK_PAGES * K2_VA_MEMPAGE_BYTES) / RAMDISK_BLOCKSIZE)

static const K2_GUID128 sgDiskGuid = K2OS_RAMDISK_GUID;
static K2_GUID128 const sgBasicPartGuid = GPT_BASIC_DATA_PART_GUID;

K2STAT 
RAMDISK_FatWrite(
    K2FATFS_OPS const * apOps,
    const UINT8 *       apBuffer,
    UINT32              aStartSector,
    UINT32              aSectorCount
)
{
    RAMDISK_DEVICE *    pDevice;
    UINT32              v;

    pDevice = K2_GET_CONTAINER(RAMDISK_DEVICE, apOps, FatOps);

    if (aStartSector >= (RAMDISK_BLOCKCOUNT - 5))
        return K2STAT_ERROR_OUT_OF_BOUNDS;

    if (((RAMDISK_BLOCKCOUNT - 5) - aStartSector) < aSectorCount)
        return K2STAT_ERROR_OUT_OF_BOUNDS;

    v = pDevice->mVirtBase + ((3 + aStartSector) * RAMDISK_BLOCKSIZE);

    K2MEM_Copy((void *)v, apBuffer, aSectorCount * RAMDISK_BLOCKSIZE);

    return K2STAT_NO_ERROR;
}

K2STAT 
RAMDISK_FatSync(
    K2FATFS_OPS const *apOps
)
{
    return K2STAT_NO_ERROR;
}

UINT32 
RAMDISK_FatTime(
    K2FATFS_OPS const *apOps
)
{
    return 0;
}

void * RAMDISK_FatAlloc(K2FATFS_OPS const *apOps, UINT32 aSizeBytes)
{
    return K2OS_Heap_Alloc(aSizeBytes);
}

void   RAMDISK_FatFree(K2FATFS_OPS const *apOps, void * aPtr)
{
    K2OS_Heap_Free(aPtr);
}

K2STAT
RAMDISK_PartitionAndFormat(
    RAMDISK_DEVICE * apDevice
)
{
    GPT_HEADER *            pHdr;
    GPT_HEADER *            pHdr2;
    GPT_ENTRY *             pParts;
    GPT_ENTRY *             pParts2;
    K2FATFS                 fatFs;
    K2_STORAGE_VOLUME       storVol;
    K2STAT                  stat;
    K2FATFS_FORMAT_PARAM    formatParam;

    pHdr = (GPT_HEADER *)(apDevice->mVirtBase + RAMDISK_BLOCKSIZE);
    K2MEM_Zero(pHdr, RAMDISK_BLOCKSIZE);

    K2ASC_Copy((char *)&pHdr->Signature, "EFI PART");
    pHdr->Revision = 0x00010000;
    pHdr->HeaderSize = sizeof(GPT_HEADER);
    pHdr->MyLBA = 1;
    pHdr->AlternateLBA = RAMDISK_BLOCKCOUNT - 1;
    pHdr->FirstUsableLBA = 3;
    pHdr->LastUsableLBA = RAMDISK_BLOCKCOUNT - 3;
    K2MEM_Copy(&pHdr->DiskGuid, &sgDiskGuid, sizeof(K2_GUID128));
    pHdr->PartitionEntryLBA = 2;
    pHdr->NumberOfPartitionEntries = RAMDISK_BLOCKSIZE / sizeof(GPT_ENTRY);
    pHdr->SizeOfPartitionEntry = sizeof(GPT_ENTRY);

    pParts = (GPT_ENTRY *)(apDevice->mVirtBase + (RAMDISK_BLOCKSIZE * 2));

    K2MEM_Zero(pParts, RAMDISK_BLOCKSIZE);
    K2MEM_Copy(&pParts->PartitionTypeGuid, &sgBasicPartGuid, sizeof(K2_GUID128));
    K2MEM_Copy(&pParts->UniquePartitionGuid, &sgDiskGuid, sizeof(K2_GUID128));
    pParts->StartingLBA = pHdr->FirstUsableLBA;
    pParts->EndingLBA = pHdr->LastUsableLBA;
    K2MEM_Copy(&pParts->UnicodePartitionName, L"RAMDISK", 8 * sizeof(UINT16));

    pHdr->PartitionEntryArrayCRC32 = K2CRC_Calc32(0, pParts, RAMDISK_BLOCKSIZE);

    pHdr->HeaderCRC32 = K2CRC_Calc32(0, pHdr, pHdr->HeaderSize);

    pHdr2 = (GPT_HEADER *)(apDevice->mVirtBase + (((UINT32)pHdr->AlternateLBA) * RAMDISK_BLOCKSIZE));
    K2MEM_Copy(pHdr2, pHdr, sizeof(GPT_HEADER));
    pHdr2->HeaderCRC32 = 0;
    pHdr2->MyLBA = pHdr->AlternateLBA;
    pHdr2->AlternateLBA = pHdr->MyLBA;
    pHdr2->PartitionEntryLBA = pHdr2->MyLBA - 1;

    pParts2 = (GPT_ENTRY *)(((UINT32)pHdr2) - RAMDISK_BLOCKSIZE);
    K2MEM_Copy(pParts2, pParts, RAMDISK_BLOCKSIZE);

    pHdr2->HeaderCRC32 = K2CRC_Calc32(0, pHdr2, pHdr2->HeaderSize);

    K2MEM_Zero(&storVol, sizeof(storVol));
    storVol.mBlockCount = (UINT32)(pHdr->LastUsableLBA - pHdr->FirstUsableLBA + 1);
    storVol.mBlockSizeBytes = RAMDISK_BLOCKSIZE;
    storVol.mPartitionCount = 1;
    storVol.mTotalBytes = ((UINT64)storVol.mBlockCount) * ((UINT64)storVol.mBlockSizeBytes);
    K2MEM_Copy(&storVol.mUniqueId, &sgDiskGuid, sizeof(K2_GUID128));

    apDevice->FatOps.MemAlloc = RAMDISK_FatAlloc;
    apDevice->FatOps.MemFree = RAMDISK_FatFree;
    apDevice->FatOps.DiskWrite = RAMDISK_FatWrite;
    apDevice->FatOps.DiskSync = RAMDISK_FatSync;
    apDevice->FatOps.GetFatTime = RAMDISK_FatTime;

    K2MEM_Zero(&fatFs, sizeof(fatFs));
    K2FATFS_init(&fatFs, &apDevice->FatOps, &storVol);

    K2MEM_Zero(&formatParam, sizeof(formatParam));
    formatParam.mFatCount = 2;
    formatParam.mFormatType = K2FATFS_FORMAT_ANY;
    formatParam.mRootDirEntCount = 64;
    formatParam.mSectorsPerCluster = 8;

    stat = K2FATFS_formatvolume(&fatFs, &formatParam, NULL, formatParam.mSectorsPerCluster * RAMDISK_BLOCKSIZE);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));

    return K2STAT_NO_ERROR;
}

K2STAT
RAMDISK_GetMedia(
    RAMDISK_DEVICE *        apDevice,
    K2OS_STORAGE_MEDIA *    apRetMedia
)
{
    apRetMedia->mBlockCount = RAMDISK_BLOCKCOUNT;
    apRetMedia->mBlockSizeBytes = RAMDISK_BLOCKSIZE;
    K2ASC_Copy(apRetMedia->mFriendly, "RAMDISK");
    apRetMedia->mTotalBytes = ((UINT64)apRetMedia->mBlockCount) * ((UINT64)RAMDISK_BLOCKSIZE);
    K2MEM_Copy(&apRetMedia->mUniqueId, &sgDiskGuid, sizeof(K2_GUID128));
    return K2STAT_NO_ERROR;
}

K2STAT
RAMDISK_Transfer(
    RAMDISK_DEVICE *                apDevice,
    K2OS_BLOCKIO_TRANSFER const *   apTransfer
)
{
    UINT8 * pBlock;

    pBlock = (UINT8 *)(apDevice->mVirtBase + (((UINT32)apTransfer->mStartBlock) * RAMDISK_BLOCKSIZE));

    if (!apTransfer->mIsWrite)
    {
        K2MEM_Copy((void *)apTransfer->mAddress, pBlock, apTransfer->mBlockCount * RAMDISK_BLOCKSIZE);
    }
    else
    {
        K2MEM_Copy(pBlock, (void *)apTransfer->mAddress, apTransfer->mBlockCount * RAMDISK_BLOCKSIZE);
    }

    K2_CpuFullBarrier();

    return K2STAT_NO_ERROR;
}

static K2OSDDK_BLOCKIO_REGISTER sgBlockIoFuncTab =
{
    { FALSE },  // does not use hardware addresses
    (K2OSDDK_pf_BlockIo_GetMedia)RAMDISK_GetMedia,
    (K2OSDDK_pf_BlockIo_Transfer)RAMDISK_Transfer
};

K2STAT
CreateInstance(
    K2OS_DEVCTX aDevCtx,
    void **     appRetDriverContext
)
{
    RAMDISK_DEVICE *    pDevice;
    K2STAT              stat;

    pDevice = (RAMDISK_DEVICE *)K2OS_Heap_Alloc(sizeof(RAMDISK_DEVICE));
    if (NULL == pDevice)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    K2MEM_Zero(pDevice, sizeof(RAMDISK_DEVICE));

    pDevice->mDevCtx = aDevCtx;
    *appRetDriverContext = pDevice;

    return K2STAT_NO_ERROR;
}

K2STAT
StartDriver(
    RAMDISK_DEVICE *    apDevice
)
{
    K2STAT                  stat;
    K2OS_PAGEARRAY_TOKEN    tokPageArray;

    apDevice->mPageCount = RAMDISK_PAGES;

    tokPageArray = K2OS_PageArray_Create(apDevice->mPageCount);
    if (NULL == tokPageArray)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        return stat;
    }

    apDevice->mVirtBase = K2OS_Virt_Reserve(apDevice->mPageCount);
    if (0 == apDevice->mVirtBase)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OS_Token_Destroy(tokPageArray);
        return stat;
    }

    apDevice->mTokVirtMap = K2OS_VirtMap_Create(
        tokPageArray, 
        0, 
        apDevice->mPageCount, 
        apDevice->mVirtBase, 
        K2OS_MapType_Data_ReadWrite
    );

    K2OS_Token_Destroy(tokPageArray);

    if (NULL == apDevice->mTokVirtMap)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OS_Virt_Release(apDevice->mVirtBase);
        apDevice->mVirtBase = 0;
    }

    stat = RAMDISK_PartitionAndFormat(apDevice);
    if (!K2STAT_IS_ERROR(stat))
    {
        stat = K2OSDDK_DriverStarted(apDevice->mDevCtx);
        if (!K2STAT_IS_ERROR(stat))
        {
            stat = K2OSDDK_BlockIoRegister(apDevice->mDevCtx, apDevice, &sgBlockIoFuncTab, &apDevice->mpNotifyKey);

            if (!K2STAT_IS_ERROR(stat))
            {
                stat = K2OSDDK_SetEnable(apDevice->mDevCtx, TRUE);

                if (K2STAT_IS_ERROR(stat))
                {
                    K2OSDDK_BlockIoDeregister(apDevice->mDevCtx, apDevice);
                }
            }

            if (K2STAT_IS_ERROR(stat))
            {
                K2OSDDK_DriverStopped(apDevice->mDevCtx, stat);
            }
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        K2OS_Token_Destroy(apDevice->mTokVirtMap);
        apDevice->mTokVirtMap = NULL;

        K2OS_Virt_Release(apDevice->mVirtBase);
        apDevice->mVirtBase = 0;
    }

    return stat;
}

K2STAT
StopDriver(
    RAMDISK_DEVICE *    apDevice
)
{
    K2OSDDK_BlockIoDeregister(apDevice->mDevCtx, apDevice);

    K2OSDDK_DriverStopped(apDevice->mDevCtx, K2STAT_NO_ERROR);

    return K2STAT_NO_ERROR;
}

K2STAT
DeleteInstance(
    RAMDISK_DEVICE *    apDevice
)
{
    K2OS_Token_Destroy(apDevice->mTokVirtMap);
    apDevice->mTokVirtMap = NULL;

    K2OS_Virt_Release(apDevice->mVirtBase);
    apDevice->mVirtBase = 0;

    K2OS_Heap_Free(apDevice);

    return K2STAT_NO_ERROR;
}

