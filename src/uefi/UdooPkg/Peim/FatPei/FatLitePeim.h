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
  Data structures for FAT recovery PEIM

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FAT_PEIM_H_
#define _FAT_PEIM_H_

#include <PiPei.h>

#include <Ppi/BlockIo.h>
#include <Ppi/BlockIo2.h>
#include <Ppi/DxeFvLoadPpi.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>

#include "FatLiteApi.h"
#include "FatLiteFmt.h"

//
// Definitions
//

#define PEI_FAT_CACHE_SIZE                            4
#define PEI_FAT_MAX_BLOCK_SIZE                        8192
#define FAT_MAX_FILE_NAME_LENGTH                      128
#define PEI_FAT_MAX_BLOCK_DEVICE                      64
#define PEI_FAT_MAX_BLOCK_IO_PPI                      32
#define PEI_FAT_MAX_VOLUME                            64

#define PEI_FAT_MEMMORY_PAGE_SIZE                     0x1000

//
// Data Structures
//
//
// The block device
//
typedef struct {

  UINT32                        BlockSize;
  UINT64                        LastBlock;
  UINT32                        IoAlign;
  BOOLEAN                       Logical;
  BOOLEAN                       PartitionChecked;

  //
  // Following fields only valid for logical device
  //
  CHAR8                         PartitionFlag[8];
  UINT64                        StartingPos;
  UINTN                         ParentDevNo;

  //
  // Following fields only valid for physical device
  //
  EFI_PEI_BLOCK_DEVICE_TYPE     DevType;
  UINT8                         InterfaceType;
  //
  // EFI_PEI_READ_BLOCKS         ReadFunc;
  //
  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *BlockIo;
  EFI_PEI_RECOVERY_BLOCK_IO2_PPI *BlockIo2;
  UINT8                          PhysicalDevNo;
} PEI_FAT_BLOCK_DEVICE;

//
// the Volume structure
//
typedef struct {

  UINTN         BlockDeviceNo;
  UINTN         VolumeNo;
  UINT64        VolumeSize;
  UINTN         MaxCluster;
  CHAR16        VolumeLabel[FAT_MAX_FILE_NAME_LENGTH];
  PEI_FAT_TYPE  FatType;
  UINT64        FatPos;
  UINT32        SectorSize;
  UINT32        ClusterSize;
  UINT64        FirstClusterPos;
  UINT64        RootDirPos;
  UINT32        RootEntries;
  UINT32        RootDirCluster;

} PEI_FAT_VOLUME;

//
// File instance
//
typedef struct {

  PEI_FAT_VOLUME  *Volume;
  CHAR16          FileName[FAT_MAX_FILE_NAME_LENGTH];

  BOOLEAN         IsFixedRootDir;

  UINT32          StartingCluster;
  UINT32          CurrentPos;
  UINT32          StraightReadAmount;
  UINT32          CurrentCluster;

  UINT8           Attributes;
  UINT32          FileSize;

} PEI_FAT_FILE;

//
// Cache Buffer
//
typedef struct {

  BOOLEAN Valid;
  UINTN   BlockDeviceNo;
  UINT64  Lba;
  UINT32  Lru;
  UINT64  Buffer[PEI_FAT_MAX_BLOCK_SIZE / 8];
  UINTN   Size;

} PEI_FAT_CACHE_BUFFER;

//
// Private Data.
// This structure abstracts the whole memory usage in FAT PEIM.
// The entry point routine will get a chunk of memory (by whatever
// means) whose size is sizeof(PEI_FAT_PRIVATE_DATA), which is clean
// in both 32 and 64 bit environment. The boundary of the memory chunk
// should be 64bit aligned.
//
#define PEI_FAT_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('p', 'f', 'a', 't')

