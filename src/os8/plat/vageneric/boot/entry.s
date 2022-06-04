#include "k2asma32.inc"

A32_STARTUPTEXT

BEGIN_A32_PROC(ResetVector)
    mov  r3, #0x1000
    mul  r3, r3, r0
    add  r1, r1, r3

    // r1 points to bottom of core page
    mov  r3, #0x800
    add  sp, r1, r3
    sub  sp, sp, #4
       
END_A32_PROC(ResetVector)

    .end


