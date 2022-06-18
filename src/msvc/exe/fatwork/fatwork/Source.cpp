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
#include <lib/k2stor.h>
#include <lib/k2list.h>
#include <lib/k2tree.h>
#include <lib/k2fat.h>
#include <VirtDisk.h>

#pragma warning(disable: 4477)

typedef UINT_PTR K2OS_MAILSLOT;


typedef UINT_PTR K2OS_DEVICE_USAGE;
typedef UINT_PTR K2OS_DEVICE_IO;

#define K2OS_DEVICE_PARAM_ACCESS            1
#define K2OS_DEVICE_PARAM_SHARE             2
#define K2OS_DEVICE_PARAM_IO_BLOCK_SIZE     3
#define K2OS_DEVICE_PARAM_MEDIA_STATE       4
#define K2OS_DEVICE_PARAM_MEDIA_PROP        5

K2OS_DEVICE_USAGE   K2OS_Device_Acquire(char const *apDevicePath);
K2STAT              K2OS_Device_SetConfig(K2OS_DEVICE_USAGE aDeviceUsage, UINT_PTR aParameter, UINT_PTR aConfigBytes, void const *apConfigData);
K2STAT              K2OS_Device_GetConfig(K2OS_DEVICE_USAGE aDeviceUsage, UINT_PTR aParameter, UINT_PTR aBufferBytes, void *apRetConfigData);
K2OS_DEVICE_IO      K2OS_Device_OpenIo(K2OS_DEVICE_USAGE aDeviceUsage, K2OS_MAILSLOT aMailslot);
K2STAT              K2OS_Device_Release(K2OS_DEVICE_USAGE aDeviceUsage);

typedef struct _K2OS_IOPROP K2OS_IOPROP;
struct _K2OS_IOPROP
{
    UINT64          mDeviceOffset;
    UINT_PTR        mBufferAddr;
    UINT_PTR        mBytesIo;
};

typedef struct _K2OS_IOREC K2OS_IOREC;
struct _K2OS_IOREC
{
    K2OS_IOPROP     Param;
    UINT_PTR        mUserContext;
};

K2STAT  K2OS_DeviceIo_Sync(K2OS_DEVICE_IO aDeviceIo, BOOL aIsWrite, K2OS_IOPROP *apIoProp);
BOOL    K2OS_DeviceIo_StartAsync(K2OS_DEVICE_IO aDeviceIo, BOOL aIsWrite, K2OS_IOREC *apIoRec);
BOOL    K2OS_DeviceIo_GetAsyncStatus(K2OS_IOREC *apIoRec, K2STAT *apRetStatus);
BOOL    K2OS_DeviceIo_CancelAsync(K2OS_IOREC *apIoRec);
BOOL    K2OS_DeviceIo_CompleteAsync(K2OS_IOREC *apIoRec, K2STAT *apRetStatus, UINT_PTR *apRetActualIoBytes);
K2STAT  K2OS_DeviceIo_Close(K2OS_DEVICE_IO aDeviceIo);



typedef struct _K2OS_ASYNCIO K2OS_ASYNCIO;

typedef void (*K2OS_IO_Key)(K2OS_ASYNCIO *apAsyncIo, K2STAT aResultStatus, UINT_PTR aBytesTransferred);

typedef enum _AsyncIoStateType AsyncIoStateType;
enum _AsyncIoStateType
{
    AsyncIoState_Invalid = 0,
    AsyncIoState_Submit,
    AsyncIoState_Pending,
    AsyncIoState_Abandoned,

    AsyncIoStateType_Count
};

