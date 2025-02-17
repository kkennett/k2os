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

#include "fatfs.h"

K2STAT
FAT_Probe(
    K2OS_IFINST_ID      aVolIfInstId,
    K2FAT_Type          aMatchType,
    BOOL                aWantReadWrite,
    FATFS_OBJ_COMMON *  apFat
)
{
    K2STAT  stat;
    UINT64  volOffset;

    K2MEM_Zero(apFat, sizeof(FATFS_OBJ_COMMON));

    // attach read only
    apFat->mStorVol = K2OS_Vol_Attach(
        aVolIfInstId,
        K2OS_ACCESS_R,
        K2OS_ACCESS_RW,
        NULL);
    if (NULL == apFat->mStorVol)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OSKERN_Debug("FAT_Probe: Could not attach for readonly to volume with ifinstid %d\n", aVolIfInstId);
        return stat;
    }

    if (!K2OS_Vol_GetInfo(apFat->mStorVol, &apFat->VolInfo))
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OSKERN_Debug("FAT_Probe: Could not get info for volume %d\n", aVolIfInstId);
    }

    if (aWantReadWrite)
    {
        K2OS_Vol_Detach(apFat->mStorVol);

        if (0 == (apFat->VolInfo.mAttributes & K2_STORAGE_VOLUME_ATTRIB_READ_ONLY))
        {
            //
            // volume is not read only so we should be able to open RW
            //
            apFat->mStorVol = K2OS_Vol_Attach(
                aVolIfInstId,
                K2OS_ACCESS_RW,
                K2OS_ACCESS_R,
                NULL);
            if (NULL == apFat->mStorVol)
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                K2OSKERN_Debug("FAT_Probe: Could not attach R/W to volume with ifinstid %d\n", aVolIfInstId);
                return stat;
            }

            if (!K2OS_Vol_GetInfo(apFat->mStorVol, &apFat->VolInfo))
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                K2OSKERN_Debug("FAT_Probe: Could not get info for volume %d\n", aVolIfInstId);
                K2OS_Vol_Detach(apFat->mStorVol);
                return stat;
            }

            K2OSKERN_Debug("FAT_Probe: Probing %d with RW volume\n", aVolIfInstId);
        }
        else
        {
            // r/w required and volume is read only
            K2OSKERN_Debug("FAT_Probe: desired R/W with %d but volume is RO\n", aVolIfInstId);
            return K2STAT_ERROR_READ_ONLY;
        }
    }
    else
    {
        K2OSKERN_Debug("FAT_Probe: Probing %d with RO volume\n", aVolIfInstId);
    }


    //
    // opened with correct access and sharing, volume info retrieved
    //

    apFat->mVolIfInstId = aVolIfInstId;

    stat = K2STAT_ERROR_NOT_FOUND;

    do {
        if ((apFat->VolInfo.mBlockSizeBytes == 0) ||
            (apFat->VolInfo.mBlockCount == 0))
        {
            stat = K2STAT_ERROR_INVALID_MEDIA;
            K2OSKERN_Debug("FAT_Probe: volume %d has zero block size bytes or count\n", aVolIfInstId);
            break;
        }

        apFat->mpBootSecBuffer = (UINT8 *)K2OS_Heap_Alloc(apFat->VolInfo.mBlockSizeBytes * 2);
        if (NULL == apFat->mpBootSecBuffer)
        {
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            K2OSKERN_Debug("FAT_Probe: Out of memory allocating sector buffer for volume %d\n", aVolIfInstId);
            break;
        }

        do {
            apFat->mpBootSector = (UINT8 *)(((((UINT32)apFat->mpBootSecBuffer) + (apFat->VolInfo.mBlockSizeBytes - 1)) / apFat->VolInfo.mBlockSizeBytes) * apFat->VolInfo.mBlockSizeBytes);
            volOffset = 0;
            if (!K2OS_Vol_Read(apFat->mStorVol, &volOffset, apFat->mpBootSector, apFat->VolInfo.mBlockSizeBytes))
            {
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                K2OSKERN_Debug("FAT_Probe: Failed to read sector 0 of voluem %d, error %08X\n", aVolIfInstId, stat);
                break;
            }

            if ((apFat->mpBootSector[510] == 0x55) &&
                (apFat->mpBootSector[511] == 0xAA))
            {
                stat = K2FAT_ParseBootSector((FAT_GENERIC_BOOTSECTOR const *)apFat->mpBootSector, &apFat->FatPart);
                if (!K2STAT_IS_ERROR(stat))
                {
                    if (apFat->FatPart.mBytesPerSector != apFat->VolInfo.mBlockSizeBytes)
                    {
                        K2OSKERN_Debug(
                            "FAT_Probe: FAT on volume %d bytesPerSector(%d) does not match volume blockSizeBytes(%d)\n",
                            aVolIfInstId,
                            apFat->FatPart.mBytesPerSector,
                            apFat->VolInfo.mBlockSizeBytes
                        );
                        stat = K2STAT_ERROR_INVALID_MEDIA;
                    }
                    else
                    {
                        if (apFat->FatPart.mFATType == aMatchType)
                        {
                            K2OSKERN_Debug("FOUND FAT type %d\n", aMatchType);
                            stat = K2STAT_NO_ERROR;
                        }
                        else
                        {
                            K2OSKERN_Debug("Fat probed as %d but trying to match %d\n", apFat->FatPart.mFATType, aMatchType);
                            stat = K2STAT_ERROR_NOT_FOUND;
                        }
                    }
                }
                else
                {
                    K2OSKERN_Debug("*****Volume %d looks like FAT but determination failed %08X\n", aVolIfInstId, stat);
                }
            }

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            K2OS_Heap_Free(apFat->mpBootSecBuffer);
            apFat->mpBootSecBuffer = NULL;
        }

    } while (0);

    if ((K2STAT_IS_ERROR(stat)) &&
        (NULL != apFat->mStorVol))
    {
        K2OS_Vol_Detach(apFat->mStorVol);
        apFat->mStorVol = NULL;
    }

    return stat;
}

K2STAT
FATCom_Probe(
    void *      apContext,
    K2FAT_Type  aFatType,
    BOOL        aWantReadWrite,
    BOOL *      apRetMatched
)
{
    FATFS_OBJ_COMMON    common;
    K2STAT              stat;

    // apContext is K2OS_IFINST_ID of volume interface

    stat = FAT_Probe((K2OS_IFINST_ID)apContext, aFatType, aWantReadWrite, &common);
    if (!K2STAT_IS_ERROR(stat))
    {
        *apRetMatched = TRUE;
        K2OS_Heap_Free(common.mpBootSecBuffer);
        K2OS_Vol_Detach(common.mStorVol);
    }
    else
    {
        *apRetMatched = FALSE;
    }
    return stat;
}
