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
#ifndef __K2UEFI_H
#define __K2UEFI_H

#include <k2basetype.h>
#include <spec/fat.h>
#include <spec/gpt.h>

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//
// these structures mirror EFI structures but do not require EFI headers
//
//------------------------------------------------------------------------
//

#define K2EFIAPI K2_CALLCONV_CALLERCLEANS

#if K2_TARGET_ARCH_IS_32BIT
typedef INT32   INTN;
typedef UINT32  UINTN;
#else
typedef INT64   INTN;
typedef UINT64  UINTN;
#endif

typedef UINT8   K2EFIBOOL;
typedef UINTN   K2EFI_STATUS;
typedef UINT64  K2EFI_PHYSICAL_ADDRESS;
typedef UINT64  K2EFI_VIRTUAL_ADDRESS;
typedef UINT16  CHAR16;
typedef UINT64  K2EFI_LBA;

typedef enum _K2EFI_MEMORY_TYPE K2EFI_MEMORY_TYPE;
enum _K2EFI_MEMORY_TYPE
{
    K2EFI_MEMTYPE_Reserved,          // 0
    K2EFI_MEMTYPE_Loader_Code,       // 1
    K2EFI_MEMTYPE_Loader_Data,       // 2
    K2EFI_MEMTYPE_Boot_Code,         // 3
    K2EFI_MEMTYPE_Boot_Data,         // 4
    K2EFI_MEMTYPE_Run_Code,          // 5
    K2EFI_MEMTYPE_Run_Data,          // 6
    K2EFI_MEMTYPE_Conventional,      // 7
    K2EFI_MEMTYPE_Unusable,          // 8
    K2EFI_MEMTYPE_ACPI_Reclaim,      // 9
    K2EFI_MEMTYPE_ACPI_NVS,          // 10
    K2EFI_MEMTYPE_MappedIO,          // 11
    K2EFI_MEMTYPE_MappedIOPort,      // 12
    K2EFI_MEMTYPE_PAL,               // 13
    K2EFI_MEMTYPE_NV,                // 14
    K2EFI_MEMTYPE_COUNT              // 15
};

typedef struct _K2EFI_TABLE_HEADER K2EFI_TABLE_HEADER;
struct _K2EFI_TABLE_HEADER
{
    UINT64  Signature;
    UINT32  Revision;
    UINT32  HeaderSize;
    UINT32  CRC32;
    UINT32  Reserved;
};

typedef struct _K2EFI_CONFIG_TABLE K2EFI_CONFIG_TABLE;
struct _K2EFI_CONFIG_TABLE
{
    K2_GUID128  VendorGuid;
    void *      VendorTable;
};

typedef struct _K2EFI_TIME K2EFI_TIME;
struct _K2EFI_TIME 
{
    UINT16  Year;
    UINT8   Month;
    UINT8   Day;
    UINT8   Hour;
    UINT8   Minute;
    UINT8   Second;
    UINT8   Pad1;
    UINT32  Nanosecond;
    INT16   TimeZone;
    UINT8   Daylight;
    UINT8   Pad2;
};

typedef struct _K2EFI_TIME_CAPABILITIES K2EFI_TIME_CAPABILITIES;
struct _K2EFI_TIME_CAPABILITIES
{
    UINT32      Resolution;
    UINT32      Accuracy;
    K2EFIBOOL   SetsToZero;
};

#define K2EFI_MEMORYFLAG_UC              0x0000000000000001ULL
#define K2EFI_MEMORYFLAG_WC              0x0000000000000002ULL
#define K2EFI_MEMORYFLAG_WT              0x0000000000000004ULL
#define K2EFI_MEMORYFLAG_WB              0x0000000000000008ULL
#define K2EFI_MEMORYFLAG_UCE             0x0000000000000010ULL
#define K2EFI_MEMORYFLAG_WP              0x0000000000001000ULL
#define K2EFI_MEMORYFLAG_RP              0x0000000000002000ULL
#define K2EFI_MEMORYFLAG_XP              0x0000000000004000ULL
#define K2EFI_MEMORYFLAG_NV              0x0000000000008000ULL
#define K2EFI_MEMORYFLAG_MORE_RELIABLE   0x0000000000010000ULL
#define K2EFI_MEMORYFLAG_RO              0x0000000000020000ULL
#define K2EFI_MEMORYFLAG_PROP_MASK       0x00000000000FFFFFULL
#define K2EFI_MEMORYFLAG_RUNTIME         0x8000000000000000ULL
#define K2EFI_MEMORY_DESCRIPTOR_VERSION  1

