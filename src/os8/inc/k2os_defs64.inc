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

#ifndef __K2OS_DEFS64_INC
#define __K2OS_DEFS64_INC

#ifndef __K2OS_DEFS_INC 
#error !!! include k2os_defs.inc instead
#endif

/* --------------------------------------------------------------------------------- */

//
// must exactly match K2OS_UEFI_LOADINFO64 in k2osbase.h
//
#define K2OS_UEFI_LOADINFO_OFFSET_MARKER                        0x0000      // 'K2OS'
#define K2OS_UEFI_LOADINFO_OFFSET_CPU_CORE_COUNT                0x0004
#define K2OS_UEFI_LOADINFO_OFFSET_EFIST                         0x0008
#define K2OS_UEFI_LOADINFO_OFFSET_KERNXDLENTRY                  0x0010 
#define K2OS_UEFI_LOADINFO_OFFSET_SYSVIRTENTRY                  0x0018      // encoded in code bytes in uefi loader. careful
#define K2OS_UEFI_LOADINFO_OFFSET_KERN_ARENA_LOW                0x0020
#define K2OS_UEFI_LOADINFO_OFFSET_KERN_ARENA_HIGH               0x0028
#define K2OS_UEFI_LOADINFO_OFFSET_BUILTIN_ROFS_PHYS             0x0030
#define K2OS_UEFI_LOADINFO_OFFSET_TRANSBASE_PHYS                0x0038      // encoded in code bytes in uefi loader. careful
#define K2OS_UEFI_LOADINFO_OFFSET_ZEROPAGE_PHYS                 0x0040
#define K2OS_UEFI_LOADINFO_OFFSET_TRANS_PAGE_ADDR               0x0048
#define K2OS_UEFI_LOADINFO_OFFSET_DEBUG_PAGE_VIRT               0x0050
#define K2OS_UEFI_LOADINFO_OFFSET_EFIMAP_SIZE                   0x0058
#define K2OS_UEFI_LOADINFO_OFFSET_EFIMAP_DESC_SIZE              0x0060 
#define K2OS_UEFI_LOADINFO_OFFSET_EFIMAP_DESC_VER               0x0068
#define K2OS_UEFI_LOADINFO_OFFSET_FWTAB_PAGES_PHYS              0x0070
#define K2OS_UEFI_LOADINFO_OFFSET_FWTAB_PAGES_VIRT              0x0078
#define K2OS_UEFI_LOADINFO_OFFSET_FWTAB_PAGE_COUNT              0x0080
#define K2OS_UEFI_LOADINFO_OFFSET_FWFACS_PHYS                   0x0088
#define K2OS_UEFI_LOADINFO_OFFSET_XFWFACS_PHYS                  0x0090
#define K2OS_UEFI_LOADINFO_OFFSET_XDLCRT                        0x0098
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_FRAMEBUFFERPHYS      0x00A0
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_FRAMEBUFFERBYTES     0x00A8
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_MODE_VERSION         0x00AC
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_HORIZONTALRESOLUTION 0x00B0
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_VERTICALRESOLUTION   0x00B4
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_PIXELFORMATID        0x00B8
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_PIXELREDMASK         0x00BC
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_PIXELGREENMASK       0x00C0
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_PIXELBLUEMASK        0x00C4
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_PIXELRESERVEDMASK    0x00C8
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_PIXELSPERSCANLINE    0x00CC
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_BGRT_BMP_WIDTH       0x00D0 
#define K2OS_UEFI_LOADINFO_OFFSET_BOOTGRAF_BGRT_BMP_HEIGHT      0x00D4
#define K2OS_UEFI_LOADINFO_SIZE_BYTES                           0x00D8

//
// size of physical per-page tracking structure
//
#define K2OS_PHYSTRACK_BYTES                        (8 * 4)

//
// max memory pages to use for EFI memory map coming from UEFI 
//
#define K2OS_EFIMAP_PAGECOUNT                       2

//
// max # of threads in the system - threadptrs pages hold 8kB/8 = 1048 = 0x400 of these
//
#define K2OS_MAX_THREAD_COUNT                       0x3FE   // 0 and 0x3FF reserved


/* --------------------------------------------------------------------------------- */

#endif // __K2OS_DEFS64_INC
