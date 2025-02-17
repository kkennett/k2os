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

void A32KernAsm_StackBridge(UINT32 aCpuIx, UINT32 aCoreMemStacksBase);

struct _A32AUXCPU_STARTDATA
{
    UINT32  mReg_DACR;
    UINT32  mReg_TTBR;
    UINT32  mReg_COPROC;
    UINT32  mReg_CONTROL;
    UINT32  mVirtContinue;
    UINT32  mReserved[3];
};
typedef struct _A32AUXCPU_STARTDATA A32AUXCPU_STARTDATA;

void A32KernAsm_AuxCpuStart(void);
void A32KernAsm_AuxCpuStart_END(void);

void    
KernArch_SetCoreToProcess(
    K2OSKERN_CPUCORE volatile * apThisCore, 
    K2OSKERN_OBJ_PROCESS *      apNewProc
) 
{
    BOOL                    disp;
    K2OSKERN_OBJ_PROCESS *  pCurProc;

    pCurProc = apThisCore->MappedProcRef.AsProc;
    if (apNewProc == pCurProc)
        return;

    disp = K2OSKERN_SetIntr(FALSE);

    if (NULL != pCurProc)
    {
        A32_TLBInvalidateASID_UP(pCurProc->mId);
    }

    if (NULL == apNewProc)
    {
        //
        // cannot have an active thread because FS target (holding current thread global ix)
        // lives in user space.  Any interrupt with current thread set will cause a check
        // whether the global ix is set correclty.  If this is set non-null it will cause
        // a recursive segment fault inside the interrupt handler.
        //
        K2_ASSERT(apThisCore->mpActiveThread == NULL);
        A32_WriteTTBR0(A32_ReadTTBR1());
        A32_WriteCONTEXT(0);
    }
    else
    {
        A32_WriteTTBR0((A32_ReadTTBR1() & ~A32_TTBASE_PHYS_MASK) | apNewProc->mPhysTransBase);
        A32_WriteCONTEXT(apNewProc->mId);
    }

    K2OSKERN_SetIntr(disp);

    if (NULL != pCurProc)
    {
        KernObj_ReleaseRef((K2OSKERN_OBJREF *)&apThisCore->MappedProcRef);
    }
    if (NULL != apNewProc)
    {
        KernObj_CreateRef((K2OSKERN_OBJREF *)&apThisCore->MappedProcRef, &apNewProc->Hdr);
    }
}

void A32Kern_CpuLaunch2(UINT32 aCpuIx, UINT32 aSysStackPtr)
{
    A32_REG_SCTRL                   sctrl;
    UINT32                          v;
    K2OSKERN_COREMEMORY volatile *  pThisCoreMem;

    pThisCoreMem = K2OSKERN_COREIX_TO_COREMEMORY(aCpuIx);

    K2_ASSERT(pThisCoreMem->CpuCore.mCoreIx == aCpuIx);

    //
    // set up SCTRL
    //
    sctrl.mAsUINT32 = A32_ReadSCTRL();
    sctrl.Bits.mAFE = 0;    // afe off 
    sctrl.Bits.mTRE = 0;    // tex remap off
    sctrl.Bits.mV = 1;      // high vectors
    A32_WriteSCTRL(sctrl.mAsUINT32);
    A32_DSB();
    A32_ISB();

    //
    // flush cache before switching TTB
    //
    K2OS_CacheOperation(K2OS_CACHEOP_FlushDataAll, 0, 0);

    //
    // switch over to main TTB
    //
    A32_TLBInvalidateAll_UP();
    v = A32_ReadTTBR1();    // everything except address of table will be correct
    v = (v & ~A32_TTBASE_PHYS_MASK) | gData.mpShared->LoadInfo.mTransBasePhys;
    A32_WriteTTBR0(v);
    A32_WriteTTBR1(v);
    A32_WriteTTBCR(1);  // use 0x80000000 sized TTBR0
    A32_ISB();
    A32_TLBInvalidateAll_UP();

    //
    // flush cache again after switch
    //
    K2OS_CacheOperation(K2OS_CACHEOP_FlushDataAll, 0, 0);

//    K2OSKERN_Debug("Cpu%d off startup goo now\n", aCpuIx);

    A32Kern_IntrInitGicPerCore();

    // ensure private timer disabled
    A32Kern_IntrSetEnable(A32_MP_PTIMERS_IRQ, FALSE);
    MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_CONTROL, 0);
    do
    {
        v = MMREG_READ32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_INTSTATUS);
        if (0 == v)
            break;
        MMREG_WRITE32(A32KERN_MP_PRIVATE_TIMERS_VIRT, A32_PERIF_PTIMERS_OFFSET_INTSTATUS, v);
    } while (1);

