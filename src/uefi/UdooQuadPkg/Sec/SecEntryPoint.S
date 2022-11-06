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
#include <Library/PcdLib.h>
#include <AutoGen.h>
#include <IMX6def.inc>
#include <UdooQuadDef.inc>

.text
.align 3
.arch_extension sec

GCC_ASM_EXPORT(_ModuleEntryPoint)
GCC_ASM_IMPORT(UdooQuadPrimaryCoreSecStart)
GCC_ASM_IMPORT(UdooQuadSecondaryCoreSecStart)
GCC_ASM_IMPORT(NonSecureSMC)
GCC_ASM_EXPORT(ArmGetScuBaseAddress)

#define A9_SCU_INVALL_OFFSET        0xC

#define A32_ACTRL_MAINT_BCAST               0x001
#define A32_ACTLR_L1_PREFETCH_ENABLE        0x004
#define A32_ACTLR_WRITE_ZEROS_MODE          0x008
#define A32_ACTLR_SMP_MODE                  0x040
#define A32_ACTLR_EXCLUSIVE_CACHE_CONFIG    0x080
#define A32_ACTLR_CACHE_ALLOC_ONEWAY        0x100
#define A32_ACTLR_PARITY_CHECK_ENABLE       0x200

#define A32_SCTRL_M_MMUENABLE               0x00000001  // 0
#define A32_SCTRL_A_ALIGNFAULTCHECKENABLE   0x00000002  // 1
#define A32_SCTRL_C_DCACHEENABLE            0x00000004  // 2
#define A32_SCTRL_Z_BRANCHPREDICTENABLE     0x00000800  // 11
#define A32_SCTRL_I_ICACHEENABLE            0x00001000  // 12
#define A32_SCTRL_V_HIGHVECTORS             0x00002000  // 13
#define A32_SCTRL_VE_IMPVECTORSENABLE       0x01000000  // 24
#define A32_SCTRL_EE_EXCEPTIONENDIAN        0x02000000  // 25

#define A32_PSR_F_BIT                       0x00000040
#define A32_PSR_I_BIT                       0x00000080
#define A32_PSR_MODE_SVC                    0x13
#define A32_PSR_MODE_MONITOR                0x16
#define A32_PSR_MODE_MASK                   0x1F

#define ARM_GIC_OFFSET_ICCPMR               0x04

ASM_PFX(_ModuleEntryPoint):
    // disable interrupts and set SVC mode
    // do not r/w memory
    mrs r12, cpsr
    bic r12, r12, #A32_PSR_MODE_MASK
    orr r12, r12, #(A32_PSR_F_BIT | A32_PSR_I_BIT)
    orr r12, r12, #A32_PSR_MODE_SVC
    msr cpsr, r12      

    // now in SVC mode in secure state. clear link register and backup our cpsr to spsr
    // do not r/w memory
    mov lr, #0
    mrs r12, cpsr
    msr spsr_cxsf, r12

    // determine if we are the primary core and if so jump to primary-only init
    // do not r/w memory
    mrc   p15, 0, r12, c0, c0, 5    // read MPIDR
    ands  r12, r12, #3
    mov   r7, r12
    bne   _SecondaryInit

    // Primary Core Only -------------------------------------------------

    // turn off all other cores (via IMX6_PHYSADDR_SRC_SCR)
    // do not r/w memory
    movw r0, #0x8000
    movt r0, #0x020D
    ldr r12, [r0]
    bic r12, r12, #IMX6_SRC_SCR_CORE1_RST
    bic r12, r12, #IMX6_SRC_SCR_CORE1_ENABLE
    bic r12, r12, #IMX6_SRC_SCR_CORE2_RST
    bic r12, r12, #IMX6_SRC_SCR_CORE2_ENABLE
    bic r12, r12, #IMX6_SRC_SCR_CORE3_RST
    bic r12, r12, #IMX6_SRC_SCR_CORE3_ENABLE
    str r12, [r0]

    // turn off stuff but not the caches
    // do not r/w memory
    mrc p15, 0, r12, c1, c0, 0   // read SCTRL
    bic r12, r12, #A32_SCTRL_M_MMUENABLE
    bic r12, r12, #A32_SCTRL_A_ALIGNFAULTCHECKENABLE
    orr r12, r12, #A32_SCTRL_V_HIGHVECTORS          // turn on high vectors so VBAR is always ignored
    bic r12, r12, #A32_SCTRL_VE_IMPVECTORSENABLE    // use IRQ and FIQ from the vector table
    bic r12, r12, #A32_SCTRL_EE_EXCEPTIONENDIAN
    mcr p15, 0, r12, c1, c0, 0   // write SCTRL
    isb

    // clean and invalidate the L1
    // do not r/w memory
    mov     r0, #0                      // select L1 data cache
    mcr     p15, 2, r0, c0, c0, 0       // write CSSELR
    mrc     p15, 1, r0, c0, c0, 0       // read current CCSIDR 
    mov     r1, #0x8000             // make mask 0x7FFF 
    sub     r1, r1, #1              
    and     r2, r1, r0, lsr #13     // r2 = extract # of sets (15 bits)
    mov     r1, #0x400              // make mask 0x3FF
    sub     r1, r1, #1
    and     r3, r1, r0, lsr #3      // r3 = extract # of associativity (10 bits)
    add     r2, r2, #1              // number of sets is -1 in register, so add back one
    and     r0, r0, #0x7            // r0 = extract line size (power of two words in cache - 2)
    add     r0, r0, #4
    clz     r1, r3                  // r1 = cound leading zeros of associativity 
    add     r4, r3, #1              // associativity is -1 in register, so add backk now
