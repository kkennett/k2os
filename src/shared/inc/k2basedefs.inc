/*   
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
*/
#ifndef __K2BASEDEFS_INC
#define __K2BASEDEFS_INC

//
//------------------------------------------------------------------------
//

#define K2_ARCH_NONE        0
#define K2_ARCH_X32         1
#define K2_ARCH_A32         2
#define K2_ARCH_X64         3
#define K2_ARCH_A64         4

#ifdef _MSC_VER

#define K2_TOOLCHAIN_IS_MS  1
#define K2_TOOLCHAIN_IS_GCC 0

#if (defined(_M_IX86) || defined(_M_X64))

#ifdef _M_X64
// build system and 64-bit tools use this
#define K2_ARCH                 K2_ARCH_X64
#define K2_TARGET_ARCH_IS_INTEL 1
#define K2_TARGET_ARCH_IS_ARM   0
#define K2_TARGET_ARCH_IS_32BIT 0
#define K2_TARGET_ARCH_IS_64BIT 1
#else
#define K2_ARCH                 K2_ARCH_X32
#define K2_TARGET_ARCH_IS_INTEL 1
#define K2_TARGET_ARCH_IS_ARM   0
#define K2_TARGET_ARCH_IS_32BIT 1
#define K2_TARGET_ARCH_IS_64BIT 0
#endif

#else

#ifdef _M_A64
#define K2_ARCH                 K2_ARCH_A64
#define K2_TARGET_ARCH_IS_INTEL 0
#define K2_TARGET_ARCH_IS_ARM   1
#define K2_TARGET_ARCH_IS_32BIT 0
#define K2_TARGET_ARCH_IS_64BIT 1
#else
#define K2_ARCH                 K2_ARCH_A32
#define K2_TARGET_ARCH_IS_INTEL 0
#define K2_TARGET_ARCH_IS_ARM   1
#define K2_TARGET_ARCH_IS_32BIT 1
#define K2_TARGET_ARCH_IS_64BIT 0
#endif
#endif

#if defined(NDEBUG)
#define K2_FINAL 1
#undef K2_DEBUG
#else
#define K2_DEBUG 1
#undef K2_FINAL
#endif

#else

#define K2_TOOLCHAIN_IS_MS  0  
#define K2_TOOLCHAIN_IS_GCC 1

#ifdef __arm__
#if (__SIZEOF_POINTER__==8)
#define K2_ARCH                 K2_ARCH_A64
#define K2_TARGET_ARCH_IS_INTEL 0
#define K2_TARGET_ARCH_IS_ARM   1
#define K2_TARGET_ARCH_IS_32BIT 0
#define K2_TARGET_ARCH_IS_64BIT 1
#else
#define K2_ARCH                 K2_ARCH_A32
#define K2_TARGET_ARCH_IS_INTEL 0
#define K2_TARGET_ARCH_IS_ARM   1
#define K2_TARGET_ARCH_IS_32BIT 1
#define K2_TARGET_ARCH_IS_64BIT 0
#endif
#else
#if (__SIZEOF_POINTER__==8)
#define K2_ARCH                 K2_ARCH_X64
#define K2_TARGET_ARCH_IS_INTEL 1
#define K2_TARGET_ARCH_IS_ARM   0
#define K2_TARGET_ARCH_IS_32BIT 0
#define K2_TARGET_ARCH_IS_64BIT 1
#else
#define K2_ARCH                 K2_ARCH_X32
#define K2_TARGET_ARCH_IS_INTEL 1
#define K2_TARGET_ARCH_IS_ARM   0
#define K2_TARGET_ARCH_IS_32BIT 1
#define K2_TARGET_ARCH_IS_64BIT 0
#endif
#endif

#if !defined(K2_DEBUG) && !defined(K2_FINAL)
#define K2_DEBUG 1
#endif

#endif

#if !defined(K2_DEBUG)
#if !defined(K2_FINAL)
#error Build mode not set. Either define K2_DEBUG=1 or K2_FINAL=1 to compiler/assembler
#endif
#else
#if defined(K2_FINAL)
#error Both K2_DEBUG AND K2_FINAL defined 
#endif
#endif

//
// argument passing direction markers
#define IN 
#define OUT 
#define INOUT

//
//------------------------------------------------------------------------
//

#define X32_EFLAGS_CARRY                0x00000001
#define X32_EFLAGS_SBO                  0x00000002
#define X32_EFLAGS_PARITY               0x00000004
#define X32_EFLAGS_AUXCARRY             0x00000010
#define X32_EFLAGS_ZERO                 0x00000040
#define X32_EFLAGS_SIGN                 0x00000080
#define X32_EFLAGS_TRAP                 0x00000100
#define X32_EFLAGS_INTENABLE            0x00000200
#define X32_EFLAGS_DIRECTION            0x00000400
#define X32_EFLAGS_OVERFLOW             0x00000800
#define X32_EFLAGS_IOPL_MASK            0x00003000
#define X32_EFLAGS_NESTED_TASK          0x00004000
#define X32_EFLAGS_RESUME               0x00010000
#define X32_EFLAGS_ALIGNMENT            0x00040000
#define X32_EFLAGS_IDENT                0x00200000

#define X32_SIZEOF_GDTENTRY             8
#define X32_SIZEOF_IDTENTRY             8

#define X32_CR0_PM_ENABLE               0x00000001
#define X32_CR0_COPROC_PRESENT          0x00000002
#define X32_CR0_FPEMUL                  0x00000004
#define X32_CR0_TASK_SWITCHED           0x00000008
#define X32_CR0_EXTENSION               0x00000010
#define X32_CR0_FPERR_ENABLE            0x00000020
#define X32_CR0_WRITE_PROTECT           0x00010000
#define X32_CR0_ALIGN_MASK              0x00040000
#define X32_CR0_PAGE_NOT_WRITE_THRU     0x20000000
#define X32_CR0_PAGE_CACHE_DISABLE      0x40000000
#define X32_CR0_PAGING_ENABLE           0x80000000

#define X32_CR3_WRITETHROUGH            0x00000008
#define X32_CR3_CACHEDISABLE            0x00000010
#define X32_CR3_DIRPHYS_MASK            0xFFFFF000
#define X32_CR3_RESERVED_MASK           0x00000FE3

#define X32_PAGEDIR_PHYS_SIZE           0x00001000

#define X32_PDE_PRESENT                 0x00000001
#define X32_PDE_WRITEABLE               0x00000002
#define X32_PDE_USER                    0x00000004
#define X32_PDE_WRITETHROUGH            0x00000008
#define X32_PDE_CACHEDISABLE            0x00000010
#define X32_PDE_ACCESSED                0x00000020
#define X32_PDE_AVAIL_040               0x00000040
#define X32_PDE_MUST_BE_ZERO            0x00000080
#define X32_PDE_AVAIL_100               0x00000100
#define X32_PDE_AVAIL_200               0x00000200
#define X32_PDE_AVAIL_400               0x00000400
#define X32_PDE_AVAIL_800               0x00000800
#define X32_PDE_PTPHYS_MASK             0xFFFFF000

#define X32_KERN_PAGETABLE_PROTO        (X32_PDE_PRESENT | X32_PDE_WRITEABLE)
#define X32_USER_PAGETABLE_PROTO        (X32_PDE_PRESENT | X32_PDE_WRITEABLE | X32_PDE_USER)

