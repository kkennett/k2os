#   
#   BSD 3-Clause License
#   
#   Copyright (c) 2020, Kurt Kennett
#   All rights reserved.
#   
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are met:
#   
#   1. Redistributions of source code must retain the above copyright notice, this
#      list of conditions and the following disclaimer.
#   
#   2. Redistributions in binary form must reproduce the above copyright notice,
#      this list of conditions and the following disclaimer in the documentation
#      and/or other materials provided with the distribution.
#   
#   3. Neither the name of the copyright holder nor the names of its
#      contributors may be used to endorse or promote products derived from
#      this software without specific prior written permission.
#   
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

###################################################################################################
#
# Platform Definition
#
###################################################################################################
[Defines]
    PLATFORM_NAME                  = WandQuadPkg
    PLATFORM_GUID                  = BC95C1BC-E6A0-41CF-B99C-ADD9A554D2A3
    PLATFORM_VERSION               = 0.1
    DSC_SPECIFICATION              = 0x00010005
    OUTPUT_DIRECTORY               = Build/WandQuad
    SUPPORTED_ARCHITECTURES        = ARM
    BUILD_TARGETS                  = DEBUG|RELEASE
    SKUID_IDENTIFIER               = DEFAULT
    FLASH_DEFINITION               = WandQuadPkg/WandQuadPkg.fdf

###################################################################################################
#
# Default Platform Library Classes and Build Options
#
###################################################################################################
[BuildOptions.common.EDKII.SEC, BuildOptions.common.EDKII.PEI_CORE, BuildOptions.common.EDKII.PEIM]
    *_GCC48_ARM_DLINK_FLAGS = -z common-page-size=0x1000

[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER]
    *_GCC48_ARM_DLINK_FLAGS = -z common-page-size=0x1000

[BuildOptions.common]
    *_GCC48_*_ASL_PATH = C:\repo\k2os\src\uefi\BaseTools\Bin\asl\iasl.exe

[LibraryClasses.common]
    BaseLib                     |MdePkg/Library/BaseLib/BaseLib.inf
    PcdLib                      |MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
    PrintLib                    |MdePkg/Library/BasePrintLib/BasePrintLib.inf
    IoLib                       |MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
    UefiLib                     |MdePkg/Library/UefiLib/UefiLib.inf
    UefiBootServicesTableLib    |MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
    UefiRuntimeLib              |MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
    UefiRuntimeServicesTableLib |MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
    UefiDriverEntryPoint        |MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
    ReportStatusCodeLib         |IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf

[LibraryClasses.ARM]
    ArmLib                      |ArmPkg/Library/ArmLib/ArmBaseLib.inf
    CacheMaintenanceLib         |ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf

    BaseMemoryLib               |MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf

    TimerLib                    |WandQuadPkg/Library/TimerLib/TimerLib.inf
    ArmPlatformLib              |WandQuadPkg/Library/ArmPlatformLib/ArmPlatformLib.inf

    IMX6GptLib                  |NxpPkg/Library/IMX6/IMX6GptLib/IMX6GptLib.inf
    IMX6UartLib                 |NxpPkg/Library/IMX6/IMX6UartLib/IMX6UartLib.inf
    IMX6EpitLib                 |NxpPkg/Library/IMX6/IMX6EpitLib/IMX6EpitLib.inf
    IMX6ClockLib                |NxpPkg/Library/IMX6/IMX6ClockLib/IMX6ClockLib.inf
    IMX6UsdhcLib                |NxpPkg/Library/IMX6/IMX6UsdhcLib/IMX6UsdhcLib.inf

    NULL                        |ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf
    NULL                        |MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf


###################################################################################################
#
# ArmPkg PCD Override
#
###################################################################################################
[PcdsFixedAtBuild]
    gArmTokenSpaceGuid.PcdGicDistributorBase            | 0x00A01000          #0                  |UINT32                         
    gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase     | 0x00A00100          #0                  |UINT32                  
    gArmTokenSpaceGuid.PcdVFPEnabled                    | 1                   #0                  |UINT32                                  
    gArmTokenSpaceGuid.PcdL2x0ControllerBase            | 0x00A02000          #0                  |UINT32                          
    gArmTokenSpaceGuid.PcdSystemMemoryBase              | 0x0000000010000000  #0                  |UINT64                           
    gArmTokenSpaceGuid.PcdSystemMemorySize              | 0x0000000080000000  #0                  |UINT64                           


