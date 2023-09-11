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

#include "fatwork.h"

class MyPartition : public IStorPartition
{
public:
    MyPartition(IStorMediaSession *apSession, UINT_PTR aPartIx, UINT_PTR aBufIx, StorMediaProp const *apMediaProp) :
        mpSession(apSession),
        mMediaPartIx(aPartIx),
        mMediaPartBufIx(aBufIx),
        MediaProp(*apMediaProp)
    {
        K2MEM_Zero(&mPartTypeGuid, sizeof(mPartTypeGuid));
        K2MEM_Zero(&mPartIdGuid, sizeof(mPartIdGuid));
        mpSession->AddRef();
        mAttributes = 0;
        mStartBlock = 0;
        mBlockCount = 0;
        mIsReadOnly = FALSE;
        mFlagActive = FALSE;
        mFlagEFI = FALSE;
        mLegacyType = 0;
    }

    virtual ~MyPartition(void)
    {
        mpSession->Release();
    }

    UINT_PTR AddRef(void) { return K2ATOMIC_Inc((INT32 *)&mRefs); }
    UINT_PTR Release(void) { UINT_PTR result = K2ATOMIC_Dec((INT32 *)&mRefs); if (0 == result) delete this; return result; }

    BOOL AcquireMediaSession(IStorMediaSession **appRetSession)
    {
        if (NULL == appRetSession)
        {
            SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
            return FALSE;
        }
        mpSession->AddRef();
        *appRetSession = mpSession;
        return TRUE;
    }

    BOOL GetGuid(K2_GUID128 *apRetId)
    {
        if (NULL == apRetId)
        {
            SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
            return FALSE;
        }
        K2MEM_Copy(apRetId, &mPartIdGuid, sizeof(K2_GUID128));
        return TRUE;
    }

    BOOL GetTypeGuid(K2_GUID128 *apRetTypeId)
    {
        if (NULL == apRetTypeId)
        {
            SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
            return FALSE;
        }
        K2MEM_Copy(apRetTypeId, &mPartTypeGuid, sizeof(K2_GUID128));
        return TRUE;
    }

    UINT_PTR GetLegacyType(void)
    {
        return mLegacyType;
    }

    BOOL GetAttributes(UINT64 *apRetAttributes)
    {
        if (NULL == apRetAttributes)
        {
            SetLastError(K2STAT_ERROR_BAD_ARGUMENT);
            return FALSE;
        }
        *apRetAttributes = mAttributes;
        return TRUE;
    }

    UINT_PTR GetStartBlock(void)
    {
        return mStartBlock;
    }

    UINT_PTR GetBlockCount(void)
    {
        return mBlockCount;
    }

    void GetSizeBytes(UINT64 *apRetSizeBytes)
    {
        *apRetSizeBytes = ((UINT64)mBlockCount) * ((UINT64)MediaProp.mBlockSizeBytes);
    }

    UINT_PTR    ReadBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount, void *apAlignedBuffer);
    UINT_PTR    WriteBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount, void const *apAlignedBuffer, BOOL aWriteThrough);
    BOOL        FlushDirtyBlocks(UINT_PTR aStartBlock, UINT_PTR aBlockCount);

    UINT_PTR    mRefs;

    IStorMediaSession * const   mpSession;
    UINT_PTR const              mMediaPartIx;
    UINT_PTR const              mMediaPartBufIx;

    StorMediaProp               MediaProp;
    K2_GUID128                  mPartTypeGuid;
    K2_GUID128                  mPartIdGuid;
    UINT64                      mAttributes;
    UINT_PTR                    mStartBlock;
    UINT_PTR                    mBlockCount;
    UINT_PTR                    mLegacyType;
    BOOL                        mIsReadOnly;
    BOOL                        mFlagActive;
    BOOL                        mFlagEFI;
};

UINT_PTR    
MyPartition::ReadBlocks(
    UINT_PTR    aStartBlock,
    UINT_PTR    aBlockCount,
    void *      apAlignedBuffer
)
{
    return 0;
}

UINT_PTR    
MyPartition::WriteBlocks(
    UINT_PTR    aStartBlock,
    UINT_PTR    aBlockCount,
    void const *apAlignedBuffer,
    BOOL        aWriteThrough
)
{
    return 0;
}

BOOL        
MyPartition::FlushDirtyBlocks(
    UINT_PTR aStartBlock,
    UINT_PTR aBlockCount
)
{
    return FALSE;
}

