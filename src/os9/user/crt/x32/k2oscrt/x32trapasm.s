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
#include <k2asmx32.inc>

#if 0
/*-------------------------------------------------------------------------------*/
//BOOL K2_CALLCONV_REGS TrapExec(K2_EXCEPTION_TRAP * apTrap, UINT32 aResult);
BEGIN_X32_PROC(TrapExecAsm)
    mov  dword ptr [%ecx + 4], %edx         // put result in its place in the trap 
    mov %edx, dword ptr [%ecx]              // get next trap address to edx now
    mov  %eax,%fs:0x0                       // thread index to ecx
    shl  %eax, 0xC                          // convert thread index to page address 
    mov  dword ptr [%eax + 0x128], %edx     // store next trap address in thread page in correct place
    mov  %esp, %ecx                         // prep for context restore
    add %esp, 8                             // skip over next trap pointer and exception code
    popfd                                   // flags pushed second
    popad                                   // regs pushed first
    pop %eax                                // this is resume address at end of trap saved context.  Will ALWAYS be nonzero
    mov %esp, dword ptr [%esp - 24]         // restore original stack pointer that was saved (and tweaked)
    jmp %eax
END_X32_PROC(TrapExecAsm)
#endif

/*-------------------------------------------------------------------------------*/
//BOOL K2_CALLCONV_REGS X32CrtAsm_ExTrap_Mount(K2_EXCEPTION_TRAP *apTrap)

BEGIN_X32_PROC(X32CrtAsm_ExTrap_Mount)
    // return address is already on stack at esp, this is BOS for X32_CONTEXT
    pushad                      // 8 * 4
    pushfd                      // 1 * 4
.extern X32Crt_ExTrap_Mount_C
    call X32Crt_ExTrap_Mount_C  // ecx still points to trap
    add %esp, (9*4)             // get rid of pusha, pushf 
    ret
END_X32_PROC(X32CrtAsm_ExTrap_Mount)

/*-------------------------------------------------------------------------------*/

    .end
