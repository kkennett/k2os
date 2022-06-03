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

#ifndef _VADISKADAPTERREGS_H_
#define _VADISKADAPTERREGS_H_

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

typedef struct _VIRTARM_DISKADAPTER_REGS
{
/* 0x00 */ _REG32 mVID;                     /* ro : Vendor ID */
/* 0x04 */ _REG32 mPID;                     /* ro : Product ID */
/* 0x08 */ _REG32 mVersion;                 /* ro : high 16 bits = major. low 16 bits = minor */
/* 0x0C */ _REG32 mReserved;                /* ro : reserved. SBZ */

/* 0x10 */ _REG32 mControlStatus;           /* rw */
/* 0x14 */ _REG32 mIntStatus;               /* ro */
/* 0x18 */ _REG32 mIntMask;                 /* ro */
/* 0x1C */ _REG32 mIntPend;                 /* rw */

/* 0x20 */ _REG32 mIntSetMask;              /* wo */
/* 0x24 */ _REG32 mIntClrMask;              /* wo */
/* 0x28 */ _REG32 mReserved2[2];            /* ro : reserved. SBZ */
/* 0x2C */ 

/* 0x30 */ _REG32 mDiskTotalSectors;        /* ro */
/* 0x34 */ _REG32 mSessionId;               /* ro */
/* 0x38 */ _REG32 mWindowCmd;               /* rw */
/* 0x3C */ _REG32 mWindowArg;               /* rw */

/* 0x40 */ _REG32 mWindow0BufferOffset;     /* ro */
/* 0x44 */ _REG32 mWindow0StartSector;      /* ro */
/* 0x48 */ _REG32 mWindow0SectorRun;        /* ro */
/* 0x4C */ _REG32 mWindow0Reserved;         /* ro : reserved. SBZ */

/* 0x50 */ _REG32 mWindow1BufferOffset;     /* ro */
/* 0x54 */ _REG32 mWindow1StartSector;      /* ro */
/* 0x58 */ _REG32 mWindow1SectorRun;        /* ro */
/* 0x5C */ _REG32 mWindow1Reserved;         /* ro : reserved. SBZ */

/* 0x60 */ _REG32 mWindow2BufferOffset;     /* ro */
/* 0x64 */ _REG32 mWindow2StartSector;      /* ro */
/* 0x68 */ _REG32 mWindow2SectorRun;        /* ro */
/* 0x6C */ _REG32 mWindow2Reserved;         /* ro : reserved. SBZ */

/* 0x70 */ _REG32 mWindow3BufferOffset;     /* ro */
/* 0x74 */ _REG32 mWindow3StartSector;      /* ro */
/* 0x78 */ _REG32 mWindow3SectorRun;        /* ro */
/* 0x7C */ _REG32 mWindow3Reserved;         /* ro : reserved. SBZ */

/* 0x80 */ _REG32 mMediaGuid_0;             /* ro : mounted media id bits  0-32  */
/* 0x84 */ _REG32 mMediaGuid_1;             /* ro : mounted media id bits 33-63  */
/* 0x88 */ _REG32 mMediaGuid_2;             /* ro : mounted media id bits 64-95  */
/* 0x8C */ _REG32 mMediaGuid_3;             /* ro : mounted media id bits 96-127 */

} VIRTARM_DISKADAPTER_REGS;

#define VIRTARM_DISKADAPTER_VID                     0x0000045E
#define VIRTARM_DISKADAPTER_PID                     0x00000606

#define VIRTARM_DISKADAPTER_CTRLSTAT_ENABLE_TOGGLE  0x80000000
#define VIRTARM_DISKADAPTER_CTRLSTAT_MEDIA_PRESENCE 0x00000001
#define VIRTARM_DISKADAPTER_CTRLSTAT_MEDIA_READONLY 0x00000002

#define VIRTARM_DISKADAPTER_INTR_PRESENCE_CHANGE    0x00000001
#define VIRTARM_DISKADAPTER_INTR_ALL                0x00000001

// ---------------------------------------------------------------------

/* SRAM locations (256k) */
#define VIRTARM_DISKADAPTER_SRAM_OFFSET_SECTORBUFFER    0
#define VIRTARM_DISKADAPTER_SECTOR_BYTES                512
#define VIRTARM_DISKADAPTER_MAX_NUM_SECTORS            ((VIRTARM_PHYSSIZE_ADAPTER_SRAM - VIRTARM_DISKADAPTER_SRAM_OFFSET_SECTORBUFFER) / VIRTARM_DISKADAPTER_SECTOR_BYTES)

// ---------------------------------------------------------------------

#define VIRTARM_DISKADAPTER_WINDOWCMD_IXMASK    0x0000000F
#define VIRTARM_DISKADAPTER_WINDOWCMD_CMDMASK   0x00000030

#define VIRTARM_DISKADAPTER_WINDOWCMD_FRAME     0x00000010
/* 
   mWindowCmd    IN: (VIRTARM_DISKADAPTER_WINDOWCMD_FRAME | <window index>)
   mWindowArg    IN: sector # to frame into window (somewhere)
   ------
   mWindowCmd   OUT: 0 for success (window regs updated), nonzero = error code
   mWindowArg   OUT: SRAM offset to precise sector requested
*/

#define VIRTARM_DISKADAPTER_WINDOWCMD_FLUSH_WRITE 0x00000020
/* 
   mWindowCmd    IN: (VIRTARM_DISKADAPTER_WINDOWCMD_FLUSH_WRITE | <window index>)
   mWindowArg    IN: ignored. SBZ
   ------
   mWindowCmd   OUT: 0 for success, nonzero = error code
   mWindowArg   OUT: undefined
*/

/* window command errors - read the WindowCmd register after a write to get these */
#define VIRTARM_DISKADAPTER_IOERR_NONE          0
#define VIRTARM_DISKADAPTER_IOERR_OUTOFRANGE    0x807B0001
#define VIRTARM_DISKADAPTER_IOERR_BADCOMMAND    0x807B0002
#define VIRTARM_DISKADAPTER_IOERR_READONLY      0x807B0003

// ---------------------------------------------------------------------

#ifdef __cplusplus
};
#endif

#endif // _VADISKADAPTERREGS_H_
