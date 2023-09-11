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
.extern KernCpu_RunMonitor

//void A32KernAsm_MonitorMainLoop(void)
BEGIN_A32_PROC(A32KernAsm_MonitorMainLoop)
    mov r11, #0
    bl KernCpu_RunMonitor
    // if we return interrupts are off and we are headed to WFI on this core
    dsb
    dmb
    mcr p15, 0, r0, c7, c5, 0   // ICIALLU
    isb
    wfi
    b  A32KernAsm_MonitorMainLoop
END_A32_PROC(A32KernAsm_MonitorMainLoop)

//void A32KernAsm_ResumeInMonitor(UINT32 aStackPtr);
BEGIN_A32_PROC(A32KernAsm_ResumeInMonitor)
    // make sure core is in SYS mode
    mrs r12, cpsr
    bic r12, r12, #A32_PSR_MODE_MASK
    orr r12, r12, #A32_PSR_MODE_SYS
    msr cpsr, r12
    // set stack ptr  
    mov r13, r0
    // off to svc loop
    b   A32KernAsm_MonitorMainLoop
END_A32_PROC(A32KernAsm_ResumeInMonitor)

    .end



