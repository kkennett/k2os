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

.extern X32Kern_CpuLaunch
.extern X32KernAsm_GDTFlush
.extern X32KernAsm_IDTFlush

.extern gX32Kern_KernelPageDirPhysAddr

// void K2_CALLCONV_REGS X32KernAsm_CpuLaunchEntryPoint(void)
BEGIN_X32_PROC(X32KernAsm_CpuLaunchEntryPoint)
    // ecx holds CPU index
    // we have no idea what GDT or page tables we are on

    //
    // put stack at proper place in core page for this core to init
    // the core data is at the start of the page and the stacks are
    // put at the end.  At launch there is the init stack at the end
    // which we can overwrite 
    //
    mov %eax, %ecx
    mov %ebx, K2OS_KVA_COREMEMORY_BASE
    mov %edx, 0x4000
    mul %edx
    add %eax, 0x3FFC
    add %eax, %ebx
    mov %esp, %eax

    mov %ebp, %esp
    xor %eax, %eax
    mov [%esp], %eax
    sub %esp, 4

    // ok our stack is in proper usable place now at the top of the memory for this core.
    // save the core index and load the real GDT and IDT
    push %ecx
    call X32KernAsm_GDTFlush
    call X32KernAsm_IDTFlush
    pop %ecx

    // real data and stack segment descriptors loaded in GDTFlush
    // real code segment also jumped to

    // we are off the old GDT now.  Load the new page directory
    mov %eax, [gX32Kern_KernelPageDirPhysAddr]
    mov %cr3, %eax

    // make sure WP bit is set in CR0
    mov %eax, %cr0
    or  %eax, X32_CR0_WRITE_PROTECT
    wbinvd
    mov %cr0, %eax
    wbinvd

    // we are completely onto real config structures now.  go into regular launch code
    // ecx has the core index
    jmp X32Kern_CpuLaunch
           
END_X32_PROC(X32KernAsm_CpuLaunchEntryPoint)
  
    .end

