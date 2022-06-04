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

#ifndef __VIRTARM_DEBUGADAPTER_H
#define __VIRTARM_DEBUGADAPTER_H

#include <virtarm.h>
#include <Adapters\VADebugAdapterRegs.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
void    VIRTARMDEBUG_Init(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, BOOLEAN aUseAdapterBuffer, BOOLEAN aSetKitlMode);

void    VIRTARMDEBUG_SetKitlMode(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, BOOLEAN aSetKitlMode);

void    VIRTARMDEBUG_OutputString(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, CHAR16 const *apString, UINTN aStrLen);
void    VIRTARMDEBUG_OutputStringZ(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, CHAR16 const *apStringZ);

UINT32  VIRTARMDEBUG_ReadPROM(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, UINT32 aAddress);
void    VIRTARMDEBUG_WritePROM(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, UINT32 aAddress, UINT32 aData);

UINT8 * VIRTARMDEBUG_GetXmitPacketBuffer(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, UINT32 aDataBytesToSend);
UINT32  VIRTARMDEBUG_SendPacket(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, UINT8 *apPacketBuffer);

BOOLEAN VIRTARMDEBUG_IsRecvPacketAvail(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter);
UINT8 * VIRTARMDEBUG_GetRecvPacketBuffer(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, UINT32 *apRetDataBytesRecv);
UINT32  VIRTARMDEBUG_ReleaseBuffer(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter, UINT8 *apPacketBuffer);

void    VIRTARMDEBUG_ClearPacketBuffers(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter);

//------------------------------------------------------------------------------

void    VIRTARMDEBUG_TRANSPORT_SetAdapter(volatile VIRTARM_DEBUGADAPTER_REGS *apAdapter);
BOOLEAN VIRTARMDEBUG_TRANSPORT_Init(BOOLEAN fSetKitlMode);
void    VIRTARMDEBUG_TRANSPORT_DeInit(void);
UINT16  VIRTARMDEBUG_TRANSPORT_SendFrame(UINT8 *apBuffer, UINT16 aLength);
UINT16  VIRTARMDEBUG_TRANSPORT_GetFrame(UINT8 *apBuffer, UINT16 aMaxLength);
void    VIRTARMDEBUG_TRANSPORT_CurrentPacketFilter(UINTN aFilter);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif  // __VIRTARM_DEBUGADAPTER_H