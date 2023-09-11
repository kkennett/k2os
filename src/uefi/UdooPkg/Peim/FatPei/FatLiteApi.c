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
  FAT recovery PEIM entry point, Ppi Functions and FAT Api functions.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FatLitePeim.h"

PEI_FAT_PRIVATE_DATA  *mPrivateData = NULL;

/**
  BlockIo installation nofication function. Find out all the current BlockIO
  PPIs in the system and add them into private data. Assume there is

  @param  PeiServices             General purpose services available to every
                                  PEIM.
  @param  NotifyDescriptor        The typedef structure of the notification
                                  descriptor. Not used in this function.
  @param  Ppi                     The typedef structure of the PPI descriptor.
                                  Not used in this function.

  @retval EFI_SUCCESS             The function completed successfully.

**/
EFI_STATUS
EFIAPI
BlockIoNotifyEntry (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );


/**
  Discover all the block I/O devices to find the FAT volume.

  @param  PrivateData             Global memory map for accessing global
                                  variables.
  @param  BlockIo2                Boolean to show whether using BlockIo2 or BlockIo

  @retval EFI_SUCCESS             The function completed successfully.

**/
EFI_STATUS
UpdateBlocksAndVolumes(
    IN OUT PEI_FAT_PRIVATE_DATA            *PrivateData
)
{
    static BOOLEAN                 EverFoundFatVolume = FALSE;
    static EFI_PEI_PPI_DESCRIPTOR  SysPartFatVolPpi = {
        EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
        &gEfiPeiSysPartFatAvailGuid,
        NULL
    };

    EFI_STATUS                     Status;
    EFI_PEI_PPI_DESCRIPTOR         *TempPpiDescriptor;
    UINTN                          BlockIoPpiInstance;
    EFI_PEI_RECOVERY_BLOCK_IO_PPI  *BlockIoPpi;
    UINTN                          NumberBlockDevices;
    UINTN                          Index;
    EFI_PEI_BLOCK_IO_MEDIA         Media;
    PEI_FAT_VOLUME                 Volume;
    EFI_PEI_SERVICES               **PeiServices;

    PeiServices = (EFI_PEI_SERVICES **)GetPeiServicesTablePointer();
    BlockIoPpi = NULL;
    //
    // Clean up caches
    //
    for (Index = 0; Index < PEI_FAT_CACHE_SIZE; Index++) {
        PrivateData->CacheBuffer[Index].Valid = FALSE;
    }

    PrivateData->BlockDeviceCount = 0;

    //
    // Find out all Block Io Ppi instances within the system
    // Assuming all device Block Io Peims are dispatched already
    //
    for (BlockIoPpiInstance = 0; BlockIoPpiInstance < PEI_FAT_MAX_BLOCK_IO_PPI; BlockIoPpiInstance++) {
        Status = PeiServicesLocatePpi(
            &gEfiPeiVirtualBlockIoPpiGuid,
            BlockIoPpiInstance,
            &TempPpiDescriptor,
            (VOID **)&BlockIoPpi
        );
        if (EFI_ERROR(Status)) {
            //
            // Done with all Block Io Ppis
            //
            break;
        }

        Status = BlockIoPpi->GetNumberOfBlockDevices(
            PeiServices,
            BlockIoPpi,
            &NumberBlockDevices
        );
        if (EFI_ERROR(Status)) {
            continue;
        }

        for (Index = 1; Index <= NumberBlockDevices && PrivateData->BlockDeviceCount < PEI_FAT_MAX_BLOCK_DEVICE; Index++) {

            Status = BlockIoPpi->GetBlockDeviceMediaInfo(
                PeiServices,
                BlockIoPpi,
                Index,
                &Media
            );
            if (EFI_ERROR(Status) || !Media.MediaPresent) {
                continue;
            }
            PrivateData->BlockDevice[PrivateData->BlockDeviceCount].BlockIo = BlockIoPpi;
            PrivateData->BlockDevice[PrivateData->BlockDeviceCount].DevType = Media.DeviceType;
            PrivateData->BlockDevice[PrivateData->BlockDeviceCount].LastBlock = Media.LastBlock;
            PrivateData->BlockDevice[PrivateData->BlockDeviceCount].BlockSize = (UINT32)Media.BlockSize;

            PrivateData->BlockDevice[PrivateData->BlockDeviceCount].IoAlign = 0;
            //
            // Not used here
            //
            PrivateData->BlockDevice[PrivateData->BlockDeviceCount].Logical = FALSE;
            PrivateData->BlockDevice[PrivateData->BlockDeviceCount].PartitionChecked = FALSE;

            PrivateData->BlockDevice[PrivateData->BlockDeviceCount].PhysicalDevNo = (UINT8)Index;
            PrivateData->BlockDeviceCount++;
        }
    }
    //
    // Find out all logical devices
    //
    FatFindPartitions(PrivateData);

    //
    // Build up file system volume array
    //
    PrivateData->VolumeCount = 0;
    for (Index = 0; Index < PrivateData->BlockDeviceCount; Index++) {
        Volume.BlockDeviceNo = Index;
        Status = FatGetBpbInfo(PrivateData, &Volume);
        if (Status == EFI_SUCCESS) {
            //
            // Add the detected volume to the volume array
            //
            CopyMem(
                (UINT8 *)&(PrivateData->Volume[PrivateData->VolumeCount]),
                (UINT8 *)&Volume,
                sizeof(PEI_FAT_VOLUME)
            );
            PrivateData->VolumeCount += 1;
            if (PrivateData->VolumeCount >= PEI_FAT_MAX_VOLUME) {
                break;
            }
            if (!EverFoundFatVolume)
            {
                EverFoundFatVolume = TRUE;
                PeiServicesInstallPpi(&SysPartFatVolPpi);
            }
        }
    }

    return EFI_SUCCESS;
}


