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

#if K2_TARGET_ARCH_IS_INTEL
#include <spec/pcdef.inc>

#define MAKE_PCICONFIG_ADDR(bus,device,function,reg) \
    (0x80000000ul | \
    ((((UINT32)bus)&0xFFul) << 16) | \
    ((((UINT32)device)&0x1Ful) << 11) | \
    ((((UINT32)function)&0x7ul) << 8) | \
    ((((UINT32)reg)&0xFC)))

#endif

//
// this is for port access.  memory mapped access if applicable is done 
// via the overrides installed by k2osexec that knows about the physical
// address space and the location of the enhanced configuration access method (ECAM)
//

static K2OSACPI_pf_ReadPciConfiguration    gfOverrideRead = NULL;
static K2OSACPI_pf_WritePciConfiguration   gfOverrideWrite = NULL;

K2STAT
K2OSACPI_SetPciConfigOverride(
    K2OSACPI_pf_ReadPciConfiguration    afRead,
    K2OSACPI_pf_WritePciConfiguration   afWrite
)
{
    K2_ASSERT(afRead != NULL);
    gfOverrideRead = afRead;
    
    K2_ASSERT(afWrite != NULL);
    gfOverrideWrite = afWrite;

    return K2STAT_NO_ERROR;
}

ACPI_STATUS
AcpiOsReadPciConfiguration(
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  *Value,
    UINT32                  Width
)
{
#if K2_TARGET_ARCH_IS_INTEL
    UINT32 addr;
    UINT32 val32;
#endif

    K2_ASSERT(0 == (Width & 7));
    K2_ASSERT((Width >> 3) <= 4);
    if (Reg & 1)
    {
        K2_ASSERT(Width == 8);
    }
    else if ((Reg & 3) == 2)
    {
        K2_ASSERT(Width <= 16);
    }

#if K2_TARGET_ARCH_IS_INTEL
    if (NULL == gfOverrideRead)
    {
//        AcpiOsPrintf("RawPciRead(%d,%d,%d,%04X)\n", PciId->Bus, PciId->Device, PciId->Function, Reg);

        addr = MAKE_PCICONFIG_ADDR(PciId->Bus, PciId->Device, PciId->Function, Reg);
        X32_IoWrite32(addr, PC_PCI_CONFIG_ADDR_IOPORT);
        val32 = X32_IoRead32(PC_PCI_CONFIG_DATA_IOPORT) >> ((Reg & 3) * 8);

        switch (Width)
        {
        case 8:
            *Value = (UINT64)(val32 & 0xFF);
            break;
        case 16:
            *Value = (UINT64)(val32 & 0xFFFF);
            break;
        case 32:
            *Value = (UINT64)val32;
            break;
        default:
            *Value = 0;
            K2_ASSERT(0);
        }
        return AE_OK;
    }
#else
    if (NULL == gfOverrideRead)
    {
        *Value = 0;
        return AE_ERROR;
    }
#endif

    return gfOverrideRead(PciId, Reg, Value, Width);
}

ACPI_STATUS
AcpiOsWritePciConfiguration(
    ACPI_PCI_ID *   PciId,
    UINT32          Reg,
    UINT64          Value,
    UINT32          Width)
{
#if K2_TARGET_ARCH_IS_INTEL
    UINT32 addr;
    UINT32 val32;
    UINT32 mask32;
#endif

    K2_ASSERT(Width != 0);
    K2_ASSERT(0 == (Width & 7));
    K2_ASSERT((Width >> 3) <= 4);
    if (Reg & 1)
    {
        K2_ASSERT(Width == 8);
    }
    else if ((Reg & 3) == 2)
    {
        K2_ASSERT(Width <= 16);
    }

#if K2_TARGET_ARCH_IS_INTEL
    if (NULL == gfOverrideWrite)
    {
//        AcpiOsPrintf("RawPciWrite(%d,%d,%d,%04X)\n", PciId->Bus, PciId->Device, PciId->Function, Reg);

        switch (Width)
        {
        case 8:
            val32 = ((UINT32)Value) & 0xFF;
            mask32 = 0xFF;
            break;
        case 16:
            val32 = ((UINT32)Value) & 0xFFFF;
            mask32 = 0xFFFF;
            break;
        case 32:
            val32 = ((UINT32)Value);
            mask32 = 0xFFFFFFFF;
            break;
        default:
            return AE_ERROR;
        }

        val32 <<= ((Reg & 3) * 8);
        mask32 <<= ((Reg & 3) * 8);

        addr = MAKE_PCICONFIG_ADDR(PciId->Bus, PciId->Device, PciId->Function, Reg);
        X32_IoWrite32(addr, PC_PCI_CONFIG_ADDR_IOPORT);
        X32_IoWrite32(((X32_IoRead32(PC_PCI_CONFIG_DATA_IOPORT) & ~mask32) | val32), PC_PCI_CONFIG_DATA_IOPORT);

        return AE_OK;
    }
#else
    if (NULL == gfOverrideWrite)
        return AE_ERROR;
#endif

    return gfOverrideWrite(PciId, Reg, Value, Width);
}

