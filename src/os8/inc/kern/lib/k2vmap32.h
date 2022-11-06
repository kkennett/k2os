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

#ifndef __K2VMAP32_H
#define __K2VMAP32_H

/* --------------------------------------------------------------------------------- */

#include <k2osbase.h>

#ifdef __cplusplus
extern "C" {
#endif

#define K2VMAP32_FLAG_PRESENT           K2OS_MEMPAGE_ATTR_AUX
#define K2VMAP32_PAGEPHYS_MASK          K2_VA_PAGEFRAME_MASK
#define K2VMAP32_MAPTYPE_MASK           K2OS_MEMPAGE_ATTR_MASK

#define K2VMAP32_FLAG_REALIZED          1
#define K2VMAP32_FLAG_MULTIPROC         2

typedef K2STAT (*pfK2VMAP32_PT_Alloc)(UINT32 *apRetPageTableAddr);
typedef void   (*pfK2VMAP32_PT_Free)(UINT32 aPageTableAddr);
typedef void   (*pfK2VMAP32_Dump)(BOOL aIsPT, UINT32 aVirtAddr, UINT32 aPTE);

typedef struct _K2VMAP32_CONTEXT K2VMAP32_CONTEXT;
struct _K2VMAP32_CONTEXT
{
    UINT32              mTransBasePhys;
    UINT32              mVirtMapBase;
    UINT32              mFlags;
    pfK2VMAP32_PT_Alloc PT_Alloc;
    pfK2VMAP32_PT_Free  PT_Free;
};

K2STAT
K2VMAP32_Init(
    K2VMAP32_CONTEXT *  apContext,
    UINT32              aVirtMapBase,
    UINT32              aTransBasePhys,
    UINT32              aTransBaseVirt,
    BOOL                aIsMultiproc,
    pfK2VMAP32_PT_Alloc afPT_Alloc,
    pfK2VMAP32_PT_Free  afPT_Free
    );

UINT32
K2VMAP32_ReadPDE(
    K2VMAP32_CONTEXT *  apContext,
    UINT32              aIndex
    );

void
K2VMAP32_WritePDE(
    K2VMAP32_CONTEXT *  apContext,
    UINT32              aIndex,
    UINT32              aPDE
    );

UINT32      
K2VMAP32_VirtToPTE(
    K2VMAP32_CONTEXT *  apContext,
    UINT32              aVirtAddr, 
    BOOL *              apRetFaultPDE, 
    BOOL *              apRetFaultPTE
    );

K2STAT 
K2VMAP32_MapPage(
    K2VMAP32_CONTEXT *  apContext,
    UINT32              aVirtAddr, 
    UINT32              aPhysAddr, 
    UINT32              aPageMapAttr
    );

K2STAT
K2VMAP32_VerifyOrMapPageTableForAddr(
    K2VMAP32_CONTEXT *  apContext,
    UINT32              aVirtAddr
    );

K2STAT
K2VMAP32_FindUnmappedVirtualPage(
    K2VMAP32_CONTEXT *  apContext,
    UINT32 *            apVirtAddr,
    UINT32              aVirtEnd
    );

void
K2VMAP32_RealizeArchMappings(
    K2VMAP32_CONTEXT * apContext
    );

void
K2VMAP32_Dump(
    K2VMAP32_CONTEXT *  apContext,
    pfK2VMAP32_Dump     afDump,
    UINT32              aStartVirt,
    UINT32              aEndVirt
    );

#ifdef __cplusplus
};  // extern "C"
#endif

/* --------------------------------------------------------------------------------- */

#endif // __K2VMAP32_H