int
RunParts(
    UINT_PTR        aPartCount,
    MyPartition **  appParts
)
{
    return -1;
}

int 
RunMbrPart(
    IStorMediaSession * apSession, 
    UINT_PTR            aScratchBufferPage
)
{
    // block 0 is loaded at start of aScratchBufferPage
    // block 1 does not contain "EFI PART" at its start

    return K2STAT_ERROR_NOT_IMPL;
}


K2STAT
EfiPart_ValidateGPT1(
    GPT_SECTOR const *      apSector1,
    StorMediaProp const *   apMedia
)
{
    UINT64  partitionTableSize;
    UINT32  crc;
    UINT32  zero;

    if ((NULL == apSector1) ||
        (NULL == apMedia))
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((apSector1->Header.HeaderSize < sizeof(GPT_HEADER)) ||
        (apSector1->Header.HeaderSize >= apMedia->mBlockSizeBytes))
        return K2STAT_ERROR_CORRUPTED;

    zero = 0;
    crc = K2CRC_Calc32(0, apSector1, 16);
    crc = K2CRC_Calc32(crc, &zero, 4);
    crc = K2CRC_Calc32(crc, &apSector1->Header.Reserved, apSector1->Header.HeaderSize - 20);
    if (apSector1->Header.HeaderCRC32 != crc)
        return K2STAT_ERROR_CORRUPTED;

    if ((apSector1->Header.MyLBA != 1) ||
        (apSector1->Header.FirstUsableLBA < 2) ||
        (apSector1->Header.AlternateLBA >= apMedia->mBlockCount) ||
        (apSector1->Header.LastUsableLBA >= apMedia->mBlockCount) ||
        (apSector1->Header.FirstUsableLBA >= apSector1->Header.LastUsableLBA))
        return K2STAT_ERROR_CORRUPTED;

    if ((apSector1->Header.PartitionEntryLBA < 2) ||
        ((apSector1->Header.PartitionEntryLBA >= apSector1->Header.FirstUsableLBA) &&
            (apSector1->Header.PartitionEntryLBA <= apSector1->Header.LastUsableLBA)) ||
        (sizeof(GPT_ENTRY) > apSector1->Header.SizeOfPartitionEntry) ||
        (apSector1->Header.NumberOfPartitionEntries == 0))
        return K2STAT_ERROR_CORRUPTED;

    partitionTableSize = apSector1->Header.SizeOfPartitionEntry * apSector1->Header.NumberOfPartitionEntries;
    partitionTableSize = (partitionTableSize + (apMedia->mBlockSizeBytes - 1)) / (apMedia->mBlockSizeBytes);
    if (apSector1->Header.PartitionEntryLBA < apSector1->Header.FirstUsableLBA)
    {
        if ((apSector1->Header.FirstUsableLBA - apSector1->Header.PartitionEntryLBA) < partitionTableSize)
            return K2STAT_ERROR_CORRUPTED;
    }
    else
    {
        if ((apMedia->mBlockCount - apSector1->Header.PartitionEntryLBA) < partitionTableSize)
            return K2STAT_ERROR_CORRUPTED;
    }

    return K2STAT_NO_ERROR;
}

