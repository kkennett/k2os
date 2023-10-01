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
#ifndef __K2OSDDK_H
#define __K2OSDDK_H
 
#include "k2oskern.h"

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

typedef struct _K2OS_DEVCTX_OPAQUE K2OS_DEVCTX_OPAQUE;
typedef K2OS_DEVCTX_OPAQUE *    K2OS_DEVCTX;

typedef struct _K2OSDDK_RESDEF K2OSDDK_RESDEF;
struct _K2OSDDK_RESDEF
{
    UINT32  mType;
    UINT32  mId;
    union {
        struct {
            K2OS_IOPORT_RANGE   Range;
        } Io;
        struct {
            K2OS_PHYSADDR_RANGE Range;
        } Phys;
        struct {
            K2OS_IRQ_CONFIG     Config;
        } Irq;
    };
};

typedef struct _K2OSDDK_RES K2OSDDK_RES;
struct _K2OSDDK_RES
{
    K2OSDDK_RESDEF  Def;
    union {
        struct {
            K2OS_PAGEARRAY_TOKEN    mTokPageArray;
        } Phys;
        struct {
            K2OS_INTERRUPT_TOKEN    mTokInterrupt;
        } Irq;
    };
};

#define K2OSDDK_MOUNTCHILD_FLAGS_BUSTYPE_MASK   0x0000000F

K2STAT K2OSDDK_GetInstanceInfo(K2OS_DEVCTX aDevCtx, K2_DEVICE_IDENT *apRetIdent, UINT32 *apRetCountIo, UINT32 *apRetCountPhys, UINT32 *apRetCountIrq);
K2STAT K2OSDDK_GetRes(K2OS_DEVCTX aDevCtx, UINT32 aResType, UINT32 aIndex, K2OSDDK_RES *apRetRes);
K2STAT K2OSDDK_MountChild(K2OS_DEVCTX aDevCtx, UINT32 aFlagsAndBusType, UINT64 const *apBusSpecificAddress, K2_DEVICE_IDENT const *apIdent, UINT32 *apRetChildInstanceId);
K2STAT K2OSDDK_SetEnable(K2OS_DEVCTX aDevCtx, BOOL aSetEnable);
K2STAT K2OSDDK_RunAcpiMethod(K2OS_DEVCTX aDevCtx, UINT32 aMethod, UINT32 aFlags, UINT32 aInput, UINT32 *apRetResult);
K2STAT K2OSDDK_AddChildRes(K2OS_DEVCTX aDevCtx, UINT32 aChildInstanceId, K2OSDDK_RESDEF const *apResDef);
K2STAT K2OSDDK_DriverStarted(K2OS_DEVCTX aDevCtx);
UINT32 K2OSDDK_DriverStopped(K2OS_DEVCTX aDevCtx, K2STAT aResult);

//
//------------------------------------------------------------------------
//

typedef struct _K2OS_BLOCKIO_CONFIG K2OS_BLOCKIO_CONFIG;
struct _K2OS_BLOCKIO_CONFIG
{
    BOOL    mUseHwDma;
};

typedef enum _K2OS_BlockIoTransferType K2OS_BlockIoTransferType;
enum _K2OS_BlockIoTransferType
{
    K2OS_BlockIoTransfer_Invalid = 0,

    K2OS_BlockIoTransfer_Read = 1,
    K2OS_BlockIoTransfer_Write = 2,
    K2OS_BlockIoTransfer_Erase = 3,

    K2OS_BlockIoTransferType_Count
};

typedef struct _K2OS_BLOCKIO_TRANSFER K2OS_BLOCKIO_TRANSFER;
struct _K2OS_BLOCKIO_TRANSFER
{
    K2OS_BlockIoTransferType    mType;
    UINT32                      mStartBlock;
    UINT32                      mBlockCount;
    UINT32                      mTargetAddr;    // phys addr if using hw dma, otherwise virtual cached addr
};

typedef K2STAT (*K2OSDDK_pf_BlockIo_GetMedia)(void *apDevice, K2OS_STORAGE_MEDIA *apRetMedia);
typedef K2STAT (*K2OSDDK_pf_BlockIo_Transfer)(void *apDevice, K2OS_BLOCKIO_TRANSFER const *apTransfer);

typedef struct _K2OSDDK_BLOCKIO_REGISTER K2OSDDK_BLOCKIO_REGISTER;
struct _K2OSDDK_BLOCKIO_REGISTER
{
    K2OS_BLOCKIO_CONFIG             Config;
    K2OSDDK_pf_BlockIo_GetMedia     GetMedia;
    K2OSDDK_pf_BlockIo_Transfer     Transfer;
};

K2STAT K2OSDDK_BlockIoRegister(K2OS_DEVCTX aDevCtx, void *apDevice, K2OSDDK_BLOCKIO_REGISTER const *apRegister);
K2STAT K2OSDDK_BlockIoDeregister(K2OS_DEVCTX aDevCtx, void *apDevice);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif


#endif // __K2OSDDK_H
