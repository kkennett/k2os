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

#include <lib/k2fat.h>

typedef struct _K2FAT_TOTSEC_TO_SEC_PER_CLUS K2FAT_TOTSEC_TO_SEC_PER_CLUS;
struct _K2FAT_TOTSEC_TO_SEC_PER_CLUS
{
    UINT32  mSectorCount;
    UINT8   mSectorsPerCluster;
};

static K2FAT_TOTSEC_TO_SEC_PER_CLUS ConvTableFAT16[] = {
    {8400, 0}, /* disks up to 4.1 MB, the 0 value for SecPerClusVal trips an error */
    {32680, 2}, /* disks up to 16 MB, 1k cluster */
    {262144, 4}, /* disks up to 128 MB, 2k cluster */
    {524288, 8}, /* disks up to 256 MB, 4k cluster */
    {1048576, 16}, /* disks up to 512 MB, 8k cluster */
    /* The entries after this point are not used unless FAT16 is forced */
    {2097152, 32}, /* disks up to 1 GB, 16k cluster */
    {4194304, 64}, /* disks up to 2 GB, 32k cluster */
    {0xFFFFFFFF, 0} /*any disk greater than 2GB, 0 value for SecPerClusVal trips an error */
};

static K2FAT_TOTSEC_TO_SEC_PER_CLUS ConvTableFAT32[] = {
    {66600, 0}, /* disks up to 32.5 MB, the 0 value for SecPerClusVal trips an error */
    {532480, 1}, /* disks up to 260 MB, .5k cluster */
    {16777216, 8}, /* disks up to 8 GB, 4k cluster */
    {33554432, 16}, /* disks up to 16 GB, 8k cluster */
    {67108864, 32}, /* disks up to 32 GB, 16k cluster */
    {0xFFFFFFFF, 64}/* disks greater than 32GB, 32k cluster */
};

