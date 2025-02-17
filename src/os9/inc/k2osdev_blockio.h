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
#ifndef __K2OSDEV_BLOCKIO_H
#define __K2OSDEV_BLOCKIO_H

#include <k2osdev.h>

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

// {5D2DAF34-2B80-48DC-8ED5-DB4E1DDA4055}
#define K2OS_IFACE_BLOCKIO_DEVICE   { 0x5d2daf34, 0x2b80, 0x48dc, { 0x8e, 0xd5, 0xdb, 0x4e, 0x1d, 0xda, 0x40, 0x55 } }

typedef enum _K2OS_BlockIo_Method K2OS_BlockIo_Method;
enum _K2OS_BlockIo_Method
{
    K2OS_BlockIo_Method_Invalid = 0,

    K2OS_BlockIo_Method_Config,
    K2OS_BlockIo_Method_GetMedia,
    K2OS_BlockIo_Method_RangeCreate,
    K2OS_BlockIo_Method_RangeDelete,
    K2OS_BlockIo_Method_Transfer,

    K2OS_BlockIo_Method_Count
};

typedef enum _K2OS_BlockIo_Notify K2OS_BlockIo_Notify;
enum _K2OS_BlockIo_Notify
{
    K2OS_BlockIo_Notify_Invalid = 0,

    K2OS_BlockIo_Notify_MediaChanged,

    K2OS_BlockIo_Notify_Count
};

typedef struct _K2OSSTOR_BLOCKIO_OPAQUE K2OSSTOR_BLOCKIO_OPAQUE;
typedef K2OSSTOR_BLOCKIO_OPAQUE *       K2OSSTOR_BLOCKIO;

typedef struct _K2OSSTOR_BLOCKIO_RANGE_OPAQUE   K2OSSTOR_BLOCKIO_RANGE_OPAQUE;
typedef K2OSSTOR_BLOCKIO_RANGE_OPAQUE *         K2OSSTOR_BLOCKIO_RANGE;

typedef struct _K2OS_BLOCKIO_CONFIG_IN K2OS_BLOCKIO_CONFIG_IN;
struct _K2OS_BLOCKIO_CONFIG_IN
{
    UINT32  mAccess;
    UINT32  mShare;
};

typedef struct _K2OS_BLOCKIO_RANGE_CREATE_IN K2OS_BLOCKIO_RANGE_CREATE_IN;
struct _K2OS_BLOCKIO_RANGE_CREATE_IN
{
    UINT64  mRangeBaseBlock;
    UINT64  mRangeBlockCount;
    BOOL    mMakePrivate;
};
typedef struct _K2OS_BLOCKIO_RANGE_CREATE_OUT K2OS_BLOCKIO_RANGE_CREATE_OUT;
struct _K2OS_BLOCKIO_RANGE_CREATE_OUT
{
    K2OSSTOR_BLOCKIO_RANGE  mRange;
};

typedef struct _K2OS_BLOCKIO_RANGE_DELETE_IN K2OS_BLOCKIO_RANGE_DELETE_IN;
struct _K2OS_BLOCKIO_RANGE_DELETE_IN
{
    K2OSSTOR_BLOCKIO_RANGE  mRange;
};

typedef struct _K2OS_BLOCKIO_TRANSFER_IN K2OS_BLOCKIO_TRANSFER_IN;
struct _K2OS_BLOCKIO_TRANSFER_IN
{
    UINT64                  mBytesOffset;
    UINT32                  mByteCount;
    UINT32                  mMemAddr;
    K2OSSTOR_BLOCKIO_RANGE  mRange;
    BOOL                    mIsWrite;
};

K2OSSTOR_BLOCKIO        K2OS_BlockIo_Attach(K2OS_IFINST_ID aIfInstId, UINT32 aAccess, UINT32 aShare, K2OS_MAILBOX_TOKEN aTokNotifyMailbox);
BOOL                    K2OS_BlockIo_GetMedia(K2OSSTOR_BLOCKIO aStorBlockIo, K2OS_STORAGE_MEDIA *apRetMedia);
K2OSSTOR_BLOCKIO_RANGE  K2OS_BlockIo_RangeCreate(K2OSSTOR_BLOCKIO aStorBlockIo, UINT64 const *apRangeBaseBlock, UINT64 const *apRangeBlockCount, BOOL aMakePrivate);
BOOL                    K2OS_BlockIo_RangeDelete(K2OSSTOR_BLOCKIO aStorBlockIo, K2OSSTOR_BLOCKIO_RANGE aRange);
BOOL                    K2OS_BlockIo_Read(K2OSSTOR_BLOCKIO aStorBlockIo, K2OSSTOR_BLOCKIO_RANGE aRange, UINT64 const *apBytesOffset, void *apBuffer, UINT32 aByteCount);
BOOL                    K2OS_BlockIo_Write(K2OSSTOR_BLOCKIO aStorBlockIo, K2OSSTOR_BLOCKIO_RANGE aRange, UINT64 const *apBytesOffset, void const *apBuffer, UINT32 aByteCount);
BOOL                    K2OS_BlockIo_Detach(K2OSSTOR_BLOCKIO aStorBlockIo);

//
//------------------------------------------------------------------------
//


#if __cplusplus
}
#endif

#endif // __K2OSDEV_BLOCKIO_H