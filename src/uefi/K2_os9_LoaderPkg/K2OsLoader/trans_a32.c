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

static UINT32 const sTransitionCode[] =
{
    //
    // put address of data into r0, which is halfway into 
    // the page this code is executing out of
    //
    0xe1a0000f,    //    mov r0, pc
    0xe3c000ff,    //    bic r0, r0, #0xFF
    0xe3c00c0f,    //    bic r0, r0, #0xF00
    0xe2800b02,    //    add r0, r0, #0x800

    // get TT base register physical address
    0xe5905020,    //    ldr r5, [r0, #K2_UEFI_LOADINFO_OFFSET_TRANSBASE_PHYS]

    // set TTB config bits
    0xe3855013,    //   orr     r5, r5, #0x13

    // set TTBR and CR
    0xee025f10,    //   mcr     15, 0, r5, cr2, cr0, { 0 }
    0xee025f30,    //   mcr     15, 0, r5, cr2, cr0, { 1 }
    0xf57ff04f,    //   dsb     sy
    0xf57ff06f,    //   isb     sy
    0xe3a06001,    //   mov     r6, #1
    0xee026f50,    //   mcr     15, 0, r6, cr2, cr0, { 2 }
    0xf57ff04f,    //   dsb     sy
    0xf57ff06f,    //   isb     sy

    // set domain access control register so that domain 0 has client access
    // and all others have no access
    0xe3a03001,    //    mov r3, #1
    0xee033f10,    //    mcr p15, 0, r3, c3, c0, 0 
    0xf57ff06f,    //    isb

    //
    // load virtual entrypoint to r4
    // and system entrypoint to r1
    //
    0xe5901010,    //    ldr r1, [r0, #K2OS_UEFI_LOADINFO_OFFSET_SYSVIRTENTRY]
    0xe28f4074,    //    adr r4, _JUMP_TO_KERNEL

    // clear CONTEXTIDR
    0xe3a07000,    //    mov     r7, #0
    0xee0d7f30,    //    mcr     15, 0, r7, cr13, cr0, {1}
    0xf57ff06f,    //    isb     sy

    // enable mmu
    0xee113f10,    //    mrc p15, 0, r3, c1, c0, 0   
    0xe3833001,    //    orr r3, r3, #A32_SCTRL_M_MMUENABLE
    0xe3833a01,    //    orr r3, r3, #4096; 0x1000
    0xe3833004,    //    orr r3, r3, #4
    0xe3833b02,    //    orr r3, r3, #2048; 0x800
    0xe3c33202,    //    bic r3, r3, #A32_SCTRL_AFE_ACCESSFLAGENABLE  // make sure turn off AFE (B3.6.1 in TRM)
    0xe3c33201,    //    bic r3, r3, #A32_SCTRL_TRE_TEXREMAPENABLE    // make sure turn off TEX remap
    0xee013f10,    //    mcr p15, 0, r3, c1, c0, 0   
    0xf57ff04f,    //    dsb
    0xf57ff06f,    //    isb

    // invalidate tlb
    0xe3a07000,    //    mov r7, #0
    0xee087f17,    //    mcr p15, 0, r7, c8, c7, 0
    0xf57ff04f,    //    dsb
    0xf57ff06f,    //    isb

    // invalidate i cache (even though it is off)
    0xee077f15,    //    mcr p15, 0, r7, c7, c5, 0
    0xee077fd5,    //    mcr p15, 0, r7, c7, c5, 6       
    0xf57ff04f,    //    dsb
    0xf57ff06f,    //    isb

    //
    // in case of fault during transition:
    //
    // r0   phys==virt address of loadinfo in transition page
    // r1   virtual kernel k2oscrt.xdl entrypoint
    // r2   MPIDR high 2 bits 
    // r3   value that was put into SCTLR
    // r4   address of _JUMP_TO_KERNEL below
    // r5   value put into both TTBR0, TTBR1
    // r6   value put into TTBCR
    // r7   zero

    // jump over 32 bytes to flush the pipeline
    0xe1a0f004,    //    mov pc, r4
    0xe320f000,    //    nop
    0xe320f000,    //    nop
    0xe320f000,    //    nop
    0xe320f000,    //    nop
    0xe320f000,    //    nop
    0xe320f000,    //    nop
    0xe320f000,    //    nop
    0xe320f000,    //    nop

//_JUMP_TO_KERNEL:
    // never coming back from here
    0xe1a0f001     //    mov pc, r1
};

typedef void(*pfEntry)(void);
        
void Loader_Arch_Transition(void)
{
    // Interrupts should have been disabled by the GIC driver via ExitBootServices

    //
    // copy transition page code into transition page
    //
    K2MEM_Copy((void *)gData.LoadInfo.mTransitionPageAddr, sTransitionCode, sizeof(sTransitionCode));

    //
    // clean up caches and disable MMU
    //
    ArmCleanInvalidateDataCache();
    ArmInvalidateInstructionCache();
    ArmDisableDataCache();
    ArmDisableInstructionCache();
    ArmDisableMmu();
    // in case anything snuck in
    ArmCleanInvalidateDataCache();
    ArmInvalidateInstructionCache();

    //
    // executing physically with caches disabled. jump into direct-mapped page
    // to set up the basics and jump into the kernel core xdl entrypoint at its 
    // virtual entrypoint address
    //
    ((pfEntry)gData.LoadInfo.mTransitionPageAddr)();

    while (1);
}

