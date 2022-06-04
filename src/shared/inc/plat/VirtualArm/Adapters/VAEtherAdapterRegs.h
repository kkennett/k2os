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

#ifndef _VAETHERADAPTERREGS_H_
#define _VAETHERADAPTERREGS_H_

/*--------------------------------------------*
         RAW MULTI-PLATFORM INCLUDE FILE
         DO NOT ASSUME ANY PARTICULAR OS
           OR ANY PARTICULAR COMPILER
  --------------------------------------------*/

#include "virtarm.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ETHERNET_MTU
#define ETHERNET_MTU    1518
#endif

// ---------------------------------------------------------------------

typedef struct _VIRTARM_ETHERADAPTER_REGS
{
/* 0x00 */ _REG32   mVID;           /* ro : Vendor ID */
/* 0x04 */ _REG32   mPID;           /* ro : Product ID */
/* 0x08 */ _REG32   mVersion;       /* ro : high 16 bits = major. low 16 bits = minor */
/* 0x0C */ _REG32   mReserved;      /* ro : reserved. SBZ */

/* 0x10 */ _REG32   mControlStatus;
/* 0x14 */ _REG32   mIntStatus;
/* 0x18 */ _REG32   mIntMask;
/* 0x1C */ _REG32   mIntPend;

/* 0x20 */ _REG32   mIntSetMask;
/* 0x24 */ _REG32   mIntClrMask;

/* 0x28 */ _REG16   mFilterControl_16;
/* 0x2A */ _REG16   mMAC01_16;
/* 0x2C */ _REG16   mMAC23_16;
/* 0x2E */ _REG16   mMAC45_16;

/* 0x30 */ _REG32   mIOCommandStatus;
/* 0x34 */ _REG32   mIOVar1;
/* 0x38 */ _REG32   mIOVar2;

} VIRTARM_ETHERADAPTER_REGS;

#define VIRTARM_ETHERADAPTER_VID                     0x0000045E
#define VIRTARM_ETHERADAPTER_PID                     0x00000505

/* SRAM is used entirely for frame i/o */
#define VIRTARM_ETHERADAPTER_SRAM_OFFSET_FRAMESBUFFER   0
#define VIRTARM_ETHERADAPTER_FRAME_MAX_BYTES            ETHERNET_MTU
#define VIRTARM_ETHERADAPTER_MAX_NUM_FRAMES             ((VIRTARM_PHYSSIZE_ADAPTER_SRAM - VIRTARM_ETHERADAPTER_SRAM_OFFSET_FRAMESBUFFER) / VIRTARM_ETHERADAPTER_FRAME_MAX_BYTES)

/* read - whether the interface is enabled and can send/recv */
/* write - toggle to opposite state */
#define VIRTARM_ETHERADAPTER_CTRLSTAT_ENABLE            0x00000001

/* read - whether a cable is connected or not */
/* write - nothing */
#define VIRTARM_ETHERADAPTER_CTRLSTAT_CABLESENSE        0x80000000

#define VIRTARM_ETHERADAPTER_INTR_RECV_AVAIL            0x00000001 
#define VIRTARM_ETHERADAPTER_INTR_XMIT_EMPTY            0x00000002
#define VIRTARM_ETHERADAPTER_INTR_ALL                   0x00000003

#define VIRTARM_ETHERADAPTER_FILTER_ALLOW_NONE          0x00000000
#define VIRTARM_ETHERADAPTER_FILTER_ALLOW_DIRECTED      0x00000001
#define VIRTARM_ETHERADAPTER_FILTER_ALLOW_MULTICAST     0x00000002
#define VIRTARM_ETHERADAPTER_FILTER_ALLOW_ALL_MULTICAST 0x00000004
#define VIRTARM_ETHERADAPTER_FILTER_ALLOW_BROADCAST     0x00000008
#define VIRTARM_ETHERADAPTER_FILTER_ALLOW_PROMISCUOUS   0x00000020
#define VIRTARM_ETHERADAPTER_FILTER_MASK                0x0000002F

/* IO commands for frames */