#if 0
    K2OSKERN_Debug("-CPU %d--(sys@ %08X)-----------\n", pThisCoreMem->CpuCore.mCoreIx, aSysStackPtr);
    K2OSKERN_Debug("STACK = %08X\n", A32_ReadStackPointer());
    K2OSKERN_Debug("SCTLR = %08X\n", A32_ReadSCTRL());
    K2OSKERN_Debug("ACTLR = %08X\n", A32_ReadAUXCTRL());
    K2OSKERN_Debug("SCU   = %08X\n", *((UINT32 *)K2OS_KVA_A32_PERIPHBASE));
//    K2OSKERN_Debug("SCR   = %08X\n", A32_ReadSCR());    // will fault in NS mode
    K2OSKERN_Debug("DACR  = %08X\n", A32_ReadDACR());
    K2OSKERN_Debug("TTBCR = %08X\n", A32_ReadTTBCR());
    K2OSKERN_Debug("TTBR0 = %08X\n", A32_ReadTTBR0());
    K2OSKERN_Debug("TTBR1 = %08X\n", A32_ReadTTBR1());
    K2OSKERN_Debug("--------------------\n");

    v = ((UINT32)pThisCoreMem) + ((K2_VA_MEMPAGE_BYTES * 4) - 4);
    K2OSKERN_Debug("CPU %d Started. EntryStack @ %08X\n", pThisCoreMem->CpuCore.mCoreIx, v);
#endif

    pThisCoreMem->CpuCore.mIsExecuting = TRUE;
    K2_CpuWriteBarrier();

    if (aCpuIx == 0)
    {
        gData.mCore0MonitorStarted = TRUE;
        K2OS_CacheOperation(K2OS_CACHEOP_FlushDataAll, 0, 0);
        K2_CpuWriteBarrier();
    }
    pThisCoreMem->CpuCore.mIsInMonitor = TRUE;
    K2_CpuWriteBarrier();

//    K2OSKERN_Debug("Core %d resume in monitor...\n", aCpuIx);
    A32KernAsm_ResumeInMonitor(aSysStackPtr);
    
    //
    // should never return here
    //

    K2OSKERN_Panic("*** A32Kern_CpuLaunc2 returned\n");
    while (1);
}

void A32Kern_CpuLaunch1(UINT32 aCpuIx)
{
    K2OSKERN_COREMEMORY volatile * pThisCorePage;
    pThisCorePage = K2OSKERN_COREIX_TO_COREMEMORY(aCpuIx);
    A32KernAsm_StackBridge(aCpuIx, (((UINT32)&pThisCorePage->mStacks) + 4) & ~3);
}

static void sStartAuxCpu(UINT32 aCpuIx, UINT32 transitPhys)
{
    UINT32                          physAddr;
    ACPI_MADT_SUB_GIC volatile *    pSubGic;

    pSubGic = gpA32Kern_MADT_GICC[aCpuIx];
    physAddr = (UINT32)pSubGic->ParkedAddress;

//    K2OSKERN_Debug("Park page core %d @ Phys %08X\n", aCpuIx, physAddr);

    //
    // map the parking page for this processor
    //
    KernPte_MakePageMap(NULL, A32KERN_APSTART_PARKPAGE_VIRT, physAddr & K2_VA_PAGEFRAME_MASK, K2OS_MAPTYPE_KERN_UNCACHED);

    //
    // tell CPU aCpuIx to jump to transit page + sizeof(A32AUXCPU_STARTDATA)
    //
    *((UINT32 *)A32KERN_APSTART_PARKPAGE_VIRT) = transitPhys + sizeof(A32AUXCPU_STARTDATA);
    K2_CpuWriteBarrier();

    KernPte_BreakPageMap(NULL, A32KERN_APSTART_PARKPAGE_VIRT, 0);
    KernArch_InvalidateTlbPageOnCurrentCore(A32KERN_APSTART_PARKPAGE_VIRT);
 
//    K2OSKERN_Debug("Sending SGI...\n");
    //
    // this fires the SGI that wakes up the aux core 
    //
    A32_ISB();
    MMREG_WRITE32(A32KERN_MP_GICD_VIRT, A32_PERIF_GICD_OFFSET_ICDSGIR, (1 << (aCpuIx + 16)));
}

