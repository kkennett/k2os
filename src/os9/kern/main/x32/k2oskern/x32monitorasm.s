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

#include "x32kernasm.inc"

.extern KernCpu_RunMonitor

BEGIN_X32_PROC(X32KernAsm_MonitorMainLoop)
    call KernCpu_RunMonitor
    // if we return we are WFI on this core
Wait_For_Interrupt:
    sti
    hlt 
    // should never return but if we do keep waiting for interrupt
    jmp Wait_For_Interrupt
END_X32_PROC(X32KernAsm_MonitorMainLoop)

//void K2_CALLCONV_REGS X32KernAsm_EnterMonitor(UINT32 aESP);
BEGIN_X32_PROC(X32KernAsm_EnterMonitor)
    mov %esp, %ecx
    mov %ebp, 0
    jmp X32KernAsm_MonitorMainLoop
END_X32_PROC(X32KernAsm_EnterMonitor)

// void K2_CALLCONV_REGS X32KernAsm_EnterMonitorFromKernelThread(UINT32 aNewStackPtr, UINT32 aDummy);
BEGIN_X32_PROC(X32KernAsm_EnterMonitorFromKernelThread)
    pop %edx                        // retrieve return address
    mov dword ptr [%esp-12], %edx   // store as EIP in saved context on thread stack under eflags and cs 

    pushf               // EFLAGS
    pop %edx
    or %edx, X32_EFLAGS_INTENABLE
    push %edx           // pushed with interrupts enabled 

    xor %edx, %edx 
    mov %edx, %cs       
    push %edx           // CS

    sub %esp, 4         // EIP saved above 

    xor %edx, %edx
    push %edx           // errorcode - store 0
    push %edx           // exception vector - store 0
    
    pusha

    mov %edx, %ds       // DS
    push %edx 

    // ecx unchanged which is core stack address at which to enter monitor
    jmp X32KernAsm_EnterMonitor 
END_X32_PROC(X32KernAsm_EnterMonitorFromKernelThread)
  
    .end

