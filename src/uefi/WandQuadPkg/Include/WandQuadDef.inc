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

#ifndef __WANDQUADDEF_INC__
#define __WANDQUADDEF_INC__

#include "IMX6def.inc"

// DDR attributes
#define DDR_ATTRIBUTES_CACHED                       ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED                     ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

// OCRAM
#define WANDQUAD_OCRAM_PHYSICAL_BASE                IMX6_PHYSADDR_OCRAM
#define WANDQUAD_OCRAM_PHYSICAL_LENGTH              IMX6_PHYSSIZE_OCRAM
#define WANDQUAD_OCRAM_SEC_STACKS_MEM               IMX6_PHYSADDR_OCRAM_FREE
#define WANDQUAD_OCRAM_SEC_STACKS_SIZE_PERCORE      0x400
#define WANDQUAD_OCRAM_PHYSICAL_PARK_PAGES          ((IMX6_PHYSADDR_OCRAM_FREE + IMX6_PHYSSIZE_OCRAM_FREE) - (4 * 0x1000))
#define WANDQUAD_OCRAM_PHYSICAL_SHARED_DATA_PAGE    (WANDQUAD_OCRAM_PHYSICAL_PARK_PAGES - 0x1000)
#define WANDQUAD_OCRAM_ATTRIBUTES                   ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

#define WANDQUAD_ALLRAM_PHYSICAL_LENGTH             0x80000000

#define WANDQUAD_MAINRAM_PHYSICAL_BASE              IMX6_PHYSADDR_SDRAM_BASE
#define WANDQUAD_MAINRAM_PHYSICAL_LENGTH            WANDQUAD_ALLRAM_PHYSICAL_LENGTH

#define WANDQUAD_WDOG_PHYSADDR                      IMX6_PHYSADDR_WDOG1
#define WANDQUAD_SNVS_PHYSADDR                      IMX6_PHYSADDR_SNVS

#define WANDQUAD_GPT_SRC_FREQ_HZ                    IMX6_HIGH_OSC_CLOCK_FREQ

#define WANDQUAD_HIGH_FREQUENCY_REF_HZ              66000000

#define WANDQUAD_DEBUG_UART                         IMX6_PHYSADDR_UART1

#endif  // __WANDQUADDEF_INC__
