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

#ifndef __QEMUQUAD_PLATFORM_PEIM_H
#define __QEMUQUAD_PLATFORM_PEIM_H

#include <PiPei.h>
#include <Ppi/DxeFvLoadPpi.h>
#include <Ppi/BlockIo.h>

#include <Guid\VariableFormat.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/CacheMaintenanceLib.h>

#include <QemuQuad.h>

#include <Library/IMX6/IMX6ClockLib.h>
#include <Library/IMX6/IMX6GptLib.h>
#include <Library/IMX6/IMX6UsdhcLib.h>

typedef struct _SDCARD_INST SDCARD_INST;
struct _SDCARD_INST
{
    IMX6_GPT                        Gpt;
    IMX6_USDHC                      Usdhc;
    QEMUQUAD_EFI_PEI_BLOCK_IO_MEDIA Media;
};

extern SDCARD_INST gCardInst;

EFI_STATUS CardBlockIo_Init(void);

void MmioSetBits32(UINT32 Addr, UINT32 Bits);
void MmioClrBits32(UINT32 Addr, UINT32 Bits);

EFI_STATUS EFIAPI InitializePlatformPeim(IN EFI_PEI_FILE_HANDLE FileHandle, IN CONST EFI_PEI_SERVICES **PeiServices);

#endif