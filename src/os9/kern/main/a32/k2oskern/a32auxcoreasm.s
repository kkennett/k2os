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

#include "a32kernasm.inc"

BEGIN_A32_PROC(A32KernAsm_AuxCpuStart)

A32AUXCPU_DACR:
    .word   0x00000000
A32AUXCPU_TTBR0:
    .word   0x00000000
A32AUXCPU_COPROCACCESS:
    .word   0x00000000
A32AUXCPU_CONTROL:
    .word   0x00000000
A32AUXCPU_CONTINUE:
    .word   0x00000000
    .word   0x00000000
    .word   0x00000000
    .word   0x00000000

    //---------------------------------------------------------------
    // disable interrupts and set SVC mode
    //---------------------------------------------------------------
    mrs r12, cpsr
    bic r12, r12, #A32_PSR_MODE_MASK
    orr r12, r12, #(A32_PSR_F_BIT | A32_PSR_I_BIT)
    orr r12, r12, #A32_PSR_MODE_SVC
    msr cpsr_c, r12      

    //---------------------------------------------------------------
    // Invalidate TLB, I Cache and BP array
    //---------------------------------------------------------------
    mov r0, #0                          // setup up for MCR
    mcr p15, 0, r0, c8, c7, 0           // invalidate TLB's
    mcr p15, 0, r0, c7, c5, 0           // invalidate I cache
    mcr p15, 0, r0, c7, c5, 6           // invalidate BP array
    dsb
    isb 
    
    //---------------------------------------------------------------
    // Initialize CP15 control register
    //---------------------------------------------------------------
    mrc p15, 0, r0, c1, c0, 0   

    // turn these off
    bic r0, r0, #A32_SCTRL_M_MMUENABLE
    bic r0, r0, #A32_SCTRL_V_HIGHVECTORS
    bic r0, r0, #A32_SCTRL_I_ICACHEENABLE
    bic r0, r0, #A32_SCTRL_C_DCACHEENABLE
    bic r0, r0, #A32_SCTRL_Z_BRANCHPREDICTENABLE
    
    // turn this on
    orr r0, r0, #A32_SCTRL_A_ALIGNFAULTCHECKENABLE
    mcr p15, 0, r0, c1, c0, 0   
    dsb
    isb

    //---------------------------------------------------------------
    // Invalidate TLB, I Cache and BP array again
    //---------------------------------------------------------------
    mov r0, #0                          // setup up for MCR
    mcr p15, 0, r0, c8, c7, 0           // invalidate TLB's
    mcr p15, 0, r0, c7, c5, 0           // invalidate I cache
    mcr p15, 0, r0, c7, c5, 6           // invalidate BP array
    dsb
    isb 

    //---------------------------------------------------------------
    // Initialize stacks for transition so we can see where faults happen
    //---------------------------------------------------------------
    mov r0, pc
    orr r0, r0, #0xF00
    orr r0, r0, #0x0FF
    bic r0, r0, #3          // set r0 to pc page at 0xFFC offset 
    mov sp, r0              // SVC to 0x?????FFC 

    mrs r12, cpsr
    bic r12, r12, #A32_PSR_MODE_MASK
    orr r12, r12, #A32_PSR_MODE_ABT
    msr cpsr_c, r12      
    sub r0, r0, #0x200
    mov sp, r0              // ABT to 0x?????DFC

    bic r12, r12, #A32_PSR_MODE_MASK
    orr r12, r12, #A32_PSR_MODE_UNDEF
    msr cpsr_c, r12      
    sub r0, r0, #0x100
    mov sp, r0              // UNDEF to 0x?????CFC

    bic r12, r12, #A32_PSR_MODE_MASK
    orr r12, r12, #A32_PSR_MODE_SVC
    msr cpsr_c, r12      

    //---------------------------------------------------------------
    // Load values from passed variables
    //---------------------------------------------------------------
    add r0, pc, #A32AUXCPU_DACR-(.+8)
    ldr r1, [r0]
    dsb
    mcr p15, 0, r1, c3, c0, 0
    isb

    // ttbr will already have correct RGN/IRGN/S etc. in it
    add r0, pc, #A32AUXCPU_TTBR0-(.+8)
    ldr r1, [r0]
    dsb
    mcr p15, 0, r1, c2, c0, 0   // set TTBR 0       // these are the *TRANSITION* TTB address
    mcr p15, 0, r1, c2, c0, 1   // set TTBR 1
    isb
    mov r1, #1
    mcr p15, 0, r1, c2, c0, 2   // set TTBCR to 1
    isb

    // invalidate all TLBs
    mov r1, #0
    mcr p15, 0, r1, c8, c7, 0
    isb

    add r0, pc, #A32AUXCPU_COPROCACCESS-(.+8)
    ldr r1, [r0]
    dsb
    mcr p15, 0, r1, c1, c0, 2
    isb

    add r0, pc, #A32AUXCPU_CONTROL-(.+8)
    ldr r1, [r0]
    dsb
    orr r1, r1, #A32_SCTRL_I_ICACHEENABLE
    orr r1, r1, #A32_SCTRL_C_DCACHEENABLE
    orr r1, r1, #A32_SCTRL_Z_BRANCHPREDICTENABLE

    add r2, pc, #A32AUXCPU_CONTINUE-(.+8)
    ldr r3, [r2]
    dsb

    mov r7, #0
    mcr 15, 0, r7, cr13, cr0, 1
    isb

    // enable MMU and jump over a cache line
    mcr p15, 0, r1, c1, c0, 0
    b 100f
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
100:
    isb
    // put CPU index into r0
    mrc p15, 0, r0, c0, c0, 5
    and r0, r0, #0x3

    bx  r3
    .global A32KernAsm_AuxCpuStart_END
A32KernAsm_AuxCpuStart_END:
    b A32KernAsm_AuxCpuStart_END

END_A32_PROC(A32KernAsm_AuxCpuStart)
    .ltorg

    .end