#define X32_PTE_PRESENT                 0x00000001
#define X32_PTE_WRITEABLE               0x00000002
#define X32_PTE_USER                    0x00000004
#define X32_PTE_WRITETHROUGH            0x00000008
#define X32_PTE_CACHEDISABLE            0x00000010
#define X32_PTE_ACCESSED                0x00000020
#define X32_PTE_DIRTY                   0x00000040
#define X32_PTE_PAT                     0x00000080
#define X32_PTE_GLOBAL                  0x00000100
#define X32_PTE_AVAIL_200               0x00000200
#define X32_PTE_AVAIL_400               0x00000400
#define X32_PTE_AVAIL_800               0x00000800
#define X32_PTE_PAGEPHYS_MASK           0xFFFFF000

#define X32_MSRNUM_APIC_BASE            0x01B
#define X32_MSRNUM_SYSENTER_CS          0x174
#define X32_MSRNUM_SYSENTER_ESP         0x175
#define X32_MSRNUM_SYSENTER_EIP         0x176
#define X32_MSRNUM_BUS_FREQ             0x0CD
#define X32_MSRNUM_PERF_STAT            0x198
#define X32_MSRNUM_PLATFORM_ID          0x017
#define X32_MSRNUM_PLATFORM_INFO        0x0CE

#define X32_SELECTOR_RPL_KERNEL         0
#define X32_SELECTOR_RPL_USER           3

#define X32_CPUID1_MASK_STEPPING        0x0000000F
#define X32_CPUID1_SHFT_STEPPING        0
#define X32_CPUID1_MASK_MODEL           0x000000F0
#define X32_CPUID1_SHFT_MODEL           4
#define X32_CPUID1_MASK_FAMILY          0x00000F00
#define X32_CPUID1_SHFT_FAMILY          8
#define X32_CPUID1_MASK_TYPE            0x00003000
#define X32_CPUID1_SHFT_TYPE            12
#define X32_CPUID1_MASK_EXMODEL         0x000F0000
#define X32_CPUID1_SHFT_EXMODEL         16
#define X32_CPUID1_MASK_EXFAMILY        0x0FF00000
#define X32_CPUID1_SHFT_EXFAMILY        20

//
// fault    precise, before instruction, PC = instruction
// trap     precise, after instruction completed, PC = after instruction
// abort    imprecise, unrecoverable
//

#define X32_EX_DIVIDE_BY_ZERO           0   // fault
#define X32_EX_DEBUG                    1   // instruction=fault, data+single-step=trap
#define X32_EX_NMI                      2   // trap
#define X32_EX_BREAKPOINT               3   // trap
#define X32_EX_OVERFLOW                 4   // trap
#define X32_EX_BOUNDS                   5   // fault
#define X32_EX_INVALID_OPCODE           6   // fault
#define X32_EX_FPU_NOT_AVAILABLE        7   // fault
#define X32_EX_DOUBLE_FAULT             8   // abort
#define X32_EX_FPU_OVERRUN              9   // abort
#define X32_EX_INVALID_TSS              10  // fault
#define X32_EX_SEGMENT_NOT_PRESENT      11  // fault
#define X32_EX_STACK_FAULT              12  // fault
#define X32_EX_GENERAL                  13  // fault or trap
#define X32_EX_PAGE_FAULT               14  // fault
#define X32_EX_FPU_ERROR                16  // fault
#define X32_EX_ALIGNMENT                17  // fault
#define X32_EX_MACHINE_CHECK            18  // abort

//
// generic exception error code format
//
#define X32_EX_ERRORCODE_EXT            0x00000001
#define X32_EX_ERRORCODE_IDT            0x00000002
#define X32_EX_ERRORCODE_TI             0x00000004
#define X32_EX_ERRORCODE_SEL_IX         0x0000FFF8

//
// page fault reason (error code)
//
#define X32_EX_PAGE_FAULT_PRESENT       0x00000001  // Present bit was set
#define X32_EX_PAGE_FAULT_ON_WRITE      0x00000002  // write attempted to non-writable address
#define X32_EX_PAGE_FAULT_FROM_USER     0x00000004  // execution was in user mode


//
// generic system definitions for IDT,GDT,LDT
//
#define X32_MAX_CPU_COUNT                       8

#define X32_NUM_IDT_ENTRIES                     256

#define X32_SEGMENT_KERNEL_CODE                 0x01
#define X32_SEGMENT_KERNEL_DATA                 0x02    // must be right after KERNEL_CODE (for SYSENTER/SYSEXIT)
#define X32_SEGMENT_USER_CODE                   0x03    // must be right after KERNEL_DATA (for SYSENTER/SYSEXIT)
#define X32_SEGMENT_USER_DATA                   0x04    // must be right after USER_CODE   (for SYSENTER/SYSEXIT)
#define X32_SEGMENT_USER_LDT                    0x05
#define X32_SEGMENT_TSS0                        0x06
#define X32_NUM_SEGMENTS                        (X32_SEGMENT_TSS0 + X32_MAX_CPU_COUNT)

#define X32_SEGMENT_SELECTOR_KERNEL_CODE        (X32_SEGMENT_KERNEL_CODE     * X32_SIZEOF_GDTENTRY)
#define X32_SEGMENT_SELECTOR_KERNEL_DATA        (X32_SEGMENT_KERNEL_DATA     * X32_SIZEOF_GDTENTRY)
#define X32_SEGMENT_SELECTOR_USER_CODE          (X32_SEGMENT_USER_CODE       * X32_SIZEOF_GDTENTRY)
#define X32_SEGMENT_SELECTOR_USER_DATA          (X32_SEGMENT_USER_DATA       * X32_SIZEOF_GDTENTRY)
#define X32_SEGMENT_SELECTOR_USER_LDT           (X32_SEGMENT_USER_LDT        * X32_SIZEOF_GDTENTRY)
#define X32_SEGMENT_SELECTOR_TSS(ix)            ((X32_SEGMENT_TSS0 + (ix))   * X32_SIZEOF_GDTENTRY)

#define X32_SELECTOR_RPL_KERNEL                 0
#define X32_SELECTOR_RPL_USER                   3

#define X32_SELECTOR_TI_LDT                     0x0004

//
// IO APIC
//

#define X32_IOAPIC_OFFSET_IOREGSEL      0x00
#define X32_IOAPIC_OFFSET_IOWIN         0x10
#define X32_IOAPIC_REGIX_IOAPICID       0x00
#define X32_IOAPIC_REGIX_IOAPICVER      0x01
#define X32_IOAPIC_REGIX_IOAPICARB      0x02
#define X32_IOAPIC_REGIX_REDTBL0        0x10
#define X32_IOAPIC_REGIX_REDLO(x)       (X32_IOAPIC_REGIX_REDTBL0 + (2 * (x)))
#define X32_IOAPIC_REGIX_REDHI(x)       (X32_IOAPIC_REGIX_REDTBL0 + ((2 * (x))+1))

#define X32_IOAPIC_REDHI_DEST_MASK      0xFF000000
#define X32_IOAPIC_REDHI_DEST_SHIFT     24

