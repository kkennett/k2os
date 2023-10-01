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
#ifndef __RAMDISK_H
#define __RAMDISK_H

#include <k2osddk.h>
#include <k2osdev_blockio.h>

/* ------------------------------------------------------------------------- */

typedef struct _RAMDISK_DEVICE RAMDISK_DEVICE;
struct _RAMDISK_DEVICE
{
    K2OS_DEVCTX         mDevCtx;
    K2_DEVICE_IDENT     Ident;

    UINT32              mVirtBase;
    UINT32              mPageCount;
    K2OS_VIRTMAP_TOKEN  mTokVirtMap;
};

K2STAT RAMDISK_GetMedia(RAMDISK_DEVICE *apDevice, K2OS_STORAGE_MEDIA *apRetMedia);
K2STAT RAMDISK_Transfer(RAMDISK_DEVICE *apDevice, K2OS_BLOCKIO_TRANSFER const *apTransfer);

/* ------------------------------------------------------------------------- */

#endif // __RAMDISK_H