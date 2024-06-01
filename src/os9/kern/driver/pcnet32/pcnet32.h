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
#ifndef __PCNET32_H
#define __PCNET32_H

#include <k2osddk.h>
#include <k2osdev_netio.h>
#include <spec/ether.h>
#include <spec/k2pci.h>

#if K2_TARGET_ARCH_IS_INTEL
#include <lib/k2archx32.h>
#else
#include <lib/k2archa32.h>
#endif

/* ------------------------------------------------------------------------- */

// DWORD write to RDP switches to 32-bit mode (see table 20)
// DWIO = 0
#define PCNET32_APROM_OFFSET            0
#define PCNET32_APROM_LENGTH            0x10

#define PCNET32_REG16_RDP_OFFSET        0x10
#define PCNET32_REG16_RDP_LENGTH        2
#define PCNET32_REG16_RAP_OFFSET        0x12
#define PCNET32_REG16_RAP_LENGTH        2
#define PCNET32_REG16_RESET_OFFSET      0x14
#define PCNET32_REG16_RESET_LENGTH      2
#define PCNET32_REG16_BDP_OFFSET        0x16
#define PCNET32_REG16_BDP_LENGTH        2
#define PCNET32_REG16_RESERVED_OFFSET   0x18
#define PCNET32_REG16_RESERVED_LENGTH   8

#define PCNET32_REG32_RDP_OFFSET        0x10
#define PCNET32_REG32_RDP_LENGTH        4
#define PCNET32_REG32_RAP_OFFSET        0x14
#define PCNET32_REG32_RAP_LENGTH        4
#define PCNET32_REG32_RESET_OFFSET      0x18
#define PCNET32_REG32_RESET_LENGTH      4
#define PCNET32_REG32_BDP_OFFSET        0x1C
#define PCNET32_REG32_BDP_LENGTH        4

#define PCNET32_PHYAD_INTERNAL          0x03C0      // 0x1E << 5 see BCR33

#define PCNET32_INIT_DESC_SIZE          0x1C
#define PCNET32_RXTX_DESC_SIZE          0x10

#define PCNET32_DESC_ADAPTER_OWNS       0x8000
#define PCNET32_DESC_ERR                0x4000
#define PCNET32_DESC_ADD_FCS            0x2000
#define PCNET32_DESC_MORE               0x1000
#define PCNET32_DESC_ONE                0x0800
#define PCNET32_DESC_DEF                0x0400
#define PCNET32_DESC_STP                0x0200
#define PCNET32_DESC_ENP                0x0100
#define PCNET32_DESC_BPE                0x0080

#define RX_COUNT_POW2                   7 //3 //7                       // 128
#define TX_COUNT_POW2                   6 //2 //6                       // 64
#define RX_COUNT                        (1 << RX_COUNT_POW2)
#define TX_COUNT                        (1 << TX_COUNT_POW2)

#define RX_IX_MASK                      (RX_COUNT - 1)
#define TX_IX_MASK                      (TX_COUNT - 1)

#define MAP_OFFSET_INIT                 0
#define MAP_OFFSET_RX_RING              ((((MAP_OFFSET_INIT + PCNET32_INIT_DESC_SIZE) + (PCNET32_RXTX_DESC_SIZE-1)) / PCNET32_RXTX_DESC_SIZE) * PCNET32_RXTX_DESC_SIZE)
#define MAP_OFFSET_TX_RING              (MAP_OFFSET_RX_RING + (RX_COUNT * PCNET32_RXTX_DESC_SIZE))
#define MAP_OFFSET_RINGS_END            (MAP_OFFSET_TX_RING + (TX_COUNT * PCNET32_RXTX_DESC_SIZE))
K2_STATIC_ASSERT(MAP_OFFSET_RINGS_END < K2_VA_MEMPAGE_BYTES);

