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

static
K2STAT
sSetupToTrackOneDescriptor(
    EFI_MEMORY_DESCRIPTOR *apDesc
    )
{
    UINT32                  firstByte;
    UINT32                  areaSize;
    UINT32                  lastByte;
    UINT32                  virtTrackWorkPage;
    UINT32                  virtTrackLastPage;
    UINT32                  mapType;
    BOOL                    pdeFault;
    BOOL                    pteFault;
    K2STAT                  status;
    EFI_STATUS              efiStatus;
    EFI_PHYSICAL_ADDRESS    efiPhysAddr;

    firstByte = (UINT32)(apDesc->PhysicalStart & 0x00000000FFFFFFFFULL);
//    K2Printf(L"FirstByte = %08X\n", firstByte);
    areaSize = (UINT32)((apDesc->NumberOfPages * K2_VA_MEMPAGE_BYTES) & 0x00000000FFFFFFFFULL);
    lastByte = firstByte + areaSize - 1;
//    K2Printf(L" LastByte = %08X\n", lastByte);

//    K2Printf(L"%08X->%08X %08X att %08X typ %08X", firstByte, lastByte, areaSize, (UINT32)(apDesc->Attribute & 0x00000000FFFFFFFFULL), apDesc->Type);
    if (apDesc->Attribute & K2EFI_MEMORYFLAG_RUNTIME)
    {
        //
        // make space for this runtime region in the virtual map
        // shoving it into the top of the map
        //
        virtTrackLastPage = gData.LoadInfo.mKernArenaHigh - K2_VA_MEMPAGE_BYTES;
        virtTrackWorkPage = gData.LoadInfo.mKernArenaHigh - areaSize;
        gData.LoadInfo.mKernArenaHigh = virtTrackWorkPage;
//        K2Printf(L" %08X-%08X", virtTrackWorkPage, virtTrackWorkPage + areaSize - 1);
        efiPhysAddr = apDesc->PhysicalStart;
        if (apDesc->Type == EfiRuntimeServicesCode)
            mapType = K2OS_MAPTYPE_KERN_TEXT;
        else if ((apDesc->Type == EfiMemoryMappedIO) ||
                 (apDesc->Type == EfiMemoryMappedIOPortSpace) ||
                 (apDesc->Type == EfiACPIMemoryNVS))
            mapType = K2OS_MAPTYPE_KERN_DEVICEIO;
        else
            mapType = K2OS_MAPTYPE_KERN_DATA;
        do
        {
//            K2Printf(L"Map runtime type %d page %08X\n", mapType, virtTrackWorkPage);
            status = K2VMAP32_MapPage(
                &gData.Map,
                virtTrackWorkPage,
                (UINT32)efiPhysAddr,
                mapType);
            if (K2STAT_IS_ERROR(status))
                return status;
            virtTrackWorkPage += K2_VA_MEMPAGE_BYTES;
            efiPhysAddr += K2_VA_MEMPAGE_BYTES;
        } while (virtTrackWorkPage <= virtTrackLastPage);
    }
//    K2Printf(L"\n");

    virtTrackWorkPage = K2OS_PHYS32_TO_PHYSTRACK(firstByte) & K2_VA_PAGEFRAME_MASK;
//    K2Printf(L"vTrakFrst = %08X\n", virtTrackWorkPage);
    virtTrackLastPage = K2OS_PHYS32_TO_PHYSTRACK(lastByte) & K2_VA_PAGEFRAME_MASK;
//    K2Printf(L"vTrakLast = %08X\n", virtTrackLastPage);

    // virtTrackWorkPage to virtTrackLastPage needs to be mapped 
    // to real usable memory that has been zeroed out
    do
    {
        K2VMAP32_VirtToPTE(&gData.Map, virtTrackWorkPage, &pdeFault, &pteFault);
        if (pdeFault || pteFault)
        {
//            K2Printf(L"Eat page for Tracking 0x%08X\n", virtTrackWorkPage);
            efiStatus = gBS->AllocatePages(AllocateAnyPages, K2OS_EFIMEMTYPE_PHYS_TRACK, 1, &efiPhysAddr);
            if (efiStatus != EFI_SUCCESS) 
            {
                K2Printf(L"*** Out of memory allocating for physical tracking\n");
                return K2STAT_ERROR_OUT_OF_MEMORY;
            }
            K2MEM_Zero((void *)((UINTN)efiPhysAddr), K2_VA_MEMPAGE_BYTES);
            status = K2VMAP32_MapPage(&gData.Map, virtTrackWorkPage, (UINT32)efiPhysAddr, K2OS_MAPTYPE_KERN_DATA);
            if (K2STAT_IS_ERROR(status))
            {
                K2Printf(L"*** Error %08X mapping page for tracking\n", status);
                return status;
            }
        }
        virtTrackWorkPage += K2_VA_MEMPAGE_BYTES;
    } while (virtTrackWorkPage <= virtTrackLastPage);

    return 0;
}

