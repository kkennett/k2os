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

typedef UINT_PTR K2XDL_HOST_FILE;

typedef struct _K2XDL_LOADCTX K2XDL_LOADCTX;
struct _K2XDL_LOADCTX
{
    UINT_PTR        mAcqContext;
    char const *    mpFilePath;
    char const *    mpNamePart;
    UINT_PTR        mNameLen;
};

typedef struct _K2XDL_OPENRESULT K2XDL_OPENRESULT;
struct _K2XDL_OPENRESULT
{
    K2XDL_HOST_FILE mHostFile;
    UINT_PTR        mFileSectorCount;
    UINT64          mXdlPage_DataAddr;
    UINT64          mXdlPage_LinkAddr;
};

typedef struct _K2XDL_SECTION_ADDRS K2XDL_SECTION_ADDRS;
struct _K2XDL_SECTION_ADDRS
{
    UINT_PTR        mDataAddr[XDLSectionIx_Count];
};

typedef K2STAT (*K2XDL_pf_CritSec)(BOOL aEnter);
typedef K2STAT (*K2XDL_pf_Open)(K2XDL_LOADCTX const *apLoadCtx, K2XDL_OPENRESULT *apRetResult);
typedef K2STAT (*K2XDL_pf_ReadSectors)(K2XDL_LOADCTX const *apLoadCtx, K2XDL_HOST_FILE aHostFile, void *apBuffer, UINT_PTR aSectorCount);
typedef K2STAT (*K2XDL_pf_Prepare)(K2XDL_LOADCTX const *apLoadCtx, K2XDL_HOST_FILE aHostFile, XDL_FILE_HEADER const *apFileHdr, BOOL aKeepSymbols, K2XDL_SECTION_ADDRS *apRetSectionDataAddrs);
typedef K2STAT (*K2XDL_pf_Finalize)(K2XDL_LOADCTX const *apLoadCtx, K2XDL_HOST_FILE aHostFile, K2XDL_SECTION_ADDRS *apRetSectionDataAddrs);
typedef K2STAT (*K2XDL_pf_Purge)(K2XDL_HOST_FILE aHostFile);

typedef struct _K2XDL_HOST K2XDL_HOST;
struct _K2XDL_HOST
{
    K2XDL_pf_CritSec        CritSec;
    K2XDL_pf_Open           Open;
    K2XDL_pf_ReadSectors    ReadSectors;
    K2XDL_pf_Prepare        Prepare;
    K2XDL_pf_Finalize       Finalize;
    K2XDL_pf_Purge          Purge;
};

typedef BOOL (*K2XDL_pf_ConvertLoadPtr)(UINT_PTR * apAddr);

K2STAT K2XDL_Init(void * apXdlControlPage, K2XDL_HOST *apHost, BOOL aKeepSym, BOOL aReInit);

K2STAT K2XDL_Handoff(XDL ** appEntryXdl, K2XDL_pf_ConvertLoadPtr ConvertLoadPtr, XDL_pf_ENTRYPOINT * apRetEntrypoint);

/* -------------------------------------------------------------------------------- - */

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // __K2XDL_H