typedef struct _K2EFI_MEMORY_DESCRIPTOR K2EFI_MEMORY_DESCRIPTOR;
struct _K2EFI_MEMORY_DESCRIPTOR
{
    UINT32                  Type;
    K2EFI_PHYSICAL_ADDRESS  PhysicalStart;
    K2EFI_VIRTUAL_ADDRESS   VirtualStart;
    UINT64                  NumberOfPages;
    UINT64                  Attribute;
};

typedef struct _K2EFI_CAPSULE_HEADER K2EFI_CAPSULE_HEADER;
struct _K2EFI_CAPSULE_HEADER
{
    K2_GUID128  CapsuleGuid;
    UINT32      HeaderSize;
    UINT32      Flags;
    UINT32      CapsuleImageSize;
};

typedef struct _K2EFI_FIRMWARE_VOLUME_HEADER K2EFI_FIRMWARE_VOLUME_HEADER;
struct _K2EFI_FIRMWARE_VOLUME_HEADER
{
    UINT8       ZeroVector[16];
    K2_GUID128  FileSystemGuid;
    UINT64      FvLength;
    UINT32      Signature;
    UINT32      Attributes;
    UINT16      HeaderLength;
    UINT16      Checksum;
    UINT16      ExtHeaderOffset;
    UINT8       Reserved[1];
    UINT8       Revision;
//  block map follows
};

#define K2EFI_FV_FILETYPE_SECURITY_CORE  0x03
#define K2EFI_SECTION_TE                 0x12

typedef struct _K2EFI_FFS_FILE_HEADER K2EFI_FFS_FILE_HEADER;
struct _K2EFI_FFS_FILE_HEADER
{
    K2_GUID128  Name;
    UINT16      IntegrityCheck;
    UINT8       Type;
    UINT8       Attributes;
    UINT8       Size[3];
    UINT8       State;
};

typedef struct _K2EFI_IMAGE_DATA_DIRECTORY K2EFI_IMAGE_DATA_DIRECTORY;
struct _K2EFI_IMAGE_DATA_DIRECTORY
{
    UINT32 VirtualAddress;
    UINT32 Size;
};

#define K2EFI_TE_IMAGE_HEADER_SIGNATURE 0x5A56 // “VZ”

typedef struct _K2EFI_TE_IMAGE_HEADER K2EFI_TE_IMAGE_HEADER;
struct _K2EFI_TE_IMAGE_HEADER
{
    UINT16                      Signature;
    UINT16                      Machine;
    UINT8                       NumberOfSections;
    UINT8                       Subsystem;
    UINT16                      StrippedSize;
    UINT32                      AddressOfEntryPoint;
    UINT32                      BaseOfCode;
    UINT64                      ImageBase;
    K2EFI_IMAGE_DATA_DIRECTORY  DataDirectory[2];
};

// {8C8CE578-8A3D-4f1c-9935-896185C32DD3}
#define K2EFI_FIRMWARE_FILE_SYSTEM2_GUID { 0x8c8ce578, 0x8a3d, 0x4f1c, { 0x99, 0x35, 0x89, 0x61, 0x85, 0xc3, 0x2d, 0xd3 } }

// {5473C07A-3DCB-4dca-BD6F-1E9689E7349A}
#define K2EFI_FIRMWARE_FILE_SYSTEM3_GUID { 0x5473c07a, 0x3dcb, 0x4dca, { 0xbd, 0x6f, 0x1e, 0x96, 0x89, 0xe7, 0x34, 0x9a } }

