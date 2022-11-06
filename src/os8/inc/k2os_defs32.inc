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

#ifndef __K2OS_DEFS32_INC
#define __K2OS_DEFS32_INC

#ifndef __K2OS_DEFS_INC 
#error !!! include k2os_defs.inc instead
#endif

/* --------------------------------------------------------------------------------- */

//
// must exactly match K2OS_UEFI_LOADINFO32 in k2osbase.h
//
#define K2OS_UEFI_LOADINFO_OFFSET_MARKER                        0x0000      // 'K2OS'
#define K2OS_UEFI_LOADINFO_OFFSET_CPU_CORE_COUNT                0x0004
#define K2OS_UEFI_LOADINFO_OFFSET_EFIST                         0x0008
#define K2OS_UEFI_LOADINFO_OFFSET_KERNXDLENTRY                  0x000C 
#define K2OS_UEFI_LOADINFO_OFFSET_SYSVIRTENTRY                  0x0010      // encoded in code bytes in uefi loader. careful
#define K2OS_UEFI_LOADINFO_OFFSET_KERN_ARENA_LOW                0x0014
#define K2OS_UEFI_LOADINFO_OFFSET_KERN_ARENA_HIGH               0x0018
#define K2OS_UEFI_LOADINFO_OFFSET_BUILTIN_ROFS_PHYS             0x001C
#define K2OS_UEFI_LOADINFO_OFFSET_TRANSBASE_PHYS                0x0020      // encoded in code bytes in uefi loader. careful
#define K2OS_UEFI_LOADINFO_OFFSET_ZEROPAGE_PHYS                 0x0024
#define K2OS_UEFI_LOADINFO_OFFSET_TRANS_PAGE_ADDR               0x0028
#define K2OS_UEFI_LOADINFO_OFFSET_DEBUG_PAGE_VIRT               0x002C
#define K2OS_UEFI_LOADINFO_OFFSET_EFIMAP_SIZE                   0x0030
#define K2OS_UEFI_LOADINFO_OFFSET_EFIMAP_DESC_SIZE              0x0034 
#define K2OS_UEFI_LOADINFO_OFFSET_EFIMAP_DESC_VER               0x0038
#define K2OS_UEFI_LOADINFO_OFFSET_FWTAB_PAGES_PHYS              0x003C
#define K2OS_UEFI_LOADINFO_OFFSET_FWTAB_PAGES_VIRT              0x0040
#define K2OS_UEFI_LOADINFO_OFFSET_FWTAB_PAGE_COUNT              0x0044
#define K2OS_UEFI_LOADINFO_OFFSET_FWFACS_PHYS                   0x0048
#define K2OS_UEFI_LOADINFO_OFFSET_XFWFACS_PHYS                  0x004C
#define K2OS_UEFI_LOADINFO_OFFSET_XDLCRT                        0x0050
#define K2OS_UEFI_LOADINFO_OFFSET_RESERVED                      0x0054
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_FRAMEBUFFERPHYS      0x0058
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_FRAMEBUFFERBYTES     0x0060
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_MODE_VERSION         0x0064
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_HORIZONTALRESOLUTION 0x0068
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_VERTICALRESOLUTION   0x006C
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_PIXELFORMATID        0x0070
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_PIXELREDMASK         0x0074
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_PIXELGREENMASK       0x0078
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_PIXELBLUEMASK        0x007C
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_PIXELRESERVEDMASK    0x0080
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_PIXELSPERSCANLINE    0x0084
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_BGRT_BMP_WIDTH       0x0088 
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_BGRT_BMP_HEIGHT      0x008C
#define K2OS_UEFI_LOADINFO_SIZE_BYTES                           0x0090

//
// size of physical per-page tracking structure
//
#define K2OS_PHYSTRACK_BYTES                        (4 * 4)

//
// max memory pages to use for EFI memory map coming from UEFI 
//
#define K2OS_EFIMAP_PAGECOUNT                       2