K2STAT
EfiPart_ValidateGPTAlt(
    GPT_SECTOR const *      apSector1,
    GPT_SECTOR const *      apAltSector,
    StorMediaProp const *   apMedia
)
{
    UINT64  partitionTable1Size;
    UINT64  partitionTable2Size;
    UINT32  crc;
    UINT32  zero;
    K2STAT  stat;

    if ((NULL == apSector1) ||
        (NULL == apAltSector) ||
        (NULL == apMedia))
        return K2STAT_ERROR_BAD_ARGUMENT;

    stat = EfiPart_ValidateGPT1(apSector1, apMedia);
    if (K2STAT_IS_ERROR(stat))
        return stat;

    partitionTable1Size = apSector1->Header.SizeOfPartitionEntry * apSector1->Header.NumberOfPartitionEntries;
    partitionTable1Size = (partitionTable1Size + (apMedia->mBlockSizeBytes - 1)) / (apMedia->mBlockSizeBytes);

    if ((apAltSector->Header.HeaderSize != apSector1->Header.HeaderSize) ||
        (apSector1->Header.AlternateLBA != apAltSector->Header.MyLBA))
        return K2STAT_ERROR_CORRUPTED;

    zero = 0;
    crc = K2CRC_Calc32(0, apAltSector, 16);
    crc = K2CRC_Calc32(crc, &zero, 4);
    crc = K2CRC_Calc32(crc, &apAltSector->Header.Reserved, apAltSector->Header.HeaderSize - 20);
    if (apAltSector->Header.HeaderCRC32 != crc)
        return K2STAT_ERROR_CORRUPTED;

    if ((apAltSector->Header.Revision != apSector1->Header.Revision) ||
        (apAltSector->Header.HeaderSize != apSector1->Header.HeaderSize) ||
        (apAltSector->Header.MyLBA != apSector1->Header.AlternateLBA) ||
        (apAltSector->Header.AlternateLBA != apSector1->Header.MyLBA) ||
        (apAltSector->Header.FirstUsableLBA != apSector1->Header.FirstUsableLBA) ||
        (apAltSector->Header.LastUsableLBA != apSector1->Header.LastUsableLBA) ||
        (0 != K2MEM_Compare(&apAltSector->Header.DiskGuid, &apSector1->Header.DiskGuid, sizeof(K2_GUID128))) ||
        (apAltSector->Header.NumberOfPartitionEntries != apSector1->Header.NumberOfPartitionEntries) ||
        (apAltSector->Header.SizeOfPartitionEntry != apSector1->Header.SizeOfPartitionEntry) ||
        (apAltSector->Header.PartitionEntryArrayCRC32 != apSector1->Header.PartitionEntryArrayCRC32) ||
        (apAltSector->Header.PartitionEntryLBA >= apMedia->mBlockCount))
        return K2STAT_ERROR_CORRUPTED;

    if ((apAltSector->Header.PartitionEntryLBA < 2) ||
        ((apAltSector->Header.PartitionEntryLBA >= apAltSector->Header.FirstUsableLBA) &&
            (apAltSector->Header.PartitionEntryLBA <= apAltSector->Header.LastUsableLBA)))
        return K2STAT_ERROR_CORRUPTED;

    partitionTable2Size = apSector1->Header.SizeOfPartitionEntry * apSector1->Header.NumberOfPartitionEntries;
    partitionTable2Size = (partitionTable2Size + (apMedia->mBlockSizeBytes - 1)) / (apMedia->mBlockSizeBytes);
    if (partitionTable1Size != partitionTable2Size)
        return K2STAT_ERROR_CORRUPTED;

    if (apAltSector->Header.PartitionEntryLBA < apAltSector->Header.FirstUsableLBA)
    {
        if ((apAltSector->Header.FirstUsableLBA - apAltSector->Header.PartitionEntryLBA) < partitionTable1Size)
            return K2STAT_ERROR_CORRUPTED;
    }
    else
    {
        if ((apMedia->mBlockCount - apAltSector->Header.PartitionEntryLBA) < partitionTable1Size)
            return K2STAT_ERROR_CORRUPTED;
    }

    return K2STAT_NO_ERROR;
}

K2STAT
EfiPart_ValidateGPTPartitions(
    GPT_SECTOR const *      apSector1,
    GPT_SECTOR const *      apAltSector,
    StorMediaProp const *   apMedia,
    UINT8 const *           apPartTab1,
    UINT8 const *           apPartTab2,
    UINT_PTR *              apRetNonEmptyPartCount
)
{
    UINT64          partitionTableSize;
    UINT32          alignEntry[(sizeof(GPT_ENTRY) + 3) / 4];
    GPT_ENTRY *     pEntry;
    UINT8 const *   pScan;
    UINT_PTR        partCount;
    UINT_PTR        ixPart;

    if ((NULL == apSector1) ||
        (NULL == apAltSector) ||
        (NULL == apMedia) ||
        (NULL == apPartTab1) ||
        (NULL == apPartTab2))
        return K2STAT_ERROR_BAD_ARGUMENT;

    partitionTableSize = apSector1->Header.SizeOfPartitionEntry * apSector1->Header.NumberOfPartitionEntries;

    if (0 != K2MEM_Compare(apPartTab1, apPartTab2, (UINT_PTR)partitionTableSize))
        return K2STAT_ERROR_CORRUPTED;

    if (apSector1->Header.PartitionEntryArrayCRC32 !=
        K2CRC_Calc32(0, apPartTab1, (UINT_PTR)partitionTableSize))
        return K2STAT_ERROR_CORRUPTED;

    pEntry = (GPT_ENTRY *)&alignEntry[0];

    pScan = apPartTab1;
    partCount = 0;
    for (ixPart = 0; ixPart < apSector1->Header.NumberOfPartitionEntries; ixPart++)
    {
        K2MEM_Copy(pEntry, pScan, sizeof(GPT_ENTRY));
        pScan += apSector1->Header.SizeOfPartitionEntry;
        if (0 == K2MEM_VerifyZero(&pEntry->PartitionTypeGuid, sizeof(K2_GUID128)))
        {
            if ((pEntry->StartingLBA >= apMedia->mBlockCount) ||
                (pEntry->EndingLBA >= apMedia->mBlockCount) ||
                (pEntry->EndingLBA < pEntry->StartingLBA))
            {
                //
                // invalid partition
                //
                return K2STAT_ERROR_CORRUPTED;
            }

            partCount++;
        }
    }

    if (NULL != apRetNonEmptyPartCount)
        *apRetNonEmptyPartCount = partCount;

    return K2STAT_NO_ERROR;
}