// {1BA0062E-C779-4582-8566-336AE8F78F09}
#define K2EFI_FFS_VOLUME_TOP_FILE_GUID   { 0x1BA0062E, 0xC779, 0x4582, { 0x85, 0x66, 0x33, 0x6A, 0xE8, 0xF7, 0x8F, 0x9 } }

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_GET_TIME)(
    K2EFI_TIME *                Time,
    K2EFI_TIME_CAPABILITIES *   Capabilities
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_SET_TIME)(
    K2EFI_TIME *    Time
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_GET_WAKEUP_TIME)(
    K2EFIBOOL *     Enabled,
    K2EFIBOOL *     Pending,
    K2EFI_TIME *    Time
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_SET_WAKEUP_TIME)(
    K2EFIBOOL       Enable,
    K2EFI_TIME *    Time
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_SET_VIRTUAL_ADDRESS_MAP)(
    UINTN                       MemoryMapSize,
    UINTN                       DescriptorSize,
    UINT32                      DescriptorVersion,
    K2EFI_MEMORY_DESCRIPTOR *   VirtualMap
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_CONVERT_POINTER)(
    UINTN   DebugDisposition,
    void ** Address
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_GET_VARIABLE)(
    CHAR16 *        VariableName,
    K2_GUID128 *    VendorGuid,
    UINT32 *        Attributes,
    UINTN *         DataSize,
    void *          Data
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_GET_NEXT_VARIABLE_NAME)(
    UINTN *         VariableNameSize,
    CHAR16 *        VariableName,
    K2_GUID128 *    VendorGuid
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_SET_VARIABLE)(
    CHAR16 *        VariableName,
    K2_GUID128 *    VendorGuid,
    UINT32          Attributes,
    UINTN           DataSize,
    void *          Data
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_GET_NEXT_HIGH_MONO_COUNT)(
    UINT32 *    HighCount
    );

typedef
void
(K2EFIAPI *K2EFI_RESET_SYSTEM)(
    INTN            ResetType,
    K2EFI_STATUS    ResetStatus,
    UINTN           DataSize,
    void *          ResetData
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_UPDATE_CAPSULE)(
    K2EFI_CAPSULE_HEADER ** CapsuleHeaderArray,
    UINTN                   CapsuleCount,
    K2EFI_PHYSICAL_ADDRESS  ScatterGatherList
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_QUERY_CAPSULE_CAPABILITIES)(
    K2EFI_CAPSULE_HEADER ** CapsuleHeaderArray,
    UINTN                   CapsuleCount,
    UINT64 *                MaximumCapsuleSize,
    UINTN *                 ResetType
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_QUERY_VARIABLE_INFO)(
    UINT32      Attributes,
    UINT64 *    MaximumVariableStorageSize,
    UINT64 *    RemainingVariableStorageSize,
    UINT64 *    MaximumVariableSize
    );

typedef struct _K2EFI_RUNTIME_SERVICES K2EFI_RUNTIME_SERVICES;
struct _K2EFI_RUNTIME_SERVICES
{
    K2EFI_TABLE_HEADER                  Hdr;
    K2EFI_GET_TIME                      GetTime;
    K2EFI_SET_TIME                      SetTime;
    K2EFI_GET_WAKEUP_TIME               GetWakeupTime;
    K2EFI_SET_WAKEUP_TIME               SetWakeupTime;
    K2EFI_SET_VIRTUAL_ADDRESS_MAP       SetVirtualAddressMap;
    K2EFI_CONVERT_POINTER               ConvertPointer;
    K2EFI_GET_VARIABLE                  GetVariable;
    K2EFI_GET_NEXT_VARIABLE_NAME        GetNextVariableName;
    K2EFI_SET_VARIABLE                  SetVariable;
    K2EFI_GET_NEXT_HIGH_MONO_COUNT      GetNextHighMonotonicCount;
    K2EFI_RESET_SYSTEM                  ResetSystem;
    K2EFI_UPDATE_CAPSULE                UpdateCapsule;
    K2EFI_QUERY_CAPSULE_CAPABILITIES    QueryCapsuleCapabilities;
    K2EFI_QUERY_VARIABLE_INFO           QueryVariableInfo;
};

