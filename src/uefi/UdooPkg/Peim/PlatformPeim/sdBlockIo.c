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

static
EFI_STATUS
sPpiGetNumDevices(
    IN  EFI_PEI_SERVICES               **PeiServices,
    IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
    OUT UINTN                          *NumberBlockDevices
    )
{
    *NumberBlockDevices = 1;
    return EFI_SUCCESS;
}

static
EFI_STATUS
sPpiGetMediaInfo(
    IN  EFI_PEI_SERVICES               **PeiServices,
    IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
    IN  UINTN                          DeviceIndex,
    OUT EFI_PEI_BLOCK_IO_MEDIA         *MediaInfo
)
{
    if (DeviceIndex != 1)
        return EFI_UNSUPPORTED;
    CopyMem(MediaInfo, &gCardInst.Media, sizeof(gCardInst.Media));
    return EFI_SUCCESS;
}

static
EFI_STATUS
sPpiCardReadBlocks(
    IN  EFI_PEI_SERVICES               **PeiServices,
    IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
    IN  UINTN                          DeviceIndex,
    IN  EFI_PEI_LBA                    StartLBA,
    IN  UINTN                          BufferSizeBytes,
    OUT VOID                           *Buffer
)
{
    if (DeviceIndex != 1)
        return EFI_UNSUPPORTED;
    return IMX6_USDHC_DoTransfer(&gCardInst.Usdhc, FALSE, StartLBA, BufferSizeBytes, Buffer);
}

static EFI_PEI_RECOVERY_BLOCK_IO_PPI sgBlockIoPpi =
{
    sPpiGetNumDevices,
    sPpiGetMediaInfo,
    sPpiCardReadBlocks
};

static CONST EFI_PEI_PPI_DESCRIPTOR sgBlockIoPpiDesc =
{
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiVirtualBlockIoPpiGuid,
    &sgBlockIoPpi
};

EFI_STATUS
CardBlockIo_Init(
    void
)
{
    EFI_STATUS Status;
    
    IMX6_GPT_Init(&gCardInst.Gpt, IMX6_PHYSADDR_CCM, IMX6_PHYSADDR_GPT);

    if (!IMX6_USDHC_Setup(&gCardInst.Usdhc, UDOO_BOOT_USDHC_UNITNUM, IMX6_PHYSADDR_CCM, &gCardInst.Gpt))
    {
        return EFI_DEVICE_ERROR;
    }

    if (!IMX6_USDHC_Initialize(&gCardInst.Usdhc))
    {
        return EFI_DEVICE_ERROR;
    }

    gCardInst.Media.DeviceType = SD;
    gCardInst.Media.MediaPresent = TRUE;
    gCardInst.Media.LastBlock = gCardInst.Usdhc.Info.BlockCount - 1;
    gCardInst.Media.BlockSize = gCardInst.Usdhc.Info.BlockSize;

    Status = PeiServicesInstallPpi(&sgBlockIoPpiDesc);
//    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    //
    // leave a copy of the Usdhc setup in a hob for DXE to find
    //
    BuildGuidDataHob(&gUdooPeiBootCardMediaGuid, &gCardInst.Usdhc, sizeof(gCardInst.Usdhc));

    return Status;
}

