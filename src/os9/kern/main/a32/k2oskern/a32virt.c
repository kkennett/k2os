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

#include "a32kern.h"

void A32KernAsm_UndExceptionVector(void);
void A32KernAsm_SvcExceptionVector(void);
void A32KernAsm_PrefetchAbortExceptionVector(void);
void A32KernAsm_DataAbortExceptionVector(void);
void A32KernAsm_IRQExceptionVector(void);
void A32KernAsm_FIQExceptionVector(void);

void 
KernArch_VirtInit(void)
{
    A32_TTBEQUAD *      pQuad;
    UINT32 *            pPTE;
    UINT32              quadLeft;
    UINT32              pteLeft;
    UINT32              virtAddr;
    UINT32              workAddr;
    UINT32 *            pPtPTE;
    K2OSKERN_PHYSTRACK *pTrack;
    UINT32              pte;
    UINT32              ttQuad;
    K2OSKERN_PHYSRES    res;
    BOOL                ok;
    K2STAT              stat;
    K2LIST_ANCHOR       trackList;
    UINT32 *            pOut;
    UINT32              offset;
    UINT32              val32;

    //
    // clean out any mappings under kernel base address
    //
    quadLeft = K2_VA32_PAGETABLES_FOR_2G;
    pQuad = (A32_TTBEQUAD *)K2OS_KVA_TRANSTAB_BASE;
    virtAddr = 0;
    do
    {
        ttQuad = pQuad->Quad[0].mAsUINT32;
        if (0 != ttQuad)
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
                    A32_TLBInvalidateMVA_MP_AllASID(workAddr);
                    KernPhys_FreeTrack(pTrack);
                }
                pPTE++;
                workAddr += K2_VA_MEMPAGE_BYTES;
            } while (--pteLeft);

            pte = *pPtPTE;
            K2_ASSERT((ttQuad & K2_VA_PAGEFRAME_MASK) == (pte & K2_VA_PAGEFRAME_MASK));
            *pPtPTE = 0;
            K2_CpuWriteBarrier();

            pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(pte & K2_VA_PAGEFRAME_MASK);
            K2_ASSERT(0 != pTrack->Flags.Field.Exists);
            K2_ASSERT(0 == pTrack->Flags.Field.Free);
            K2_ASSERT(pTrack->Flags.Field.BlockSize == K2_VA_MEMPAGE_BYTES_POW2);
            KernPhys_FreeTrack(pTrack);

            pQuad->Quad[0].mAsUINT32 = 0;
            pQuad->Quad[1].mAsUINT32 = 0;
            pQuad->Quad[2].mAsUINT32 = 0;
            pQuad->Quad[3].mAsUINT32 = 0;
            K2OS_CacheOperation(K2OS_CACHEOP_FlushDataRange, (UINT32)pQuad, sizeof(A32_TTBEQUAD));
            A32_TLBInvalidateMVA_MP_AllASID(virtAddr);
            A32_TLBInvalidateMVA_MP_AllASID(virtAddr + (K2_VA32_PAGETABLE_MAP_BYTES / 4));
            A32_TLBInvalidateMVA_MP_AllASID(virtAddr + (K2_VA32_PAGETABLE_MAP_BYTES / 2));
            A32_TLBInvalidateMVA_MP_AllASID(virtAddr + (K2_VA32_PAGETABLE_MAP_BYTES / 2) + (K2_VA32_PAGETABLE_MAP_BYTES / 4));
        }

        virtAddr += K2_VA32_PAGETABLE_MAP_BYTES;
        pQuad++;
    } while (--quadLeft);

    //
    // get physical memory for vector page
    //
    ok = KernPhys_Reserve_Init(&res, 1);
    K2_ASSERT(ok);
    stat = KernPhys_AllocSparsePages(&res, 1, &trackList);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));
    pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, trackList.mpHead, ListLink);
    K2LIST_AddAtTail(&gData.Phys.PageList[KernPhysPageList_Overhead], &pTrack->ListLink);
    gA32Kern_VectorPagePhys = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);

    //
    // get physical memory for start area. 
    //
    ok = KernPhys_Reserve_Init(&res, K2OS_A32_APSTART_SPACE_PHYS_PAGECOUNT);
    K2_ASSERT(ok);
    stat = KernPhys_AllocPow2Bytes(&res, 4 * K2_VA_MEMPAGE_BYTES, &gpA32Kern_StartTtbPhysTrack);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));
    // TTB block must be 16KB aligned; which it will be since 
    // our physical allocation mechanism allocates aligned blocks based on their size
    K2_ASSERT(0 == (0x3FFF & K2OS_PHYSTRACK_TO_PHYS32((UINT32)gpA32Kern_StartTtbPhysTrack)));
    stat = KernPhys_AllocSparsePages(&res, K2OS_A32_APSTART_SPACE_PHYS_PAGECOUNT - 4, &gA32Kern_StartAreaPhysList);
    K2_ASSERT(!K2STAT_IS_ERROR(stat));
    KernPhys_CutTrackListIntoPages(&gA32Kern_StartAreaPhysList, FALSE, NULL, FALSE, 0);

    //
    // install vector page
    //
    K2_ASSERT(A32_HIGH_VECTORS_ADDRESS == K2OS_KVA_ARCHSPEC_BASE);
    K2_ASSERT(0 != gA32Kern_VectorPagePhys);
    KernPte_MakePageMap(NULL, K2OS_KVA_ARCHSPEC_BASE, gA32Kern_VectorPagePhys, K2OS_MAPTYPE_KERN_DATA);

    val32 = K2OS_KVA_ARCHSPEC_BASE + 8;  /* actual PC at point of jump */
    pOut = (UINT32 *)K2OS_KVA_ARCHSPEC_BASE;

    // reset vector goes to infinite loop (jump to yourself)
    *pOut = 0xEAFFFFFE;
    //    K2OSKERN_Debug("[%08X] = %08X\n", pOut, *pOut);
    pOut++;
    val32 += 4;

    offset = ((UINT32)A32KernAsm_UndExceptionVector) - val32;
    K2_ASSERT((((INT32)offset) >= -33554432) && (((INT32)offset) <= 33554428)); // encoding A1 - page A8-333 in ARM DDI 0406C.d
    *pOut = 0xEA000000 + (0xFFFFFF & (offset >> 2));
    //    K2OSKERN_Debug("[%08X] = %08X\n", pOut, *pOut);
    pOut++;
    val32 += 4;

    offset = ((UINT32)A32KernAsm_SvcExceptionVector) - val32;
    K2_ASSERT((((INT32)offset) >= -33554432) && (((INT32)offset) <= 33554428)); // encoding A1 - page A8-333 in ARM DDI 0406C.d
    *pOut = 0xEA000000 + (0xFFFFFF & (offset >> 2));
    //    K2OSKERN_Debug("[%08X] = %08X\n", pOut, *pOut);
    pOut++;
    val32 += 4;

    offset = ((UINT32)A32KernAsm_PrefetchAbortExceptionVector) - val32;
    K2_ASSERT((((INT32)offset) >= -33554432) && (((INT32)offset) <= 33554428)); // encoding A1 - page A8-333 in ARM DDI 0406C.d
    *pOut = 0xEA000000 + (0xFFFFFF & (offset >> 2));
    //    K2OSKERN_Debug("[%08X] = %08X\n", pOut, *pOut);
    pOut++;
    val32 += 4;

    offset = ((UINT32)A32KernAsm_DataAbortExceptionVector) - val32;
    K2_ASSERT((((INT32)offset) >= -33554432) && (((INT32)offset) <= 33554428)); // encoding A1 - page A8-333 in ARM DDI 0406C.d
    *pOut = 0xEA000000 + (0xFFFFFF & (offset >> 2));
    //    K2OSKERN_Debug("[%08X] = %08X\n", pOut, *pOut);
    pOut++;
    val32 += 4;

    *pOut = 0xEAFFFFFE;
    //    K2OSKERN_Debug("[%08X] = %08X\n", pOut, *pOut);
    pOut++;
    val32 += 4;

    offset = ((UINT32)A32KernAsm_IRQExceptionVector) - val32;
    K2_ASSERT((((INT32)offset) >= -33554432) && (((INT32)offset) <= 33554428)); // encoding A1 - page A8-333 in ARM DDI 0406C.d
    *pOut = 0xEA000000 + (0xFFFFFF & (offset >> 2));
    //    K2OSKERN_Debug("[%08X] = %08X\n", pOut, *pOut);
    pOut++;
    val32 += 4;

    offset = ((UINT32)A32KernAsm_FIQExceptionVector) - val32;
    K2_ASSERT((((INT32)offset) >= -33554432) && (((INT32)offset) <= 33554428)); // encoding A1 - page A8-333 in ARM DDI 0406C.d
    *pOut = 0xEA000000 + (0xFFFFFF & (offset >> 2));
    //    K2OSKERN_Debug("[%08X] = %08X\n", pOut, *pOut);
    pOut++;

    //
    // flush the vector table
    //
    val32 = sizeof(UINT32) * ((UINT32)(pOut - ((UINT32*)K2OS_KVA_ARCHSPEC_BASE)));
    K2OS_CacheOperation(K2OS_CACHEOP_FlushDataRange, K2OS_KVA_ARCHSPEC_BASE, val32);

    // 
    // break the mapping
    //
    KernPte_BreakPageMap(NULL, K2OS_KVA_ARCHSPEC_BASE, 0);

    //
    // re-make the mapping as code
    //
    KernPte_MakePageMap(NULL, K2OS_KVA_ARCHSPEC_BASE, gA32Kern_VectorPagePhys, K2OS_MAPTYPE_KERN_TEXT);
    K2OS_CacheOperation(K2OS_CACHEOP_InvalidateInstructionRange, K2OS_KVA_ARCHSPEC_BASE, val32);

    //
    // verify high vectors are on
    //
    val32 = A32_ReadSCTRL();
    if (0 == (val32 & A32_SCTRL_V_HIGHVECTORS))
    {
        val32 |= A32_SCTRL_V_HIGHVECTORS;
        A32_WriteSCTRL(val32);
        A32_DSB();
        A32_ISB();
    }
}

