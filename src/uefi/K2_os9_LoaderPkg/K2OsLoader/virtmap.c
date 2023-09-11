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
#include "K2OsLoader.h"

#if K2_TARGET_ARCH_IS_ARM

#define NUM_PDE_PAGES_ALLOC 7
static UINT32 sgSparePage[3];
static UINT32 sgSparePageCount;

#else

#define NUM_PDE_PAGES_ALLOC 1

#endif

static
K2STAT 
sEfiPageTableAlloc(
    UINT32 *apRetPageTableAddr
    )
{
    EFI_STATUS              efiStatus;
    EFI_PHYSICAL_ADDRESS    efiPhysAddr;

#if K2_TARGET_ARCH_IS_ARM
    if (sgSparePageCount > 0)
    {
        sgSparePageCount--;
        *apRetPageTableAddr = sgSparePage[sgSparePageCount];
        return 0;
    }
#endif

    efiStatus = gBS->AllocatePages(AllocateAnyPages, K2OS_EFIMEMTYPE_PAGING, 1, &efiPhysAddr);
    if (efiStatus != EFI_SUCCESS)
        return K2STAT_ERROR_OUT_OF_MEMORY;

    *apRetPageTableAddr = (UINT32)efiPhysAddr;

    return 0;
}

void 
sEfiPageTableFree(
    UINT32 aPageTableAddr
    )
{
    EFI_PHYSICAL_ADDRESS    efiPhysAddr;

#if K2_TARGET_ARCH_IS_ARM
    UINTN                   ix;

    for (ix = 0;ix < 3;ix++)
    {
        if (aPageTableAddr == sgSparePage[ix])
            return;
    }
#endif

    efiPhysAddr = (EFI_PHYSICAL_ADDRESS)aPageTableAddr;

    gBS->FreePages(efiPhysAddr, 1);
}

#if 0
static void sDumpMap(char const *apFormat)
{
    UINT32                  entCount;
    EFI_MEMORY_DESCRIPTOR   desc;
    UINT8 const *           pWork;

    Loader_UpdateMemoryMap();
    K2Printf(L"\n%a\n", apFormat);
    entCount = gData.LoadInfo.mEfiMapSize / gData.LoadInfo.mEfiMemDescSize;
    pWork = (UINT8 const *)gData.mpMemoryMap;
    do
    {
        
        K2MEM_Copy(&desc, pWork, sizeof(K2EFI_MEMORY_DESCRIPTOR));
        K2Printf(L"%08X: %08X%08X %08X%08X %6d %08X%08X\n",
            desc.Type,
            (UINT32)(((UINT64)desc.PhysicalStart) >> 32), (UINT32)(((UINT64)desc.PhysicalStart) & 0x00000000FFFFFFFFull),
            (UINT32)(((UINT64)desc.VirtualStart) >> 32), (UINT32)(((UINT64)desc.VirtualStart) & 0x00000000FFFFFFFFull),
            (UINT32)desc.NumberOfPages,
            (UINT32)(((UINT64)desc.Attribute) >> 32), (UINT32)(((UINT64)desc.Attribute) & 0x00000000FFFFFFFFull));
        pWork += gData.LoadInfo.mEfiMemDescSize;
    } while (--entCount > 0);
}
#endif

