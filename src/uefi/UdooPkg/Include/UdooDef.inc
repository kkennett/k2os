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

#ifndef __UDOODEF_INC__
#define __UDOODEF_INC__

#include "IMX6def.inc"

#define WANDBOARD 0

// DDR attributes
#define DDR_ATTRIBUTES_CACHED                       ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED                     ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

#define UDOO_PHYSADDR_TEMP_RAM_AREA                 IMX6_PHYSADDR_OCRAM_FREE
#define UDOO_PHYSSIZE_TEMP_RAM_AREA                 0x05000

#define UDOO_PHYSADDR_PERCPU_PAGES                  (UDOO_PHYSADDR_TEMP_RAM_AREA + UDOO_PHYSSIZE_TEMP_RAM_AREA)
#define UDOO_PHYSSIZE_PERCPU_PAGES                  (0x1000 * 4)

// these sit at the end of the per-core pages, just above the nonsecure initialization stacks
#define UDOO_PHYSSIZE_PERCPU_SEC_STACK              0x400

#define UDOO_PHYSADDR_SEC_FD                        (UDOO_PHYSADDR_PERCPU_PAGES + UDOO_PHYSSIZE_PERCPU_PAGES)
#define UDOO_PHYSSIZE_SEC_FD                        0x31000

#define UDOO_OCRAM_ATTRIBUTES                       ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

#if WANDBOARD
#define UDOO_ALLRAM_PHYSICAL_LENGTH                 0x80000000
#else
#define UDOO_ALLRAM_PHYSICAL_LENGTH                 0x40000000
#endif

#define UDOO_BOOT_USDHC_UNITNUM                     3

#define UDOO_MAINRAM_PHYSICAL_BASE                  IMX6_PHYSADDR_SDRAM_BASE
#define UDOO_MAINRAM_PHYSICAL_LENGTH                UDOO_ALLRAM_PHYSICAL_LENGTH

#define UDOO_PHYSADDR_VARSHADOW                     UDOO_MAINRAM_PHYSICAL_BASE
#define UDOO_PHYSADDR_VARSHADOW_VARBASE             UDOO_PHYSADDR_VARSHADOW
//------------------ START OF AREA SHADOWED FROM DISK (size 0x0004F000 starting at sector 392, offset 0x00031000)-------------------
#define UDOO_PHYSSIZE_VARSHADOW_VARBASE             0x0001A000
#define UDOO_PHYSADDR_VARSHADOW_WORKING             (UDOO_PHYSADDR_VARSHADOW_VARBASE + UDOO_PHYSSIZE_VARSHADOW_VARBASE)
#define UDOO_PHYSSIZE_VARSHADOW_WORKING             0x0001B000
#define UDOO_PHYSADDR_VARSHADOW_SPARE               (UDOO_PHYSADDR_VARSHADOW_WORKING + UDOO_PHYSSIZE_VARSHADOW_WORKING)
#define UDOO_PHYSSIZE_VARSHADOW_SPARE               0x0001A000
//------------------ END OF AREA SHADOWED FROM DISK -------------------
#define UDOO_PHYSSIZE_VARSHADOW                     (UDOO_PHYSSIZE_VARSHADOW_VARBASE + UDOO_PHYSSIZE_VARSHADOW_WORKING + UDOO_PHYSSIZE_VARSHADOW_SPARE)

#define UDOO_PHYSADDR_PEIFV_FILE_BASE               (UDOO_PHYSADDR_VARSHADOW + UDOO_PHYSSIZE_VARSHADOW)

#define UDOO_WDOG_PHYSADDR                          IMX6_PHYSADDR_WDOG1
#define UDOO_SNVS_PHYSADDR                          IMX6_PHYSADDR_SNVS

#define UDOO_GPT_SRC_FREQ_HZ                        IMX6_HIGH_OSC_CLOCK_FREQ

#define UDOO_HIGH_FREQUENCY_REF_HZ                  66000000

#define SD_BLOCK_SIZE_POW2      9
#define SD_BLOCK_SIZE           (1ul << SD_BLOCK_SIZE_POW2)
#define SD_BLOCK_OFFSET_MASK    ((1ul << SD_BLOCK_SIZE_POW2)-1)
#define SD_BLOCK_NUMBER_MASK    (~SD_BLOCK_OFFSET_MASK)

#endif  // __UDOODEF_INC__
