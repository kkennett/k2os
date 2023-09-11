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
#ifndef __K2XDL_H
#define __K2XDL_H

#include <k2systype.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------------- - */

typedef UINT_PTR K2XDL_HOST_FILE;

typedef struct _K2XDL_OPENARGS  K2XDL_OPENARGS;
typedef struct _K2XDL_LOADCTX   K2XDL_LOADCTX;

struct _K2XDL_OPENARGS
{
    K2XDL_LOADCTX const *   mpParentLoadCtx;
    UINT_PTR                mAcqContext;
    char const *            mpPath;
    char const *            mpNamePart;
    UINT_PTR                mNameLen;
};

struct _K2XDL_LOADCTX
{
    K2XDL_OPENARGS          OpenArgs;
    K2XDL_HOST_FILE         mHostFile;
    UINT_PTR                mModulePageDataAddr;
    UINT_PTR                mModulePageLinkAddr;
};

typedef struct _K2XDL_SEGMENT_ADDRS K2XDL_SEGMENT_ADDRS;
struct _K2XDL_SEGMENT_ADDRS
{
    UINT64  mData[XDLSegmentIx_Count];
    UINT64  mLink[XDLSegmentIx_Count];
};

typedef K2STAT (*K2XDL_pf_CritSec)(BOOL aEnter);

typedef K2STAT (*K2XDL_pf_Open)(K2XDL_OPENARGS const *apArgs, K2XDL_HOST_FILE *apRetHostFile, UINT_PTR *apRetModuleDataAddr, UINT_PTR *apRetModuleLinkAddr);
typedef K2STAT (*K2XDL_pf_ResizeCopyModulePage)(K2XDL_LOADCTX const *apLoadCtx, UINT_PTR aNewPageCount, UINT_PTR *apModuleDataAddr, UINT_PTR *apModuleLinkAddr);
typedef K2STAT (*K2XDL_pf_ReadSectors)(K2XDL_LOADCTX const *apLoadCtx, void *apBuffer, UINT64 const *apSectorCount);
typedef K2STAT (*K2XDL_pf_Prepare)(K2XDL_LOADCTX const *apLoadCtx, BOOL aKeepSymbols, XDL_FILE_HEADER const *apFileHdr, K2XDL_SEGMENT_ADDRS *apRetSegAddrs);
typedef BOOL   (*K2XDL_pf_PreCallback)(K2XDL_LOADCTX const *apLoadCtx, BOOL aIsLoad, XDL *apXdl);
typedef void   (*K2XDL_pf_PostCallback)(K2XDL_LOADCTX const *apLoadCtx, K2STAT aUserStatus, XDL *apXdl);
typedef K2STAT (*K2XDL_pf_Finalize)(K2XDL_LOADCTX const *apLoadCtx, BOOL aKeepSymbols, XDL_FILE_HEADER const * apFileHdr, K2XDL_SEGMENT_ADDRS *apUpdateSegAddrs);
typedef K2STAT (*K2XDL_pf_Purge)(K2XDL_LOADCTX const *apLoadCtx, K2XDL_SEGMENT_ADDRS const * apSegAddrs);
typedef void   (*K2XDL_pf_AtReInit)(XDL *apXdl, UINT_PTR aModulePageLinkAddr, K2XDL_HOST_FILE *apInOutHostFile);

typedef struct _K2XDL_HOST K2XDL_HOST;
struct _K2XDL_HOST
{
    K2XDL_pf_CritSec                CritSec;
    K2XDL_pf_Open                   Open;
    K2XDL_pf_ResizeCopyModulePage   ResizeCopyModulePage;
    K2XDL_pf_ReadSectors            ReadSectors;
    K2XDL_pf_Prepare                Prepare;
    K2XDL_pf_PreCallback            PreCallback;
    K2XDL_pf_PostCallback           PostCallback;
    K2XDL_pf_Finalize               Finalize;
    K2XDL_pf_Purge                  Purge;
    K2XDL_pf_AtReInit               AtReInit;
};

typedef BOOL (*K2XDL_pf_ConvertLoadPtr)(UINT_PTR * apAddr);

K2STAT K2XDL_Init(void * apXdlControlPage, K2XDL_HOST *apHost, BOOL aKeepSym, BOOL aReInit);

K2STAT K2XDL_InitSelf(void *apXdlControlPage, void *apSelfPage, XDL_FILE_HEADER const *apHeader, K2XDL_HOST *apHost, BOOL aKeepSym);

K2STAT K2XDL_Handoff(XDL ** appEntryXdl, K2XDL_pf_ConvertLoadPtr ConvertLoadPtr, XDL_pf_ENTRYPOINT * apRetEntrypoint);

/* -------------------------------------------------------------------------------- - */

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // __K2XDL_H

