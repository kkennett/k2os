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
#include "VADebugAdapterRegs.inc"

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



/* IO commands for packets */
//#define VIRTARM_DEBUGADAPTER_IOCMD_GETXMITBUF       1   
/* IOArgument  IN:  Size to transmit
   IOArgument OUT:  buffer OFFSET from start of SRAM 
   IOCommand  OUT:  error code or 0 for no error
*/

//#define VIRTARM_DEBUGADAPTER_IOCMD_DOXMIT           2   
/* IOArgument  IN:  buffer OFFSET from start of SRAM to xmit
   IOArgument OUT:  nothing 
   IOCommand  OUT:  error code or 0 for no error
*/

//#define VIRTARM_DEBUGADAPTER_IOCMD_GETRECVBUF       3   
/* IOArgument  IN:  nothing
   IOArgument OUT:  buffer OFFSET from start of SRAM or 0xFFFFFFFF for no buffer
   IOCommand  OUT:  data bytes in buffer if > 0, error code if <= 0
*/

//#define VIRTARM_DEBUGADAPTER_IOCMD_RELEASE          4   
/* IOArgument  IN:  buffer OFFSET from start of SRAM to release (buffer not xmitted or received buffer)
   IOArgument OUT:  nothing 
   IOCommand  OUT:  error code or 0 for no error
*/

//#define VIRTARM_DEBUGADAPTER_IOCMD_CLEARBUFFERS     5   
/* IOArgument  IN:  nothing
   IOArgument OUT:  nothing 
   IOCommand  OUT:  error code or 0 for no error
*/

// ---------------------------------------------------------------------

#ifdef _MSC_VER

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

#endif

// ---------------------------------------------------------------------

#ifdef __cplusplus
};
#endif

#endif // _VADEBUGADAPTERREGS_H_