//
// max # of threads in the system - threadptrs page holds 4kB/4 = 1024 = 0x400 of these
//
#define K2OS_MAX_THREAD_COUNT                       0x3FE   // 0 and 0x3FF reserved

/* --------------------------------------------------------------------------------- */

//
// Kernel Virtual Address Range (KVA) top down
//
#define K2OS_PUBLICAPI_SIZE                         0x00001000
#define K2OS_KVA_PUBLICAPI_BASE                     0xFFFFF000

#define K2OS_KVA_FACS_SIZE                          0x00001000
#define K2OS_KVA_FACS_BASE                          0xFFFFE000

#define K2OS_KVA_XFACS_SIZE                         0x00001000
#define K2OS_KVA_XFACS_BASE                         0xFFFFD000

#define K2OS_KVA_ARCHSPEC_SIZE                      0x0000D000
#define K2OS_KVA_ARCHSPEC_BASE                      0xFFFF0000  // must be here for A32

#define K2OS_KVA_COREMEMORY_SIZE                    0x00020000  // 0x4000 * 8 = 0x20000
#define K2OS_KVA_COREMEMORY_BASE                    0xFFFD0000
#define K2OS_KVA_COREMEMORY_BASE_HIGH16             0xFFFD
#define K2OS_KVA_COREMEMORY_BASE_LOW16              0x0000

#define K2OS_KVA_PERCOREWORKPAGES_SIZE              0x00010000  // two virtual pages per core
#define K2OS_PERCOREWORKPAGES_PERCORE               2
#define K2OS_KVA_PERCOREWORKPAGES_BASE              0xFFFC0000

#define K2OS_KVA_BUILTIN_FS_BASE                    0xFFC00000
#define K2OS_KVA_BUILTIN_MAX_SIZE                   0x003C0000 // 3840 KB
#define K2OS_KVA_BUILTIN_AREA_END                   (K2OS_KVA_BUILTIN_FS_BASE + K2OS_KVA_BUILTIN_MAX_SIZE)

// this must be virtually 4MB aligned. pagetable not installed by boot loader
#define K2OS_KVA_TLSAREA_BASE                       0xFF800000
#define K2OS_KVA_TLSAREA_SIZE                       0x00400000  

#define K2OS_KVA_KERNEL_ARENA_INIT_TOP              K2OS_KVA_TLSAREA_BASE

/* --------------------------------------------------------------------------------- */

#define K2OS_X32_ARCH_PAGES_BASE                    0
#define K2OS_X32_APSTART_TRANSIT_PAGE_ADDR          0x00001000
#define K2OS_X32_APSTART_PAGEDIR_ADDR               0x00002000
#define K2OS_X32_APSTART_PAGETABLE_ADDR             0x00003000
#define K2OS_X32_ARCH_PAGES_COUNT                   4
#define K2OS_X32_MAX_IOAPICS_COUNT                  2

// 0xFFFF0000
#define K2OS_KVA_X32_LOCAPIC                        K2OS_KVA_ARCHSPEC_BASE
#define K2OS_KVA_X32_LOCAPIC_SIZE                   K2_VA_MEMPAGE_BYTES

// 0xFFFF1000
#define K2OS_KVA_X32_IOAPICS                        (K2OS_KVA_X32_LOCAPIC + K2OS_KVA_X32_LOCAPIC_SIZE)
#define K2OS_KVA_X32_IOAPICS_SIZE                   (K2_VA_MEMPAGE_BYTES * K2OS_X32_MAX_IOAPICS_COUNT)

// 0xFFFF3000
#define K2OS_KVA_X32_HPET                           (K2OS_KVA_X32_IOAPICS + K2OS_KVA_X32_IOAPICS_SIZE)
#define K2OS_KVA_X32_HPET_SIZE                      K2_VA_MEMPAGE_BYTES

