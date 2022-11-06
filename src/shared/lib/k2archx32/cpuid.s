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
#include <k2asmx32.inc>

/*-------------------------------------------------------------------------------*/
// void K2_CALLCONV_REGS X32_CallCPUID(X32_CPUID *apIo);
BEGIN_X32_PROC(X32_CallCPUID)
    push 0
    push %ecx
    mov %eax, dword ptr [%ecx]
    cpuid
    mov dword ptr [%esp+4], %ecx
    pop %ecx
    mov dword ptr [%ecx], %eax
    mov dword ptr [%ecx+4], %ebx
    mov dword ptr [%ecx+12], %edx
    pop %eax
    mov dword ptr [%ecx+8], %eax
    ret
END_X32_PROC(X32_CallCPUID)

#if 0

BEGIN_X32_PROC(Transition)
    // invalidate instruction cache
    wbinvd

    // disable paging in cr0 so we can change PAE bit
    // we are executing in 1:1 on this page so we can do this
    mov %eax, %cr0
    and %eax, 0x7FFFFFFF
    mov %cr0, %eax

    // now that paging is off, disable PAE in CR4 - K2OS uses 32-bit pagetables
    mov %eax, %cr4
    and %eax, 0xFFFFFFDF
    mov %cr4, %eax

    // get currently executing page address
    call    nextInstr
nextInstr:
    pop     %ecx
    and     %ecx,0xfffff000

    // halfway into the page is the load info
    add     %ecx,0x800

    // get the translation table base register value
    mov     %eax,DWORD PTR[%ecx + 0x18]    //  0x18 is K2_UEFI_LOADINFO_OFFSET_TRANSBASE_PHYS
    mov     %cr3,%eax

    // enable paging and kernel RO page write protect in CR0
    mov     %eax,%cr0
    or      %eax,0x80000000    // PAGING_ENABLE
    or      %eax,0x00010000    // WRITE_PROTECT(kernel cant write to RO pages)

    // turn it on
    wbinvd
    mov     %cr0,%eax
    wbinvd

    // re-flush all translation with a cycle of cr3
    mov     %eax,%cr3
    mov     %cr3,%eax

    // jump into kernel virtual entry point
    mov     %eax,DWORD PTR[%ecx + 0x8]     // 0x8 is K2OS_UEFI_LOADINFO_OFFSET_SYSVIRTENTRY
    jmp     %eax
END_X32_PROC(Transition)

#endif

/*-------------------------------------------------------------------------------*/

    .end
