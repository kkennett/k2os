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

#ifndef __K2OSRWLOCK_H
#define __K2OSRWLOCK_H

#include <k2os.h>
#include <lib/k2rwlock.h>

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

#define K2OS_RWLOCK_SLOT_COUNT 8

struct _K2OS_RWLOCK
{
    K2RWLOCK            RwLock;
    K2OS_CRITSEC        Sec;
    UINT8               mSlotsMem[K2OS_CACHELINE_BYTES * (K2OS_RWLOCK_SLOT_COUNT + 1)];
};
typedef struct _K2OS_RWLOCK K2OS_RWLOCK;

void K2OS_RWLOCK_Init(K2OS_RWLOCK *apLock);
void K2OS_RWLOCK_Done(K2OS_RWLOCK *apLock);
void K2OS_RWLOCK_ReadLock(K2OS_RWLOCK *apLock);
void K2OS_RWLOCK_ReadUnlock(K2OS_RWLOCK *apLock);
void K2OS_RWLOCK_UpgradeReadLockToWriteLock(K2OS_RWLOCK *apLock);
void K2OS_RWLOCK_WriteLock(K2OS_RWLOCK *apLock);
void K2OS_RWLOCK_WriteUnlock(K2OS_RWLOCK *apLock);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __K2OSRWLOCK_H