#define VIRTARM_ETHERADAPTER_IOCMD_GETXMITBUF       1   
/* 
   IOCommandStatus  IN: command code
   IOVar1           IN: ignored. Set to zero for back compat
   IOVar2           IN: ignored. Set to zero for back compat
   ------
   IOCommandStatus OUT: 0 for success. nonzero = error code
   IOVar1          OUT: buffer OFFSET from start of SRAM for xmit buffer or 0xFFFFFFFF for no buffer
   IOVar2          OUT: undefined
*/


#define VIRTARM_ETHERADAPTER_IOCMD_DOXMIT           2   
/* 
   IOCommandStatus  IN: command code
   IOVar1           IN: buffer OFFSET from start of SRAM (retrieved via GETXMITBUF)
   IOVar2           IN: data bytes in frame to transmit
   ------
   IOCommandStatus OUT: 0 for success. nonzero = error code
   IOVar1          OUT: undefined
   IOVar2          OUT: undefined
*/


#define VIRTARM_ETHERADAPTER_IOCMD_GETRECVBUF       3   
/* 
   IOCommandStatus  IN: command code
   IOVar1           IN: ignored. Set to zero for back compat
   IOVar2           IN: ignored. Set to zero for back compat
   ------
   IOCommandStatus OUT: 0 for success. nonzero = error code
   IOVar1          OUT: buffer OFFSET from start of SRAM or 0xFFFFFFFF for no buffer
   IOVar2          OUT: # of data bytes in buffer
*/


#define VIRTARM_ETHERADAPTER_IOCMD_RELEASE          4   
/* 
   IOCommandStatus  IN: command code
   IOVar1           IN: buffer OFFSET from start of SRAM (retrieved via GETXMITBUF or GETRECVBUF)
   IOVar2           IN: ignored. Set to zero for back compat
   ------
   IOCommandStatus OUT: 0 for success. nonzero = error code
   IOVar1          OUT: undefined
   IOVar2          OUT: undefined
*/

#define VIRTARM_ETHERADAPTER_IOCMD_QUERY_BUFSIZE    5
/* 
   IOCommandStatus  IN: command code
   IOVar1           IN: ignored. Set to zero for back compat
   IOVar2           IN: ignored. Set to zero for back compat
   ------
   IOCommandStatus OUT: 0 for success. nonzero = error code
   IOVar1          OUT: max # of recv packet buffers
   IOVar2          OUT: max # of xmit packet buffers
*/

#define VIRTARM_ETHERADAPTER_IOCMD_HOLDRECVBUF      6
/* 
   IOCommandStatus  IN: command code
   IOVar1           IN: ignored. Set to zero for back compat
   IOVar2           IN: ignored. Set to zero for back compat
   ------
   IOCommandStatus OUT: 0 for success. nonzero = error code
   IOVar1          OUT: undefined
   IOVar2          OUT: undefined
*/

/* IO errors - read the command register after a write to get these */
#define VIRTARM_ETHERADAPTER_IOERR_NONE             0
#define VIRTARM_ETHERADAPTER_IOERR_BADCOMMAND       0x807A0001
#define VIRTARM_ETHERADAPTER_IOERR_NODATA           0x807A0002
#define VIRTARM_ETHERADAPTER_IOERR_BADRELEASE       0x807A0003
#define VIRTARM_ETHERADAPTER_IOERR_FRAMETOOBIG      0x807A0004
#define VIRTARM_ETHERADAPTER_IOERR_FRAMETOOSMALL    0x807A0005
#define VIRTARM_ETHERADAPTER_IOERR_BADFRAMEPTR      0x807A0006
#define VIRTARM_ETHERADAPTER_IOERR_NOFREEFRAMES     0x807A0007
#define VIRTARM_ETHERADAPTER_IOERR_NOTTXFRAME       0x807A0008
#define VIRTARM_ETHERADAPTER_IOERR_TXFAILED         0x807A0009


// ---------------------------------------------------------------------

#ifdef __cplusplus
};
#endif

#endif // _VAETHERADAPTERREGS_H_
