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

#include <PiPei.h>
#include <Udoo.h>

#include <Guid/MemoryTypeInformation.h>

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/ArmMmuLib.h>

VOID
BuildMemoryTypeInformationHob (
  VOID
  )
{
  EFI_MEMORY_TYPE_INFORMATION   Info[10];

  Info[0].Type          = EfiACPIReclaimMemory;
  Info[0].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiACPIReclaimMemory);
  Info[1].Type          = EfiACPIMemoryNVS;
  Info[1].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiACPIMemoryNVS);
  Info[2].Type          = EfiReservedMemoryType;
  Info[2].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiReservedMemoryType);
  Info[3].Type          = EfiRuntimeServicesData;
  Info[3].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiRuntimeServicesData);
  Info[4].Type          = EfiRuntimeServicesCode;
  Info[4].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiRuntimeServicesCode);
  Info[5].Type          = EfiBootServicesCode;
  Info[5].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiBootServicesCode);
  Info[6].Type          = EfiBootServicesData;
  Info[6].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiBootServicesData);
  Info[7].Type          = EfiLoaderCode;
  Info[7].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiLoaderCode);
  Info[8].Type          = EfiLoaderData;
  Info[8].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiLoaderData);

  // Terminator for the list
  Info[9].Type          = EfiMaxMemoryType;
  Info[9].NumberOfPages = 0;

  BuildGuidDataHob (&gEfiMemoryTypeInformationGuid, &Info, sizeof (Info));
}

#define PEI_MEMORY_SIZE 0x01000000

static ARM_MEMORY_REGION_DESCRIPTOR MemoryTable[] =
{
    {
        IMX6_PHYSADDR_BOOTROM,
        IMX6_PHYSADDR_BOOTROM,
        IMX6_PHYSSIZE_BOOTROM,
        ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED
    },
    {
        IMX6_PHYSADDR_AIPS1,
        IMX6_PHYSADDR_AIPS1,
        IMX6_PHYSSIZE_AIPS1 + IMX6_PHYSSIZE_AIPS2,
        ARM_MEMORY_REGION_ATTRIBUTE_DEVICE
    },
    {
        IMX6_PHYSADDR_ARM_PERIPH,
        IMX6_PHYSADDR_ARM_PERIPH,
        IMX6_PHYSSIZE_ARM_PERIPH + IMX6_PHYSSIZE_PL310,
        ARM_MEMORY_REGION_ATTRIBUTE_DEVICE
    },
    {
        IMX6_PHYSADDR_OCRAM,
        IMX6_PHYSADDR_OCRAM,
        IMX6_PHYSSIZE_OCRAM,
        ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED
    },
    {
        UDOO_PHYSADDR_VARSHADOW,
        UDOO_PHYSADDR_VARSHADOW,
        UDOO_PHYSSIZE_VARSHADOW,
        DDR_ATTRIBUTES_UNCACHED
    },
    {
        UDOO_PHYSADDR_PEIFV_FILE_BASE,
        UDOO_PHYSADDR_PEIFV_FILE_BASE,
        UDOO_MAINRAM_PHYSICAL_LENGTH - (UDOO_PHYSADDR_PEIFV_FILE_BASE - UDOO_MAINRAM_PHYSICAL_BASE),
        DDR_ATTRIBUTES_CACHED
    },
    {
        0,
        0,
        0,
        (ARM_MEMORY_REGION_ATTRIBUTES)0
    }
};

EFI_STATUS
EFIAPI
InitializeMemory(
    IN       EFI_PEI_FILE_HANDLE  FileHandle,
    IN CONST EFI_PEI_SERVICES     **PeiServices
)
{
    EFI_PHYSICAL_ADDRESS            efiMemPhys;
    EFI_FIRMWARE_VOLUME_HEADER *    pVolHeader;
    UINT64                          allocLen;
    VOID *                          TranslationTableBase;
    UINTN                           TranslationTableSize;
    RETURN_STATUS                   Status;

    //
    // declare the OCRAM area
    //
    BuildResourceDescriptorHob(
        EFI_RESOURCE_SYSTEM_MEMORY,
        EFI_RESOURCE_ATTRIBUTE_PRESENT |
            EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
            EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
            EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
            EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
            EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
            EFI_RESOURCE_ATTRIBUTE_TESTED,
        IMX6_PHYSADDR_OCRAM,
        IMX6_PHYSSIZE_OCRAM);

    //
    // note that OCram is completely allocated
    //
    BuildMemoryAllocationHob((EFI_PHYSICAL_ADDRESS)IMX6_PHYSADDR_OCRAM, IMX6_PHYSSIZE_OCRAM, EfiReservedMemoryType);

    //
    // declare system memory, do not include varshadow; it gets added to the GCD domain as a separate
    // region that is used at runtime.
    //
    BuildResourceDescriptorHob(
        EFI_RESOURCE_SYSTEM_MEMORY,
        EFI_RESOURCE_ATTRIBUTE_PRESENT |
            EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
            EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
            EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
            EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
            EFI_RESOURCE_ATTRIBUTE_TESTED,
        UDOO_PHYSADDR_PEIFV_FILE_BASE,
        UDOO_MAINRAM_PHYSICAL_LENGTH - (UDOO_PHYSADDR_PEIFV_FILE_BASE - UDOO_MAINRAM_PHYSICAL_BASE));

    //
    // declare memory allocation for decompressed PEI FV
    //
    pVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UDOO_PHYSADDR_PEIFV_FILE_BASE + 0x1000);
    allocLen = (pVolHeader->FvLength + 0xFFF) & ~0xFFF;
    BuildMemoryAllocationHob((EFI_PHYSICAL_ADDRESS)(UINTN)pVolHeader, allocLen, EfiBootServicesData);
    
    //
    // declare the decompressed PEI FV that sits in in the allocation we just made (where we are currently executing from)
    //
    BuildFvHob((EFI_PHYSICAL_ADDRESS)(UINTN)pVolHeader, pVolHeader->FvLength);

    //
    // install the PEI memory region at the end of the system memory
    //
    efiMemPhys = (((EFI_PHYSICAL_ADDRESS)UDOO_MAINRAM_PHYSICAL_BASE) + UDOO_MAINRAM_PHYSICAL_LENGTH) - PEI_MEMORY_SIZE;
    efiMemPhys &= ~0xFFF;
    Status = PeiServicesInstallPeiMemory(efiMemPhys, PEI_MEMORY_SIZE);
    if (EFI_ERROR(Status))
        return Status;
//    ASSERT_EFI_ERROR(Status);

    //
    // Configure the MMU and turn it on
    //
    Status = ArmConfigureMmu(MemoryTable, &TranslationTableBase, &TranslationTableSize);
//    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    BuildMemoryTypeInformationHob();

    return Status;
}