int 
RunEfiPart(
    IStorMediaSession * apSession,
    UINT_PTR            aScratchBufferPage
)
{
    static K2_GUID128 const sBasicPartGuid = GPT_BASIC_DATA_PART_GUID;

    // block 1 is loaded at start of aScratchBufferPage
    // and contains at its start "EFI PART"
    K2STAT              stat;
    GPT_SECTOR const *  pSector1;
    GPT_SECTOR *        pAltSector;
    UINT_PTR            partCount;
    UINT_PTR            partitionTableBlocks;
    UINT_PTR            partitionTablePages;
    GPT_ENTRY           entry;
    StorMediaProp       MediaProp;
    BOOL                mediaReadOnly;
    UINT8 *             pPartTab1;
    UINT8 *             pPartTab2;
    int                 result;
    MyPartition **      ppParts;
    MyPartition *       pPart;
    UINT_PTR            partIx;
    UINT_PTR            bufIx;

    pSector1 = (GPT_SECTOR const *)aScratchBufferPage;
    if (!apSession->GetMediaProp(&MediaProp, &mediaReadOnly))
    {
        return GetLastError();
    }

    stat = EfiPart_ValidateGPT1(pSector1, &MediaProp);
    if (K2STAT_IS_ERROR(stat))
    {
        SetLastError(stat);
        return GetLastError();
    }

    pAltSector = (GPT_SECTOR *)(aScratchBufferPage + MediaProp.mBlockSizeBytes);
    if (!apSession->ReadBlocks((UINT_PTR)pSector1->Header.AlternateLBA, 1, pAltSector))
    {
        return GetLastError();
    }

    stat = EfiPart_ValidateGPTAlt(pSector1, pAltSector, &MediaProp);
    if (K2STAT_IS_ERROR(stat))
    {
        SetLastError(stat);
        return GetLastError();
    }

    partitionTableBlocks = pSector1->Header.SizeOfPartitionEntry * pSector1->Header.NumberOfPartitionEntries;
    partitionTablePages = (partitionTableBlocks / K2_VA_MEMPAGE_BYTES) * K2_VA_MEMPAGE_BYTES;
    partitionTableBlocks = (partitionTableBlocks + (MediaProp.mBlockSizeBytes - 1)) / (MediaProp.mBlockSizeBytes);

    pPartTab1 = (UINT8 *)VirtualAlloc(0, partitionTablePages * K2_VA_MEMPAGE_BYTES, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (NULL == pPartTab1)
    {
        return FALSE;
    }

    result = -1;
    partCount = 0;

    do {
        if (!apSession->ReadBlocks((UINT_PTR)pSector1->Header.PartitionEntryLBA, partitionTableBlocks, pPartTab1))
        {
            result = GetLastError();
            break;
        }

        pPartTab2 = (UINT8 *)VirtualAlloc(0, partitionTablePages * K2_VA_MEMPAGE_BYTES, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (NULL == pPartTab2)
        {
            result = GetLastError();
            break;
        }

        do {
            if (!apSession->ReadBlocks((UINT_PTR)pAltSector->Header.PartitionEntryLBA, partitionTableBlocks, pPartTab2))
            {
                result = GetLastError();
                break;
            }

            stat = EfiPart_ValidateGPTPartitions(pSector1, pAltSector, &MediaProp, pPartTab1, pPartTab2, &partCount);
            if (K2STAT_IS_ERROR(stat))
            {
                SetLastError(stat);
                result = stat;
                break;
            }

            VirtualFree((LPVOID)pPartTab2, 0, MEM_RELEASE);
            pPartTab2 = NULL;

            if (0 != partCount)
            {
                ppParts = new MyPartition * [partCount];
                if (NULL == ppParts)
                {
                    SetLastError(K2STAT_ERROR_OUT_OF_MEMORY);
                    break;
                }

                pPartTab2 = pPartTab1;
                partIx = 0;

                for (bufIx = 0; bufIx < pSector1->Header.NumberOfPartitionEntries; bufIx++)
                {
                    K2MEM_Copy(&entry, pPartTab2, sizeof(GPT_ENTRY));
                    pPartTab2 += pSector1->Header.SizeOfPartitionEntry;

                    if (!K2MEM_VerifyZero(&entry.PartitionTypeGuid, sizeof(K2_GUID128)))
                    {
                        if ((entry.StartingLBA < MediaProp.mBlockCount) &&
                            (entry.EndingLBA < MediaProp.mBlockCount) &&
                            (entry.EndingLBA >= entry.StartingLBA))
                        {
                            ppParts[partIx] = pPart = new MyPartition(apSession, partIx, bufIx, &MediaProp);
                            if (NULL == pPart)
                            {
                                result = K2STAT_ERROR_OUT_OF_MEMORY;
                                break;
                            }

                            K2MEM_Copy(&pPart->mPartTypeGuid, &entry.PartitionTypeGuid, sizeof(K2_GUID128));
                            K2MEM_Copy(&pPart->mPartIdGuid, &entry.UniquePartitionGuid, sizeof(K2_GUID128));
                            pPart->mAttributes = entry.Attributes;
                            pPart->mStartBlock = (UINT_PTR)entry.StartingLBA;
                            pPart->mBlockCount = (UINT_PTR)(entry.EndingLBA - entry.StartingLBA + 1);
                            if (0 == K2MEM_Compare(&entry.PartitionTypeGuid, &sBasicPartGuid, sizeof(K2_GUID128)))
                            {
                                pPart->mIsReadOnly = ((entry.Attributes & GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY) != 0) ? 0xFF : 0;
                            }
                            pPart->mFlagActive = ((entry.Attributes & GPT_ATTRIBUTE_LEGACY_BIOS_BOOTABLE) != 0) ? 0xFF : 0;
                            pPart->mFlagEFI = 0xFF;
                            pPart++;
                            if (++partIx == partCount)
                                break;
                        }
                    }
                }

                if (partIx == partCount)
                {
                    result = RunParts(partCount, ppParts);
                }

                if (partIx > 0)
                {
                    do {
                        partIx--;
                        pPart = ppParts[partIx];
                        pPart->Release();
                    } while (partIx > 0);
                }

                delete[] ppParts;
            }
            else
            {
                result = 0;
            }

        } while (0);

        if (NULL != pPartTab2)
        {
            VirtualFree((LPVOID)pPartTab2, 0, MEM_RELEASE);
        }

    } while (0);

    VirtualFree((LPVOID)pPartTab1, 0, MEM_RELEASE);

    return result;
}

int RunAdapter(IStorAdapter *apAdapter)
{
    IStorMediaSession * pSession;
    int                 result;
    UINT_PTR            partPageAddr;

    result = 0;

    do {
        if (!apAdapter->AcquireSession(&pSession))
        {
            result = GetLastError();
            break;
        }

        do {
            partPageAddr = (UINT_PTR)VirtualAlloc(0, K2_VA_MEMPAGE_BYTES, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (0 == partPageAddr)
            {
                result = GetLastError();
                break;
            }

            do {
                //
                // look for GPT
                //
                if (!pSession->ReadBlocks(1, 1, (void *)partPageAddr))
                {
                    result = GetLastError();
                    break;
                }

                if (0 == K2MEM_Compare((void *)partPageAddr, "EFI PART", 8))
                {
                    result = RunEfiPart(pSession, partPageAddr);
                }
                else
                {
                    if (!pSession->ReadBlocks(0, 1, (void *)partPageAddr))
                    {
                        result = GetLastError();
                        break;
                    }

                    result = RunMbrPart(pSession, partPageAddr);
                }


            } while (0);

            VirtualFree((LPVOID)partPageAddr, 0, MEM_RELEASE);

        } while (0);

        pSession->Release();
        pSession = NULL;

    } while (1);

    return result;
}
