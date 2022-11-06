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


// SdCardPrep.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#define DISK_SECTOR_SIZE_POW2       9
#define DISK_SECTOR_BYTES           (1<<DISK_SECTOR_SIZE_POW2)

#define PARTITION_TABLE_START_LBA   128
#define PARTITION_TABLE_ENTRY_COUNT 16
#define PARTITION_TABLE_LBA_COUNT   4

#define DISK_HEADER_SIZE            (2 * 1024 * 1024)
#define EFI_FILE_SIZE               ((1024+512) * 1024)
#define EFI_SPARE_SIZE              (384 * 1024)
#define EFI_VARSTORE_SIZE           (128 * 1024)

typedef int checkHdrSize[(DISK_HEADER_SIZE > EFI_FILE_SIZE) ? 1 : -1];
typedef int checkAlign[((DISK_HEADER_SIZE % DISK_SECTOR_BYTES) == 0) ? 1 : -1];
typedef int checkFileAlign[((EFI_FILE_SIZE % DISK_SECTOR_BYTES) == 0) ? 1 : -1];
typedef int checkTotalSize[((EFI_FILE_SIZE + EFI_SPARE_SIZE + EFI_VARSTORE_SIZE) == DISK_HEADER_SIZE) ? 1 : -1];

#pragma pack(push, 1)
typedef struct _GPT_HEADER
{
    UINT8   Signature[8];       //  "EFI PART"
    UINT32  Revision;           //  0x00010000
    UINT32  HeaderSize;
    UINT32  HeaderCRC32;
    UINT32  Reserved;
    UINT64  MyLBA;
    UINT64  AlternateLBA;
    UINT64  FirstUsableLBA;
    UINT64  LastUsableLBA;
    GUID    DiskGuid;
    UINT64  PartitionEntryLBA;
    UINT32  NumberOfPartitionEntries;
    UINT32  SizeOfPartitionEntry;
    UINT32  PartitionEntryArrayCRC32;
} GPT_HEADER;
#pragma pack(pop)
typedef int check_GPT_HEADER[(sizeof(GPT_HEADER) == 92) ? 1 : -1];

typedef struct _GPT_SECTOR
{
    GPT_HEADER  Header;
    UINT8       Reserved[DISK_SECTOR_BYTES - 92];
} GPT_SECTOR;
typedef int check_GPT_SECTOR[(sizeof(GPT_SECTOR) == DISK_SECTOR_BYTES) ? 1 : -1];

#pragma pack(push, 1)
typedef struct _GPT_ENTRY
{
    GUID    PartitionTypeGuid;
    GUID    UniquePartitionGuid;
    UINT64  StartingLBA;
    UINT64  EndingLBA;
    UINT64  Attributes;
    WCHAR   PartitionName[36];
} GPT_ENTRY;
#pragma pack(pop)
typedef int check_GPT_ENTRY[(sizeof(GPT_ENTRY) == 128) ? 1 : -1];

struct DiskEntry
{
    VDS_OBJECT_ID   mDiskId;
    ULONGLONG       mBytes;
    ULONG           mPartType;
    bool            mIsRemovable;
    WCHAR           mFriendlyString[MAX_PATH];
    DiskEntry *     mpNext;
};

struct NotifyEntry
{
    NotifyEntry *       mpNext;
    LONG                mNum;
    VDS_NOTIFICATION    mEntries[1];  /* may be more than 1 */
};

class CSdCardPrepApp : public CWinApp
{
public:
	CSdCardPrepApp();

// Overrides
public:
	virtual BOOL InitInstance();
    bool FindProviders(void);

    bool FindDisk(DiskEntry *apEntry, IVdsDisk **appRetDisk, IVdsPack **appRetPack);

// Implementation

	DECLARE_MESSAGE_MAP()

public:
    /* available always */
    IVdsService *       mpServ;
    IVdsSwProvider *    mpBasic;
    
    /* dialog configuration fields */
    struct DiskEntry *      mpDisk;
    TCHAR *                 mpEFIPath;
    HANDLE                  mhEFIFile;
    UINT8 *                 mpEFIAlloc;
    UINT8 *                 mpEFIData;

    /* only available after 'ok' is pressed */
    bool                    mDiskPartOk;
    bool                    mForceClean;
    IVdsDisk *              mpVdsDisk;
    IVdsPack *              mpVdsPack;
    VDS_DISK_PROP           mDiskProp;
    VDS_OBJECT_ID           mNewVolumeId;
};

extern CSdCardPrepApp theApp;