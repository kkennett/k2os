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

#include "x32kern.h"

typedef struct _StartArgs_1800
{
    UINT32                          mTransitionCR3;  /* assembly code relies on this being here */
    UINT32                          mSubCoreEntry;   /* assembly code relies on this being here */
    UINT32                          mCpuIx;
    pf_X32Kern_CpuLaunchEntryPoint  mKernelEntry;
} STARTARGS_1800;

UINT32 K2_CALLCONV_REGS 
K2OSKERN_GetCpuIndex(
    void
)
{
    if (!gX32Kern_ApicReady)
        return 0;
    return (MMREG_READ32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_ID) >> 24);
}

static UINT8 const sgCpuStartCode[] = {

    /* --- Real Mode --- */
    /*    1000:       fa                      cli                           */     0xfa,
    /*    1001:       66 31 c0                xor    eax,eax                */     0x66, 0x31, 0xc0,
    /*    1004:       66 31 db                xor    ebx,ebx                */     0x66, 0x31, 0xdb,
    /*    1007:       66 31 c9                xor    ecx,ecx                */     0x66, 0x31, 0xc9,
    /*    100a:       66 31 d2                xor    edx,edx                */     0x66, 0x31, 0xd2,
    /*    100d:       66 31 f6                xor    esi,esi                */     0x66, 0x31, 0xf6,
    /*    1010:       66 31 ff                xor    edi,edi                */     0x66, 0x31, 0xff,
    /*    1013:       66 31 ed                xor    ebp,ebp                */     0x66, 0x31, 0xed,
    /*    1016:       8e d8                   mov    ds,ax                  */     0x8e, 0xd8, 
    /*    1018:       8e c0                   mov    es,ax                  */     0x8e, 0xc0,
    /*    101a:       8e d0                   mov    ss,ax                  */     0x8e, 0xd0,
    /*    101c:       8e e0                   mov    fs,ax                  */     0x8e, 0xe0,
    /*    101e:       8e e8                   mov    gs,ax                  */     0x8e, 0xe8,
    /*    1020:       66 bc fc 1f 00 00       mov    esp,0x1ffc             */     0x66, 0xbc, 0xfc, 0x1f, 0x00, 0x00,
    /*    1026:       eb 08                   jmp    1030                   */     0xeb, 0x08,
    /*    1028:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    102c:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    1030:       ea 35 10 00 00          jmp    0x0:0x1035             */     0xea, 0x35, 0x10, 0x00, 0x00,
    /*    1035:       66 b8 10 11 00 00       mov    eax,0x1110             */     0x66, 0xb8, 0x10, 0x11, 0x00, 0x00,
    /*    103b:       67 66 2e 0f 01 10       lgdtd  cs:[eax]               */     0x67, 0x66, 0x2e, 0x0f, 0x01, 0x10,
    /*    1041:       e4 64                   in     al,0x64                */     0xe4, 0x64, 
    /*    1043:       a8 02                   test   al,0x2                 */     0xa8, 0x02,
    /*    1045:       75 fa                   jne    1041                   */     0x75, 0xfa,
    /*    1047:       b0 d1                   mov    al,0xd1                */     0xb0, 0xd1,
    /*    1049:       e6 64                   out    0x64,al                */     0xe6, 0x64,
    /*    104b:       e4 64                   in     al,0x64                */     0xe4, 0x64,
    /*    104d:       a8 02                   test   al,0x2                 */     0xa8, 0x02,
    /*    104f:       75 fa                   jne    104b                   */     0x75, 0xfa,
    /*    1051:       b0 df                   mov    al,0xdf                */     0xb0, 0xdf,
    /*    1053:       e6 60                   out    0x60,al                */     0xe6, 0x60,
    /*    1055:       0f 20 c0                mov    eax,cr0                */     0x0f, 0x20, 0xc0,
    /*    1058:       66 66 83 c8 01          data16 or eax,0x1             */     0x66, 0x66, 0x83, 0xc8, 0x01,
    /*    105d:       0f 22 c0                mov    cr0,eax                */     0x0f, 0x22, 0xc0,
    /*    1060:       66 ea 68 10 00 00 08    jmp    0x8:0x1068             */     0x66, 0xea, 0x68, 0x10, 0x00, 0x00, 0x08,
    /*    1067:       00                                                    */     0x00,
    /* --- Protected Mode --- */
    /*    1068:       31 c0                   xor    eax,eax                */     0x31, 0xc0,
    /*    106a:       66 b8 10 00             mov    ax,0x10                */     0x66, 0xb8, 0x10, 0x00,
    /*    106e:       66 8e d0                mov    ss,ax                  */     0x66, 0x8e, 0xd0,
    /*    1071:       66 8e d8                mov    ds,ax                  */     0x66, 0x8e, 0xd8,
    /*    1074:       66 8e c0                mov    es,ax                  */     0x66, 0x8e, 0xc0,
    /*    1077:       66 8e e0                mov    fs,ax                  */     0x66, 0x8e, 0xe0,
    /*    107a:       66 8e e8                mov    gs,ax                  */     0x66, 0x8e, 0xe8,
    /*    107d:       bb 90 10 00 00          mov    ebx,0x1090             */     0xbb, 0x90, 0x10, 0x00, 0x00,
    /*    1082:       0f 20 c0                mov    eax,cr0                */     0x0f, 0x20, 0xc0,
    /*    1085:       25 ff ff ff 9f          and    eax,0x9fffffff         */     0x25, 0xff, 0xff, 0xff, 0x9f,
    /*    108a:       0f 22 c0                mov    cr0,eax                */     0x0f, 0x22, 0xc0,
    /*    108d:       ff e3                   jmp    ebx                    */     0xff, 0xe3,
    /*    108f:       90                      nop                           */     0x90,
    /*    1090:       0f 09                   wbinvd                        */     0x0f, 0x09, 
    /*    1092:       bb 00 18 00 00          mov    ebx,0x1800             */     0xbb, 0x00, 0x18, 0x00, 0x00,
    /*    1097:       8b 03                   mov    eax,DWORD PTR [ebx]    */     0x8b, 0x03,
    /*    1099:       0f 22 d8                mov    cr3,eax                */     0x0f, 0x22, 0xd8,
    /*    109c:       0f 20 c0                mov    eax,cr0                */     0x0f, 0x20, 0xc0,
    /*    109f:       0d 00 00 00 80          or     eax,0x80000000         */     0x0d, 0x00, 0x00, 0x00, 0x80,
    /*    10a4:       0f 09                   wbinvd                        */     0x0f, 0x09,
    /*    10a6:       0f 22 c0                mov    cr0,eax                */     0x0f, 0x22, 0xc0,
    /*    10a9:       0f 09                   wbinvd                        */     0x0f, 0x09,
    /*    10ab:       ea e0 10 00 00 08 00    jmp    0x8:0x10e0             */     0xea, 0xe0, 0x10, 0x00, 0x00, 0x08, 0x00,
    /*    10b2:       90 90                   nops                          */     0x90, 0x90, 
    /*    10b4:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    10b8:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    10bc:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    10c0:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    10c4:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    10c8:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    10cc:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    10d0:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    10d4:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    10d8:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    10dc:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /*    10e0:       bb 04 18 00 00          mov    ebx,0x1804             */     0xbb, 0x04, 0x18, 0x00, 0x00,
    /*    10e5:       8b 03                   mov    eax,DWORD PTR [ebx]    */     0x8b, 0x03, 
    /*    10e7:       ff e0                   jmp    eax                    */     0xff, 0xe0, 
    /*    10e9:       90 90 90                nops                          */     0x90, 0x90, 0x90,
    /*    10ec:       90 90 90 90             nops                          */     0x90, 0x90, 0x90, 0x90,
    /* --- GDT Data --- */
    /*    10f0:       00 00 00 00 00 00 00 00      null descriptor            */   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*    10f8:       FF FF 00 00 00 9A CF 00      kernel code                */   0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00,
    /*    1100:       FF FF 00 00 00 92 CF 00      kernel data                */   0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00,
    /*    1108:       00 00 00 00 00 00 00 00      alignment                  */   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* --- GDT PTR --- */
    /*    1110:       17 00 f0 10 00 00            gdt pointer                */   0x17, 0x00, 0xf0, 0x10, 0x00, 0x00
};
K2_STATIC_ASSERT(sizeof(sgCpuStartCode) == 0x0116);

