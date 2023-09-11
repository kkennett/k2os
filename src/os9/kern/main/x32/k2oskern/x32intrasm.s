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

// void K2_CALLCONV_REGS X32KernAsm_SysEnter_Entry(void);
BEGIN_X32_PROC(X32KernAsm_SysEnter_Entry)
    //
    // eax contains the return ESP in user mode
    // ecx contains call argument (will change to return ESP in fast system call)
    // edx contains call argument (will change to return EIP in fast system call)
    //
    cmp %ecx, 0xFF          // ecx is system call number (encoded)
    jnz notFF               // if it is not 0xFF it is not a fast call 
    mov %ecx, %eax          // set return stack pointer
    mov %edx, 0x7FFFF004    // set fixed return code address 

    //
    // for indexing into per-core data array. edx is unused argument so we can use it here 
    //
    mov %eax, %ss:[K2OS_KVA_X32_LOCAPIC + X32_LOCAPIC_OFFSET_ID]
    shr %eax, 21    // 24 would be to bit pos 0, but we want it at bit pos 3 = multiplied by 8 
    and %eax, 0x38  // 111000 - 8 processors and knock out the lowest 3 bits 
    add %eax, (X32_SELECTOR_TI_LDT | X32_SELECTOR_RPL_KERNEL)
    mov %fs, %ax 

    //
    // set system call result to per-core thread index value 
    //
    mov %eax, %fs:[0] 

    sysexit                 // fast return to user mode

notFF:
    // CS is kernel code segment 
    // SS is kernel data segment. ************* DS IS NOT ******************
    // ESP is bottom of kernel core stack.  
    // 
    push (X32_SEGMENT_SELECTOR_USER_DATA | X32_SELECTOR_RPL_USER)   // SS on return to user mode
    push %eax                                                       // ESP on return to user mode
    pushf                                                           // EFLAGS on return to user mode (before fixup)
    push (X32_SEGMENT_SELECTOR_USER_CODE | X32_SELECTOR_RPL_USER)   // CS on return to user mode
    push 0x7FFFF004                                                 // EIP on return to user mode
    push 0                                                          // error code slot
    push 0xFF                                                       // vector slot (int 255 is reserved to denote system call) 
    pusha                                                           // all user mode registers
    push (X32_SEGMENT_SELECTOR_USER_DATA | X32_SELECTOR_RPL_USER)   // DS on return to user mode

    //
    // fix up kernel mode ebp now that user mode ebp is save above
    // 
    mov %ebp, %esp  
    add %ebp, X32_SIZEOF_USER_EXCEPTION_CONTEXT

    //
    // set other segments to kernel data that SYSENTER did not set 
    //
    mov %ax, (X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_KERNEL)
    mov %ds, %ax
    mov %es, %ax
    mov %gs, %ax

    //
    // full into kernel now - can see kernel data
    //

    //
    // push return address of raw interrupt return so that
    // returning from X32Kern_InterruptHandler goes back to there
    // and not here.  then jump rather than call interrupt handler
    push offset X32KernAsm_RawInterruptReturn
.extern X32Kern_InterruptHandler
    jmp X32Kern_InterruptHandler

END_X32_PROC(X32KernAsm_SysEnter_Entry)

//void K2_CALLCONV_REGS X32KernAsm_IntrIdle(UINT32 aCoreStackBottom);
BEGIN_X32_PROC(X32KernAsm_IntrIdle)
    mov %esp, %ecx 
    mov %ebp, 0
    sti 
CpuIdleLoop:
    hlt
    //
    // should never get here since we should interrupt out of this. 
    // but if we do for some reason we should jump back to do it again
    //
    jmp CpuIdleLoop 
END_X32_PROC(X32KernAsm_IntrIdle)

BEGIN_X32_PROC(X32KernAsm_RawInterruptReturn)
   pop %eax 
   mov %ds, %ax
   mov %es, %ax
   mov %gs, %ax

   popa
   add %esp, 8 
   
   iret
END_X32_PROC(X32KernAsm_RawInterruptReturn)

//void K2_CALLCONV_REGS X32KernAsm_InterruptReturn(UINT32 aContextAddr);
BEGIN_X32_PROC(X32KernAsm_InterruptReturn)
    mov %esp, %ecx
    jmp X32KernAsm_RawInterruptReturn
END_X32_PROC(X32KernAsm_InterruptReturn)

//void K2_CALLCONV_REGS X32KernAsm_KernelExTrapReturn(X32_EXCEPTION_CONTEXT *apSrcContext, UINT32 aTargetESP);
BEGIN_X32_PROC(X32KernAsm_KernelExTrapReturn)
    // set the new stack address at the new return context
    mov %esp, %edx

    // move the context down in the stack (up in memory)
    // need to move backwards
    mov %esi, %ecx
    add %esi, (X32_SIZEOF_KERN_EXCEPTION_CONTEXT-4)
    mov %edi, %edx
    add %edi, (X32_SIZEOF_KERN_EXCEPTION_CONTEXT-4)
    mov %ecx, (X32_SIZEOF_KERN_EXCEPTION_CONTEXT >> 2)
    std
    rep movsd
    cld

    jmp X32KernAsm_RawInterruptReturn