typedef struct {

  UINTN                               Signature;
  EFI_PEI_DXE_FV_LOAD_PPI             DxeFvLoaderPpi;
  EFI_PEI_PPI_DESCRIPTOR              PpiDescriptor;
  EFI_PEI_NOTIFY_DESCRIPTOR           NotifyDescriptor[2];

  UINT8                               UnicodeCaseMap[0x300];
  CHAR8                               *EngUpperMap;
  CHAR8                               *EngLowerMap;
  CHAR8                               *EngInfoMap;

  UINT64                              BlockData[PEI_FAT_MAX_BLOCK_SIZE / 8];
  UINTN                               BlockDeviceCount;
  PEI_FAT_BLOCK_DEVICE                BlockDevice[PEI_FAT_MAX_BLOCK_DEVICE];
  UINTN                               VolumeCount;
  PEI_FAT_VOLUME                      Volume[PEI_FAT_MAX_VOLUME];
  PEI_FAT_FILE                        File;
  PEI_FAT_CACHE_BUFFER                CacheBuffer[PEI_FAT_CACHE_SIZE];

} PEI_FAT_PRIVATE_DATA;

#define PEI_FAT_PRIVATE_DATA_FROM_THIS(a) \
  CR (a,  PEI_FAT_PRIVATE_DATA, DxeFvLoaderPpi, PEI_FAT_PRIVATE_DATA_SIGNATURE)

//
// Extract INT32 from char array
//
#define UNPACK_INT32(a) \
  (INT32) ((((UINT8 *) a)[0] << 0) | (((UINT8 *) a)[1] << 8) | (((UINT8 *) a)[2] << 16) | (((UINT8 *) a)[3] << 24))

//
// Extract UINT32 from char array
//
#define UNPACK_UINT32(a) \
  (UINT32) ((((UINT8 *) a)[0] << 0) | (((UINT8 *) a)[1] << 8) | (((UINT8 *) a)[2] << 16) | (((UINT8 *) a)[3] << 24))


//
// API functions
//

EFI_STATUS
EFIAPI
GetDxeFvFileInfo (
  IN  EFI_PEI_SERVICES **       PeiServices,
  IN  EFI_PEI_DXE_FV_LOAD_PPI * This,
  OUT UINTN *                   Size
  );

EFI_STATUS
EFIAPI
LoadDxeFvFile (
  IN EFI_PEI_SERVICES **        PeiServices,
  IN EFI_PEI_DXE_FV_LOAD_PPI *  This,
  OUT VOID *                    Buffer
  );


/**
  This version is different from the version in Unicode collation
  protocol in that this version strips off trailing blanks.
  Converts an 8.3 FAT file name using an OEM character set
  to a Null-terminated Unicode string.
  Here does not expand DBCS FAT chars.

  @param  FatSize           The size of the string Fat in bytes.
  @param  Fat               A pointer to a Null-terminated string that contains
                            an 8.3 file name using an OEM character set.
  @param  Str               A pointer to a Null-terminated Unicode string. The
                            string must be allocated in advance to hold FatSize
                            Unicode characters

**/
VOID
EngFatToStr (
  IN UINTN                            FatSize,
  IN CHAR8                            *Fat,
  OUT CHAR16                          *Str
  );


/**
  Performs a case-insensitive comparison of two Null-terminated Unicode strings.

  @param  PrivateData       Global memory map for accessing global variables
  @param  Str1              First string to perform case insensitive comparison.
  @param  Str2              Second string to perform case insensitive comparison.

**/
BOOLEAN
EngStriColl (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN CHAR16                 *Str1,
  IN CHAR16                 *Str2
  );


/**
  Reads a block of data from the block device by calling
  underlying Block I/O service.

  @param  PrivateData       Global memory map for accessing global variables
  @param  BlockDeviceNo     The index for the block device number.
  @param  Lba               The logic block address to read data from.
  @param  BufferSize        The size of data in byte to read.
  @param  Buffer            The buffer of the

  @retval EFI_DEVICE_ERROR  The specified block device number exceeds the maximum
                            device number.
  @retval EFI_DEVICE_ERROR  The maximum address has exceeded the maximum address
                            of the block device.

**/
EFI_STATUS
FatReadBlock (
  IN  PEI_FAT_PRIVATE_DATA   *PrivateData,
  IN  UINTN                  BlockDeviceNo,
  IN  EFI_PEI_LBA            Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  );