1:  sub     r2, r2, #1              // dec set
    mov     r3, r4                  // init r3 to associatvity
2:  subs    r3, r3, #1              // dec associativity
    mov     r5, r3, lsl r1          // r5 = associativity * in right pos for mcr
    mov     r6, r2, lsl r0          // r6 = set * in right pos for mcr
    orr     r5, r5, r6              // combine r5 and r6
    mcr     p15, 0, r5, c7, c14, 2   // clean and invalidate data cache by set/way
    bgt     2b                      // if more associativity, go back
    cmp     r2, #0                  // check if there are sets left
    bgt     1b                      // if more sets left go do next set
    dsb
    isb

    // clean and invalidate the entire L2 (as per app note in manual)
    // do not r/w memory
    movw r0, #0x27FC    // 0x00A027FC is clean/invalidate by way
    movt r0, #0x00A0
    movw r1, #0xFFFF
    movt r1, #0
    str r1, [r0]
_Wait_CleanInvalidate_Completed:
    ldr r2, [r0]
    ands r3, r2, r1
    bne _Wait_CleanInvalidate_Completed

    // invalidate the instruction caches
    // do not r/w memory
    mov r12, #0      
    mcr p15, 0, r12, c8, c7, 0           // invalidate TLBs - TLBIALL
    mcr p15, 0, r12, c7, c5, 0           // invalidate I cache - ICIALLU  
    mcr p15, 0, r12, c7, c5, 6           // invalidate BP array - BPIALL

    // turn off the caches now
    // do not r/w memory
    mrc p15, 0, r12, c1, c0, 0   // read SCTRL
    bic r12, r12, #A32_SCTRL_I_ICACHEENABLE
    bic r12, r12, #A32_SCTRL_C_DCACHEENABLE
    bic r12, r12, #A32_SCTRL_Z_BRANCHPREDICTENABLE
    mcr p15, 0, r12, c1, c0, 0   // write SCTRL
    isb

    // L2 Cache disable
    // do not r/w memory
    movw r0, #0x2100    // 0x00A02100 is PL310 control register
    movt r0, #0x00A0
    ldr r1, [r0]
    bic r1, r1, #1  // clear the enable bit
    str r1, [r0]
    dsb
    isb

    // L1 and L2 caches are both now off. invalidate the instruction caches again
    // do not r/w memory
    mov r12, #0      
    mcr p15, 0, r12, c8, c7, 0           // invalidate TLBs - TLBIALL
    mcr p15, 0, r12, c7, c5, 0           // invalidate I cache - ICIALLU  
    mcr p15, 0, r12, c7, c5, 6           // invalidate BP array - BPIALL

    // all caches should be off and empty now.  invalidate L1 in case anything was prefetched/snuck in 
    // do not r/w memory
    mov     r0, #0                  // select L1 data cache
    mcr     p15, 2, r0, c0, c0, 0   // write CSSELR
    mrc     p15, 1, r0, c0, c0, 0   // read current CCSIDR 
    mov     r1, #0x8000             // make mask 0x7FFF 
    sub     r1, r1, #1              
    and     r2, r1, r0, lsr #13     // r2 = extract # of sets (15 bits)
    mov     r1, #0x400              // make mask 0x3FF
    sub     r1, r1, #1
    and     r3, r1, r0, lsr #3      // r3 = extract # of associativity (10 bits)
    add     r2, r2, #1              // number of sets is -1 in register, so add back one
    and     r0, r0, #0x7            // r0 = extract line size (power of two words in cache - 2)
    add     r0, r0, #4
    clz     r1, r3                  // r1 = cound leading zeros of associativity 
    add     r4, r3, #1              // associativity is -1 in register, so add backk now
