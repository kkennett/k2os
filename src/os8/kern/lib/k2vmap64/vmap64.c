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

#include <kern/lib/k2vmap64.h>
#include <lib/k2mem.h>

#if K2_TARGET_ARCH_IS_ARM

UINT64
K2VMAP64_ReadPML4E(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex
)
{
    return 0;
}

void
K2VMAP64_WritePML4E(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex,
    UINT64              aPML4E
)
{

}

UINT64
K2VMAP64_ReadPDPTE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex
)
{
    return 0;
}

void
K2VMAP64_WritePDPTE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex,
    UINT64              aPDPTE
)
{

}

UINT64
K2VMAP64_ReadPDE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex
    )
{
    return *(((UINT64 *)apContext->mTransBasePhys) + ((aIndex & 0x1FF) * 8));
}

void
K2VMAP64_WritePDE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex,
    UINT64              aPDE
    )
{
    UINT64 *pPDE;

    if (apContext->mFlags & K2VMAP64_FLAG_REALIZED)
        return;

    pPDE = (((UINT64 *)apContext->mTransBasePhys) + ((aIndex & 0x1FF) * 8));
    pPDE[0] = aPDE;
    pPDE[1] = (aPDE + 0x400);
    pPDE[2] = (aPDE + 0x800);
    pPDE[3] = (aPDE + 0xC00);
}

#else

UINT64
K2VMAP64_ReadPML4E(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex
)
{
    return *(((UINT64 *)apContext->mTransBasePhys) + (aIndex & 0x1FF));
}

void
K2VMAP64_WritePML4E(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex,
    UINT64              aPML4E
)
{
    if (apContext->mFlags & K2VMAP64_FLAG_REALIZED)
        return;
    if (aPML4E & K2VMAP64_FLAG_PRESENT)
    {
        K2_ASSERT((aPML4E & K2OS_MAPTYPE_KERN_PAGEDIR) == K2OS_MAPTYPE_KERN_PAGEDIR);
    }
    *(((UINT64 *)apContext->mTransBasePhys) + (aIndex & 0x1FF)) = aPML4E;
}

UINT64
K2VMAP64_ReadPDPTE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex
)
{
    return 0;
}

void
K2VMAP64_WritePDPTE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex,
    UINT64              aPDPTE
)
{

}

UINT64
K2VMAP64_ReadPDE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex
    )
{
    return 0;
}

void
K2VMAP64_WritePDE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aIndex,
    UINT64              aPDE
    )
{

}

#endif

K2STAT
K2VMAP64_Init(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aVirtMapBase,
    UINT64              aTransBasePhys,
    UINT64              aTransBaseVirt,
    BOOL                aIsMultiproc,
    pfK2VMAP64_PT_Alloc afPT_Alloc,
    pfK2VMAP64_PT_Free  afPT_Free
    )
{
    UINT64  virtWork;
    UINT64  physWork;
    UINT64  physEnd;
    K2STAT  status;

    if ((aTransBasePhys & (K2_VA64_TRANSTAB_SIZE - 1)) != 0)
        return K2STAT_ERROR_BAD_ARGUMENT;
    if ((aTransBaseVirt & K2_VA_MEMPAGE_OFFSET_MASK) != 0)
        return K2STAT_ERROR_BAD_ARGUMENT;
    if ((aVirtMapBase & K2_VA_MEMPAGE_OFFSET_MASK) != 0)
        return K2STAT_ERROR_BAD_ARGUMENT;

    if ((afPT_Alloc == NULL) ||
        (afPT_Free == NULL))
        return K2STAT_ERROR_BAD_ARGUMENT;

    apContext->mTransBasePhys = aTransBasePhys;
    apContext->mVirtMapBase = aVirtMapBase;
    if (aIsMultiproc)
        apContext->mFlags = K2VMAP64_FLAG_MULTIPROC;
    else
        apContext->mFlags = 0;
    apContext->PT_Alloc = afPT_Alloc;
    apContext->PT_Free = afPT_Free;

    virtWork = aTransBaseVirt;
    physWork = aTransBasePhys;
    physEnd = physWork + K2_VA64_TRANSTAB_SIZE;
    do
    {
        status = K2VMAP64_MapPage(apContext, virtWork, physWork, K2OS_MAPTYPE_KERN_PAGEDIR);
        if (K2STAT_IS_ERROR(status))
            return status;
        virtWork += K2_VA_MEMPAGE_BYTES;
        physWork += K2_VA_MEMPAGE_BYTES;
    } while (physWork != physEnd);

    return 0;
}