// 0xFFFF4000
#define K2OS_KVA_X32_AP_TRANSITION                  (K2OS_KVA_X32_HPET + K2OS_KVA_X32_HPET_SIZE)
#define K2OS_KVA_X32_AP_TRANSITION_SIZE             K2_VA_MEMPAGE_BYTES

// 0xFFFF5000
#define K2OS_KVA_X32_AP_PAGEDIR                     (K2OS_KVA_X32_AP_TRANSITION + K2OS_KVA_X32_AP_TRANSITION_SIZE)
#define K2OS_KVA_X32_AP_PAGEDIR_SIZE                K2_VA_MEMPAGE_BYTES

// 0xFFFF6000
#define K2OS_KVA_X32_AP_PAGETABLE                   (K2OS_KVA_X32_AP_PAGEDIR + K2OS_KVA_X32_AP_PAGEDIR_SIZE)
#define K2OS_KVA_X32_AP_PAGETABLE_SIZE              K2_VA_MEMPAGE_BYTES

// 0xFFFF7000
#define K2OS_KVA_X32_ARCHSPEC_END                   (K2OS_KVA_X32_AP_PAGETABLE + K2OS_KVA_X32_AP_PAGETABLE_SIZE)

/* --------------------------------------------------------------------------------- */

#define K2OS_A32_APSTART_SPACE_PHYS_PAGECOUNT       10  

// 0xFFFF0000
#define K2OS_KVA_A32_VECTORS                        K2OS_KVA_ARCHSPEC_BASE  // MUST BE HERE
#define K2OS_KVA_A32_VECTORS_SIZE                   K2_VA_MEMPAGE_BYTES

// 0xFFFF1000
#define K2OS_KVA_A32_PERIPHBASE                     (K2OS_KVA_A32_VECTORS + K2OS_KVA_A32_VECTORS_SIZE)
#define K2OS_KVA_A32_PERIPHBASE_SIZE                K2_VA_MEMPAGE_BYTES

// 0xFFFF2000
#define K2OS_KVA_A32_GICD                           (K2OS_KVA_A32_PERIPHBASE + K2OS_KVA_A32_PERIPHBASE_SIZE)
#define K2OS_KVA_A32_GICD_SIZE                      K2_VA_MEMPAGE_BYTES

// 0xFFFF3000
#define K2OS_KVA_A32_GICC                           (K2OS_KVA_A32_GICD + K2OS_KVA_A32_GICD_SIZE)
#define K2OS_KVA_A32_GICC_SIZE                      K2_VA_MEMPAGE_BYTES

// 0xFFFF4000
#define K2OS_KVA_A32_AP_TTB                         (K2OS_KVA_A32_GICC + K2OS_KVA_A32_GICC_SIZE)  // must be 16K aligned
#define K2OS_KVA_A32_AP_TTB_SIZE                    (K2_VA_MEMPAGE_BYTES * 4)

// 0xFFFF8000
#define K2OS_KVA_A32_AP_PAGETABLE                   (K2OS_KVA_A32_AP_TTB + K2OS_KVA_A32_AP_TTB_SIZE)
#define K2OS_KVA_A32_AP_PAGETABLE_SIZE              K2_VA_MEMPAGE_BYTES

// 0xFFFF9000
#define K2OS_KVA_A32_AP_TRANSIT                     (K2OS_KVA_A32_AP_PAGETABLE + K2OS_KVA_A32_AP_PAGETABLE_SIZE)
#define K2OS_KVA_A32_AP_TRANSIT_SIZE                K2_VA_MEMPAGE_BYTES

// 0xFFFFA000 
#define K2OS_KVA_A32_ARCHSPEC_END                   (K2OS_KVA_A32_AP_TRANSIT + K2OS_KVA_A32_AP_TRANSIT_SIZE)

/* --------------------------------------------------------------------------------- */

//
// Kernel Virtual Address Range (KVA) bottom up
//
#define K2OS_KVA_KERN_BASE                          0x80000000             