struct _K2OS_ASYNCIO
{
    K2TREE_NODE         TreeNode;               // mUserVal is pointer to K2OS_IOREC
    AsyncIoStateType    mState;
    K2STAT              mStatus;
    UINT_PTR            mBytesTransferred;
    K2OS_IO_Key         mKey;
};







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
K2STOR_PART_DiscoverGPT(
    GPT_SECTOR const *      apSector1,
    K2STOR_BLOCKIO const *  apBlockIo,
    K2STOR_MEDIA const *    apMedia,
    UINT_PTR *              apRetPartCount,
    K2STOR_PART **          appRetPartArray
)
{
    static K2_GUID128 const sBasicPartGuid = GPT_BASIC_DATA_PART_GUID;

    K2STAT          stat;
    UINT_PTR        bufBytes;
    UINT8 *         pBlockBuffer;
    UINT_PTR        align;
    GPT_SECTOR *    pAltSector;
    UINT64          blockIx;
    UINT_PTR        partitionTableSize;
    UINT8 *         pTableBuffer1;
    UINT8 *         pPartTab1;
    UINT8 *         pTableBuffer2;
    UINT8 *         pPartTab2;
    UINT_PTR        partCount;
    K2STOR_PART *   pPart;
    GPT_ENTRY       entry;

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
        pAltSector = (GPT_SECTOR *)align;

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
            stat = apBlockIo->Transfer(apBlockIo, &blockIx, partitionTableSize, FALSE, align);
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
                stat = apBlockIo->Transfer(apBlockIo, &blockIx, partitionTableSize, FALSE, align);
                if (K2STAT_IS_ERROR(stat))
                    break;

                stat = K2STOR_PART_ValidateGPTPartitions(apSector1, pAltSector, apMedia, pPartTab1, pPartTab2, &partCount);
                if (K2STAT_IS_ERROR(stat))
                    break;

                //
                // there are no malformed partitions
                //
                *apRetPartCount = partCount;

                if (0 != partCount)
                {
                    pPart = (K2STOR_PART *)malloc(sizeof(K2STOR_PART) * partCount);
                    if (NULL == pPart)
                    {
                        stat = K2STAT_ERROR_OUT_OF_MEMORY;
                        break;
                    }

                    //
                    // cannot fail from here on
                    //

                    *appRetPartArray = pPart;
                    pPartTab2 = pPartTab1;
                    bufBytes = 0;
                    for (align = 0; align < apSector1->Header.NumberOfPartitionEntries; align++)
                    {
                        K2MEM_Copy(&entry, pPartTab2, sizeof(GPT_ENTRY));
                        pPartTab2 += apSector1->Header.SizeOfPartitionEntry;

                        if (!K2MEM_VerifyZero(&entry.PartitionTypeGuid, sizeof(K2_GUID128)))
                        {
                            if ((entry.StartingLBA < apMedia->mTotalSectorsCount) &&
                                (entry.EndingLBA < apMedia->mTotalSectorsCount) &&
                                (entry.EndingLBA >= entry.StartingLBA))
                            {
                                pPart->mPartTableEntryIx = align;
                                K2MEM_Copy(&pPart->mPartTypeGuid, &entry.PartitionTypeGuid, sizeof(K2_GUID128));
                                K2MEM_Copy(&pPart->mPartIdGuid, &entry.UniquePartitionGuid, sizeof(K2_GUID128));
                                pPart->mAttributes = entry.Attributes;
                                pPart->mMediaStartSectorOffset = entry.StartingLBA;
                                pPart->mMediaSectorsCount = entry.EndingLBA - entry.StartingLBA + 1;
                                if (0 == K2MEM_Compare(&entry.PartitionTypeGuid, &sBasicPartGuid, sizeof(K2_GUID128)))
                                {
                                    pPart->mFlagReadOnly = ((entry.Attributes & GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY) != 0) ? 0xFF : 0;
                                }
                                pPart->mFlagActive = ((entry.Attributes & GPT_ATTRIBUTE_LEGACY_BIOS_BOOTABLE) != 0) ? 0xFF : 0;
                                pPart->mFlagEFI = 0xFF;
                                pPart++;
                                if (++bufBytes == partCount)
                                    break;
                            }
                        }
                    }
                }

            } while (0);

            free(pTableBuffer2);

        } while (0);

        free(pTableBuffer1);

    } while (0);

    free(pBlockBuffer);

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
    UINT8           diskSectorBuffer[K2STOR_SECTOR_BYTES * 2];
    UINT_PTR        align;
    GPT_SECTOR *    pGptSector;
    UINT64          blockIx;
    K2STAT          stat;

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
    pGptSector = (GPT_SECTOR *)align;

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
    K2STOR_BLOCKDEV   BlockDev;
};