#define X32_IOAPIC_REDLO_MASK           0x00010000
#define X32_IOAPIC_REDLO_TRIGGER_EDGE   0x00000000
#define X32_IOAPIC_REDLO_TRIGGER_LEVEL  0x00008000
#define X32_IOAPIC_REDLO_REMOTE         0x00004000
#define X32_IOAPIC_REDLO_POLARITY_HIGH  0x00000000
#define X32_IOAPIC_REDLO_POLARITY_LOW   0x00002000
#define X32_IOAPIC_REDLO_DEL_STATUS     0x00001000
#define X32_IOAPIC_REDLO_DEST_PHYS      0x00000000
#define X32_IOAPIC_REDLO_DEST_LOG       0x00000800
#define X32_IOAPIC_REDLO_MODE_MASK      0x00000700
#define X32_IOAPIC_REDLO_MODE_FIXED     0x00000000
#define X32_IOAPIC_REDLO_MODE_LOWPRI    0x00000100
#define X32_IOAPIC_REDLO_MODE_SMI       0x00000200
#define X32_IOAPIC_REDLO_MODE_NMI       0x00000400
#define X32_IOAPIC_REDLO_MODE_INIT      0x00000500
#define X32_IOAPIC_REDLO_MODE_EXTINT    0x00000700
#define X32_IOAPIC_REDLO_VECTOR_MASK    0x000000FF

//
// Local APIC
//

#define X32_LOCAPIC_OFFSET_ID               0x0020
#define X32_LOCAPIC_OFFSET_VERSION          0x0030
#define X32_LOCAPIC_OFFSET_TPR              0x0080  // RW Task Priority
#define X32_LOCAPIC_OFFSET_PPR              0x00A0  // R  Processor Priority
#define X32_LOCAPIC_OFFSET_EOI              0x00B0  //  W End of interrupt
#define X32_LOCAPIC_OFFSET_LDR              0x00D0  // RW Logical Destination
#define X32_LOCAPIC_OFFSET_DFR              0x00E0  // RW Destination Format
#define X32_LOCAPIC_OFFSET_SPUR             0x00F0  // RW Spurious
#define X32_LOCAPIC_OFFSET_ISR_0            0x0100  // R  In-Service
#define X32_LOCAPIC_OFFSET_ISR_32           0x0110
#define X32_LOCAPIC_OFFSET_ISR_64           0x0120
#define X32_LOCAPIC_OFFSET_ISR_96           0x0130
#define X32_LOCAPIC_OFFSET_ISR_128          0x0140
#define X32_LOCAPIC_OFFSET_ISR_160          0x0150
#define X32_LOCAPIC_OFFSET_ISR_192          0x0160
#define X32_LOCAPIC_OFFSET_ISR_224          0x0170
#define X32_LOCAPIC_OFFSET_TMR_0            0x0180  // R  Trigger Mode
#define X32_LOCAPIC_OFFSET_TMR_32           0x0190
#define X32_LOCAPIC_OFFSET_TMR_64           0x01A0
#define X32_LOCAPIC_OFFSET_TMR_96           0x01B0
#define X32_LOCAPIC_OFFSET_TMR_128          0x01C0
#define X32_LOCAPIC_OFFSET_TMR_160          0x01D0
#define X32_LOCAPIC_OFFSET_TMR_192          0x01E0
#define X32_LOCAPIC_OFFSET_TMR_224          0x01F0
#define X32_LOCAPIC_OFFSET_IRR_0            0x0200  // R  Interrupt request
#define X32_LOCAPIC_OFFSET_IRR_32           0x0210
#define X32_LOCAPIC_OFFSET_IRR_64           0x0220
#define X32_LOCAPIC_OFFSET_IRR_96           0x0230
#define X32_LOCAPIC_OFFSET_IRR_128          0x0240
#define X32_LOCAPIC_OFFSET_IRR_160          0x0250
#define X32_LOCAPIC_OFFSET_IRR_192          0x0260
#define X32_LOCAPIC_OFFSET_IRR_224          0x0270
#define X32_LOCAPIC_OFFSET_ERRSTAT          0x0280  // R  Error Status
#define X32_LOCAPIC_OFFSET_LVT_CMCI         0x02F0  // RW
#define X32_LOCAPIC_OFFSET_ICR_LOW32        0x0300  // RW Interrupt Command Low  
#define X32_LOCAPIC_OFFSET_ICR_HIGH32       0x0310  // RW Interrupt Command High
#define X32_LOCAPIC_OFFSET_LVT_TIMER        0x0320  // RW
#define X32_LOCAPIC_OFFSET_LVT_THERM        0x0330  // RW
#define X32_LOCAPIC_OFFSET_LVT_PERF         0x0340  // RW
#define X32_LOCAPIC_OFFSET_LVT_LINT0        0x0350  // RW
#define X32_LOCAPIC_OFFSET_LVT_LINT1        0x0360  // RW
#define X32_LOCAPIC_OFFSET_LVT_ERROR        0x0370  // RW
#define X32_LOCAPIC_OFFSET_TIMER_INIT       0x0380  // RW
#define X32_LOCAPIC_OFFSET_TIMER_CURR       0x0390  // R
#define X32_LOCAPIC_OFFSET_TIMER_DIV        0x03E0  // RW

#define X32_LOCAPIC_ICR_HIGH_DESTMASK       0xFF000000
#define X32_LOCAPIC_ICR_HIGH_DESTSHIFT      24
#define X32_LOCAPIC_ICR_LOW_DESTSH_NONE     0x00000000
#define X32_LOCAPIC_ICR_LOW_DESTSH_SELF     0x00040000
#define X32_LOCAPIC_ICR_LOW_DESTSH_ALL      0x00080000
#define X32_LOCAPIC_ICR_LOW_DESTSH_ALLEXME  0x000C0000
#define X32_LOCAPIC_ICR_LOW_TRIGGER_EDGE    0x00000000
#define X32_LOCAPIC_ICR_LOW_TRIGGER_LEVEL   0x00008000
#define X32_LOCAPIC_ICR_LOW_LEVEL_DEASSERT  0x00000000
#define X32_LOCAPIC_ICR_LOW_LEVEL_ASSERT    0x00004000
#define X32_LOCAPIC_ICR_LOW_DELIVER_STATUS  0x00001000
#define X32_LOCAPIC_ICR_LOW_PHYSICAL        0x00000000
#define X32_LOCAPIC_ICR_LOW_LOGICAL         0x00000800
#define X32_LOCAPIC_ICR_LOW_MODE_FIXED      0x00000000
#define X32_LOCAPIC_ICR_LOW_MODE_LOWPRI     0x00000100
#define X32_LOCAPIC_ICR_LOW_MODE_SMI        0x00000200
#define X32_LOCAPIC_ICR_LOW_MODE_NMI        0x00000400
#define X32_LOCAPIC_ICR_LOW_MODE_INIT       0x00000500
#define X32_LOCAPIC_ICR_LOW_MODE_STARTUP    0x00000600
#define X32_LOCAPIC_ICR_LOW_VECTOR_MASK     0x000000FF

#define X32_LOCAPIC_TIMER_DIV_MASK          0x0000000B
#define X32_LOCAPIC_TIMER_DIV_2             0x00000000
#define X32_LOCAPIC_TIMER_DIV_4             0x00000001
#define X32_LOCAPIC_TIMER_DIV_8             0x00000002
#define X32_LOCAPIC_TIMER_DIV_16            0x00000003
#define X32_LOCAPIC_TIMER_DIV_32            0x00000008
#define X32_LOCAPIC_TIMER_DIV_64            0x00000009
#define X32_LOCAPIC_TIMER_DIV_128           0x0000000A
#define X32_LOCAPIC_TIMER_DIV_1             0x0000000B

