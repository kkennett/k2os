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
        (0 == apBlockIo->mBlockSizeInBytes))
        return K2STAT_ERROR_BAD_ARGUMENT;

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
    media.mpCurrentMount = &gVHD.BlockDev;
 
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
    K2STOR_PART *pFatPart = gVHD.BlockDev.mpCurrentMedia->mpCurrentPartArray;
    for (partIx = 0; partIx < media.mCurrentPartCount; partIx++)
    {
        blockIx = 0;
        stat = K2STOR_PART_Transfer(pFatPart, &blockIx, 1, FALSE, (UINT_PTR)&bootSector);
        if (K2STAT_IS_ERROR(stat))
        {
            return -1;
        }
        if (bootSector.BS_Signature == 0xAA55)
        {
            break;
        }
        pFatPart++;
    }

    if (partIx == media.mCurrentPartCount)
        return -1;

    //
    // pFatPart has the signature.  try to determine what type of FAT this is
    //




    return 0;
}