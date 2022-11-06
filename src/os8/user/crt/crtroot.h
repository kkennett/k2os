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
#ifndef __CRTROOT_H

#include <k2os.h>
#include <lib/k2ring.h>
#include "..\..\kern\main\bits32\kerniface.h"

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

K2OS_PROCESS_TOKEN   K2_CALLCONV_REGS K2OS_Root_Launch(char const *apPath, char const *apArgs, UINT_PTR *apRetId);
BOOL                 K2_CALLCONV_REGS K2OS_Root_StopProcess(K2OS_PROCESS_TOKEN aProcessToken, UINT_PTR aExitCode, BOOL aWait);
UINT_PTR             K2_CALLCONV_REGS K2OS_Root_GetProcessExitCode(K2OS_PROCESS_TOKEN aProcessToken);

BOOL                                  K2OS_Root_AddIoRange(K2OS_PROCESS_TOKEN aTokProcess, UINT32 aIoBase, UINT32 aPortCount);

K2OS_PAGEARRAY_TOKEN K2_CALLCONV_REGS K2OS_Root_CreatePageArrayAt(UINT32 aPhysAddr, UINT32 aPageCount);

UINT_PTR             K2_CALLCONV_REGS K2OS_Root_CreateTokenForProcess(K2OS_PROCESS_TOKEN aTokProcess, K2OS_TOKEN aToken);
K2OS_TOKEN           K2_CALLCONV_REGS K2OS_Root_CreateTokenForRoot(K2OS_PROCESS_TOKEN aTokProcess, UINT_PTR aProcessTokenValue);

UINT_PTR                              K2OS_Root_CreatePlatNode(UINT_PTR aParentPlatContext, UINT_PTR aNodeName, UINT_PTR aRootContext, char *apIoBuf, UINT_PTR aBufSizeBytes, UINT_PTR *apIoBufBytes);
BOOL                 K2_CALLCONV_REGS K2OS_Root_DeletePlatNode(UINT_PTR aPlatContext);
UINT_PTR                              K2OS_Root_AddPlatResource(UINT_PTR aPlatContext, UINT_PTR aResType, UINT_PTR aResInfo1, UINT_PTR aResInfo2);

K2OS_INTERRUPT_TOKEN K2_CALLCONV_REGS K2OS_Root_HookPlatIntr(UINT_PTR aPlatContext, UINT_PTR aIrqIndex);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __CRTROOT_H
