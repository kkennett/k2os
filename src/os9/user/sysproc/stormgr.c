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

#include "sysproc.h"
#include <k2osstor.h>
#include <k2osdev_blockio.h>

static K2OS_THREAD_TOKEN    sgStorMgrTokThread;
static UINT32               sgStorMgrThreadId;
static K2OS_CRITSEC         sgStorDevListSec;
static K2LIST_ANCHOR        sgStorDevList;
static K2OS_MAILBOX_TOKEN   sgStorMgrTokMailbox;

static K2OS_RPC_CLASS       sgGptRpcClass;
static K2OS_RPC_CLASS       sgMbrRpcClass;

typedef struct _STORPART        STORPART;
typedef struct _STORMEDIA       STORMEDIA;
typedef struct _STORDEV         STORDEV;
typedef struct _STORDEV_ACTION  STORDEV_ACTION;

typedef enum _StorDev_ActionType StorDev_ActionType;
enum _StorDev_ActionType
{
    StorDev_Action_Invalid = 0,

    StorDev_Action_Media_Changed,
    StorDev_Action_BlockIo_Departed,

    StorDev_ActionType_Count
};

struct _STORDEV_ACTION
{
    StorDev_ActionType          mAction;
    STORDEV_ACTION * volatile   mpNext;
};

#define DEV_BLOCKBUFFER_COUNT   2       // first is always primary gpt;  second varies

