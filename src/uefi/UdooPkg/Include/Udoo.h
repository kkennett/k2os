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

#ifndef __UDOO_H__
#define __UDOO_H__

#include <Uefi/UefiBaseType.h>
#include "UdooDef.inc"
#include <Library/IMX6/IMX6UsdhcLib.h>

extern EFI_GUID gUdooTokenSpaceGuid;
extern EFI_GUID gVarRunDxeFileGuid;
extern EFI_GUID gEfiPeiDxeFvLoadPpiGuid;
extern EFI_GUID gEfiPeiSysPartFatAvailGuid;

extern EFI_GUID gUdooPeiBootCardMediaGuid;
 
typedef enum {
    UDOO_EFI_PEI_BLOCK_DEVICE_TYPE_SD = 4
} UDOO_EFI_PEI_BLOCK_DEVICE_TYPE;

typedef struct {
    UDOO_EFI_PEI_BLOCK_DEVICE_TYPE  DeviceType;
    BOOLEAN                         MediaPresent;
    UINTN                           LastBlock;
    UINTN                           BlockSize;
} UDOO_EFI_PEI_BLOCK_IO_MEDIA;

#endif