#define K2EFI_SYSTEM_TABLE_SIGNATURE      0x5453595320494249
#define K2EFI_2_70_SYSTEM_TABLE_REVISION  ((2<<16) | (70))
#define K2EFI_2_60_SYSTEM_TABLE_REVISION  ((2<<16) | (60))
#define K2EFI_2_50_SYSTEM_TABLE_REVISION  ((2<<16) | (50))
#define K2EFI_2_40_SYSTEM_TABLE_REVISION  ((2<<16) | (40))
#define K2EFI_2_31_SYSTEM_TABLE_REVISION  ((2<<16) | (31))
#define K2EFI_2_30_SYSTEM_TABLE_REVISION  ((2<<16) | (30))
#define K2EFI_2_20_SYSTEM_TABLE_REVISION  ((2<<16) | (20))
#define K2EFI_2_10_SYSTEM_TABLE_REVISION  ((2<<16) | (10))
#define K2EFI_2_00_SYSTEM_TABLE_REVISION  ((2<<16) | (00))
#define K2EFI_1_10_SYSTEM_TABLE_REVISION  ((1<<16) | (10))
#define K2EFI_1_02_SYSTEM_TABLE_REVISION  ((1<<16) | (02))
#define K2EFI_SPECIFICATION_VERSION       K2EFI_SYSTEM_TABLE_REVISION
#define K2EFI_SYSTEM_TABLE_REVISION       K2EFI_2_70_SYSTEM_TABLE_REVISION

typedef struct _K2EFI_SYSTEM_TABLE K2EFI_SYSTEM_TABLE;
struct _K2EFI_SYSTEM_TABLE
{
    K2EFI_TABLE_HEADER          Hdr;
    CHAR16 *                    Unic_FirmwareVendor;
    UINT32                      FirmwareRevision;
    void *                      Boot_ConsoleInHandle;       // unusable after boot phase
    void *                      Boot_ConIn;                 // unusable after boot phase
    void *                      Boot_ConsoleOutHandle;      // unusable after boot phase
    void *                      Boot_ConOut;                // unusable after boot phase
    void *                      Boot_StandardErrorHandle;   // unusable after boot phase
    void *                      Boot_StdErr;                // unusable after boot phase
    K2EFI_RUNTIME_SERVICES *    RuntimeServices;
    void *                      Boot_Services;              // unusable after boot phase
    UINTN                       NumberOfTableEntries;
    K2EFI_CONFIG_TABLE *        ConfigurationTable;
};

//
//------------------------------------------------------------------------
//

typedef struct {
    UINT32            RedMask;
    UINT32            GreenMask;
    UINT32            BlueMask;
    UINT32            ReservedMask;
} K2EFI_PIXEL_BITMASK;

#define K2Efi_PixelFormat_RGBX_8888 0
#define K2Efi_PixelFormat_BGRX_8888 1
#define K2Efi_PixelFormat_Mask      2
#define K2Efi_PixelFormat_NoBuffer  3

typedef struct {
    UINT32  Version;
    UINT32  HorizontalResolution;   // in pixels
    UINT32  VerticalResolution;     // in pixels
    UINT32  PixelFormatId;          // K2Efi_PixelFormat_xxxx
    UINT32  PixelRedMask;
    UINT32  PixelGreenMask;
    UINT32  PixelBlueMask;
    UINT32  PixelReservedMask;
    UINT32  PixelsPerScanLine;
} K2EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

//
//------------------------------------------------------------------------
//

#define K2EFI_BLOCK_IO_PROTOCOL_GUID \
  { \
    0x964e5b21, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

typedef struct _K2EFI_BLOCK_IO_PROTOCOL K2EFI_BLOCK_IO_PROTOCOL;

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_BLOCK_RESET)(
    IN K2EFI_BLOCK_IO_PROTOCOL *    This,
    IN K2EFIBOOL                    ExtendedVerification
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_BLOCK_READ)(
    IN K2EFI_BLOCK_IO_PROTOCOL *    This,
    IN UINT32                       MediaId,
    IN K2EFI_LBA                    Lba,
    IN UINTN                        BufferSize,
    OUT void *                      Buffer
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_BLOCK_WRITE)(
    IN K2EFI_BLOCK_IO_PROTOCOL *    This,
    IN UINT32                       MediaId,
    IN K2EFI_LBA                    Lba,
    IN UINTN                        BufferSize,
    IN void *                       Buffer
    );

