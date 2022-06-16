#include <lib/fatutil/cfatutil.h>
#include <lib/memory/cmemory.h>

static
errCode
sNextFat12Cluster(
    UINT8 const *   apFATData
,   UINT32          aClusterNumber
,   UINT32 *        apRetNextCluster
)
{
    /* 3 nybbles per cluster. */
    UINT val = 3 * ((UINT)aClusterNumber);
    UINT8 const *pData = &apFATData[val/2];
    if (aClusterNumber&1)
        val = (UINT)((pData[0]>>4)&0x0F) | (((UINT)pData[1])<<4);
    else
        val = ((((UINT)pData[1])&0x0F)<<8) | ((UINT)pData[0]);
    if (FAT_CLUSTER12_IS_NEXT_PTR(val))
    {
        *apRetNextCluster = val;
        return 0;
    }
    *apRetNextCluster = 0;
    if (FAT_CLUSTER12_IS_CHAIN_END(val))
        return 0;
    if (FAT_CLUSTER12_IS_UNUSED(val) ||
        FAT_CLUSTER12_IS_RESERVED(val))
        return ERRCODE_FATUTIL_CORRUPTED;
    return ERRCODE_FATUTIL_BADFAT;
}

static
errCode
sNextFat16Cluster(
    UINT16 const *  apFATData
,   UINT32          aClusterNumber
,   UINT32 *        apRetNextCluster
)
{
    UINT16 clusterVal = apFATData[aClusterNumber];
    if (FAT_CLUSTER16_IS_NEXT_PTR(clusterVal))
    {
        *apRetNextCluster = clusterVal;
        return 0;
    }
    *apRetNextCluster = 0;
    if (FAT_CLUSTER16_IS_CHAIN_END(clusterVal))
        return 0;
    if (FAT_CLUSTER16_IS_UNUSED(clusterVal) ||
        FAT_CLUSTER16_IS_RESERVED(clusterVal))
        return ERRCODE_FATUTIL_CORRUPTED;
    return ERRCODE_FATUTIL_BADFAT;
}

static
errCode
sNextFat32Cluster(
    UINT32 const *  apFATData
,   UINT32          aClusterNumber
,   UINT32 *        apRetNextCluster
)
{
    UINT32 clusterVal = apFATData[aClusterNumber];
    if (FAT_CLUSTER32_IS_NEXT_PTR(clusterVal))
    {
        *apRetNextCluster = clusterVal;
        return 0;
    }
    *apRetNextCluster = 0;
    if (FAT_CLUSTER32_IS_CHAIN_END(clusterVal))
        return 0;
    if (FAT_CLUSTER32_IS_UNUSED(clusterVal) ||
        FAT_CLUSTER32_IS_RESERVED(clusterVal))
        return ERRCODE_FATUTIL_CORRUPTED;
    return ERRCODE_FATUTIL_BADFAT;
}

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
