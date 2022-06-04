#include "k2asma32.inc"
#include "..\VAGenericDef.inc"

A32_STARTUPTEXT

BEGIN_A32_PROC(ResetVector)
    //---------------------------------------------------------------
    // disable interrupts on this CPU interface
    //---------------------------------------------------------------
    ldr r8, =VIRTARM_PHYSADDR_CORTEXA9MP_PERCPUINT
    mov r1, #0
    str r1, [r8, #CORTEXA9MP_PERCPUINT_REGS_OFFSET_ICCICR]

    //---------------------------------------------------------------
    // Set SVC mode & disable IRQ/FIQ
    //---------------------------------------------------------------    
    mrs     r0, cpsr                        // Get current mode bits.
    bic     r0, r0, #0x1F                   // Clear mode bits.
    orr     r0, r0, #0xD3                   // Disable IRQs/FIQs, SVC mode
    msr     cpsr_c, r0                      // Enter supervisor mode

    //---------------------------------------------------------------
    // set domain access for domain 0 to client
    //---------------------------------------------------------------
    mov     r0, #1                  
    mcr     p15, 0, r0, c3, c0, 0 
    
    //---------------------------------------------------------------
    // set coprocessor access register to allow full access (011b) for cp10 and cp11 (VFP(s))
    //---------------------------------------------------------------
    mrc     p15, 0, r0, c1, c0, 2
    orr     r0, r0, #0x00F00000
    mcr     p15, 0, r0, c1, c0, 2

    //---------------------------------------------------------------
    // Flush all caches
    //---------------------------------------------------------------
    mov     r0, #0                          // setup up for MCR
    mcr     p15, 0, r0, c8, c7, 0           // invalidate TLBs
    mcr     p15, 0, r0, c7, c5, 0           // invalidate icache

    //---------------------------------------------------------------
    // Initialize CP15 control register
    //---------------------------------------------------------------
    mrc     p15, 0, r0, c1, c0, 0
    bic     r0, r0, #(1 << 13)           // Exception vector location (V bit) (0=normal)
    bic     r0, r0, #(1 << 12)           // I-cache (disabled)
    orr     r0, r0, #(1 << 11)           // Branch prediction (enabled)
    bic     r0, r0, #(1 << 2)            // D-cache (disabled - enabled within WinCE kernel startup)
    orr     r0, r0, #(1 << 1)            // alignment fault (enabled)
    bic     r0, r0, #(1 << 0)            // MMU (disabled - enabled within WinCE kernel startup)
    mcr     p15, 0, r0, c1, c0, 0

    //---------------------------------------------------------------
    // Get CPU id to R0
    //---------------------------------------------------------------
    mrc     p15, 0, r0, c0, c0, 5           // read multiprocessor id register
    and     r0, r0, #3                      // mask off CPU index
    
    //---------------------------------------------------------------
    // cpu index is at r0
    // fork to the right continuation point in the BSP
    //---------------------------------------------------------------
    cmp     r0, #0
    bne     StartCpuN

    ldr     r0,=VIRTARM_PHYSADDR_ADAPTER00_REGS
    ldr     r1,='0'
    str     r1, [r0, #VADEBUGADAPTER_OFFSET_DEBUGOUT]
     
    //---------------------------------------------------------------
    // copy bootloader from ROM to RAM
    //---------------------------------------------------------------    
    ldr r8, =VIRTARM_PHYSADDR_BOOTROM
    ldr r1, =VIRTARM_PHYSADDR_RAMBANK
    ldr r2, =(VAGENERIC_PHYSSIZE_BOOTROM / 16)
        
    // Do 4x32-bit block copies from flash->RAM (corrupts r4-r7).
blRamCopy:
    ldmia   r8!, {r4-r7}
    stmia   r1!, {r4-r7}
    subs    r2, r2, #1
    bne     blRamCopy

    // Verify that the copy succeeded by comparing the flash and RAM contents.
    ldr     r0, =VAGENERIC_PHYSSIZE_BOOTROM
    ldr     r1, =VIRTARM_PHYSADDR_RAMBANK
    ldr     r2, =VIRTARM_PHYSADDR_BOOTROM

    //---------------------------------------------------------------
    // verify copy
    //---------------------------------------------------------------    
blRamVerify:
    ldr     r3, [r1], #4        // Read longword from DRAM.
    ldr     r4, [r2], #4        // Read longword from flash.
    cmp     r3, r4              // Compare.
    bne     blRamFailed         // Not the same? Fail.
    subs    r0, r0, #4          //
    bne     blRamVerify         // Continue?
    b       blRamOk             // Done (success).

blRamFailed:
    // write to the debug port to say that we died
    ldr     r0,=VIRTARM_PHYSADDR_ADAPTER00_REGS
    ldr     r1,='B'
    str     r1, [r0, #VADEBUGADAPTER_OFFSET_DEBUGOUT]
    ldr     r1,='A'
    str     r1, [r0, #VADEBUGADAPTER_OFFSET_DEBUGOUT]
    ldr     r1,='D'
    str     r1, [r0, #VADEBUGADAPTER_OFFSET_DEBUGOUT]
    ldr     r1,=' '
    str     r1, [r0, #VADEBUGADAPTER_OFFSET_DEBUGOUT]
    ldr     r1,='R'
    str     r1, [r0, #VADEBUGADAPTER_OFFSET_DEBUGOUT]
    ldr     r1,='A'
    str     r1, [r0, #VADEBUGADAPTER_OFFSET_DEBUGOUT]
    ldr     r1,='M'
    str     r1, [r0, #VADEBUGADAPTER_OFFSET_DEBUGOUT]
blRamFailedLoop:
    b       blRamFailedLoop  // Spin forever.

    //---------------------------------------------------------------
    // jump to RAM image
    //---------------------------------------------------------------    
blRamOk:
    // Now that we have copied ourselves to RAM, jump to the RAM image.  Use the "CodeInRAM" label
    // to determine the RAM-based code address to which we should jump.
    add     r2, pc, #blCodeInRam-(.+8)   
    ldr     r3, =VIRTARM_PHYSADDR_BOOTROM       
    ldr     r1, =VIRTARM_PHYSADDR_RAMBANK
    sub     r1, r1, r3   
    add     r1, r1, r2                   
    mov     pc, r1                       
    nop
    nop
    nop

blCodeInRam:
    //---------------------------------------------------------------
    // executing in RAM at same address now
    //---------------------------------------------------------------    
    ldr     r0,=VIRTARM_PHYSADDR_ADAPTER00_REGS
    ldr     r1,='V'
    str     r1, [r0, #VADEBUGADAPTER_OFFSET_DEBUGOUT]
blSpinNow:
    ldr     r0,=0x80000200
    mov     pc, r0
    b       blSpinNow

StartCpuN:
    ldr     r0,=VIRTARM_PHYSADDR_ADAPTER00_REGS
    ldr     r1,='1'
    str     r1, [r0, #VADEBUGADAPTER_OFFSET_DEBUGOUT]
CpuNDies:
    b CpuNDies
        
END_A32_PROC(ResetVector)

    .end



