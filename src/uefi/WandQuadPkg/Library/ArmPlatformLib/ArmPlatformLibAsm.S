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

#include <AsmMacroIoLib.h>
#include <AutoGen.h>

.text
.align 2

GCC_ASM_EXPORT(ArmPlatformIsPrimaryCore)
GCC_ASM_EXPORT(ArmPlatformGetPrimaryCoreMpId)
GCC_ASM_EXPORT(ArmPlatformPeiBootAction)
GCC_ASM_EXPORT(ArmPlatformGetCorePosition)

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_PFX(ArmPlatformIsPrimaryCore):
    mrc   p15,0,r0,c0,c0,5
    ands  r0,r0,#3
    moveq r0,#1
    movne r0,#0
    bx 	lr

ASM_PFX(ArmPlatformPeiBootAction):
    //---------------------------------------------------------------
    // disable interrupts and set SVC mode
    //---------------------------------------------------------------
    mrs r12, cpsr
    bic r12, r12, #0x1F
    orr r12, r12, #0xD3
    msr cpsr_c, r12      

    //---------------------------------------------------------------
    // Flush all caches, Invalidate TLB and I cache
    //---------------------------------------------------------------
    mov r0, #0                          // setup up for MCR
    mcr p15, 0, r0, c8, c7, 0           // invalidate TLB's
    mcr p15, 0, r0, c7, c5, 0           // invalidate I cache
    mcr p15, 0, r0, c7, c5, 6           // invalidate BP array

    //---------------------------------------------------------------
    // Initialize CP15 control register
    //---------------------------------------------------------------
    mrc p15, 0, r0, c1, c0, 0   
    bic r0, r0, #0x00000001 // disable MMU
    bic r0, r0, #0x00002000 // disable high vectors
    bic r0, r0, #0x00001000 // disable I cache
    bic r0, r0, #0x00000004 // disable D cache
    bic r0, r0, #0x00000800 // disable branch predictor
    mcr p15, 0, r0, c1, c0, 0   
    isb
    
    //---------------------------------------------------------------
    // set manager access for all domains
    //---------------------------------------------------------------
    mvn r0, #0                  
    mcr p15, 0, r0, c3, c0, 0 

    bx lr

//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ASM_PFX(ArmPlatformGetPrimaryCoreMpId):
    mov r0, #0
    bx    lr

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
ASM_PFX(ArmPlatformGetCorePosition):
  and   r0, r0, #3
  bx    lr

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