#define X32_LOCAPIC_LVT_MASK                0x00010000
#define X32_LOCAPIC_LVT_STATUS              0x00001000
#define X32_LOCAPIC_LVT_MODE_MASK           0x00000700
#define X32_LOCAPIC_LVT_MODE_FIXED          0x00000000
#define X32_LOCAPIC_LVT_MODE_SMI            0x00000200
#define X32_LOCAPIC_LVT_MODE_NMI            0x00000400
#define X32_LOCAPIC_LVT_MODE_EXTINT         0x00000700
#define X32_LOCAPIC_LVT_MODE_INIT           0x00000500
#define X32_LOCAPIC_LVT_VECTOR_MASK         0x000000FF

#define X32_LOCAPIC_LVT_TIMER_MODE_MASK     0x00060000
#define X32_LOCAPIC_LVT_TIMER_ONESHOT       0x00000000
#define X32_LOCAPIC_LVT_TIMER_PERIODIC      0x00020000
#define X32_LOCAPIC_LVT_TIMER_TSCDEAD       0x00040000

#define X32_LOCAPIC_LVT_LINT_TRIGGER_EDGE   0x00000000
#define X32_LOCAPIC_LVT_LINT_TRIGGER_LEVEL  0x00008000
#define X32_LOCAPIC_LVT_LINT_REMOTE         0x00004000
#define X32_LOCAPIC_LVT_LINT_POLARITY       0x00002000

//
//------------------------------------------------------------------------
//

#define A32_HIGH_VECTORS_ADDRESS                    0xFFFF0000

#define A32_VECTOR_OFFSET_RESET                     0x0  
#define A32_VECTOR_OFFSET_UNDEXCEPTION              0x4  
#define A32_VECTOR_OFFSET_SVCEXCEPTION              0x8  
#define A32_VECTOR_OFFSET_PREFETCHABORTEXCEPTION    0xc  
#define A32_VECTOR_OFFSET_DATAABORTEXCEPTION        0x10 
#define A32_VECTOR_OFFSET_IRQEXCEPTION              0x18 
#define A32_VECTOR_OFFSET_FIQEXCEPTION              0x1c 
#define A32_VECTORS_LENGTH_BYTES                    0x20

#define A32_PSR_FLAG_N                              0x80000000
#define A32_PSR_FLAG_Z                              0x40000000
#define A32_PSR_FLAG_C                              0x20000000
#define A32_PSR_FLAG_V                              0x10000000
#define A32_PSR_FLAG_Q                              0x08000000
#define A32_PSR_FLAG_J                              0x01000000
#define A32_PSR_GE_MASK                             0x000F0000
#define A32_PSR_T_BIT                               0x00000020
#define A32_PSR_F_BIT                               0x00000040
#define A32_PSR_I_BIT                               0x00000080
#define A32_PSR_A_BIT                               0x00000100

#define A32_PSR_MODE_USR                            0x10
#define A32_PSR_MODE_FIQ                            0x11
#define A32_PSR_MODE_IRQ                            0x12
#define A32_PSR_MODE_SVC                            0x13
#define A32_PSR_MODE_ABT                            0x17
#define A32_PSR_MODE_UNDEF                          0x1B
#define A32_PSR_MODE_SYS                            0x1F
#define A32_PSR_MODE_MASK                           0x1F

#define A32_FSR_STATUS_03_MASK                      0x0000000F
#define A32_DFSR_DOMAIN_MASK                        0x000000F0
#define A32_DFSR_DOMAIN_SHL                         4
#define A32_FSR_STATUS_4                            0x00000400 
#define A32_FSR_STATUS_4_SHL                        10
#define A32_DFSR_WnR                                0x00000800

#define A32_FAULT_TYPE_ALIGNMENT                                0x01
#define A32_FAULT_TYPE_DEBUG                                    0x02
#define A32_FAULT_TYPE_ACCESS_FLAG_FIRST_LEVEL                  0x03
#define A32_FAULT_TYPE_ACCESS_FLAG_SECOND_LEVEL                 0x06
#define A32_FAULT_TYPE_ICACHE_MAINT                             0x04
#define A32_FAULT_TYPE_TRANSLATION_FIRST_LEVEL                  0x05
#define A32_FAULT_TYPE_TRANSLATION_SECOND_LEVEL                 0x07
#define A32_FAULT_TYPE_SYNC_EXT_ABORT                           0x08
#define A32_FAULT_TYPE_DOMAIN_FAULT_FIRST_LEVEL                 0x09
#define A32_FAULT_TYPE_DOMAIN_FAULT_SECOND_LEVEL                0x0B
#define A32_FAULT_TYPE_SYNC_ABORT_ON_TRANSLATION_FIRST_LEVEL    0x0C
#define A32_FAULT_TYPE_SYNC_ABORT_ON_TRANSLATION_SECOND_LEVEL   0x0E
#define A32_FAULT_TYPE_PERMISSION_FIRST_LEVEL                   0x0D
#define A32_FAULT_TYPE_PERMISSION_SECOND_LEVEL                  0x0F
#define A32_FAULT_TYPE_TLB_CONFLICT                             0x10
#define A32_FAULT_TYPE_ASYNC_EXT_ABORT                          0x16
#define A32_FAULT_TYPE_ASYNC_PARITY_ERROR                       0x18
#define A32_FAULT_TYPE_SYNC_PARITY_ERROR                        0x19
#define A32_FAULT_TYPE_SYNC_PARITY_ON_TRANSLATION_FIRST_LEVEL   0x1C
#define A32_FAULT_TYPE_SYNC_PARITY_ON_TRANSLATION_SECOND_LEVEL  0x1E

#define A32_DOMAIN_NOACCESS                         0x0
#define A32_DOMAIN_CLIENT                           0x1
#define A32_DOMAIN_RESERVED                         0x2
#define A32_DOMAIN_MANAGER                          0x3

#define A32_TTBASE_PHYS_MASK                        0xFFFFC000
#define A32_TTBASE_PHYS_SIZE                        0x00004000

#define A32_TTBASE_PROC_PHYS_MASK                   0xFFFFE000
#define A32_TTBASE_PROC_PHYS_SIZE                   0x00002000

#define A32_MAX_CACHE_LEVELS                        8

#define A32_SGI_COUNT                               16