1:  sub     r2, r2, #1              // dec set
    mov     r3, r4                  // init r3 to associatvity
2:  subs    r3, r3, #1              // dec associativity
    mov     r5, r3, lsl r1          // r5 = associativity * in right pos for mcr
    mov     r6, r2, lsl r0          // r6 = set * in right pos for mcr
    orr     r5, r5, r6              // combine r5 and r6
    mcr     p15, 0, r5, c7, c6, 2   // invalidate data cache by set/way
    bgt     2b                      // if more associativity, go back
    cmp     r2, #0                  // check if there are sets left
    bgt     1b                      // if more sets left go do next set
    dsb
    isb

    // L2 shoudl be off.  invalidate the entire L2 anyway in case anything was prefetched/snuck in 
    // do not r/w memory
    movw r0, #0x277C    // 0x00A0277C is invalidate by way 
    movt r0, #0x00A0
    movw r1, #0xFFFF
    movt r1, #0
    str r1, [r0]
_Wait_Invalidate_Completed:
    ldr r2, [r0]
    ands r3, r2, r1
    bne _Wait_Invalidate_Completed

    // done caches =============================================================

    // set manager access for all domains
    // do not r/w memory
    mvn r12, #0                  
    mcr p15, 0, r12, c3, c0, 0      // write DACR

    // SCU setup
    mrc p15, 4, r0, c15, c0, 0      // read CBAR (PERIPHBASE)
    mov r12, #0                     // clear enable
    str r12, [r0]
    add r1, r0, #A9_SCU_INVALL_OFFSET   // set to INVALL offset 
    mvn r12, #0                  
    str r12, [r1]                   // invalidate all 
    mov r12, #1
    str r12, [r0]                   // set SCU enable
    add r1, r0, #A9_SCU_INVALL_OFFSET   // set to INVALL offset 
    mvn r12, #0                  
    str r12, [r1]                   // invalidate all 

    // set SMP mode
    mrc	p15, 0, r12, c1, c0, 1      // read ACTLR
    orr r12, r12, #(A32_ACTLR_SMP_MODE | A32_ACTRL_MAINT_BCAST)
    mcr p15, 0, r12, c1, c0, 1      // write ACTLR
    isb

    // set monitor vector table base address (MVBAR)
    adr r12, _MonitorResetEntry
    mcr p15, 0, r12, c12, c0, 1     // write MVBAR
    isb

    // set interrupt priority mask to maximum to allow all priorities through
    ldr   r0, =FixedPcdGet32(PcdGicInterruptInterfaceBase)
    mov   r12, #0xFF
    str   r12, [r0, #ARM_GIC_OFFSET_ICCPMR]

    // set Primary core SVC stack location - 1KB stack at beginning of free OCRAM
    ldr sp, =(UDOOQUAD_OCRAM_SEC_STACKS_MEM + UDOOQUAD_OCRAM_SEC_STACKS_SIZE_PERCORE - 4)

    // do C setup, return value is jump address for monitor mode
    blx ASM_PFX(UdooQuadPrimaryCoreSecStart)

_Do_Monitor_Init:
    // store trusted monitor init address in r4
    mov r4, r0

    // initialize the secure monitor SPSR by jumping into it (errata?)
    mrs r0, cpsr
    isb 
    smc #0

    // should be back from secure monitor SPSR init here, in SVC-SECURE
    mov r0, sp      // save sp

    // switch to MONITOR-SECURE
    mrs r1, cpsr
    bic r12, r1, #A32_PSR_MODE_MASK
    orr r12, r12, #A32_PSR_MODE_MONITOR
    msr cpsr, r12      
    isb

    // call UINT32 xxxMonitorInit(UINT32 StackPointer, UINT32 SVC_SPSR, UINT32 MON_SPSR, UINT32 MON_CPSR);
    mrs r2, spsr
    mrs r3, cpsr
    mov sp, r0
    blx r4

    mov r4, r0
    // r4 is return address in nonsecure mode

    // set NSACR to allow CP10 and CP11 in NS mode
    mov r12, #0xC00
    mcr p15, 0, r12, c1, c1, 2  // write NSACR
    isb

    // leaving SEC here  - caches should still be OFF and EMPTY =============================================================

    // set SCR to be in NS mode
    mov r12, #0x30 // #0x31
    mcr p15, 0, r12, c1, c1, 0  //  write SCR
    isb

    // now return from exception to NonSecure-SVC
    adr lr, _Monitor_Return
    movs pc, lr
_Monitor_Return:
    bx r4
_Hang:
    b   _Hang

_SecondaryInit:
    // SCU setup
    mrc p15, 4, r0, c15, c0, 0      // read CBAR (PERIPHBASE)
    mov r12, #0                     // clear enable
    str r12, [r0]
    add r1, r0, #A9_SCU_INVALL_OFFSET   // set to INVALL offset 
    mvn r12, #0                  
    str r12, [r1]                   // invalidate all 
    mov r12, #1
    str r12, [r0]                   // set SCU enable
    add r1, r0, #A9_SCU_INVALL_OFFSET   // set to INVALL offset 
    mvn r12, #0                  
    str r12, [r1]                   // invalidate all 

    // set SMP mode
    mrc	p15, 0, r12, c1, c0, 1      // read ACTLR
    orr r12, r12, #(A32_ACTLR_SMP_MODE | A32_ACTRL_MAINT_BCAST)
    mcr p15, 0, r12, c1, c0, 1      // write ACTLR
    isb

    // set monitor vector table base address (MVBAR)
    adr r12, _MonitorResetEntry
    mcr p15, 0, r12, c12, c0, 1     // write MVBAR
    isb

    // set interrupt priority mask to maximum to allow all priorities through
    ldr   r0, =FixedPcdGet32(PcdGicInterruptInterfaceBase)
    mov   r12, #0xFF
    str   r12, [r0, #ARM_GIC_OFFSET_ICCPMR]

    // set secondary core stack location - 1KB each stack in SEC
    ldr r0, =(UDOOQUAD_OCRAM_SEC_STACKS_MEM + UDOOQUAD_OCRAM_SEC_STACKS_SIZE_PERCORE - 4)
    ldr r1, =UDOOQUAD_OCRAM_SEC_STACKS_SIZE_PERCORE
    mul r2, r1, r7
    add sp, r0, r2

    // put core index in first arg, stack pointer in second arg
    mov r0, r7
    mov r1, sp
    blx ASM_PFX(UdooQuadSecondaryCoreSecStart)

    b _Do_Monitor_Init

    // monitor vector table
.align 5
_MonitorResetEntry:
    b   _MonitorResetEntry
_MonitorUndefinedEntry:
    b   _MonitorUndefinedEntry
_MonitorSmcEntry:
    b   _MonitorCall
_MonitorPrefetchEntry:
    b   _MonitorPrefetchEntry
_MonitorDataAbortEntry:
    b   _MonitorDataAbortEntry
_MonitorReservedEntry:
    b   _MonitorReservedEntry
_MonitorIrqEntry:
    b   _MonitorIrqEntry
_MonitorFiqEntry:
    b   _MonitorFiqEntry

_MonitorCall:
    mrc p15, 0, r12, c1, c1, 0      // Read SCR
    tst r12, #1                     // Test NS bit - 0 means secure 
    bne _CallNotFromSecure
    // only call from secure is to initialize spsr to return to svc (above)
    msr spsr_cxsf, r0 
    isb
    movs pc, lr

_CallNotFromSecure:
    mrs r12, spsr 
    stmfd sp!, {r12, lr}
    blx  NonSecureSMC 
    ldmfd sp!, {r12, lr}
    msr spsr_cxsf, r12
    isb
    movs pc, lr

	// used by C code
ArmGetScuBaseAddress:
    mrc   p15, 4, r0, c15, c0, 0
    bx    lr