#define CSR0_ERR_RO         0x8000
#define CSR0_CERR_W1C       0x2000
#define CSR0_MISS_W1C       0x1000
#define CSR0_MERR_W1C       0x0800
#define CSR0_RINT_W1C       0x0400
#define CSR0_TINT_W1C       0x0200
#define CSR0_IDON_W1C       0x0100
#define CSR0_INTR_RO        0x0080
#define CSR0_IENA           0x0040
#define CSR0_RXON_RO        0x0020
#define CSR0_TXON_RO        0x0010
#define CSR0_TDMD_CMD       0x0008
#define CSR0_STOP_CMD       0x0004
#define CSR0_STRT_CMD       0x0002
#define CSR0_INIT_CMD       0x0001

#define CSR3_MISSM          0x1000
#define CSR3_MERRM          0x0800
#define CSR3_RINTM          0x0400
#define CSR3_TINTM          0x0200
#define CSR3_IDONM          0x0100
#define CSR3_DXSUFLO        0x0040
#define CSR3_LAPPEN         0x0020
#define CSR3_DXMT2PD        0x0010
#define CSR3_EMBA           0x0008
#define CSR3_BSWP           0x0004

#define CSR15_PROM          0x8000
#define CSR15_DRCVBC        0x4000
#define CSR15_DRCVPA        0x2000
#define CSR15_PORTSEL_MASK  0x0180
#define CSR15_INTL          0x0040
#define CSR15_DRTY          0x0020
#define CSR15_FCOLL         0x0010
#define CSR15_DXMTFCS       0x0008
#define CSR15_LOOP          0x0004
#define CSR15_DTX           0x0002
#define CSR15_DRX           0x0001

K2_PACKED_PUSH
typedef struct _PCNET32_INIT PCNET32_INIT;
struct _PCNET32_INIT // table 55
{
    UINT32  MODE : 16;
    UINT32  reserved10 : 4;
    UINT32  RLEN : 4;
    UINT32  reserved18 : 4;
    UINT32  TLEN : 4;

    UINT64  PADR;

    UINT64  LADRF;

    UINT32  RDRA;
    UINT32  TDRA;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
K2_STATIC_ASSERT(sizeof(PCNET32_INIT) == PCNET32_INIT_DESC_SIZE);

// SWSTYLE 3

K2_PACKED_PUSH
typedef struct _PCNET32_RX_HDR PCNET32_RX_HDR;
struct _PCNET32_RX_HDR
{
    UINT16  mMsgCount;
    UINT16  reserved2;
    UINT16  mBufLen;    // twos complement
    UINT16  mStatus;
    UINT32  mBufPhys;
    UINT32  mUserSpace;
} K2_PACKED_ATTRIB;
K2_PACKED_POP;
K2_STATIC_ASSERT(sizeof(PCNET32_RX_HDR) == PCNET32_RXTX_DESC_SIZE);

K2_PACKED_PUSH
typedef struct _PCNET32_TX_HDR PCNET32_TX_HDR;
struct _PCNET32_TX_HDR
{
    UINT16  mRetryCount;
    UINT16  mErrors;
    UINT16  mBufLen;    // twos complement
    UINT16  mStatus;
    UINT32  mBufPhys;
    UINT32  mUserSpace;
} K2_PACKED_ATTRIB;
K2_PACKED_POP;
K2_STATIC_ASSERT(sizeof(PCNET32_TX_HDR) == PCNET32_RXTX_DESC_SIZE);

typedef struct _PCNET32_DEVICE PCNET32_DEVICE;
struct _PCNET32_DEVICE
{
    //
    // set up by CreateInstance
    K2OS_DEVCTX             mDevCtx;
    K2OSKERN_SEQLOCK        SeqLock;
    K2OS_CRITSEC            TxSec;              // serializes user calls to Xmit
    UINT32                  mRingsPhys;
    K2OS_PAGEARRAY_TOKEN    mTokRingsPageArray;
    UINT32                  mRingsVirt;
    K2OS_VIRTMAP_TOKEN      mTokRingsVirtMap;
    UINT32                  mFramesPagesCount;  // may be < # of pages in page array, due to need to power of 2 alloc phys
    K2OS_PAGEARRAY_TOKEN    mTokFramesPageArray;
    UINT32                  mFramesPhys;
    UINT32 *                mpFramePhysAddrs;