typedef
K2EFI_STATUS
(K2EFIAPI *K2EFI_BLOCK_FLUSH)(
    IN K2EFI_BLOCK_IO_PROTOCOL  *   This
    );

typedef struct
{
    ///
    /// The curent media Id. If the media changes, this value is changed.
    ///
    UINT32  MediaId;

    ///
    /// TRUE if the media is removable; otherwise, FALSE.
    ///
    K2EFIBOOL RemovableMedia;

    ///
    /// TRUE if there is a media currently present in the device;
    /// othersise, FALSE. THis field shows the media present status
    /// as of the most recent ReadBlocks() or WriteBlocks() call.
    ///
    K2EFIBOOL MediaPresent;

    ///
    /// TRUE if LBA 0 is the first block of a partition; otherwise
    /// FALSE. For media with only one partition this would be TRUE.
    ///
    K2EFIBOOL LogicalPartition;

    ///
    /// TRUE if the media is marked read-only otherwise, FALSE.
    /// This field shows the read-only status as of the most recent WriteBlocks () call.
    ///
    K2EFIBOOL ReadOnly;

    ///
    /// TRUE if the WriteBlock () function caches write data.
    ///
    K2EFIBOOL WriteCaching;

    ///
    /// The intrinsic block size of the device. If the media changes, then
    /// this field is updated.
    ///
    UINT32  BlockSize;

    ///
    /// Supplies the alignment requirement for any buffer to read or write block(s).
    ///
    UINT32  IoAlign;

    ///
    /// The last logical block address on the device.
    /// If the media changes, then this field is updated.
    ///
    K2EFI_LBA LastBlock;

    ///
    /// Only present if K2EFI_BLOCK_IO_PROTOCOL.Revision is greater than or equal to
    /// EFI_BLOCK_IO_PROTOCOL_REVISION2. Returns the first LBA is aligned to
    /// a physical block boundary.
    ///
    K2EFI_LBA LowestAlignedLba;

    ///
    /// Only present if K2EFI_BLOCK_IO_PROTOCOL.Revision is greater than or equal to
    /// EFI_BLOCK_IO_PROTOCOL_REVISION2. Returns the number of logical blocks
    /// per physical block.
    ///
    UINT32 LogicalBlocksPerPhysicalBlock;

    ///
    /// Only present if K2EFI_BLOCK_IO_PROTOCOL.Revision is greater than or equal to
    /// EFI_BLOCK_IO_PROTOCOL_REVISION3. Returns the optimal transfer length
    /// granularity as a number of logical blocks.
    ///
    UINT32 OptimalTransferLengthGranularity;
} K2EFI_BLOCK_IO_MEDIA;

#define K2EFI_BLOCK_IO_PROTOCOL_REVISION  0x00010000
#define K2EFI_BLOCK_IO_PROTOCOL_REVISION2 0x00020001
#define K2EFI_BLOCK_IO_PROTOCOL_REVISION3 0x00020031

///
/// Revision defined in EFI1.1.
///
#define K2EFI_BLOCK_IO_INTERFACE_REVISION   K2EFI_BLOCK_IO_PROTOCOL_REVISION

///
///  This protocol provides control over block devices.
///
struct _K2EFI_BLOCK_IO_PROTOCOL
{
    ///
    /// The revision to which the block IO interface adheres. All future
    /// revisions must be backwards compatible. If a future version is not
    /// back wards compatible, it is not the same GUID.
    ///
    UINT64                  Revision;
    ///
    /// Pointer to the EFI_BLOCK_IO_MEDIA data for this device.
    ///
    K2EFI_BLOCK_IO_MEDIA *  Media;
    K2EFI_BLOCK_RESET       Reset;
    K2EFI_BLOCK_READ        ReadBlocks;
    K2EFI_BLOCK_WRITE       WriteBlocks;
    K2EFI_BLOCK_FLUSH       FlushBlocks;
};

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif  // __K2UEFI_H