static
K2STAT
sSetupPhysTrackZero(
    void
    )
{
    UINT32      virtTrackWorkPage;
    UINT32      virtTrackLastPage;
    BOOL        pdeFault;
    BOOL        pteFault;
    UINTN       ixPDE;
    UINTN       ixPTE;
    UINT32      pde;
    UINT32      virtPT;
    UINT32 *    pPT;
    UINT32      largePTZeroPhys;
    BOOL        createdLargePTZero;
    K2STAT status;

//    K2Printf(L"ZERO\n");
    virtTrackWorkPage = K2OS_PHYS32_TO_PHYSTRACK(0) & K2_VA_PAGEFRAME_MASK;
//    K2Printf(L"vTrakFrst = %08X\n", virtTrackWorkPage);
    virtTrackLastPage = K2OS_PHYS32_TO_PHYSTRACK(0xFFFFFFFF) & K2_VA_PAGEFRAME_MASK;
//    K2Printf(L"vTrakLast = %08X\n", virtTrackLastPage);
    createdLargePTZero = FALSE;
    largePTZeroPhys = 0;
    do
    {
        K2VMAP32_VirtToPTE(&gData.Map, virtTrackWorkPage, &pdeFault, &pteFault);
        if (pdeFault)
        {
//            K2Printf(L"PDE FAULT 0x%08X\n", virtTrackWorkPage);
            K2_ASSERT((virtTrackWorkPage & (K2_VA32_PAGETABLE_MAP_BYTES - 1)) == 0);

            ixPDE = virtTrackWorkPage / K2_VA32_PAGETABLE_MAP_BYTES;

            // entire 4MB area needs to be set to zeros
            if (!createdLargePTZero)
            {
//                K2Printf(L"  Create large PTZero - map %08X to %08X\n", virtTrackWorkPage, gData.LoadInfo.mZeroPagePhys);
                status = K2VMAP32_MapPage(&gData.Map, virtTrackWorkPage, gData.LoadInfo.mZeroPagePhys, K2OS_MAPTYPE_KERN_READ);
                if (K2STAT_IS_ERROR(status))
                    return status;

                pde = K2VMAP32_ReadPDE(&gData.Map, ixPDE);
                K2_ASSERT(pde != 0);
                largePTZeroPhys = pde & K2VMAP32_PAGEPHYS_MASK;
//                K2Printf(L"LargePTZeroPhys = %08X\n", largePTZeroPhys);

                pPT = (UINT32 *)largePTZeroPhys;
                for (ixPTE = 1; ixPTE < K2_VA32_ENTRIES_PER_PAGETABLE; ixPTE++)
                    pPT[ixPTE] = pPT[0];

                createdLargePTZero = TRUE;
            }
            else
            {
//                K2Printf(L"  Use large PTZero\n");
                virtPT = K2OS_KVA_TO_PT_ADDR(virtTrackWorkPage);
//                K2Printf(L"  virtPT = 0x%08X\n", virtPT);
                K2VMAP32_VirtToPTE(&gData.Map, virtPT, &pdeFault, &pteFault);
                K2_ASSERT(!pdeFault);
                K2_ASSERT(pteFault);
                status = K2VMAP32_MapPage(&gData.Map, virtPT, largePTZeroPhys, K2OS_MAPTYPE_KERN_PAGETABLE);
                if (K2STAT_IS_ERROR(status))
                    return status;
                K2VMAP32_VirtToPTE(&gData.Map, virtPT, &pdeFault, &pteFault);
                K2_ASSERT(!pdeFault);
                K2_ASSERT(!pteFault);

                K2VMAP32_WritePDE(&gData.Map, ixPDE, largePTZeroPhys | K2OS_MAPTYPE_KERN_PAGEDIR | K2VMAP32_FLAG_PRESENT);

                K2VMAP32_VirtToPTE(&gData.Map, virtTrackWorkPage, &pdeFault, &pteFault);
                K2_ASSERT(!pdeFault);
                K2_ASSERT(!pteFault);
            }

            K2VMAP32_VirtToPTE(&gData.Map, virtTrackWorkPage, &pdeFault, &pteFault);
            if ((pdeFault) || (pteFault))
            {
                K2Printf(L"LargeZero map failed 0x%08X -- %d,%d\n", virtTrackWorkPage, pdeFault, pteFault);
                K2_ASSERT(0);
            }

//            virtTrackWorkPage += K2_VA32_PAGETABLE_MAP_BYTES;
            virtTrackWorkPage += K2_VA_MEMPAGE_BYTES;
        }
        else
        {
            if (pteFault)
            {
                status = K2VMAP32_MapPage(&gData.Map, virtTrackWorkPage, gData.LoadInfo.mZeroPagePhys, K2OS_MAPTYPE_KERN_READ);
                if (K2STAT_IS_ERROR(status))
                    return status;
            }

            virtTrackWorkPage += K2_VA_MEMPAGE_BYTES;
        }

    } while (virtTrackWorkPage <= virtTrackLastPage);

//    K2Printf(L"Done zero\n");
    return 0;
}

