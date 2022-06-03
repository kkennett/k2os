//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED 'AS IS', WITH NO WARRANTIES OR INDEMNITIES.
//
// Code Originator:  Kurt Kennett
// Last Updated By:  Kurt Kennett
//

#ifndef _VADEBUGADAPTERREGS_H_
#define _VADEBUGADAPTERREGS_H_

/*--------------------------------------------*
         RAW MULTI-PLATFORM INCLUDE FILE
         DO NOT ASSUME ANY PARTICULAR OS
           OR ANY PARTICULAR COMPILER
  --------------------------------------------*/

#include "virtarm.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------

typedef struct _VIRTARM_DEBUGADAPTER_REGS
{
/* 0x00 */ _REG32 mVID;           /* ro : Vendor ID */
/* 0x04 */ _REG32 mPID;           /* ro : Product ID */
/* 0x08 */ _REG32 mVersion;       /* ro : high 16 bits = major. low 16 bits = minor */
/* 0x0C */ _REG32 mReserved;      /* ro : reserved. SBZ */

/* 0x10 */ _REG32 mDebugOut;                                /* write only */
/* 0x14 */ _REG32 mDebugIn;                                 /* read only */
/* 0x18 */ _REG32 mDebugInCount;                            /* read only */
/* 0x1C */ _REG32 mHostTick;                                /* read only */

/* 0x20 */ _REG32 mNumCpusToUse;                            /* read only */
/* 0x24 */ _REG32 mSizeSDRAM;                               /* read only */
/* 0x28 */ _REG32 mControlAndStatus;                        /* read/write VIRTARM_DEBUGADAPTER_CTRLSTAT_xxx below*/
/* 0x2C */ _REG32 mInterrupt;                               /* read/write VIRTARM_DEBUGADAPTER_INTR_xxx below */

/* 0x30
   0x34 */ _REG32 mScratchPad[VIRTARM_MAX_NUMBER_OF_CPUS];  /* read/write */
/* 0x38-
   0xAC */ _REG32 mReserved2[30];                           /* reserved */

/* 0xB0 */ _REG32 mPROMAddr;                                /* read/write */
/* 0xB4 */ _REG32 mPROMData;                                /* read/write */
/* 0xB8 */ _REG32 mIOCommand;                               /* read/write */
/* 0xBC */ _REG32 mIOArgument;                              /* read/write */

/* 0xC0 */ _REG32 mRandomGen;                               /* read only */

} VIRTARM_DEBUGADAPTER_REGS;

#define VIRTARM_DEBUGADAPTER_VID                     0x0000045E
#define VIRTARM_DEBUGADAPTER_PID                     0x00000101

/* SRAM locations (256k) */
#define VIRTARM_DEBUGADAPTER_SRAM_OFFSET_MESSAGEBUFFER  0
#define VIRTARM_DEBUGADAPTER_SRAM_OFFSET_PACKETBUFFER   4096
#define VIRTARM_DEBUGADAPTER_PACKET_MAX_BYTES           2048
#define VIRTARM_DEBUGADAPTER_MAX_NUM_PACKETS            ((VIRTARM_PHYSSIZE_ADAPTER_SRAM - VIRTARM_DEBUGADAPTER_SRAM_OFFSET_PACKETBUFFER) / VIRTARM_DEBUGADAPTER_PACKET_MAX_BYTES)

/* set the adddress register mPROMAddr, then read or write the mPROMData to set or get the data at that address */
/* data is always persisted instantly following the register write on a store */
#define VIRTARM_DEBUGADAPTER_SIZE_PROM_BYTES            (4*1024)

/* read  - 1s indicate raw interrupt status */
/* write - ignored */
#define VIRTARM_DEBUGADAPTER_INTR_STAT_EXTPOWEROFF_REQ      0x00000001
#define VIRTARM_DEBUGADAPTER_INTR_STAT_EXTSUSPEND_REQ       0x00000002
#define VIRTARM_DEBUGADAPTER_INTR_STAT_EXTRESET_REQ         0x00000004
#define VIRTARM_DEBUGADAPTER_INTR_STAT_PACKET_RECV          0x00000008
#define VIRTARM_DEBUGADAPTER_INTR_STAT_PACKET_XMIT          0x00000010
#define VIRTARM_DEBUGADAPTER_INTR_STAT_ALL                  0x0000001F

