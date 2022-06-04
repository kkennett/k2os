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

#ifndef __VAGENERICDEF_INC__
#define __VAGENERICDEF_INC__

#include "plat/VirtualArm/virtarm.inc"

#define VAGENERIC_PHYSSIZE_BOOTROM  0x40000

#include "plat/VirtualArm/Adapters/VADebugAdapterRegs.inc"
#define VAGENERIC_DEBUG_PHYSADDR_SRAM       VIRTARM_PHYSADDR_ADAPTER00_SRAM
#define VAGENERIC_DEBUG_PHYSADDR_REGS       VIRTARM_PHYSADDR_ADAPTER00_REGS

#include "plat/VirtualArm/Adapters/VARealTimeAdapterRegs.inc"
#define VAGENERIC_REALTIME_PHYSADDR_SRAM    VIRTARM_PHYSADDR_ADAPTER01_SRAM
#define VAGENERIC_REALTIME_PHYSADDR_SRAM    VIRTARM_PHYSADDR_ADAPTER01_SRAM


#endif  // __VAGENERICDEF_INC__
