//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2020, Kurt Kennett
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
#ifndef __K2OS_H
#define __K2OS_H

#include "k2osbase.h"

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

#define K2OS_TIMEOUT_INFINITE   ((UINT_PTR)-1)
#define K2OS_HFTIMEOUT_INFINITE ((UINT64)-1)

typedef void * K2OS_TOKEN;
typedef void * K2OS_MAILBOX;
typedef void * K2OS_XDL;
typedef void * K2OS_IPCEND;

#define K2OS_MSG_CONTROL_SYSTEM_FLAG     0x80000000
#define K2OS_MSG_CONTROL_SYSTEM_FUNC    (0x40000000 | K2OS_MSG_CONTROL_SYSTEM_FLAG)
#define K2OS_MSG_CONTROL_SYSTEM_IPC     (0x20000000 | K2OS_MSG_CONTROL_SYSTEM_FLAG)

typedef struct _K2OS_MSG K2OS_MSG;
struct _K2OS_MSG
{
    UINT_PTR mControl;
    UINT_PTR mPayload[3];
};

typedef UINT_PTR (*K2OS_pf_THREAD_ENTRY)(void *apArgument);

typedef struct _K2OS_USER_THREAD_CONFIG K2OS_USER_THREAD_CONFIG;
struct _K2OS_USER_THREAD_CONFIG
{
    UINT8   mStackPages;
    UINT8   mPriority;
    UINT8   mAffinityMask;
    UINT8   mQuantum;
};

typedef struct _K2OS_CRITSEC K2OS_CRITSEC;
struct _K2OS_CRITSEC
{
    UINT8 mOpaque[K2OS_CACHELINE_BYTES * 2];
};

typedef enum _K2OS_User_MapType K2OS_User_MapType;
enum _K2OS_User_MapType
{
    K2OS_MapType_Invalid,
    K2OS_MapType_Text,
    K2OS_MapType_Data_ReadOnly,
    K2OS_MapType_Data_ReadWrite,
    K2OS_MapType_Data_CopyOnWrite,
    K2OS_MapType_Thread_Stack,
    K2OS_MapType_MemMappedIo_ReadOnly,
    K2OS_MapType_MemMappedIo_ReadWrite,
    K2OS_MapType_Count,
};

#define K2OS_IFCLASS_DEVMGR     1
#define K2OS_IFCLASS_DEVICE     2
#define K2OS_IFCLASS_RPC        3

// {53DF54FD-D431-4027-86E5-BA5CC6DFD93D}
#define K2OS_IFACE_ID_DEVMGR_HOST { 0x53df54fd, 0xd431, 0x4027, { 0x86, 0xe5, 0xba, 0x5c, 0xc6, 0xdf, 0xd9, 0x3d } }

// {60C8FB73-F82C-465F-B364-DF8E46CB62D3}
#define K2OS_IFACE_ID_DEVICE      { 0x60c8fb73, 0xf82c, 0x465f, { 0xb3, 0x64, 0xdf, 0x8e, 0x46, 0xcb, 0x62, 0xd3 } }

typedef struct _K2OS_IFENUM_ENTRY K2OS_IFENUM_ENTRY;
struct _K2OS_IFENUM_ENTRY
{
    UINT_PTR    mClassCode;
    K2_GUID128  SpecificGuid;
    UINT_PTR    mProcessId;
    UINT_PTR    mGlobalInstanceId;
};

typedef void (*K2OS_IPCEND_pf_OnConnect)(K2OS_IPCEND aEndpoint, void *apContext, UINT_PTR aRemoteMaxMsgBytes);
typedef void (*K2OS_IPCEND_pf_OnRecv)(K2OS_IPCEND aEndpoint, void *apContext, UINT8 const *apData, UINT_PTR aByteCount);
typedef void (*K2OS_IPCEND_pf_OnDisconnect)(K2OS_IPCEND aEndpoint, void *apContext);
typedef void (*K2OS_IPCEND_pf_OnRejected)(K2OS_IPCEND aEndpoint, void *apContext, UINT_PTR aReasonCode);