UINT64      
K2VMAP64_VirtToPTE(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aVirtAddr,
    BOOL *              apRetFaultPDE,
    BOOL *              apRetFaultPTE
    )
{
    UINT64  entry;
    UINT64  ixEntry;

    *apRetFaultPDE = TRUE;
    *apRetFaultPTE = TRUE;

    if (apContext->mFlags & K2VMAP64_FLAG_REALIZED)
        return 0;

    ixEntry = aVirtAddr / K2_VA64_PAGETABLE_MAP_BYTES;
    entry = K2VMAP64_ReadPDE(apContext, ixEntry);
    if ((entry & K2VMAP64_FLAG_PRESENT) == 0)
        return 0;

    *apRetFaultPDE = FALSE;

    ixEntry = (aVirtAddr / K2_VA_MEMPAGE_BYTES) & ((1 << K2_VA64_PAGETABLE_PAGES_POW2) - 1);
    entry = *((UINT64 *)((entry & K2VMAP64_PAGEPHYS_MASK) + (ixEntry * sizeof(UINT64))));
    if ((entry & K2VMAP64_FLAG_PRESENT) == 0)
        return 0;

    *apRetFaultPTE = FALSE;

    return entry;
}

K2STAT 
K2VMAP64_MapPage(
    K2VMAP64_CONTEXT *  apContext,
    UINT64              aVirtAddr,
    UINT64              aPhysAddr,
    UINT64              aPageMapAttr
    )
{
    UINT64      entry;
    UINT64      ixEntry;
    UINT64      virtPT;
    UINT64      physPT;
    UINT64 *    pPTE;
    K2STAT      status;

    if (apContext->mFlags & K2VMAP64_FLAG_REALIZED)
        return K2STAT_ERROR_API_ORDER;

    if ((aPageMapAttr & K2OS_MEMPAGE_ATTR_MASK) == K2OS_MEMPAGE_ATTR_NONE)
        return K2STAT_ERROR_BAD_ARGUMENT;

    ixEntry = aVirtAddr / K2_VA64_PML4_MAP_BYTES;
    entry = K2VMAP64_ReadPML4(apContext, ixEntry);
    if ((entry & K2VMAP64_FLAG_PRESENT) == 0)
    {
        virtPML4 = K2_VA64_TO_PML4_ADDR(apContext->mVirtMapBase, aVirtAddr);

0000000000000000000000000000000000000000000000000000000000000000
0000000000000111111111111111111111111111111111111111111111111111
and

0x0000800000000000

0x1000000000    == 64GB of virtmap space for kernel, 64GB of virtmap space per process 

0x2000000000    == 128GB of virtual map space per process

0xFFF8000000000000
0x0000001000000000

1111111111111000000000000000000000000000000000000000000000000000
1111111111111111111111111111111111111111111111111111111111111111



    ixEntry = aVirtAddr / K2_VA64_PAGETABLE_MAP_BYTES;
    entry = K2VMAP64_ReadPDE(apContext, ixEntry);
    if ((entry & K2VMAP64_FLAG_PRESENT) == 0)
    {
        virtPT = K2_VA64_TO_PT_ADDR(apContext->mVirtMapBase, aVirtAddr);
        if (virtPT != aVirtAddr)
        {
            status = apContext->PT_Alloc(&physPT);
            if (K2STAT_IS_ERROR(status))
                return status;

            K2MEM_Zero((void *)physPT, K2_VA_MEMPAGE_BYTES);

            status = K2VMAP64_MapPage(apContext, virtPT, physPT, K2OS_MAPTYPE_KERN_PAGETABLE);
            if (K2STAT_IS_ERROR(status))
            {
                apContext->PT_Free(physPT);
                return status;
            }
        }
        else
            physPT = aPhysAddr;

        entry = physPT | K2OS_MAPTYPE_KERN_PAGEDIR | K2VMAP64_FLAG_PRESENT;

        K2VMAP64_WritePDE(apContext, ixEntry, entry);
    }

    ixEntry = (aVirtAddr / K2_VA_MEMPAGE_BYTES) & ((1 << K2_VA64_PAGETABLE_PAGES_POW2) - 1);

    pPTE = (UINT64 *)((entry & K2VMAP64_PAGEPHYS_MASK) + (ixEntry * sizeof(UINT64)));
    if (((*pPTE) & K2VMAP64_FLAG_PRESENT) != 0)
        return K2STAT_ERROR_BAD_ARGUMENT;

    *pPTE = (aPhysAddr & K2VMAP64_PAGEPHYS_MASK) | aPageMapAttr | K2VMAP64_FLAG_PRESENT;

    return 0;
}

K2STAT
K2VMAP64_VerifyOrMapPageTableForAddr(
    K2VMAP64_CONTEXT *    apContext,
    UINT64              aVirtAddr
    )
{
    //
    // only creates a pagetable if one does not already exist for the specified address
    //
    UINT64  entry;
    UINT64  ixEntry;
    UINT64  physPT;
    K2STAT  status;

    if (apContext->mFlags & K2VMAP64_FLAG_REALIZED)
        return K2STAT_ERROR_API_ORDER;

    ixEntry = aVirtAddr / K2_VA64_PAGETABLE_MAP_BYTES;
    entry = K2VMAP64_ReadPDE(apContext, ixEntry);
    if ((entry & K2VMAP64_FLAG_PRESENT) != 0)
        return 0;

    status = apContext->PT_Alloc(&physPT);
    if (K2STAT_IS_ERROR(status))
        return status;

    K2MEM_Zero((void *)physPT, K2_VA_MEMPAGE_BYTES);

    aVirtAddr = K2_VA64_TO_PT_ADDR(apContext->mVirtMapBase, aVirtAddr);

    status = K2VMAP64_MapPage(apContext, aVirtAddr, physPT, K2OS_MAPTYPE_KERN_PAGETABLE);
    if (K2STAT_IS_ERROR(status))
    {
        apContext->PT_Free(physPT);
        return status;
    }

    entry = physPT | K2OS_MAPTYPE_KERN_PAGEDIR | K2VMAP64_FLAG_PRESENT;

    K2VMAP64_WritePDE(apContext, ixEntry, entry);

    return 0;
}

K2STAT
K2VMAP64_FindUnmappedVirtualPage(
    K2VMAP64_CONTEXT *  apContext,
    UINT64 *            apVirtAddr,
    UINT64              aVirtEnd
    )
{
    BOOL    faultPDE;
    BOOL    faultPTE;

    if (apContext->mFlags & K2VMAP64_FLAG_REALIZED)
        return K2STAT_ERROR_API_ORDER;

    *apVirtAddr = (*apVirtAddr) & K2_VA_PAGEFRAME_MASK;
    aVirtEnd &= K2_VA_PAGEFRAME_MASK;

    do
    {
        K2VMAP64_VirtToPTE(apContext, *apVirtAddr, &faultPDE, &faultPTE);
        if ((faultPDE) || (faultPTE))
            return 0;

        (*apVirtAddr) += K2_VA_MEMPAGE_BYTES;

    } while ((*apVirtAddr) != aVirtEnd);

    return K2STAT_ERROR_NOT_FOUND;
}

void
K2VMAP64_RealizeArchMappings(
    K2VMAP64_CONTEXT * apContext
)
{
    if (apContext->mFlags & K2VMAP64_FLAG_REALIZED)
        return;

    K2_ASSERT(0);

    apContext->mFlags |= K2VMAP64_FLAG_REALIZED;
}
