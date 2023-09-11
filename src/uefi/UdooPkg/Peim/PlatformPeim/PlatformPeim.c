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
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "PlatformPeim.h"

SDCARD_INST gCardInst;

typedef int _check_varshadow[(UDOO_PHYSSIZE_VARSHADOW == 0x4F000) ? 1 : -1];

void MmioSetBits32(UINT32 Addr, UINT32 Bits)
{
    MmioWrite32(Addr, MmioRead32(Addr) | Bits);
}

void MmioClrBits32(UINT32 Addr, UINT32 Bits)
{
    MmioWrite32(Addr, MmioRead32(Addr) & ~Bits);
}

static CONST EFI_PEI_PPI_DESCRIPTOR sgMasterBootModePpi = 
{
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiMasterBootModePpiGuid,
    NULL
};

static
EFI_STATUS
EFIAPI
sNotifySysPartFatAvail(
    IN EFI_PEI_SERVICES           **PeiServices,
    IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
    IN VOID                       *Ppi
)
{
    EFI_STATUS                      Status;
    EFI_PEI_DXE_FV_LOAD_PPI *       LoadPpi;
    UINTN                           DxeFvSize;
    UINTN                           MemPages;
    EFI_PHYSICAL_ADDRESS            DxeFvAddr;
    EFI_FIRMWARE_VOLUME_HEADER *    pVolHeader;
    EFI_HOB_FIRMWARE_VOLUME  *      Hob;

    Status = PeiServicesLocatePpi(&gEfiPeiDxeFvLoadPpiGuid, 0, NULL, (void **)&LoadPpi);
//    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    DxeFvSize = 0;
    Status = LoadPpi->GetInfo(PeiServices, LoadPpi, &DxeFvSize);
    if (EFI_ERROR(Status))
        return Status;
    //    ASSERT_EFI_ERROR(Status);
//    ASSERT(0 < DxeFvSize);
    if (0 == DxeFvSize)
        return EFI_DEVICE_ERROR;

    MemPages = (DxeFvSize + 4095) / 4096;
    Status = PeiServicesAllocatePages(
        EfiBootServicesCode,
        MemPages,
        &DxeFvAddr
    );
//    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    Status = LoadPpi->Load(PeiServices, LoadPpi, (void *)(UINTN)DxeFvAddr);
//    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    pVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)DxeFvAddr;
    if (pVolHeader->Signature != EFI_FVH_SIGNATURE)
    {
//        DEBUG((EFI_D_ERROR, "*** Loaded Dxe Fv does not have EFI FV header signature.\n"));
        return EFI_NOT_FOUND;
    }

    Status = PeiServicesCreateHob(EFI_HOB_TYPE_FV, (UINT16)sizeof(EFI_HOB_FIRMWARE_VOLUME), (void **)&Hob);
//    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    Hob->BaseAddress = DxeFvAddr;
    Hob->Length = pVolHeader->FvLength;

    PeiServicesInstallFvInfoPpi(
        &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)DxeFvAddr)->FileSystemGuid),
        (VOID *)(UINTN)DxeFvAddr,
        (UINT32)pVolHeader->FvLength,
        NULL,
        NULL
    );

    //
    // installing this PPI descriptor will allow dxeipl to load
    // and execute
    //
    Status = PeiServicesInstallPpi(&sgMasterBootModePpi);
//    ASSERT_EFI_ERROR(Status);

    return Status;
}

static CONST EFI_PEI_NOTIFY_DESCRIPTOR sgNotifySysPartFatAvailPpi =
{
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiSysPartFatAvailGuid,
    sNotifySysPartFatAvail
};

static void sInitSDHCHw(void)
{
    UINT32 regVal;

    // all SD pads have this config
    regVal =
        IMX6_MUX_PADCTL_HYS |
        IMX6_MUX_PADCTL_PUS_47KOHM_PU |
        IMX6_MUX_PADCTL_SPEED_50MHZ |
        IMX6_MUX_PADCTL_DSE_80_OHM |
        IMX6_MUX_PADCTL_SRE_FAST;

    // USDHC3
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_CLK, 0);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_CMD, 0);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA0, 0);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA1, 0);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA2, 0);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA3, 0);

    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_CLK, regVal);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_CMD, regVal);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA0, regVal);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA1, regVal);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA2, regVal);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA3, regVal);

    regVal = IMX6_MUX_PADCTL_SPEED_100MHZ |
        IMX6_MUX_PADCTL_PKE |
        IMX6_MUX_PADCTL_DSE_40_OHM |
        IMX6_MUX_PADCTL_SRE_SLOW;

