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

.extern A32Kern_CpuLaunch1
.extern A32Kern_CpuLaunch2

// void A32KernAsm_StackBridge(UINT32 aCpuIx, UINT32 aCoreMemStacksBase);
BEGIN_A32_PROC(A32KernAsm_StackBridge)
    mov r3, #0

    // IRQ first
    mrs r12, cpsr
    bic r12, r12, #A32_PSR_MODE_MASK
    orr r12, r12, #A32_PSR_MODE_IRQ
    msr cpsr, r12      
    add r1, r1, #A32KERN_CORE_IRQ_STACK_BYTES
    sub r1, r1, #4 
    str r3, [r1]
    sub r1, r1, #4 
    str r3, [r1]
    mov sp, r1 
    add r1, r1, #8

    // now ABT
    mrs r12, cpsr
    bic r12, r12, #A32_PSR_MODE_MASK
    orr r12, r12, #A32_PSR_MODE_ABT
    msr cpsr, r12      
    add r1, r1, #A32KERN_CORE_ABT_STACK_BYTES
    sub r1, r1, #4 
    str r3, [r1]
    sub r1, r1, #4 
    str r3, [r1]
    mov sp, r1 
    add r1, r1, #8

    // now UND
    mrs r12, cpsr
    bic r12, r12, #A32_PSR_MODE_MASK
    orr r12, r12, #A32_PSR_MODE_UNDEF
    msr cpsr, r12      
    add r1, r1, #A32KERN_CORE_UND_STACK_BYTES
    sub r1, r1, #4 
    str r3, [r1]
    sub r1, r1, #4 
    str r3, [r1]
    mov sp, r1 
    add r1, r1, #8

    // now SVC
    mrs r12, cpsr
    bic r12, r12, #A32_PSR_MODE_MASK
    orr r12, r12, #A32_PSR_MODE_SVC
    msr cpsr, r12      
    add r1, r1, #A32KERN_CORE_SVC_STACK_BYTES
    sub r1, r1, #4 
    str r3, [r1]
    sub r1, r1, #4 
    str r3, [r1]
    mov sp, r1 
    add r1, r1, #8

    // finally SYS
    mrs r12, cpsr
    bic r12, r12, #A32_PSR_MODE_MASK
    orr r12, r12, #A32_PSR_MODE_SYS
    msr cpsr, r12 

    movw r2, #0xFFF 
    movt r2, #0 
    mvn r2, r2 
    and r1, r1, r2
    add r1, r1, #0x4000 
    sub r1, r1, #4 
    str r3, [r1]
    sub r1, r1, #4
    str r3, [r1]

    // this is our sys stack pointer
    mov sp, r1

    // all done - stay in SYS mode
    b A32Kern_CpuLaunch2

END_A32_PROC(A32KernAsm_StackBridge)

// void A32KernAsm_LaunchEntryPoint(UINT32 aCoreIndex);
BEGIN_A32_PROC(A32KernAsm_LaunchEntryPoint)
    // do not use memory. ldr rx,= will use memory as it will load a literal. dont do that.
    movw r1, #K2OS_KVA_COREMEMORY_BASE_LOW16
    movt r1, #K2OS_KVA_COREMEMORY_BASE_HIGH16 
    mov  r3, #0x1000
    mul  r3, r3, r0
    add  r1, r1, r3

    // r1 points to bottom of core page
    mov  r3, #0xFC
    orr  r3, r3, #0xF00
    add  sp, r1, r3

    // r0 still holds CPU index
.extern A32Kern_CpuLaunch1    
    b A32Kern_CpuLaunch1
       
END_A32_PROC(A32KernAsm_LaunchEntryPoint)

    .end