###################################################################################################
#
# ArmPlatformPkg PCD Override
#
###################################################################################################
[PcdsFixedAtBuild.common]                                               
    gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize      | 0x00008000         #0x10000            |UINT32             
    gArmPlatformTokenSpaceGuid.PcdCPUCoreSecondaryStackSize    | 0x00001000         #0x1000             |UINT32
    gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize   | 0x08000000         #0x08000000         |UINT32       
    gArmPlatformTokenSpaceGuid.PcdCoreCount                    | 4

###################################################################################################
#
# EmbeddedPkg Pcd Override
#
###################################################################################################
[PcdsFeatureFlag.common]
    gEmbeddedTokenSpaceGuid.PcdPrePiProduceMemoryTypeInformationHob | TRUE        #FALSE              |BOOLEAN

###################################################################################################
#
# MdeModulePkg Pcd Override
#
###################################################################################################
[PcdsFixedAtBuild]
    gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString     | L"ZERO"   #L""                |VOID*
    gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut             | 3         #0xffff             |UINT16
    # Point to the UEFI shell ==  7C04A583-9E3E-4f1c-AD65-E05268D0B4D1
    gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile       |{ 0x83, 0xA5, 0x04, 0x7C, 0x3E, 0x9E, 0x1C, 0x4F, 0xAD, 0x65, 0xE0, 0x52, 0x68, 0xD0, 0xB4, 0xD1 }

    gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemId          |"K2____"           # |VOID*|0x30001034
    gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemTableId     |0x44415551444E4157 # |UINT64|0x30001035
    gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemRevision    |0x00000001         # |UINT32|0x30001036
    gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorId      |0x20202020         # |UINT32|0x30001037
    gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorRevision|0x01000013         # |UINT32|0x30001038
   

###################################################################################################
#
# MdePkg Pcd Override
#
###################################################################################################

    gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType             |4

# DEBUG_ASSERT_ENABLED       0x01
# DEBUG_PRINT_ENABLED        0x02
# DEBUG_CODE_ENABLED         0x04
# CLEAR_MEMORY_ENABLED       0x08
# ASSERT_BREAKPOINT_ENABLED  0x10
# ASSERT_DEADLOOP_ENABLED    0x20
    gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask               | 0x2f        #0                  |UINT8

  #  DEBUG_INIT      0x00000001  # Initialization
  #  DEBUG_WARN      0x00000002  # Warnings
  #  DEBUG_LOAD      0x00000004  # Load events
  #  DEBUG_FS        0x00000008  # EFI File system
  #  DEBUG_POOL      0x00000010  # Alloc & Free's
  #  DEBUG_PAGE      0x00000020  # Alloc & Free's
  #  DEBUG_INFO      0x00000040  # Informational debug messages
  #  DEBUG_DISPATCH  0x00000080  # PEI/DXE/SMM Dispatchers
  #  DEBUG_VARIABLE  0x00000100  # Variable
  #  DEBUG_BM        0x00000400  # Boot Manager
  #  DEBUG_BLKIO     0x00001000  # BlkIo Driver
  #  DEBUG_NET       0x00004000  # SNI Driver
  #  DEBUG_UNDI      0x00010000  # UNDI Driver
  #  DEBUG_LOADFILE  0x00020000  # UNDI Driver
  #  DEBUG_EVENT     0x00080000  # Event messages
  #  DEBUG_GCD       0x00100000  # Global Coherency Database changes
  #  DEBUG_CACHE     0x00200000  # Memory range cachability changes
  #  DEBUG_VERBOSE   0x00400000  # Detailed debug messages that may significantly impact boot performance
  #  DEBUG_ERROR     0x80000000  # Error
     gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel           | 0x8008040F  #0x80000000         |UINT32
#     gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel           | 0x800805CF  #0x80000000         |UINT32
#     gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel           | 0x80080403  #0x80000000         |UINT32



