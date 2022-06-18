#include <lib/fatutil/cfatutil.h>
#include <lib/memory/cmemory.h>


errCode
FATUTIL_GetNextCluster(
    FATUTIL_INFO const *apFATInfo
,   UINT8 const *       apFATData
,   UINT32              aClusterNumber
,   UINT32 *            apRetNextCluster
)
{
    if ((!apFATInfo) || (!apFATData) || (!apRetNextCluster))
        return ERRCODE_BADARG;
    if ((apFATInfo->mFATFormat < FAT_FSFORMAT_FAT12) ||
        (apFATInfo->mFATFormat > FAT_FSFORMAT_FAT32))
        return ERRCODE_INVALIDDATA;
    if ((aClusterNumber < 2) ||
        (aClusterNumber >= (apFATInfo->mCountOfClusters+2)))
        return ERRCODE_BADARG;

    switch (apFATInfo->mFATFormat)
    {
    case FAT_FSFORMAT_FAT12:
        return sNextFat12Cluster(apFATData,aClusterNumber,apRetNextCluster);
    case FAT_FSFORMAT_FAT16:
        return sNextFat16Cluster((UINT16 const *)apFATData,aClusterNumber,apRetNextCluster);
    case FAT_FSFORMAT_FAT32:
        return sNextFat32Cluster((UINT32 const *)apFATData,aClusterNumber,apRetNextCluster);
    }

    return ERRCODE_UNKNOWN;
}
