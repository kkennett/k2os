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
#ifndef __CRT32_H
#define __CRT32_H

#include "../crtuser.h"

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

#if K2_TARGET_ARCH_IS_ARM
#define CRT_GET_CURRENT_THREAD_INDEX A32_ReadTPIDRURO()

typedef UINT32 (K2_CALLCONV_REGS *K2OS_pf_SysCall)(UINT32 aId, UINT32 aArg1, UINT32 aArg2, UINT32 aArg3);
#define K2OS_SYSCALL ((K2OS_pf_SysCall)(K2OS_UVA_PUBLICAPI_SYSCALL))

#define CrtKern_SysCall1(x,y)     K2OS_SYSCALL((x),(y))
#define CrtKern_SysCall2(x,y,z)   K2OS_SYSCALL((x),(y),(z))
#define CrtKern_SysCall3(x,y,z,w) K2OS_SYSCALL((x),(y),(z),(w))

#elif K2_TARGET_ARCH_IS_INTEL

UINT32 K2_CALLCONV_REGS X32_GetThreadIndex(void);
#define CRT_GET_CURRENT_THREAD_INDEX X32_GetThreadIndex()

typedef UINT32 (K2_CALLCONV_REGS *K2OS_pf_SysCall)(UINT_PTR aId, UINT_PTR aArg0);
#define K2OS_SYSCALL ((K2OS_pf_SysCall)(K2OS_UVA_PUBLICAPI_SYSCALL))

#define CrtKern_SysCall1(x,y) K2OS_SYSCALL((x),(y))

#else
#error !!!Unsupported Architecture
#endif

UINT_PTR CrtKern_SysCall2(UINT_PTR aId, UINT_PTR aArg0, UINT_PTR aArg1);
UINT_PTR CrtKern_SysCall3(UINT_PTR aId, UINT_PTR aArg0, UINT_PTR aArg1, UINT_PTR aArg2);
UINT_PTR CrtKern_SysCall4(UINT_PTR aId, UINT_PTR aArg0, UINT_PTR aArg1, UINT_PTR aArg2, UINT_PTR aArg3);
UINT_PTR CrtKern_SysCall5(UINT_PTR aId, UINT_PTR aArg0, UINT_PTR aArg1, UINT_PTR aArg2, UINT_PTR aArg3, UINT_PTR aArg4);
UINT_PTR CrtKern_SysCall6(UINT_PTR aId, UINT_PTR aArg0, UINT_PTR aArg1, UINT_PTR aArg2, UINT_PTR aArg3, UINT_PTR aArg4, UINT_PTR aArg5);
UINT_PTR CrtKern_SysCall7(UINT_PTR aId, UINT_PTR aArg0, UINT_PTR aArg1, UINT_PTR aArg2, UINT_PTR aArg3, UINT_PTR aArg4, UINT_PTR aArg5, UINT_PTR aArg6);
UINT_PTR CrtKern_SysCall8(UINT_PTR aId, UINT_PTR aArg0, UINT_PTR aArg1, UINT_PTR aArg2, UINT_PTR aArg3, UINT_PTR aArg4, UINT_PTR aArg5, UINT_PTR aArg6, UINT_PTR aArg7);

//
//------------------------------------------------------------------------
//

#define CRT_MBOXREF_SENTINEL_1  K2_MAKEID4('m','B','o','x')
#define CRT_MBOXREF_SENTINEL_2  K2_MAKEID4('X','O','b','m')

typedef struct _CRT_MBOXREF CRT_MBOXREF;
struct _CRT_MBOXREF
{
    UINT32                  mSentinel1;
    INT32 volatile          mUseCount;
    BOOL                    mUserDeleted;
    K2OS_MAILBOX_OWNER *    mpMailboxOwner;
    K2TREE_NODE             TreeNode;
    UINT32                  mSentinel2;
};

void                    CrtMailbox_Init(void);
CRT_MBOXREF *           CrtMailbox_CreateRef(CRT_MBOXREF *apMailboxRef);
K2OS_MAILBOX_OWNER *    CrtMailboxRef_FindAddUse(K2OS_MAILBOX aMailbox);
BOOL                    CrtMailboxRef_DecUse(K2OS_MAILBOX aMailbox, K2OS_MAILBOX_OWNER *apMailboxOwner, BOOL aIsUserDelete);

//
//------------------------------------------------------------------------
//

typedef void (*CRT_pf_SysMsgRecv)(void *apKey, UINT32 aMsgType, UINT32 aArg0, UINT32 aArg1);

typedef struct _CRT_USER_IPCEND CRT_USER_IPCEND;
struct _CRT_USER_IPCEND
{
    INT32 volatile              mRefs;

    K2OS_TOKEN                  mIpcEndToken;
    K2OS_MAILBOX                mLocalMailbox;
    K2OS_MAP_TOKEN              mRecvMapToken;
    K2RING *                    mpRecvRing;

    BOOL                        mUserDeleted;

    BOOL                        mRequesting;        // StartConnect has been called 
    UINT_PTR                    mRequestIfInstId;   // only valid while 'mRequesting' is true

    BOOL                        mConnected;         // true between recv connect msg and issue of ack disconnect
    K2OS_MAP_TOKEN              mSendMapToken;      // valid between recv connect msg and issue of ack disconnect
    K2RING *                    mpSendRing;         // valid between recv connect msg and issue of ack disconnect
    UINT32                      mRemoteMaxMsgCount; // valid between recv connect msg and issue of ack disconnect
    UINT32                      mRemoteMaxMsgBytes; // valid between recv connect msg and issue of ack disconnect
    UINT32                      mRemoteChunkBytes;

    K2OS_IPCEND_FUNCTAB         FuncTab;

    void *                      mpContext;
    UINT32                      mMaxMsgCount;
    UINT32                      mMaxMsgBytes;
    UINT32                      mLocalChunkBytes;

    CRT_pf_SysMsgRecv           mfRecv;

    K2OS_CRITSEC                Sec;

    K2TREE_NODE                 TreeNode;
};

void              CrtIpcEnd_Init(void);
CRT_USER_IPCEND * CrtIpcEnd_FindAddRef(K2OS_IPCEND aEndpoint);
BOOL              CrtIpcEnd_Release(K2OS_IPCEND aEndpoint, BOOL aIsUserDelete);

//
//------------------------------------------------------------------------
//

void    CrtRootIf_Init(void);
K2STAT  CrtRootIf_Call(UINT_PTR aMethodId, UINT8 const * apInBuf, UINT8 *apOutBuf, UINT_PTR aInBufByteCount, UINT_PTR aOutBufByteCount, UINT_PTR *apRetUsedOutByteCount);

//
//------------------------------------------------------------------------
//

extern UINT32                           gProcessId;
extern CRT_LAUNCH_INFO const * const    gpLaunchInfo;
extern UINT32                           gTimerAddr;
extern UINT32                           gTimerFreq;

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __CRT32_H