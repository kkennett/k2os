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

#ifndef __K2OSBASE_H
#define __K2OSBASE_H

//
// Systems that interact with the OS can include this without
// including the OS specific primitives.  For example, UEFI
// source can include this as long as it has the shared/inc
// path on its list of include paths
//

#include <k2systype.h>

#include "k2osdefs.inc"

//
// globally defined public specifications
//
#include <spec/acpi.h>
#include <spec/fat.h>
#include <spec/smbios.h>
#include <spec/k2uefi.h>

//
// all these libraries are in the shared global CRT
//
#include <lib/k2mem.h>
#include <lib/k2asc.h>
#include <lib/k2list.h>
#include <lib/k2atomic.h>
#include <lib/k2tree.h>
#include <lib/k2crc.h>

#if __cplusplus
extern "C" {
#endif

//
//------------------------------------------------------------------------
//

typedef struct _K2OS_UEFI_LOADINFO K2OS_UEFI_LOADINFO;

typedef struct {
    UINT64                                  mFrameBufferPhys;
    UINT32                                  mFrameBufferBytes;
    K2EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  ModeInfo;
    UINT32                                  mBgrtBmpWidth;
    UINT32                                  mBgrtBmpHeight;
} K2OS_BOOT_GRAPHICS;

//
// this data comes in halfway into the
// transition page between UEFI and the kernel
//
struct _K2OS_UEFI_LOADINFO
{
    UINT32                  mMarker;
    UINT32                  mCpuCoreCount;
    K2EFI_SYSTEM_TABLE *    mpEFIST;
    UINT32                  mKernXdlEntry;
    UINT32                  mSysVirtEntry;
    UINT32                  mKernArenaLow;
    UINT32                  mKernArenaHigh;
    UINT32                  mBuiltinRofsPhys;
    UINT32                  mTransBasePhys;
    UINT32                  mZeroPagePhys;
    UINT32                  mTransitionPageAddr;
    UINT32                  mDebugPageVirt;
    UINT32                  mEfiMapSize;
    UINT32                  mEfiMemDescSize;
    UINT32                  mEfiMemDescVer;
    UINT32                  mFwTabPagesPhys;
    UINT32                  mFwTabPagesVirt;
    UINT32                  mFwTabPageCount;
    UINT32                  mFwFacsPhys;
    UINT32                  mFwXFacsPhys;
    XDL *                   mpXdlCrt;
    UINT32                  mArchTimerRate;
    K2OS_BOOT_GRAPHICS      BootGraf;
};
K2_STATIC_ASSERT(sizeof(K2OS_UEFI_LOADINFO) < (K2_VA_MEMPAGE_BYTES / 2));
K2_STATIC_ASSERT(sizeof(K2OS_UEFI_LOADINFO) == K2OS_UEFI_LOADINFO_SIZE_BYTES);

//
// There is one of these entries per page of physical address space (4GB)
// passed in from EFI to the kernel.  Depending on how much
// physical memory there actually is, not all the virtual pages holding this
// information will be mapped to distinct physical pages. There is a 'zero page'
// that any empty space regions of 4MB aligned to 4MB boundaries will be tracked by.
// This means that if you only have 64MB of memory in your system, you are only 
// paying for the physical memory to track that 64MB, not for the physical memory
// needed to track 4GB.  If you *do* have 4GB of memory in your system, you will
// pay a maximum of 16MB memory charge to track it at the page level.
// 16 bytes * 1 Million pages = 16MB.  THis is the size of the virtual area
// carved out in the kernel va space for this purpose.  VA space is cheap
//
typedef struct _K2OS_PHYSTRACK_UEFI K2OS_PHYSTRACK_UEFI;
struct _K2OS_PHYSTRACK_UEFI
{
    UINT32  mProp;
    UINT32  mEfiMemType;
    UINT32  mUnused0;
    UINT32  mUnused1;
};

K2_STATIC_ASSERT(sizeof(K2OS_PHYSTRACK_UEFI) == K2OS_PHYSTRACK_BYTES);
K2_STATIC_ASSERT(sizeof(K2OS_PHYSTRACK_UEFI) == sizeof(K2TREE_NODE));

//
//------------------------------------------------------------------------
//

enum _K2OS_CACHEOP
{
    K2OS_CACHEOP_FlushDataAll,
    K2OS_CACHEOP_InvalidateDataAll,
    K2OS_CACHEOP_FlushInvalidateDataAll,
    K2OS_CACHEOP_InvalidateInstructionAll,
    K2OS_CACHEOP_FlushDataRange,
    K2OS_CACHEOP_InvalidateDataRange,
    K2OS_CACHEOP_FlushInvalidateDataRange,
    K2OS_CACHEOP_InvalidateInstructionRange
};
typedef enum _K2OS_CACHEOP K2OS_CACHEOP;

void
K2OS_CacheOperation(
    K2OS_CACHEOP  aCacheOp,
    UINT32        aAddress,
    UINT32        aBytes
);

//
//------------------------------------------------------------------------
//

#if __cplusplus
}
#endif

#endif // __K2OSBASE_H