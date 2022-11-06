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

#ifndef __K2OS_DEFS_INC
#define __K2OS_DEFS_INC

#include <k2basedefs.inc>

/* --------------------------------------------------------------------------------- */

#define K2OS_MAX_CPU_COUNT      8
#define K2OS_CACHELINE_BYTES    64

#if K2_TARGET_ARCH_IS_32BIT
#include "k2os_defs32.inc"
#else
#include "k2os_defs64.inc"
#endif

/* --------------------------------------------------------------------------------- */

//
// UEFI 2.5 Spec Page 152, EFI_BOOT_SERVICES.AllocatePages
//
// "MemoryType values in the range 0x80000000..0xFFFFFFFF are reserved for use by UEFI OS loaders
// that are provided by operating system vendors."
//
#define K2OS_EFIMEMTYPE_PAGING          0x80000000  //  page directory and page tables
#define K2OS_EFIMEMTYPE_XDL_TEXT        0x80000001  //  xdl code
#define K2OS_EFIMEMTYPE_XDL_READ        0x80000002  //  xdl read-only
#define K2OS_EFIMEMTYPE_XDL_DATA        0x80000003  //  xdl read/write
#define K2OS_EFIMEMTYPE_XDL_PAGE        0x80000004  //  loader XDL page
#define K2OS_EFIMEMTYPE_XDL_LOADER      0x80000005  //  loader master page
#define K2OS_EFIMEMTYPE_ZERO            0x80000006  //  one and only zero page
#define K2OS_EFIMEMTYPE_CORES           0x80000007  //  4 pages per core
#define K2OS_EFIMEMTYPE_PHYS_TRACK      0x80000008  //  used for tracking physical memory
#define K2OS_EFIMEMTYPE_TRANSITION      0x80000009  //  one and only transition page
#define K2OS_EFIMEMTYPE_EFI_MAP         0x8000000A  //  EFI memory map
#define K2OS_EFIMEMTYPE_FW_TABLES       0x8000000B  //  Firmware ACPI and SMBIOS tables
#define K2OS_EFIMEMTYPE_ARCH_SPEC       0x8000000C  //  Architecture Specific reservation
#define K2OS_EFIMEMTYPE_BUILTIN         0x8000000D  //  Builtin Read Only File System (ROFS)
#define K2OS_EFIMEMTYPE_THREADPTRS      0x8000000E  //  thread pointers page 
#define K2OS_EFIMEMTYPE_LAST            K2OS_EFIMEMTYPE_THREADPTRS

/* --------------------------------------------------------------------------------- */

#define K2OS_MEMPAGE_ATTR_NONE                      0
#define K2OS_MEMPAGE_ATTR_EXEC                      PF_X        // elf32, 0x00000001
#define K2OS_MEMPAGE_ATTR_WRITEABLE                 PF_W        // elf32, 0x00000002
#define K2OS_MEMPAGE_ATTR_READABLE                  PF_R        // elf32, 0x00000004
#define K2OS_MEMPAGE_ATTR_READWRITE                 (K2OS_MEMPAGE_ATTR_READABLE | K2OS_MEMPAGE_ATTR_WRITEABLE)
#define K2OS_MEMPAGE_ATTR_UNCACHED                  0x00000008
#define K2OS_MEMPAGE_ATTR_DEVICEIO                  0x00000010
#define K2OS_MEMPAGE_ATTR_WRITE_THRU                0x00000020
#define K2OS_MEMPAGE_ATTR_USER                      0x00000040
#define K2OS_MEMPAGE_ATTR_PAGING                    0x00000080
#define K2OS_MEMPAGE_ATTR_TRANSITION                0x00000100
#define K2OS_MEMPAGE_ATTR_AUX                       0x00000200 
#define K2OS_MEMPAGE_ATTR_SPEC_NP                   0x00000400 
#define K2OS_MEMPAGE_ATTR_COPY_ON_WRITE             0x00000800
#define K2OS_MEMPAGE_ATTR_MASK                      0x00000FFF  // can't be any bigger

/* --------------------------------------------------------------------------------- */

