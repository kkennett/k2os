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

// BOOL K2_CALLCONV_REGS KernArch_ExTrapMount(K2_EXCEPTION_TRAP *apTrap);
BEGIN_X32_PROC(KernArch_ExTrapMount)
    // ecx has pointer to exception trap record
    // edx has garbage

    // make sure there is enough room for an exception context here
    // this will cause a stack fault HERE if there isnt enough room
    sub %esp, (X32_SIZEOF_KERN_EXCEPTION_CONTEXT - 4) // -4 for the return address already pushed
    push 0                                              // push so that its a stack fault and not an access violation
    add %esp, X32_SIZEOF_KERN_EXCEPTION_CONTEXT       // restore takes into account push 0 just done

    // now put the saved state onto the stack using pusha
    //
    pusha   // esp will get stack before push
    pushf   // active flags
    cli     // disable interrupts

    // push return address and jump into C
    push offset exTrap_Return
.extern X32Kern_MountExceptionTrap
    jmp X32Kern_MountExceptionTrap

exTrap_Return:
    // back from C
    popf                // pop active flags (should restore interrupts)
    add %esp, (8 * 4)   // reverse pusha
    xor %eax, %eax      // force clear eax (return false from mount)
    ret

END_X32_PROC(KernArch_ExTrapMount)

    .end

