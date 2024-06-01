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

K2STAT 
K2FAT_DetermineFromBootSector(
    FAT_GENERIC_BOOTSECTOR const *  apBootSector,
    K2FAT_PART *                    apRetPart
)
{
    if (0 == K2MEM_Compare(apBootSector->BS_OEMName, "EXFAT", 5))
        return K2STAT_ERROR_UNSUPPORTED;

    if ((0 == apBootSector->BPB_BytesPerSec) || (0 == apBootSector->BPB_SecPerClus))
        return K2STAT_ERROR_BAD_FORMAT;

    K2MEM_Zero((UINT8 *)apRetPart, sizeof(K2FAT_PART));

    apRetPart->mBytesPerSector = apBootSector->BPB_BytesPerSec;

    if (apBootSector->BPB_FATSz16 != 0)
        apRetPart->mNumSectorsPerFAT = apBootSector->BPB_FATSz16;
    else
        apRetPart->mNumSectorsPerFAT = ((FAT_BOOTSECTOR32 const *)apBootSector)->BPB_FATSz32;

    if (apBootSector->BPB_TotSec16 != 0)
        apRetPart->mNumTotalSectors64 = apBootSector->BPB_TotSec16;
    else
        apRetPart->mNumTotalSectors64 = apBootSector->BPB_TotSec32;

    apRetPart->mFirstFATSector64 = apBootSector->BPB_ResvdSecCnt;

    apRetPart->mNumFATs = apBootSector->BPB_NumFATs;

    if (0 == apRetPart->mNumFATs)
    {
        return K2STAT_ERROR_BAD_FORMAT;
    }

    apRetPart->mMediaType = apBootSector->BPB_Media;
    apRetPart->mNumHeads = apBootSector->BPB_NumHeads;
    apRetPart->mSectorsPerTrack = apBootSector->BPB_SecPerTrk;
    apRetPart->mSectorsPerCluster = apBootSector->BPB_SecPerClus;

    /* these are fat12/fat16 calcs */
    apRetPart->mNumRootDirEntries = apBootSector->BPB_RootEntCnt;
    apRetPart->mNumRootDirSectors = ((apRetPart->mNumRootDirEntries * 32) + (apRetPart->mBytesPerSector - 1)) / apRetPart->mBytesPerSector;
    apRetPart->mFirstRootDirSector64 = apRetPart->mFirstFATSector64 + (apRetPart->mNumFATs * apRetPart->mNumSectorsPerFAT);
    apRetPart->mFirstDataSector64 = apRetPart->mFirstRootDirSector64 + apRetPart->mNumRootDirSectors;
    apRetPart->mNumDataSectors64 = apRetPart->mNumTotalSectors64 - apRetPart->mFirstDataSector64;
    apRetPart->mCountOfClusters = (UINT32)(apRetPart->mNumDataSectors64 / apRetPart->mSectorsPerCluster);

    /* only at this point can we determine if we are fat32, in which case we need to recalculate some of the
       above values to fit that model */

    if (apRetPart->mCountOfClusters >= 65525)
    {
        /* recalc layout */
        apRetPart->mFirstDataSector64 = apRetPart->mFirstFATSector64 + (apRetPart->mNumFATs * apRetPart->mNumSectorsPerFAT);
        apRetPart->mNumDataSectors64 = apRetPart->mNumTotalSectors64 - apRetPart->mFirstDataSector64;
        apRetPart->mCountOfClusters = (UINT32)(apRetPart->mNumDataSectors64 / apRetPart->mSectorsPerCluster);

        apRetPart->mNumRootDirEntries = 0;
        apRetPart->mNumRootDirSectors = 0;
        apRetPart->mFirstRootDirSector64 = FAT_CLUSTER_TO_SECTOR(apRetPart->mFirstDataSector64, apRetPart->mSectorsPerCluster, ((FAT_BOOTSECTOR32 *)apBootSector)->BPB_RootClus);

        apRetPart->mFATType = K2FAT_Type32; 
        apRetPart->mDriveNumber = ((FAT_BOOTSECTOR32 *)apBootSector)->BS_DrvNum;
    }
    else
    {
        if (apRetPart->mCountOfClusters < 4085)
        {
            apRetPart->mFATType = K2FAT_Type12;
            apRetPart->mDriveNumber = ((FAT_BOOTSECTOR12 *)apBootSector)->BS_DrvNum;
        }
        else
        {
            apRetPart->mFATType = K2FAT_Type16;
            apRetPart->mDriveNumber = ((FAT_BOOTSECTOR16 *)apBootSector)->BS_DrvNum;
        }
    }

    return K2STAT_NO_ERROR;
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
            if (FAT_DIRENTRY_IS_LONGNAME(pLong->mAttribute & 0x0F))
            {
                compare83 = 0;
                if (pLong->mMustBeZero != 0)
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

                if (0 == (pLong->mCaseSpec & FAT_DIRENTRY_CASE_NO83NAME))
                {
                    compare83 = 1;
                    if ((pEnt->mFileName[0] != FAT_DIRENTRY_NAME0_AVAIL) &&
                        (pEnt->mFileName[0] != FAT_DIRENTRY_NAME0_DOT) &&
                        (pEnt->mFileName[0] != FAT_DIRENTRY_NAME0_ERASED))
                    {
                        /* calc the checksum and check the long name stuffs */
                        chkSum = K2FAT_LFN_Checksum(pEnt->mFileName);
                        pLong++;
                        do {
                            pLong--;
                            if (pLong->m83NameChecksum != chkSum)
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
                    if (0 == (pLong->mAttribute & FAT_DIRENTRY_ATTR_LONGEND))
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
                        K2MEM_Copy(pConstruct, pFirst->mNameChars3, 2 * sizeof(UINT16));
                        pConstruct -= (6 * sizeof(UINT16));
                        K2MEM_Copy(pConstruct, pFirst->mNameChars2, 6 * sizeof(UINT16));
                        pConstruct -= (5 * sizeof(UINT16));
                        K2MEM_Copy(pConstruct, pFirst->mNameChars1, 5 * sizeof(UINT16));
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
            if (FAT_DIRENTRY_IS_LONGNAME(pEnt->mAttribute))
            {
                pLong = (FAT_LONGENTRY const *)pEnt;
                if (pLong->mMustBeZero == 0)
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
            if ((pEnt->mFileName[0] != FAT_DIRENTRY_NAME0_AVAIL) &&
                (pEnt->mFileName[0] != FAT_DIRENTRY_NAME0_DOT) &&
                (pEnt->mFileName[0] != FAT_DIRENTRY_NAME0_ERASED) &&
                (!(pEnt->mAttribute & (FAT_DIRENTRY_ATTR_LABEL | FAT_DIRENTRY_ATTR_DIR))))
            {
                /* remove any internal spaces in the 8.3 name before comparison */
                pConstruct = constructedName;
                entryLen = 0;
                for (chkIx = 0; chkIx < 8; chkIx++)
                {
                    if ((!pEnt->mFileName[chkIx]) || (pEnt->mFileName[chkIx] == ' '))
                        break;
                    *(pConstruct++) = pEnt->mFileName[chkIx];
                    entryLen++;
                }
                if ((pEnt->mExtension[0]) && (pEnt->mExtension[0] != ' '))
                {
                    *pConstruct = '.';
                    entryLen++;
                    pConstruct++;
                }
                *pConstruct = 0;
                for (chkIx = 0; chkIx < 3; chkIx++)
                {
                    if ((!pEnt->mExtension[chkIx]) || (pEnt->mExtension[chkIx] == ' '))
                        break;
                    *(pConstruct++) = pEnt->mExtension[chkIx];
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

    if ((apFatPart->mFATType == K2FAT_Invalid) ||
        (apFatPart->mFATType >= K2FAT_TypeCount))
    {
        return K2STAT_ERROR_BAD_ARGUMENT;
    }

    if ((aClusterNumber < 2) ||
        (aClusterNumber >= (apFatPart->mCountOfClusters + 2)))
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