K2_PACKED_PUSH
struct _STORPART
{
    UINT32                  mArrayIndex;
    UINT32                  mTableIndex;
    K2OS_STORAGE_PARTITION  Info;
    K2OS_RPC_OBJ            mRpcObj;
    K2OS_RPC_OBJ_HANDLE     mRpcObjHandle;
    K2OS_RPC_IFINST         mRpcIfInst;
    K2OS_IFINST_ID          mIfInstId;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

struct _STORMEDIA
{
    STORDEV *           mpStorDev;
    K2OS_STORAGE_MEDIA  Def;
    UINT32              mPartCount;
    STORPART            Part[1];    // always at least one
};

struct _STORDEV
{
    K2OS_IFINST_ID              mIfInstId;
    UINT32                      mThreadId;
    K2LIST_LINK                 ListLink;
    K2OS_SIGNAL_TOKEN           mTokActionNotify;
    K2OSSTOR_BLOCKIO            mStorBlockIo;
    STORDEV_ACTION * volatile   mpActions;
    UINT8 *                     mpBlockMem;
    UINT8 *                     mpBlockBuffer;
    STORMEDIA *                 mpMedia;
    BOOL                        mIsGpt;
    K2OSSTOR_BLOCKIO_RANGE      mRanges[4]; // gpt, parttab, alt-gpt, alt-parttab, or just mbr sector
};

K2STAT 
StorPart_Create(
    K2OS_RPC_OBJ                aObject, 
    K2OS_RPC_OBJ_CREATE const * apCreate, 
    UINT32 *                    apRetContext
)
{
    STORPART *  pPart;

    if (apCreate->mCreatorProcessId != K2OS_Process_GetId())
    {
        return K2STAT_ERROR_NOT_ALLOWED;
    }

    pPart = (STORPART *)apCreate->mCreatorContext;
    *apRetContext = (UINT32)pPart;

    pPart->mRpcObj = aObject;

    return K2STAT_NO_ERROR;
}

K2STAT 
StorPart_Call(
    K2OS_RPC_OBJ_CALL const *   apCall, 
    UINT32 *                    apRetUsedOutBytes
)
{
    STORPART *                              pPart;
    K2STAT                                  stat;
    STORMEDIA *                             pMedia;
    K2OS_STORAGE_PARTITION_GET_MEDIA_OUT *  pMediaOut;
    K2OS_STORAGE_PARTITION_GET_INFO_OUT *   pGetInfoOut;

    pPart = (STORPART *)apCall->mObjContext;
    K2_ASSERT(pPart->mRpcObj == apCall->mObj);
    pMedia = K2_GET_CONTAINER(STORMEDIA, pPart, Part[pPart->mArrayIndex]);

    switch (apCall->Args.mMethodId)
    {
    case K2OS_STORAGE_PARTITION_METHOD_GET_MEDIA:
        pMediaOut = (K2OS_STORAGE_PARTITION_GET_MEDIA_OUT *)apCall->Args.mpOutBuf;
        if ((0 != apCall->Args.mInBufByteCount) ||
            (apCall->Args.mOutBufByteCount < sizeof(K2OS_STORAGE_PARTITION_GET_MEDIA_OUT)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            K2MEM_Copy(&pMediaOut->Media, &pMedia->Def, sizeof(K2OS_STORAGE_MEDIA));
            pMediaOut->mDevIfInstId = pMedia->mpStorDev->mIfInstId;
            pMediaOut->mIsGpt = pMedia->mpStorDev->mIsGpt;
            pMediaOut->mPartitionCount = pMedia->mPartCount;
            *apRetUsedOutBytes = sizeof(K2OS_STORAGE_PARTITION_GET_MEDIA_OUT);
            stat = K2STAT_NO_ERROR;
        }
        break;

    case K2OS_STORAGE_PARTITION_METHOD_GET_INFO:
        pGetInfoOut = (K2OS_STORAGE_PARTITION_GET_INFO_OUT *)apCall->Args.mpOutBuf;
        if ((0 != apCall->Args.mInBufByteCount) ||
            (apCall->Args.mOutBufByteCount < sizeof(K2OS_STORAGE_PARTITION_GET_INFO_OUT)))
        {
            stat = K2STAT_ERROR_BAD_ARGUMENT;
        }
        else
        {
            K2MEM_Copy(&pGetInfoOut->Info, &pPart->Info, sizeof(K2OS_STORAGE_PARTITION));
            pGetInfoOut->mIndex = pPart->mArrayIndex;
            pGetInfoOut->mTableIndex = pPart->mTableIndex;
            pGetInfoOut->mDevIfInstId = pMedia->mpStorDev->mIfInstId;
            *apRetUsedOutBytes = sizeof(K2OS_STORAGE_PARTITION_GET_INFO_OUT);
            stat = K2STAT_NO_ERROR;
        }
        break;

    default:
        stat = K2STAT_ERROR_NO_INTERFACE;
        break;
    }

    return stat;
}

K2STAT 
StorPart_Delete(
    K2OS_RPC_OBJ    aObject, 
    UINT32          aObjContext
)
{
    STORPART *  pPart;

    pPart = (STORPART *)aObjContext;
    pPart->mRpcObj = NULL;

    return K2STAT_NO_ERROR;
}

static K2OS_RPC_OBJ_CLASSDEF sgGptPartClassDef =
{
    K2OS_GPT_PARTITION_OBJECT_CLASSID,
    StorPart_Create,
    NULL,
    NULL,
    StorPart_Call,
    StorPart_Delete
};

static K2OS_RPC_OBJ_CLASSDEF sgMbrPartClassDef =
{
    K2OS_MBR_PARTITION_OBJECT_CLASSID,
    StorPart_Create,
    NULL,
    NULL,
    StorPart_Call,
    StorPart_Delete
};

K2STAT
StorDev_ValidateGPT1(
    GPT_SECTOR const *          apSector1,
    K2OS_STORAGE_MEDIA const *  apMedia
)
{
    UINT64  partitionTableSize;
    UINT32  crc;
    UINT32  zero;

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
StorDev_ValidateGptAlt(
    GPT_SECTOR const *          apSector1,
    GPT_SECTOR const *          apAltSector,
    K2OS_STORAGE_MEDIA const *  apMedia
)
{
    UINT64  partitionTable1Size;
    UINT64  partitionTable2Size;
    UINT32  crc;
    UINT32  zero;

    if ((apAltSector->Header.HeaderSize < sizeof(GPT_HEADER)) ||
        (apAltSector->Header.HeaderSize >= apMedia->mBlockSizeBytes))
        return K2STAT_ERROR_CORRUPTED;

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
StorDev_ValidateGptPartitions(
    GPT_SECTOR const *          apSector1,
    GPT_SECTOR const *          apAltSector,
    K2OS_STORAGE_MEDIA const *  apMedia,
    UINT8 const *               apPartTab1,
    UINT8 const *               apPartTab2,
    UINT_PTR *                  apRetNonEmptyPartCount
)
{
    UINT32          partitionTableSize;
    UINT32          alignEntry[(sizeof(GPT_ENTRY) + 3) / 4];
    GPT_ENTRY *     pEntry;
    UINT8 const *   pScan;
    UINT_PTR        partCount;
    UINT_PTR        ixPart;

    partitionTableSize = apSector1->Header.SizeOfPartitionEntry * apSector1->Header.NumberOfPartitionEntries;

    if (0 != K2MEM_Compare(apPartTab1, apPartTab2, partitionTableSize))
    {
        Debug_Printf("Partition table contents are not the same\n");
        return K2STAT_ERROR_CORRUPTED;
    }

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
                Debug_Printf("!!!STORMGR: Gpt Partition table %d is not empty and not valid. Ignored\n", ixPart);
                return K2STAT_ERROR_CORRUPTED;
            }

            partCount++;
        }
    }

    if (NULL != apRetNonEmptyPartCount)
        *apRetNonEmptyPartCount = partCount;

    return K2STAT_NO_ERROR;
}

void
StorDev_FillGptPart(
    GPT_SECTOR const *  apSector1,
    STORMEDIA *         pStorMedia,
    UINT8 const *       apPartTab1
)
{
    static K2_GUID128 const sBasicPartGuid = GPT_BASIC_DATA_PART_GUID;
    static K2_GUID128 const sEfiSystemPartGuid = GPT_EFI_SYSTEM_PARTITION_GUID;

    UINT32          alignEntry[(sizeof(GPT_ENTRY) + 3) / 4];
    GPT_ENTRY *     pEntry;
    UINT8 const *   pScan;
    UINT_PTR        ixPart;
    STORPART *      pPart;

    pEntry = (GPT_ENTRY *)&alignEntry[0];

    pScan = apPartTab1;
    pPart = &pStorMedia->Part[0];
    for (ixPart = 0; ixPart < apSector1->Header.NumberOfPartitionEntries; ixPart++)
    {
        K2MEM_Copy(pEntry, pScan, sizeof(GPT_ENTRY));
        pScan += apSector1->Header.SizeOfPartitionEntry;
        if (0 == K2MEM_VerifyZero(&pEntry->PartitionTypeGuid, sizeof(K2_GUID128)))
        {
            if ((pEntry->StartingLBA < pStorMedia->Def.mBlockCount) &&
                (pEntry->EndingLBA < pStorMedia->Def.mBlockCount) &&
                (pEntry->EndingLBA >= pEntry->StartingLBA))
            {
                K2MEM_Copy(&pPart->Info.mIdGuid, &pEntry->UniquePartitionGuid, sizeof(K2_GUID128));
                K2MEM_Copy(&pPart->Info.mTypeGuid, &pEntry->PartitionTypeGuid, sizeof(K2_GUID128));
                pPart->mTableIndex = ixPart;
                pPart->Info.mAttributes = pEntry->Attributes;
                pPart->Info.mFlagActive = (0 != (pEntry->Attributes & GPT_ATTRIBUTE_LEGACY_BIOS_BOOTABLE)) ? TRUE : FALSE;
                if (0 == K2MEM_Compare(&pPart->Info.mTypeGuid, &sBasicPartGuid, sizeof(K2_GUID128)))
                {
                    if (0 != (pEntry->Attributes & GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY))
                    {
                        pPart->Info.mFlagReadOnly = TRUE;
                    }
                }
                else if (0 == K2MEM_Compare(&pPart->Info.mTypeGuid, &sEfiSystemPartGuid, sizeof(K2_GUID128)))
                {
                    pPart->Info.mFlagEFI = TRUE;
                }
                pPart->Info.mStartBlock = pEntry->StartingLBA;
                pPart->Info.mBlockCount = (pEntry->EndingLBA - pEntry->StartingLBA) + 1;
                pPart->Info.mBlockSizeBytes = pStorMedia->Def.mBlockSizeBytes;
                pPart++;
            }
        }
    }
}

K2STAT
StorDev_MountGpt(
    STORDEV *                   apDev,
    K2OS_STORAGE_MEDIA const *  apMedia,
    K2OSSTOR_BLOCKIO_RANGE *    apRange
)
{
    static K2_GUID128 const sPartitionIfaceClassId = K2OS_IFACE_STORAGE_PARTITION;

    K2STAT          stat;
    UINT64          devByteOffset;
    GPT_SECTOR *    pGptSector;
    GPT_SECTOR *    pAltSector;
    UINT8 *         pTable1Mem;
    UINT8 *         pPartTab1;
    UINT8 *         pTable2Mem;
    UINT8 *         pPartTab2;
    UINT32          partitionTableSize;
    UINT32          partitionTableSizeSectorBytes;
    UINT64          byteOffset;
    UINT32          u;
    UINT32          partCount;
    BOOL            ok;
    STORMEDIA *     pStorMedia;
    STORPART *      pPart;
    UINT32          byteCount;
    UINT64          rangeBlocks;

    u = apMedia->mBlockSizeBytes;

    pGptSector = (GPT_SECTOR *)apDev->mpBlockBuffer;
    pAltSector = (GPT_SECTOR *)(apDev->mpBlockBuffer + u);

    stat = StorDev_ValidateGPT1(pGptSector, apMedia);
    if (K2STAT_IS_ERROR(stat))
    {
        Debug_Printf("STORMGR: Gpt1 did not validate properly (%08X)\n", stat);
        return stat;
    }

    devByteOffset = pGptSector->Header.AlternateLBA * u;

    stat = K2OS_BlockIo_Read(apDev->mStorBlockIo, *apRange, &devByteOffset, pAltSector, u);
    if (K2STAT_IS_ERROR(stat))
    {
        Debug_Printf("STORMGR: Failed to read alt gpt in block %d from media\n", (UINT32)(devByteOffset / u));
        return stat;
    }

    stat = StorDev_ValidateGptAlt(pGptSector, pAltSector, apMedia);
    if (K2STAT_IS_ERROR(stat))
    {
        Debug_Printf("STORMGR: GptAlt did not validate properly (%08X)\n", stat);
        return stat;
    }

    //
    // load both partition tables into memory
    //
    partitionTableSize = pGptSector->Header.SizeOfPartitionEntry * pGptSector->Header.NumberOfPartitionEntries;
    partitionTableSizeSectorBytes = ((partitionTableSize + (u - 1)) / u) * u;

    pTable1Mem = (UINT8 *)K2OS_Heap_Alloc(partitionTableSizeSectorBytes + u);
    if (NULL == pTable1Mem)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        Debug_Printf("STORMGR: Out of memory allocating buffer for partition table 1\n");
        return stat;
    }
    pPartTab1 = (UINT8 *)(((((UINT32)pTable1Mem) + (u - 1)) / u) * u);
    byteOffset = pGptSector->Header.PartitionEntryLBA * u;
    if (!K2OS_BlockIo_Read(apDev->mStorBlockIo, *apRange, &byteOffset, pPartTab1, partitionTableSizeSectorBytes))
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        Debug_Printf("STORMGR: Could not load partition table 1\n");
        K2OS_Heap_Free(pTable1Mem);
        return stat;
    }
    if (pGptSector->Header.PartitionEntryArrayCRC32 != K2CRC_Calc32(0, pPartTab1, partitionTableSize))
    {
        Debug_Printf("Partition table 1 crc invalid\n");
        K2OS_Heap_Free(pTable1Mem);
        return K2STAT_ERROR_CORRUPTED;
    }

    pTable2Mem = (UINT8 *)K2OS_Heap_Alloc(partitionTableSizeSectorBytes + u);
    if (NULL == pTable2Mem)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        Debug_Printf("STORMGR: Out of memory allocating buffer for partition table 2\n");
        K2OS_Heap_Free(pTable1Mem);
        return stat;
    }
    pPartTab2 = (UINT8 *)(((((UINT32)pTable2Mem) + (u - 1)) / u) * u);
    byteOffset = pAltSector->Header.PartitionEntryLBA * u;
    if (!K2OS_BlockIo_Read(apDev->mStorBlockIo, *apRange, &byteOffset, pPartTab2, partitionTableSizeSectorBytes))
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        Debug_Printf("STORMGR: Could not load partition table 2\n");
        K2OS_Heap_Free(pTable2Mem);
        K2OS_Heap_Free(pTable1Mem);
        return stat;
    }
    if (pAltSector->Header.PartitionEntryArrayCRC32 != K2CRC_Calc32(0, pPartTab2, partitionTableSize))
    {
        Debug_Printf("Partition alt table crc invalid\n");
        K2OS_Heap_Free(pTable2Mem);
        K2OS_Heap_Free(pTable1Mem);
        return K2STAT_ERROR_CORRUPTED;
    }

    partCount = 0;
    stat = StorDev_ValidateGptPartitions(pGptSector, pAltSector, apMedia, pPartTab1, pPartTab2, &partCount);
    K2OS_Heap_Free(pTable2Mem);
    pTable2Mem = pPartTab2 = NULL;
    if (K2STAT_IS_ERROR(stat))
    {
        Debug_Printf("STORMGR: GPT partitions are not conherent/valid\n");
        K2OS_Heap_Free(pTable1Mem);
        return stat;
    }

    apDev->mIsGpt = TRUE;

    if (0 == partCount)
    {
        Debug_Printf("STORMGR: No valid partitions found\n");
        K2OS_Heap_Free(pTable1Mem);
        return K2STAT_ERROR_EMPTY;
    }

    byteCount = sizeof(STORMEDIA) + ((partCount - 1) * sizeof(STORPART));
    pStorMedia = (STORMEDIA *)K2OS_Heap_Alloc(byteCount);
    if (NULL == pStorMedia)
    {
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        Debug_Printf("STORMGR: Could not allocate memory for storage media\n");
        K2OS_Heap_Free(pTable1Mem);
        return stat;
    }
    K2MEM_Zero(pStorMedia, byteCount);
    K2MEM_Copy(&pStorMedia->Def, apMedia, sizeof(K2OS_STORAGE_MEDIA));
    pStorMedia->mpStorDev = apDev;
    pStorMedia->mPartCount = partCount;
    for (byteCount = 0; byteCount < partCount; byteCount++)
    {
        pStorMedia->Part[byteCount].mArrayIndex = byteCount;
    }

    StorDev_FillGptPart(pGptSector, pStorMedia, pPartTab1);

    //
    // partition table is valid.  create the private ranges covering the gpt and partition tables (regular and alt)
    //
    rangeBlocks = 1;
    apDev->mRanges[0] = K2OS_BlockIo_RangeCreate(apDev->mStorBlockIo, &pGptSector->Header.MyLBA, &rangeBlocks, TRUE);
    if (NULL == apDev->mRanges[0])
    {
        Debug_Printf("STORMGR: Could not create private range for primary GPT\n");
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
        K2OS_Heap_Free(pStorMedia);
        K2OS_Heap_Free(pTable1Mem);
        return stat;
    }

    do {
        rangeBlocks = partitionTableSize / u;
        apDev->mRanges[1] = K2OS_BlockIo_RangeCreate(apDev->mStorBlockIo, &pGptSector->Header.PartitionEntryLBA, &rangeBlocks, TRUE);
        if (NULL == apDev->mRanges[1])
        {
            Debug_Printf("STORMGR: Could not create private range for primary partition table\n");
            stat = K2OS_Thread_GetLastStatus();
            K2_ASSERT(K2STAT_IS_ERROR(stat));
            break;
        }
        do {
            rangeBlocks = 1;
            apDev->mRanges[2] = K2OS_BlockIo_RangeCreate(apDev->mStorBlockIo, &pAltSector->Header.MyLBA, &rangeBlocks, TRUE);
            if (NULL == apDev->mRanges[2])
            {
                Debug_Printf("STORMGR: Could not create private range for alt GPT\n");
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                break;
            }

            rangeBlocks = partitionTableSize / u;
            apDev->mRanges[3] = K2OS_BlockIo_RangeCreate(apDev->mStorBlockIo, &pAltSector->Header.PartitionEntryLBA, &rangeBlocks, TRUE);
            if (NULL == apDev->mRanges[3])
            {
                Debug_Printf("STORMGR: Could not create private range for primary partition table\n");
                stat = K2OS_Thread_GetLastStatus();
                K2_ASSERT(K2STAT_IS_ERROR(stat));
                ok = K2OS_BlockIo_RangeDelete(apDev->mStorBlockIo, apDev->mRanges[2]);
                K2_ASSERT(ok);
                apDev->mRanges[2] = NULL;
            }

            stat = K2STAT_NO_ERROR;

        } while (0);

        if (K2STAT_IS_ERROR(stat))
        {
            ok = K2OS_BlockIo_RangeDelete(apDev->mStorBlockIo, apDev->mRanges[1]);
            K2_ASSERT(ok);
            apDev->mRanges[1] = NULL;
        }

    } while (0);

    if (K2STAT_IS_ERROR(stat))
    {
        ok = K2OS_BlockIo_RangeDelete(apDev->mStorBlockIo, apDev->mRanges[0]);
        K2_ASSERT(ok);
        apDev->mRanges[0] = NULL;
        K2OS_Heap_Free(pStorMedia);
        K2OS_Heap_Free(pTable1Mem);
        return stat;
    }

    //
    // cannot fail from this point on
    //

    ok = K2OS_BlockIo_RangeDelete(apDev->mStorBlockIo, *apRange);
    K2_ASSERT(ok);
    *apRange = NULL;

    apDev->mpMedia = pStorMedia;

    //
    // create the objects, interface instances, publish them for the partitions
    //
    for (byteCount = 0; byteCount < partCount; byteCount++)
    {
        pPart = &pStorMedia->Part[byteCount];

        pPart->mRpcObjHandle = K2OS_Rpc_CreateObj(0, &sgGptPartClassDef.ClassId, (UINT32)pPart);
        if (NULL == pPart->mRpcObjHandle)
        {
            Debug_Printf("STORMGR: Could not create RPC object for media gpt partition %d\n", byteCount);
        }
        else
        {
            K2_ASSERT(NULL != pPart->mRpcObj);
            pPart->mRpcIfInst = K2OS_RpcObj_AddIfInst(
                pPart->mRpcObj,
                K2OS_IFACE_CLASSCODE_STORAGE_PARTITION,
                &sPartitionIfaceClassId,
                &pPart->mIfInstId,
                TRUE
            );
            if (NULL == pPart->mRpcIfInst)
            {
                Debug_Printf("STORMGR: Could not create interface instance for media gpt partition %d\n", byteCount);
                K2OS_Rpc_Release(pPart->mRpcObjHandle);
                pPart->mRpcObjHandle = NULL;
                K2_ASSERT(NULL == pPart->mRpcObj);
            }
        }
    }

    return K2STAT_NO_ERROR;
}