EFI_STATUS 
Loader_CreateVirtualMap(
    void
    )
{
    K2STAT                  status;
    EFI_STATUS              efiStatus;
    UINT32                  virtAddr;
    EFI_PHYSICAL_ADDRESS    efiPhysAddr;
    BOOL                    isMultiproc;
    UINTN                   ix;

#if K2_TARGET_ARCH_IS_ARM
    isMultiproc = ((ArmReadMpidr() & 0xC0000000) == 0x80000000) ? TRUE : FALSE;
#else
    isMultiproc = 0;
#endif

    efiStatus = gBS->AllocatePages(AllocateAnyPages, K2OS_EFIMEMTYPE_PAGING, NUM_PDE_PAGES_ALLOC, &efiPhysAddr);
    if (efiStatus != EFI_SUCCESS)
    {
        K2Printf(L"*** Allocate pages for translation table failed with %r\n", efiStatus);
        return efiStatus;
    }

    gData.LoadInfo.mTransBasePhys = (UINT32)efiPhysAddr;
   
#if K2_TARGET_ARCH_IS_ARM
    ix = 3;
    while ((gData.LoadInfo.mTransBasePhys & ~A32_TTBASE_PHYS_MASK) != 0)
    {
        ix--;
        sgSparePage[ix] = gData.LoadInfo.mTransBasePhys;
        gData.LoadInfo.mTransBasePhys += K2_VA_MEMPAGE_BYTES;
    }
    if (ix > 0)
    {
        do
        {
            ix--;
            sgSparePage[ix] = (gData.LoadInfo.mTransBasePhys + A32_TTBASE_PHYS_SIZE) + (ix * K2_VA_MEMPAGE_BYTES);
        } while (ix > 0);
    }
    sgSparePageCount = 3;
#endif
    K2MEM_Zero((void *)gData.LoadInfo.mTransBasePhys, K2_VA32_TRANSTAB_SIZE);

    status = K2VMAP32_Init(
        &gData.Map,
        K2OS_KVA_KERNVAMAP_BASE,
        gData.LoadInfo.mTransBasePhys,
        K2OS_KVA_TRANSTAB_BASE,
        isMultiproc,
        sEfiPageTableAlloc,
        sEfiPageTableFree
        );

    if (K2STAT_IS_ERROR(status))
    {
        K2Printf(L"*** K2VMAP32_Init failed with status 0x%08X\n", status);
        return status;
    }

    //
    // start manual map stuff now - EFIMAP, PTPAGECOUNT, PROC1
    //

    //
    // efi map, map manually
    //
    virtAddr = K2OS_KVA_EFIMAP_BASE;
    ix = K2OS_EFIMAP_PAGECOUNT;
    efiPhysAddr = (EFI_PHYSICAL_ADDRESS)(UINT32)gData.mpMemoryMap;
    do
    {
        status = K2VMAP32_MapPage(&gData.Map, virtAddr, (UINT32)efiPhysAddr, K2OS_MAPTYPE_KERN_DATA);
        if (K2STAT_IS_ERROR(status))
        {
            K2Printf(L"*** K2VMAP32_MapPage for efi map page failed with status 0x%08X\n", status);
            return status;
        }
        virtAddr += K2_VA_MEMPAGE_BYTES;
        efiPhysAddr += K2_VA_MEMPAGE_BYTES;
    } while (--ix > 0);

    //
    // threadptrs page map manually
    //
    efiStatus = gBS->AllocatePages(AllocateAnyPages, 
        K2OS_EFIMEMTYPE_THREADPTRS, 
        1,
        &efiPhysAddr);
    if (efiStatus != EFI_SUCCESS)
    {
        K2Printf(L"*** Allocate page for threadptrs failed with %r\n", efiStatus);
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }
    K2MEM_Zero((void *)((UINTN)efiPhysAddr), K2_VA_MEMPAGE_BYTES);
    status = K2VMAP32_MapPage(&gData.Map, K2OS_KVA_THREADPTRS_BASE, (UINTN)efiPhysAddr, K2OS_MAPTYPE_KERN_DATA);
    if (K2STAT_IS_ERROR(status))
    {
        K2Printf(L"*** K2VMAP32_MapPage for proc0 tokens failed with status 0x%08X\n", status);
        return status;
    }

    //
    // core memory for cores (4 pages per core)
    //
    efiStatus = gBS->AllocatePages(AllocateAnyPages, K2OS_EFIMEMTYPE_CORES, 4 * gData.LoadInfo.mCpuCoreCount, &efiPhysAddr);
    if (efiStatus != EFI_SUCCESS)
    {
        K2Printf(L"*** Allocate page for cores failed with %r\n", efiStatus);
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }
    K2MEM_Zero((void *)((UINTN)efiPhysAddr), K2_VA_MEMPAGE_BYTES *  4 * gData.LoadInfo.mCpuCoreCount);

    virtAddr = K2OS_KVA_COREMEMORY_BASE;
    ix = 4 * gData.LoadInfo.mCpuCoreCount;
    do
    {
        status = K2VMAP32_MapPage(&gData.Map, virtAddr, (UINTN)efiPhysAddr, K2OS_MAPTYPE_KERN_DATA);
        if (K2STAT_IS_ERROR(status))
        {
            K2Printf(L"*** K2VMAP32_MapPage for cores failed with status 0x%08X\n", status);
            return status;
        }
        virtAddr += K2_VA_MEMPAGE_BYTES;
        efiPhysAddr += K2_VA_MEMPAGE_BYTES;
    } while (--ix > 0);

    //
    // loader page
    //
    status = K2VMAP32_MapPage(&gData.Map, K2OS_KVA_LOADERPAGE_BASE, (UINT32)gData.mLoaderPagePhys, K2OS_MAPTYPE_KERN_DATA);
    if (K2STAT_IS_ERROR(status))
    {
        K2Printf(L"*** mapping loader page failed with status 0x%08X\n", status);
    }

    //
    // builtin area
    //
    virtAddr = K2OS_KVA_BUILTIN_FS_BASE;
    efiPhysAddr = gData.LoadInfo.mBuiltinRofsPhys;
    ix = gData.mRofsPages;
    do
    {
        status = K2VMAP32_MapPage(&gData.Map, virtAddr, (UINTN)efiPhysAddr, K2OS_MAPTYPE_KERN_READ);
        if (K2STAT_IS_ERROR(status))
        {
            K2Printf(L"*** K2VMAP32_MapPage for builtin failed with status 0x%08X\n", status);
            return status;
        }
        virtAddr += K2_VA_MEMPAGE_BYTES;
        efiPhysAddr += K2_VA_MEMPAGE_BYTES;
    } while (--ix > 0);

    return status;
}
