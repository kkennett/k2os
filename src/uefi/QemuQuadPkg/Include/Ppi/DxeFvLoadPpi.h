/** @file
  This file declares the Device Recovery Module PPI.

  The interface of this PPI does the following:
    - Reports the number of recovery DXE capsules that exist on the associated device(s)
    - Finds the requested firmware binary capsule
    - Loads that capsule into memory

  A device can be either a group of devices, such as a block device, or an individual device.
  The module determines the internal search order, with capsule number 1 as the highest load
  priority and number N as the lowest priority.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This PPI is defined in UEFI Platform Initialization Specification 1.2 Volume 1:
  Pre-EFI Initalization Core Interface

**/

#ifndef _PEI_DXE_FV_LOAD_PPI_H_
#define _PEI_DXE_FV_LOAD_PPI_H_

#define EFI_PEI_DXE_FV_LOAD_PPI_GUID \
  { \
    0x8e1b4a4c, 0x2dce, 0x4a91, {0x99, 0x64, 0xed, 0x69, 0xfd, 0x54, 0xda, 0xe8 } \
  }

#define EFI_PEI_SYS_PART_FAT_AVAIL_GUID \
  { \
    0xee77e13b, 0x3461, 0x49dc, { 0x81, 0x8d, 0x7b, 0x8d, 0xdc, 0x39, 0x87, 0x9c } \
  }

typedef struct _EFI_PEI_DXE_FV_LOAD_PPI EFI_PEI_DXE_FV_LOAD_PPI;

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_DXE_FV_LOAD_GET_INFO)(
  IN  EFI_PEI_SERVICES **       PeiServices,
  IN  EFI_PEI_DXE_FV_LOAD_PPI * This,
  OUT UINTN *                   Size
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_DXE_FV_LOAD)(
  IN     EFI_PEI_SERVICES **        PeiServices,
  IN     EFI_PEI_DXE_FV_LOAD_PPI *  This,
  OUT    VOID *                     Buffer
  );

struct _EFI_PEI_DXE_FV_LOAD_PPI {
    EFI_PEI_DXE_FV_LOAD_GET_INFO    GetInfo;
    EFI_PEI_DXE_FV_LOAD             Load;
};

extern EFI_GUID gEfiPeiDxeFvLoadPpiGuid;
extern EFI_GUID gEfiPeiSysPartFatAvailGuid;

#endif  /* _PEI_DXE_FV_LOAD_PPI_H_ */