K2STAT
K2FAT_CreateBootSector(
    UINT32                  aBytesPerSector,
    UINT32                  aMediaType,
    UINT32                  aSectorCount,
    UINT32                  aFatDateTime,
    FAT_GENERIC_BOOTSECTOR *apRetBootSector
)
{
    UINT64      totSizeBytes;
    K2FAT_Type  fatType;
    UINT_PTR    ix;
    UINT_PTR    clusterCount;
    UINT64      clusterCount64;
    UINT_PTR    rootDirSectors;
    UINT_PTR    fatSectors;
    UINT_PTR    resSectors;

    if ((NULL == apRetBootSector) ||
        (0 == aSectorCount))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    // these are the only allowed FAT bytes per sector values 
    if ((512 != aBytesPerSector) &&
        (1024 != aBytesPerSector) &&
        (2048 != aBytesPerSector) &&
        (4096 != aBytesPerSector))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    K2MEM_Zero(apRetBootSector, aBytesPerSector);

    apRetBootSector->BS_jmpBoot[0] = 0xEB;
    apRetBootSector->BS_jmpBoot[1] = 0x3C;
    apRetBootSector->BS_jmpBoot[2] = 0x90;
    K2ASC_Copy((char *)apRetBootSector->BS_OEMName, "K2OS");
    apRetBootSector->BPB_Media = (UINT8)aMediaType;
    K2MEM_WriteAsBytes_UINT16(&apRetBootSector->BS_Signature, 0xAA55);

    K2MEM_WriteAsBytes_UINT16(&apRetBootSector->BPB_BytesPerSec, (UINT16)aBytesPerSector);

    totSizeBytes = ((UINT64)aSectorCount) * ((UINT64)aBytesPerSector);

    if (aMediaType == FAT_MEDIATYPE_FLOPPY)
    {
        if (aBytesPerSector != 512)
        {
            return K2STAT_ERROR_NOT_SUPPORTED;
        }
        if (aSectorCount >= 0xFF5)
        {
            //
            // floppies must be FAT12, and FAT12 cannot
            // handle >= 4MB
            //
            return K2STAT_ERROR_TOO_BIG;
        }
        fatType = K2FAT_Type12;
        clusterCount = aSectorCount;
        apRetBootSector->BPB_SecPerClus = 1;
        K2MEM_WriteAsBytes_UINT16(&apRetBootSector->BPB_SecPerTrk, 18);
        K2MEM_WriteAsBytes_UINT16(&apRetBootSector->BPB_NumHeads, 2);
    }
    else
    {
        //
        // not a floppy disk media
        //
        if (512 == aBytesPerSector)
        {
            if (aSectorCount < 0xFF0)
            {
                fatType = K2FAT_Type12;
                apRetBootSector->BPB_SecPerClus = 1;
            }
            else
            {
                //
                // 512-byte sector media uses fixed tables to determine sectors per cluster
                //
                ix = 0;
                if (totSizeBytes < (512 * 1024 * 1024))
                {
                    fatType = K2FAT_Type16;
                    do
                    {
                        if (aSectorCount <= ((UINT64)ConvTableFAT16[ix].mSectorCount))
                        {
                            apRetBootSector->BPB_SecPerClus = ConvTableFAT16[ix].mSectorsPerCluster;
                            break;
                        }
                        ix++;
                    } while (1);
                }
                else
                {
                    // FAT32 - these two settings are forced. check above prevents overrun of table
                    fatType = K2FAT_Type32;
                    do
                    {
                        if (aSectorCount <= ((UINT64)ConvTableFAT32[ix].mSectorCount))
                        {
                            apRetBootSector->BPB_SecPerClus = ConvTableFAT32[ix].mSectorsPerCluster;
                            break;
                        }
                        ix++;
                    } while (1);
                }
            }
        }
        else
        {
            //
            // not a 512-byte sector media
            //
            clusterCount64 = totSizeBytes / (UINT64)aBytesPerSector;
            if (clusterCount64 < 0xFF5)
            {
                fatType = K2FAT_Type12;
                apRetBootSector->BPB_SecPerClus = 1;
            }
            else if (clusterCount64 < 65525ull)
            {
                fatType = K2FAT_Type16;
                apRetBootSector->BPB_SecPerClus = 1;
            }
            else
            {
                fatType = K2FAT_Type32;
                ix = 1;
                while (clusterCount64 >= 0x0FFFFFF5)
                {
                    ix++;
                    clusterCount64 >>= 1;
                }
                if (ix > 7)
                {
                    // volume is too big for FAT32 so clip it to the max based on the sector size and max cluster size
                    totSizeBytes = 0x0FFFFFF5ull * 128ull * ((UINT64)aBytesPerSector);
                    aSectorCount = (UINT32)(totSizeBytes / ((UINT64)aBytesPerSector));
                    ix = 7;
                }
                apRetBootSector->BPB_SecPerClus = (UINT8)(1 << ix);
            }
        }
    }

    //
    // fat type and sectors per cluster are set now
    // 
    // calculate number of fats and sectors per fat
    //

    clusterCount = (UINT32)(totSizeBytes / (((UINT64)aBytesPerSector) / ((UINT64)apRetBootSector->BPB_SecPerClus)));

    if (K2FAT_Type12 == fatType)
    {
        apRetBootSector->BPB_NumFATs = 2;
        K2MEM_WriteAsBytes_UINT16(&apRetBootSector->BPB_RootEntCnt, 224);
        fatSectors = (((clusterCount * 3) / 2) + (aBytesPerSector - 1)) / aBytesPerSector;
        resSectors = 1;
    }
    else
    {
        apRetBootSector->BPB_NumFATs = 2;
        if (K2FAT_Type16 == fatType)
        {
            fatSectors = ((clusterCount * 2) + (aBytesPerSector - 1)) / aBytesPerSector;
            K2MEM_WriteAsBytes_UINT16(&apRetBootSector->BPB_RootEntCnt, 512);
            resSectors = 1;
        }
        else
        {
            K2_ASSERT(K2FAT_Type32 == fatType);
            fatSectors = ((clusterCount * 4) + (aBytesPerSector - 1)) / aBytesPerSector;
            resSectors = 32;
        }
    }

    rootDirSectors = ((K2MEM_ReadAsBytes_UINT16(&apRetBootSector->BPB_RootEntCnt) * 32) + (aBytesPerSector - 1)) / aBytesPerSector;

    //
    // align data area using number of reserved sectors, which will always be > 1
    // and always be at least 32 for a FAT32 volume
    //
    ix = apRetBootSector->BPB_SecPerClus;
    while (0 != ((rootDirSectors + (fatSectors * apRetBootSector->BPB_NumFATs) + resSectors) % ix))
        resSectors++;

    K2MEM_WriteAsBytes_UINT16(&apRetBootSector->BPB_ResvdSecCnt, resSectors);

    if (K2FAT_Type32 != fatType)
    {
        if (FAT_MEDIATYPE_FLOPPY != aMediaType)
        {
            ((FAT_BOOTSECTOR12OR16 *)apRetBootSector)->BS_DrvNum = 0x80;
        }

        K2ASC_Copy((char *)((FAT_BOOTSECTOR12OR16 *)apRetBootSector)->BS_VolLab, "NO NAME    ");

        ((FAT_BOOTSECTOR12OR16 *)apRetBootSector)->BS_BootSig = 0x29;

        if (K2FAT_Type12 == fatType)
        {
            K2ASC_Copy((char *)((FAT_BOOTSECTOR12OR16 *)apRetBootSector)->BS_FilSysType, "FAT12   ");
        }
        else
        {
            K2ASC_Copy((char *)((FAT_BOOTSECTOR12OR16 *)apRetBootSector)->BS_FilSysType, "FAT16   ");
        }

        K2MEM_WriteAsBytes_UINT32(&((FAT_BOOTSECTOR12OR16 *)apRetBootSector)->BS_VolID, aFatDateTime);

        K2MEM_WriteAsBytes_UINT16(&apRetBootSector->BPB_FATSz16, (UINT16)fatSectors);
        if (aSectorCount > 0xFFFF)
        {
            K2MEM_WriteAsBytes_UINT32(&apRetBootSector->BPB_TotSec32, aSectorCount);
        }
        else
        {
            K2MEM_WriteAsBytes_UINT16(&apRetBootSector->BPB_TotSec16, (UINT16)aSectorCount);
        }
    }
    else
    {
        K2MEM_WriteAsBytes_UINT32(&((FAT_BOOTSECTOR32 *)apRetBootSector)->BPB_RootClus, 2);
        K2MEM_WriteAsBytes_UINT32(&((FAT_BOOTSECTOR32 *)apRetBootSector)->BS_VolID, aFatDateTime);
        ((FAT_BOOTSECTOR32 *)apRetBootSector)->BS_BootSig = 0x29;
        K2ASC_Copy((char *)((FAT_BOOTSECTOR32 *)apRetBootSector)->BS_VolLab, "NO NAME    ");
        K2ASC_Copy((char *)((FAT_BOOTSECTOR32 *)apRetBootSector)->BS_FilSysType, "FAT32   ");
        K2MEM_WriteAsBytes_UINT32(&((FAT_BOOTSECTOR32 *)apRetBootSector)->BPB_FATSz32, fatSectors);
        K2MEM_WriteAsBytes_UINT32(&apRetBootSector->BPB_TotSec32, aSectorCount);
    }

    return K2STAT_NO_ERROR;
}

