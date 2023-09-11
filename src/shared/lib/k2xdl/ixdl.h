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

#ifndef __IXDL_H
#define __IXDL_H

#include <k2systype.h>

#include <lib/k2asc.h>
#include <lib/k2mem.h>
#include <lib/k2list.h>
#include <lib/k2crc.h>
#include <lib/k2elf.h>
#include <lib/k2tree.h>

#include "xdl_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XDL_SECTOR_SPARE_BYTES_COUNT  (XDL_SECTOR_BYTES - (sizeof(XDL) + sizeof(K2XDL_LOADCTX)))
typedef struct _XDL_SECTOR XDL_SECTOR;
struct _XDL_SECTOR
{
    XDL                 Module;
    K2XDL_LOADCTX       LoadCtx;
    UINT8               mSpare[XDL_SECTOR_SPARE_BYTES_COUNT];
};
K2_STATIC_ASSERT(sizeof(XDL_SECTOR) == XDL_SECTOR_BYTES);

#define XDL_PAGE_HDRSECTORS_BYTES    ((XDL_SECTORS_PER_PAGE - 1) * XDL_SECTOR_BYTES)
typedef struct _XDL_PAGE XDL_PAGE;
struct _XDL_PAGE
{
    XDL_SECTOR  ModuleSector;
    UINT8       mHdrSectorsBuffer[XDL_PAGE_HDRSECTORS_BYTES];
};
K2_STATIC_ASSERT(sizeof(XDL_PAGE) == K2_VA_MEMPAGE_BYTES);

typedef struct _XDL_GLOBAL XDL_GLOBAL;
struct _XDL_GLOBAL
{
    K2LIST_ANCHOR   LoadedList;     // this must come first

    K2XDL_HOST      Host;
    K2LIST_ANCHOR   AcqList;
    BOOL            mAcqDisabled;
    BOOL            mKeepSym;
    BOOL            mHandedOff;
};

extern XDL_GLOBAL * gpXdlGlobal;

BOOL   IXDL_FindAndAddRef(XDL *apXdl);
K2STAT IXDL_Link(XDL *apXdl);
K2STAT IXDL_DoCallback(XDL *apXdl, BOOL aIsLoad);
K2STAT IXDL_AcqContain(UINT_PTR aAddr, XDL **appRetXdl, UINT_PTR *apRetSegment);

void   IXDL_ReleaseModule(XDL *apXdl);

#ifdef __cplusplus
};  // extern "C"
#endif


#endif // __IXDL_H