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
#ifndef __FATFS_H
#define __FATFS_H

#include <kern/k2osddk.h>
#include <k2osstor.h>
#include <lib/k2fat.h>
//#include <lib/k2rundown.h>

/* ------------------------------------------------------------------------- */

// {80BD0D97-725C-4620-AD86-315DFB7017D1}
#define FATFS_OBJ_CLASS_GUID  { 0x80bd0d97, 0x725c, 0x4620, { 0xad, 0x86, 0x31, 0x5d, 0xfb, 0x70, 0x17, 0xd1 } }

typedef struct _FATFS FATFS;
struct _FATFS
{
    K2OS_CRITSEC        Sec;
    UINT32              mVolInstId;
    K2OSSTOR_VOLUME     mVol;
    K2OS_STORAGE_VOLUME VolInfo;
    K2FAT_PART          FatPart;
    UINT32              mThreadId;
    K2OS_SIGNAL_TOKEN   mTokStart;
    K2STAT              mLastStatus;
    K2OS_MAILBOX_TOKEN  mTokMailbox;
    K2OS_THREAD_TOKEN   mTokThread;
    K2LIST_LINK         ListLink;
    K2OS_RPC_OBJ_HANDLE mhRpcObj;
    K2OS_RPC_OBJ        mRpcObj;
};

extern K2OS_CRITSEC     gFsSec;
extern K2LIST_ANCHOR    gFsList;

K2STAT FATFS_RpcObj_Create(K2OS_RPC_OBJ aObject, K2OS_RPC_OBJ_CREATE const *apCreate, UINT32 *apRetContext);
K2STAT FATFS_RpcObj_OnAttach(K2OS_RPC_OBJ aObject, UINT32 aObjContext, UINT32 aProcessId, UINT32 *apRetUseContext);
K2STAT FATFS_RpcObj_OnDetach(K2OS_RPC_OBJ aObject, UINT32 aObjContext, UINT32 aUseContext);
K2STAT FATFS_RpcObj_Call(K2OS_RPC_OBJ_CALL const *apCall, UINT32 *apRetUsedOutBytes);
K2STAT FATFS_RpcObj_Delete(K2OS_RPC_OBJ aObject, UINT32 aObjContext);

/* ------------------------------------------------------------------------- */

#endif // __FATFS_H