K2STAT 
K2FAT_ParseBootSector(
    FAT_GENERIC_BOOTSECTOR const *  apBootSector,
    K2FAT_PART *                    apRetPart
)
{
    UINT16 bytesPerSector;

    if (0 == K2MEM_Compare(apBootSector->BS_OEMName, "EXFAT", 5))
        return K2STAT_ERROR_UNSUPPORTED;

    bytesPerSector = K2MEM_ReadAsBytes_UINT16(&apBootSector->BPB_BytesPerSec);

    if ((0 == bytesPerSector) || (0 == apBootSector->BPB_SecPerClus))
        return K2STAT_ERROR_BAD_FORMAT;

    K2MEM_Zero((UINT8 *)apRetPart, sizeof(K2FAT_PART));

    apRetPart->mBytesPerSector = bytesPerSector;

    if (apBootSector->BPB_FATSz16 != 0)
        apRetPart->mNumSectorsPerFAT = K2MEM_ReadAsBytes_UINT16(&apBootSector->BPB_FATSz16);
    else
        apRetPart->mNumSectorsPerFAT = K2MEM_ReadAsBytes_UINT32(&((FAT_BOOTSECTOR32 const *)apBootSector)->BPB_FATSz32);

    if (apBootSector->BPB_TotSec16 != 0)
        apRetPart->mNumTotalSectors = K2MEM_ReadAsBytes_UINT16(&apBootSector->BPB_TotSec16);
    else
        apRetPart->mNumTotalSectors = K2MEM_ReadAsBytes_UINT32(&apBootSector->BPB_TotSec32);

    apRetPart->mNumReservedSectors = K2MEM_ReadAsBytes_UINT16(&apBootSector->BPB_ResvdSecCnt);

    apRetPart->mNumFATs = apBootSector->BPB_NumFATs;

    if (0 == apRetPart->mNumFATs)
    {
        return K2STAT_ERROR_BAD_FORMAT;
    }

    apRetPart->mMediaType = apBootSector->BPB_Media;
    apRetPart->mNumHeads = K2MEM_ReadAsBytes_UINT16(&apBootSector->BPB_NumHeads);
    apRetPart->mSectorsPerTrack = K2MEM_ReadAsBytes_UINT16(&apBootSector->BPB_SecPerTrk);
    apRetPart->mSectorsPerCluster = apBootSector->BPB_SecPerClus;

    /* these are fat12/fat16 calcs */
    apRetPart->mNumRootDirEntries = K2MEM_ReadAsBytes_UINT16(&apBootSector->BPB_RootEntCnt);
    apRetPart->mNumRootDirSectors = ((apRetPart->mNumRootDirEntries * 32) + (apRetPart->mBytesPerSector - 1)) / apRetPart->mBytesPerSector;
    apRetPart->mFirstRootDirSector = apRetPart->mNumReservedSectors + (apRetPart->mNumFATs * apRetPart->mNumSectorsPerFAT);
    apRetPart->mDataAreaStartSector = apRetPart->mFirstRootDirSector + apRetPart->mNumRootDirSectors;
    apRetPart->mNumDataSectors = apRetPart->mNumTotalSectors - apRetPart->mDataAreaStartSector;
    apRetPart->mLastValidClusterIndex = (UINT32)(apRetPart->mNumDataSectors / apRetPart->mSectorsPerCluster) + 2;

    /* only at this point can we determine if we are fat32, in which case we need to recalculate some of the
       above values to fit that model */

    if (apRetPart->mLastValidClusterIndex >= 65525)
    {
        /* recalc layout */
        apRetPart->mDataAreaStartSector = apRetPart->mNumReservedSectors + (apRetPart->mNumFATs * apRetPart->mNumSectorsPerFAT);
        apRetPart->mNumDataSectors = apRetPart->mNumTotalSectors - apRetPart->mDataAreaStartSector;
        apRetPart->mLastValidClusterIndex = (UINT32)(apRetPart->mNumDataSectors / apRetPart->mSectorsPerCluster) + 2;

        apRetPart->mNumRootDirEntries = 0;
        apRetPart->mNumRootDirSectors = 0;
        apRetPart->mFirstRootDirSector = FAT_CLUSTER_TO_SECTOR(apRetPart->mDataAreaStartSector, apRetPart->mSectorsPerCluster, K2MEM_ReadAsBytes_UINT32(&((FAT_BOOTSECTOR32 *)apBootSector)->BPB_RootClus));

        apRetPart->mFATType = K2FAT_Type32; 
        apRetPart->mDriveNumber = ((FAT_BOOTSECTOR32 *)apBootSector)->BS_DrvNum;
    }
    else
    {
        if ((apRetPart->mLastValidClusterIndex < 0xFF5) && 
            (0 == K2ASC_CompLen(((FAT_BOOTSECTOR12OR16 const *)apBootSector)->BS_FilSysType,"FAT12 ",6)))
        {
            apRetPart->mFATType = K2FAT_Type12;
            apRetPart->mDriveNumber = ((FAT_BOOTSECTOR12OR16 *)apBootSector)->BS_DrvNum;
        }
        else
        {
            apRetPart->mFATType = K2FAT_Type16;
            apRetPart->mDriveNumber = ((FAT_BOOTSECTOR12OR16 *)apBootSector)->BS_DrvNum;
        }
    }

    return K2STAT_NO_ERROR;
}

