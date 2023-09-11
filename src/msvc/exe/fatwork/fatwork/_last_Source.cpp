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
#define INITGUID
#include "fatwork.h"

StorMediaProp gVhdProp;

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
K2_STATIC_ASSERT(512 == sizeof(VHD_FOOTER));
VHD_FOOTER vhdFooter;

typedef struct _VHD VHD;
struct _VHD
{
    HANDLE      mHandle;
    VHD_FOOTER  VhdFooter;
};

VHD gVHD;

K2STAT
vhdTransfer(
    UINT64 const *  apBlockIx,
    UINT_PTR        aBlockCount,
    BOOL            aIsWrite,
    UINT_PTR        aBufferAddr
)
{
    if (0 == aBlockCount)
        return K2STAT_NO_ERROR;

    if ((aBufferAddr == 0) ||
        (0 != (aBufferAddr % gVhdProp.mBlockSizeBytes)))
    {
        return K2STAT_ERROR_BAD_ALIGNMENT;
    }

    LARGE_INTEGER li;
    li.QuadPart = gVhdProp.mBlockSizeBytes;
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
        ok = WriteFile(gVHD.mHandle, (void *)aBufferAddr, gVhdProp.mBlockSizeBytes * aBlockCount, &bytesIo, &overlapped);
    }
    else
    {
        ok = ReadFile(gVHD.mHandle, (void *)aBufferAddr, gVhdProp.mBlockSizeBytes * aBlockCount, &bytesIo, &overlapped);
    }
    if (!ok)
    {
        if (ERROR_IO_PENDING == GetLastError())
        {
            if (!GetOverlappedResult(gVHD.mHandle, &overlapped, &bytesIo, TRUE))
            {
                return (K2STAT)(HRESULT_FROM_WIN32(GetLastError()));
            }
        }
    }

    return K2STAT_NO_ERROR;
}

int main(int argc, char **argv)
{
//    static WCHAR const *spDiskFileName = L"C:\\users\\kurtkennett\\desktop\\Test.VHD";
    static WCHAR const *spDiskFileName = L"C:\\users\\kurt_\\desktop\\Test.VHD";

    ZeroMemory(&gVHD, sizeof(gVHD));

    HANDLE hFile;
    hFile = CreateFileW(spDiskFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (INVALID_HANDLE_VALUE == hFile)
        return -1;
    ULONG fileBytes = GetFileSize(hFile, NULL);
    if (INVALID_FILE_SIZE == fileBytes)
        return -2;
    if (0 != (fileBytes % 512))
        return -3;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, -512, NULL, FILE_END))
        return -4;
    DWORD bytesIo = 0;
    if (!ReadFile(hFile, &gVHD.VhdFooter, 512, &bytesIo, NULL))
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

    gVhdProp.mBlockSizeBytes = info.Size.SectorSize;
    gVhdProp.mBlockCount = (UINT32)(info.Size.VirtualSize / info.Size.SectorSize);
    K2ASC_Copy(gVhdProp.mFriendly, "VHD");
    gVhdProp.mUniqueId = K2CRC_Calc32(0, &vhdFooter.mUniqueId, 8);
    gVhdProp.mUniqueId |= ((UINT64)(K2CRC_Calc32(0, &vhdFooter.mUniqueId[8], 8))) << 32;
    gVhdProp.mTotalBytes = ((UINT64)gVhdProp.mBlockCount) * ((UINT64)gVhdProp.mBlockSizeBytes);

    return RunTest();
}
