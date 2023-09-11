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
/** @file
  Routines supporting partition discovery and
  logical device reading

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi/UefiGpt.h>
#include <Guid/Gpt.h>

#include "FatLitePeim.h"

/**
  This function finds Mbr partitions. Main algorithm
  is ported from DXE partition driver.

  @param  PrivateData       The global memory map
  @param  ParentBlockDevNo  The parent block device

  @retval TRUE              New partitions are detected and logical block devices
                            are  added to block device array
  @retval FALSE             No New partitions are added;

**/
BOOLEAN
FatFindGptPartitionsOnDevice(
    IN  PEI_FAT_PRIVATE_DATA *PrivateData,
    IN  UINTN                ParentBlockDevNo
)
{
    static const EFI_GUID BasicDataPartitionGuid =
    { 0xebd0a0a2, 0xb9e5, 0x4433, { 0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7 } };

    EFI_STATUS                      Status;
    EFI_PARTITION_TABLE_HEADER *    GptHeader;
    BOOLEAN                         Found;
    PEI_FAT_BLOCK_DEVICE  *         ParentBlockDev;
    PEI_FAT_BLOCK_DEVICE  *         BlockDev;
    UINT32                          checkCrc;
    EFI_LBA                         FirstUsable;
    EFI_LBA                         LastUsable;
    EFI_LBA                         CurrentLBA;
    UINT32                          PartitionEntriesLeft;
    UINT32                          EntriesLeftInBlock;
    UINT32                          PartitionEntryBytes;
    UINT32                          PartitionEntryBlockCount;
    UINT32                          PartitionEntriesPerBlock;
    EFI_PARTITION_ENTRY             PartEntry;
    UINT8 *                         Buffer;
    UINT8 *                         CopyOut;

    //
    // return TRUE if we found a fat partition and registered it
    //
    if (ParentBlockDevNo > PEI_FAT_MAX_BLOCK_DEVICE - 1) {
        return FALSE;
    }

    ParentBlockDev = &(PrivateData->BlockDevice[ParentBlockDevNo]);
    GptHeader = (EFI_PARTITION_TABLE_HEADER *)PrivateData->BlockData;
    ParentBlockDev->PartitionChecked = TRUE;

    Status = FatReadBlock(
        PrivateData,
        ParentBlockDevNo,
        PRIMARY_PART_HEADER_LBA,
        ParentBlockDev->BlockSize,
        GptHeader
    );

    if (EFI_ERROR(Status) || 
        (GptHeader->Header.Signature != EFI_PTAB_HEADER_ID) ||
        (GptHeader->Header.HeaderSize > ParentBlockDev->BlockSize) ||
        (GptHeader->MyLBA != PRIMARY_PART_HEADER_LBA) ||
        (0 == GptHeader->NumberOfPartitionEntries) ||
        (0 == GptHeader->SizeOfPartitionEntry))
    {
//        DEBUG((EFI_D_INFO, "***GPT Not found or invalid\n"));
        return FALSE;
    }

    checkCrc = GptHeader->Header.CRC32;
    GptHeader->Header.CRC32 = 0;
    if (checkCrc != CalculateCrc32(GptHeader, GptHeader->Header.HeaderSize))
    {
//        DEBUG((EFI_D_INFO, "***GPT header crc failed\n"));
        return FALSE;
    }

    PartitionEntryBytes = GptHeader->SizeOfPartitionEntry;
    PartitionEntriesPerBlock = ParentBlockDev->BlockSize / PartitionEntryBytes;
    if (0 == PartitionEntriesPerBlock)
    {
//        DEBUG((EFI_D_INFO, "***Size of partition entry is invalid\n"));
        return FALSE;
    }
    PartitionEntriesLeft = GptHeader->NumberOfPartitionEntries;
    PartitionEntryBlockCount = (PartitionEntriesLeft + (PartitionEntriesPerBlock - 1)) / PartitionEntriesPerBlock;

    if ((GptHeader->FirstUsableLBA > GptHeader->LastUsableLBA) ||
        (GptHeader->LastUsableLBA > ParentBlockDev->LastBlock) ||
        (GptHeader->PartitionEntryLBA > (ParentBlockDev->LastBlock - (PartitionEntryBlockCount - 1))))
    {
//        DEBUG((EFI_D_INFO, "***GPT header contains invalid values\n"));
        return FALSE;
    }

    FirstUsable = GptHeader->FirstUsableLBA;
    LastUsable = GptHeader->LastUsableLBA;
    CurrentLBA = GptHeader->PartitionEntryLBA;
    Found = FALSE;

    Buffer = (UINT8 *)PrivateData->BlockData;

    do {
        Status = FatReadBlock(
            PrivateData,
            ParentBlockDevNo,
            CurrentLBA,
            ParentBlockDev->BlockSize,
            Buffer
        );
        if (EFI_ERROR(Status))
        {
//            DEBUG((EFI_D_ERROR, "***FatReadBlock(%d) failed (%r)\n", (UINTN)CurrentLBA, Status));
            break;
        }
        
        EntriesLeftInBlock = PartitionEntryBlockCount;
        CopyOut = Buffer;
        do {
            CopyMem(&PartEntry, CopyOut, sizeof(EFI_PARTITION_ENTRY));
            CopyOut += PartitionEntryBytes;
            if ((PartEntry.StartingLBA >= FirstUsable) &&
                (PartEntry.EndingLBA <= LastUsable))
            {
                if (CompareGuid(&PartEntry.PartitionTypeGUID, 
//                    &gEfiPartTypeSystemPartGuid
                    &BasicDataPartitionGuid
                )) 
                {
                    //
                    // Register this partition
                    //
                    if (PrivateData->BlockDeviceCount < PEI_FAT_MAX_BLOCK_DEVICE) 
                    {
                        Found = TRUE;

                        BlockDev = &(PrivateData->BlockDevice[PrivateData->BlockDeviceCount]);

                        BlockDev->BlockSize = ParentBlockDev->BlockSize;
                        BlockDev->StartingPos = MultU64x32(PartEntry.StartingLBA, ParentBlockDev->BlockSize);
                        BlockDev->LastBlock = MultU64x32(PartEntry.EndingLBA, ParentBlockDev->BlockSize);
                        BlockDev->IoAlign = ParentBlockDev->IoAlign;
                        BlockDev->Logical = TRUE;
                        BlockDev->PartitionChecked = TRUE;
                        BlockDev->ParentDevNo = ParentBlockDevNo;

                        PrivateData->BlockDeviceCount++;
                    }
                }
            }
            if (0 == --PartitionEntriesLeft)
                break;
        } while (--EntriesLeftInBlock);
        CurrentLBA++;
    } while (0 != PartitionEntriesLeft);

    return Found;
}


/**
  This function finds partitions (logical devices) in physical block devices.

  @param  PrivateData       Global memory map for accessing global variables.

**/
VOID
FatFindPartitions(
    IN  PEI_FAT_PRIVATE_DATA  *PrivateData
)
{
    BOOLEAN Found;
    UINTN   Index;

    do {
        Found = FALSE;

        for (Index = 0; Index < PrivateData->BlockDeviceCount; Index++) {
            if (!PrivateData->BlockDevice[Index].PartitionChecked) {
                Found = FatFindGptPartitionsOnDevice(PrivateData, Index);
            }
        }
    } while (Found && PrivateData->BlockDeviceCount <= PEI_FAT_MAX_BLOCK_DEVICE);
}
