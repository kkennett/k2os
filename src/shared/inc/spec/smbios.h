//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2020, Kurt Kennett
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
#ifndef __SPEC_SMBIOS_H
#define __SPEC_SMBIOS_H

//
// --------------------------------------------------------------------------------- 
//

#include <k2basetype.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SMBIOS_TYPE_BIOS_INFORMATION                        0
#define SMBIOS_TYPE_SYSTEM_INFORMATION                      1
#define SMBIOS_TYPE_BASEBOARD_INFORMATION                   2
#define SMBIOS_TYPE_SYSTEM_ENCLOSURE                        3
#define SMBIOS_TYPE_PROCESSOR_INFORMATION                   4
#define SMBIOS_TYPE_MEMORY_CONTROLLER_INFORMATION           5
#define SMBIOS_TYPE_MEMORY_MODULE_INFORMATON                6
#define SMBIOS_TYPE_CACHE_INFORMATION                       7
#define SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION              8
#define SMBIOS_TYPE_SYSTEM_SLOTS                            9
#define SMBIOS_TYPE_ONBOARD_DEVICE_INFORMATION              10
#define SMBIOS_TYPE_OEM_STRINGS                             11
#define SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS            12
#define SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION               13
#define SMBIOS_TYPE_GROUP_ASSOCIATIONS                      14
#define SMBIOS_TYPE_SYSTEM_EVENT_LOG                        15
#define SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY                   16
#define SMBIOS_TYPE_MEMORY_DEVICE                           17
#define SMBIOS_TYPE_32BIT_MEMORY_ERROR_INFORMATION          18
#define SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS             19
#define SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS            20
#define SMBIOS_TYPE_BUILT_IN_POINTING_DEVICE                21
#define SMBIOS_TYPE_PORTABLE_BATTERY                        22
#define SMBIOS_TYPE_SYSTEM_RESET                            23
#define SMBIOS_TYPE_HARDWARE_SECURITY                       24
#define SMBIOS_TYPE_SYSTEM_POWER_CONTROLS                   25
#define SMBIOS_TYPE_VOLTAGE_PROBE                           26
#define SMBIOS_TYPE_COOLING_DEVICE                          27
#define SMBIOS_TYPE_TEMPERATURE_PROBE                       28
#define SMBIOS_TYPE_ELECTRICAL_CURRENT_PROBE                29
#define SMBIOS_TYPE_OUT_OF_BAND_REMOTE_ACCESS               30
#define SMBIOS_TYPE_BOOT_INTEGRITY_SERVICE                  31
#define SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION                 32
#define SMBIOS_TYPE_64BIT_MEMORY_ERROR_INFORMATION          33
#define SMBIOS_TYPE_MANAGEMENT_DEVICE                       34
#define SMBIOS_TYPE_MANAGEMENT_DEVICE_COMPONENT             35
#define SMBIOS_TYPE_MANAGEMENT_DEVICE_THRESHOLD_DATA        36
#define SMBIOS_TYPE_MEMORY_CHANNEL                          37
#define SMBIOS_TYPE_IPMI_DEVICE_INFORMATION                 38
#define SMBIOS_TYPE_SYSTEM_POWER_SUPPLY                     39
#define SMBIOS_TYPE_ADDITIONAL_INFORMATION                  40
#define SMBIOS_TYPE_ONBOARD_DEVICES_EXTENDED_INFORMATION    41
#define SMBIOS_TYPE_MANAGEMENT_CONTROLLER_HOST_INTERFACE    42
#define SMBIOS_TYPE_TPM_DEVICE                              43
#define SMBIOS_TYPE_INACTIVE                                0x007E
#define SMBIOS_TYPE_END_OF_TABLE                            0x007F
#define SMBIOS_TYPE_OEM_BEGIN                               0x0080
#define SMBIOS_TYPE_OEM_END                                 0x00FF