###################################################################################################
#
# SEC and PEI
#
###################################################################################################
[Components]
    WandQuadPkg/Sec/Sec.inf {
        <LibraryClasses>
        ArmGicLib                   |WandQuadPkg/Drivers/ArmPkg/ArmGic/ArmGicSecLib.inf
        ArmGicArchLib               |ArmPkg/Library/ArmGicArchSecLib/ArmGicArchSecLib.inf
        SynchronizationLib          |MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
    }

    ArmPlatformPkg/PrePi/PeiMPCore.inf {
        <LibraryClasses>
        ArmGicLib                   |WandQuadPkg/Drivers/ArmPkg/ArmGic/ArmGicLib.inf
        ArmGicArchLib               |ArmPkg/Library/ArmGicArchSecLib/ArmGicArchSecLib.inf
        ArmMmuLib                   |ArmPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf
        ArmPlatformStackLib         |ArmPlatformPkg/Library/ArmPlatformStackLib/ArmPlatformStackLib.inf
        PrePiHobListPointerLib      |ArmPlatformPkg/Library/PrePiHobListPointerLib/PrePiHobListPointerLib.inf
        PlatformPeiLib              |WandQuadPkg/Library/PlatformPeiLib/PlatformPeiLib.inf
        MemoryInitPeiLib            |WandQuadPkg/Library/MemoryInitPeiLib/MemoryInitPeiLib.inf
        PeCoffGetEntryPointLib      |MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
        UefiDecompressLib           |MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
        PeCoffLib                   |MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
        PerformanceLib              |MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
        DebugAgentLib               |MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
        ExtractGuidedSectionLib     |EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf
        PrePiLib                    |EmbeddedPkg/Library/PrePiLib/PrePiLib.inf
        MemoryAllocationLib         |EmbeddedPkg/Library/PrePiMemoryAllocationLib/PrePiMemoryAllocationLib.inf
        HobLib                      |EmbeddedPkg/Library/PrePiHobLib/PrePiHobLib.inf
        LzmaDecompressLib           |IntelFrameworkModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
    }

[LibraryClasses.common.SEC]
#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
#    SerialPortLib               |MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
#    PeCoffExtraActionLib        |ArmPkg/Library/DebugPeCoffExtraActionLib/DebugPeCoffExtraActionLib.inf
    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
    SerialPortLib               |WandQuadPkg/Library/SerialPortLibOutOnly/SerialPortLibOutOnly.inf


###################################################################################################
#
# DXE_CORE
#
###################################################################################################
[Components]
    MdeModulePkg/Core/Dxe/DxeMain.inf {
        <LibraryClasses>
        DevicePathLib               |MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
        DxeCoreEntryPoint           |MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
        DxeServicesLib              |MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
        ExtractGuidedSectionLib     |MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
        HobLib                      |MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
        PeCoffGetEntryPointLib      |MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
        PeCoffLib                   |MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
        UefiDecompressLib           |MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
        CpuExceptionHandlerLib      |MdeModulePkg/Library/CpuExceptionHandlerLibNull/CpuExceptionHandlerLibNull.inf
        DebugAgentLib               |MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
        MemoryAllocationLib         |MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
        PerformanceLib              |MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf
    }

[LibraryClasses.common.DXE_CORE]
#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
#    SerialPortLib               |MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
#    PeCoffExtraActionLib        |ArmPkg/Library/DebugPeCoffExtraActionLib/DebugPeCoffExtraActionLib.inf
    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
    SerialPortLib               |WandQuadPkg/Library/SerialPortLibOutOnly/SerialPortLibOutOnly.inf


###################################################################################################
#
# DXE_DRIVER
#
###################################################################################################
[Components]
    MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf {
        <LibraryClasses>
        DevicePathLib               |MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
    }

    ArmPkg/Drivers/CpuDxe/CpuDxe.inf {
        <LibraryClasses>
        ArmMmuLib                   |ArmPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf
        CpuLib                      |MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
        CpuExceptionHandlerLib      |ArmPkg/Library/ArmExceptionLib/ArmExceptionLib.inf
        DefaultExceptionHandlerLib  |ArmPkg/Library/DefaultExceptionHandlerLib/DefaultExceptionHandlerLib.inf
        DxeServicesTableLib         |MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
        HobLib                      |MdePkg/Library/DxeHobLib/DxeHobLib.inf
        PeCoffGetEntryPointLib      |MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
        ArmDisassemblerLib          |ArmPkg/Library/ArmDisassemblerLib/ArmDisassemblerLib.inf
    }

    ArmPkg/Drivers/ArmGic/ArmGicDxe.inf  {
        <LibraryClasses>
        ArmGicLib                   |ArmPkg/Drivers/ArmGic/ArmGicLib.inf
        ArmGicArchLib               |ArmPkg/Library/ArmGicArchLib/ArmGicArchLib.inf
    }

    MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf {
        <LibraryClasses>
        SecurityManagementLib       |MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
        DxeServicesLib              |MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
    }

    MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf

    EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf

    NxpPkg/Drivers/IMX6/TimerDxe/TimerDxe.inf

    MdeModulePkg/Universal/SerialDxe/SerialDxe.inf {
        <LibraryClasses>
        SerialPortLib               |WandQuadPkg/Library/SerialPortLibInOut/SerialPortLibInOut.inf
    }

    MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf {
        <LibraryClasses>
        PcdLib                      |WandQuadPkg/Library/PcdLibDummy/PcdLibDummy.inf
    }

    WandQuadPkg/Drivers/PciEmulation/PciEmulation.inf {
        <LibraryClasses>
        NonDiscoverableDeviceRegistrationLib|MdeModulePkg/Library/NonDiscoverableDeviceRegistrationLib/NonDiscoverableDeviceRegistrationLib.inf
    }

    WandQuadPkg/Drivers/MinBdsDxe/MinBdsDxe.inf

    MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf

    WandQuadPkg/Drivers/PlatformSmbiosDxe/PlatformSmbiosDxe.inf

    MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf

    MdeModulePkg/Universal/Acpi/AcpiPlatformDxe/AcpiPlatformDxe.inf   {
       <LibraryClasses>
        DxeServicesLib              |MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
    }

    WandQuadPkg/AcpiTables/AcpiTables.inf

