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

#ifndef __VIRTARM_DISKADAPTER_H
#define __VIRTARM_DISKADAPTER_H

#include <virtarm.h>
#include <Adapters\VADiskAdapterRegs.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

void        VIRTARMDISK_Init(volatile VIRTARM_DISKADAPTER_REGS *apAdapter);

UINT32      VIRTARMDISK_GetMaxNumSectors(volatile VIRTARM_DISKADAPTER_REGS *apAdapter);

EFI_STATUS  VIRTARMDISK_ReadSectors(volatile VIRTARM_DISKADAPTER_REGS *apAdapter, UINT32 aStartSector, UINTN aNumSectors, UINT8 *apBuffer);

EFI_STATUS  VIRTARMDISK_WriteSectors(volatile VIRTARM_DISKADAPTER_REGS *apAdapter, UINT32 aStartSector, UINTN aNumSectors, UINT8 *apBuffer, BOOLEAN aDoFlushAtEnd);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif  // __VIRTARM_DISKADAPTER_H