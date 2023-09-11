#   
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
    PLATFORM_NAME                  = QemuQuadPkg
    PLATFORM_GUID                  = F8190040-6BB9-42BF-8018-12DA175D8B16
    PLATFORM_VERSION               = 0.1
    DSC_SPECIFICATION              = 0x00010005
    OUTPUT_DIRECTORY               = Build/QemuQuad
    SUPPORTED_ARCHITECTURES        = ARM
    BUILD_TARGETS                  = DEBUG|RELEASE
    SKUID_IDENTIFIER               = DEFAULT
    FLASH_DEFINITION               = QemuQuadPkg/QemuQuadPkg.fdf

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
    RELEASE_*_*_CC_FLAGS  = -DMDEPKG_NDEBUG

[LibraryClasses.common]
    ExtractGuidedSectionLib     |EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf
    BaseLib                     |MdePkg/Library/BaseLib/BaseLib.inf
    PrintLib                    |MdePkg/Library/BasePrintLib/BasePrintLib.inf
    IoLib                       |MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
    PeCoffGetEntryPointLib      |MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
    PerformanceLib              |MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
    PeCoffLib                   |MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
    BaseMemoryLib               |MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
    SynchronizationLib          |MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
    UefiDecompressLib           |MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
    UefiLib                     |MdePkg/Library/UefiLib/UefiLib.inf
    UefiBootServicesTableLib    |MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
    UefiRuntimeLib              |MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
    UefiRuntimeServicesTableLib |MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
    UefiDriverEntryPoint        |MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
    OemHookStatusCodeLib        |MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
    DebugAgentLib               |MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
    
[LibraryClasses.ARM]
    ArmLib                      |ArmPkg/Library/ArmLib/ArmBaseLib.inf
    ArmMmuLib                   |ArmPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf
    ArmSmcLib                   |ArmPkg/Library/ArmSmcLib/ArmSmcLib.inf
    CacheMaintenanceLib         |ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
    NULL                        |ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf
    NULL                        |MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf
    IMX6GptLib                  |NxpPkg/Library/IMX6/IMX6GptLib/IMX6GptLib.inf
    IMX6UartLib                 |NxpPkg/Library/IMX6/IMX6UartLib/IMX6UartLib.inf
    IMX6ClockLib                |NxpPkg/Library/IMX6/IMX6ClockLib/IMX6ClockLib.inf
    IMX6UsdhcLib                |NxpPkg/Library/IMX6/IMX6UsdhcLib/IMX6UsdhcLib.inf
    IMX6EpitLib                 |NxpPkg/Library/IMX6/IMX6EpitLib/IMX6EpitLib.inf
    TimerLib                    |QemuQuadPkg/Library/TimerLib/TimerLib.inf

###################################################################################################
#
# ArmPkg PCD Override
#
###################################################################################################
[PcdsFixedAtBuild]
    gArmTokenSpaceGuid.PcdGicDistributorBase            | 0x00A01000          #0                  |UINT32                         
    gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase     | 0x00A00100          #0                  |UINT32                  
    gArmTokenSpaceGuid.PcdVFPEnabled                    | 1                   #0                  |UINT32                                  

###################################################################################################
#
# ArmPlatformPkg PCD Override
#
###################################################################################################
[PcdsFixedAtBuild.common]                                               
    gArmPlatformTokenSpaceGuid.PcdCoreCount                    | 4

###################################################################################################
#
# MdePkg Pcd Override
#
###################################################################################################
[PcdsFixedAtBuild]
    gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut                         | 3         #0xffff             |UINT16

  ## Indicates the usable type of terminal.<BR><BR>
  #  0 - PCANSI<BR>
  #  1 - VT100<BR>
  #  2 - VT100+<BR>
  #  3 - UTF8<BR>
  #  4 - TTYTERM, NOT defined in UEFI SPEC<BR>
    gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType                         | 4

    # Point to the UEFI shell ==  7C04A583-9E3E-4f1c-AD65-E05268D0B4D1
    gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile       |{ 0x83, 0xA5, 0x04, 0x7C, 0x3E, 0x9E, 0x1C, 0x4F, 0xAD, 0x65, 0xE0, 0x52, 0x68, 0xD0, 0xB4, 0xD1 }

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
# MdeModulePkg Pcd Override
#
###################################################################################################
[PcdsFeatureFlag]
    gEfiMdeModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathFromText   | TRUE
    gEfiMdeModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathToText     | TRUE

