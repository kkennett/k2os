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
#ifndef __K2RWLOCK_H
#define __K2RWLOCK_H

#include <k2systype.h>
#include <lib/k2list.h>

//
//------------------------------------------------------------------------
//

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _K2RWLOCK_WAIT       K2RWLOCK_WAIT;
typedef enum   _K2RWLOCK_HolderType K2RWLOCK_HolderType;
typedef struct _K2RWLOCK_OPS        K2RWLOCK_OPS;
typedef struct _K2RWLOCK            K2RWLOCK;

struct _K2RWLOCK_WAIT
{
    UINT_PTR volatile   mWaitingThreadCount;
    BOOL                mIsWriter;
    UINT_PTR            mGateOpaque;
    K2LIST_LINK         ListLink;
};

enum _K2RWLOCK_HolderType
{
    K2RWLock_Holder_Nobody = 0,
    K2RWLock_Holder_Reader,
    K2RWLock_Holder_Writer,
    K2RWLock_HolderType_Count
};

typedef UINT_PTR        (*K2RWLOCK_pf_StartAtomic)(K2RWLOCK *apRwLock);
typedef void            (*K2RWLOCK_pf_EndAtomic)(K2RWLOCK *apRwLock, UINT_PTR aStartResult);
typedef K2RWLOCK_WAIT * (*K2RWLOCK_pf_GetWait)(K2RWLOCK *apRwLock);
typedef void            (*K2RWLOCK_pf_PutWait)(K2RWLOCK *apRwLock, K2RWLOCK_WAIT *apWait);
typedef void            (*K2RWLOCK_pf_WaitForGate)(UINT_PTR aGateOpaque);
typedef void            (*K2RWLOCK_pf_SetGate)(UINT_PTR aGateOpaque, BOOL aSetOpen);

struct _K2RWLOCK_OPS
{
    K2RWLOCK_pf_StartAtomic StartAtomic;
    K2RWLOCK_pf_EndAtomic   EndAtomic;
    K2RWLOCK_pf_GetWait     GetWait;
    K2RWLOCK_pf_PutWait     PutWait;
    K2RWLOCK_pf_WaitForGate WaitForGate;
    K2RWLOCK_pf_SetGate     SetGate;
};

struct _K2RWLOCK
{
    K2RWLOCK_OPS            Ops;
    K2RWLOCK_HolderType     mHolder;
    UINT_PTR                mHeldCount;
    K2LIST_ANCHOR           WaitList;
};

void K2RWLOCK_Init(K2RWLOCK *apRwLock, K2RWLOCK_OPS const *apOps);
void K2RWLOCK_Done(K2RWLOCK *apRwLock);

void K2RWLOCK_ReadLock(K2RWLOCK *apLock);
void K2RWLOCK_UpgradeReadToWrite(K2RWLOCK *apLock);
void K2RWLOCK_ReadUnlock(K2RWLOCK *apLock);

void K2RWLOCK_WriteLock(K2RWLOCK *apLock);
void K2RWLOCK_WriteUnlock(K2RWLOCK *apLock);

#ifdef __cplusplus
};  // extern "C"
#endif

//
//------------------------------------------------------------------------
//

#endif  // __K2RWLOCK_H

