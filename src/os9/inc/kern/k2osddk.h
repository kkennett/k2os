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
#include <k2osdev.h>
#include <k2osstor.h>
#include <k2osnet.h>

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
    };
};

#define K2OSDDK_MOUNTCHILD_FLAGS_BUSTYPE_MASK   0x0000000F

typedef struct _K2OSDDK_INSTINFO K2OSDDK_INSTINFO;
struct _K2OSDDK_INSTINFO
{
    K2_DEVICE_IDENT Ident;

    UINT32          mBusType;
    K2OS_IFINST_ID  mBusIfInstId;
    UINT32          mBusChildId;

    UINT32          mCountIo;
    UINT32          mCountPhys;
    UINT32          mCountIrq;
};

K2STAT K2OSDDK_GetInstanceInfo(K2OS_DEVCTX aDevCtx, K2OSDDK_INSTINFO *apRetInfo);
K2STAT K2OSDDK_GetRes(K2OS_DEVCTX aDevCtx, UINT32 aResType, UINT32 aIndex, K2OSDDK_RES *apRetRes);
K2STAT K2OSDDK_MountChild(K2OS_DEVCTX aDevCtx, UINT32 aFlagsAndBusType, UINT64 const *apBusSpecificAddress, K2_DEVICE_IDENT const *apIdent, K2OS_IFINST_ID aBusIfInstId, UINT32 *apRetChildInstanceId);
K2STAT K2OSDDK_SetEnable(K2OS_DEVCTX aDevCtx, BOOL aSetEnable);
K2STAT K2OSDDK_AddChildRes(K2OS_DEVCTX aDevCtx, UINT32 aChildInstanceId, K2OSDDK_RESDEF const *apResDef);
K2STAT K2OSDDK_DriverStarted(K2OS_DEVCTX aDevCtx);
UINT32 K2OSDDK_DriverStopped(K2OS_DEVCTX aDevCtx, K2STAT aResult);

K2OS_PAGEARRAY_TOKEN K2OSDDK_PageArray_CreateIo(UINT32 aFlags, UINT32 aPageCountPow2, UINT32 *apRetPhysBase);

K2STAT K2OSDDK_GetPciIrqRoutingTable(K2OS_DEVCTX aDevCtx, void **appRetTable);

//
//------------------------------------------------------------------------
//

// {8A1CA0E4-59B3-435D-B32F-FA08903A3FC8}
#define K2OS_IFACE_ACPIBUS  { 0x8a1ca0e4, 0x59b3, 0x435d, { 0xb3, 0x2f, 0xfa, 0x8, 0x90, 0x3a, 0x3f, 0xc8 } }

#define K2OS_ACPIBUS_METHOD_RUNMETHOD   1
typedef struct _K2OS_ACPIBUS_RUNMETHOD_IN K2OS_ACPIBUS_RUNMETHOD_IN;
struct _K2OS_ACPIBUS_RUNMETHOD_IN
{
    K2OS_DEVCTX mDevCtx;
    UINT32      mMethod;
    UINT32      mFlags;
    UINT32      mInput;
};

typedef struct _K2OS_ACPIBUS_RUNMETHOD_OUT K2OS_ACPIBUS_RUNMETHOD_OUT;
struct _K2OS_ACPIBUS_RUNMETHOD_OUT
{
    UINT32      mResult;
};

//
//------------------------------------------------------------------------
//

// {8EDB0B70-55B9-49F9-AB80-BB086763109A}
#define K2OS_IFACE_PCIBUS   { 0x8edb0b70, 0x55b9, 0x49f9, { 0xab, 0x80, 0xbb, 0x8, 0x67, 0x63, 0x10, 0x9a } }

typedef struct _K2OS_PCIBUS_CFG_LOC K2OS_PCIBUS_CFG_LOC;
struct _K2OS_PCIBUS_CFG_LOC
{
    UINT32  mOffset;
    UINT32  mWidth;
};

typedef struct _K2OS_PCIBUS_CFG_READ_IN K2OS_PCIBUS_CFG_READ_IN;
struct _K2OS_PCIBUS_CFG_READ_IN
{
    UINT32              mBusChildId;    // must be first thing in struct
    K2OS_PCIBUS_CFG_LOC Loc;
};

typedef struct _K2OS_PCIBUS_CFG_WRITE_IN K2OS_PCIBUS_CFG_WRITE_IN;
struct _K2OS_PCIBUS_CFG_WRITE_IN
{
    UINT32              mBusChildId;    // must be first thing in struct
    K2OS_PCIBUS_CFG_LOC Loc;
    UINT64              mValue;
};

#define K2OS_PCIBUS_METHOD_CFG_READ     1   // in = CFG_READ_IN,  out = UINT64
#define K2OS_PCIBUS_METHOD_CFG_WRITE    2   // in = CFG_WRITE_IN, out = nothing

