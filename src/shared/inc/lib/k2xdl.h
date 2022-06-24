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
#ifndef __K2XDL_H
#define __K2XDL_H

#include <k2systype.h>

#include <lib/k2list.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------------- - */

#ifndef XDL_ENTRY_REASON_UNLOAD
#define XDL_ENTRY_REASON_UNLOAD ((UINT_PTR)-1)
#endif

#ifndef XDL_ENTRY_REASON_LOAD
#define XDL_ENTRY_REASON_LOAD   1
#endif

typedef K2STAT (K2_CALLCONV_REGS *XDL_pf_ENTRYPOINT)(XDL * apXdl, UINT_PTR aReason);

K2STAT
K2_CALLCONV_REGS
xdl_entry(
    XDL *       apXdl,
    UINT_PTR    aReason
);

/* -------------------------------------------------------------------------------- - */

K2STAT
XDL_Acquire(
    char const *    apFileSpec,
    void *          apContext,
    XDL **          appRetXdl,
    UINT_PTR *      apRetEntryStackReq,
    K2_GUID128 *    apRetID
);

K2STAT
XDL_GetIdent(
    XDL *           apXdl,
    char *          apNameBuf,
    UINT_PTR        aNameBufLen,
    K2_GUID128  *   apRetID
);

K2STAT
XDL_Release(
    XDL *           apXdl
);

K2STAT
XDL_FindExport(
    XDL *           apXdl,
    XDLExportType   aType,
    char const *    apName,
    UINT_PTR *      apRetAddr
);

K2STAT
XDL_AcquireContaining(
    UINT_PTR    aAddr,
    XDL **      appRetXdl,
    UINT_PTR *  apRetSegment
);

K2STAT
XDL_FindAddrName(
    UINT_PTR    aAddr,
    char *      apRetNameBuffer,
    UINT_PTR    aBufferLen
);

UINT_PTR
XDL_AddrToName(
    XDL *       apXdl,
    UINT_PTR    aAddr,
    UINT_PTR    aSegHint,
    char *      apRetNameBuffer,
    UINT_PTR    aBufferLen
);

/* -------------------------------------------------------------------------------- - */

typedef UINT_PTR K2XDL_HOST_FILE;



/* -------------------------------------------------------------------------------- - */

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // __K2XDL_H