// 0x80000000 - 0x803FFFFF -- this must be virtually 4MB aligned 
#define K2OS_KVA_KERNVAMAP_BASE                     K2OS_KVA_KERN_BASE
#define K2OS_KVA_KERNVAMAP_SIZE                     (K2_VA32_PAGEFRAMES_FOR_4G * 4)     

// 0x80400000 - 0x813FFFFF
#define K2OS_KVA_PHYSTRACKAREA_BASE                 (K2OS_KVA_KERNVAMAP_BASE + K2OS_KVA_KERNVAMAP_SIZE)
#define K2OS_KVA_PHYSTRACKAREA_SIZE                 (K2_VA32_PAGEFRAMES_FOR_4G * K2OS_PHYSTRACK_BYTES)
#define K2OS_KVA_PHYSTRACKAREA_END                  (K2OS_KVA_PHYSTRACKAREA_BASE + K2OS_KVA_PHYSTRACKAREA_SIZE)

//
// translation table base must be aligned on 16KB boundary on A32
//
// 0x81400000
#define K2OS_KVA_TRANSTAB_BASE                      K2OS_KVA_PHYSTRACKAREA_END

// 0x81401000 x32 / 0x81404000 a32
#define K2OS_KVA_EFIMAP_BASE                        (K2OS_KVA_TRANSTAB_BASE + K2_VA32_TRANSTAB_SIZE)
#define K2OS_KVA_EFIMAP_SIZE                        (K2OS_EFIMAP_PAGECOUNT * K2_VA_MEMPAGE_BYTES)

// 0x81403000 x32 / 0x81406000 a32
#define K2OS_KVA_THREADPTRS_BASE                    (K2OS_KVA_EFIMAP_BASE + K2OS_KVA_EFIMAP_SIZE)

// 0x81404000 x32 / 0x81407000 a32
#define K2OS_KVA_LOADERPAGE_BASE                    (K2OS_KVA_THREADPTRS_BASE + K2_VA_MEMPAGE_BYTES)

// 0x81405000 x32 / 0x81408000 a32
#define K2OS_KVA_KERNEL_ARENA_INIT_BOTTOM           (K2OS_KVA_LOADERPAGE_BASE + K2_VA_MEMPAGE_BYTES)


/* --------------------------------------------------------------------------------- */

//
// derived definitions
//

// 0xFFFD3FFC 
#define K2OS_KVA_CORE0_STACK_INIT                   (K2OS_KVA_COREMEMORY_BASE + (K2_VA_MEMPAGE_BYTES * 4) - 4)

#define K2OS_KVA_TO_PTE_ADDR(x)                     K2_VA32_TO_PTE_ADDR(K2OS_KVA_KERNVAMAP_BASE, (x))
#define K2OS_KVA_TO_PT_ADDR(x)                      K2_VA32_TO_PT_ADDR(K2OS_KVA_KERNVAMAP_BASE, (x))

#define K2OS_PHYS32_TO_PHYSTRACK(x)                 (K2OS_KVA_PHYSTRACKAREA_BASE + (((x) >> K2_VA_MEMPAGE_BYTES_POW2) * K2OS_PHYSTRACK_BYTES))
#define K2OS_PHYSTRACK_TO_PHYS32(x)                 ((((x) - K2OS_KVA_PHYSTRACKAREA_BASE) / K2OS_PHYSTRACK_BYTES) << K2_VA_MEMPAGE_BYTES_POW2)

/* --------------------------------------------------------------------------------- */

