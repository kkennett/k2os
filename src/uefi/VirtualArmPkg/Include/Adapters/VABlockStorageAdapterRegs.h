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

#ifndef _VABLOCKSTORAGEADAPTERREGS_H_
#define _VABLOCKSTORAGEADAPTERREGS_H_

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

typedef struct _VIRTARM_BLOCKSTORAGEADAPTER_REGS
{
/* 0x00 */ _REG32 mVID;                     /* ro : Vendor ID */
/* 0x04 */ _REG32 mPID;                     /* ro : Product ID */
/* 0x08 */ _REG32 mVersion;                 /* ro : high 16 bits = major. low 16 bits = minor */
/* 0x0C */ _REG32 mIoResult;                /* ro */

/* 0x10 */ _REG32 mControlStatus;           /* rw */
/* 0x14 */ _REG32 mIntStatus;               /* ro */
/* 0x18 */ _REG32 mIntMask;                 /* ro */
/* 0x1C */ _REG32 mIntPend;                 /* rw */

/* 0x20 */ _REG32 mIntSetMask;              /* wo */
/* 0x24 */ _REG32 mIntClrMask;              /* wo */
/* 0x28 */ _REG32 mDiskTotalSectorsLo32;    /* ro */
/* 0x2C */ _REG32 mDiskTotalSectorsHi32;    /* ro */

/* 0x30 */ _REG32 mSessionIdHi16;           /* ro - session id in high 16 bits.  low 16 bits SBZ */
/* 0x34 */ _REG32 mMediaTypeId;             /* ro */
/* 0x38 */ _REG32 mSectorBytes;             /* ro */
/* 0x3C */ _REG32 mSessionCommandResult;    /* rw - send (session id | command), get back result*/

/* 0x40 */ _REG32 mArgMemory;               /* rw */
/* 0x44 */ _REG32 mArgSectorLo32;           /* rw */
/* 0x48 */ _REG32 mArgSectorHi32;           /* rw */
/* 0x4C */ _REG32 mArgSectorCount;          /* rw */

/* 0x50 */ _REG32 mMediaGuid_0;             /* ro : mounted media id bits  0-32  */
/* 0x54 */ _REG32 mMediaGuid_1;             /* ro : mounted media id bits 33-63  */
/* 0x58 */ _REG32 mMediaGuid_2;             /* ro : mounted media id bits 64-95  */
/* 0x5C */ _REG32 mMediaGuid_3;             /* ro : mounted media id bits 96-127 */

} VIRTARM_BLOCKSTORAGEADAPTER_REGS;

#define VIRTARM_BLOCKSTORAGEADAPTER_VID                     0x0000045E
#define VIRTARM_BLOCKSTORAGEADAPTER_PID                     0x00000909

#define VIRTARM_BLOCKSTORAGEADAPTER_CTRLSTAT_ENABLE_TOGGLE  0x80000000
#define VIRTARM_BLOCKSTORAGEADAPTER_CTRLSTAT_MEDIA_PRESENCE 0x00000001
#define VIRTARM_BLOCKSTORAGEADAPTER_CTRLSTAT_MEDIA_READONLY 0x00000002

#define VIRTARM_BLOCKSTORAGEADAPTER_INTR_PRESENCE_CHANGE    0x00000001
#define VIRTARM_BLOCKSTORAGEADAPTER_INTR_IO_COMPLETED       0x00000002
#define VIRTARM_BLOCKSTORAGEADAPTER_INTR_ALL                0x00000003

// ---------------------------------------------------------------------

#define VIRTARM_BLOCKSTORAGEADAPTER_COMMAND_READ            0x00000001
/*
    mArgMemory               IN: start offset into adapter memory to put sectors read
    mArgSectorLo32           IN: starting sector to read low 32 bits
    mArgSectorHi32           IN: starting sector to read high 32 bits
    mArgSectorCount          IN: number of sectors to read
    mSessionCommandResult    IN: <session id> | 1
    ----
    mArgMemory              OUT: undefined
    mArgSectorLo32          OUT: undefined
    mArgSectorHi32          OUT: undefined
    mArgSectorCount         OUT: undefined
    mSessionCommandResult   OUT: 0 for success, HRESULT for error
*/

#define VIRTARM_BLOCKSTORAGEADAPTER_COMMAND_WRITE           0x00000002
/*
    mArgMemory               IN: start offset into adapter memory holding sectors to write
    mArgSectorLo32           IN: starting sector to write low 32 bits
    mArgSectorHi32           IN: starting sector to write high 32 bits
    mArgSectorCount          IN: number of sectors to write
    mSessionCommandResult    IN: <session id> | 2
    ----
    mArgMemory              OUT: undefined
    mArgSectorLo32          OUT: undefined
    mArgSectorHi32          OUT: undefined
    mArgSectorCount         OUT: undefined
    mSessionCommandResult   OUT: 0 for success, HRESULT for error
*/

#define VIRTARM_BLOCKSTORAGEADAPTER_COMMAND_FLUSHWRITES     0x00000003
/*
    mArgMemory               IN: ignored. SBZ.
    mArgSectorLo32           IN: ignored. SBZ.
    mArgSectorHi32           IN: ignored. SBZ.
    mArgSectorCount          IN: ignored. SBZ.
    mSessionCommandResult    IN: <session id> | 3
    ----
    mArgMemory              OUT: undefined
    mArgSectorLo32          OUT: undefined
    mArgSectorHi32          OUT: undefined
    mArgSectorCount         OUT: undefined
    mSessionCommandResult   OUT: 0 for success, HRESULT for error
*/

/* command errors */
#define VIRTARM_BLOCKSTORAGEADAPTER_ERR_NONE                0
#define VIRTARM_BLOCKSTORAGEADAPTER_ERR_MEDIA_CHANGED       0x807B0001
#define VIRTARM_BLOCKSTORAGEADAPTER_ERR_SECTOR_OUTOFRANGE   0x807B0002
#define VIRTARM_BLOCKSTORAGEADAPTER_ERR_MEMORY_OUTOFRANGE   0x807B0003
#define VIRTARM_BLOCKSTORAGEADAPTER_ERR_BADCOMMAND          0x807B0004
#define VIRTARM_BLOCKSTORAGEADAPTER_ERR_READONLY            0x807B0005
#define VIRTARM_BLOCKSTORAGEADAPTER_ERR_MEDIA_FAILURE       0x807B0006
#define VIRTARM_BLOCKSTORAGEADAPTER_ERR_BUSY                0x807B0007

// ---------------------------------------------------------------------

#ifdef __cplusplus
};
#endif

#endif // _VABLOCKSTORAGEADAPTERREGS_H_