void K2_CALLCONV_REGS 
X32Kern_CpuLaunch(
    UINT32 aThisCpuCoreIndex
)
{
    K2OSKERN_COREMEMORY volatile *  pCoreMem;
    X32_TSS volatile *              pTSS;
    X32_GDTENTRY *                  pTSSEntry;
    UINT32                          stackPtr;

    /* we are on our own core stack */
    pCoreMem = (K2OSKERN_COREMEMORY volatile *)(K2OS_KVA_COREMEMORY_BASE + ((aThisCpuCoreIndex) * (4 * K2_VA_MEMPAGE_BYTES)));

    K2_ASSERT(pCoreMem->CpuCore.mCoreIx == aThisCpuCoreIndex);

    /* set up TSS selector in GDT */
    pTSS = &gX32Kern_TSS[aThisCpuCoreIndex].TSS;
    pTSSEntry = &gX32Kern_GDT[X32_SEGMENT_TSS0 + aThisCpuCoreIndex];
    pTSSEntry->mLimitLow16 = sizeof(X32KERN_TSS) - 1;
    pTSSEntry->mBaseLow16 = (UINT16)(((UINT32)pTSS) & 0xFFFF);
    pTSSEntry->mBaseMid8 = (UINT8)((((UINT32)pTSS) & 0xFF0000) >> 16);
    pTSSEntry->mAttrib = 0xE9;
    pTSSEntry->mLimitHigh4Attrib4 = 0;
    pTSSEntry->mBaseHigh8 = (UINT8)((((UINT32)pTSS) & 0xFF000000) >> 24);

    /* set up the TSS itself along with the proper ring 0 stack pointer address */
    stackPtr = ((UINT32)pCoreMem) + ((K2_VA_MEMPAGE_BYTES * 4) - 4);
    *((UINT32 *)stackPtr) = 0;
    stackPtr -= sizeof(UINT32);
    *((UINT32 *)stackPtr) = 0;

    X32Kern_TSSSetup(aThisCpuCoreIndex, (X32KERN_TSS *)pTSS, stackPtr);

    /* flush the GDT with the TSS entry changes and valid TSS it points to */
    X32Kern_GDTFlush();

    /* load the kernel LDT.  This never changes after init */
    X32_LoadLDT(X32_SEGMENT_SELECTOR_USER_LDT | X32_SELECTOR_RPL_USER);

    /* now load the task register. we never change this either after init */
    X32_LoadTR(X32_SEGMENT_SELECTOR_TSS(aThisCpuCoreIndex));

    /* set FS to point to our per-core value holding the current thread on that core */
    X32_SetFS((aThisCpuCoreIndex * X32_SIZEOF_GDTENTRY) | X32_SELECTOR_TI_LDT | X32_SELECTOR_RPL_USER);

    /* clean cache so we're ready to init the scheduler */
    X32_CacheFlushAll();
    K2_CpuFullBarrier();

    /* set up SYSENTER */
    X32_MSR_Write32(X32_MSRNUM_SYSENTER_CS, X32_SEGMENT_SELECTOR_KERNEL_CODE);
    X32_MSR_Write32(X32_MSRNUM_SYSENTER_EIP, (UINT32)X32Kern_SysEnter_Entry);
    X32_MSR_Write32(X32_MSRNUM_SYSENTER_ESP, stackPtr);

    pCoreMem->CpuCore.mIsExecuting = TRUE;
    K2_CpuWriteBarrier();

    if (aThisCpuCoreIndex == 0)
    {
        gData.mCore0MonitorStarted = TRUE;
        X32_CacheFlushAll();
        K2_CpuWriteBarrier();
    }

//    K2OSKERN_Debug("Core %d starts\n", aThisCpuCoreIndex);
    pCoreMem->CpuCore.mIsInMonitor = TRUE;
    X32Kern_EnterMonitor(stackPtr);

    //
    // should never return 
    //
    while (1);
}