/**
  BlockIo installation notification function. Find out all the current BlockIO
  PPIs in the system and add them into private data. Assume there is

  @param  PeiServices             General purpose services available to every
                                  PEIM.
  @param  NotifyDescriptor        The typedef structure of the notification
                                  descriptor. Not used in this function.
  @param  Ppi                     The typedef structure of the PPI descriptor.
                                  Not used in this function.

  @retval EFI_SUCCESS             The function completed successfully.

**/
EFI_STATUS
EFIAPI
BlockIoNotifyEntry(
    IN EFI_PEI_SERVICES           **PeiServices,
    IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
    IN VOID                       *Ppi
)
{
    UpdateBlocksAndVolumes(mPrivateData);
    return EFI_SUCCESS;
}


/**
  Installs the Device Recovery Module PPI, Initialize BlockIo Ppi
  installation notification

  @param  FileHandle              Handle of the file being invoked. Type
                                  EFI_PEI_FILE_HANDLE is defined in
                                  FfsFindNextFile().
  @param  PeiServices             Describes the list of possible PEI Services.

  @retval EFI_SUCCESS             The entry point was executed successfully.
  @retval EFI_OUT_OF_RESOURCES    There is no enough memory to complete the
                                  operations.

**/
EFI_STATUS
EFIAPI
FatPeimEntry(
    IN EFI_PEI_FILE_HANDLE       FileHandle,
    IN CONST EFI_PEI_SERVICES    **PeiServices
)
{
    EFI_STATUS            Status;
    EFI_PHYSICAL_ADDRESS  Address;
    PEI_FAT_PRIVATE_DATA  *PrivateData;

    Status = PeiServicesRegisterForShadow(FileHandle);
    if (!EFI_ERROR(Status)) {
        return Status;
    }

    Status = PeiServicesAllocatePages(
        EfiBootServicesCode,
        (sizeof(PEI_FAT_PRIVATE_DATA) - 1) / PEI_FAT_MEMMORY_PAGE_SIZE + 1,
        &Address
    );
    if (EFI_ERROR(Status)) {
        return EFI_OUT_OF_RESOURCES;
    }

    PrivateData = (PEI_FAT_PRIVATE_DATA *)(UINTN)Address;

    //
    // Initialize Private Data (to zero, as is required by subsequent operations)
    //
    ZeroMem((UINT8 *)PrivateData, sizeof(PEI_FAT_PRIVATE_DATA));

    PrivateData->Signature = PEI_FAT_PRIVATE_DATA_SIGNATURE;

    //
    // Set up PPI to install when we find the Dxe FV file
    //
    PrivateData->DxeFvLoaderPpi.GetInfo = GetDxeFvFileInfo;
    PrivateData->DxeFvLoaderPpi.Load = LoadDxeFvFile;
    PrivateData->PpiDescriptor.Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
    PrivateData->PpiDescriptor.Guid = &gEfiPeiDxeFvLoadPpiGuid;
    PrivateData->PpiDescriptor.Ppi = &PrivateData->DxeFvLoaderPpi;

    Status = PeiServicesInstallPpi(&PrivateData->PpiDescriptor);
    if (EFI_ERROR(Status)) {
        return EFI_OUT_OF_RESOURCES;
    }

    //
    // Other initializations
    //
    PrivateData->BlockDeviceCount = 0;
    UpdateBlocksAndVolumes(PrivateData);

    //
    // PrivateData is allocated now, set it to the module variable
    //
    mPrivateData = PrivateData;

    //
    // Installs Block Io Ppi notification function
    //
    PrivateData->NotifyDescriptor[0].Flags = EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
    PrivateData->NotifyDescriptor[0].Guid = &gEfiPeiVirtualBlockIoPpiGuid;
    PrivateData->NotifyDescriptor[0].Notify = BlockIoNotifyEntry;
    Status = PeiServicesNotifyPpi(&PrivateData->NotifyDescriptor[0]);
    if (EFI_ERROR(Status)) {
        return EFI_OUT_OF_RESOURCES;
    }

    return Status;
}