K2STAT
vhdTransfer(
    K2STOR_BLOCKIO const *  apBlockIo,
    UINT64 const *          apBlockIx,
    UINT_PTR                aBlockCount,
    BOOL                    aIsWrite,
    UINT_PTR                aBufferAddr
)
{
    VHD *           pVhd;
    K2STOR_MEDIA *  pMedia;

    if ((NULL == apBlockIo) ||
        (vhdTransfer != apBlockIo->Transfer) ||
        (0 == apBlockIo->mBlockSizeInBytes) ||
        (0 == apBlockIo->mTransferAlignBytes) ||
        (aBlockCount > apBlockIo->mMaxBlocksOneTransfer))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if (0 != (aBufferAddr % apBlockIo->mTransferAlignBytes))
        return K2STAT_ERROR_BAD_ALIGNMENT;

    if (0 == aBlockCount)
        return K2STAT_NO_ERROR;

    pVhd = K2_GET_CONTAINER(VHD, apBlockIo, BlockDev.BlockIo);
    pMedia = pVhd->BlockDev.mpCurrentMedia;
    if (NULL == pMedia)
        return K2STAT_ERROR_NO_MEDIA;

    if (pVhd->BlockDev.BlockIo.mBlockSizeInBytes != pMedia->mBytesPerSector)
        return K2STAT_ERROR_CORRUPTED;

    LARGE_INTEGER li;
    li.QuadPart = pMedia->mBytesPerSector;
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
        ok = WriteFile(pVhd->mHandle, (void *)aBufferAddr, apBlockIo->mBlockSizeInBytes * aBlockCount, &bytesIo, &overlapped);
    }
    else
    {
        ok = ReadFile(pVhd->mHandle, (void *)aBufferAddr, apBlockIo->mBlockSizeInBytes * aBlockCount, &bytesIo, &overlapped);
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

    gVHD.BlockDev.BlockIo.Transfer = vhdTransfer;
    gVHD.BlockDev.BlockIo.mBlockSizeInBytes = info.Size.SectorSize;
    gVHD.BlockDev.BlockIo.mMaxBlocksOneTransfer = (UINT_PTR)-1;
    gVHD.BlockDev.BlockIo.mTransferAlignBytes = 1;
    gVHD.BlockDev.BlockIo.mIsReadOnlyDevice = FALSE;

    K2STOR_MEDIA media;
    K2MEM_Zero(&media, sizeof(media));

    _snprintf_s((char *)&media.mSerialNumber, 31, 17, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
        vhdFooter.mUniqueId[0], vhdFooter.mUniqueId[1], vhdFooter.mUniqueId[2], vhdFooter.mUniqueId[3],
        vhdFooter.mUniqueId[4], vhdFooter.mUniqueId[5], vhdFooter.mUniqueId[6], vhdFooter.mUniqueId[7],
        vhdFooter.mUniqueId[8], vhdFooter.mUniqueId[9], vhdFooter.mUniqueId[10], vhdFooter.mUniqueId[11],
        vhdFooter.mUniqueId[12], vhdFooter.mUniqueId[13], vhdFooter.mUniqueId[14], vhdFooter.mUniqueId[15]);
    media.mNumSectorsPerTrack = vhdFooter.mDiskGeo_NumSectorsPerTrack;
    media.mNumCylinders = vhdFooter.mDiskGeo_NumCyl;
    media.mNumHeads = vhdFooter.mDiskGeo_NumHeads;
    media.mBytesPerSector = (UINT16)info.Size.SectorSize;
    media.mTotalSectorsCount = info.Size.VirtualSize / info.Size.SectorSize;



    //
    // mount the media
    //
    gVHD.BlockDev.mpCurrentMedia = &media;

    //
    // discover the partitions on the media
    //
    K2STAT stat;
    stat = K2STOR_BLOCKDEV_DiscoverMediaPartitions(&gVHD.BlockDev);
    if (K2STAT_IS_ERROR(stat))
    {
        return -1;
    }

    //
    // find the first FAT partition
    //
    UINT64 blockIx;
    FAT_GENERIC_BOOTSECTOR bootSector;
    UINT_PTR partIx;
    for (partIx = 0; partIx < gVHD.BlockDev.mCurrentPartCount; partIx++)
    {
        blockIx = 0;
        stat = K2STOR_PART_Transfer(&gVHD.BlockDev, partIx, &blockIx, 1, FALSE, (UINT_PTR)&bootSector);
        if (K2STAT_IS_ERROR(stat))
        {
            return -1;
        }
        if ((bootSector.BS_Signature == 0xAA55) &&
            (bootSector.BPB_BytesPerSec == gVHD.BlockDev.BlockIo.mBlockSizeInBytes))
        {
            break;
        }
    }

    if (partIx == gVHD.BlockDev.mCurrentPartCount)
        return -1;

    //
    // partIx has the signature.  try to determine what type of FAT this is
    //
    K2FAT_PART part;;
    stat = K2FAT_DetermineFromBootSector(&bootSector, &part);
    if (K2STAT_IS_ERROR(stat))
    {
        return -1;
    }

    //
    // load the first FAT from the disk
    //
    blockIx = part.mFirstFATSector64;
    sizeUsed = (part.mNumSectorsPerFAT * part.mBytesPerSector) + gVHD.BlockDev.BlockIo.mTransferAlignBytes;
    UINT_PTR bufAddr = (UINT_PTR)malloc(sizeUsed);
    if (0 == bufAddr)
    {
        return -2;
    }

    bufAddr = ((bufAddr + (gVHD.BlockDev.BlockIo.mTransferAlignBytes - 1)) / gVHD.BlockDev.BlockIo.mTransferAlignBytes) * gVHD.BlockDev.BlockIo.mTransferAlignBytes;

    UINT8 *pFat = (UINT8 *)bufAddr;

    stat = K2STOR_PART_Transfer(&gVHD.BlockDev, partIx, &blockIx, part.mNumSectorsPerFAT, FALSE, bufAddr);
    if (K2STAT_IS_ERROR(stat))
    {
        return -3;
    }

    //
    // load the root directory from the disk
    //
    if (part.mFATType == K2FAT_Type32)
    {
        //
        // scan cluster chain to determine size of root directory
        //
        UINT_PTR clusterNumber;

        part.mNumRootDirSectors = 0;

        clusterNumber = ((FAT_BOOTSECTOR32 *)&bootSector)->BPB_RootClus;
        do {
            part.mNumRootDirSectors += part.mSectorsPerCluster;
            stat = K2FAT_GetNextCluster(&part, pFat, clusterNumber, &clusterNumber);
            if (K2STAT_IS_ERROR(stat))
            {
                return -4;
            }
        } while (0 != clusterNumber);
    }

    sizeUsed = (part.mNumRootDirSectors * part.mBytesPerSector) + gVHD.BlockDev.BlockIo.mTransferAlignBytes;

    bufAddr = (UINT_PTR)malloc(sizeUsed);
    if (0 == bufAddr)
    {
        return -5;
    }

    bufAddr = ((bufAddr + (gVHD.BlockDev.BlockIo.mTransferAlignBytes - 1)) / gVHD.BlockDev.BlockIo.mTransferAlignBytes) * gVHD.BlockDev.BlockIo.mTransferAlignBytes;

    FAT_DIRENTRY *pRootDir = (FAT_DIRENTRY *)bufAddr;

    if (part.mFATType == K2FAT_Type32)
    {
        UINT_PTR clusterNumber;

        clusterNumber = ((FAT_BOOTSECTOR32 *)&bootSector)->BPB_RootClus;
        do {

            blockIx = FAT_CLUSTER_TO_SECTOR(part.mFirstDataSector64, part.mSectorsPerCluster, clusterNumber);

            stat = K2STOR_PART_Transfer(&gVHD.BlockDev, partIx, &blockIx, part.mSectorsPerCluster, FALSE, bufAddr);
            if (K2STAT_IS_ERROR(stat))
            {
                return -6;
            }

            part.mNumRootDirEntries += (part.mBytesPerSector * part.mSectorsPerCluster) / sizeof(FAT_DIRENTRY);
            bufAddr += part.mBytesPerSector * part.mSectorsPerCluster;

            stat = K2FAT_GetNextCluster(&part, pFat, clusterNumber, &clusterNumber);
            if (K2STAT_IS_ERROR(stat))
            {
                return -5;
            }
        } while (0 != clusterNumber);
    }
    else
    {
        blockIx = part.mFirstRootDirSector64;
        stat = K2STOR_PART_Transfer(&gVHD.BlockDev, partIx, &blockIx, part.mNumRootDirSectors, FALSE, bufAddr);
        if (K2STAT_IS_ERROR(stat))
        {
            return -6;
        }
    }

    FAT_DIRENTRY dirEntry;
    stat = K2FAT_FindDirEntry((UINT8 *)pRootDir, part.mNumRootDirEntries, "configcmd.txt", 13, &dirEntry);
    if (K2STAT_IS_ERROR(stat))
    {
        return -7;
    }
    
    //
    // load the file into memory
    //
    sizeUsed = ((dirEntry.mBytesLength + (part.mBytesPerSector - 1)) / part.mBytesPerSector) * part.mBytesPerSector;
    sizeUsed = sizeUsed + gVHD.BlockDev.BlockIo.mTransferAlignBytes;

    bufAddr = (UINT_PTR)malloc(sizeUsed);
    if (0 == bufAddr)
        return -7;

    bufAddr = ((bufAddr + (gVHD.BlockDev.BlockIo.mTransferAlignBytes - 1)) / gVHD.BlockDev.BlockIo.mTransferAlignBytes) * gVHD.BlockDev.BlockIo.mTransferAlignBytes;

    char *pFileData = (char *)bufAddr;

    UINT_PTR clusterNumber;

    clusterNumber = dirEntry.mStartClusterLow;
    if (dirEntry.mEAorHighCluster != 0xEA)
    {
        clusterNumber += (((UINT_PTR)dirEntry.mEAorHighCluster) << 16);
    }
    do {
        blockIx = FAT_CLUSTER_TO_SECTOR(part.mFirstDataSector64, part.mSectorsPerCluster, clusterNumber);

        stat = K2STOR_PART_Transfer(&gVHD.BlockDev, partIx, &blockIx, part.mSectorsPerCluster, FALSE, bufAddr);
        if (K2STAT_IS_ERROR(stat))
        {
            return -6;
        }

        bufAddr += part.mBytesPerSector * part.mSectorsPerCluster;

        stat = K2FAT_GetNextCluster(&part, pFat, clusterNumber, &clusterNumber);
        if (K2STAT_IS_ERROR(stat))
        {
            return -5;
        }
    } while (0 != clusterNumber);




    return 0;
}