UINT16
K2FAT_MakeDate(
    UINT_PTR aYear,
    UINT_PTR aMonth,
    UINT_PTR aDay
)
{
    UINT16 ret;

    ret = (((UINT16)(aYear - 1980)) << FAT_DATE_YEAR_SHL) & FAT_DATE_YEAR_MASK;
    ret |= (((UINT16)aMonth) << FAT_DATE_MONTH_SHL) & FAT_DATE_MONTH_MASK;
    return ret | ((UINT16)aDay) & FAT_DATE_DOM_MASK;
}

UINT16
K2FAT_MakeTime(
    UINT_PTR aHour,
    UINT_PTR aMinute,
    UINT_PTR aSecond
)
{
    UINT16 ret;

    ret = (((UINT16)aHour) << FAT_TIME_HOUR_SHL) & FAT_TIME_HOUR_MASK;
    ret |= (((UINT16)aMinute) << FAT_TIME_MINUTE_SHL) & FAT_TIME_MINUTE_MASK;
    return ret | ((((UINT16)aSecond) / 2) & FAT_TIME_SEC2);
}

UINT32
K2FAT_OsTimeToDateTime(
    K2_DATETIME const * apOsTime
)
{
    UINT32 ret;

    if ((NULL == apOsTime) ||
        (!K2_IsOsTimeValid(apOsTime)))
        return 0;

    // time low date high
    ret = K2FAT_MakeTime(apOsTime->mHour, apOsTime->mMinute, apOsTime->mSecond);
    return ret | ((UINT32)K2FAT_MakeDate(apOsTime->mYear, apOsTime->mMonth, apOsTime->mDay)) << 16;
}