K2STAT
StorDev_MountMbr(
    STORDEV *                   apDev,
    K2OS_STORAGE_MEDIA const *  apMedia,
    K2OSSTOR_BLOCKIO_RANGE *        apRange
)
{
    return K2STAT_ERROR_NOT_SUPPORTED;
}

void
StorDev_Mount(
    STORDEV *               apDev,
    K2OS_STORAGE_MEDIA *    apMedia
)
{
    K2OSSTOR_BLOCKIO_RANGE  range;
    GPT_SECTOR *            pGptSector;
    K2STAT                  stat;
    UINT32                  u;
    UINT64                  rangeBlock;

//    Debug_Printf("STORMGR: Thread %d IfInstId %d Mount \"%s\"\n", K2OS_Thread_GetId(), apDev->mIfInstId, apMedia->mFriendly);
    rangeBlock = 0;
    range = K2OS_BlockIo_RangeCreate(apDev->mStorBlockIo, &rangeBlock, &apMedia->mBlockCount, TRUE);
    if (NULL == range)
    {
        Debug_Printf("STORMGR: Failed to create private range (whole device)\n");
        return;
    }

    apDev->mpBlockMem = (UINT8 *)K2OS_Heap_Alloc(apMedia->mBlockSizeBytes * (DEV_BLOCKBUFFER_COUNT + 1));
    if (NULL == apDev->mpBlockMem)
    {
        Debug_Printf("STORMGR: memory alloc for block buffer failed\n");
        K2OS_BlockIo_RangeDelete(apDev->mStorBlockIo, range);
        return;
    }

    // align to block size for transfers
    u = apMedia->mBlockSizeBytes;
    apDev->mpBlockBuffer = (UINT8 *)(((((UINT32)apDev->mpBlockMem) + (u - 1)) / u) * u);

    pGptSector = (GPT_SECTOR *)apDev->mpBlockBuffer;
    rangeBlock = u;
    if (!K2OS_BlockIo_Read(apDev->mStorBlockIo, range, &rangeBlock, pGptSector, u))
    {
        Debug_Printf("STORMGR: Failed to read block 1 of block device with ifinstid %d\n", apDev->mIfInstId);
        stat = K2OS_Thread_GetLastStatus();
        K2_ASSERT(K2STAT_IS_ERROR(stat));
    }
    else
    {
        if (0 == K2ASC_CompLen((char *)&pGptSector->Header.Signature, "EFI PART", 8))
        {
            stat = StorDev_MountGpt(apDev, apMedia, &range);
        }
        else
        {
            stat = K2STAT_ERROR_NOT_FOUND;
        }
        if (K2STAT_IS_ERROR(stat))
        {
            // try mbr
            stat = StorDev_MountMbr(apDev, apMedia, &range);
        }
    }

    if (K2STAT_IS_ERROR(stat))
    {
        apDev->mpBlockBuffer = NULL;
        K2OS_Heap_Free(apDev->mpBlockMem);
        apDev->mpBlockMem = NULL;
    }

    // if private range is still around get rid of it
    if (NULL != range)
    {
        K2OS_BlockIo_RangeDelete(apDev->mStorBlockIo, range);
    }
}

