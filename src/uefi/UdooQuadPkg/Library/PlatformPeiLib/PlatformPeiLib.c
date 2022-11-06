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

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <UdooQuad.h>

//
// sanity
//
#define STATIC_ASSERT(x) \
typedef int check ## __COUNTER__ [ (x) ? 1 : -1 ];
STATIC_ASSERT(((UINT32)FixedPcdGet64(PcdSystemMemoryBase)) == UDOOQUAD_MAINRAM_PHYSICAL_BASE);
STATIC_ASSERT(((UINT32)FixedPcdGet64(PcdSystemMemorySize)) == UDOOQUAD_MAINRAM_PHYSICAL_LENGTH);

EFI_STATUS
EFIAPI
PlatformPeim(
    VOID
)
{
    EFI_PEI_HOB_POINTERS    Hob;

    //
    // find stack HOB. change its memory allocation type to EfiReservedMemoryType
    // so that it will not be reclaimed by OS.  Secondary core stacks must survive
    // to secondary core statup code in OS, or they will/can crash coming out of
    // their WFI loop, as they use regular function calls when they come out of the
    // loop
    //
    Hob.Raw = GetHobList();
    while ((Hob.Raw = GetNextHob(EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) 
    {
        if (CompareGuid(&gEfiHobMemoryAllocStackGuid, &(Hob.MemoryAllocationStack->AllocDescriptor.Name))) 
        {
            if (EfiBootServicesData == Hob.MemoryAllocationStack->AllocDescriptor.MemoryType)
            {
                Hob.MemoryAllocationStack->AllocDescriptor.MemoryType = EfiReservedMemoryType;
            }
            break;
        }
        Hob.Raw = GET_NEXT_HOB(Hob);
    }
    if (NULL == Hob.Raw)
    {
        DebugPrint(0xFFFFFFFF, "!!!Could not find Stack hob\n");
    }

    //
    // add PEI FV
    //
    BuildFvHob(PcdGet64(PcdFvBaseAddress), PcdGet32(PcdFvSize));

    return EFI_SUCCESS;
}