//
// User Virtual Address Range (UVA)
//
#define K2OS_UVA_PUBLICAPI_BASE                     0x7FFFF000 
#define K2OS_UVA_TIMER_IOPAGE_BASE                  0x7FFFE000
#define K2OS_UVA_PUBLICAPI_PAGETABLE_BASE           0x7FC00000
#define K2OS_UVA_TLSAREA_BASE                       0x00000000 
#define K2OS_UVA_LOW_BASE                           0x00400000 
#define K2OS_UVA_XDL_GLOBALS                        K2OS_UVA_LOW_BASE 
#define K2OS_UVA_XDL_CRTPAGE                        (K2OS_UVA_XDL_GLOBALS + K2_VA_MEMPAGE_BYTES)
#define K2OS_UVA_CRTXDL_LOADPOINT                   (K2OS_UVA_XDL_CRTPAGE + K2_VA_MEMPAGE_BYTES)

/* --------------------------------------------------------------------------------- */

#define K2OS_PUBLICAPI_OFFSET_TIMER_FREQ            0xFFC
#define K2OS_PUBLICAPI_OFFSET_TIMER_ADDR            0xFF8 
#define K2OS_PUBLICAPI_OFFSET_DATETIME              0xFE0
#define K2OS_PUBLICAPI_OFFSET_CORE7_DATA            0xFCC 
#define K2OS_PUBLICAPI_OFFSET_CORE6_DATA            0xFC8 
#define K2OS_PUBLICAPI_OFFSET_CORE5_DATA            0xFC4 
#define K2OS_PUBLICAPI_OFFSET_CORE4_DATA            0xFC0 
#define K2OS_PUBLICAPI_OFFSET_CORE3_DATA            0xFBC 
#define K2OS_PUBLICAPI_OFFSET_CORE2_DATA            0xFB8 
#define K2OS_PUBLICAPI_OFFSET_CORE1_DATA            0xFB4 
#define K2OS_PUBLICAPI_OFFSET_CORE0_DATA            0xFB0 
#define K2OS_PUBLIACPI_OFFSET_SYSCALL               0

#define K2OS_KVA_PUBLICAPI_TIMER_FREQ               (K2OS_KVA_PUBLICAPI_BASE + K2OS_PUBLICAPI_OFFSET_TIMER_FREQ)
#define K2OS_KVA_PUBLICAPI_TIMER_ADDR               (K2OS_KVA_PUBLICAPI_BASE + K2OS_PUBLICAPI_OFFSET_TIMER_ADDR)
#define K2OS_KVA_PUBLICAPI_DATETIME                 (K2OS_KVA_PUBLICAPI_BASE + K2OS_PUBLICAPI_OFFSET_DATETIME)
#define K2OS_KVA_PUBLICAPI_PERCORE_DATA             (K2OS_KVA_PUBLICAPI_BASE + K2OS_PUBLICAPI_OFFSET_CORE0_DATA)
#define K2OS_KVA_PUBLICAPI_SYSCALL                  (K2OS_KVA_PUBLICAPI_BASE + K2OS_PUBLIACPI_OFFSET_SYSCALL)

#define K2OS_UVA_PUBLICAPI_TIMER_FREQ               (K2OS_UVA_PUBLICAPI_BASE + K2OS_PUBLICAPI_OFFSET_TIMER_FREQ)
#define K2OS_UVA_PUBLICAPI_TIMER_ADDR               (K2OS_UVA_PUBLICAPI_BASE + K2OS_PUBLICAPI_OFFSET_TIMER_ADDR)
#define K2OS_UVA_PUBLICAPI_DATETIME                 (K2OS_UVA_PUBLICAPI_BASE + K2OS_PUBLICAPI_OFFSET_DATETIME)
#define K2OS_UVA_PUBLICAPI_PERCORE_DATA             (K2OS_UVA_PUBLICAPI_BASE + K2OS_PUBLICAPI_OFFSET_CORE0_DATA)
#define K2OS_UVA_PUBLICAPI_SYSCALL                  (K2OS_UVA_PUBLICAPI_BASE + K2OS_PUBLIACPI_OFFSET_SYSCALL)

/* --------------------------------------------------------------------------------- */

#endif // __K2OS_DEFS32_INC
