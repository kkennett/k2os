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
#ifndef __CRTUSER_H
#define __CRTUSER_H

#include <k2os.h>

#include "crtroot.h"

#include <lib/k2xdl.h>
#include <lib/k2elf.h>
#include <lib/k2bit.h>
#include <lib/k2rofshelp.h>
#include <lib/k2heap.h>
#include <lib/k2ramheap.h>

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
K2STAT  CrtXdl_Acquire(char const *apFileSpec, XDL ** appRetXdl, UINT_PTR * apRetEntryStackReq, K2_GUID128 * apRetID);

void    CrtHeap_Init(void);

UINT_PTR    CrtDbg_Printf(char const *apFormat, ...);

void    CrtLaunch(void);

void        CrtMem_Init(void);
void        CrtMem_Touch(void *apData, UINT_PTR aBytes);
UINT_PTR    CrtMem_CreateRwSegment(UINT_PTR aPageCount);
UINT_PTR    CrtMem_CreateStackSegment(UINT_PTR aPageCount);
BOOL        CrtMem_RemapSegment(UINT_PTR aSegAddr, K2OS_User_MapType aNewMapType);
BOOL        CrtMem_FreeSegment(UINT_PTR aSegAddr, BOOL aNoVirtFree);

void K2_CALLCONV_REGS CrtThread_EntryPoint(K2OS_pf_THREAD_ENTRY aUserEntry, void *apArgument);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __CRTUSER_H