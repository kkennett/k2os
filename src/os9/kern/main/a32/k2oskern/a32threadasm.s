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

// void A32KernAsm_ResumeThread(UINT32 aKernModeStackPtr, UINT32 aSvcScratch);
BEGIN_A32_PROC(A32KernAsm_ResumeThread)
    // in system mode on core SYS stack
    // r0 points to saved context for thread in its thread structure
    // r1 poinst to SVC stack work area
    
    // move saved r12 and r15 to svc mode stack scratch
    sub r1, r1, #8      // make space 
    ldr r2, [r0, #52]   // load saved context r12 to r2
    str r2, [r1, #4]    // store saved context r12 to the temp space slot #0
    ldr r2, [r0, #64]   // load saved context r15 to r2
    str r2, [r1, #8]    // store saved context r15 to the temp space slot #1

    // restore registers r13 and r14 while in system mode
    ldr r13, [r0, #56]  // load user mode stack pointer to r13; we are off system stack after this
    ldr r14, [r0, #60]  // load user mode link register to r14;

    // switch to service mode and set stack
    mrs r2, cpsr
    bic r2, r2, #A32_PSR_MODE_MASK
    orr r2, r2, #A32_PSR_MODE_SVC
    msr cpsr, r2
    mov r13, r1         // svc mode r13 is just below where we made space
    mov r12, r0         // svc mode r12 is the saved context base

    // start to consume exception context - set SVC mode SPSR for resume
    ldmia r12!, {r0}    // restore user mode SPSR (first thing in struct) to svc r0 
    msr spsr_cxsf, r0   // put user mode PSR into svc mode spsr

    // now eat main chunk of registers
    ldmia r12!, {r0-r11}    // load r0-r11 to svc mode registers from the saved stack 

    // r0-r11, r13, r14 restored.  spsr set.  do resume now.
    ldmib r13!, {r12, pc}^  // slots 0 and 1 contain user r12 and user pc respectively. svc stack left in proper order

END_A32_PROC(A32KernAsm_ResumeThread)

    .end



