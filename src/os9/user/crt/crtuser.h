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
#ifndef __CRTUSER_H
#define __CRTUSER_H

#include <k2os.h>
#include <k2osdev.h>
#include <k2osdev_blockio.h>
#include <k2osstor.h>

#include "..\..\kern\main\kerniface.h"

#include <lib/k2xdl.h>
#include <lib/k2elf.h>
#include <lib/k2bit.h>
#include <lib/k2rofshelp.h>
#include <lib/k2heap.h>
#include <lib/k2ramheap.h>
#include <lib/k2ring.h>

#if K2_TARGET_ARCH_IS_INTEL
#include <lib/k2archx32.h>
#endif
#if K2_TARGET_ARCH_IS_ARM
#include <lib/k2archa32.h>
#endif

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

typedef void(*__vfpv)(void *);

void    CrtAtExit_Init(void);

void    CrtXdl_Init(K2ROFS const * apROFS);
K2STAT  CrtXdl_Acquire(char const *apFileSpec, XDL ** appRetXdl, UINT32 * apRetEntryStackReq, K2_GUID128 * apRetID);

void    CrtHeap_Init(void);

UINT32  CrtDbg_Printf(char const *apFormat, ...);

void    CrtLaunch(void);

void    CrtMem_Init(void);
UINT32  CrtMem_CreateRwSegment(UINT32 aPageCount);
UINT32  CrtMem_CreateStackSegment(UINT32 aPageCount);
BOOL    CrtMem_RemapSegment(UINT32 aSegAddr, K2OS_VirtToPhys_MapType aNewMapType);
BOOL    CrtMem_FreeSegment(UINT32 aSegAddr, BOOL aNoVirtFree);

void K2_CALLCONV_REGS CrtThread_EntryPoint(K2OS_pf_THREAD_ENTRY aUserEntry, void *apArgument);

void    CrtMail_Init(void);
BOOL    CrtMail_TokenDestroy(K2OS_TOKEN aToken);
BOOL    CrtMail_Cloned(K2OS_TOKEN aTokOriginal, K2OS_TOKEN aTokClone);

void                CrtRpc_Init(void);
K2OS_RPC_OBJ_HANDLE CrtRpc_Obj(void);

//
//------------------------------------------------------------------------
//

typedef void (*CRT_pf_SysMsgRecv)(void* apKey, K2OS_MSG const *apMsg);

typedef struct _CRT_USER_IPCEND CRT_USER_IPCEND;
struct _CRT_USER_IPCEND
{
    INT32 volatile                  mRefs;

    K2OS_IPCEND_TOKEN               mIpcEndToken;
    K2OS_MAILBOX_TOKEN              mTokMailbox;
    K2OS_VIRTMAP_TOKEN              mTokRecvVirtMap;
    K2RING *                        mpRecvRing;

    BOOL                            mUserDeleted;

    BOOL                            mRequesting;        // StartConnect has been called 
    K2OS_IFINST_ID                  mRequestIfInstId;   // only valid while 'mRequesting' is true

    BOOL                            mConnected;         // true between recv connect msg and issue of ack disconnect
    K2OS_VIRTMAP_TOKEN              mTokSendVirtMap;    // valid between recv connect msg and issue of ack disconnect
    K2RING *                        mpSendRing;         // valid between recv connect msg and issue of ack disconnect
    UINT32                          mRemoteMaxMsgCount; // valid between recv connect msg and issue of ack disconnect
    UINT32                          mRemoteMaxMsgBytes; // valid between recv connect msg and issue of ack disconnect
    UINT32                          mRemoteChunkBytes;

    K2OS_IPCPROCESSMSG_CALLBACKS    Callbacks;

    void *                          mpContext;
    UINT32                          mMaxMsgCount;
    UINT32                          mMaxMsgBytes;
    UINT32                          mLocalChunkBytes;

    CRT_pf_SysMsgRecv               mfRecv;

    K2OS_CRITSEC                    Sec;

    K2TREE_NODE                     TreeNode;
};

void                CrtIpcEnd_Init(void);
CRT_USER_IPCEND *   CrtIpcEnd_FindAddRef(K2OS_IPCEND aEndpoint);
BOOL                CrtIpcEnd_Release(K2OS_IPCEND aEndpoint, BOOL aIsUserDelete);

//
//------------------------------------------------------------------------
//

#if K2_TARGET_ARCH_IS_ARM
#define CRT_GET_CURRENT_THREAD_INDEX A32_ReadTPIDRURO()
#elif K2_TARGET_ARCH_IS_INTEL
UINT32 K2_CALLCONV_REGS X32Asm_GetThreadIndex(void);
#define CRT_GET_CURRENT_THREAD_INDEX X32Asm_GetThreadIndex()
#else
#error !!!Unsupported Architecture
#endif

#define CrtKern_SysCall1(x,y) K2OS_Kern_SysCall1(x,y)
UINT32 CrtKern_SysCall2(UINT32 aId, UINT32 aArg0, UINT32 aArg1);
UINT32 CrtKern_SysCall3(UINT32 aId, UINT32 aArg0, UINT32 aArg1, UINT32 aArg2);
UINT32 CrtKern_SysCall4(UINT32 aId, UINT32 aArg0, UINT32 aArg1, UINT32 aArg2, UINT32 aArg3);
UINT32 CrtKern_SysCall5(UINT32 aId, UINT32 aArg0, UINT32 aArg1, UINT32 aArg2, UINT32 aArg3, UINT32 aArg4);
UINT32 CrtKern_SysCall6(UINT32 aId, UINT32 aArg0, UINT32 aArg1, UINT32 aArg2, UINT32 aArg3, UINT32 aArg4, UINT32 aArg5);
UINT32 CrtKern_SysCall7(UINT32 aId, UINT32 aArg0, UINT32 aArg1, UINT32 aArg2, UINT32 aArg3, UINT32 aArg4, UINT32 aArg5, UINT32 aArg6);
UINT32 CrtKern_SysCall8(UINT32 aId, UINT32 aArg0, UINT32 aArg1, UINT32 aArg2, UINT32 aArg3, UINT32 aArg4, UINT32 aArg5, UINT32 aArg6, UINT32 aArg7);

//
//------------------------------------------------------------------------
//

extern UINT32                               gProcessId;
extern K2OS_PROC_START_INFO const * const   gpLaunchInfo;
extern UINT32                               gTimerAddr;
extern UINT32                               gTimerFreq;

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __CRTUSER_H