#define A32_SCTRL_M_MMUENABLE                       0x00000001  // 0
#define A32_SCTRL_A_ALIGNFAULTCHECKENABLE           0x00000002  // 1
#define A32_SCTRL_C_DCACHEENABLE                    0x00000004  // 2
#define A32_SCTRL_SB1_3                             0x00000008  // 3
#define A32_SCTRL_SB1_4                             0x00000010  // 4
#define A32_SCTRL_CP15BEN                           0x00000020  // 5
#define A32_SCTRL_SB1_6                             0x00000040  // 6
#define A32_SCTRL_B_BIGENDIAN                       0x00000080  // 7
#define A32_SCTRL_SBZ_8                             0x00000100  // 8
#define A32_SCTRL_SBZ_9                             0x00000200  // 9
#define A32_SCTRL_SW_SWPENABLE                      0x00000400  // 10
#define A32_SCTRL_Z_BRANCHPREDICTENABLE             0x00000800  // 11
#define A32_SCTRL_I_ICACHEENABLE                    0x00001000  // 12
#define A32_SCTRL_V_HIGHVECTORS                     0x00002000  // 13
#define A32_SCTRL_RR_CACHEPREDICTABLE               0x00004000  // 14
#define A32_SCTRL_SBZ_15                            0x00008000  // 15
#define A32_SCTRL_SB1_16                            0x00010000  // 16
#define A32_SCTRL_HA_HWACCESSFLAG                   0x00020000  // 17
#define A32_SCTRL_SB1_18                            0x00040000  // 18
#define A32_SCTRL_UWXN_UNPROV_XN_FORCE              0x00080000  // 19
#define A32_SCTRL_UWXN_XN_FORCE                     0x00100000  // 20
#define A32_SCTRL_FI_FASTINTENABLE                  0x00200000  // 21
#define A32_SCTRL_SB1_22                            0x00400000  // 22 
#define A32_SCTRL_SB1_23                            0x00800000  // 23
#define A32_SCTRL_VE_IMPVECTORSENABLE               0x01000000  // 24
#define A32_SCTRL_EE_EXCEPTIONENDIAN                0x02000000  // 25
#define A32_SCTRL_SBZ_26                            0x04000000  // 26
#define A32_SCTRL_NMFI_NONMASKFASTINT               0x08000000  // 27
#define A32_SCTRL_TRE_TEXREMAPENABLE                0x10000000  // 28
#define A32_SCTRL_AFE_ACCESSFLAGENABLE              0x20000000  // 29
#define A32_SCTRL_TE_THUMBEXCEPTIONS                0x40000000  // 30
#define A32_SCTRL_SBZ_31                            0x80000000  // 31

#define A32_SCR_nET_NOTEARLYTERM                    0x00000040
#define A32_SCR_AW_ABITWRITEABLE                    0x00000020
#define A32_SCR_FW_FBITWRITEABLE                    0x00000010
#define A32_SCR_EA_EXTERNALABORT                    0x00000008
#define A32_SCR_FIQ_HANDLER                         0x00000004
#define A32_SCR_IRQ_HANDLER                         0x00000002
#define A32_SCR_NS_NOTSECURE                        0x00000001

#define A32_IDPFR1_MODEL_MASK                       0x0000000F
#define A32_IDPFR1_SECEXT_MASK                      0x000000F0
#define A32_IDPFR1_VIRT_MASK                        0x0000F000
#define A32_IDPFR1_TIMER_MASK                       0x000F0000

#define A32_TTBE_PT_PRESENT                         0x00000001
#define A32_TTBE_PT_NS                              0x00000008  // only used in SEC mode
#define A32_TTBE_PT_DOMAIN_MASK                     0x000001E0
#define A32_TTBE_PT_DOMAIN_SHIFT                    5
#define A32_TTBE_PT_PHYSADDR_MASK                   0xFFFFFC00

#define A32_TTBE_PAGETABLE_PROTO                    A32_TTBE_PT_PRESENT

#define A32_PTE_EXEC_NEVER                          0x00000001
#define A32_PTE_PRESENT                             0x00000002
#define A32_PTE_B                                   0x00000004
#define A32_PTE_C                                   0x00000008
#define A32_PTE_AP_MASK                             0x00000230
#define A32_PTE_TEX_MASK                            0x000001C0
#define A32_PTE_SHARED                              0x00000400
#define A32_PTE_NOT_GLOBAL                          0x00000800
#define A32_PTE_PAGEPHYS_MASK                       0xFFFFF000

#define A32_MMU_PTE_AP_000                          0x00000000
#define A32_MMU_PTE_AP_001                          0x00000010
#define A32_MMU_PTE_AP_010                          0x00000020
#define A32_MMU_PTE_AP_011                          0x00000030
#define A32_MMU_PTE_AP_100                          0x00000200
#define A32_MMU_PTE_AP_101                          0x00000210
#define A32_MMU_PTE_AP_110                          0x00000220
#define A32_MMU_PTE_AP_111                          0x00000230

#define A32_MMU_PTE_TEX_000                         0x00000000
#define A32_MMU_PTE_TEX_001                         0x00000040
#define A32_MMU_PTE_TEX_010                         0x00000080
#define A32_MMU_PTE_TEX_011                         0x000000C0
#define A32_MMU_PTE_TEX_100                         0x00000100
#define A32_MMU_PTE_TEX_101                         0x00000140
#define A32_MMU_PTE_TEX_110                         0x00000180
#define A32_MMU_PTE_TEX_111                         0x000001C0

/* TEX remap disabled - section B3.7.2 in TRM */
#define A32_MMU_PTE_REGIONTYPE_STRONG               A32_MMU_PTE_TEX_000
#define A32_MMU_PTE_REGIONTYPE_SHAREABLE_DEVICE     (A32_MMU_PTE_TEX_000 | A32_PTE_B)
#define A32_MMU_PTE_REGIONTYPE_NONSHAREABLE_DEVICE  A32_MMU_PTE_TEX_010

#define A32_MMU_PTE_REGIONTYPE_UNCACHED             A32_MMU_PTE_TEX_001
#define A32_MMU_PTE_REGIONTYPE_CACHED_WRITETHRU     (A32_MMU_PTE_TEX_110 | A32_PTE_C)
#define A32_MMU_PTE_REGIONTYPE_CACHED_WRITEBACK     (A32_MMU_PTE_TEX_101 | A32_PTE_B)

/* SCTRL.AFE = 0 */
#define A32_MMU_PTE_PERMIT_MASK                     A32_MMU_PTE_AP_111
#define A32_MMU_PTE_PERMIT_NONE                     A32_MMU_PTE_AP_000
#define A32_MMU_PTE_PERMIT_KERN_RW_USER_NONE        A32_MMU_PTE_AP_001
#define A32_MMU_PTE_PERMIT_KERN_RW_USER_RO          A32_MMU_PTE_AP_010
#define A32_MMU_PTE_PERMIT_KERN_RW_USER_RW          A32_MMU_PTE_AP_011
#define A32_MMU_PTE_PERMIT_KERN_RO_USER_NONE        A32_MMU_PTE_AP_101
#define A32_MMU_PTE_PERMIT_KERN_RO_USER_RO          A32_MMU_PTE_AP_111

/* on MPCore systems, we have CBAR and PERIPHBASE inside it  */
#define A32_MPCORE_CBAR_LOW32_PERIPH_MASK               0xFFFF80000
#define A32_PERIPH_PAGES                                2
#define A32_PERIPH_OFFSET_SCU                           0x0000
#define A32_PERIPH_OFFSET_GICC                          0x0100
#define A32_PERIPH_OFFSET_GLOBAL_TIMER                  0x0200
#define A32_PERIPH_OFFSET_PRIVATE_TIMERS                0x0600
#define A32_PERIPH_OFFSET_GICD                          0x1000