static
void
sTrackOneDescriptor(
    EFI_MEMORY_DESCRIPTOR *apDesc
    )
{
    UINT32                  firstPage;
    UINT32                  lastPage;
    UINT32                  virtTrackWork;
    UINT32                  virtTrackLast;
    BOOL                    pdeFault;
    BOOL                    pteFault;
    UINT32                  lastMap;
    UINT32                  v2pOffset;
    K2OS_PHYSTRACK_UEFI *   pTrack;

    firstPage = (UINT32)(apDesc->PhysicalStart & 0x00000000FFFFFFFFULL);
//    K2Printf(L"FirstPage = %08X\n", firstPage);
    lastPage = firstPage - 1 + (UINT32)((apDesc->NumberOfPages * K2_VA_MEMPAGE_BYTES) & 0x00000000FFFFFFFFULL);
//    K2Printf(L" LastPage = %08X\n", lastPage);

    virtTrackWork = K2OS_PHYS32_TO_PHYSTRACK(firstPage);
//    K2Printf(L"vTrakFrst = %08X\n", virtTrackWork);
    virtTrackLast = K2OS_PHYS32_TO_PHYSTRACK(lastPage);
//    K2Printf(L"vTrakLast = %08X\n", virtTrackLast);

    // virtTrackWork to virtTrackLast needs to be set up with
    // the properties of this range of physical space
    lastMap = (UINT32)-1;
    v2pOffset = 0;
    do
    {
        if ((virtTrackWork & K2_VA_PAGEFRAME_MASK) != lastMap)
        {
            v2pOffset = K2VMAP32_VirtToPTE(&gData.Map, virtTrackWork, &pdeFault, &pteFault);
            K2_ASSERT(pdeFault == FALSE);
            K2_ASSERT(pteFault == FALSE);
            K2_ASSERT((v2pOffset & K2OS_MAPTYPE_KERN_DATA) == K2OS_MAPTYPE_KERN_DATA);

            v2pOffset &= K2_VA_PAGEFRAME_MASK;

            K2_ASSERT(v2pOffset != gData.LoadInfo.mZeroPagePhys);
//            K2Printf(L"Mapped Tracking Page at v%08X to p%08X\n", virtTrackWork, v2pOffset);
            lastMap = (virtTrackWork & K2_VA_PAGEFRAME_MASK);
            v2pOffset = v2pOffset - lastMap;
        }
        pTrack = (K2OS_PHYSTRACK_UEFI *)(virtTrackWork + v2pOffset);
        K2_ASSERT(pTrack->mProp == 0);
        pTrack->mProp = (UINT32)(apDesc->Attribute & K2EFI_MEMORYFLAG_PROP_MASK);
        pTrack->mEfiMemType = apDesc->Type;
        virtTrackWork += K2OS_PHYSTRACK_BYTES;
    } while (virtTrackWork <= virtTrackLast);
}