#if !WANDBOARD
    //
    // CD is on SD3_DAT5 - GPIO7_IO00, input
    //
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA5, 5);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA5, regVal);
    MmioClrBits32(IMX6_PHYSADDR_GPIO7_GDIR, 1 << 0);

    //
    // PWR is on NANDF_D5 - GPIO2_IO05, output 
    //
    MmioSetBits32(IMX6_PHYSADDR_GPIO2_DR, 1 << 5);
    MmioSetBits32(IMX6_PHYSADDR_GPIO2_GDIR, 1 << 5);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_DATA05, regVal);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_DATA05, 5);
#else
    //
    // CD is on EIM_DA9 - GPIO3_IO09, input
    //
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD09, 5);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD09, regVal);
    MmioClrBits32(IMX6_PHYSADDR_GPIO3_GDIR, 1 << 9);

    //
    // PWR is AO on Wandboard
    //
#endif
};

static
void
sInitWatchdog(
    void
)
{
    // set values to defaults for both watchdogs
    MmioWrite16(IMX6_PHYSADDR_WDOG2_WCR, 0x0030);
    MmioWrite16(IMX6_PHYSADDR_WDOG2_WSR, 0x0000);
    MmioWrite16(IMX6_PHYSADDR_WDOG2_WICR, 0x0004);
    MmioWrite16(IMX6_PHYSADDR_WDOG2_WMCR, 0x0000);

    MmioWrite16(IMX6_PHYSADDR_WDOG1_WCR, 0x0030);
    MmioWrite16(IMX6_PHYSADDR_WDOG1_WSR, 0x0000);
    MmioWrite16(IMX6_PHYSADDR_WDOG1_WICR, 0x0004);
    MmioWrite16(IMX6_PHYSADDR_WDOG1_WMCR, 0x0000);
}

static
EFI_STATUS
sValidateFvHeader(
    void
)
{
    UINT16                      Checksum;
    EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
    VARIABLE_STORE_HEADER       *VariableStoreHeader;
    UINTN                       VariableStoreLength;
    UINTN                       FwVolLength;

    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER*)UDOO_PHYSADDR_VARSHADOW;
    FwVolLength = PcdGet32(PcdFlashNvStorageVariableSize) +
        PcdGet32(PcdFlashNvStorageFtwWorkingSize) +
        PcdGet32(PcdFlashNvStorageFtwSpareSize);

    //
    // Verify the header revision, header signature, length
    // Length of FvBlock cannot be 2**64-1
    // HeaderLength cannot be an odd number
    //
    if ((FwVolHeader->Revision != EFI_FVH_REVISION)
        || (FwVolHeader->Signature != EFI_FVH_SIGNATURE)
        || (FwVolHeader->FvLength != FwVolLength)
        )
    {
        return EFI_NOT_FOUND;
    }

    // Check the Firmware Volume Guid
    if (CompareGuid(&FwVolHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid) == FALSE)
    {
        return EFI_NOT_FOUND;
    }

    // Verify the header checksum
    Checksum = CalculateSum16((UINT16*)FwVolHeader, FwVolHeader->HeaderLength);
    if (Checksum != 0)
    {
        return EFI_NOT_FOUND;
    }

    VariableStoreHeader = (VARIABLE_STORE_HEADER*)((UINTN)FwVolHeader + FwVolHeader->HeaderLength);

    // Check the Variable Store Guid
    if (!CompareGuid(&VariableStoreHeader->Signature, &gEfiVariableGuid))
    {
        return EFI_NOT_FOUND;
    }

    VariableStoreLength = PcdGet32(PcdFlashNvStorageVariableSize) - FwVolHeader->HeaderLength;
    if (VariableStoreHeader->Size != VariableStoreLength)
    {
        return EFI_NOT_FOUND;
    }

    return EFI_SUCCESS;
}

#define HEADERS_LEN (sizeof(EFI_FIRMWARE_VOLUME_HEADER) + sizeof(EFI_FV_BLOCK_MAP_ENTRY) + sizeof(VARIABLE_STORE_HEADER))
static UINT8 VolInitTemp[((HEADERS_LEN + (SD_BLOCK_SIZE-1)) / SD_BLOCK_SIZE) * SD_BLOCK_SIZE];

