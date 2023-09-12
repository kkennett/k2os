//   
//   BSD 3-Clause License
//   
//   Copyright (c) 2023, Kurt Kennett
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

#define K2OS_TIMEOUT_INFINITE   ((UINT32)-1)
#define K2OS_HFTIMEOUT_INFINITE ((UINT64)-1)

typedef struct _K2OS_TOKEN_OPAQUE K2OS_TOKEN_OPAQUE;
typedef K2OS_TOKEN_OPAQUE *         K2OS_TOKEN;

typedef struct _K2OS_XDL_OPAQUE K2OS_XDL_OPAQUE;
typedef K2OS_XDL_OPAQUE *           K2OS_XDL;

typedef struct _K2OS_IPCEND_OPAQUE K2OS_IPCEND_OPAQUE;
typedef K2OS_IPCEND_OPAQUE *        K2OS_IPCEND;

typedef UINT32 K2OS_IFINST_ID;

typedef UINT32 (*K2OS_pf_THREAD_ENTRY)(void *apArgument);

typedef struct _K2OS_THREAD_CONFIG K2OS_THREAD_CONFIG;
struct _K2OS_THREAD_CONFIG
{
    UINT8   mStackPages;
    UINT8   mPriority;
    UINT8   mAffinityMask;
    UINT8   mQuantum;
};

#define K2OS_THREAD_NAME_BUFFER_CHARS   32

typedef struct _K2OS_CRITSEC K2OS_CRITSEC;
struct _K2OS_CRITSEC
{
    UINT8 mOpaque[K2OS_CACHELINE_BYTES * 2];
};

typedef struct _K2OS_FWINFO K2OS_FWINFO;
struct _K2OS_FWINFO
{
    UINT32 mFwBasePhys;
    UINT32 mFwSizeBytes;
    UINT32 mFacsPhys;
    UINT32 mXFacsPhys;
};

typedef enum _K2OS_VirtToPhys_MapType K2OS_VirtToPhys_MapType;
enum _K2OS_VirtToPhys_MapType
{
    K2OS_MapType_Invalid,
    K2OS_MapType_Text,
    K2OS_MapType_Data_ReadOnly,
    K2OS_MapType_Data_ReadWrite,
    K2OS_MapType_Data_CopyOnWrite,
    K2OS_MapType_Thread_Stack,
    K2OS_MapType_MemMappedIo_ReadOnly,
    K2OS_MapType_MemMappedIo_ReadWrite,
    K2OS_MapType_Write_Thru,
    K2OS_MapType_Count,
};

#define K2OS_SYSPROC_ID                         1

#define K2OS_IFACE_CLASSCODE_RPC                1
#define K2OS_IFACE_CLASSCODE_STORAGE_DEVICE     2