typedef struct _K2OS_IPCEND_FUNCTAB K2OS_IPCEND_FUNCTAB;
struct _K2OS_IPCEND_FUNCTAB
{
    K2OS_IPCEND_pf_OnConnect    OnConnect;
    K2OS_IPCEND_pf_OnRecv       OnRecv;
    K2OS_IPCEND_pf_OnDisconnect OnDisconnect;
    K2OS_IPCEND_pf_OnRejected   OnRejected;
};

K2_PACKED_PUSH
typedef struct _K2OS_IRQ_LINE_CONFIG    K2OS_IRQ_LINE_CONFIG;
struct _K2OS_IRQ_LINE_CONFIG
{
    UINT8 mIsEdgeTriggered;
    UINT8 mIsActiveLow;
    UINT8 mShareConfig;
    UINT8 mWakeConfig;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

K2_PACKED_PUSH
typedef struct _K2OS_IRQ_CONFIG         K2OS_IRQ_CONFIG;
struct _K2OS_IRQ_CONFIG
{
    UINT16                  mSourceIrq;
    UINT16                  mDestCoreIx;
    K2OS_IRQ_LINE_CONFIG    Line;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

K2_PACKED_PUSH
typedef struct _K2OS_IOPORT_RANGE       K2OS_IOPORT_RANGE;
struct _K2OS_IOPORT_RANGE
{
    UINT16  mBasePort;
    UINT16  mSizeBytes;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

K2_PACKED_PUSH
typedef struct _K2OS_PHYSADDR_RANGE32  K2OS_PHYSADDR_RANGE32;
struct _K2OS_PHYSADDR_RANGE32
{
    UINT32  mBaseAddr;
    UINT32  mSizeBytes;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

K2_PACKED_PUSH
typedef struct _K2OS_PHYSADDR_RANGE64  K2OS_PHYSADDR_RANGE64;
struct _K2OS_PHYSADDR_RANGE64
{
    UINT64  mBaseAddr;
    UINT64  mSizeBytes;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

#if K2_TARGET_ARCH_IS_32BIT
#define K2OS_PHYSADDR_RANGE K2OS_PHYSADDR_RANGE32
#else
#define K2OS_PHYSADDR_RANGE K2OS_PHYSADDR_RANGE64
#endif

#define K2OS_DEVICE_DESC_FRIENDLY_CHARS 28

typedef struct _K2OS_DEVICE_DESC K2OS_DEVICE_DESC;
struct _K2OS_DEVICE_DESC
{
    UINT_PTR        mSystemDeviceInstanceId;
    char            mFriendlyDesc[K2OS_DEVICE_DESC_FRIENDLY_CHARS];
    K2_GUID128      mClassGuid;
    K2_DEVICE_IDENT Ident;
};

typedef K2OS_TOKEN  K2OS_WAITABLE_TOKEN;
typedef K2OS_TOKEN  K2OS_PAGEARRAY_TOKEN;
typedef K2OS_TOKEN  K2OS_MAP_TOKEN;
typedef K2OS_TOKEN  K2OS_MAILSLOT_TOKEN;
typedef K2OS_TOKEN  K2OS_IFENUM_TOKEN;
typedef K2OS_TOKEN  K2OS_IFSUBS_TOKEN;

typedef K2OS_WAITABLE_TOKEN     K2OS_PROCESS_TOKEN;
typedef K2OS_WAITABLE_TOKEN     K2OS_THREAD_TOKEN;
typedef K2OS_WAITABLE_TOKEN     K2OS_NOTIFY_TOKEN;
typedef K2OS_WAITABLE_TOKEN     K2OS_GATE_TOKEN;
typedef K2OS_WAITABLE_TOKEN     K2OS_SEMAPHORE_TOKEN;
typedef K2OS_WAITABLE_TOKEN     K2OS_ALARM_TOKEN;
typedef K2OS_WAITABLE_TOKEN     K2OS_INTERRUPT_TOKEN;

//
//------------------------------------------------------------------------
//

UINT_PTR    K2_CALLCONV_REGS K2OS_Debug_OutputString(char const *apStr);
void        K2_CALLCONV_REGS K2OS_Debug_Break(void);

//
//------------------------------------------------------------------------
//

void K2_CALLCONV_REGS K2OS_RaiseException(K2STAT aExceptionCode);

//
//------------------------------------------------------------------------
//

//
// K2OS_SYSINFO_FWINFO
//
typedef struct _K2OS_FWINFO K2OS_FWINFO;
struct _K2OS_FWINFO
{
    UINT_PTR mFwBasePhys;
    UINT_PTR mFwSizeBytes;
    UINT_PTR mFacsPhys;
    UINT_PTR mXFacsPhys;
};

//
// K2OS_SYSINFO_PHYSADDR
//
typedef struct _K2OS_PHYSADDR_INFO K2OS_PHYSADDR_INFO;
struct _K2OS_PHYSADDR_INFO
{
    UINT_PTR mBaseAddr;
    UINT_PTR mSizeBytes;
};

//
// K2OS_SYSINFO_IOADDR
//
typedef struct _K2OS_IOADDR_INFO K2OS_IOADDR_INFO;
struct _K2OS_IOADDR_INFO
{
    UINT16  mPortBase;
    UINT16  mPortCount;
};

#define K2OS_SYSINFO_CPUCOUNT       0
#define K2OS_SYSINFO_FWINFO         1
#define K2OS_SYSINFO_PHYSADDR       2
#define K2OS_SYSINFO_IOADDR         3

UINT_PTR K2_CALLCONV_REGS   K2OS_System_GetCpuCoreCount(void);

K2STAT                      K2OS_System_GetInfo(UINT_PTR aInfoId, UINT_PTR aInfoIx, void *apTargetBuffer, UINT_PTR *apIoBytes);

BOOL     K2_CALLCONV_REGS   K2OS_System_ProcessMsg(K2OS_MSG const *apMsg);

void     K2_CALLCONV_REGS   K2OS_System_GetMsTick(UINT64 *apRetMsTick);
UINT32   K2_CALLCONV_REGS   K2OS_System_GetMsTick32(void);

UINT32   K2_CALLCONV_REGS   K2OS_System_GetHfFreq(void);
void     K2_CALLCONV_REGS   K2OS_System_GetHfTick(UINT64 *apRetHfTick);

void     K2_CALLCONV_REGS   K2OS_System_HfTickFromMsTick(UINT64 *apRetHfTick, UINT64 const *apMsTick);
void     K2_CALLCONV_REGS   K2OS_System_MsTickFromHfTick(UINT64 *apRetMsTick, UINT64 const *apHfTick);

void     K2_CALLCONV_REGS   K2OS_System_HfTickFromMsTick32(UINT64 *apRetHfTick, UINT32 aMsTick32);
UINT32   K2_CALLCONV_REGS   K2OS_System_MsTick32FromHfTick(UINT64 const *apHfTick);

//
//------------------------------------------------------------------------
//

UINT_PTR    K2_CALLCONV_REGS K2OS_Process_GetId(void);
void        K2_CALLCONV_REGS K2OS_Process_Exit(UINT_PTR aExitCode);

//
//------------------------------------------------------------------------
//

K2OS_THREAD_TOKEN            K2OS_Thread_Create(K2OS_pf_THREAD_ENTRY aEntryPoint, void *apArg, K2OS_USER_THREAD_CONFIG const *apConfig, UINT_PTR *apRetThreadId);
UINT_PTR    K2_CALLCONV_REGS K2OS_Thread_GetId(void);
K2STAT      K2_CALLCONV_REGS K2OS_Thread_GetLastStatus(void);
K2STAT      K2_CALLCONV_REGS K2OS_Thread_SetLastStatus(K2STAT aStatus);
UINT_PTR    K2_CALLCONV_REGS K2OS_Thread_GetCpuCoreAffinityMask(void);
UINT_PTR    K2_CALLCONV_REGS K2OS_Thread_SetCpuCoreAffinityMask(UINT_PTR aNewAffinity);
void        K2_CALLCONV_REGS K2OS_Thread_Exit(UINT_PTR aExitCode);
BOOL        K2_CALLCONV_REGS K2OS_Thread_Sleep(UINT_PTR aTimeoutMs);
void        K2_CALLCONV_REGS K2OS_Thread_StallMicroseconds(UINT_PTR aMicroseconds);
UINT_PTR    K2_CALLCONV_REGS K2OS_Thread_GetExitCode(K2OS_THREAD_TOKEN aThreadToken);

//
//------------------------------------------------------------------------
//

UINT_PTR K2OS_Wait_OnMailbox(K2OS_MAILBOX aMailbox, K2OS_MSG *apRetMsg, UINT_PTR aTimeoutMs);

UINT_PTR K2_CALLCONV_REGS K2OS_Wait_One(K2OS_WAITABLE_TOKEN aToken, UINT_PTR aTimeoutMs);
UINT_PTR K2OS_Wait_OnMailboxAndOne(K2OS_MAILBOX aMailbox, K2OS_MSG *apRetMsg, K2OS_WAITABLE_TOKEN aToken, UINT_PTR aTimeoutMs);

UINT_PTR K2OS_Wait_Many(UINT_PTR aCount, K2OS_WAITABLE_TOKEN const *apWaitableTokens, BOOL aWaitAll, UINT_PTR aTimeoutMs);
UINT_PTR K2OS_Wait_OnMailboxAndMany(K2OS_MAILBOX aMailbox, K2OS_MSG *apRetMsg, UINT_PTR aCount, K2OS_WAITABLE_TOKEN const *apWaitableTokens, UINT_PTR aTimeoutMs);

//
//------------------------------------------------------------------------
//

BOOL K2_CALLCONV_REGS K2OS_CritSec_Init(K2OS_CRITSEC *apSec);
BOOL K2_CALLCONV_REGS K2OS_CritSec_TryEnter(K2OS_CRITSEC *apSec);
void K2_CALLCONV_REGS K2OS_CritSec_Enter(K2OS_CRITSEC *apSec);
void K2_CALLCONV_REGS K2OS_CritSec_Leave(K2OS_CRITSEC *apSec);
BOOL K2_CALLCONV_REGS K2OS_CritSec_Done(K2OS_CRITSEC *apSec);

//
//------------------------------------------------------------------------
//

BOOL K2_CALLCONV_REGS K2OS_Tls_AllocSlot(UINT_PTR *apRetNewIndex);
BOOL K2_CALLCONV_REGS K2OS_Tls_FreeSlot(UINT_PTR aSlotIndex);
BOOL K2_CALLCONV_REGS K2OS_Tls_SetValue(UINT_PTR aSlotIndex, UINT_PTR aValue);
BOOL K2_CALLCONV_REGS K2OS_Tls_GetValue(UINT_PTR aSlotIndex, UINT_PTR *apRetValue);

//
//------------------------------------------------------------------------
//

void * K2_CALLCONV_REGS K2OS_Heap_Alloc(UINT_PTR aByteCount);
BOOL   K2_CALLCONV_REGS K2OS_Heap_Free(void *aPtr);

//
//------------------------------------------------------------------------
//

BOOL   K2_CALLCONV_REGS K2OS_Token_Destroy(K2OS_TOKEN aToken);

//
//------------------------------------------------------------------------
//

K2OS_NOTIFY_TOKEN K2_CALLCONV_REGS K2OS_Notify_Create(BOOL aInitSignalled);
BOOL              K2_CALLCONV_REGS K2OS_Notify_Signal(K2OS_NOTIFY_TOKEN aTokNotify);

//
//------------------------------------------------------------------------
//

K2OS_GATE_TOKEN K2_CALLCONV_REGS K2OS_Gate_Create(BOOL aInitOpen);
BOOL            K2_CALLCONV_REGS K2OS_Gate_Open(K2OS_GATE_TOKEN aTokGate);
BOOL            K2_CALLCONV_REGS K2OS_Gate_Close(K2OS_GATE_TOKEN aTokGate);

//
//------------------------------------------------------------------------
//

K2OS_SEMAPHORE_TOKEN  K2_CALLCONV_REGS K2OS_Semaphore_Create(UINT_PTR aMaxCount, UINT_PTR aInitCount);
BOOL                                   K2OS_Semaphore_Inc(K2OS_SEMAPHORE_TOKEN aTokSemaphore, UINT_PTR aIncCount, UINT_PTR *apRetNewCount);

//
//------------------------------------------------------------------------
//

K2OS_PAGEARRAY_TOKEN K2_CALLCONV_REGS K2OS_PageArray_Create(UINT_PTR aPageCount);
UINT_PTR             K2_CALLCONV_REGS K2OS_PageArray_GetLength(K2OS_PAGEARRAY_TOKEN aTokPageArray);

//
//------------------------------------------------------------------------
//

UINT_PTR    K2_CALLCONV_REGS K2OS_Virt_Reserve(UINT_PTR aPageCount);
UINT_PTR    K2_CALLCONV_REGS K2OS_Virt_Get(UINT_PTR aVirtAddr, UINT_PTR *apRetPageCount);
BOOL        K2_CALLCONV_REGS K2OS_Virt_Release(UINT_PTR aVirtAddr);

//
//------------------------------------------------------------------------
//

K2OS_MAP_TOKEN                          K2OS_Map_Create(K2OS_PAGEARRAY_TOKEN aTokPageArray, UINT_PTR aPageOffset, UINT_PTR aPageCount, UINT_PTR aVirtAddr, K2OS_User_MapType aMapType);
K2OS_PAGEARRAY_TOKEN K2_CALLCONV_REGS   K2OS_Map_AcquirePageArray(K2OS_MAP_TOKEN aTokMap, UINT_PTR * apRetStartPageIndex);
K2OS_MAP_TOKEN                          K2OS_Map_Acquire(UINT_PTR aProcAddr, K2OS_User_MapType *apRetMapType, UINT_PTR *apRetMapPageIndex);
UINT_PTR                                K2OS_Map_GetInfo(K2OS_MAP_TOKEN aTokMap, K2OS_User_MapType *apRetMapType, UINT_PTR *apRetMapPageCount);

//
//------------------------------------------------------------------------
//

K2OS_MAILBOX    K2_CALLCONV_REGS K2OS_Mailbox_Create(K2OS_MAILSLOT_TOKEN *apRetTokMailslot);
BOOL            K2_CALLCONV_REGS K2OS_Mailbox_Delete(K2OS_MAILBOX aMailbox);

BOOL            K2_CALLCONV_REGS K2OS_Mailslot_Send(K2OS_MAILSLOT_TOKEN aSlot, K2OS_MSG const *apMsg);

//
//------------------------------------------------------------------------
//

K2OS_IFENUM_TOKEN   K2OS_IfEnum_Create(UINT_PTR aProcessId, UINT_PTR aClassCode, K2_GUID128 const * apSpecific);
BOOL                K2OS_IfEnum_Reset(K2OS_IFENUM_TOKEN aIfEnumToken);
BOOL                K2OS_IfEnum_Next(K2OS_IFENUM_TOKEN aIfEnumToken, K2OS_IFENUM_ENTRY *apEntryBuffer, UINT_PTR *apIoEntryBufferCount);

#define K2OS_SYSTEM_MSG_IFINST_ARRIVE   (K2OS_MSG_CONTROL_SYSTEM_FLAG | 1)
#define K2OS_SYSTEM_MSG_IFINST_DEPART   (K2OS_MSG_CONTROL_SYSTEM_FLAG | 2)
K2OS_IFSUBS_TOKEN   K2OS_IfSubs_Create(K2OS_MAILBOX aMailbox, UINT_PTR aClassCode, K2_GUID128 const *apSpecific, UINT_PTR aBacklogCount, BOOL aProcSelfNotify, void *apContext);

UINT_PTR    K2OS_IfInstance_Create(K2OS_MAILBOX aMailbox, UINT_PTR aRequestorProcessId, void *apContext);
BOOL        K2OS_IfInstance_Publish(UINT_PTR aInstanceId, UINT_PTR aClassCode, K2_GUID128 const *apSpecific);
BOOL        K2OS_IfInstance_Remove(UINT_PTR aInstanceId);

//
//------------------------------------------------------------------------
//

#define K2OS_SYSTEM_MSG_IPC_REQUEST     (K2OS_MSG_CONTROL_SYSTEM_IPC | 10)

K2OS_IPCEND                  K2OS_Ipc_CreateEndpoint(K2OS_MAILBOX aMailbox, UINT_PTR aMaxMsgCount, UINT_PTR aMaxMsgBytes, void *apContext, K2OS_IPCEND_FUNCTAB const *apFuncTab);
UINT_PTR                     K2OS_Ipc_GetEndpointParam(K2OS_IPCEND aEndpoint, UINT_PTR *apRetMaxMsgCount, void **appRetContext);
BOOL        K2_CALLCONV_REGS K2OS_Ipc_SendRequest(K2OS_IPCEND aEndpoint, UINT_PTR aGlobalInterfaceInstanceId);
BOOL        K2_CALLCONV_REGS K2OS_Ipc_AcceptRequest(K2OS_IPCEND aEndpoint, UINT_PTR aRequestId);
BOOL        K2_CALLCONV_REGS K2OS_Ipc_RejectRequest(UINT_PTR aRequestId, UINT_PTR aReasonCode);
BOOL                         K2OS_Ipc_Send(K2OS_IPCEND aEndpoint, void const *apBuffer, UINT_PTR aByteCount);
BOOL                         K2OS_Ipc_SendVector(K2OS_IPCEND aEndpoint, UINT_PTR aVectorCount, K2MEM_BUFVECTOR const *apVectors);
BOOL        K2_CALLCONV_REGS K2OS_Ipc_DisconnectEndpoint(K2OS_IPCEND aEndpoint);
BOOL        K2_CALLCONV_REGS K2OS_Ipc_DeleteEndpoint(K2OS_IPCEND aEndpoint);
BOOL        K2_CALLCONV_REGS K2OS_Ipc_ProcessMsg(K2OS_MSG const *apMsg);

//
//------------------------------------------------------------------------
//

K2OS_ALARM_TOKEN K2_CALLCONV_REGS K2OS_Alarm_Create(UINT_PTR aPeriodMs, BOOL aIsPeriodic);

//
//------------------------------------------------------------------------ 
// 

BOOL K2_CALLCONV_REGS K2OS_Intr_SetEnable(K2OS_INTERRUPT_TOKEN aInterruptToken, BOOL aSetEnable);
BOOL K2_CALLCONV_REGS K2OS_Intr_End(K2OS_INTERRUPT_TOKEN aInterruptToken);

//
//------------------------------------------------------------------------
//

K2OS_XDL    K2_CALLCONV_REGS K2OS_Xdl_Acquire(char const *apFilePath);
BOOL        K2_CALLCONV_REGS K2OS_Xdl_AddRef(K2OS_XDL aXdl);
K2OS_XDL    K2_CALLCONV_REGS K2OS_Xdl_AddRefContaining(UINT_PTR aAddr);
BOOL        K2_CALLCONV_REGS K2OS_Xdl_Release(K2OS_XDL aXdl);
UINT_PTR                     K2OS_Xdl_FindExport(K2OS_XDL aXdl, BOOL aIsText, char const *apExportName);
BOOL                         K2OS_Xdl_GetIdent(K2OS_XDL aXdl, char * apNameBuf, UINT_PTR aNameBufLen, K2_GUID128  *apRetID);

//
//------------------------------------------------------------------------
//

UINT_PTR    K2_CALLCONV_REGS K2OS_Device_GetCount(K2_GUID128 const *apClassFilter);
K2STAT                       K2OS_Device_GetDesc(K2_GUID128 const *apClassFilter, UINT_PTR aMatchIndex, K2OS_DEVICE_DESC *apRetDesc);
K2STAT                       K2OS_Device_Attach(UINT_PTR aSystemDeviceInstanceId, K2OS_MAILSLOT_TOKEN aMailslotToken, UINT_PTR *apRetRpcIfaceId, UINT_PTR *apRetUserObjectId);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __K2OS_H