[LibraryClasses.common.DXE_DRIVER]
    MemoryAllocationLib         |MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    DevicePathLib               |MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
    DxeServicesTableLib         |MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf

#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
#    SerialPortLib               |MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
    SerialPortLib               |WandQuadPkg/Library/SerialPortLibOutOnly/SerialPortLibOutOnly.inf


###################################################################################################
#
# DXE_RUNTIME_DRIVER
#
###################################################################################################
[Components]
    MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf {
        <LibraryClasses>
        MemoryAllocationLib     |MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
        PeCoffLib               |MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
        DevicePathLib           |MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
    }

    MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf {
        <LibraryClasses>
        CapsuleLib              |MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
        UefiRuntimeLib          |MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
    }

    MdeModulePkg/Universal/Variable/EmuRuntimeDxe/EmuVariableRuntimeDxe.inf {
        <LibraryClasses>
        UefiRuntimeLib          |MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
        MemoryAllocationLib     |MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
        HobLib                  |MdePkg/Library/DxeHobLib/DxeHobLib.inf
        DevicePathLib           |MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
    }

    EmbeddedPkg/EmbeddedMonotonicCounter/EmbeddedMonotonicCounter.inf

    EmbeddedPkg/ResetRuntimeDxe/ResetRuntimeDxe.inf {
        <LibraryClasses>
        EfiResetSystemLib       |WandQuadPkg/Library/ResetSystemLib/ResetSystemLib.inf
        MemoryAllocationLib     |MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
        DxeServicesTableLib     |MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
        DevicePathLib           |MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
    }

    EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf {
        <LibraryClasses>
        RealTimeClockLib        |WandQuadPkg/Library/RealTimeClockLib/RealTimeClockLib.inf
        MemoryAllocationLib     |MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
        DxeServicesTableLib     |MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
        DevicePathLib           |MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
    }

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
#    SerialPortLib               |MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
    SerialPortLib               |WandQuadPkg/Library/SerialPortLibOutOnly/SerialPortLibOutOnly.inf



###################################################################################################
#
# UEFI_DRIVER
#
###################################################################################################
[Components]
    MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
    MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
    MdeModulePkg/Universal/FvSimpleFileSystemDxe/FvSimpleFileSystemDxe.inf
    MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
    MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
    MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
    MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
    MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
    FatPkg/EnhancedFatDxe/Fat.inf
    WandQuadPkg/Drivers/HiiResources/HiiResourcesDxe.inf
    MdeModulePkg/Bus/Pci/NonDiscoverablePciDeviceDxe/NonDiscoverablePciDeviceDxe.inf   {
       <LibraryClasses>
       DxeServicesTableLib         |MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
    }

[LibraryClasses.common.UEFI_DRIVER]
    DevicePathLib               |MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
    MemoryAllocationLib         |MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    HiiLib                      |MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
    UefiHiiServicesLib          |MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf

#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
#    SerialPortLib               |MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
    SerialPortLib               |WandQuadPkg/Library/SerialPortLibOutOnly/SerialPortLibOutOnly.inf

###################################################################################################
#
# UEFI_APPLICATION
#
###################################################################################################
[Components]
    ShellBinPkg/UefiShell/UefiShell.inf

[LibraryClasses.common.UEFI_APPLICATION]
#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
#    SerialPortLib               |MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
    SerialPortLib               |WandQuadPkg/Library/SerialPortLibOutOnly/SerialPortLibOutOnly.inf
