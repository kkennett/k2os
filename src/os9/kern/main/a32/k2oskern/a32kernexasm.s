#include "a32kernasm.inc"

.extern A32Kern_MountExceptionTrap

// BOOL K2_CALLCONV_REGS KernArch_ExTrapMount(K2_EXCEPTION_TRAP *apTrap);

BEGIN_A32_PROC(KernArch_ExTrapMount)
    // r0 has pointer to exception trap record

    // save state to context using r0 and put it back after
    add     r0, r0, #(8 + (16 * 4)) // bypass nextTrap and result in structure, move to end of register set
    stmda   r0!, {r14}              // store link register as pc 
    stmda   r0!, {r0-r14}           // store rest of registers to structure
    mrs     r1, cpsr
    isb
    str     r1, [r0]
    sub     r0, r0, #8 

    // save interrupt bits status
    and     r2, r1, #(A32_PSR_F_BIT | A32_PSR_I_BIT)

    // make sure interrupts are masked
    orr     r1, r1, #(A32_PSR_F_BIT | A32_PSR_I_BIT)
    msr     cpsr, r1

    // save stuff we want back again on the stack
    stmfd   r13!, {r2, r12, lr}

    // jump to C routine
    mov     lr, pc
    b       A32Kern_MountExceptionTrap

    // back from C routine. restore goop we stored on the stack
    ldmfd   r13!, {r2, r12, lr}

    // restore interrupts bits status 
    mrs     r1, cpsr
    bic     r1, r1, #(A32_PSR_F_BIT | A32_PSR_I_BIT)
    orr     r1, r1, r2 
    msr     cpsr, r1

    // force return value as zero and return to caller
    mov     r0, #0
    mov     pc, lr

END_A32_PROC(KernArch_ExTrapMount)

    .end


