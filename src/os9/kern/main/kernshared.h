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

#ifndef __KERNSHARED_H
#define __KERNSHARED_H

#include <kern/k2osplat.h>
#include <lib/k2xdl.h>

/* --------------------------------------------------------------------------------- */

typedef void (*CrtKern_pf_ThreadedPostInit)(void);
typedef void (K2_CALLCONV_REGS *CrtKern_pf_Exec)(void);

typedef void *  (*K2OS_pf_HeapAlloc)(UINT32 aByteCount);
typedef BOOL    (*K2OS_pf_HeapFree)(void *aPtr);

typedef BOOL    (*K2OS_pf_CritSecInit)(K2OS_CRITSEC *apSec);
typedef void    (*K2OS_pf_CritSecEnter)(K2OS_CRITSEC *apSec);
typedef void    (*K2OS_pf_CritSecLeave)(K2OS_CRITSEC *apSec);

typedef struct _K2OSKERN_FUNCTAB K2OSKERN_FUNCTAB;
struct _K2OSKERN_FUNCTAB
{
    CrtKern_pf_ThreadedPostInit CoreThreadedPostInit;
    CrtKern_pf_Exec             Exec;

    K2OSKERN_pf_Debug           Debug;
    K2OSKERN_pf_Panic           Panic;
    K2OSKERN_pf_MicroStall      MicroStall;
    K2OSKERN_pf_SetIntr         SetIntr;
    K2OSKERN_pf_GetIntr         GetIntr;
    K2OSKERN_pf_SeqInit         SeqInit;
    K2OSKERN_pf_SeqLock         SeqLock;
    K2OSKERN_pf_SeqUnlock       SeqUnlock;
    K2OSKERN_pf_GetCpuIndex     GetCpuIndex;

    K2OS_pf_HeapAlloc           HeapAlloc;
    K2OS_pf_HeapFree            HeapFree;

    K2OSPLAT_pf_DebugOut        DebugOut;
    K2OSPLAT_pf_DebugIn         DebugIn;

    K2OS_pf_CritSecInit         CritSecInit;
    K2OS_pf_CritSecEnter        CritSecEnter;
    K2OS_pf_CritSecLeave        CritSecLeave;

    K2_pf_ASSERT                Assert;
    K2_pf_EXTRAP_MOUNT          ExTrap_Mount;
    K2_pf_EXTRAP_DISMOUNT       ExTrap_Dismount;
    K2_pf_RAISE_EXCEPTION       RaiseException;
};

typedef struct _K2OSKERN_SHARED K2OSKERN_SHARED;
struct _K2OSKERN_SHARED
{
    K2OS_UEFI_LOADINFO      LoadInfo;
    K2_CACHEINFO const *    mpCacheInfo;
    K2OSKERN_FUNCTAB        FuncTab;
};

/* --------------------------------------------------------------------------------- */


#endif // __KERNSHARED_H