K2_PACKED_PUSH
struct _SMBIOS_ENTRYPOINT
{
    UINT8   AnchorString[4];
    UINT8   EntryPointStructureChecksum;
    UINT8   EntryPointLength;
    UINT8   MajorVersion;
    UINT8   MinorVersion;
    UINT16  MaxStructureSize;
    UINT8   EntryPointRevision;
    UINT8   FormattedArea[5];
    UINT8   IntermediateAnchorString[5];
    UINT8   IntermediateChecksum;
    UINT16  TableLength;
    UINT32  TableAddress;
    UINT16  NumberOfSmbiosStructures;
    UINT8   SmbiosBcdRevision;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _SMBIOS_ENTRYPOINT SMBIOS_ENTRYPOINT;

K2_PACKED_PUSH
struct _SMBIOS_HEADER
{
    UINT8   Type;
    UINT8   Length;
    UINT16  Handle;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _SMBIOS_HEADER SMBIOS_HEADER;

K2_PACKED_PUSH
struct _SMBIOS_MISC_BIOS_CHARACTERISTICS
{
    UINT32  Reserved : 2;  ///< Bits 0-1.
    UINT32  Unknown : 1;
    UINT32  BiosCharacteristicsNotSupported : 1;
    UINT32  IsaIsSupported : 1;
    UINT32  McaIsSupported : 1;
    UINT32  EisaIsSupported : 1;
    UINT32  PciIsSupported : 1;
    UINT32  PcmciaIsSupported : 1;
    UINT32  PlugAndPlayIsSupported : 1;
    UINT32  ApmIsSupported : 1;
    UINT32  BiosIsUpgradable : 1;
    UINT32  BiosShadowingAllowed : 1;
    UINT32  VlVesaIsSupported : 1;
    UINT32  EscdSupportIsAvailable : 1;
    UINT32  BootFromCdIsSupported : 1;
    UINT32  SelectableBootIsSupported : 1;
    UINT32  RomBiosIsSocketed : 1;
    UINT32  BootFromPcmciaIsSupported : 1;
    UINT32  EDDSpecificationIsSupported : 1;
    UINT32  JapaneseNecFloppyIsSupported : 1;
    UINT32  JapaneseToshibaFloppyIsSupported : 1;
    UINT32  Floppy525_360IsSupported : 1;
    UINT32  Floppy525_12IsSupported : 1;
    UINT32  Floppy35_720IsSupported : 1;
    UINT32  Floppy35_288IsSupported : 1;
    UINT32  PrintScreenIsSupported : 1;
    UINT32  Keyboard8042IsSupported : 1;
    UINT32  SerialIsSupported : 1;
    UINT32  PrinterIsSupported : 1;
    UINT32  CgaMonoIsSupported : 1;
    UINT32  NecPc98 : 1;
    UINT32  ReservedForVendor : 32;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _SMBIOS_MISC_BIOS_CHARACTERISTICS SMBIOS_MISC_BIOS_CHARACTERISTICS;

K2_PACKED_PUSH
struct _SMBIOS_MISC_EXT_BIOS_RESERVED
{
    UINT8  AcpiIsSupported : 1;
    UINT8  UsbLegacyIsSupported : 1;
    UINT8  AgpIsSupported : 1;
    UINT8  I2OBootIsSupported : 1;
    UINT8  Ls120BootIsSupported : 1;
    UINT8  AtapiZipDriveBootIsSupported : 1;
    UINT8  Boot1394IsSupported : 1;
    UINT8  SmartBatteryIsSupported : 1;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _SMBIOS_MISC_EXT_BIOS_RESERVED SMBIOS_MISC_EXT_BIOS_RESERVED;

K2_PACKED_PUSH
struct _SMBIOS_MISC_EXT_SYSTEM_RESERVED
{
    UINT8  BiosBootSpecIsSupported : 1;
    UINT8  FunctionKeyNetworkBootIsSupported : 1;
    UINT8  TargetContentDistributionEnabled : 1;
    UINT8  UefiSpecificationSupported : 1;
    UINT8  VirtualMachineSupported : 1;
    UINT8  ExtensionByte2Reserved : 3;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _SMBIOS_MISC_EXT_SYSTEM_RESERVED SMBIOS_MISC_EXT_SYSTEM_RESERVED;

K2_PACKED_PUSH
struct _SMBIOS_MISC_EXT
{
    SMBIOS_MISC_EXT_BIOS_RESERVED    BiosReserved;
    SMBIOS_MISC_EXT_SYSTEM_RESERVED  SystemReserved;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _SMBIOS_MISC_EXT SMBIOS_MISC_EXT;

K2_PACKED_PUSH
struct _SMBIOS_TBL_TYPE0
{
    SMBIOS_HEADER                       Hdr;
    UINT8                               Vendor;
    UINT8                               BiosVersion;
    UINT16                              BiosSegment;
    UINT8                               BiosReleaseDate;
    UINT8                               BiosSize;
    SMBIOS_MISC_BIOS_CHARACTERISTICS    BiosCharacteristics;
    UINT8                               BIOSCharacteristicsExtensionBytes[2];
    UINT8                               SystemBiosMajorRelease;
    UINT8                               SystemBiosMinorRelease;
    UINT8                               EmbeddedControllerFirmwareMajorRelease;
    UINT8                               EmbeddedControllerFirmwareMinorRelease;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _SMBIOS_TBL_TYPE0 SMBIOS_TBL_TYPE0;

K2_PACKED_PUSH
struct _SMBIOS_TBL_TYPE1
{
    SMBIOS_HEADER   Hdr;
    UINT8           Manufacturer;
    UINT8           ProductName;
    UINT8           Version;
    UINT8           SerialNumber;
    K2_GUID128      Uuid;
    UINT8           WakeUpType;
    UINT8           SKUNumber;
    UINT8           Family;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _SMBIOS_TBL_TYPE1 SMBIOS_TBL_TYPE1;

K2_PACKED_PUSH
struct _SMBIOS_TBL_TYPE2
{
    SMBIOS_HEADER   Hdr;
    UINT8           Manufacturer;
    UINT8           ProductName;
    UINT8           Version;
    UINT8           SerialNumber;
    UINT8           AssetTag;
    UINT8           FeatureFlag;
    UINT8           LocationInChassis;
    UINT16          ChassisHandle;
    UINT8           BoardType;              ///< The enumeration value from BASE_BOARD_TYPE.
    UINT8           NumberOfContainedObjectHandles;
    UINT16          ContainedObjectHandles[1];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _SMBIOS_TBL_TYPE2 SMBIOS_TBL_TYPE2;

#ifdef __cplusplus
};  // extern "C"
#endif

/* ------------------------------------------------------------------------- */

#endif  // __SPEC_SMBIOS_H

