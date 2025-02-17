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

#ifndef __KERNDEF_INC
#define __KERNDEF_INC

#include "k2osdefs.inc"

/* --------------------------------------------------------------------------------- */

#define K2OSKERN_PHYSTRACK_FLAGS_EXISTS             0x00000001
#define K2OSKERN_PHYSTRACK_FLAGS_FREE               0x00000002

#define K2OSKERN_PHYSTRACK_FLAGS_PROP_MASK          0x000003FC

#define K2OSKERN_PHYSTRACK_FLAGS_PROP_XP_CAP        0x00000200  // The memory region supports being configured so it is protected by system hardware from executing code.
#define K2OSKERN_PHYSTRACK_FLAGS_PROP_RP_CAP        0x00000100  // The memory region supports being configured as read-protected by system hardware.
#define K2OSKERN_PHYSTRACK_FLAGS_PROP_WP_CAP        0x00000080  // The memory region supports being configured as write-protected by system hardware.
#define K2OSKERN_PHYSTRACK_FLAGS_PROP_UCE_CAP       0x00000040  // supports being configured as not cacheable, exported, and supports the “fetch and add” semaphore mechanism.
#define K2OSKERN_PHYSTRACK_FLAGS_PROP_WB_CAP        0x00000020  // supports being configured as cacheable with a “write back” policy. Reads and writes that hit in the cache do not propagate to main memory. Dirty data is written back to main memory when a new cache line is allocated.
#define K2OSKERN_PHYSTRACK_FLAGS_PROP_WT_CAP        0x00000010  // supports being configured as cacheable with a “write through” policy. Writes that hit in the cache will also be written to main memory.
#define K2OSKERN_PHYSTRACK_FLAGS_PROP_WC_CAP        0x00000008  // supports being configured as write combining.
#define K2OSKERN_PHYSTRACK_FLAGS_PROP_UC_CAP        0x00000004  // supports being configured as not cacheable.

#define K2OSKERN_PHYSTRACK_FLAGS_MASK_UNCACHEABLE   (K2OSKERN_PHYSTRACK_FLAGS_PROP_UCE_CAP | K2OSKERN_PHYSTRACK_FLAGS_PROP_UC_CAP)

#define K2OSKERN_PHYSTRACK_FLAGS_PAGELIST_MASK      0x00007C00
#define K2OSKERN_PHYSTRACK_FLAGS_PAGELIST_SHL       10
#define K2OSKERN_PHYSTRACK_FLAGS_PAGELIST_GET(x)    (((x) & K2OSKERN_PHYSTRACK_FLAGS_PAGELIST_MASK) >> K2OSKERN_PHYSTRACK_FLAGS_PAGELIST_SHL)

#define K2OSKERN_PHYSTRACK_FLAGS_BLOCKSIZE_MASK     0x000F8000
#define K2OSKERN_PHYSTRACK_FLAGS_BLOCKSIZE_SHL      15 
#define K2OSKERN_PHYSTRACK_FLAGS_BLOCKSIZE_GET(x)   (((x) & K2OSKERN_PHYSTRACK_FLAGS_BLOCKSIZE_MASK) >> K2OSKERN_PHYSTRACK_FLAGS_BLOCKSIZE_SHL)

/* --------------------------------------------------------------------------------- */

#if K2_TARGET_ARCH_IS_ARM
#define K2OSKERN_PDE_PRESENT_BIT            A32_TTBE_PT_PRESENT
#define K2OSKERN_PTE_PRESENT_BIT            A32_PTE_PRESENT
#define K2OSKERN_PTE_NP_BIT                 0x00000001
#else
#define K2OSKERN_PDE_PRESENT_BIT            X32_PDE_PRESENT
#define K2OSKERN_PTE_PRESENT_BIT            X32_PTE_PRESENT
#define K2OSKERN_PTE_NP_BIT                 0x00000002
#endif
#define K2OSKERN_PTE_NP_TYPE_MASK           0x0000000C
#define K2OSKERN_PTE_NP_TYPE_1              0x0
#define K2OSKERN_PTE_NP_TYPE_2              0x4
#define K2OSKERN_PTE_NP_TYPE_3              0x8
#define K2OSKERN_PTE_NP_TYPE_4              0xC
#define K2OSKERN_PTE_NP_CONTENT_MASK        0xFFFFFFF0    // 256M possible entries

#define K2OSKERN_PTE_NP_1                   (K2OSKERN_PTE_NP_TYPE_1 | K2OSKERN_PTE_NP_BIT)
#define K2OSKERN_PTE_NP_2                   (K2OSKERN_PTE_NP_TYPE_2 | K2OSKERN_PTE_NP_BIT)
#define K2OSKERN_PTE_NP_3                   (K2OSKERN_PTE_NP_TYPE_3 | K2OSKERN_PTE_NP_BIT)
#define K2OSKERN_PTE_NP_4                   (K2OSKERN_PTE_NP_TYPE_4 | K2OSKERN_PTE_NP_BIT)

#define K2OSKERN_PTE_UNUSED                 K2OSKERN_PTE_NP_1
#define K2OSKERN_PTE_MAP_CREATE             K2OSKERN_PTE_NP_2
#define K2OSKERN_PTE_HEAPMAP_PENDING        K2OSKERN_PTE_NP_3
#define K2OSKERN_PTE_STACK_GUARD            K2OSKERN_PTE_NP_4

/* --------------------------------------------------------------------------------- */

#endif // __KERNDEF_INC