/* read  - 1s indicate pending interrupt status (sticky) */
/* write - 1s indicate pending interrupts to clear */
#define VIRTARM_DEBUGADAPTER_INTR_PEND_EXTPOWEROFF_REQ      0x00000100
#define VIRTARM_DEBUGADAPTER_INTR_PEND_EXTSUSPEND_REQ       0x00000200
#define VIRTARM_DEBUGADAPTER_INTR_PEND_EXTRESET_REQ         0x00000400
#define VIRTARM_DEBUGADAPTER_INTR_PEND_PACKET_RECV          0x00000800
#define VIRTARM_DEBUGADAPTER_INTR_PEND_PACKET_XMIT          0x00001000
#define VIRTARM_DEBUGADAPTER_INTR_PEND_ALL                  0x00001F00

/* read  - 1s indicate what interrupts are masked */
/* write - 1s say what interrupts should be masked */
#define VIRTARM_DEBUGADAPTER_INTR_MASKSET_EXTPOWEROFF       0x00010000
#define VIRTARM_DEBUGADAPTER_INTR_MASKSET_EXTSUSPEND        0x00020000
#define VIRTARM_DEBUGADAPTER_INTR_MASKSET_EXTRESET          0x00040000
#define VIRTARM_DEBUGADAPTER_INTR_MASKSET_PACKET_RECV       0x00080000
#define VIRTARM_DEBUGADAPTER_INTR_MASKSET_PACKET_XMIT       0x00100000
#define VIRTARM_DEBUGADAPTER_INTR_MASKSET_ALL               0x001F0000

/* read  - 1s indicate what interrupts are masked */
/* write - 1s say what interrupts should be unmasked */
#define VIRTARM_DEBUGADAPTER_INTR_MASKCLR_EXTPOWEROFF       0x01000000
#define VIRTARM_DEBUGADAPTER_INTR_MASKCLR_EXTSUSPEND        0x02000000
#define VIRTARM_DEBUGADAPTER_INTR_MASKCLR_EXTRESET          0x04000000
#define VIRTARM_DEBUGADAPTER_INTR_MASKCLR_PACKET_RECV       0x08000000
#define VIRTARM_DEBUGADAPTER_INTR_MASKCLR_PACKET_XMIT       0x10000000
#define VIRTARM_DEBUGADAPTER_INTR_MASKCLR_ALL               0x1F000000

/* read  - whether adapter is in kitl mode (powers on not in kitl mode) */
/* write - toggle kitl mode to the opposite state */
#define VIRTARM_DEBUGADAPTER_CTRLSTAT_KITLMODE              0x00000001

/* read  - whether message buffer in first 4k of SRAM is being used */
/* write - toggle message buffer to the opposite state */
#define VIRTARM_DEBUGADAPTER_CTRLSTAT_MESSAGEBUFFER         0x00000002

/* read  - nothing */
/* write - 1s say what commands to exec */
#define VIRTARM_DEBUGADAPTER_CTRLSTAT_SUSPEND               0x00000004
#define VIRTARM_DEBUGADAPTER_CTRLSTAT_POWEROFF              0x00000008
#define VIRTARM_DEBUGADAPTER_CTRLSTAT_HARDRESET             0x00000010

/* read - whether a debugging console is present (debug messages can be seen) */
/* write - nothing */
#define VIRTARM_DEBUGADAPTER_CTRLSTAT_CONSOLE_PRESENT       0x00000020

/* IO commands for packets */
#define VIRTARM_DEBUGADAPTER_IOCMD_GETXMITBUF       1   
/* IOArgument  IN:  Size to transmit
   IOArgument OUT:  buffer OFFSET from start of SRAM 
   IOCommand  OUT:  error code or 0 for no error
*/