#define A32_PERIF_SCU_OFFSET_CONTROL                    0x0000
#define A32_PERIF_SCU_OFFSET_CONFIG                     0x0004
#define A32_PERIF_SCU_OFFSET_POWERSTATUS                0x0008 
#define A32_PERIF_SCU_OFFSET_INV_SEC                    0x000C
#define A32_PERIF_SCU_OFFSET_FILTER_START               0x0040
#define A32_PERIF_SCU_OFFSET_FILTER_END                 0x0044
#define A32_PERIF_SCU_OFFSET_ACCESS                     0x0050
#define A32_PERIF_SCU_OFFSET_ACCESS_SEC                 0x0054

#define A32_PERIF_SCU_CONTROL_SCU_ENABLE                0x00000001
#define A32_PERIF_SCU_CONTROL_ADDRFILT_ENABLE           0x00000002
#define A32_PERIF_SCU_CONTROL_PARITYON                  0x00000004
#define A32_PERIF_SCU_CONTROL_SPECLINEFILL              0x00000008
#define A32_PERIF_SCU_CONTROL_FORCEPORT0_ENABLE         0x00000010
#define A32_PERIF_SCU_CONTROL_SCUSTANDBY_ENABLE         0x00000020
#define A32_PERIF_SCU_CONTROL_ICSTANDBY_ENABLE          0x00000040

#define A32_PERIF_SCU_CONFIG_NUMCPUSMINUS1_MASK         0x00000003
#define A32_PERIF_SCU_CONFIG_SMPCPUS_MASK               0x000000F0
#define A32_PERIF_SCU_CONFIG_SMPCPUS_SHIFT              4
#define A32_PERIF_SCU_CONFIG_TAGRAMSIZE_MASK            0x0000FF00
#define A32_PERIF_SCU_CONFIG_TAGRAMSIZE_SHIFT           8

#define A32_PERIF_SCU_POWERSTATUS_CPU0_MASK             0x00000003
#define A32_PERIF_SCU_POWERSTATUS_CPU0_SHIFT            0
#define A32_PERIF_SCU_POWERSTATUS_CPU1_MASK             0x00000300
#define A32_PERIF_SCU_POWERSTATUS_CPU1_SHIFT            8
#define A32_PERIF_SCU_POWERSTATUS_CPU2_MASK             0x00030000
#define A32_PERIF_SCU_POWERSTATUS_CPU2_SHIFT            16
#define A32_PERIF_SCU_POWERSTATUS_CPU3_MASK             0x03000000
#define A32_PERIF_SCU_POWERSTATUS_CPU3_SHIFT            24

#define A32_PERIF_SCU_INVALIDATESECURE_CPU0WAYS_MASK    0x0000000F
#define A32_PERIF_SCU_INVALIDATESECURE_CPU0WAYS_SHIFT   0
#define A32_PERIF_SCU_INVALIDATESECURE_CPU1WAYS_MASK    0x000000F0
#define A32_PERIF_SCU_INVALIDATESECURE_CPU1WAYS_SHIFT   4
#define A32_PERIF_SCU_INVALIDATESECURE_CPU2WAYS_MASK    0x00000F00
#define A32_PERIF_SCU_INVALIDATESECURE_CPU2WAYS_SHIFT   8
#define A32_PERIF_SCU_INVALIDATESECURE_CPU3WAYS_MASK    0x0000F000
#define A32_PERIF_SCU_INVALIDATESECURE_CPU3WAYS_SHIFT   12

#define A32_PERIF_SCU_FILTERADDR_MASK                   0xFFF00000

#define A32_PERIF_SCU_ACCESS_CPU0                       0x00000001
#define A32_PERIF_SCU_ACCESS_CPU1                       0x00000002
#define A32_PERIF_SCU_ACCESS_CPU2                       0x00000004
#define A32_PERIF_SCU_ACCESS_CPU3                       0x00000008

#define A32_PERIF_SCU_NONSECACCESS_CPU0_COMPONENTS      0x00000001
#define A32_PERIF_SCU_NONSECACCESS_CPU1_COMPONENTS      0x00000002
#define A32_PERIF_SCU_NONSECACCESS_CPU2_COMPONENTS      0x00000004
#define A32_PERIF_SCU_NONSECACCESS_CPU3_COMPONENTS      0x00000008

#define A32_PERIF_SCU_NONSECACCESS_CPU0_PRIVTIMER       0x00000010
#define A32_PERIF_SCU_NONSECACCESS_CPU1_PRIVTIMER       0x00000020
#define A32_PERIF_SCU_NONSECACCESS_CPU2_PRIVTIMER       0x00000040
#define A32_PERIF_SCU_NONSECACCESS_CPU3_PRIVTIMER       0x00000080

#define A32_PERIF_SCU_NONSECACCESS_CPU0_GTIMER          0x00000100
#define A32_PERIF_SCU_NONSECACCESS_CPU1_GTIMER          0x00000200
#define A32_PERIF_SCU_NONSECACCESS_CPU2_GTIMER          0x00000400
#define A32_PERIF_SCU_NONSECACCESS_CPU3_GTIMER          0x00000800

#define A32_PERIF_GICC_OFFSET_ICCICR                    0x0000
#define A32_PERIF_GICC_OFFSET_ICCPMR                    0x0004
#define A32_PERIF_GICC_OFFSET_ICCBPR                    0x0008
#define A32_PERIF_GICC_OFFSET_ICCIAR                    0x000C
#define A32_PERIF_GICC_OFFSET_ICCEOIR                   0x0010
#define A32_PERIF_GICC_OFFSET_ICCRPR                    0x0014
#define A32_PERIF_GICC_OFFSET_ICCHPIR                   0x0018
#define A32_PERIF_GICC_OFFSET_ICCABPR                   0x001C
#define A32_PERIF_GICC_OFFSET_ICPIIR                    0x00FC

#define A32_PERIF_GICC_IMPIDENT_PARTNUM_MASK            0xFFF00000
#define A32_PERIF_GICC_IMPIDENT_PARTNUM_SHIFT           20
#define A32_PERIF_GICC_IMPIDENT_ARCHNUM_MASK            0x000F0000
#define A32_PERIF_GICC_IMPIDENT_ARCHNUM_SHIFT           16
#define A32_PERIF_GICC_IMPIDENT_REVNUM_MASK             0x0000F000
#define A32_PERIF_GICC_IMPIDENT_REVNUM_SHIFT            12
#define A32_PERIF_GICC_IMPIDENT_IMPID_MASK              0x00000FFF
#define A32_PERIF_GICC_IMPIDENT_IMPID_SHIFT             0

#define A32_PERIF_GICC_ICDSGIR_FILTER_TARGETLIST        0x0
#define A32_PERIF_GICC_ICDSGIR_FILTER_EVERYONEELSE      0x1
#define A32_PERIF_GICC_ICDSGIR_FILTER_ITSELF            0x2

#define A32_PERIF_GICC_ICCICR_ENABLE_SECURE             0x01
#define A32_PERIF_GICC_ICCICR_ENABLE_NS                 0x02
#define A32_PERIF_GICC_ICCICR_ACK_CTL                   0x04
#define A32_PERIF_GICC_ICCICR_SIGNAL_SECURE_TO_FIQ      0x08
#define A32_PERIF_GICC_ICCICR_USE_SBPR                  0x10

#define A32_PERIF_GICC_ICCIAR_ACKINTID                  0x3FF