void
StorDev_Unmount(
    STORDEV *   apDev
)
{
    K2_ASSERT(NULL != apDev->mpMedia);
    Debug_Printf("STORMGR: Thread %d IfInstId %d Unmount \"%s\"\n", K2OS_Thread_GetId(), apDev->mIfInstId, apDev->mpMedia->Def.mFriendly);
}

void
StorDev_OnMediaChange(
    STORDEV *   apDev
)
{
    K2STAT              stat;
    K2OS_STORAGE_MEDIA  media;

    if (NULL != apDev->mpMedia)
    {
        StorDev_Unmount(apDev);
    }

    // media should be empty here
    K2_ASSERT(NULL == apDev->mpMedia);

    if (K2OS_BlockIo_GetMedia(apDev->mStorBlockIo, &media))
    {
        if ((media.mBlockSizeBytes > 0) &&
            (media.mBlockCount > 0))
        {
            StorDev_Mount(apDev, &media);
        }
        else
        {
            Debug_Printf("STORMGR: Thread %d IfInstId %d -- no blocks or 0 byte blocks\n", K2OS_Thread_GetId(), apDev->mIfInstId);
        }
    }
    else
    {
        stat = K2OS_Thread_GetLastStatus();
        if (stat == K2STAT_ERROR_NO_MEDIA)
        {
//            Debug_Printf("STORMGR: Thread %d IfInstId %d -- No media\n", K2OS_Thread_GetId(), apDev->mIfInstId);
        }
        else
        {
            Debug_Printf("STORMGR: Thread %d IfInstId %d returned %08X to GetMedia()\n", K2OS_Thread_GetId(), apDev->mIfInstId, stat);
        }
    }
}