/**
  Check if there is a valid FAT in the corresponding Block device
  of the volume and if yes, fill in the relevant fields for the
  volume structure. Note there should be a valid Block device number
  already set.

  @param  PrivateData            Global memory map for accessing global
                                 variables.
  @param  Volume                 On input, the BlockDeviceNumber field of the
                                 Volume  should be a valid value. On successful
                                 output, all  fields except the VolumeNumber
                                 field is initialized.

  @retval EFI_SUCCESS            A FAT is found and the volume structure is
                                 initialized.
  @retval EFI_NOT_FOUND          There is no FAT on the corresponding device.
  @retval EFI_DEVICE_ERROR       There is something error while accessing device.

**/
EFI_STATUS
FatGetBpbInfo (
  IN      PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN OUT  PEI_FAT_VOLUME        *Volume
  );


/**
  Gets the next cluster in the cluster chain.

  @param  PrivateData            Global memory map for accessing global variables
  @param  Volume                 The volume
  @param  Cluster                The cluster
  @param  NextCluster            The cluster number of the next cluster

  @retval EFI_SUCCESS            The address is got
  @retval EFI_INVALID_PARAMETER  ClusterNo exceeds the MaxCluster of the volume.
  @retval EFI_DEVICE_ERROR       Read disk error

**/
EFI_STATUS
FatGetNextCluster (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  PEI_FAT_VOLUME        *Volume,
  IN  UINT32                Cluster,
  OUT UINT32                *NextCluster
  );


/**
  Disk reading.

  @param  PrivateData       the global memory map;
  @param  BlockDeviceNo     the block device to read;
  @param  StartingAddress   the starting address.
  @param  Size              the amount of data to read.
  @param  Buffer            the buffer holding the data

  @retval EFI_SUCCESS       The function completed successfully.
  @retval EFI_DEVICE_ERROR  Something error.

**/
EFI_STATUS
FatReadDisk (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  UINTN                 BlockDeviceNo,
  IN  UINT64                StartingAddress,
  IN  UINTN                 Size,
  OUT VOID                  *Buffer
  );


/**
  Set a file's CurrentPos and CurrentCluster, then compute StraightReadAmount.

  @param  PrivateData            the global memory map
  @param  File                   the file
  @param  Pos                    the Position which is offset from the file's
                                 CurrentPos

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Pos is beyond file's size.
  @retval EFI_DEVICE_ERROR       Something error while accessing media.

**/
EFI_STATUS
FatSetFilePos (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  PEI_FAT_FILE          *File,
  IN  UINT32                Pos
  );


/**
  Reads file data. Updates the file's CurrentPos.

  @param  PrivateData            Global memory map for accessing global variables
  @param  File                   The file.
  @param  Size                   The amount of data to read.
  @param  Buffer                 The buffer storing the data.

  @retval EFI_SUCCESS            The data is read.
  @retval EFI_INVALID_PARAMETER  File is invalid.
  @retval EFI_DEVICE_ERROR       Something error while accessing media.

**/
EFI_STATUS
FatReadFile (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  PEI_FAT_FILE          *File,
  IN  UINTN                 Size,
  OUT VOID                  *Buffer
  );


/**
  This function reads the next item in the parent directory and
  initializes the output parameter SubFile (CurrentPos is initialized to 0).
  The function updates the CurrentPos of the parent dir to after the item read.
  If no more items were found, the function returns EFI_NOT_FOUND.

  @param  PrivateData            Global memory map for accessing global variables
  @param  ParentDir              The parent directory.
  @param  SubFile                The File structure containing the sub file that
                                 is caught.

  @retval EFI_SUCCESS            The next sub file is obtained.
  @retval EFI_INVALID_PARAMETER  The ParentDir is not a directory.
  @retval EFI_NOT_FOUND          No more sub file exists.
  @retval EFI_DEVICE_ERROR       Something error while accessing media.

**/
EFI_STATUS
FatReadNextDirectoryEntry (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  PEI_FAT_FILE          *ParentDir,
  OUT PEI_FAT_FILE          *SubFile
  );


/**
  This function finds partitions (logical devices) in physical block devices.

  @param  PrivateData       Global memory map for accessing global variables.

**/
VOID
FatFindPartitions (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData
  );

#endif // _FAT_PEIM_H_