typedef struct _K2OS_IFINST_DETAIL K2OS_IFINST_DETAIL;
struct _K2OS_IFINST_DETAIL
{
    K2OS_IFINST_ID  mInstId;
    UINT32          mClassCode;
    K2_GUID128      SpecificGuid;
    UINT32          mOwningProcessId;
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
typedef struct _K2OS_IRQ_CONFIG     K2OS_IRQ_CONFIG;
struct _K2OS_IRQ_CONFIG
{
    UINT16                  mSourceIrq;
    UINT16                  mDestCoreIx;
    K2OS_IRQ_LINE_CONFIG    Line;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

K2_PACKED_PUSH
typedef struct _K2OS_IOPORT_RANGE   K2OS_IOPORT_RANGE;
struct _K2OS_IOPORT_RANGE
{
    UINT16  mBasePort;
    UINT16  mSizeBytes;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

K2_PACKED_PUSH
typedef struct _K2OS_PHYSADDR_RANGE K2OS_PHYSADDR_RANGE;
struct _K2OS_PHYSADDR_RANGE
{
    UINT32  mBaseAddr;
    UINT32  mSizeBytes;
} K2_PACKED_ATTRIB;
K2_PACKED_POP

#define K2OS_MSGTYPE_SYSTEM_FLAG        0x8000
#define K2OS_SYSTEM_MSGTYPE_IFINST      (K2OS_MSGTYPE_SYSTEM_FLAG | 1)
#define K2OS_SYSTEM_MSGTYPE_IPCEND      (K2OS_MSGTYPE_SYSTEM_FLAG | 2)
#define K2OS_SYSTEM_MSGTYPE_IPC         (K2OS_MSGTYPE_SYSTEM_FLAG | 3)
#define K2OS_SYSTEM_MSGTYPE_SYSPROC     (K2OS_MSGTYPE_SYSTEM_FLAG | 4)
#define K2OS_SYSTEM_MSGTYPE_DDK         (K2OS_MSGTYPE_SYSTEM_FLAG | 5)
#define K2OS_SYSTEM_MSGTYPE_RPC         (K2OS_MSGTYPE_SYSTEM_FLAG | 6)

K2_PACKED_PUSH
typedef struct _K2OS_MSG K2OS_MSG;
struct _K2OS_MSG
{
    UINT16  mType;
    UINT16  mShort;
    UINT32  mPayload[3];
} K2_PACKED_ATTRIB;
K2_PACKED_POP

typedef void (*K2OS_IPCEND_pf_Callback_OnConnect)(K2OS_IPCEND aEndpoint, void *apContext, UINT32 aRemoteMaxMsgBytes);
typedef void (*K2OS_IPCEND_pf_Callback_OnRecv)(K2OS_IPCEND aEndpoint, void *apContext, UINT8 const *apData, UINT32 aByteCount);
typedef void (*K2OS_IPCEND_pf_Callback_OnDisconnect)(K2OS_IPCEND aEndpoint, void *apContext);
typedef void (*K2OS_IPCEND_pf_Callback_OnRejected)(K2OS_IPCEND aEndpoint, void *apContext, UINT32 aReasonCode);

typedef struct _K2OS_IPCPROCESSMSG_CALLBACKS K2OS_IPCPROCESSMSG_CALLBACKS;
struct _K2OS_IPCPROCESSMSG_CALLBACKS
{
    K2OS_IPCEND_pf_Callback_OnConnect    OnConnect;
    K2OS_IPCEND_pf_Callback_OnRecv       OnRecv;
    K2OS_IPCEND_pf_Callback_OnDisconnect OnDisconnect;
    K2OS_IPCEND_pf_Callback_OnRejected   OnRejected;
};

typedef K2OS_TOKEN  K2OS_WAITABLE_TOKEN;
typedef K2OS_TOKEN  K2OS_PAGEARRAY_TOKEN;
typedef K2OS_TOKEN  K2OS_VIRTMAP_TOKEN;
typedef K2OS_TOKEN  K2OS_IFINST_TOKEN;
typedef K2OS_TOKEN  K2OS_IFENUM_TOKEN;
typedef K2OS_TOKEN  K2OS_IFSUBS_TOKEN;

typedef K2OS_WAITABLE_TOKEN K2OS_PROCESS_TOKEN;
typedef K2OS_WAITABLE_TOKEN K2OS_THREAD_TOKEN;
typedef K2OS_WAITABLE_TOKEN K2OS_NOTIFY_TOKEN;
typedef K2OS_WAITABLE_TOKEN K2OS_GATE_TOKEN;
typedef K2OS_WAITABLE_TOKEN K2OS_SEMAPHORE_TOKEN;
typedef K2OS_WAITABLE_TOKEN K2OS_ALARM_TOKEN;
typedef K2OS_WAITABLE_TOKEN K2OS_INTERRUPT_TOKEN;
typedef K2OS_WAITABLE_TOKEN K2OS_MAILBOX_TOKEN;

#define K2OS_THREAD_WAIT_MAX_ITEMS 32

typedef enum _K2OS_WaitResult K2OS_WaitResult;
enum _K2OS_WaitResult
{
    K2OS_Wait_Signalled_0 = 0,
    K2OS_Wait_Abandoned_0 = 64,
    K2OS_Wait_Failed_0 = 128,
    K2OS_Wait_Failed_SleepForever = 253,
    K2OS_Wait_TooManyTokens = 254,
    K2OS_Wait_TimedOut = 255
};

typedef struct _K2OS_CREATED_PROCESS K2OS_CREATED_PROCESS;
struct _K2OS_CREATED_PROCESS
{
    UINT32              mProcessId;
    UINT32              mMainThreadId;
    K2OS_PROCESS_TOKEN  mTokProcess;
    K2OS_THREAD_TOKEN   mTokThread;
};

// {0577283B-90AA-48D8-A10F-B6E44BAF0C3A}
#define K2OS_IFACE_RPC_SERVER_CLASSID { 0x577283b, 0x90aa, 0x48d8, { 0xa1, 0xf, 0xb6, 0xe4, 0x4b, 0xaf, 0xc, 0x3a } }

typedef struct _K2OS_RPC_CLASS_OPAQUE       K2OS_RPC_CLASS_OPAQUE;
typedef struct _K2OS_RPC_OBJ_HANDLE_OPAQUE  K2OS_RPC_OBJ_HANDLE_OPAQUE;
typedef struct _K2OS_RPC_OBJ_OPAQUE         K2OS_RPC_OBJ_OPAQUE;
typedef struct _K2OS_RPC_IFINST_OPAQUE      K2OS_RPC_IFINST_OPAQUE;

typedef K2OS_RPC_CLASS_OPAQUE *         K2OS_RPC_CLASS;
typedef K2OS_RPC_OBJ_HANDLE_OPAQUE *    K2OS_RPC_OBJ_HANDLE;
typedef K2OS_RPC_OBJ_OPAQUE *           K2OS_RPC_OBJ;
typedef K2OS_RPC_IFINST_OPAQUE *        K2OS_RPC_IFINST;

typedef struct _K2OS_RPC_OBJECT_CLASSDEF K2OS_RPC_OBJECT_CLASSDEF;

typedef struct _K2OS_RPC_OBJECT_CREATE K2OS_RPC_OBJECT_CREATE;
struct _K2OS_RPC_OBJECT_CREATE
{
    K2OS_GATE_TOKEN mRemoteDisconnectedGateToken;
    UINT32          mClassRegisterContext;
    UINT32          mCreatorProcessId;
    UINT32          mCreatorContext;
};
typedef K2STAT(*K2OS_RPC_pf_Object_Create)(K2OS_RPC_OBJ aObject, K2OS_RPC_OBJECT_CREATE const *apCreate, UINT32 *apRetContext, BOOL *apRetSingleUsage);

typedef struct _K2OS_RPC_CALLARGS K2OS_RPC_CALLARGS;
struct _K2OS_RPC_CALLARGS
{
    UINT32          mMethodId;
    UINT8 const *   mpInBuf;
    UINT8 *         mpOutBuf;
    UINT32          mInBufByteCount;
    UINT32          mOutBufByteCount;
};

typedef struct _K2OS_RPC_OBJECT_CALL K2OS_RPC_OBJECT_CALL;
struct _K2OS_RPC_OBJECT_CALL
{
    K2OS_GATE_TOKEN     mRemoteDisconnectedGateToken;
    K2OS_RPC_CALLARGS   Args;
};
typedef K2STAT(*K2OS_RPC_pf_Object_Call)(K2OS_RPC_OBJ aObject, UINT32 aObjContext, K2OS_RPC_OBJECT_CALL const *apCall, UINT32 *apRetUsedOutBytes);

typedef K2STAT(*K2OS_RPC_pf_Object_Delete)(K2OS_RPC_OBJ aObject, UINT32 aObjContext);

struct _K2OS_RPC_OBJECT_CLASSDEF
{
    K2_GUID128                  ClassId;