[PcdsFixedAtBuild]
    gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase            | 0x10000000
    gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize            | 0x0001A000
    gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase          | 0x1001A000
    gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize          | 0x0001A000
    gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase            | 0x10034000
    gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize            | 0x0001A000

    # Point to the UEFI shell ==  7C04A583-9E3E-4f1c-AD65-E05268D0B4D1
    gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile                   | { 0x83, 0xA5, 0x04, 0x7C, 0x3E, 0x9E, 0x1C, 0x4F, 0xAD, 0x65, 0xE0, 0x52, 0x68, 0xD0, 0xB4, 0xD1 }

    gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemId                      | "K2____"
    gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemTableId                 | 0x444155514F4F4455
    gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemRevision                | 0x00000001
    gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorId                  | 0x20202020
    gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorRevision            | 0x01000013


###################################################################################################
#
# SEC
#
###################################################################################################
[Components]
    QemuQuadPkg/Sec/Sec.inf

[LibraryClasses.common.SEC]
    ArmGicLib                   |QemuQuadPkg/Drivers/ArmPkg/ArmGic/ArmGicSecLib.inf
    ArmGicArchLib               |ArmPkg/Library/ArmGicArchSecLib/ArmGicArchSecLib.inf
    NULL                        |MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
    PcdLib                      |MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    SerialPortLib               |MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    SerialPortLib               |QemuQuadPkg/Library/SerialPortLib/SerialPortLib.inf


###################################################################################################
#
# PEI_CORE
#
###################################################################################################
[Components]
    MdeModulePkg/Core/Pei/PeiMain.inf

[LibraryClasses.common.PEI_CORE]
    PeiServicesLib              |MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
    HobLib                      |MdePkg/Library/PeiHobLib/PeiHobLib.inf
    PeiCoreEntryPoint           |MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
    MemoryAllocationLib         |MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
    PeiServicesTablePointerLib  |MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
    PcdLib                      |MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    ReportStatusCodeLib         |MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    SerialPortLib               |QemuQuadPkg/Library/SerialPortLib/SerialPortLib.inf
    ReportStatusCodeLib         |MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf

###################################################################################################
#
# PEIMs
#
###################################################################################################
[Components]
    ArmPkg/Drivers/CpuPei/CpuPei.inf

    QemuQuadPkg/Peim/MemoryInitPeim/MemoryInitPeim.inf

    QemuQuadPkg/Peim/PlatformPeim/PlatformPeim.inf

    MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf

    QemuQuadPkg/Peim/FatPei/FatPei.inf

    MdeModulePkg/Universal/PCD/Pei/Pcd.inf {
        <LibraryClasses>
        PcdLib                  |MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
    }

[LibraryClasses.common.PEIM]
    PeimEntryPoint              |MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
    PeiServicesLib              |MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
    HobLib                      |MdePkg/Library/PeiHobLib/PeiHobLib.inf
    MemoryAllocationLib         |MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
    PeiServicesTablePointerLib  |MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
    PcdLib                      |MdePkg/Library/PeiPcdLib/PeiPcdLib.inf

#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    ReportStatusCodeLib         |MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    SerialPortLib               |QemuQuadPkg/Library/SerialPortLib/SerialPortLib.inf
    ReportStatusCodeLib         |MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf

###################################################################################################
#
# DXE_CORE
#
###################################################################################################
[Components]
    MdeModulePkg/Core/Dxe/DxeMain.inf

[LibraryClasses.common.DXE_CORE]
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
    PeCoffExtraActionLib        |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
    PcdLib                      |MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    ReportStatusCodeLib         |MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    SerialPortLib               |QemuQuadPkg/Library/SerialPortLib/SerialPortLib.inf
    ReportStatusCodeLib         |MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf


###################################################################################################
#
# DXE_DRIVERs
#
###################################################################################################
[Components]
    MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf {
        <LibraryClasses>
        DevicePathLib           |MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
        PcdLib                  |MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
    }

    ArmPkg/Drivers/CpuDxe/CpuDxe.inf {
        <LibraryClasses>
        ArmDisassemblerLib          |ArmPkg/Library/ArmDisassemblerLib/ArmDisassemblerLib.inf
        CpuLib                      |MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
        CpuExceptionHandlerLib      |ArmPkg/Library/ArmExceptionLib/ArmExceptionLib.inf
        DefaultExceptionHandlerLib  |ArmPkg/Library/DefaultExceptionHandlerLib/DefaultExceptionHandlerLib.inf
    }

    ArmPkg/Drivers/ArmGic/ArmGicDxe.inf {
        <LibraryClasses>
        ArmGicLib               |QemuQuadPkg/Drivers/ArmPkg/ArmGic/ArmGicLib.inf
        ArmGicArchLib           |ArmPkg/Library/ArmGicArchLib/ArmGicArchLib.inf
    }

    EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf

    NxpPkg/Drivers/IMX6/TimerDxe/TimerDxe.inf

    MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf {
        <LibraryClasses>
        SecurityManagementLib       |MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
    }

    MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf

    MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
    QemuQuadPkg/Drivers/PlatformSmbiosDxe/PlatformSmbiosDxe.inf

    MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
    MdeModulePkg/Universal/Acpi/AcpiPlatformDxe/AcpiPlatformDxe.inf   
    QemuQuadPkg/AcpiTables/AcpiTables.inf

    MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
        <LibraryClasses>
        PcdLib                  |MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
    }

    MdeModulePkg/Universal/SerialDxe/SerialDxe.inf

    MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf

    QemuQuadPkg/Drivers/MinBdsDxe/MinBdsDxe.inf