K2STAT
Loader_AssignRuntimeVirtual(
    void
    )
{
    UINT32                  virtWork;
    UINT32                  virtEnd;
    UINT32                  pagePhys;
    BOOL                    pdeFault;
    BOOL                    pteFault;
    UINTN                   entCount;
    UINTN                   entIx;
    EFI_MEMORY_DESCRIPTOR   desc;

    //
    // CANNOT DO DEBUG PRINTS HERE UNLESS WE ARE FAILING
    // AS PRINTING ON THE DEBUG CHANNEL WILL ALLOCATE MEMORY WHICH
    // WILL CHANGE THE MEMORY MAP!!
    //

    //
    // every allocation between arenahigh and preruntimehigh must be
    // an EFI runtime allocation.  stuff above that is other junk
    //
    virtWork = gData.LoadInfo.mKernArenaHigh;
    virtEnd = gData.mPreRuntimeHigh;
    entCount = gData.LoadInfo.mEfiMapSize / gData.LoadInfo.mEfiMemDescSize;

    do
    {
        pagePhys = K2VMAP32_VirtToPTE(&gData.Map, virtWork, &pdeFault, &pteFault);
        K2_ASSERT(pdeFault == FALSE);
        K2_ASSERT(pteFault == FALSE);

        pagePhys &= K2_VA_PAGEFRAME_MASK;

        for (entIx = 0; entIx < entCount; entIx++)
        {
            K2MEM_Copy(&desc, ((UINT8 *)gData.mpMemoryMap) + (gData.LoadInfo.mEfiMemDescSize * entIx), sizeof(EFI_MEMORY_DESCRIPTOR));
            if (desc.NumberOfPages == 0)
                break;
            if (desc.Attribute & K2EFI_MEMORYFLAG_RUNTIME)
            {
                if (pagePhys == (UINT32)desc.PhysicalStart)
                    break;
            }
        }

        if (entIx == entCount)
        {
            K2Printf(L"*** Virtual %08X could not be found in runtime required mappings\n");
            return K2STAT_ERROR_UNKNOWN;
        }

        desc.VirtualStart = virtWork;

        K2MEM_Copy(((UINT8 *)gData.mpMemoryMap) + (gData.LoadInfo.mEfiMemDescSize * entIx), &desc, sizeof(EFI_MEMORY_DESCRIPTOR));

        virtWork += (UINT32)((desc.NumberOfPages * K2_VA_MEMPAGE_BYTES) & 0x00000000FFFFFFFFULL);

    } while (virtWork < virtEnd);

    return 0;
}