EFI_STATUS
FindDxeFvFile(
    IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
    IN  UINTN                 VolumeIndex,
    IN  CHAR16                *FileName,
    OUT PEI_FILE_HANDLE       *Handle
)
{
    EFI_STATUS    Status;
    PEI_FAT_FILE  Parent;
    PEI_FAT_FILE  *File;

    File = &PrivateData->File;

    //
    // VolumeIndex must be less than PEI_FAT_MAX_VOLUME because PrivateData->VolumeCount
    // cannot be larger than PEI_FAT_MAX_VOLUME when detecting volume containing the target file
    //
    if (VolumeIndex >= PEI_FAT_MAX_VOLUME)
    {
        return EFI_INVALID_PARAMETER;
    }

    //
    // Construct root directory file
    //
    ZeroMem(&Parent, sizeof(PEI_FAT_FILE));
    Parent.IsFixedRootDir = (BOOLEAN)((PrivateData->Volume[VolumeIndex].FatType == Fat32) ? FALSE : TRUE);
    Parent.Attributes = FAT_ATTR_DIRECTORY;
    Parent.CurrentPos = 0;
    Parent.CurrentCluster = Parent.IsFixedRootDir ? 0 : PrivateData->Volume[VolumeIndex].RootDirCluster;
    Parent.StartingCluster = Parent.CurrentCluster;
    Parent.Volume = &PrivateData->Volume[VolumeIndex];

    Status = FatSetFilePos(PrivateData, &Parent, 0);
    if (EFI_ERROR(Status)) {
        return EFI_DEVICE_ERROR;
    }
    //
    // Search for dxe fv in root directory
    //
    Status = FatReadNextDirectoryEntry(PrivateData, &Parent, File);
    while (Status == EFI_SUCCESS) {
        //
        // Compare whether the file name is the requested one
        //
        if (EngStriColl(PrivateData, FileName, File->FileName)) {
            break;
        }

        Status = FatReadNextDirectoryEntry(PrivateData, &Parent, File);
    }

    if (EFI_ERROR(Status)) {
        return EFI_NOT_FOUND;
    }

    //
    // Get the dxe fv file, set its file position to 0.
    //
    if (File->StartingCluster != 0) {
        Status = FatSetFilePos(PrivateData, File, 0);
    }

    *Handle = File;

    return EFI_SUCCESS;

}

EFI_STATUS
GetDxeFvFile(
    IN  PEI_FAT_PRIVATE_DATA  * PrivateData,
    OUT PEI_FAT_FILE **         FatFile
)
{
    EFI_STATUS  Status;
    UINTN       Index;

    for (Index = 0; Index < PrivateData->VolumeCount; Index++)
    {
        Status = FindDxeFvFile(PrivateData, Index, L"BOARDDXE.FV", (PEI_FILE_HANDLE *)FatFile);
        if (!EFI_ERROR(Status))
            break;
    }
    if (Index == PrivateData->VolumeCount)
    {
        return EFI_NOT_FOUND;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetDxeFvFileInfo(
    IN  EFI_PEI_SERVICES **       PeiServices,
    IN  EFI_PEI_DXE_FV_LOAD_PPI * This,
    OUT UINTN *                   Size
)
{
    PEI_FAT_PRIVATE_DATA  * PrivateData;
    EFI_STATUS              Status;
    PEI_FAT_FILE *          FatFile;

    PrivateData = PEI_FAT_PRIVATE_DATA_FROM_THIS(This);

    Status = GetDxeFvFile(PrivateData, &FatFile);
    if (EFI_ERROR(Status))
        return Status;

    *Size = FatFile->FileSize;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LoadDxeFvFile(
    IN EFI_PEI_SERVICES **        PeiServices,
    IN EFI_PEI_DXE_FV_LOAD_PPI *  This,
    OUT VOID *                    Buffer
)
{
    PEI_FAT_PRIVATE_DATA  * PrivateData;
    EFI_STATUS              Status;
    PEI_FAT_FILE *          FatFile;

    PrivateData = PEI_FAT_PRIVATE_DATA_FROM_THIS(This);

    Status = GetDxeFvFile(PrivateData, &FatFile);
    if (EFI_ERROR(Status))
        return Status;

    return FatReadFile(
        PrivateData,
        (PEI_FILE_HANDLE)FatFile,
        (UINTN)FatFile->FileSize,
        Buffer
    );
}