    //
    // set up by StartDriver
    K2OSDDK_INSTINFO                InstInfo;
    K2OS_RPC_OBJ_HANDLE             mhBusRpc;
    BOOL                            mMappedIo;
    UINT32                          mRegsPageCount;
    UINT32                          mRegsVirtAddr;
    K2OS_VIRTMAP_TOKEN              mTokRegsVirtMap;
    K2OSDDK_RES                     Res;    // phys or io
    K2OSDDK_RES                     ResIrq;
    K2OS_INTERRUPT_TOKEN            mTokIntr;
    K2OSKERN_pf_Hook_Key            mIrqHookKey;
    K2OS_THREAD_TOKEN               mTokIntrThread;
    UINT32                          mIntrThreadId;
    UINT8                           mCacheAPROM[PCNET32_APROM_LENGTH];
    BOOL volatile                   mIsRunning;
    K2OSDDK_pf_NetIo_RecvKey *      mpRecvKey;
    K2OSDDK_pf_NetIo_XmitDoneKey *  mpXmitDoneKey;
    K2OSDDK_pf_NetIo_NotifyKey *    mpNotifyKey;

    //
    // set up by enable
    //
    UINT32 volatile mTx_NextRingBufToUse;
    UINT32 volatile mTx_NextRingBufDone;
    UINT32 volatile mTx_Avail;

    UINT32 volatile mRx_RingNext;
    UINT32 volatile mRx_RingBufsFull;
    BOOL            mUserEnable;
};

void    PCNET32_ReadAPROM(PCNET32_DEVICE *apDevice, UINT32 aOffset, UINT32 aLength, UINT8 *apBuffer);

UINT16  PCNET32_ReadRDP(PCNET32_DEVICE *apDevice);
void    PCNET32_WriteRDP(PCNET32_DEVICE *apDevice, UINT16 aValue);

UINT16  PCNET32_ReadRAP(PCNET32_DEVICE *apDevice);
void    PCNET32_WriteRAP(PCNET32_DEVICE *apDevice, UINT16 aValue);

UINT16  PCNET32_ReadRESET(PCNET32_DEVICE *apDevice);
void    PCNET32_WriteRESET(PCNET32_DEVICE *apDevice, UINT16 aValue);

UINT16  PCNET32_ReadBDP(PCNET32_DEVICE *apDevice);
void    PCNET32_WriteBDP(PCNET32_DEVICE *apDevice, UINT16 aValue);

void    PCNET32_WriteCSR(PCNET32_DEVICE *apDevice, UINT32 aIndex, UINT16 aValue);
UINT16  PCNET32_ReadCSR(PCNET32_DEVICE *apDevice, UINT32 aIndex);

void    PCNET32_WriteBCR(PCNET32_DEVICE *apDevice, UINT32 aIndex, UINT16 aValue);
UINT16  PCNET32_ReadBCR(PCNET32_DEVICE *apDevice, UINT32 aIndex);

void    PCNET32_WriteANR(PCNET32_DEVICE *apDevice, UINT32 aIndex, UINT16 aValue);
UINT16  PCNET32_ReadANR(PCNET32_DEVICE *apDevice, UINT32 aIndex);

KernIntrDispType PCNET32_Isr(void *apKey, KernIntrActionType aAction);

void    PCNET32_WritePciCfg(PCNET32_DEVICE *apDevice, UINT32 aOffset, UINT32 aWidth, UINT32 aValue);
UINT32  PCNET32_ReadPciCfg(PCNET32_DEVICE *apDevice, UINT32 aOffset, UINT32 aWidth);

UINT32  PCNET32_ServiceThread(PCNET32_DEVICE *apDevice);

BOOL    PCNET32_SetEnable(PCNET32_DEVICE *apDevice, BOOL aSetEnable);
BOOL    PCNET32_GetState(PCNET32_DEVICE *apDevice, BOOL *apRetPhysConn, BOOL *apRetIsUp);
BOOL    PCNET32_DoneRecv(PCNET32_DEVICE *apDevice, UINT32 aBufIx);
BOOL    PCNET32_Xmit(PCNET32_DEVICE *apDevice, UINT32 aBufIx, UINT32 aDataLen);


/* ------------------------------------------------------------------------- */

#endif // __PCNET32_H