static
K2STAT
sFindTransitionPage(
    void
    )
{
    UINTN                   entCount;
    UINTN                   entIx;
    EFI_MEMORY_DESCRIPTOR   desc;
    UINT32                  virtAddr;
    K2STAT             status;
    EFI_STATUS              efiStatus;
    EFI_PHYSICAL_ADDRESS    efiPhysAddr;

    entCount = gData.LoadInfo.mEfiMapSize / gData.LoadInfo.mEfiMemDescSize;
    for (entIx = 0; entIx < entCount; entIx++)
    {
        K2MEM_Copy(&desc, ((UINT8 *)gData.mpMemoryMap) + (gData.LoadInfo.mEfiMemDescSize * entIx), sizeof(EFI_MEMORY_DESCRIPTOR));
        if (desc.NumberOfPages == 0)
            break;
        if ((desc.Type == K2EFI_MEMTYPE_Conventional) && (desc.NumberOfPages > 0))
        {
            virtAddr = (UINT32)(desc.PhysicalStart & 0x00000000FFFFFFFFULL);
            status = K2VMAP32_FindUnmappedVirtualPage(
                &gData.Map,
                &virtAddr,
                virtAddr + (UINT32)((desc.NumberOfPages * K2_VA_MEMPAGE_BYTES) & 0x00000000FFFFFFFFULL));
            if (!K2STAT_IS_ERROR(status))
            {
                efiPhysAddr = virtAddr;
                efiStatus = gBS->AllocatePages(AllocateAddress, K2OS_EFIMEMTYPE_TRANSITION, 1, &efiPhysAddr);
                if (efiStatus == EFI_SUCCESS)
                {
                    status = K2VMAP32_MapPage(&gData.Map, (UINT32)virtAddr, (UINT32)virtAddr, K2OS_MAPTYPE_KERN_TRANSITION);
                    if (!K2STAT_IS_ERROR(status))
                    {
//                        K2Printf(L"Direct-Mapped Transition Page at 0x%08X\n", virtAddr);
                        gData.LoadInfo.mTransitionPageAddr = virtAddr;
                        return 0;
                    }
                    K2Printf(L"*** Failed to direct map transition page\n");
                }
                else
                {
                    K2Printf(L"*** Allocate Transition page failed\n");
                }
            }
            else
            {
                K2Printf(L"*** Range is completely mapped\n");
            }
        }
    }

    return K2STAT_ERROR_NOT_FOUND;
}