static void 
sSubCore_Entry(
    void
)
{
    /* running on stack at 0x1FFC that was setup in real mode (above) */
    STARTARGS_1800 *pArgs;

    /* get startup args so we know our index*/
    pArgs = (STARTARGS_1800 *)0x1800;

    /* initialize APIC for this core */
    X32Kern_APICInit(pArgs->mCpuIx);

    /* jump into kernel */
    pArgs->mKernelEntry(pArgs->mCpuIx);

    K2OSKERN_Panic("sSubCore_Entry returned\n");
}

static void 
sStartOtherCpu(
    UINT32 aCpuIndex
)
{
    STARTARGS_1800 *pArgs;
    UINT32          reg;

    //
    // copy in startup code
    //
    K2MEM_Copy((void *)K2OS_KVA_X32_AP_TRANSITION, sgCpuStartCode, sizeof(sgCpuStartCode));

    //
    // set arguments
    //
    pArgs = (STARTARGS_1800 *)(K2OS_KVA_X32_AP_TRANSITION + 0x800);
    pArgs->mTransitionCR3 = K2OS_X32_APSTART_PAGEDIR_ADDR;
    pArgs->mSubCoreEntry = (UINT32)sSubCore_Entry;
    pArgs->mCpuIx = aCpuIndex;
    pArgs->mKernelEntry = X32Kern_CpuLaunchEntryPoint;

    //
    // flush cache and wait for writes to complete
    //
    X32_CacheFlushAll();
    K2_CpuWriteBarrier();

    //
    // send physical INIT IPI to target processor
    //
    MMREG_WRITE32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_ICR_HIGH32, (aCpuIndex << 24));
    MMREG_WRITE32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_ICR_LOW32, X32_LOCAPIC_ICR_LOW_LEVEL_ASSERT | X32_LOCAPIC_ICR_LOW_PHYSICAL | X32_LOCAPIC_ICR_LOW_MODE_INIT);
    do {
        reg = MMREG_READ32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_ICR_LOW32);
    } while (reg & X32_LOCAPIC_ICR_LOW_DELIVER_STATUS);

    //
    // send physical START IPI to target processor
    //
    MMREG_WRITE32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_ICR_HIGH32, (aCpuIndex << 24));
    MMREG_WRITE32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_ICR_LOW32, X32_LOCAPIC_ICR_LOW_LEVEL_ASSERT | X32_LOCAPIC_ICR_LOW_PHYSICAL | X32_LOCAPIC_ICR_LOW_MODE_STARTUP | 0x01); /* vector to 0x1000 */
    do {
        reg = MMREG_READ32(K2OS_KVA_X32_LOCAPIC, X32_LOCAPIC_OFFSET_ICR_LOW32);
    } while (reg & X32_LOCAPIC_ICR_LOW_DELIVER_STATUS);
}