void
StorDev_OnDepart(
    STORDEV *   apDev
)
{
    if (NULL != apDev->mpMedia)
    {
        StorDev_Unmount(apDev);
    }

    apDev->mIfInstId = 0;

    K2OS_BlockIo_Detach(apDev->mStorBlockIo);
    apDev->mStorBlockIo = NULL;
}

UINT32
StorMgr_DevThread(
    void *apArg
)
{
    STORDEV *           pThisDev;
    K2OS_WaitResult     waitResult;
    STORDEV_ACTION *    pActions;
    STORDEV_ACTION *    pLast;
    STORDEV_ACTION *    pNext;

    pThisDev = (STORDEV *)apArg;

//    Debug_Printf("STORMGR: Thread %d is servicing device with ifinstid %d\n", K2OS_Thread_GetId(), pThisDev->mIfInstId);

    StorDev_OnMediaChange(pThisDev);

    do {
        if (!K2OS_Thread_WaitOne(&waitResult, pThisDev->mTokActionNotify, K2OS_TIMEOUT_INFINITE))
            break;
        K2_ASSERT(K2OS_Wait_Signalled_0 == waitResult);

        do {
            //
            // snapshot actions
            //
            pActions = (STORDEV_ACTION *)K2ATOMIC_Exchange((volatile UINT32 *)&pThisDev->mpActions, 0);
            if (NULL == pActions)
                break;

            // reverse the list
            pLast = NULL;
            do {
                pNext = pActions->mpNext;
                pActions->mpNext = pLast;
                pLast = pActions;
                pActions = pNext;
            } while (NULL != pActions);
            pActions = pLast;

            do {
                pNext = pActions;
                pActions = pActions->mpNext;

                switch (pNext->mAction)
                {
                    case StorDev_Action_Media_Changed:
                        StorDev_OnMediaChange(pThisDev);
                        break;

                    case StorDev_Action_BlockIo_Departed:
                        StorDev_OnDepart(pThisDev);
                        break;

                    default:
                        K2_ASSERT(0);
                        break;
                }

                K2OS_Heap_Free(pNext);

            } while (NULL != pActions);

        } while (NULL != pThisDev->mStorBlockIo);

    } while (1);

    K2OS_Token_Destroy(pThisDev->mTokActionNotify);

    K2OS_CritSec_Enter(&sgStorDevListSec);
    K2LIST_Remove(&sgStorDevList, &pThisDev->ListLink);
    K2OS_CritSec_Leave(&sgStorDevListSec);

    K2OS_Heap_Free(pThisDev);

    return 0;
}

