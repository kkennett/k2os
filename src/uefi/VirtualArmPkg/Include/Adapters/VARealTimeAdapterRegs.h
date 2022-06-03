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

#ifndef _VAREALTIMEADAPTERREGS_H_
#define _VAREALTIMEADAPTERREGS_H_

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

typedef struct _VIRTARM_REALTIMEADAPTER_REGS
{
/* 0x00 */ _REG32 mVID;           /* ro : Vendor ID */
/* 0x04 */ _REG32 mPID;           /* ro : Product ID */
/* 0x08 */ _REG32 mVersion;       /* ro : high 16 bits = major. low 16 bits = minor */
/* 0x0C */ _REG32 mReserved;      /* ro : reserved. SBZ */

/* 0x10 */ _REG32 mIntStatus;           /* ro: raw interrupt status */
/* 0x14 */ _REG32 mIntMask;             /* rw: mask interrupts */
/* 0x18 */ _REG32 mIntUnmask;           /* wo: unmask interrupts */
/* 0x1C */ _REG32 mIntPend;             /* rw: interrupt pending after mask */

/* 0x20 */ _REG32 mRealTime_Command;    /* rw: realtime commands (VIRTARM_REALTIMEADAPTER_COMMAND_XXX) */
/* 0x24 */ _REG32 mRealTime_YearMonth;  /* rw: 32 bits: yyyyyyyyyyyyyyyymmmmmmmmmmmmmmmm */
/* 0x28 */ _REG32 mRealTime_DayHour;    /* rw: 32 bits: ddddddddddddddddhhhhhhhhhhhhhhhh */
/* 0x2C */ _REG32 mRealTime_MinSec;     /* rw: 32 bits: mmmmmmmmmmmmmmmmssssssssssssssss */

/* 0x30 */ _REG32 mSystemTicks64_Low;   /* ro: 128-bit system up-time counter (ms) */
/* 0x34 */ _REG32 mSystemTicks64_High;
/* 0x38 */ _REG32 mCPUDispositionSet;   /* r/w: bitflags per cpu : read to get, write 1 to set flag */
/* 0x3C */ _REG32 mCPUDispositionClr;   /* r/w: bitflags per cpu : read to get, write 1 to clear flag */

/* 0x40 */ _REG32 mCounter;             /* ro: free running millisecond counter register */
/* 0x44 */ _REG32 mMatch0;              /* ro: interrupt match register 0 */
/* 0x48 */ _REG32 mMatch1;              /* ro: interrupt match register 1 */
/* 0x4C */ _REG32 mMatch2;              /* ro: interrupt match register 2 */

/* 0x50 */ _REG32 mHostLatency;         /* ro: host tick at interrupt assert */
/* 0x54 */ _REG32 mSetDeltaMatch0;      /* wo: interrupt on match delta set register 0 */
/* 0x58 */ _REG32 mSetDeltaMatch1;      /* wo: interrupt on match delta set register 1 */
/* 0x5C */ _REG32 mSetDeltaMatch2;      /* wo: interrupt on match delta set register 2 */

} VIRTARM_REALTIMEADAPTER_REGS;

#define VIRTARM_REALTIMEADAPTER_VID                     0x0000045E
#define VIRTARM_REALTIMEADAPTER_PID                     0x00000202

#define VIRTARM_REALTIMEADAPTER_COMMAND_GET             1
#define VIRTARM_REALTIMEADAPTER_COMMAND_SET             2
#define VIRTARM_REALTIMEADAPTER_COMMAND_SETALARM        3
#define VIRTARM_REALTIMEADAPTER_COMMAND_GETALARM        4

#define VIRTARM_REALTIMEADAPTER_INT_COUNTER_MATCH_0     0x00000001
#define VIRTARM_REALTIMEADAPTER_INT_COUNTER_MATCH_1     0x00000002
#define VIRTARM_REALTIMEADAPTER_INT_COUNTER_MATCH_2     0x00000004
#define VIRTARM_REALTIMEADAPTER_INT_ALARM               0x80000000
#define VIRTARM_REALTIMEADAPTER_INT_ALL                 0x80000007

// ---------------------------------------------------------------------
#ifdef __cplusplus
};
#endif

#endif // _VAREALTIMEADAPTERREGS_H_
