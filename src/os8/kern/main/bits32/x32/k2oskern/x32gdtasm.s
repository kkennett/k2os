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

#include "x32kernasm.inc"

.extern gX32Kern_GDTPTR

BEGIN_X32_PROC(X32Kern_GDTFlush)

    // flush cache
    wbinvd

    // flush tlb
    mov %eax, %cr3
    mov %cr3, %eax

    // load new gdt pointer
    mov    %eax, OFFSET gX32Kern_GDTPTR
    lgdt   %ds:[%eax]

    // far jump over cache line to reset code selector
    jmp    (X32_SEGMENT_SELECTOR_KERNEL_CODE | X32_SELECTOR_RPL_KERNEL):_flush    
.space 32
.align 16
_flush:

    // flush tlb again
    mov %eax, %cr3
    mov %cr3, %eax

    // reload other selectors
    xor    %eax, %eax
    mov    %ax, (X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_KERNEL)
    mov    %ds, %ax
    mov    %es, %ax
    mov    %fs, %ax
    mov    %gs, %ax
    mov    %ss, %ax

    ret
            
END_X32_PROC(X32Kern_GDTFlush)
  
    .end