#define A32_PERIF_GTIMER_OFFSET_COUNTLOW                0x000
#define A32_PERIF_GTIMER_OFFSET_COUNTHIGH               0x004
#define A32_PERIF_GTIMER_OFFSET_CONTROL                 0x008
#define A32_PERIF_GTIMER_OFFSET_INTSTATUS               0x00C
#define A32_PERIF_GTIMER_OFFSET_COMPVALLOW              0x010
#define A32_PERIF_GTIMER_OFFSET_COMPVALHIGH             0x014
#define A32_PERIF_GTIMER_OFFSET_AUTOINC                 0x018

#define A32_PERIF_GTIMER_CONTROL_ENABLE                 0x00000001
#define A32_PERIF_GTIMER_CONTROL_COMP_ENABLE            0x00000002
#define A32_PERIF_GTIMER_CONTROL_IRQ_ENABLE             0x00000004
#define A32_PERIF_GTIMER_CONTROL_AUTOINC                0x00000008
#define A32_PERIF_GTIMER_CONTROL_PRESCALE_MASK          0x0000FF00
#define A32_PERIF_GTIMER_CONTROL_PRESCALE_SHIFT         8

#define A32_PERIF_GTIMER_INTSTATUS_EVENT                0x00000001

#define A32_PERIF_PTIMERS_OFFSET_LOAD                   0x000
#define A32_PERIF_PTIMERS_OFFSET_COUNTER                0x004
#define A32_PERIF_PTIMERS_OFFSET_CONTROL                0x008
#define A32_PERIF_PTIMERS_OFFSET_INTSTATUS              0x00C
#define A32_PERIF_PTIMERS_OFFSET_WATCHDOGLOAD           0x020
#define A32_PERIF_PTIMERS_OFFSET_WATCHDOGCOUNTER        0x024
#define A32_PERIF_PTIMERS_OFFSET_WATCHDOGCONTROL        0x028
#define A32_PERIF_PTIMERS_OFFSET_WATCHDOGINTSTATUS      0x02C
#define A32_PERIF_PTIMERS_OFFSET_WATCHDOGRESETSTATUS    0x030
#define A32_PERIF_PTIMERS_OFFSET_WATCHDOGDISABLE        0x034

#define A32_PERIF_PTIMERS_CONTROL_ENABLE                0x00000001
#define A32_PERIF_PTIMERS_CONTROL_AUTORELOAD            0x00000002
#define A32_PERIF_PTIMERS_CONTROL_IRQ_ENABLE            0x00000004
#define A32_PERIF_PTIMERS_CONTROL_PRESCALER_MASK        0x0000FF00
#define A32_PERIF_PTIMERS_CONTROL_PRESCALER_SHIFT       8

#define A32_PERIF_PTIMERS_INTSTATUS_EVENT               0x00000001

#define A32_PERIF_PTIMERS_WDOG_CONTROL_ENABLE           0x00000001
#define A32_PERIF_PTIMERS_WDOG_CONTROL_AUTORELOAD       0x00000002
#define A32_PERIF_PTIMERS_WDOG_CONTROL_IT_ENABLE        0x00000004
#define A32_PERIF_PTIMERS_WDOG_CONTROL_MODE             0x00000008
#define A32_PERIF_PTIMERS_WDOG_CONTROL_PRESCALER_MASK   0x0000FF00
#define A32_PERIF_PTIMERS_WDOG_CONTROL_PRESCALER_SHIFT  8

#define A32_PERIF_PTIMERS_WDOG_INTSTATUS_EVENT          0x00000001

#define A32_PERIF_PTIMERS_WDOG_RESETSTATUS_FLAG         0x00000001

#define A32_PERIF_PTIMERS_WDOG_DISABLE_VALUE0           0x12345678
#define A32_PERIF_PTIMERS_WDOG_DISABLE_VALUE1           0x87654321

#define A32_PERIF_GICD_OFFSET_ICDDCR                    0x000 // Distributor Control Register
#define A32_PERIF_GICD_OFFSET_ICDICTR                   0x004 // Interrupt Controller Type Register
#define A32_PERIF_GICD_OFFSET_ICDIIDR                   0x008 // Implementer Identification Register
#define A32_PERIF_GICD_OFFSET_ICDISR0                   0x080 // Interrupt Security Registers
#define A32_PERIF_GICD_OFFSET_ICDISER0                  0x100 // Interrupt Set-Enable Registers
#define A32_PERIF_GICD_OFFSET_ICDICER0                  0x180 // Interrupt Clear-Enable Registers
#define A32_PERIF_GICD_OFFSET_ICDSPR0                   0x200 // Interrupt Set-Pending Registers
#define A32_PERIF_GICD_OFFSET_ICDICPR0                  0x280 // Interrupt Clear-Pending Registers
#define A32_PERIF_GICD_OFFSET_ICDABR0                   0x300 // Active Bit Registers
#define A32_PERIF_GICD_OFFSET_ICDIPR0                   0x400 // Interrupt Priority Registers
#define A32_PERIF_GICD_OFFSET_ICDIPTR0                  0x800 // Interrupt Processor Target Registers
#define A32_PERIF_GICD_OFFSET_ICDICFR0                  0xC00 // Interrupt Configuration Registers
#define A32_PERIF_GICD_OFFSET_ICDPPISR                  0xD00 // PPI Status register
#define A32_PERIF_GICD_OFFSET_ICDSGIR                   0xF00 // Software Generated Interrupt Register

#define A32_PERIF_GICD_ICDDCR_SECURE_ENABLE_SEC         0x00000001
#define A32_PERIF_GICD_ICDDCR_SECURE_ENABLE_NONSEC      0x00000002
#define A32_PERIF_GICD_ICDDCR_NONSECURE_ENABLE_NONSEC   0x00000001

#define A32_PERIF_GICD_ICDICTR_ITLINES_MASK             0x0000001F
#define A32_PERIF_GICD_ICDICTR_ITLINES_SHIFT            0
#define A32_PERIF_GICD_ICDICTR_CPUNUMBER_MASK           0x000000E0
#define A32_PERIF_GICD_ICDICTR_CPUNUMBER_SHIFT          5
#define A32_PERIF_GICD_ICDICTR_LSPIS_MASK               0x0000F800
#define A32_PERIF_GICD_ICDICTR_LSPIS_SHIFT              11

#define A32_PERIF_GICD_ICDDIR_IDIMP_MASK                0x00000FFF
#define A32_PERIF_GICD_ICDDIR_IDIMP_SHIFT               0
#define A32_PERIF_GICD_ICDDIR_REVNUM_MASK               0x00FFF000
#define A32_PERIF_GICD_ICDDIR_REVNUM_SHIFT              12
#define A32_PERIF_GICD_ICDDIR_IMPDEF_MASK               0xFF000000
#define A32_PERIF_GICD_ICDDIR_IMPDEF_SHIFT              24

#define A32_PERIF_GICD_PPISTATUS_nIRQ                   0x00008000
#define A32_PERIF_GICD_PPISTATUS_WDOG                   0x00004000
#define A32_PERIF_GICD_PPISTATUS_PRIVTIMER              0x00002000
#define A32_PERIF_GICD_PPISTATUS_nFIQ                   0x00001000
#define A32_PERIF_GICD_PPISTATUS_GTIMER                 0x00008000

#define A32_MP_GTIMER_IRQ                               27
#define A32_MP_LEGACY_FIQ_IRQ                           28
#define A32_MP_PTIMERS_IRQ                              29
#define A32_MP_WDOG_IRQ                                 30
#define A32_MP_LEGACY_IRQ                               31
#define A32_MP_PPI_START_IRQ                            32