BOOL
K2FAT_DateTimeToOsTime(
    UINT32          aFatDateTime,
    K2_DATETIME *   apRetOsTime
)
{
    if (NULL == apRetOsTime)
        return FALSE;

    // time low date high.  always extract even if is bad

    apRetOsTime->mYear = 1980 + (UINT16)(((aFatDateTime >> 16) & FAT_DATE_YEAR_MASK) >> FAT_DATE_YEAR_SHL);
    apRetOsTime->mMonth = (UINT16)(((aFatDateTime >> 16) & FAT_DATE_MONTH_MASK) >> FAT_DATE_MONTH_SHL);
    apRetOsTime->mDay = (UINT16)((aFatDateTime >> 16) & FAT_DATE_DOM_MASK);
    apRetOsTime->mHour = (UINT16)((aFatDateTime & FAT_TIME_HOUR_MASK) >> FAT_TIME_HOUR_SHL);
    apRetOsTime->mMinute = (UINT16)((aFatDateTime & FAT_TIME_MINUTE_MASK) >> FAT_TIME_MINUTE_SHL);
    apRetOsTime->mSecond = (UINT16)(((aFatDateTime & FAT_TIME_SEC2)) * 2);
    apRetOsTime->mMillisecond = 0;
    apRetOsTime->mTimeZoneId = 0;

    // user may not care
    return K2_IsOsTimeValid(apRetOsTime);
}