    K2OS_RPC_pf_Object_Create   Create;
    K2OS_RPC_pf_Object_Call     Call;
    K2OS_RPC_pf_Object_Delete   Delete;
};

//
//------------------------------------------------------------------------
//

#define K2OS_STORAGE_MEDIA_FRIENDLY_BUFFER_CHARS    32

typedef struct _K2OS_STORAGE_MEDIA K2OS_STORAGE_MEDIA;
struct _K2OS_STORAGE_MEDIA
{
    UINT64  mUniqueId;
    UINT32  mBlockCount;
    UINT32  mBlockSizeBytes;
    UINT64  mTotalBytes;
    char    mFriendly[K2OS_STORAGE_MEDIA_FRIENDLY_BUFFER_CHARS];
};

//
//------------------------------------------------------------------------
//

UINT32  K2OS_Debug_OutputString(char const *apStr);
void    K2OS_Debug_Break(void);

//
//------------------------------------------------------------------------
//

void K2_CALLCONV_REGS K2OS_RaiseException(K2STAT aExceptionCode);

//
//------------------------------------------------------------------------
//

void                K2OS_System_GetMsTick(UINT64 *apRetMsTick);
UINT32              K2OS_System_GetMsTick32(void);

UINT32              K2OS_System_GetHfFreq(void);
void                K2OS_System_GetHfTick(UINT64 *apRetHfTick);

void                K2OS_System_HfTickFromMsTick(UINT64 *apRetHfTick, UINT64 const *apMsTick);
void                K2OS_System_MsTickFromHfTick(UINT64 *apRetMsTick, UINT64 const *apHfTick);

void                K2OS_System_HfTickFromMsTick32(UINT64 *apRetHfTick, UINT32 aMsTick32);
UINT32              K2OS_System_MsTick32FromHfTick(UINT64 const *apHfTick);

K2OS_PROCESS_TOKEN  K2OS_System_CreateProcess(char const *apFilePath, char const *apArgs, UINT32 *apRetId);

//
//------------------------------------------------------------------------
//

UINT32  K2OS_Process_GetId(void);
void    K2OS_Process_Exit(UINT32 aExitCode);

//
//------------------------------------------------------------------------
//

K2OS_THREAD_TOKEN   K2OS_Thread_Create(char const *apName, K2OS_pf_THREAD_ENTRY aEntryPoint, void *apArg, K2OS_THREAD_CONFIG const *apConfig, UINT32 *apRetThreadId);
BOOL                K2OS_Thread_SetName(char const *apName);
void                K2OS_Thread_GetName(char *apRetNameBuffer);
UINT32              K2OS_Thread_GetId(void);
K2STAT              K2OS_Thread_GetLastStatus(void);
K2STAT              K2OS_Thread_SetLastStatus(K2STAT aStatus);
UINT32              K2OS_Thread_GetCpuCoreAffinityMask(void);
UINT32              K2OS_Thread_SetCpuCoreAffinityMask(UINT32 aNewAffinity);
void                K2OS_Thread_Exit(UINT32 aExitCode);
BOOL                K2OS_Thread_Sleep(UINT32 aTimeoutMs);
void                K2OS_Thread_StallMicroseconds(UINT32 aMicroseconds);
UINT32              K2OS_Thread_GetExitCode(K2OS_THREAD_TOKEN aThreadToken);
BOOL                K2OS_Thread_WaitOne(K2OS_WaitResult *apRetResult, K2OS_WAITABLE_TOKEN aToken, UINT32 aTimeoutMs);
BOOL                K2OS_Thread_WaitMany(K2OS_WaitResult *apRetResult, UINT32 aCount, K2OS_WAITABLE_TOKEN const *apWaitableTokens, BOOL aWaitAll, UINT32 aTimeoutMs);

//
//------------------------------------------------------------------------
//

BOOL K2OS_CritSec_Init(K2OS_CRITSEC *apSec);
BOOL K2OS_CritSec_TryEnter(K2OS_CRITSEC *apSec);
void K2OS_CritSec_Enter(K2OS_CRITSEC *apSec);
void K2OS_CritSec_Leave(K2OS_CRITSEC *apSec);
BOOL K2OS_CritSec_Done(K2OS_CRITSEC *apSec);

//
//------------------------------------------------------------------------
//

BOOL K2OS_Tls_AllocSlot(UINT32 *apRetNewIndex);
BOOL K2OS_Tls_FreeSlot(UINT32 aSlotIndex);
BOOL K2OS_Tls_SetValue(UINT32 aSlotIndex, UINT32 aValue);
BOOL K2OS_Tls_GetValue(UINT32 aSlotIndex, UINT32 *apRetValue);

//
//------------------------------------------------------------------------
//

void * K2OS_Heap_Alloc(UINT32 aByteCount);
BOOL   K2OS_Heap_Free(void *aPtr);

//
//------------------------------------------------------------------------
//

BOOL   K2OS_Token_Clone(K2OS_TOKEN aToken, K2OS_TOKEN *apRetNewToken);
BOOL   K2OS_Token_Destroy(K2OS_TOKEN aToken);

//
//------------------------------------------------------------------------
//

K2OS_NOTIFY_TOKEN K2OS_Notify_Create(BOOL aInitSignalled);
BOOL              K2OS_Notify_Signal(K2OS_NOTIFY_TOKEN aTokNotify);

//
//------------------------------------------------------------------------
//

K2OS_GATE_TOKEN K2OS_Gate_Create(BOOL aInitOpen);
BOOL            K2OS_Gate_Open(K2OS_GATE_TOKEN aTokGate);
BOOL            K2OS_Gate_Close(K2OS_GATE_TOKEN aTokGate);

//
//------------------------------------------------------------------------
//

K2OS_SEMAPHORE_TOKEN  K2OS_Semaphore_Create(UINT32 aMaxCount, UINT32 aInitCount);
BOOL                  K2OS_Semaphore_Inc(K2OS_SEMAPHORE_TOKEN aTokSemaphore, UINT32 aIncCount, UINT32 *apRetNewCount);

//
//------------------------------------------------------------------------
//

K2OS_PAGEARRAY_TOKEN  K2OS_PageArray_Create(UINT32 aPageCount);
UINT32                K2OS_PageArray_GetLength(K2OS_PAGEARRAY_TOKEN aTokPageArray);

//
//------------------------------------------------------------------------
//

UINT32  K2OS_Virt_Reserve(UINT32 aPageCount);
UINT32  K2OS_Virt_Get(UINT32 aVirtAddr, UINT32 *apRetPageCount);
BOOL    K2OS_Virt_Release(UINT32 aVirtAddr);

//
//------------------------------------------------------------------------
//

K2OS_VIRTMAP_TOKEN      K2OS_VirtMap_Create(K2OS_PAGEARRAY_TOKEN aTokPageArray, UINT32 aPageOffset, UINT32 aPageCount, UINT32 aVirtAddr, K2OS_VirtToPhys_MapType aMapType);
K2OS_PAGEARRAY_TOKEN    K2OS_VirtMap_AcquirePageArray(K2OS_VIRTMAP_TOKEN aTokVirtMap, UINT32 *apRetStartPageIndex);
K2OS_VIRTMAP_TOKEN      K2OS_VirtMap_Acquire(UINT32 aProcAddr, K2OS_VirtToPhys_MapType *apRetMapType, UINT32 *apRetMapPageIndex);
UINT32                  K2OS_VirtMap_GetInfo(K2OS_VIRTMAP_TOKEN aVirtMap, K2OS_VirtToPhys_MapType *apRetMapType, UINT32 *apRetMapPageCount);

//
//------------------------------------------------------------------------
//

K2OS_ALARM_TOKEN K2OS_Alarm_Create(UINT32 aPeriodMs, BOOL aIsPeriodic);

//
//------------------------------------------------------------------------
//

K2OS_XDL    K2OS_Xdl_Acquire(char const *apFilePath);
BOOL        K2OS_Xdl_AddRef(K2OS_XDL aXdl);
K2OS_XDL    K2OS_Xdl_AddRefContaining(UINT32 aAddr);
BOOL        K2OS_Xdl_Release(K2OS_XDL aXdl);
UINT32      K2OS_Xdl_FindExport(K2OS_XDL aXdl, BOOL aIsText, char const *apExportName);
BOOL        K2OS_Xdl_GetIdent(K2OS_XDL aXdl, char *apNameBuf, UINT32 aNameBufLen, K2_GUID128  *apRetID);

//
//------------------------------------------------------------------------
//

K2OS_MAILBOX_TOKEN  K2OS_Mailbox_Create(void);
BOOL                K2OS_Mailbox_Send(K2OS_MAILBOX_TOKEN aTokMailbox, K2OS_MSG const *apMsg);
BOOL                K2OS_Mailbox_Recv(K2OS_MAILBOX_TOKEN aTokMailbox, K2OS_MSG *apRetMsg, UINT32 aTimeoutMs);
UINT32              K2OS_Mailbox_Share(K2OS_MAILBOX_TOKEN aTokMailbox, UINT32 aProcessId);

//
//------------------------------------------------------------------------
//

K2OS_IFINST_TOKEN   K2OS_IfInst_Create(UINT32 aContext, K2OS_IFINST_ID *apRetId);
K2OS_IFINST_ID      K2OS_IfInst_GetId(K2OS_IFINST_TOKEN aTokIfInst);
BOOL                K2OS_IfInst_SetMailbox(K2OS_IFINST_TOKEN aTokIfInst, K2OS_MAILBOX_TOKEN aTokMailbox);
BOOL                K2OS_IfInst_GetContext(K2OS_IFINST_TOKEN aTokIfInst, UINT32 *apRetContext);
BOOL                K2OS_IfInst_SetContext(K2OS_IFINST_TOKEN aTokIfInst, UINT32 aContext);
BOOL                K2OS_IfInst_Publish(K2OS_IFINST_TOKEN aTokIfInst, UINT32 aClassCode, K2_GUID128 const* apSpecific);

BOOL                K2OS_IfInstId_GetDetail(K2OS_IFINST_ID aIfInstId, K2OS_IFINST_DETAIL *apDetail);

K2OS_IFENUM_TOKEN   K2OS_IfEnum_Create(BOOL aOneProc, UINT32 aProcessId, UINT32 aClassCode, K2_GUID128 const* apSpecific);
BOOL                K2OS_IfEnum_Reset(K2OS_IFENUM_TOKEN aIfEnumToken);
BOOL                K2OS_IfEnum_Next(K2OS_IFENUM_TOKEN aIfEnumToken, K2OS_IFINST_DETAIL *apEntryBuffer, UINT32* apIoEntryBufferCount);

#define K2OS_SYSTEM_MSG_IFINST_SHORT_ARRIVE     1
#define K2OS_SYSTEM_MSG_IFINST_SHORT_DEPART     2

K2OS_IFSUBS_TOKEN   K2OS_IfSubs_Create(K2OS_MAILBOX_TOKEN aMailbox, UINT32 aClassCode, K2_GUID128 const* apSpecific, UINT32 aBacklogCount, BOOL aProcSelfNotify, void* apContext);

//
//------------------------------------------------------------------------
//

#define K2OS_SYSTEM_MSG_IPCEND_SHORT_CREATED        1
#define K2OS_SYSTEM_MSG_IPCEND_SHORT_RECV           2 
#define K2OS_SYSTEM_MSG_IPCEND_SHORT_CONNECTED      3
#define K2OS_SYSTEM_MSG_IPCEND_SHORT_DISCONNECTED   4 
#define K2OS_SYSTEM_MSG_IPCEND_SHORT_REJECTED       5

K2OS_IPCEND K2OS_IpcEnd_Create(K2OS_MAILBOX_TOKEN aTokMailbox, UINT32 aMaxMsgCount, UINT32 aMaxMsgBytes, void *apContext, K2OS_IPCPROCESSMSG_CALLBACKS const *apFuncTab);
BOOL        K2OS_IpcEnd_GetParam(K2OS_IPCEND aEndpoint, UINT32 *apRetMaxMsgCount, UINT32 *apRetMaxMsgBytes, void **appRetContext);
BOOL        K2OS_IpcEnd_SendRequest(K2OS_IPCEND aEndpoint, K2OS_IFINST_ID aIfInstId);
BOOL        K2OS_IpcEnd_AcceptRequest(K2OS_IPCEND aEndpoint, UINT32 aRequestId);
BOOL        K2OS_IpcEnd_Send(K2OS_IPCEND aEndpoint, void const *apBuffer, UINT32 aByteCount);
BOOL        K2OS_IpcEnd_SendVector(K2OS_IPCEND aEndpoint, UINT32 aVectorCount, K2MEM_BUFVECTOR const *apVectors);
BOOL        K2OS_IpcEnd_Disconnect(K2OS_IPCEND aEndpoint);
BOOL        K2OS_IpcEnd_Delete(K2OS_IPCEND aEndpoint);
BOOL        K2OS_IpcEnd_ProcessMsg(K2OS_MSG const *apMsg);

#define K2OS_SYSTEM_MSG_IPC_SHORT_REQUEST           1

BOOL        K2OS_Ipc_RejectRequest(UINT32 aRequestId, UINT32 aReasonCode);

//
//------------------------------------------------------------------------
//

#define K2OS_SYSTEM_MSG_RPC_SHORT_NOTIFY            1

K2OS_IFINST_ID  K2OS_RpcServer_GetIfInstId(void);
K2OS_RPC_CLASS  K2OS_RpcServer_Register(K2OS_RPC_OBJECT_CLASSDEF const *apClassDef, UINT32 aContext);
BOOL            K2OS_RpcServer_Deregister(K2OS_RPC_CLASS aRegisteredClass);

BOOL            K2OS_RpcObj_GetContext(K2OS_RPC_OBJ aObject, UINT32 *apRetContext);
BOOL            K2OS_RpcObj_SendNotify(K2OS_RPC_OBJ aObject, UINT32 aNotifyCode);
K2OS_RPC_IFINST K2OS_RpcObj_PublishIfInst(K2OS_RPC_OBJ aObject, UINT32 aClassCode, K2_GUID128 const* apSpecific, K2OS_IFINST_ID *apRetId);
BOOL            K2OS_RpcObj_RemoveIfInst(K2OS_RPC_OBJ aObject, K2OS_RPC_IFINST aIfInst);

K2OS_RPC_OBJ_HANDLE K2OS_Rpc_CreateObj(K2OS_IFINST_ID aRpcServerIfInstId, K2_GUID128 const *apClassId, UINT32 aCreatorContext);
K2OS_RPC_OBJ_HANDLE K2OS_Rpc_AttachByObjId(K2OS_IFINST_ID aRpcServerIfInstId, UINT32 aObjId);
K2OS_RPC_OBJ_HANDLE K2OS_Rpc_AttachByIfInstId(K2OS_IFINST_ID aIfInstId, UINT32 *apRetObjId);
BOOL                K2OS_Rpc_GetObjId(K2OS_RPC_OBJ_HANDLE aObjHandle, UINT32 *apRetObjId);
BOOL                K2OS_Rpc_SetNotifyTarget(K2OS_RPC_OBJ_HANDLE aObjHandle, K2OS_MAILBOX_TOKEN aTokMailslot);
K2STAT              K2OS_Rpc_Call(K2OS_RPC_OBJ_HANDLE aObjHandle, K2OS_RPC_CALLARGS const *apCallArgs, UINT32 *apRetActualOutBytes);
BOOL                K2OS_Rpc_Release(K2OS_RPC_OBJ_HANDLE aObjHandle);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __K2OS_H