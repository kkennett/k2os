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
#ifndef __K2FAT_H
#define __K2FAT_H

#include <k2systype.h>
#include <lib/k2mem.h>
#include <lib/k2asc.h>
#include <spec/fat.h>

#ifdef __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------
//

typedef enum _K2FAT_Type K2FAT_Type;
enum _K2FAT_Type
{
    K2FAT_Invalid=0,
    K2FAT_Type12,
    K2FAT_Type16,
    K2FAT_Type32,
    K2FAT_TypeCount,
};

typedef struct _K2FAT_PART K2FAT_PART;
struct _K2FAT_PART
{
    UINT16 mFATType;    // K2FAT_Type above
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

K2STAT K2FAT_DetermineFromBootSector(FAT_GENERIC_BOOTSECTOR const * apBootSector, K2FAT_PART *apPart);

UINT8  K2FAT_LFN_Checksum(UINT8 const * apDirEntry83Name);

K2STAT K2FAT_FindDirEntry(UINT8 const * apDirEntryBuffer, UINT_PTR aNumDirEntries, char const *apName, UINT_PTR aNameLen, FAT_DIRENTRY *apRetEntry);

K2STAT K2FAT_GetNextCluster(K2FAT_PART const *apFatPart, UINT8 const *apFatData, UINT_PTR aClusterNumber, UINT_PTR *apRetNextCluster);

//
//------------------------------------------------------
//

#ifdef __cplusplus
}
#endif

#endif  // __K2FAT_H