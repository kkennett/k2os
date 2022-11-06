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
#include "crt32.h"

UINT32                          gProcessId;
void *                          __dso_handle;
UINT32                          gTimerAddr;
UINT32                          gTimerFreq;

extern XDL_ELF_ANCHOR const * const gpXdlAnchor;
extern void *                   __data_end;

int  __cxa_atexit(__vfpv f, void *a, XDL *apXdl);
void __call_dtors(XDL *apXdl);

#if K2_TARGET_ARCH_IS_ARM
#if K2_TARGET_ARCH_IS_32BIT

int __aeabi_atexit(void *object, __vfpv destroyer, void *dso_handle)
{
    return __cxa_atexit(destroyer, object, (XDL *)dso_handle);
}

#endif
#endif

XDL *
XDL_GetModule(
    void
)
{
    return (XDL *)__dso_handle;
}

void
K2_CALLCONV_REGS
__k2oscrt_user_entry(
    K2ROFS const *  apROFS,
    UINT32          aProcessId
    )
{
    //
    // this will never execute as aProcessId will never equal 0xFEEDF00D
    // it is here to pull in exports and must be 'reachable' code.
    // the *ADDRESS OF* gpXdlInfo is even invalid and trying to use
    // it in executing code will cause a fault.
    //
    if (aProcessId == 0xFEEDF00D)
        __dso_handle = (void *)(*((UINT32 *)gpXdlAnchor));

    //
    // baseline inits
    //
    gProcessId = aProcessId;
    gTimerAddr = *((UINT32 const *)K2OS_UVA_PUBLICAPI_TIMER_ADDR);
    gTimerFreq = *((UINT32 const *)K2OS_UVA_PUBLICAPI_TIMER_FREQ);
    K2OS_Thread_SetLastStatus(K2STAT_NO_ERROR);
    CrtAtExit_Init();

    //
    // init dynamic loader and self-init this module
    //
    CrtXdl_Init(apROFS);

    //
    // init process memory so heap allocations will work
    //
    CrtMem_Init();

    //
    // now init any other tracking
    //
    CrtMailbox_Init();
    CrtIpcEnd_Init();
    CrtRootIf_Init();

    //
    // launch this process
    //
    CrtLaunch();
}