[LibraryClasses.common.DXE_DRIVER]
    DxeServicesTableLib         |MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
    HobLib                      |MdePkg/Library/DxeHobLib/DxeHobLib.inf
    MemoryAllocationLib         |MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    DevicePathLib               |MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
    DxeServicesLib              |MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
    PcdLib                      |MdePkg/Library/DxePcdLib/DxePcdLib.inf

#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    SerialPortLib               |MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
#    ReportStatusCodeLib         |MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    SerialPortLib               |QemuQuadPkg/Library/SerialPortLib/SerialPortLib.inf
    ReportStatusCodeLib         |MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf


###################################################################################################
#
# DXE_RUNTIME_DRIVERs
#
###################################################################################################
[Components]
    MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf  {
        <LibraryClasses>
        MemoryAllocationLib     |MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
        PeCoffLib               |MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
        PeCoffExtraActionLib    |MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
    }

    MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf {
        <LibraryClasses>
        CapsuleLib              |MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
    }

    EmbeddedPkg/EmbeddedMonotonicCounter/EmbeddedMonotonicCounter.inf

    EmbeddedPkg/ResetRuntimeDxe/ResetRuntimeDxe.inf {
        <LibraryClasses>
        EfiResetSystemLib       |QemuQuadPkg/Library/ResetSystemLib/ResetSystemLib.inf
    }

    EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf {
        <LibraryClasses>
        RealTimeClockLib        |QemuQuadPkg/Library/RealTimeClockLib/RealTimeClockLib.inf
    }

    QemuQuadPkg/Drivers/StoreRunDxe/StoreRunDxe.inf

    MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf

    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
        <LibraryClasses>
        TpmMeasurementLib       |MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
        AuthVariableLib         |MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
        VarCheckLib             |MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
    }

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
    DxeServicesTableLib         |MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
    MemoryAllocationLib         |MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    DevicePathLib               |MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
    HobLib                      |MdePkg/Library/DxeHobLib/DxeHobLib.inf
    PcdLib                      |MdePkg/Library/DxePcdLib/DxePcdLib.inf

#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    ReportStatusCodeLib         |MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    SerialPortLib               |QemuQuadPkg/Library/SerialPortLib/SerialPortLib.inf
    ReportStatusCodeLib         |MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/RuntimeDxeReportStatusCodeLib.inf

###################################################################################################
#
# UEFI_DRIVERs
#
###################################################################################################
[Components]
    MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
    MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
    MdeModulePkg/Universal/FvSimpleFileSystemDxe/FvSimpleFileSystemDxe.inf
    MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
    MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
    FatPkg/EnhancedFatDxe/Fat.inf
    QemuQuadPkg/Drivers/HiiResources/HiiResourcesDxe.inf  {
        <LibraryClasses>
        UefiHiiServicesLib      |MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
        HiiLib                  |MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
    }

[LibraryClasses.common.UEFI_DRIVER]
    DevicePathLib               |MdePkg/Library/UefiDevicePathLibDevicePathProtocol/UefiDevicePathLibDevicePathProtocol.inf
    MemoryAllocationLib         |MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    PcdLib                      |MdePkg/Library/DxePcdLib/DxePcdLib.inf

#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    ReportStatusCodeLib         |MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    SerialPortLib               |QemuQuadPkg/Library/SerialPortLib/SerialPortLib.inf
    ReportStatusCodeLib         |MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf

###################################################################################################
#
# UEFI_APPLICATIONs
#
###################################################################################################
[Components]
    ShellBinPkg/UefiShell/UefiShell.inf

[LibraryClasses.common.UEFI_APPLICATION]
    PcdLib                      |MdePkg/Library/DxePcdLib/DxePcdLib.inf
#DEBUG OFF
#    DebugLib                    |MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
#    ReportStatusCodeLib         |MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
#DEBUG ON
    DebugLib                    |MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib     |MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    SerialPortLib               |QemuQuadPkg/Library/SerialPortLib/SerialPortLib.inf
    ReportStatusCodeLib         |MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
