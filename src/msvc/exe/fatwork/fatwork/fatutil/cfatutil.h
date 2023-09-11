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
#ifndef __CFATUTIL_H
#define __CFATUTIL_H

#include <sysinc/cfat.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _FATUTIL_INFO
{
    UINT16 mFATFormat;
    UINT16 mMediaType;
    UINT16 mDriveNumber;
    UINT16 mNumHeads;

    UINT32 mBytesPerSector;
    UINT32 mSectorsPerTrack;

    UINT64 mNumTotalSectors64;

    UINT64 mFirstFATSector64;

    UINT32 mNumFATs;
    UINT32 mNumSectorsPerFAT;

    UINT64 mFirstRootDirSector64;

    UINT32 mNumRootDirEntries;
    UINT32 mNumRootDirSectors;

    UINT64 mFirstDataSector64;

    UINT64 mNumDataSectors64;

    UINT32 mCountOfClusters;

    UINT32 mSectorsPerCluster;
};
typedef struct _FATUTIL_INFO FATUTIL_INFO;

typedef 
errCode (*FATUTIL_pfSyncReadRaw)(
    TOKEN   aStorage
,   UINT64  aStartSector
,   UINT    aNumSectors
,   void *  apSectorBuffer
,   UINT    aSectorBufferBytes
,   UINT *  apRetSectorsRed
    );

errCode
FATUTIL_DetermineFATTypeFromBootSector(
    FAT_GENERIC_BOOTSECTOR const * apBootSector
,   FATUTIL_INFO *                 apRetInfo
);

errCode
FATUTIL_DetermineFATType(
    TOKEN                   aStorage
,   FATUTIL_pfSyncReadRaw   afSyncReadRaw
,   FATUTIL_INFO *          apRetInfo
);

UINT8 
FATUTIL_LongFileNameChecksum(
    UINT8 const * apDirEntry83Name
);

errCode
FATUTIL_FindDirEntry(
    UINT8 const *   apDirEntryBuffer
,   UINT            aNumDirEntries
,   char const *    apName
,   UINT            aNameLen
,   FAT_DIRENTRY *  apRetHeadEntry
);

errCode
FATUTIL_GetNextCluster(
    FATUTIL_INFO const *apFATInfo
,   UINT8 const *       apFATData
,   UINT32              aClusterNumber
,   UINT32 *            apRetNextCluster
);

#ifdef __cplusplus
};  // extern "C"
#endif

#define MAKE_FATUTIL_ERRCODE(x)         MAKE_ERRCODE(FACILITY_FATUTIL, x)

#define ERRCODE_FATUTIL_CORRUPTED       MAKE_FATUTIL_ERRCODE(0x001)
#define ERRCODE_FATUTIL_BADFAT          MAKE_FATUTIL_ERRCODE(0x002)

#endif  // __CFATUTIL_H

