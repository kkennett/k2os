#include <lib/fatutil/cfatutil.h>
#include <lib/memory/cmemory.h>

errCode
FATUTIL_DetermineFATTypeFromBootSector(
    FAT_GENERIC_BOOTSECTOR const * apBootSector
,   FATUTIL_INFO *                 apRetInfo
)
{
    if (!MEM_Compare(apBootSector->BS_OEMName,"EXFAT",5))
        return ERRCODE_UNSUPPORTED;

    if ((!apBootSector->BPB_BytsPerSec) || (!apBootSector->BPB_SecPerClus))
        return ERRCODE_INVALIDDATA;

    MEM_Zero((UINT8 *)apRetInfo, sizeof(FATUTIL_INFO));

    apRetInfo->mBytesPerSector = apBootSector->BPB_BytsPerSec;

    if (apBootSector->BPB_FATSz16 != 0)
        apRetInfo->mNumSectorsPerFAT = apBootSector->BPB_FATSz16;
    else
        apRetInfo->mNumSectorsPerFAT = ((FAT_BOOTSECTOR32 const *)apBootSector)->BPB_FATSz32;

    if (apBootSector->BPB_TotSec16 != 0)
        apRetInfo->mNumTotalSectors64 = apBootSector->BPB_TotSec16;
    else
        apRetInfo->mNumTotalSectors64 = apBootSector->BPB_TotSec32;

    apRetInfo->mFirstFATSector64 = apBootSector->BPB_ResvdSecCnt;

    apRetInfo->mNumFATs = apBootSector->BPB_NumFATs;

    apRetInfo->mMediaType = apBootSector->BPB_Media;
    apRetInfo->mNumHeads = apBootSector->BPB_NumHeads;
    apRetInfo->mSectorsPerTrack = apBootSector->BPB_SecPerTrk;
    apRetInfo->mSectorsPerCluster = apBootSector->BPB_SecPerClus;

    /* these are fat12/fat16 calcs */
    apRetInfo->mNumRootDirEntries = apBootSector->BPB_RootEntCnt;
    apRetInfo->mNumRootDirSectors = ((apRetInfo->mNumRootDirEntries * 32) + (apRetInfo->mBytesPerSector - 1)) / apRetInfo->mBytesPerSector;
    apRetInfo->mFirstRootDirSector64 = apRetInfo->mFirstFATSector64 + (apRetInfo->mNumFATs * apRetInfo->mNumSectorsPerFAT);
    apRetInfo->mFirstDataSector64 = apRetInfo->mFirstRootDirSector64 + apRetInfo->mNumRootDirSectors;
    apRetInfo->mNumDataSectors64 = apRetInfo->mNumTotalSectors64 - apRetInfo->mFirstDataSector64;
    apRetInfo->mCountOfClusters = (UINT32)(apRetInfo->mNumDataSectors64 / apRetInfo->mSectorsPerCluster);

    /* only at this point can we determine if we are fat32, in which case we need to recalculate some of the
       above values to fit that model */

    if (apRetInfo->mCountOfClusters >= 65525)
    {
        /* recalc layout */
        apRetInfo->mFirstDataSector64 = apRetInfo->mFirstFATSector64 + (apRetInfo->mNumFATs * apRetInfo->mNumSectorsPerFAT);
        apRetInfo->mNumDataSectors64 = apRetInfo->mNumTotalSectors64 - apRetInfo->mFirstDataSector64;
        apRetInfo->mCountOfClusters = (UINT32)(apRetInfo->mNumDataSectors64 / apRetInfo->mSectorsPerCluster);

        apRetInfo->mNumRootDirEntries = 0;
        apRetInfo->mNumRootDirSectors = 0;
        apRetInfo->mFirstRootDirSector64 = FAT_CLUSTER_TO_SECTOR(apRetInfo->mFirstDataSector64, apRetInfo->mSectorsPerCluster, ((FAT_BOOTSECTOR32 *)apBootSector)->BPB_RootClus);

        apRetInfo->mFATFormat = FAT_FSFORMAT_FAT32;
        apRetInfo->mDriveNumber = ((FAT_BOOTSECTOR32 *)apBootSector)->BS_DrvNum;
    }
    else
    {
        if (apRetInfo->mCountOfClusters < 4085)
        {
            apRetInfo->mFATFormat = FAT_FSFORMAT_FAT12;
            apRetInfo->mDriveNumber = ((FAT_BOOTSECTOR12 *)apBootSector)->BS_DrvNum;
        }
        else
        {
            apRetInfo->mFATFormat = FAT_FSFORMAT_FAT16;
            apRetInfo->mDriveNumber = ((FAT_BOOTSECTOR16 *)apBootSector)->BS_DrvNum;
        }
    }

    return 0;
}

errCode
FATUTIL_DetermineFATType(
    TOKEN                   aDisk
,   FATUTIL_pfSyncReadRaw   afSyncReadRaw
,   FATUTIL_INFO *          apRetInfo
    )
{
    FAT_GENERIC_BOOTSECTOR bootSector;
    errCode err;
    UINT    red;

    if ((!afSyncReadRaw) || (!apRetInfo))
        return ERRCODE_BADARG;

    MEM_Zero(apRetInfo, sizeof(FATUTIL_INFO));

    err = afSyncReadRaw(aDisk, 0, 1, &bootSector, sizeof(bootSector), &red);
    if (err)
    {
        if ((!red) || (err!=ERRCODE_OUTOFBOUNDS))
            return err;
        /* otherwise we got at least 'bootsector' bytes of data read in - which is all we need */
    }
    else if (!red)
        return ERRCODE_UNKNOWN;

    return FATUTIL_DetermineFATTypeFromBootSector(&bootSector, apRetInfo);
}

