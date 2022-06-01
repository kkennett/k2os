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
#ifndef __IDLX_H
#define __IDLX_H

/* --------------------------------------------------------------------------------- */

#include <k2systype.h>

#include <lib/k2asc.h>
#include <lib/k2mem.h>
#include <lib/k2list.h>
#include <lib/k2crc.h>
#include <lib/k2elf.h>
#include <lib/k2tree.h>
#include <lib/k2dlxsupp.h>

#include "dlx_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define K2DLX_MAX_SECTION_COUNT (((DLX_SECTOR_BYTES - sizeof(DLX)) + (sizeof(UINT32) - 1)) / sizeof(UINT32))

typedef struct _K2DLX_SECTOR K2DLX_SECTOR;
struct _K2DLX_SECTOR
{
    DLX     Module;          // must be first thing in sector
    UINT32  mSecAddr[K2DLX_MAX_SECTION_COUNT];
};
K2_STATIC_ASSERT(sizeof(K2DLX_SECTOR) == DLX_SECTOR_BYTES);

#define K2DLX_PAGE_HDRSECTORS_BYTES   ((DLX_SECTORS_PER_PAGE - 1) * DLX_SECTOR_BYTES)

typedef struct _K2DLX_PAGE K2DLX_PAGE;
struct _K2DLX_PAGE
{
    K2DLX_SECTOR    ModuleSector;    // must be first thing in page
    UINT8           mHdrSectorsBuffer[K2DLX_PAGE_HDRSECTORS_BYTES];
};
K2_STATIC_ASSERT(sizeof(K2DLX_PAGE) == K2_VA_MEMPAGE_BYTES);

typedef struct _K2DLXSUPP_VARS K2DLXSUPP_VARS;
struct _K2DLXSUPP_VARS
{
    K2DLXSUPP_HOST  Host;
    K2LIST_ANCHOR   LoadedList;
    K2LIST_ANCHOR   AcqList;
    BOOL            mAcqDisabled;
    BOOL            mKeepSym;
    BOOL            mHandedOff;
};

void
iK2DLXSUPP_ReleaseImports(
    DLX *   apDlx,
    UINT32  aAcqCount
    );

void
iK2DLXSUPP_ReleaseModule(
    DLX *   apDlx
);

K2STAT
iK2DLXSUPP_DoCallback(
    void *  apAcqContext,
    DLX *   apDlx,
    BOOL    aIsLoad
    );

K2STAT
iK2DLXSUPP_LoadSegments(
    void *  apAcqContext,
    DLX *   apDlx
    );

K2STAT
iK2DLXSUPP_Link(
    DLX *   apDlx
    );

DLX *
iK2DLXSUPP_FindAndAddRef(
    DLX *   apDlx
    );

K2STAT
iK2DLXSUPP_FindExport(
    DLX_EXPORTS_SECTION const * apSec,
    char const *                apName,
    UINT32 *                    apRetAddr
    );

void
iK2DLXSUPP_Cleanup(
    DLX *   apDlx
    );

int
iK2DLXSUPP_CompareUINT32(
    UINT32          aKey,
    K2TREE_NODE *   apDlx
    );

void
iK2DLXSUPP_Preload(
    K2DLXSUPP_PRELOAD * apPreload
);

#if K2_FLAG_NODEBUG
#define K2DLXSUPP_ERRORPOINT(x)   x
#else
#define K2DLXSUPP_ERRORPOINT(x)   ((gpK2DLXSUPP_Vars->Host.ErrorPoint != NULL) ? gpK2DLXSUPP_Vars->Host.ErrorPoint(__FILE__, __LINE__,x) : (x))
#endif

extern K2DLXSUPP_VARS * gpK2DLXSUPP_Vars;

#ifdef __cplusplus
};  // extern "C"
#endif

/* --------------------------------------------------------------------------------- */

#endif  // __IDLX_H