UINT8  
K2FAT_LFN_Checksum(
    UINT8 const * apDirEntry83Name
)
{
    UINT_PTR    i;
    UINT8       sum;
    
    sum = 0;
    for (i = 11; i > 0; i--)
    {
        sum = ((sum & 1) << 7) + (sum >> 1) + *apDirEntry83Name;
        apDirEntry83Name++;
    }
    return sum;
}

K2STAT 
K2FAT_FindDirEntry(
    UINT8 const *   apDirEntryBuffer,
    UINT_PTR        aNumDirEntries,
    char const *    apName,
    UINT_PTR        aNameLen,
    FAT_DIRENTRY *  apRetEntry
)
{
    FAT_DIRENTRY const *pEnt;
    FAT_LONGENTRY const *pFirst;
    FAT_LONGENTRY const *pLong;
    UINT_PTR inLongName;
    UINT_PTR compare83;
    UINT_PTR chkIx;
    UINT_PTR entryLen;
    UINT8 chkSum;
    char constructedName[256 * sizeof(UINT16)];
    char *pConstruct;
    UINT16 *pConstruct16;

    if ((NULL == apDirEntryBuffer) ||
        (NULL == apName) ||
        (0 == (*apName)) ||
        (NULL == apRetEntry))
        return K2STAT_ERROR_BAD_ARGUMENT;

    K2MEM_Zero(apRetEntry, sizeof(FAT_DIRENTRY));

    pEnt = (FAT_DIRENTRY const *)apDirEntryBuffer;
    pFirst = NULL;
    inLongName = 0;
    compare83 = 0;
    do {
        if (inLongName)
        {
            pLong = (FAT_LONGENTRY const *)pEnt;
            if (FAT_DIRENTRY_IS_LONGNAME(pLong->LDIR_Attr & 0x0F))
            {
                compare83 = 0;
                if (pLong->LDIR_FstClusLO != 0)
                {
                    /* error in chain */
                    pFirst = NULL;
                    inLongName = 0;
                }
            }
            else
            {
                /* pEnt should point to 8.3 name for long name */
                inLongName = 0;
                pLong--;

                /* pLong should be last (first) entry in the long name now */

                if (0 == (pLong->LDIR_Type & FAT_DIRENTRY_CASE_NO83NAME))
                {
                    compare83 = 1;
                    if ((pEnt->DIR_Name[0] != FAT_DIRENTRY_NAME0_AVAIL) &&
                        (pEnt->DIR_Name[0] != FAT_DIRENTRY_NAME0_DOT) &&
                        (pEnt->DIR_Name[0] != FAT_DIRENTRY_NAME0_ERASED))
                    {
                        /* calc the checksum and check the long name stuffs */
                        chkSum = K2FAT_LFN_Checksum(pEnt->DIR_Name);
                        pLong++;
                        do {
                            pLong--;
                            if (pLong->LDIR_Chksum != chkSum)
                            {
                                pLong = NULL;
                                break;
                            }
                        } while (pLong != pFirst);
                    }
                    else
                    {
                        pLong = NULL;
                    }
                }
                else
                {
                    compare83 = 0;
                    if (0 == (pLong->LDIR_Attr & FAT_DIRENTRY_ATTR_LONGEND))
                    {
                        //
                        // long file name with no 8.3 does not have longend bit set
                        // so this is a corrupted entry
                        //
                        pLong = NULL;
                    }
                }

                if (pLong)
                {
                    entryLen = 0;
                    /* all entries in long name have the right checksum */
                    /* reconstruct the long name */
                    pConstruct = &constructedName[255];
                    *pConstruct = 0;
                    pConstruct--;
                    *pConstruct = 0;
                    do {
                        if (entryLen > aNameLen)
                            break;
                        pConstruct -= (2 * sizeof(UINT16));
                        K2MEM_Copy(pConstruct, pFirst->LDIR_Name3, 2 * sizeof(UINT16));
                        pConstruct -= (6 * sizeof(UINT16));
                        K2MEM_Copy(pConstruct, pFirst->LDIR_Name2, 6 * sizeof(UINT16));
                        pConstruct -= (5 * sizeof(UINT16));
                        K2MEM_Copy(pConstruct, pFirst->LDIR_Name1, 5 * sizeof(UINT16));
                        entryLen += 13;
                        pFirst++;
                    } while (pFirst != ((FAT_LONGENTRY const *)pEnt));

                    /* convert the unicode to ascii */
                    pConstruct16 = (UINT16 *)pConstruct;
                    pConstruct = constructedName;
                    entryLen = 0;
                    do {
                        *pConstruct = (UINT8)(*pConstruct16);
                        pConstruct++;
                        pConstruct16++;
                        entryLen++;
                    } while (*pConstruct16);
                    *pConstruct = 0;

                    /* now compare to the input name */
                    if (entryLen == aNameLen)
                    {
                        if (0 == K2ASC_CompInsLen(constructedName, apName, entryLen))
                            break;
                    }
                }
                else
                {
                    /* error in chain */
                    pFirst = NULL;
                    inLongName = 0;
                    compare83 = 0;
                }
            }
        }
        else
        {
            if (FAT_DIRENTRY_IS_LONGNAME(pEnt->DIR_Attr))
            {
                pLong = (FAT_LONGENTRY const *)pEnt;
                if (pLong->LDIR_FstClusLO == 0)
                {
                    pFirst = (FAT_LONGENTRY const *)pEnt;
                    inLongName = 1;
                }
                compare83 = 0;
            }
            else
            {
                /* this is a normal 8.3 filename (not a long name) */
                compare83 = 1;
            }
        }

        if (compare83)
        {
            compare83 = 0;
            if ((pEnt->DIR_Name[0] != FAT_DIRENTRY_NAME0_AVAIL) &&
                (pEnt->DIR_Name[0] != FAT_DIRENTRY_NAME0_DOT) &&
                (pEnt->DIR_Name[0] != FAT_DIRENTRY_NAME0_ERASED) &&
                (!(pEnt->DIR_Attr & (FAT_DIRENTRY_ATTR_LABEL | FAT_DIRENTRY_ATTR_DIR))))
            {
                /* remove any internal spaces in the 8.3 name before comparison */
                pConstruct = constructedName;
                entryLen = 0;
                for (chkIx = 0; chkIx < 8; chkIx++)
                {
                    if ((!pEnt->DIR_Name[chkIx]) || (pEnt->DIR_Name[chkIx] == ' '))
                        break;
                    *(pConstruct++) = pEnt->DIR_Name[chkIx];
                    entryLen++;
                }
                if ((pEnt->DIR_Name[8]) && (pEnt->DIR_Name[8] != ' '))
                {
                    *pConstruct = '.';
                    entryLen++;
                    pConstruct++;
                }
                *pConstruct = 0;
                for (chkIx = 0; chkIx < 3; chkIx++)
                {
                    if ((!pEnt->DIR_Name[8 + chkIx]) || (pEnt->DIR_Name[8 + chkIx] == ' '))
                        break;
                    *(pConstruct++) = pEnt->DIR_Name[8 + chkIx];
                    entryLen++;
                }
                *pConstruct = 0;

                if (entryLen == aNameLen)
                {
                    if (!K2ASC_CompInsLen(constructedName, apName, entryLen))
                        break;
                }
            }
        }

        pEnt++;

    } while (--aNumDirEntries);

    if (0 == aNumDirEntries)
        return K2STAT_ERROR_NO_MORE_ITEMS;

    K2MEM_Copy(apRetEntry, pEnt, sizeof(FAT_DIRENTRY));

    return K2STAT_NO_ERROR;
}

