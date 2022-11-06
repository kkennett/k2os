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

#ifndef __K2VMAP64_H
#define __K2VMAP64_H

/* --------------------------------------------------------------------------------- */

#include <k2osbase.h>

#ifdef __cplusplus
extern "C" {
#endif

#define K2VMAP64_FLAG_PRESENT           K2OS_MEMPAGE_ATTR_AUX
#define K2VMAP64_PAGEPHYS_MASK          K2_VA_PAGEFRAME_MASK
#define K2VMAP64_MAPTYPE_MASK           K2OS_MEMPAGE_ATTR_MASK

#define K2VMAP64_FLAG_REALIZED          1
#define K2VMAP64_FLAG_MULTIPROC         2

typedef K2STAT (*pfK2VMAP64_PT_Alloc)(UINT64 *apRetPageTableAddr);
typedef void   (*pfK2VMAP64_PT_Free)(UINT64 aPageTableAddr);
typedef void   (*pfK2VMAP64_Dump)(BOOL aIsPT, UINT64 aVirtAddr, UINT64 aPTE);

typedef struct _K2VMAP64_CONTEXT K2VMAP64_CONTEXT;
struct _K2VMAP64_CONTEXT
{
    UINT64              mTransBasePhys;
    UINT64              mVirtMapBase;
    UINT64              mFlags;
    pfK2VMAP64_PT_Alloc PT_Alloc;
    pfK2VMAP64_PT_Free  PT_Free;
};

K2STAT
K2VMAP64_Init(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aVirtMapBase,
    UINT64              aTransBasePhys,
    UINT64              aTransBaseVirt,
    BOOL                aIsMultiproc,
    pfK2VMAP64_PT_Alloc afPT_Alloc,
    pfK2VMAP64_PT_Free  afPT_Free
    );

UINT64
K2VMAP64_ReadPML4E(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex
);

void
K2VMAP64_WritePML4E(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex,
    UINT64              aPML4E
);

UINT64
K2VMAP64_ReadPDPTE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex
);

void
K2VMAP64_WritePDPTE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex,
    UINT64              aPDPTE
);

UINT64
K2VMAP64_ReadPDE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex
    );

void
K2VMAP64_WritePDE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex,
    UINT64              aPDE
    );

UINT64      
K2VMAP64_VirtToPTE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aVirtAddr, 
    BOOL *              apRetFaultPDE, 
    BOOL *              apRetFaultPTE
    );

K2STAT 
K2VMAP64_MapPage(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aVirtAddr, 
    UINT64              aPhysAddr, 
    UINT64              aPageMapAttr
    );

K2STAT
K2VMAP64_VerifyOrMapPageTableForAddr(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aVirtAddr
    );

K2STAT
K2VMAP64_FindUnmappedVirtualPage(
    K2VMAP64_CONTEXT *  apContext,
    UINT64 *            apVirtAddr,
    UINT64              aVirtEnd
    );

void
K2VMAP64_RealizeArchMappings(
    K2VMAP64_CONTEXT * apContext
    );

void
K2VMAP64_Dump(
    K2VMAP64_CONTEXT *  apContext,
    pfK2VMAP64_Dump     afDump,
    UINT64              aStartVirt,
    UINT64              aEndVirt
    );

#ifdef __cplusplus
};  // extern "C"
#endif

/* --------------------------------------------------------------------------------- */

#endif // __K2VMAP64_H