void
KernArch_LaunchCpuCores(void)
{
    A32_TRANSTBL *                  pTTB;
    A32_PAGETABLE *                 pPageTable;
    UINT32                          workVal;
    UINT32                          ttbPhys;
    UINT32                          ptPhys;
    UINT32                          transitPhys;
    A32_TTBEQUAD *                  pQuad;
    A32AUXCPU_STARTDATA *           pStartData;
    K2OSKERN_COREMEMORY volatile *  pAuxCoreMem;
    K2OSKERN_PHYSTRACK *            pTrack;

    A32Kern_StartTime();

    if (gData.mCpuCoreCount > 1)
    {
        //
        // vector page has been installed already
        //

        //
        // translation table base
        //
        ttbPhys = K2OS_PHYSTRACK_TO_PHYS32((UINT32)gpA32Kern_StartTtbPhysTrack);
        K2_ASSERT(0 == (0x3FFF & ttbPhys));
        KernPte_MakePageMap(NULL, A32KERN_APSTART_TTB_VIRT + (0 * K2_VA_MEMPAGE_BYTES), ttbPhys + 0x0000, K2OS_MAPTYPE_KERN_PAGEDIR);
        KernPte_MakePageMap(NULL, A32KERN_APSTART_TTB_VIRT + (1 * K2_VA_MEMPAGE_BYTES), ttbPhys + 0x1000, K2OS_MAPTYPE_KERN_PAGEDIR);
        KernPte_MakePageMap(NULL, A32KERN_APSTART_TTB_VIRT + (2 * K2_VA_MEMPAGE_BYTES), ttbPhys + 0x2000, K2OS_MAPTYPE_KERN_PAGEDIR);
        KernPte_MakePageMap(NULL, A32KERN_APSTART_TTB_VIRT + (3 * K2_VA_MEMPAGE_BYTES), ttbPhys + 0x3000, K2OS_MAPTYPE_KERN_PAGEDIR);
        pTTB = (A32_TRANSTBL *)A32KERN_APSTART_TTB_VIRT;
        K2MEM_Zero(pTTB, sizeof(A32_TRANSTBL));

        K2_ASSERT(0 != gA32Kern_StartAreaPhysList.mNodeCount);
        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, gA32Kern_StartAreaPhysList.mpHead, ListLink);
        K2LIST_Remove(&gA32Kern_StartAreaPhysList, &pTrack->ListLink);
        ptPhys = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);
        KernPte_MakePageMap(NULL, A32KERN_APSTART_PAGETABLE_VIRT, ptPhys, K2OS_MAPTYPE_KERN_PAGETABLE);
        pPageTable = (A32_PAGETABLE *)A32KERN_APSTART_PAGETABLE_VIRT;
        K2MEM_Zero(pPageTable, sizeof(A32_PAGETABLE));

        K2_ASSERT(0 != gA32Kern_StartAreaPhysList.mNodeCount);
        pTrack = K2_GET_CONTAINER(K2OSKERN_PHYSTRACK, gA32Kern_StartAreaPhysList.mpHead, ListLink);
        K2LIST_Remove(&gA32Kern_StartAreaPhysList, &pTrack->ListLink);
        transitPhys = K2OS_PHYSTRACK_TO_PHYS32((UINT32)pTrack);
        KernPte_MakePageMap(NULL, A32KERN_APSTART_TRANSIT_PAGE_VIRT, transitPhys, K2OS_MAPTYPE_KERN_DATA);
        K2MEM_Zero((void *)A32KERN_APSTART_TRANSIT_PAGE_VIRT, K2_VA_MEMPAGE_BYTES);

        // 
        // put the pagetable for the transition page into the AUX TTB
        //
        workVal = (transitPhys >> K2_VA32_PAGETABLE_MAP_BYTES_POW2) & (K2_VA32_PAGETABLES_FOR_4G - 1);
        pQuad = &pTTB->QuadEntry[workVal];
        workVal = A32_TTBE_PAGETABLE_PROTO | ptPhys;
        pQuad->Quad[0].mAsUINT32 = workVal;
        pQuad->Quad[1].mAsUINT32 = workVal + 0x400;
        pQuad->Quad[2].mAsUINT32 = workVal + 0x800;
        pQuad->Quad[3].mAsUINT32 = workVal + 0xC00;

        //
        // put the transition page into the aux pagetable
        //
        workVal = (transitPhys >> K2_VA_MEMPAGE_BYTES_POW2) & (K2_VA32_ENTRIES_PER_PAGETABLE - 1);
        pPageTable->Entry[workVal].mAsUINT32 =
            transitPhys |
            A32_PTE_PRESENT | A32_MMU_PTE_REGIONTYPE_UNCACHED | A32_MMU_PTE_PERMIT_KERN_RW_USER_NONE;

        //
        // put all pagetables for kernel into the TTB as well
        // this copy is why the transition page can't be in the high 2GB
        //
        K2MEM_Copy(
            &pTTB->QuadEntry[K2_VA32_PAGETABLES_FOR_4G / 2],
            (void *)(K2OS_KVA_TRANSTAB_BASE + 0x2000),
            0x2000
        );

        workVal = ((UINT32)A32KernAsm_AuxCpuStart_END) - ((UINT32)A32KernAsm_AuxCpuStart) + 4;
        K2MEM_Copy((void *)A32KERN_APSTART_TRANSIT_PAGE_VIRT, (void *)A32KernAsm_AuxCpuStart, workVal);

        //
        // copy start code to transition page
        //
        pStartData = (A32AUXCPU_STARTDATA *)A32KERN_APSTART_TRANSIT_PAGE_VIRT;
        pStartData->mReg_DACR = A32_ReadDACR();
        pStartData->mReg_TTBR = (A32_ReadTTBR1() & ~A32_TTBASE_PHYS_MASK) | ttbPhys;   // use AUX startup TTB not the master one
        pStartData->mReg_COPROC = A32_ReadCPACR();
        pStartData->mReg_CONTROL = A32_ReadSCTRL();
        pStartData->mVirtContinue = (UINT32)A32KernAsm_LaunchEntryPoint;

        //
        // flush the caches
        //
        K2OS_CacheOperation(K2OS_CACHEOP_FlushDataAll, 0, 0);

        //
        // unmap all the pages except the uncached page since we are done using them
        //
        KernPte_BreakPageMap(NULL, A32KERN_APSTART_TRANSIT_PAGE_VIRT, 0);
        KernArch_InvalidateTlbPageOnCurrentCore(A32KERN_APSTART_TRANSIT_PAGE_VIRT);
        KernPte_BreakPageMap(NULL, A32KERN_APSTART_PAGETABLE_VIRT, 0);
        KernArch_InvalidateTlbPageOnCurrentCore(A32KERN_APSTART_PAGETABLE_VIRT);
        KernPte_BreakPageMap(NULL, A32KERN_APSTART_TTB_VIRT + 0x0000, 0);
        KernArch_InvalidateTlbPageOnCurrentCore(A32KERN_APSTART_TTB_VIRT + 0x0000);
        KernPte_BreakPageMap(NULL, A32KERN_APSTART_TTB_VIRT + 0x1000, 0);
        KernArch_InvalidateTlbPageOnCurrentCore(A32KERN_APSTART_TTB_VIRT + 0x1000);
        KernPte_BreakPageMap(NULL, A32KERN_APSTART_TTB_VIRT + 0x2000, 0);
        KernArch_InvalidateTlbPageOnCurrentCore(A32KERN_APSTART_TTB_VIRT + 0x2000);
        KernPte_BreakPageMap(NULL, A32KERN_APSTART_TTB_VIRT + 0x3000, 0);
        KernArch_InvalidateTlbPageOnCurrentCore(A32KERN_APSTART_TTB_VIRT + 0x3000);

        //
        // go tell the CPUs to start up.  
        //
        for (workVal = 1; workVal < gData.mCpuCoreCount; workVal++)
        {
            pAuxCoreMem = K2OSKERN_COREIX_TO_COREMEMORY(workVal);

            K2_ASSERT(FALSE == pAuxCoreMem->CpuCore.mIsExecuting);

            sStartAuxCpu(workVal, transitPhys);

            while (FALSE == pAuxCoreMem->CpuCore.mIsExecuting);
        }

        //
        // we can free up the physical memory used to start the cores now
        //
        pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(transitPhys);
        K2LIST_AddAtTail(&gA32Kern_StartAreaPhysList, &pTrack->ListLink);
        pTrack = (K2OSKERN_PHYSTRACK *)K2OS_PHYS32_TO_PHYSTRACK(ptPhys);
        K2LIST_AddAtTail(&gA32Kern_StartAreaPhysList, &pTrack->ListLink);
        KernPhys_FreeTrackList(&gA32Kern_StartAreaPhysList);
        KernPhys_FreeTrack(gpA32Kern_StartTtbPhysTrack);
    }

    /* go to entry point for core 0 */
    A32KernAsm_LaunchEntryPoint(0);

    /* should never return */
    K2OSKERN_Panic("*** KernArch_LaunchCpuCores - returned\n");
    while (1);
}