#define VIRTARM_DEBUGADAPTER_IOCMD_DOXMIT           2   
/* IOArgument  IN:  buffer OFFSET from start of SRAM to xmit
   IOArgument OUT:  nothing 
   IOCommand  OUT:  error code or 0 for no error
*/

#define VIRTARM_DEBUGADAPTER_IOCMD_GETRECVBUF       3   
/* IOArgument  IN:  nothing
   IOArgument OUT:  buffer OFFSET from start of SRAM or 0xFFFFFFFF for no buffer
   IOCommand  OUT:  data bytes in buffer if > 0, error code if <= 0
*/

#define VIRTARM_DEBUGADAPTER_IOCMD_RELEASE          4   
/* IOArgument  IN:  buffer OFFSET from start of SRAM to release (buffer not xmitted or received buffer)
   IOArgument OUT:  nothing 
   IOCommand  OUT:  error code or 0 for no error
*/

#define VIRTARM_DEBUGADAPTER_IOCMD_CLEARBUFFERS     5   
/* IOArgument  IN:  nothing
   IOArgument OUT:  nothing 
   IOCommand  OUT:  error code or 0 for no error
*/

/* IO errors - read the command register after a write to get these */
#define VIRTARM_DEBUGADAPTER_IOERR_NONE             0
#define VIRTARM_DEBUGADAPTER_IOERR_NOFREEFRAMES     0x807A0001
#define VIRTARM_DEBUGADAPTER_IOERR_FRAMETOOBIG      0x807A0002
#define VIRTARM_DEBUGADAPTER_IOERR_FRAMETOOSMALL    0x807A0003
#define VIRTARM_DEBUGADAPTER_IOERR_BADFRAMEPTR      0x807A0004
#define VIRTARM_DEBUGADAPTER_IOERR_BADFRAMESTATUS   0x807A0005
#define VIRTARM_DEBUGADAPTER_IOERR_XMITFAILED       0x807A0006
#define VIRTARM_DEBUGADAPTER_IOERR_BADRELEASE       0x807A0007

// ---------------------------------------------------------------------

#define VATRANSPORT_MTU             VIRTARM_DEBUGADAPTER_PACKET_MAX_BYTES

#define VATRANSPORT_PKT_KITL        0xAA
#define VATRANSPORT_PKT_DLREQ       0xBB
#define VATRANSPORT_PKT_DLPKT       0xCC
#define VATRANSPORT_PKT_DLACK       0xDD
#define VATRANSPORT_PKT_JUMP        0xEE

#ifndef _UINT8
#define _UINT8  unsigned char
#endif

#ifndef _UINT16
#define _UINT16 unsigned short
#endif

#ifndef _UINT32
#define _UINT32 unsigned long
#endif

#include <pshpack1.h>
typedef struct _VATRANSPORT_HDR
{
    _UINT8   mPacketType;
    _UINT8   mMarker;        // 'K'
    _UINT16  mDataBytes;
    _UINT32  mReserved;
} VATRANSPORT_HDR;

typedef struct _VATRANSPORT_BOOTME
{
    VATRANSPORT_HDR mHdr;
    _UINT8          mDeviceId[16];
} VATRANSPORT_BOOTME;

typedef struct _VATRANSPORT_JUMP_ACK
{
    VATRANSPORT_HDR mHdr;
    _UINT32         mFlags;
} VATRANSPORT_JUMP_ACK;

typedef struct _VATRANSPORT_BLOCK_HDR
{
    VATRANSPORT_HDR mHdr;
    _UINT32         mBlockNum;
} VATRANSPORT_BLOCK_HDR;
#include <poppack.h>

#define VATRANSPORT_MAX_SEND_BYTES  (VATRANSPORT_MTU - (sizeof(VATRANSPORT_BLOCK_HDR)))

// ---------------------------------------------------------------------

#ifdef __cplusplus
};
#endif

#endif // _VADEBUGADAPTERREGS_H_
