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
#ifndef __STORERUNDXE_H
#define __STORERUNDXE_H

#include <Uefi.h>

#include <Pi/PiFirmwareVolume.h>
#include <Pi/PiHob.h>

#include <Udoo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/IMX6/IMX6GptLib.h>
#include <Library/IMX6/IMX6UsdhcLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/DevicePath.h>

#include <Guid/VariableFormat.h>
#include <Guid/SystemNvDataGuid.h>


typedef struct {
    VENDOR_DEVICE_PATH  Vendor;
    UINT32              Signature;
    EFI_DEVICE_PATH     End;
} CARD_DEVICE_PATH;

#define SIZEOF_CARD_DEVICE_PATH     (sizeof(CARD_DEVICE_PATH) - sizeof(EFI_DEVICE_PATH))

typedef struct _CARDINSTANCE
{
    EFI_BLOCK_IO_PROTOCOL               Protocol;       // must be first thing in structure (&Protocol == &CARDINSTANCE)
    EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL Protocol2;
    EFI_BLOCK_IO_MEDIA                  Media;
    CARD_DEVICE_PATH                    DevicePath;
    IMX6_GPT                            Gpt;
    IMX6_USDHC                          Usdhc;
} CARDINSTANCE;

extern UINTN            gShadowBase;
extern UINT8 *          gpTempBuf;
extern CARDINSTANCE *   gpCardInst;

EFI_STATUS  UdooStoreRunDxe_Init(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void        UdooStoreRunDxe_Update(void);

#endif // __STORERUNDXE_H