K2STAT
K2FAT_NextFat12Cluster(
    UINT8 const *   apFATData,
    UINT_PTR        aClusterNumber,
    UINT_PTR *      apRetNextCluster
)
{
    /* 3 nybbles per cluster. */
    UINT_PTR val;
    UINT8 const *pData;

    val = 3 * aClusterNumber;
    pData = &apFATData[val / 2];

    if (aClusterNumber & 1)
    {
        val = (UINT_PTR)((pData[0] >> 4) & 0x0F) | (((UINT_PTR)pData[1]) << 4);
    }
    else
    {
        val = ((((UINT_PTR)pData[1]) & 0x0F) << 8) | ((UINT_PTR)pData[0]);
    }

    if (FAT_CLUSTER12_IS_NEXT_PTR(val))
    {
        *apRetNextCluster = val;
        return K2STAT_NO_ERROR;
    }

    *apRetNextCluster = 0;

    if (FAT_CLUSTER12_IS_CHAIN_END(val))
    {
        return K2STAT_NO_ERROR;
    }

    if (FAT_CLUSTER12_IS_UNUSED(val) ||
        FAT_CLUSTER12_IS_RESERVED(val))
    {
        return K2STAT_ERROR_CORRUPTED;
    }

    return K2STAT_ERROR_BAD_FORMAT;
}

