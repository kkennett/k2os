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
#ifndef __STORERUN_H
#define __STORERUN_H

#include <k2osddk.h>
#include <k2osdev_blockio.h>

/* ------------------------------------------------------------------------- */

typedef struct _STORERUN_DEVICE STORERUN_DEVICE;
struct _STORERUN_DEVICE
{
    K2OS_DEVCTX                     mDevCtx;
    K2_DEVICE_IDENT                 Ident;

    K2EFI_BLOCK_IO_PROTOCOL *       mpProto;

    K2OSDDK_pf_BlockIo_NotifyKey *  mpNotifyKey;
};

K2STAT STORERUN_GetMedia(STORERUN_DEVICE *apDevice, K2OS_STORAGE_MEDIA *apRetMedia);
K2STAT STORERUN_Transfer(STORERUN_DEVICE *apDevice, K2OS_BLOCKIO_TRANSFER const *apTransfer);

/* ------------------------------------------------------------------------- */

#endif // __STORERUN_H