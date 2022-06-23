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
#ifndef __K2DLXSUPP_H
#define __K2DLXSUPP_H

/* --------------------------------------------------------------------------------- */

#include <k2systype.h>
#include <lib/k2list.h>

#ifdef __cplusplus
extern "C" {
#endif

#define K2DLXSUPP_FLAG_FULLY_LOADED     1
#define K2DLXSUPP_FLAG_PERMANENT        2
#define K2DLXSUPP_FLAG_ENTRY_CALLED     4
#define K2DLXSUPP_FLAG_KEEP_SYMBOLS     8

typedef struct _K2DLXSUPP_SEG K2DLXSUPP_SEG;
struct _K2DLXSUPP_SEG
{
    UINT_PTR    mDataAddr;
    UINT_PTR    mLinkAddr;
};

typedef struct _K2DLXSUPP_SEGALLOC K2DLXSUPP_SEGALLOC;
struct _K2DLXSUPP_SEGALLOC
{
    K2DLXSUPP_SEG Segment[DlxSeg_Count];
};

typedef void * K2DLXSUPP_HOST_FILE;

typedef struct _K2DLXSUPP_OPENRESULT K2DLXSUPP_OPENRESULT;
struct _K2DLXSUPP_OPENRESULT
{
    K2DLXSUPP_HOST_FILE mHostFile;
    UINT_PTR            mFileSectorCount;
    UINT_PTR            mModulePageDataAddr;
    UINT_PTR            mModulePageLinkAddr;
};

typedef K2STAT (*pfK2DLXSUPP_CritSec)(BOOL aEnter);

typedef void   (*pfK2DLXSUPP_AtReInit)(DLX *apDlx, UINT_PTR aModulePageLinkAddr, K2DLXSUPP_HOST_FILE *apInOutHostFile);

typedef K2STAT (*pfK2DLXSUPP_AcqAlreadyLoaded)(void *apAcqContext, K2DLXSUPP_HOST_FILE aHostFile);
typedef K2STAT (*pfK2DLXSUPP_Open)(void *apAcqContext, char const * apFileSpec, char const *apNamePart, UINT_PTR aNamePartLen, K2DLXSUPP_OPENRESULT *apRetResult);
typedef K2STAT (*pfK2DLXSUPP_ReadSectors)(void *apAcqContext, K2DLXSUPP_HOST_FILE aHostFile, void *apBuffer, UINT_PTR aSectorCount);
typedef K2STAT (*pfK2DLXSUPP_Prepare)(void *apAcqContext, K2DLXSUPP_HOST_FILE aHostFile, DLX_INFO32 *apInfo, UINT_PTR aInfoSize, BOOL aKeepSymbols, K2DLXSUPP_SEGALLOC *apRetAlloc);
typedef BOOL   (*pfK2DLXSUPP_PreCallback)(void *apAcqContext, K2DLXSUPP_HOST_FILE aHostFile, BOOL aIsLoad, DLX *apDlx);
typedef K2STAT (*pfK2DLXSUPP_PostCallback)(void *apAcqContext, K2DLXSUPP_HOST_FILE aHostFile, K2STAT aUserStatus, DLX *apDlx);
typedef K2STAT (*pfK2DLXSUPP_Finalize)(void *apAcqContext, K2DLXSUPP_HOST_FILE aHostFile, K2DLXSUPP_SEGALLOC *apUpdateAlloc);
typedef K2STAT (*pfK2DLXSUPP_RefChange)(K2DLXSUPP_HOST_FILE aHostFile, DLX *apDlx, INT_PTR aRefChange);
typedef K2STAT (*pfK2DLXSUPP_Purge)(K2DLXSUPP_HOST_FILE aHostFile);
typedef K2STAT (*pfK2DLXSUPP_ErrorPoint)(char const *apFile, int aLine, K2STAT aStatus);

typedef struct _K2DLXSUPP_HOST K2DLXSUPP_HOST;
struct _K2DLXSUPP_HOST
{
    UINT_PTR                      mHostSizeBytes;

    pfK2DLXSUPP_CritSec           CritSec;
    pfK2DLXSUPP_AcqAlreadyLoaded  AcqAlreadyLoaded;
    pfK2DLXSUPP_Open              Open;
    pfK2DLXSUPP_AtReInit          AtReInit;
    pfK2DLXSUPP_ReadSectors       ReadSectors;
    pfK2DLXSUPP_Prepare           Prepare;
    pfK2DLXSUPP_PreCallback       PreCallback;
    pfK2DLXSUPP_PostCallback      PostCallback;
    pfK2DLXSUPP_Finalize          Finalize;
    pfK2DLXSUPP_RefChange         RefChange;
    pfK2DLXSUPP_Purge             Purge;
    pfK2DLXSUPP_ErrorPoint        ErrorPoint;
};

typedef BOOL (*pfK2DLXSUPP_ConvertLoadPtr)(UINT_PTR * apAddr);

typedef struct _K2DLXSUPP_PRELOAD K2DLXSUPP_PRELOAD;
struct _K2DLXSUPP_PRELOAD
{
    void *          mpDlxPage;
    UINT8 const *   mpDlxFileData;
    UINT_PTR        mDlxFileSizeBytes;
    UINT_PTR        mDlxBase;
    K2LIST_ANCHOR * mpListAnchorOut;
    UINT_PTR        mDlxMemEndOut;
};

K2STAT
K2DLXSUPP_Init(
    void *              apMemoryPage,
    K2DLXSUPP_HOST *    apHost,
    BOOL                aKeepSym,
    BOOL                aReInit,
    K2DLXSUPP_PRELOAD * apPreload  // can only be non-NULL if aReInit is FALSE
    );

K2STAT
K2DLXSUPP_Handoff(
    DLX **                      appEntryDLX,
    pfK2DLXSUPP_ConvertLoadPtr  ConvertLoadPtr,
    DLX_pf_ENTRYPOINT *         apRetEntrypoint
    );

typedef
K2STAT
(*K2DLXSUPP_pf_GetInfo)(
    DLX *                   apDlx,
    UINT_PTR *              apRetFlags,
    DLX_pf_ENTRYPOINT *     apRetEntrypoint,
    DLX_ARCH_SEGMENT_INFO * apRetSegInfo,
    UINT_PTR *              apRetPageAddr
);

K2STAT
K2DLXSUPP_GetInfo(
    DLX *                   apDlx,
    UINT_PTR *              apRetFlags,
    DLX_pf_ENTRYPOINT *     apRetEntrypoint,
    DLX_ARCH_SEGMENT_INFO * apRetSegInfo,
    UINT_PTR *              apRetPageAddr,
    char const **           appRetFileName
    );

#ifdef __cplusplus
};  // extern "C"
#endif

/* --------------------------------------------------------------------------------- */

#endif  // __K2DLXSUPP_H