K2STAT
K2FAT_NextFat16Cluster(
    UINT16 const *  apFATData,
    UINT_PTR        aClusterNumber,
    UINT_PTR *      apRetNextCluster
)
{
    UINT16 clusterVal;

    clusterVal = apFATData[aClusterNumber];

    if (FAT_CLUSTER16_IS_NEXT_PTR(clusterVal))
    {
        *apRetNextCluster = clusterVal;
        return K2STAT_NO_ERROR;
    }

    *apRetNextCluster = 0;

    if (FAT_CLUSTER16_IS_CHAIN_END(clusterVal))
    {
        return K2STAT_NO_ERROR;
    }

    if (FAT_CLUSTER16_IS_UNUSED(clusterVal) ||
        FAT_CLUSTER16_IS_RESERVED(clusterVal))
    {
        return K2STAT_ERROR_CORRUPTED;
    }

    return K2STAT_ERROR_BAD_FORMAT;
}

K2STAT
K2FAT_NextFat32Cluster(
    UINT32 const *  apFATData,
    UINT_PTR        aClusterNumber,
    UINT_PTR *      apRetNextCluster
)
{
    UINT32 clusterVal;

    clusterVal = apFATData[aClusterNumber];

    if (FAT_CLUSTER32_IS_NEXT_PTR(clusterVal))
    {
        *apRetNextCluster = clusterVal;
        return K2STAT_NO_ERROR;
    }

    *apRetNextCluster = 0;

    if (FAT_CLUSTER32_IS_CHAIN_END(clusterVal))
    {
        return K2STAT_NO_ERROR;
    }

    if (FAT_CLUSTER32_IS_UNUSED(clusterVal) ||
        FAT_CLUSTER32_IS_RESERVED(clusterVal))
    {
        return K2STAT_ERROR_CORRUPTED;
    }

    return K2STAT_ERROR_BAD_FORMAT;
}

K2STAT 
K2FAT_GetNextCluster(
    K2FAT_PART const  * apFatPart,
    UINT8 const *       apFatData,
    UINT_PTR            aClusterNumber,
    UINT_PTR *          apRetNextCluster
)
{
    if ((NULL == apFatPart) ||
        (NULL == apFatData) ||
        (0 == apRetNextCluster))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((apFatPart->mFATType == K2FAT_Type_Invalid) ||
        (apFatPart->mFATType >= K2FAT_Type_Count))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if ((aClusterNumber < 2) ||
        (aClusterNumber > apFatPart->mLastValidClusterIndex))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if (apFatPart->mFATType == K2FAT_Type32)
    {
        return K2FAT_NextFat32Cluster((UINT32 const *)apFatData, aClusterNumber, apRetNextCluster);
    }

    if (apFatPart->mFATType == K2FAT_Type16)
    {
        return K2FAT_NextFat16Cluster((UINT16 const *)apFatData, aClusterNumber, apRetNextCluster);
    }

    return K2FAT_NextFat12Cluster(apFatData, aClusterNumber, apRetNextCluster);
}

