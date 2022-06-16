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

