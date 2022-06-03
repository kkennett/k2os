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

#ifndef _VABLITADAPTERREGS_H_
#define _VABLITADAPTERREGS_H_

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

typedef struct _VIRTARM_BLITADAPTER_CMDBUF
{
    UINT32  mLeft;
    UINT32  mTop;
    UINT32  mRight;
    UINT32  mBottom;
    UINT32  mArg0;
    UINT32  mArg1;
} VIRTARM_BLITADAPTER_CMDBUF;

typedef struct _VIRTARM_BLITADAPTER_REGS
{
/* 0x00 */ _REG32 mVID;           /* ro : Vendor ID */
/* 0x04 */ _REG32 mPID;           /* ro : Product ID */
/* 0x08 */ _REG32 mVersion;       /* ro : high 16 bits = major. low 16 bits = minor */
/* 0x0C */ _REG32 mReserved;      /* ro : reserved. SBZ */

/* 0x10 */ _REG32 mWidth;         /* ro: display physical width */
/* 0x14 */ _REG32 mHeight;        /* ro: display physical height */
/* 0x18 */ _REG32 mConfig;        /* rw: configuration and enable reg */
/* 0x1C */ _REG32 mCommand;       /* rw: command/result reg */

/* 0x20 */ _REG32 mPhysPixAddr;   /* rw, ro when enabled: pixel buffer system physical base address (16 byte aligned) */

} VIRTARM_BLITADAPTER_REGS;

#define VIRTARM_BLITADAPTER_VID     0x0000045E
#define VIRTARM_BLITADAPTER_PID     0x00000303

/* SRAM locations (256k) */
#define VIRTARM_BLITADAPTER_SRAM_OFFSET_CMDBUF        0

#define VIRTARM_BLITADAPTER_REGS_CONFIG_ENABLE        0x00000001

#define VIRTARM_BLITADAPTER_REGS_COMMAND_UPDATERECT   0x00000001
/* Arg0: sbz
   Arg1: sbz
*/

#define VIRTARM_BLITADAPTER_REGS_COMMAND_SOLIDRECT    0x00000002
/* Arg0: 16-bit RGB 565 solid color to paint
   Arg1: sbz
*/

// ---------------------------------------------------------------------
#ifdef __cplusplus
};
#endif

#endif // _VABLITADAPTERREGS_H_