K2STAT 
Loader_TrackEfiMap(
    void
    )
{
    EFI_STATUS              efiStatus;
    K2STAT                  status;
    UINTN                   entCount;
    EFI_MEMORY_DESCRIPTOR   desc;
    UINTN                   entIx;
    UINT32                  virtAddr;
    EFI_PHYSICAL_ADDRESS    efiPhysAddr;

    efiStatus = gBS->AllocatePages(AllocateAnyPages, K2OS_EFIMEMTYPE_ZERO, 1, &efiPhysAddr);
    if (efiStatus != EFI_SUCCESS)
    {
        K2Printf(L"Allocate Zero page failed with %r\n", efiStatus);
        return K2STAT_ERROR_OUT_OF_MEMORY;
    }
    gData.LoadInfo.mZeroPagePhys = (UINT32)efiPhysAddr;
    K2MEM_Zero((void *)gData.LoadInfo.mZeroPagePhys, K2_VA_MEMPAGE_BYTES);
//    K2Printf(L"Zero Page at Physical 0x%08X\n", gData.LoadInfo.mZeroPagePhys);

    //
    // track lowest high address prior to runtime virtual alloc, which happens last
    //
    gData.mPreRuntimeHigh = gData.LoadInfo.mKernArenaHigh;

    //
    // first pass just discovers all present physical ranges
    // and allocates pages for tracking them
    //
    efiStatus = Loader_UpdateMemoryMap();
    if (efiStatus != EFI_SUCCESS)
        return K2STAT_ERROR_OUT_OF_MEMORY;
    entCount = gData.LoadInfo.mEfiMapSize / gData.LoadInfo.mEfiMemDescSize;
    for (entIx = 0; entIx < entCount; entIx++)
    {
        K2MEM_Copy(&desc, ((UINT8 *)gData.mpMemoryMap) + (gData.LoadInfo.mEfiMemDescSize * entIx), sizeof(EFI_MEMORY_DESCRIPTOR));
        if (desc.NumberOfPages == 0)
            break;
        if ((desc.Type < K2EFI_MEMTYPE_COUNT) ||
            ((desc.Type >= 0x80000000) && (desc.Type <= K2OS_EFIMEMTYPE_LAST)))
        {
            status = sSetupToTrackOneDescriptor(&desc);
            if (K2STAT_IS_ERROR(status))
                return status;
        }
    }

    //
    // all tracking for present physical space allocated now
    // we can map any unmapped tracking area page to the zero page
    // this will allocate a whole bunch of pagetables on systems that don't have
    // 4GB of RAM
    //
    status = sSetupPhysTrackZero();
    if (K2STAT_IS_ERROR(status))
        return status;

    //
    // ensure all pagetables for area up to free bottom are present
    //
    virtAddr = K2OS_KVA_KERN_BASE;
    do
    {
        status = K2VMAP32_VerifyOrMapPageTableForAddr(&gData.Map, virtAddr);
        if (K2STAT_IS_ERROR(status))
        {
            K2Printf(L"*** K2VMAP32_VerifyOrMapPageTableForAddr failed with status 0x%08X\n", status);
            return status;
        }
        virtAddr += K2_VA32_PAGETABLE_MAP_BYTES;
    } while (virtAddr < gData.LoadInfo.mKernArenaLow);

    //
    // find a transition page now
    //
    efiStatus = Loader_UpdateMemoryMap();
    if (efiStatus != EFI_SUCCESS)
        return K2STAT_ERROR_OUT_OF_MEMORY;
    status = sFindTransitionPage();
    if (K2STAT_IS_ERROR(status))
        return status;

    //
    // re-acquire memory map now that everything 
    // should be mapped.  this will give us the proper types for 
    // each page of memory (allocated, etc etc) now that all
    // page tables are allocated and all tracking memory is
    // allocated as well.
    //
    efiStatus = Loader_UpdateMemoryMap();
    if (efiStatus != EFI_SUCCESS)
        return K2STAT_ERROR_OUT_OF_MEMORY;
    entCount = gData.LoadInfo.mEfiMapSize / gData.LoadInfo.mEfiMemDescSize;

    //
    // entire region of usable physical space has tracking mapped for every
    // phystrack structure.  nonexistent ranges are all mapped to the same
    // page full of zeroes, read-only to ensure nobody tries to alter them.
    // so not we can run throught the memory descriptors again and set the memory
    // types of the phystrack entries
    //
    for (entIx = 0; entIx < entCount; entIx++)
    {
        K2MEM_Copy(&desc, ((UINT8 *)gData.mpMemoryMap) + (gData.LoadInfo.mEfiMemDescSize * entIx), sizeof(EFI_MEMORY_DESCRIPTOR));
        if (desc.NumberOfPages == 0)
            break;
        if ((desc.Type < K2EFI_MEMTYPE_COUNT) ||
            ((desc.Type >= 0x80000000) && (desc.Type <= K2OS_EFIMEMTYPE_LAST)))
            sTrackOneDescriptor(&desc);
    }

    //
    // all UEFI-reported physical address space ranges are tracked at the page level
    //
    status = Loader_AssignRuntimeVirtual();
    if (K2STAT_IS_ERROR(status))
        return status;

    //
    // freeing the memory map now could cause a change to the memory map
    // and we don't want to do that so we leave it alone
    //

    return 0;
}