static
EFI_STATUS
sInitializeFvAndVariableStoreHeaders(
    void
)
{
    EFI_STATUS                          Status;
    EFI_FIRMWARE_VOLUME_HEADER          *FwVolHeader;
    VOID *                              Headers;
    VARIABLE_STORE_HEADER               *VariableStoreHeader;

    Headers = VolInitTemp;
    ZeroMem(VolInitTemp, sizeof(VolInitTemp));

    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER*)Headers;
    CopyGuid(&FwVolHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid);
    FwVolHeader->FvLength =
        PcdGet32(PcdFlashNvStorageVariableSize) +
        PcdGet32(PcdFlashNvStorageFtwWorkingSize) +
        PcdGet32(PcdFlashNvStorageFtwSpareSize);
    FwVolHeader->Signature = EFI_FVH_SIGNATURE;
    FwVolHeader->Attributes = (EFI_FVB_ATTRIBUTES_2)(
        EFI_FVB2_READ_ENABLED_CAP | // Reads may be enabled
        EFI_FVB2_READ_STATUS |      // Reads are currently enabled
        EFI_FVB2_STICKY_WRITE |     // A block erase is required to flip bits into EFI_FVB2_ERASE_POLARITY
        EFI_FVB2_MEMORY_MAPPED |    // It is memory mapped
        EFI_FVB2_ERASE_POLARITY |   // After erasure all bits take this value (i.e. '1')
        EFI_FVB2_WRITE_STATUS |     // Writes are currently enabled
        EFI_FVB2_WRITE_ENABLED_CAP  // Writes may be enabled
        );
    FwVolHeader->HeaderLength = sizeof(EFI_FIRMWARE_VOLUME_HEADER) + sizeof(EFI_FV_BLOCK_MAP_ENTRY);
    FwVolHeader->Revision = EFI_FVH_REVISION;
    FwVolHeader->BlockMap[0].NumBlocks = UDOO_PHYSSIZE_VARSHADOW / SD_BLOCK_SIZE;
    FwVolHeader->BlockMap[0].Length = SD_BLOCK_SIZE;
    FwVolHeader->BlockMap[1].NumBlocks = 0;
    FwVolHeader->BlockMap[1].Length = 0;
    FwVolHeader->Checksum = CalculateCheckSum16((UINT16*)FwVolHeader, FwVolHeader->HeaderLength);

    //
    // VARIABLE_STORE_HEADER
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER*)((UINTN)FwVolHeader + FwVolHeader->HeaderLength);
    CopyGuid(&VariableStoreHeader->Signature, &gEfiVariableGuid);
    VariableStoreHeader->Size = PcdGet32(PcdFlashNvStorageVariableSize) - FwVolHeader->HeaderLength;
    VariableStoreHeader->Format = VARIABLE_STORE_FORMATTED;
    VariableStoreHeader->State = VARIABLE_STORE_HEALTHY;

    // Install the combined header onto the card
    Status = IMX6_USDHC_DoTransfer(&gCardInst.Usdhc, TRUE, 
        UDOO_PHYSSIZE_SEC_FD / SD_BLOCK_SIZE, 
        sizeof(VolInitTemp),
        (VOID *)VolInitTemp);
//    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    // successful write - update shadow of what is on the card
    CopyMem((VOID *)UDOO_PHYSADDR_VARSHADOW, VolInitTemp, sizeof(VolInitTemp));

    return Status;
}

static
EFI_STATUS
sInitVarStore(
    void
)
{
    EFI_STATUS Status;

    //
    // load the variable store content to the ram buffer.  var store is located right after the sec fd on the card
    //
    Status = IMX6_USDHC_DoTransfer(&gCardInst.Usdhc, 
        FALSE, 
        UDOO_PHYSSIZE_SEC_FD / SD_BLOCK_SIZE, 
        UDOO_PHYSSIZE_VARSHADOW, 
        (VOID *)UDOO_PHYSADDR_VARSHADOW);
//    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    //
    // validate FVB header is healthy at least
    //
    Status = sValidateFvHeader();
    if (EFI_ERROR(Status))
    {
//        DEBUG((EFI_D_INFO, 
        DebugPrint(0xFFFFFFFF, "Variable Store is blank or invalid.  (re)Init takes a few seconds...\n");
//            );
        Status = sInitializeFvAndVariableStoreHeaders();
        if (EFI_ERROR(Status))
            return Status;
        Status = sValidateFvHeader();
//        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            return Status;
    }
    
    return Status;
}

EFI_STATUS
EFIAPI
InitializePlatformPeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS Status;

  sInitSDHCHw();
  sInitWatchdog();

  ZeroMem(&gCardInst, sizeof(gCardInst));

  Status = PeiServicesSetBootMode(BOOT_WITH_FULL_CONFIGURATION);
//  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR(Status))
  {
      Status = PeiServicesNotifyPpi(&sgNotifySysPartFatAvailPpi);
//      ASSERT_EFI_ERROR(Status);
      if (!EFI_ERROR(Status))
      {
          Status = CardBlockIo_Init();
//          ASSERT_EFI_ERROR(Status);
          if (!EFI_ERROR(Status))
          {
              Status = sInitVarStore();
//              ASSERT_EFI_ERROR(Status);
          }
      }
  }

  return Status;
}