//
//------------------------------------------------------------------------
//

#define K2_VA_MEMPAGE_BYTES_POW2                    12
#define K2_VA_MEMPAGE_BYTES                         (1 << K2_VA_MEMPAGE_BYTES_POW2)
#define K2_VA_MEMPAGE_OFFSET_MASK                   (K2_VA_MEMPAGE_BYTES - 1)
#define K2_VA_PAGEFRAME_MASK                        (~K2_VA_MEMPAGE_OFFSET_MASK)

/* --------------------------------------------------------------------------------- */

#define K2_VA32_PAGEDIR_PAGETABLES_POW2             10
#define K2_VA32_PAGETABLE_PAGES_POW2                10

#define K2_VA32_PAGEFRAMES_POW2                     (K2_VA32_PAGEDIR_PAGETABLES_POW2 + K2_VA32_PAGETABLE_PAGES_POW2)
#define K2_VA32_PAGETABLE_MAP_BYTES_POW2            (K2_VA32_PAGETABLE_PAGES_POW2 + K2_VA_MEMPAGE_BYTES_POW2)

#define K2_VA32_PAGEFRAMES_FOR_4G                   (1 << K2_VA32_PAGEFRAMES_POW2)
#define K2_VA32_PAGEFRAMES_FOR_2G                   (1 << (K2_VA32_PAGEFRAMES_POW2-1))
#define K2_VA32_PAGETABLE_MAP_BYTES                 (1 << K2_VA32_PAGETABLE_MAP_BYTES_POW2)
#define K2_VA32_ENTRIES_PER_PAGETABLE               (1 << K2_VA32_PAGETABLE_PAGES_POW2)
#define K2_VA32_PAGETABLES_FOR_4G                   (1 << K2_VA32_PAGEDIR_PAGETABLES_POW2)
#define K2_VA32_PAGETABLES_FOR_2G                   (1 << (K2_VA32_PAGEDIR_PAGETABLES_POW2-1))

#define K2_VA32_TO_PTE_ADDR(vamap_base, x)          ((vamap_base) + (((x) >> (K2_VA_MEMPAGE_BYTES_POW2 - 2)) & ~3))
#define K2_VA32_TO_PT_ADDR(vamap_base, x)           ((vamap_base) + (((x) >> (K2_VA_MEMPAGE_BYTES_POW2 - 2)) & K2_VA_PAGEFRAME_MASK))

#if K2_TARGET_ARCH_IS_INTEL
#define K2_VA32_TRANSTAB_SIZE       X32_PAGEDIR_PHYS_SIZE
#define K2_VA32_PROC_TRANSTAB_SIZE  X32_PAGEDIR_PHYS_SIZE
#else
#define K2_VA32_TRANSTAB_SIZE       A32_TTBASE_PHYS_SIZE
#define K2_VA32_PROC_TRANSTAB_SIZE  A32_TTBASE_PROC_PHYS_SIZE
#endif

/* --------------------------------------------------------------------------------- */

#define K2_IANA_HTYPE_ETHERNET      1
#define K2_IANA_HTYPE_IEEE_802      6
#define K2_IANA_HTYPE_SERIAL        20

// [RFC6328]
#define K2_IANA_NLPID_IPV4                  0xCC    // [RFC791]
#define K2_IANA_NLPID_PPP                   0xCF    // [RFC1661]

#define K2_PPP_AUTO_TIMER_DEFAULT_MS        3000
#define K2_PPP_AUTO_MAX_TERMINATE_COUNT     2
#define K2_PPP_AUTO_MAX_CONFIGURE_COUNT     10
#define K2_PPP_AUTO_MAX_FAILURE_COUNT       5

#define K2_PPP_AUTO_CONFIGURE_REQUEST       1
#define K2_PPP_AUTO_CONFIGURE_ACK           2
#define K2_PPP_AUTO_CONFIGURE_NACK          3
#define K2_PPP_AUTO_CONFIGURE_REJECT        4
#define K2_PPP_AUTO_TERMINATE_REQUEST       5
#define K2_PPP_AUTO_TERMINATE_ACK           6
#define K2_PPP_AUTO_CODE_REJECT             7
#define K2_PPP_AUTO_PROTOCOL_REJECT         8
#define K2_PPP_AUTO_ECHO_REQUEST            9
#define K2_PPP_AUTO_ECHO_REPLY              10
#define K2_PPP_AUTO_DISCARD_REQUEST         11

#define K2_PPP_PROTO_LCP                    0xC021
#define K2_PPP_PROTO_PAP                    0xC023
#define K2_PPP_PROTO_LQR                    0xC025
#define K2_PPP_PROTO_CHAP                   0xC223

#define K2_PPP_PROTO_IPCP                   0x8021

#define K2_PPP_PROTO_IPV4                   0x0021
#define K2_PPP_PROTO_PADDING                0x0001
#define K2_PPP_PROTO_MULTI                  0x003D

#define K2_PPP_NLP_RANGE_FIRST              0x0000
#define K2_PPP_NLP_RANGE_LAST               0x3FFF
#define K2_PPP_LV_RANGE_FIRST               0x4000
#define K2_PPP_LV_RANGE_LAST                0x7FFF
#define K2_PPP_NCP_RANGE_FIRST              0x8000
#define K2_PPP_NCP_RANGE_LAST               0xBFFF
#define K2_PPP_LCP_RANGE_FIRST              0xC000
#define K2_PPP_LCP_RANGE_LAST               0xFEFF

#define K2_PPP_LCPOPT_MRU                   1
#define K2_PPP_LCPOPT_ACCM                  2
#define K2_PPP_LCPOPT_AUTH                  3
#define K2_PPP_LCPOPT_QUAL                  4
#define K2_PPP_LCPOPT_MAGIC                 5
#define K2_PPP_LCPOPT_PFC                   7
#define K2_PPP_LCPOPT_ACFC                  8
#define K2_PPP_MRU_MINIMUM                  1500
#define K2_PPP_LCPOPT_MRU_DEFAULT_VAL       K2_PPP_MRU_MINIMUM 
#define K2_PPP_LCPOPT_ACCM_DEFAULT_VAL      0xFFFFFFFF
    
#define K2_PPP_IPCP_CONFIGURE_REQUEST       1
#define K2_PPP_IPCP_CONFIGURE_ACK           2
#define K2_PPP_IPCP_CONFIGURE_NACK          3
#define K2_PPP_IPCP_CONFIGURE_REJECT        4
#define K2_PPP_IPCP_TERMINATE_REQUEST       5
#define K2_PPP_IPCP_TERMINATE_ACK           6
#define K2_PPP_IPCP_CODE_REJECT             7

#define K2_PPP_ICPCOPT_IP_ADDRESSES         1   // deprecated
#define K2_PPP_ICPCOPT_IP_COMPRESS_PROTO    2   // RFC 1332
#define K2_PPP_ICPCOPT_IP_ADDRESS           3
#define K2_PPP_ICPCOPT_PRIMARY_DNS          129
#define K2_PPP_ICPCOPT_SECONDARY_DNS        131 // RFC 1877

/* --------------------------------------------------------------------------------- */

#endif // __K2BASEDEFS_INC