//
//------------------------------------------------------------------------
//

typedef struct _K2OS_BLOCKIO_CONFIG K2OS_BLOCKIO_CONFIG;
struct _K2OS_BLOCKIO_CONFIG
{
    BOOL    mUseHwDma;
};

typedef struct _K2OS_BLOCKIO_TRANSFER K2OS_BLOCKIO_TRANSFER;
struct _K2OS_BLOCKIO_TRANSFER
{
    UINT64          mStartBlock;
    UINT64          mBlockCount;
    UINT32          mAddress;    // phys addr if using hw dma, otherwise virtual cached addr
    BOOL            mIsWrite;
};

typedef K2STAT (*K2OSDDK_pf_BlockIo_GetMedia)(void *apDevice, K2OS_STORAGE_MEDIA *apRetMedia);
typedef K2STAT (*K2OSDDK_pf_BlockIo_Transfer)(void *apDevice, K2OS_BLOCKIO_TRANSFER const *apTransfer);

typedef struct _K2OSDDK_BLOCKIO_REGISTER K2OSDDK_BLOCKIO_REGISTER;
struct _K2OSDDK_BLOCKIO_REGISTER
{
    K2OS_BLOCKIO_CONFIG         Config;
    K2OSDDK_pf_BlockIo_GetMedia GetMedia;
    K2OSDDK_pf_BlockIo_Transfer Transfer;
};

typedef void (*K2OSDDK_pf_BlockIo_NotifyKey)(void *apKey, K2OS_DEVCTX aDevCtx, void *apDevice, UINT32 aNotifyCode);

K2STAT K2OSDDK_BlockIoRegister(K2OS_DEVCTX aDevCtx, void *apDevice, K2OSDDK_BLOCKIO_REGISTER const *apRegister, K2OSDDK_pf_BlockIo_NotifyKey **apRetNotifyKey);
K2STAT K2OSDDK_BlockIoDeregister(K2OS_DEVCTX aDevCtx, void *apDevice);

//
//------------------------------------------------------------------------
//

typedef BOOL (*K2OSDDK_pf_NetIo_SetEnable)(void *apDevice, BOOL aSetEnable);
typedef BOOL (*K2OSDDK_pf_NetIo_GetState)(void *apDevice, BOOL *apRetIsPhysConn, BOOL *apRetIsUp);
typedef BOOL (*K2OSDDK_pf_NetIo_DoneRecv)(void *apDevice, UINT32 aBufIx);
typedef BOOL (*K2OSDDK_pf_NetIo_Xmit)(void *apDevice, UINT32 aBufIx, UINT32 aDataLen);

typedef struct _K2OSDDK_NETIO_REGISTER K2OSDDK_NETIO_REGISTER;
struct _K2OSDDK_NETIO_REGISTER
{
    K2_NET_ADAPTER_DESC         Desc;
    K2OS_NETIO_BUFCOUNTS        BufCounts;
    UINT32                      mBufsPhysBaseAddr;
    UINT32 *                    mpBufsPhysAddrs;

    K2OSDDK_pf_NetIo_SetEnable  SetEnable;
    K2OSDDK_pf_NetIo_GetState   GetState;
    K2OSDDK_pf_NetIo_DoneRecv   DoneRecv;
    K2OSDDK_pf_NetIo_Xmit       Xmit;
};

typedef void (*K2OSDDK_pf_NetIo_RecvKey)(void *apKey, K2OS_DEVCTX aDevCtx, void *apDevice, UINT32 aBufAddr, UINT32 aDataLen);
typedef void (*K2OSDDK_pf_NetIo_XmitDoneKey)(void *apKey, K2OS_DEVCTX aDevCtx, void *apDevice, UINT32 aBufIx);
typedef void (*K2OSDDK_pf_NetIo_NotifyKey)(void *apKey, K2OS_DEVCTX aDevCtx, void *apDevice, UINT32 aNotifyCode);

K2STAT
K2OSDDK_NetIoRegister(
    K2OS_DEVCTX                     aDevCtx,
    void *                          apDevice,
    K2OSDDK_NETIO_REGISTER const *  apRegister,
    K2OS_PAGEARRAY_TOKEN            aFramesPageArrayToken,
    K2OSDDK_pf_NetIo_RecvKey **     apRetRecvKey,
    K2OSDDK_pf_NetIo_XmitDoneKey ** apRetXmitDoneKey,
    K2OSDDK_pf_NetIo_NotifyKey **   apRetNotifyKey
);

K2STAT 
K2OSDDK_NetIoDeregister(
    K2OS_DEVCTX aDevCtx, 
    void *      apDevice
);

//
//------------------------------------------------------------------------
//

#define K2OS_SYSMSG_DDK_SHORT_FSSTOP    1

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif


#endif // __K2OSDDK_H