#define iK2OS_MAPTYPE_READ                          K2OS_MEMPAGE_ATTR_READABLE
#define iK2OS_MAPTYPE_TEXT                          (iK2OS_MAPTYPE_READ | K2OS_MEMPAGE_ATTR_EXEC)
#define iK2OS_MAPTYPE_DATA                          (iK2OS_MAPTYPE_READ | K2OS_MEMPAGE_ATTR_WRITEABLE)
#define iK2OS_MAPTYPE_DEVICEIO                      (iK2OS_MAPTYPE_DATA | K2OS_MEMPAGE_ATTR_DEVICEIO)
#define iK2OS_MAPTYPE_WRITETHRU_CACHED              (iK2OS_MAPTYPE_DATA | K2OS_MEMPAGE_ATTR_WRITE_THRU)
#define iK2OS_MAPTYPE_UNCACHED                      (iK2OS_MAPTYPE_DATA | K2OS_MEMPAGE_ATTR_UNCACHED)

#define K2OS_MAPTYPE_KERN_TEXT                      iK2OS_MAPTYPE_TEXT            
#define K2OS_MAPTYPE_KERN_READ                      iK2OS_MAPTYPE_READ
#define K2OS_MAPTYPE_KERN_DATA                      iK2OS_MAPTYPE_DATA            
#define K2OS_MAPTYPE_KERN_DEVICEIO                  iK2OS_MAPTYPE_DEVICEIO        
#define K2OS_MAPTYPE_KERN_WRITETHRU_CACHED          iK2OS_MAPTYPE_WRITETHRU_CACHED
#define K2OS_MAPTYPE_KERN_UNCACHED                  iK2OS_MAPTYPE_UNCACHED        

#define K2OS_MAPTYPE_USER_TEXT                      (K2OS_MEMPAGE_ATTR_USER | iK2OS_MAPTYPE_TEXT)         
#define K2OS_MAPTYPE_USER_READ                      (K2OS_MEMPAGE_ATTR_USER | iK2OS_MAPTYPE_READ)
#define K2OS_MAPTYPE_USER_DATA                      (K2OS_MEMPAGE_ATTR_USER | iK2OS_MAPTYPE_DATA)
#define K2OS_MAPTYPE_USER_DEVICEIO                  (K2OS_MEMPAGE_ATTR_USER | iK2OS_MAPTYPE_DEVICEIO)
#define K2OS_MAPTYPE_USER_WRITETHRU_CACHED          (K2OS_MEMPAGE_ATTR_USER | iK2OS_MAPTYPE_WRITETHRU_CACHED)
#define K2OS_MAPTYPE_USER_UNCACHED                  (K2OS_MEMPAGE_ATTR_USER | iK2OS_MAPTYPE_UNCACHED)
#define K2OS_MAPTYPE_USER_COPY_ON_WRITE             (K2OS_MAPTYPE_USER_READ | K2OS_MEMPAGE_ATTR_COPY_ON_WRITE)
#define K2OS_MAPTYPE_KERN_TRANSITION                (K2OS_MAPTYPE_KERN_UNCACHED | K2OS_MEMPAGE_ATTR_EXEC | K2OS_MEMPAGE_ATTR_TRANSITION)
                                                       
#define K2OS_MAPTYPE_KERN_PAGETABLE                 (K2OS_MAPTYPE_KERN_DATA | K2OS_MEMPAGE_ATTR_PAGING)
#define K2OS_MAPTYPE_KERN_PAGEDIR                   K2OS_MAPTYPE_KERN_PAGETABLE

/* --------------------------------------------------------------------------------- */

#define K2OS_THREAD_WAIT_MAX_ITEMS                  32

#define K2OS_THREAD_WAIT_SIGNALLED_0                0
#define K2OS_THREAD_WAIT_MAILBOX_SIGNALLED          K2OS_THREAD_WAIT_MAX_ITEMS
#define K2OS_THREAD_WAIT_ABANDONED_0                256
#define K2OS_THREAD_WAIT_FAILED_0                   512

/* --------------------------------------------------------------------------------- */

// match K2OSDRV_RESTYPE_xxx constants 
#define K2OS_PLATRES_MIN    K2OS_PLATRES_IO
#define K2OS_PLATRES_IO     1
#define K2OS_PLATRES_PHYS   2
#define K2OS_PLATRES_IRQ    3
#define K2OS_PLATRES_MAX    K2OS_PLATRES_IRQ

/* --------------------------------------------------------------------------------- */

#endif // __K2OS_DEFS_INC
