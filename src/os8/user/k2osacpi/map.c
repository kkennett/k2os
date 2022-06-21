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

#include "ik2osacpi.h"

void *
AcpiOsMapMemory(
    ACPI_PHYSICAL_ADDRESS   Where,
    ACPI_SIZE               Length)
{
    UINT32  target;
   
    if ((Where >= gInit.mFwBasePhys) && ((Where - gInit.mFwBasePhys) < gInit.mFwSizeBytes))
    {
        if (Length <= (gInit.mFwSizeBytes - (Where - gInit.mFwBasePhys)))
        {
            target = (((UINT32)Where) - gInit.mFwBasePhys) + gInit.mFwBaseVirt;
            return (void *)target;
        }
        else
        {
            return NULL;
        }
    }

    if (gInit.mFacsPhys != 0)
    {
        if ((Where >= gInit.mFacsPhys) &&
            ((Where - gInit.mFacsPhys) < K2_VA_MEMPAGE_BYTES))
        {
            if (Length <= (K2OS_KVA_FACS_SIZE - (Where - gInit.mFacsPhys)))
            {
                target = (((UINT32)Where) - gInit.mFacsPhys) + gInit.mFacsVirt;
                return (void *)target;
            }
            else
            {
                return NULL;
            }
        }
    }
    else
    {
        if ((Where >= gInit.mXFacsPhys) && ((Where - gInit.mXFacsPhys) < K2_VA_MEMPAGE_BYTES))
        {
            if (Length <= (K2_VA_MEMPAGE_BYTES - (Where - gInit.mXFacsPhys)))
            {
                target = (((UINT32)Where) - gInit.mXFacsPhys) + gInit.mXFacsVirt;
                return (void *)target;
            }
            else
            {
                return NULL;
            }
        }
    }

    return NULL;
}

void
AcpiOsUnmapMemory(
    void                    *LogicalAddress,
    ACPI_SIZE               Size)
{
    UINT32  virtAddr;

    virtAddr = (UINT32)LogicalAddress;

    if ((virtAddr >= gInit.mFwBaseVirt) && ((virtAddr - gInit.mFwBaseVirt) < gInit.mFwSizeBytes))
    {
        if (Size <= (gInit.mFwSizeBytes - (virtAddr - gInit.mFwBaseVirt)))
        {
            return;
        }
    }

    if (gInit.mFacsPhys != 0)
    {
        if ((virtAddr >= gInit.mFacsVirt) && ((virtAddr - gInit.mFacsVirt) < K2_VA_MEMPAGE_BYTES))
        {
            if (Size <= (K2_VA_MEMPAGE_BYTES - (virtAddr - gInit.mFacsVirt)))
            {
                return;
            }
        }
    }
    else
    {
        if ((virtAddr >= gInit.mXFacsVirt) && ((virtAddr - gInit.mXFacsVirt) < K2_VA_MEMPAGE_BYTES))
        {
            if (Size <= (K2_VA_MEMPAGE_BYTES - (virtAddr - gInit.mXFacsVirt)))
            {
                return;
            }
        }
    }

    K2_ASSERT(0);
}