void 
KernArch_InstallPageTable(
    K2OSKERN_OBJ_PROCESS *  apProc, 
    UINT32                  aVirtAddrPtMaps, 
    UINT32                  aPhysPageAddr
)
{
    A32_TTBEQUAD *  pQuad;
    UINT32          ptIndex;
    UINT32          virtKernPT;
    UINT32          ttQuad;
    BOOL            disp;

    K2_ASSERT(0 == (aVirtAddrPtMaps & (K2_VA32_PAGETABLE_MAP_BYTES - 1)));

    ptIndex = aVirtAddrPtMaps / K2_VA32_PAGETABLE_MAP_BYTES;
    K2_ASSERT(ptIndex < 0x400);

    ttQuad = A32_TTBE_PAGETABLE_PROTO | (aPhysPageAddr & K2_VA_PAGEFRAME_MASK);

    if (ptIndex >= K2_VA32_PAGETABLES_FOR_2G)
    {
        //
        // pagetable maps kernel space
        //
        K2_ASSERT(apProc == NULL);
        virtKernPT = K2OS_KVA_TO_PT_ADDR(aVirtAddrPtMaps);
    }
    else
    {
        //
        // pagetable maps user mode
        //
        K2_ASSERT(apProc != NULL);
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
    if (ptIndex >= K2_VA32_PAGETABLES_FOR_2G)
    {
        disp = K2OSKERN_SeqLock(&gData.Proc.SeqLock);
        pQuad = ((A32_TTBEQUAD *)K2OS_KVA_TRANSTAB_BASE) + ptIndex;
    }
    else
    {
        pQuad = ((A32_TTBEQUAD *)apProc->mVirtTransBase) + ptIndex;
    }

    K2_ASSERT((pQuad->Quad[0].mAsUINT32 & K2OSKERN_PDE_PRESENT_BIT) == 0);

    pQuad->Quad[0].mAsUINT32 = ttQuad;
    pQuad->Quad[1].mAsUINT32 = ttQuad | 0x400;
    pQuad->Quad[2].mAsUINT32 = ttQuad | 0x800;
    pQuad->Quad[3].mAsUINT32 = ttQuad | 0xC00;
    K2_CpuWriteBarrier();

    K2OS_CacheOperation(K2OS_CACHEOP_FlushDataRange, (UINT32)pQuad, sizeof(A32_TTBEQUAD));

    if (ptIndex >= K2_VA32_PAGETABLES_FOR_2G)
    {
        K2OSKERN_SeqUnlock(&gData.Proc.SeqLock, disp);
    }
}