END_X32_PROC(X32KernAsm_KernelExTrapReturn)

BEGIN_X32_STATIC_PROC(sInterruptCommon)
   pusha

   xor %eax, %eax
   mov %ax, %ds
   push %eax

   mov %ax, (X32_SEGMENT_SELECTOR_KERNEL_DATA | X32_SELECTOR_RPL_KERNEL)
   mov %ds, %ax
   mov %es, %ax
   mov %gs, %ax

.extern X32Kern_InterruptHandler
   push offset X32KernAsm_RawInterruptReturn
   jmp X32Kern_InterruptHandler
           
END_X32_PROC(sInterruptCommon)

.macro INTR_WITHERR intNum
    cli                                 /* FA        */
    push    \intNum                     /* 6A ##    */
    .byte   0xE9                        /* E9        */
    .long   (sInterruptCommon-(.+4))    /* ## ## ## ##     */
    nop                                 /* 90        */
    nop                                 /* 90        */
    nop                                 /* 90        */
    nop                                 /* 90        */
.endm                                   /*  12 bytes */

.macro INTR_FAKEERR intNum
    cli                                 /* FA        */
    push    0                           /* 6A 00    */
    push    \intNum                     /* 6A ##    */
    .byte   0xE9                        /* E9        */
    .long   (sInterruptCommon-(.+4))    /* ## ## ## ##    */
    nop                                 /* 90        */
    nop                                 /* 90        */
.endm                                   /*  12 bytes */

BEGIN_X32_PROC(X32KernAsm_InterruptStubs)
INTR_FAKEERR 0
INTR_FAKEERR 1
INTR_FAKEERR 2
INTR_FAKEERR 3
INTR_FAKEERR 4
INTR_FAKEERR 5
INTR_FAKEERR 6
INTR_FAKEERR 7

INTR_WITHERR 8
INTR_WITHERR 9
INTR_WITHERR 10
INTR_WITHERR 11
INTR_WITHERR 12
INTR_WITHERR 13
INTR_WITHERR 14

