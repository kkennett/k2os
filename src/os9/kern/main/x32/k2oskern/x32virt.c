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

#include "x32kern.h"

void 
KernArch_VirtInit(void)
{
    UINT32 *            pPDE;
    UINT32 *            pPTE;
    UINT32              pdeLeft;
    UINT32              pteLeft;
    UINT32              virtAddr;
    UINT32              workAddr;
    UINT32 *            pPtPTE;
    K2OSKERN_PHYSTRACK *pTrack;
    UINT32              pte;
    UINT32              pde;

    //
    // clean out any mappings under kernel base address
    //

    pdeLeft = K2_VA32_PAGETABLES_FOR_2G;
    pPDE = (UINT32 *)K2OS_KVA_TRANSTAB_BASE;
    virtAddr = 0;
    do
    {
        pde = *pPDE;
        if (0 != pde)
        {
            workAddr = virtAddr;
            pPTE = (UINT32 *)K2OS_KVA_TO_PTE_ADDR(workAddr);
            pPtPTE = (UINT32 *)K2OS_KVA_TO_PTE_ADDR((UINT32)pPTE);
            pteLeft = K2_VA32_ENTRIES_PER_PAGETABLE;
            do
            {
                pte = *pPTE;
                if (0 != pte)
                {
                    *pPTE = 0;  // KernPte_BreakPageMap does checks we dont want it to
                    pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(pte & K2_VA_PAGEFRAME_MASK);
                    K2_ASSERT(0 != pTrack->Flags.Field.Exists);
                    K2_ASSERT(0 == pTrack->Flags.Field.Free);
                    K2_ASSERT(pTrack->Flags.Field.BlockSize == K2_VA_MEMPAGE_BYTES_POW2);
                    X32_TLBInvalidatePage(workAddr);
                    KernPhys_FreeTrack(pTrack);
                }
                pPTE++;
                workAddr += K2_VA_MEMPAGE_BYTES;
            } while (--pteLeft);

            pte = *pPtPTE;
            K2_ASSERT((pde & K2_VA_PAGEFRAME_MASK) == (pte & K2_VA_PAGEFRAME_MASK));
            *pPtPTE = 0;

            pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(pte & K2_VA_PAGEFRAME_MASK);
            K2_ASSERT(0 != pTrack->Flags.Field.Exists);
            K2_ASSERT(0 == pTrack->Flags.Field.Free);
            K2_ASSERT(pTrack->Flags.Field.BlockSize == K2_VA_MEMPAGE_BYTES_POW2);
            KernPhys_FreeTrack(pTrack);

            *pPDE = 0;
            X32_TLBInvalidatePage(virtAddr);
        }

        virtAddr += K2_VA32_PAGETABLE_MAP_BYTES;
        pPDE++;
    } while (--pdeLeft);
}

void 
KernArch_InstallPageTable(
    K2OSKERN_OBJ_PROCESS *  apProc, 
    UINT32                  aVirtAddrPtMaps, 
    UINT32                  aPhysPageAddr
)
{
    UINT32                  ptIndex;
    UINT32                  virtKernPT;
    UINT32 *                pPDE;
    UINT32                  pde;
    UINT32                  mapAttr;
    BOOL                    disp;
    K2OSKERN_OBJ_PROCESS *  pOtherProc;
    K2LIST_LINK *           pListLink;

    K2_ASSERT(0 == (aVirtAddrPtMaps & (K2_VA32_PAGETABLE_MAP_BYTES - 1)));

    ptIndex = aVirtAddrPtMaps / K2_VA32_PAGETABLE_MAP_BYTES;
    K2_ASSERT(ptIndex < 0x400);

    if (ptIndex >= K2_VA32_PAGETABLES_FOR_2G)
    {
        //
        // pagetable maps kernel space
        //
        K2_ASSERT(apProc == NULL);
        mapAttr = X32_KERN_PAGETABLE_PROTO;
        virtKernPT = K2OS_KVA_TO_PT_ADDR(aVirtAddrPtMaps);
    }
    else
    {
        //
        // pagetable maps user mode
        //
        K2_ASSERT(apProc != NULL);
        mapAttr = X32_USER_PAGETABLE_PROTO;
        virtKernPT = K2_VA32_TO_PT_ADDR(apProc->mVirtMapKVA, aVirtAddrPtMaps);
    }

    //
    // pagetable virtual address will always be in kernel space (virtKernPT)
    // because that's where process KVAs live
    //

    K2_ASSERT(((*((UINT32 *)K2OS_KVA_TO_PTE_ADDR(virtKernPT))) & K2OSKERN_PTE_PRESENT_BIT) == 0);
    
    KernPte_MakePageMap(NULL, virtKernPT, aPhysPageAddr, K2OS_MAPTYPE_KERN_PAGETABLE);
    K2MEM_Zero((void *)virtKernPT, K2_VA_MEMPAGE_BYTES);
    
    //
    // now install the PDE that points to this new pagetable
    //
    pde = (aPhysPageAddr & K2_VA_PAGEFRAME_MASK) | mapAttr;

    // these should be optimized out by the compiler as they are not variable comparisons
    if (K2OS_MAPTYPE_KERN_PAGEDIR & K2OS_MEMPAGE_ATTR_UNCACHED)
        pde |= X32_PDE_CACHEDISABLE;

    if (K2OS_MAPTYPE_KERN_PAGEDIR & K2OS_MEMPAGE_ATTR_WRITE_THRU)
        pde |= X32_PDE_WRITETHROUGH;

    if (ptIndex >= K2_VA32_PAGETABLES_FOR_2G)
    {
        disp = K2OSKERN_SeqLock(&gData.Proc.SeqLock);

        pPDE = (((UINT32 *)K2OS_KVA_TRANSTAB_BASE) + ptIndex);

        K2_ASSERT(((*pPDE) & K2OSKERN_PDE_PRESENT_BIT) == 0);

        *pPDE = pde;

        //
        // pde is for the kernel, so also goes into all process pagetables
        //
        pListLink = gData.Proc.List.mpHead;
        if (NULL != pListLink)
        {
            do
            {
                pOtherProc = K2_GET_CONTAINER(K2OSKERN_OBJ_PROCESS, pListLink, GlobalProcListLink);
                pListLink = pListLink->mpNext;

                pPDE = (((UINT32 *)pOtherProc->mVirtTransBase) + ptIndex);
                K2_ASSERT(((*pPDE) & K2OSKERN_PDE_PRESENT_BIT) == 0);
                *pPDE = pde;

            } while (pListLink != NULL);
        }

        K2OSKERN_SeqUnlock(&gData.Proc.SeqLock, disp);
    }
    else
    {
        pPDE = (((UINT32 *)apProc->mVirtTransBase) + (ptIndex & 0x3FF));

        K2_ASSERT(((*pPDE) & K2OSKERN_PDE_PRESENT_BIT) == 0);

        *pPDE = pde;
    }
}