void
StorMgr_BlockIo_Arrived(
    K2OS_IFINST_ID aIfInstId
)
{
    K2OSSTOR_BLOCKIO    storBlockIo;
    STORDEV *           pStorDev;
    K2OS_THREAD_TOKEN   tokThread;
    char                threadName[K2OS_THREAD_NAME_BUFFER_CHARS];

//    Debug_Printf("STORMGR: BlockIo ifInstId %d arrived\n", aIfInstId);

    storBlockIo = K2OS_BlockIo_Attach(aIfInstId, K2OS_ACCESS_RW, K2OS_ACCESS_RW, sgStorMgrTokMailbox);
    if (NULL == storBlockIo)
    {
        Debug_Printf("STORMGR: Could not attach to blockio device by interface instance id (%08X)\n", K2OS_Thread_GetLastStatus());
        return;
    }

    pStorDev = (STORDEV *)K2OS_Heap_Alloc(sizeof(STORDEV));
    if (NULL == pStorDev)
    {
        Debug_Printf("STORMGR: Failed to allocate memory for storage device with blockio ifinstid %d\n", aIfInstId);
        K2OS_BlockIo_Detach(storBlockIo);
        return;
    }

    K2MEM_Zero(pStorDev, sizeof(STORDEV));
    pStorDev->mStorBlockIo = storBlockIo;
    pStorDev->mIfInstId = aIfInstId;

    K2OS_CritSec_Enter(&sgStorDevListSec);
    K2LIST_AddAtTail(&sgStorDevList, &pStorDev->ListLink);
    K2OS_CritSec_Leave(&sgStorDevListSec);

    pStorDev->mTokActionNotify = K2OS_Notify_Create(FALSE);

    if (NULL != pStorDev->mTokActionNotify)
    {
        K2ASC_PrintfLen(threadName, K2OS_THREAD_NAME_BUFFER_CHARS - 1, "StorMgr Device Iface %d", aIfInstId);
        threadName[K2OS_THREAD_NAME_BUFFER_CHARS - 1] = 0;
        tokThread = K2OS_Thread_Create(threadName, StorMgr_DevThread, pStorDev, NULL, &pStorDev->mThreadId);
        if (NULL != tokThread)
        {
            K2OS_Token_Destroy(tokThread);
            return;
        }

        K2OS_Token_Destroy(pStorDev->mTokActionNotify);
    }

    Debug_Printf("STORMGR: failed to start thread for blockio device with ifinstid %d\n", aIfInstId);

    K2OS_CritSec_Enter(&sgStorDevListSec);
    K2LIST_Remove(&sgStorDevList, &pStorDev->ListLink);
    K2OS_CritSec_Leave(&sgStorDevListSec);

    K2OS_BlockIo_Detach(storBlockIo);

    K2OS_Heap_Free(pStorDev);
}