INTR_FAKEERR 15
INTR_FAKEERR 16
INTR_FAKEERR 17
INTR_FAKEERR 18
INTR_FAKEERR 19
INTR_FAKEERR 20
INTR_FAKEERR 21
INTR_FAKEERR 22
INTR_FAKEERR 23
INTR_FAKEERR 24
INTR_FAKEERR 25
INTR_FAKEERR 26
INTR_FAKEERR 27
INTR_FAKEERR 28
INTR_FAKEERR 29
INTR_FAKEERR 30
INTR_FAKEERR 31
INTR_FAKEERR 32
INTR_FAKEERR 33
INTR_FAKEERR 34
INTR_FAKEERR 35
INTR_FAKEERR 36
INTR_FAKEERR 37
INTR_FAKEERR 38
INTR_FAKEERR 39
INTR_FAKEERR 40
INTR_FAKEERR 41
INTR_FAKEERR 42
INTR_FAKEERR 43
INTR_FAKEERR 44
INTR_FAKEERR 45
INTR_FAKEERR 46
INTR_FAKEERR 47
INTR_FAKEERR 48
INTR_FAKEERR 49
INTR_FAKEERR 50
INTR_FAKEERR 51
INTR_FAKEERR 52
INTR_FAKEERR 53
INTR_FAKEERR 54
INTR_FAKEERR 55
INTR_FAKEERR 56
INTR_FAKEERR 57
INTR_FAKEERR 58
INTR_FAKEERR 59
INTR_FAKEERR 60
INTR_FAKEERR 61
INTR_FAKEERR 62
INTR_FAKEERR 63
INTR_FAKEERR 64
INTR_FAKEERR 65
INTR_FAKEERR 66
INTR_FAKEERR 67
INTR_FAKEERR 68
INTR_FAKEERR 69
INTR_FAKEERR 70
INTR_FAKEERR 71
INTR_FAKEERR 72
INTR_FAKEERR 73
INTR_FAKEERR 74
INTR_FAKEERR 75
INTR_FAKEERR 76
INTR_FAKEERR 77
INTR_FAKEERR 78
INTR_FAKEERR 79
INTR_FAKEERR 80
INTR_FAKEERR 81
INTR_FAKEERR 82
INTR_FAKEERR 83
INTR_FAKEERR 84
INTR_FAKEERR 85
INTR_FAKEERR 86
INTR_FAKEERR 87
INTR_FAKEERR 88
INTR_FAKEERR 89
INTR_FAKEERR 90
INTR_FAKEERR 91
INTR_FAKEERR 92
INTR_FAKEERR 93
INTR_FAKEERR 94
INTR_FAKEERR 95
INTR_FAKEERR 96
INTR_FAKEERR 97
INTR_FAKEERR 98
INTR_FAKEERR 99
INTR_FAKEERR 100
INTR_FAKEERR 101
INTR_FAKEERR 102
INTR_FAKEERR 103
INTR_FAKEERR 104
INTR_FAKEERR 105
INTR_FAKEERR 106
INTR_FAKEERR 107
INTR_FAKEERR 108
INTR_FAKEERR 109
INTR_FAKEERR 110
INTR_FAKEERR 111
INTR_FAKEERR 112
INTR_FAKEERR 113
INTR_FAKEERR 114
INTR_FAKEERR 115
INTR_FAKEERR 116
INTR_FAKEERR 117
INTR_FAKEERR 118
INTR_FAKEERR 119
INTR_FAKEERR 120
INTR_FAKEERR 121
INTR_FAKEERR 122
INTR_FAKEERR 123
INTR_FAKEERR 124
INTR_FAKEERR 125
INTR_FAKEERR 126
INTR_FAKEERR 127
INTR_FAKEERR 128
INTR_FAKEERR 129
INTR_FAKEERR 130
INTR_FAKEERR 131
INTR_FAKEERR 132
INTR_FAKEERR 133
INTR_FAKEERR 134
INTR_FAKEERR 135
INTR_FAKEERR 136
INTR_FAKEERR 137
INTR_FAKEERR 138
INTR_FAKEERR 139
INTR_FAKEERR 140
INTR_FAKEERR 141
INTR_FAKEERR 142
INTR_FAKEERR 143
INTR_FAKEERR 144
INTR_FAKEERR 145
INTR_FAKEERR 146
INTR_FAKEERR 147
INTR_FAKEERR 148
INTR_FAKEERR 149
INTR_FAKEERR 150
INTR_FAKEERR 151
INTR_FAKEERR 152
INTR_FAKEERR 153
INTR_FAKEERR 154
INTR_FAKEERR 155
INTR_FAKEERR 156
INTR_FAKEERR 157
INTR_FAKEERR 158
INTR_FAKEERR 159
INTR_FAKEERR 160
INTR_FAKEERR 161
INTR_FAKEERR 162
INTR_FAKEERR 163
INTR_FAKEERR 164
INTR_FAKEERR 165
INTR_FAKEERR 166
INTR_FAKEERR 167
INTR_FAKEERR 168
INTR_FAKEERR 169
INTR_FAKEERR 170
INTR_FAKEERR 171
INTR_FAKEERR 172
INTR_FAKEERR 173
INTR_FAKEERR 174
INTR_FAKEERR 175
INTR_FAKEERR 176
INTR_FAKEERR 177
INTR_FAKEERR 178
INTR_FAKEERR 179
INTR_FAKEERR 180
INTR_FAKEERR 181
INTR_FAKEERR 182
INTR_FAKEERR 183
INTR_FAKEERR 184
INTR_FAKEERR 185
INTR_FAKEERR 186
INTR_FAKEERR 187
INTR_FAKEERR 188
INTR_FAKEERR 189
INTR_FAKEERR 190
INTR_FAKEERR 191
INTR_FAKEERR 192
INTR_FAKEERR 193
INTR_FAKEERR 194
INTR_FAKEERR 195
INTR_FAKEERR 196
INTR_FAKEERR 197
INTR_FAKEERR 198
INTR_FAKEERR 199
INTR_FAKEERR 200
INTR_FAKEERR 201
INTR_FAKEERR 202
INTR_FAKEERR 203
INTR_FAKEERR 204
INTR_FAKEERR 205
INTR_FAKEERR 206
INTR_FAKEERR 207
INTR_FAKEERR 208
INTR_FAKEERR 209
INTR_FAKEERR 210
INTR_FAKEERR 211
INTR_FAKEERR 212
INTR_FAKEERR 213
INTR_FAKEERR 214
INTR_FAKEERR 215
INTR_FAKEERR 216
INTR_FAKEERR 217
INTR_FAKEERR 218
INTR_FAKEERR 219
INTR_FAKEERR 220
INTR_FAKEERR 221
INTR_FAKEERR 222
INTR_FAKEERR 223
INTR_FAKEERR 224
INTR_FAKEERR 225
INTR_FAKEERR 226
INTR_FAKEERR 227
INTR_FAKEERR 228
INTR_FAKEERR 229
INTR_FAKEERR 230
INTR_FAKEERR 231
INTR_FAKEERR 232
INTR_FAKEERR 233
INTR_FAKEERR 234
INTR_FAKEERR 235
INTR_FAKEERR 236
INTR_FAKEERR 237
INTR_FAKEERR 238
INTR_FAKEERR 239
INTR_FAKEERR 240
INTR_FAKEERR 241
INTR_FAKEERR 242
INTR_FAKEERR 243
INTR_FAKEERR 244
INTR_FAKEERR 245
INTR_FAKEERR 246
INTR_FAKEERR 247
INTR_FAKEERR 248
INTR_FAKEERR 249
INTR_FAKEERR 250
INTR_FAKEERR 251
INTR_FAKEERR 252
INTR_FAKEERR 253
INTR_FAKEERR 254
INTR_FAKEERR 255
END_X32_PROC(X32KernAsm_InterruptStubs)

    .end
