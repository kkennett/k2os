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

#include "k2asma32.inc"

.extern A32Crt_ExTrap_Mount_C 

/*-------------------------------------------------------------------------------*/
#if 0
//BOOL K2_CALLCONV_REGS TrapExec(K2_EXCEPTION_TRAP * apTrap, UINT32 aResult);
BEGIN_A32_PROC(TrapExecAsm)
    str r1, [r0, #4]                    // put result in its place in the trap 
    ldr r1, [r0]                        // get next trap addres to r1

    mrc p15, 0, r12, c13, c0, 3         // get thread index from thread index register
    lsl r12, #12                        // convert to thread page address
    add r12, r12, #0x128                // go to offset of next trap in page
    str r1, [r12]                       // put next trap addrss into thread page

    add r0, r0, #8                      // move past next trap and trap result fields
    ldr r12, [r0]                       // get cpsr value from trap
    msr cpsr, r12                       // restore cpsr.  protected bit writes are ignored
    add r0, r0, #8                      // move r0 over cpsr save and r0 value save.  we don't care about r0 value here

    ldmia r0!, {r1-r14}                 // restore r1 to r14 from the trap
    bx lr                               // r0 is guaranteed nonzero.  resume at link register address
END_A32_PROC(TrapExecAsm)
#endif

//BOOL K2_CALLCONV_REGS A32CrtAsm_ExTrap_Mount(K2_EXCEPTION_TRAP *apTrap);
BEGIN_A32_PROC(A32CrtAsm_ExTrap_Mount)
    mrs r12, cpsr
    add r0, r0, #12
    stmia r0, {r0 - r15}
    sub r0, r0, #4 
    str r12, [r0]
    sub r0, r0, #8 
    b A32Crt_ExTrap_Mount_C
END_A32_PROC(A32CrtAsm_ExTrap_Mount)

    .end