void
StorMgr_BlockIo_ListLocked_Notify(
    STORDEV *   apStorDev,
    UINT32      aNotifyCode
)
{
    STORDEV_ACTION *    pAct;
    UINT32              v;
    K2OS_SIGNAL_TOKEN   tokNotify;

    pAct = (STORDEV_ACTION *)K2OS_Heap_Alloc(sizeof(STORDEV_ACTION));
    if (NULL == pAct)
        return;

    K2MEM_Zero(pAct, sizeof(STORDEV_ACTION));
    pAct->mAction = StorDev_Action_Media_Changed;

    tokNotify = apStorDev->mTokActionNotify;

    do {
        v = (UINT32)apStorDev->mpActions;
        pAct->mpNext = (STORDEV_ACTION *)v;
        K2_CpuWriteBarrier();
    } while (v != K2ATOMIC_CompareExchange((UINT32 volatile *)&apStorDev->mpActions, (UINT32)pAct, v));

    K2OS_Notify_Signal(tokNotify);
}

void
StorMgr_BlockIo_Departed(
    K2OS_IFINST_ID aIfInstId
)
{
    STORDEV_ACTION *    pAct;
    K2LIST_LINK *       pListLink;
    STORDEV *           pStorDev;
    UINT32              v;
    K2OS_SIGNAL_TOKEN   tokNotify;

    pAct = (STORDEV_ACTION *)K2OS_Heap_Alloc(sizeof(STORDEV_ACTION));
    if (NULL == pAct)
        return;

    K2OS_CritSec_Enter(&sgStorDevListSec);

    pListLink = sgStorDevList.mpHead;
    if (NULL != pListLink)
    {
        do {
            pStorDev = K2_GET_CONTAINER(STORDEV, pListLink, ListLink);
            if (pStorDev->mIfInstId == aIfInstId)
                break;
        } while (NULL != pListLink);
    }

    K2OS_CritSec_Leave(&sgStorDevListSec);

    if (NULL == pListLink)
    {
        K2OS_Heap_Free(pAct);
        return;
    }
    
    K2_ASSERT(NULL != pStorDev);

    K2MEM_Zero(pAct, sizeof(STORDEV_ACTION));
    pAct->mAction = StorDev_Action_BlockIo_Departed;

    tokNotify = pStorDev->mTokActionNotify;

    do {
        v = (UINT32)pStorDev->mpActions;
        pAct->mpNext = (STORDEV_ACTION *)v;
        K2_CpuWriteBarrier();
    } while (v != K2ATOMIC_CompareExchange((UINT32 volatile *)&pStorDev->mpActions, (UINT32)pAct, v));

    // pStorDev may be GONE as soon as action is latched.  we don't care if the following fails.
    K2OS_Notify_Signal(tokNotify);
}