void
KernArch_LaunchCpuCores(
    void
)
{
    UINT32                      index;
    X32_PAGEDIR *               pKernelPageDir;
    X32_PAGEDIR *               pPageDir;
    X32_PAGETABLE *             pPageTable;
    K2OSKERN_CPUCORE volatile * pAuxCore;

    //
    // start time
    //
    X32Kern_StartTime();

    //
    // launch other cores now
    //
    if (gData.mCpuCoreCount > 1)
    {
        // 
        // map the AP processors' startup page directory so we can write to it.
        //
        KernPte_MakePageMap(NULL, K2OS_KVA_X32_AP_PAGEDIR, K2OS_X32_APSTART_PAGEDIR_ADDR, K2OS_MAPTYPE_KERN_DATA);
        pPageDir = (X32_PAGEDIR *)K2OS_KVA_X32_AP_PAGEDIR;
        K2MEM_Zero(pPageDir, K2_VA_MEMPAGE_BYTES);

        //
        // map the AP processors' startup page table so we can write to it.
        //
        KernPte_MakePageMap(NULL, K2OS_KVA_X32_AP_PAGETABLE, K2OS_X32_APSTART_PAGETABLE_ADDR, K2OS_MAPTYPE_KERN_DATA);
        pPageTable = (X32_PAGETABLE *)K2OS_KVA_X32_AP_PAGETABLE;
        K2MEM_Zero(pPageTable, K2_VA_MEMPAGE_BYTES);

        //
        // put the AP processors' startup pagetable into its page directory
        //
        pPageDir->Entry[K2OS_X32_APSTART_PAGETABLE_ADDR / K2_VA32_PAGETABLE_MAP_BYTES].mAsUINT32 = K2OS_X32_APSTART_PAGETABLE_ADDR | X32_KERN_PAGETABLE_PROTO;

        //
        // put the page directory and the pagetable into the pagetable
        //
        pPageTable->Entry[K2OS_X32_APSTART_PAGEDIR_ADDR / K2_VA_MEMPAGE_BYTES].mAsUINT32 =
            K2OS_X32_APSTART_PAGEDIR_ADDR | X32_PTE_GLOBAL | X32_PTE_PRESENT | X32_PTE_WRITEABLE | X32_PTE_WRITETHROUGH | X32_PTE_CACHEDISABLE;
        pPageTable->Entry[K2OS_X32_APSTART_PAGETABLE_ADDR / K2_VA_MEMPAGE_BYTES].mAsUINT32 =
            K2OS_X32_APSTART_PAGETABLE_ADDR | X32_PTE_GLOBAL | X32_PTE_PRESENT | X32_PTE_WRITEABLE | X32_PTE_WRITETHROUGH | X32_PTE_CACHEDISABLE;

        //
        // map the transition page.  AP startup code is hardwired to work at this address
        //
        KernPte_MakePageMap(NULL, K2OS_KVA_X32_AP_TRANSITION, K2OS_X32_APSTART_TRANSIT_PAGE_ADDR, K2OS_MAPTYPE_KERN_TRANSITION);

        //
        // put the transition page into the AP processors' pagetable
        //
        pPageTable->Entry[K2OS_X32_APSTART_TRANSIT_PAGE_ADDR / K2_VA_MEMPAGE_BYTES].mAsUINT32 =
            K2OS_X32_APSTART_TRANSIT_PAGE_ADDR | X32_PTE_GLOBAL | X32_PTE_PRESENT | X32_PTE_WRITEABLE | X32_PTE_WRITETHROUGH | X32_PTE_CACHEDISABLE;

        //
        // copy over everything in the kernel range of the real kernel's page directory
        // so that the AP processors can see everything in the real kernel
        //
        pKernelPageDir = (X32_PAGEDIR *)K2OS_KVA_TRANSTAB_BASE;
        index = K2OS_KVA_KERN_BASE / K2_VA32_PAGETABLE_MAP_BYTES;
        do
        {
            pPageDir->Entry[index].mAsUINT32 = pKernelPageDir->Entry[index].mAsUINT32;
        } while (++index < K2_VA32_PAGETABLES_FOR_4G);
        X32_CacheFlushAll();
        K2_CpuWriteBarrier();

        //
        // start all the other cores one by one, waiting for each to come up
        //
        for (index = 1; index < gData.mCpuCoreCount; index++)
        {
            pAuxCore = K2OSKERN_COREIX_TO_CPUCORE(index);
            sStartOtherCpu(index);
            while (pAuxCore->mIsExecuting == FALSE);
        }

        //
        // unmap startup stuff now
        //
        KernPte_BreakPageMap(NULL, K2OS_KVA_X32_AP_PAGEDIR, 0);
        X32_TLBInvalidatePage(K2OS_KVA_X32_AP_PAGEDIR);

        KernPte_BreakPageMap(NULL, K2OS_KVA_X32_AP_PAGETABLE, 0);
        X32_TLBInvalidatePage(K2OS_KVA_X32_AP_PAGETABLE);

        KernPte_BreakPageMap(NULL, K2OS_KVA_X32_AP_TRANSITION, 0);
        X32_TLBInvalidatePage(K2OS_KVA_X32_AP_TRANSITION);
    }

    X32Kern_CpuLaunchEntryPoint(0);

    // should never return

    K2OSKERN_Panic("CpuLaunch entry returned\n");
}