void
StorMgr_Recv_Notify(
    UINT32 aArg0,
    UINT32 aNotifyCode,
    UINT32 aNotifyData
)
{
    //K2LIST_LINK *       pListLink;
//    STORDEV *           pStorDev;

    Debug_Printf("StorMgr_Recv_Notify(%08X,%08X,%08X)\n", aArg0, aNotifyCode, aNotifyData);
}

UINT32
StorMgr_Thread(
    void *apArg
)
{
    static K2_GUID128 const sBlockIoIfaceId = K2OS_IFACE_BLOCKIO_DEVICE;
    K2OS_IFSUBS_TOKEN   tokSubs;
    K2OS_MSG            msg;
    K2OS_WaitResult     waitResult;

    sgStorMgrTokMailbox = K2OS_Mailbox_Create();
    if (NULL == sgStorMgrTokMailbox)
    {
        Debug_Printf("***StorMgr thread coult not create a mailbox\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    tokSubs = K2OS_IfSubs_Create(sgStorMgrTokMailbox, K2OS_IFACE_CLASSCODE_STORAGE_DEVICE, &sBlockIoIfaceId, 12, FALSE, NULL);
    if (NULL == tokSubs)
    {
        Debug_Printf("***StorMgr thread coult not subscribe to blockio device interface pop changes\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    do {
        if (!K2OS_Thread_WaitOne(&waitResult, sgStorMgrTokMailbox, K2OS_TIMEOUT_INFINITE))
            break;
        K2_ASSERT(K2OS_Wait_Signalled_0 == waitResult);
        if (K2OS_Mailbox_Recv(sgStorMgrTokMailbox, &msg, 0))
        {
            if (msg.mType == K2OS_SYSTEM_MSGTYPE_IFINST)
            {
                if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE)
                {
                    // payload[0] = subscription context
                    // payload[1] = interface instance id
                    // payload[2] = process that published the interface
                    StorMgr_BlockIo_Arrived((K2OS_IFINST_ID)msg.mPayload[1]);
                }
                else if (msg.mShort == K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE)
                {
                    StorMgr_BlockIo_Departed((K2OS_IFINST_ID)msg.mPayload[1]);
                }
                else
                {
                    Debug_Printf("*** SysProc StorMgr received unexpected IFINST message (%04X)\n", msg.mShort);
                }
            }
            else if (msg.mType == K2OS_SYSTEM_MSGTYPE_RPC)
            {
                if (msg.mShort == K2OS_SYSTEM_MSG_RPC_SHORT_NOTIFY)
                {
                    StorMgr_Recv_Notify(msg.mPayload[0], msg.mPayload[1], msg.mPayload[2]);
                }
                else
                {
                    Debug_Printf("*** SysProc StorMgr received unexpected RPC message (%04X)\n", msg.mShort);
                }
            }
            else
            {
                Debug_Printf("*** SysProc StorMgr received unexpected message type (%04X)\n", msg.mType);
            }
        }

    } while (1);

    return 0;
}

void
StorMgr_Init(
    void
)
{
    if (!K2OS_CritSec_Init(&sgStorDevListSec))
    {
        Debug_Printf("STORMGR: Could not create cs for storage device list\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    K2LIST_Init(&sgStorDevList);

    sgGptRpcClass = K2OS_RpcServer_Register(&sgGptPartClassDef, 0);
    if (NULL == sgGptRpcClass)
    {
        Debug_Printf("STORMGR: Could not register gpt partition rpc object class\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    sgMbrRpcClass = K2OS_RpcServer_Register(&sgMbrPartClassDef, 0);
    if (NULL == sgMbrRpcClass)
    {
        Debug_Printf("STORMGR: Could not register mbr partition rpc object class\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }

    sgStorMgrTokThread = K2OS_Thread_Create("Storage Manager", StorMgr_Thread, NULL, NULL, &sgStorMgrThreadId);
    if (NULL == sgStorMgrTokThread)
    {
        Debug_Printf("***StorMgr thread coult not be started\n");
        K2OS_Process_Exit(K2OS_Thread_GetLastStatus());
    }
}

