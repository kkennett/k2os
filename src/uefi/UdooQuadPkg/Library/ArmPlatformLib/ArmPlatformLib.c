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
#include <UdooQuad.h>
#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/PrePiLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/SerialPortLib.h>
#include <IMX6def.inc>
#include <Ppi/ArmMpCoreInfo.h>

//------------------------------------------------------------------------

void MmioSetBits32(UINT32 Addr, UINT32 Bits)
{
    MmioWrite32(Addr, MmioRead32(Addr) | Bits);
}

void MmioClrBits32(UINT32 Addr, UINT32 Bits)
{
    MmioWrite32(Addr, MmioRead32(Addr) & ~Bits);
}

#if 0

void MmioSetBits16(UINT32 Addr, UINT16 Bits)
{
    MmioWrite16(Addr, MmioRead16(Addr) | Bits);
}

void MmioClrBits16(UINT32 Addr, UINT16 Bits)
{
    MmioWrite16(Addr, MmioRead16(Addr) & ~Bits);
}

//------------------------------------------------------------------------

static void sTimerSpinDelayUs(UINT32 aMicroseconds)
{
    UINT32 c;
    UINT32 e;

    c = MmioRead32(IMX6_PHYSADDR_GPT_CNT);
    e = c + aMicroseconds;
    if (e < c)
    {
        while (e <= MmioRead32(IMX6_PHYSADDR_GPT_CNT));
    }
    while (MmioRead32(IMX6_PHYSADDR_GPT_CNT) < e);
}

#endif 

RETURN_STATUS
EFIAPI
TimerConstructor(
    VOID
)
{
    return EFI_SUCCESS;
}


static void sInitSDHCHw(void)
{
    UINT32 regVal;

    // all SD pads have this config
    regVal =
        IMX6_MUX_PADCTL_HYS |
        IMX6_MUX_PADCTL_PUS_47KOHM_PU |
        IMX6_MUX_PADCTL_SPEED_50MHZ |
        IMX6_MUX_PADCTL_DSE_80_OHM |
        IMX6_MUX_PADCTL_SRE_FAST;

    // USDHC2 is SD controller.  CD input is on GPIO6-2 and Power output is on GPIO6-1
    // CD and power(on) already set up in GPIO setup above
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_CLK  , 0);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_CMD  , 0);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA0, 0);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA1, 0);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA2, 0);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA3, 0);

    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_CLK  , regVal);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_CMD  , regVal);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA0, regVal);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA1, regVal);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA2, regVal);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA3, regVal);

    regVal = IMX6_MUX_PADCTL_SPEED_100MHZ |
        IMX6_MUX_PADCTL_PKE |
        IMX6_MUX_PADCTL_DSE_40_OHM |
        IMX6_MUX_PADCTL_SRE_SLOW;

    //
    // CD is on SD3_DAT5 - GPIO7_IO00, input
    //
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA5, 5);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA5, regVal);
    MmioClrBits32(IMX6_PHYSADDR_GPIO7_GDIR, 1 << 0);

    //
    // PWR is on NANDF_D5 - GPIO2_IO05, output 
    //
    MmioSetBits32(IMX6_PHYSADDR_GPIO2_DR, 1 << 5);
    MmioSetBits32(IMX6_PHYSADDR_GPIO2_GDIR, 1 << 5);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_DATA05, regVal);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_DATA05, 5);
};

static
void
sInitWatchdog(
    void
    )
{
    // set values to defaults for both watchdogs
    MmioWrite16(IMX6_PHYSADDR_WDOG2_WCR , 0x0030);
    MmioWrite16(IMX6_PHYSADDR_WDOG2_WSR , 0x0000);
    MmioWrite16(IMX6_PHYSADDR_WDOG2_WICR, 0x0004);
    MmioWrite16(IMX6_PHYSADDR_WDOG2_WMCR, 0x0000);

    MmioWrite16(IMX6_PHYSADDR_WDOG1_WCR , 0x0030);
    MmioWrite16(IMX6_PHYSADDR_WDOG1_WSR , 0x0000);
    MmioWrite16(IMX6_PHYSADDR_WDOG1_WICR, 0x0004);
    MmioWrite16(IMX6_PHYSADDR_WDOG1_WMCR, 0x0000);
}

RETURN_STATUS
ArmPlatformInitialize (
  IN  UINTN                     MpId
  )
{
    if (ArmPlatformIsPrimaryCore (MpId)) {
      sInitSDHCHw();
      sInitWatchdog();
    }
    return RETURN_SUCCESS;
}

//------------------------------------------------------------------------

#define CPU0_PARK   (UDOOQUAD_OCRAM_PHYSICAL_PARK_PAGES + (0 * 0x1000))
#define CPU1_PARK   (UDOOQUAD_OCRAM_PHYSICAL_PARK_PAGES + (1 * 0x1000))
#define CPU2_PARK   (UDOOQUAD_OCRAM_PHYSICAL_PARK_PAGES + (2 * 0x1000))
#define CPU3_PARK   (UDOOQUAD_OCRAM_PHYSICAL_PARK_PAGES + (3 * 0x1000))

static
ARM_CORE_INFO IMX6_Ppi[] = 
{
  {
    0x0, 0x0,
    (EFI_PHYSICAL_ADDRESS)CPU0_PARK,
    (EFI_PHYSICAL_ADDRESS)CPU0_PARK,
    (EFI_PHYSICAL_ADDRESS)CPU0_PARK,
    (UINT64)0
  },
  {
    0x0, 0x1,
    (EFI_PHYSICAL_ADDRESS)CPU1_PARK,
    (EFI_PHYSICAL_ADDRESS)CPU1_PARK,
    (EFI_PHYSICAL_ADDRESS)CPU1_PARK,
    (UINT64)0
  },
  {
    0x0, 0x2,
    (EFI_PHYSICAL_ADDRESS)CPU2_PARK,
    (EFI_PHYSICAL_ADDRESS)CPU2_PARK,
    (EFI_PHYSICAL_ADDRESS)CPU2_PARK,
    (UINT64)0
  },
  {
    0x0, 0x3,
    (EFI_PHYSICAL_ADDRESS)CPU3_PARK,
    (EFI_PHYSICAL_ADDRESS)CPU3_PARK,
    (EFI_PHYSICAL_ADDRESS)CPU3_PARK,
    (UINT64)0
  }
};

static
EFI_STATUS
sGetCoreInfo (
  OUT UINTN                   *CoreCount,
  OUT ARM_CORE_INFO           **ArmCoreTable
  )
{
    *CoreCount    = 4;
    *ArmCoreTable = IMX6_Ppi;
    return EFI_SUCCESS;
}

static EFI_GUID             sgArmMpCoreInfoPpiGuid = ARM_MP_CORE_INFO_PPI_GUID;
static ARM_MP_CORE_INFO_PPI sgCoreInfoPpi = { sGetCoreInfo };
EFI_PEI_PPI_DESCRIPTOR gUdooQuadPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &sgArmMpCoreInfoPpiGuid,
    &sgCoreInfoPpi
  }
};

VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
    *PpiListSize = sizeof(gUdooQuadPpiTable);
    *PpiList = gUdooQuadPpiTable;
}

//------------------------------------------------------------------------

EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  )
{
    return BOOT_WITH_FULL_CONFIGURATION;
}

//------------------------------------------------------------------------

VOID
ArmPlatformInitializeSystemMemory (
  VOID
  )
{
}

//------------------------------------------------------------------------

#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS 6

VOID
ArmPlatformGetVirtualMemoryMap (
  IN ARM_MEMORY_REGION_DESCRIPTOR** VirtualMemoryMap
  )
{
    //
    // PLATFORM IS IN PHYSICAL MODE WITH MMU OFF
    //
    
    UINTN                         Index = 0;
    ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;

    ASSERT(VirtualMemoryMap != NULL);

    VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR*)AllocatePages(EFI_SIZE_TO_PAGES (sizeof(ARM_MEMORY_REGION_DESCRIPTOR) * MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS));
    if (VirtualMemoryTable == NULL) 
        return;

    // BOOT ROM
    VirtualMemoryTable[Index].PhysicalBase = IMX6_PHYSADDR_BOOTROM;
    VirtualMemoryTable[Index].VirtualBase  = IMX6_PHYSADDR_BOOTROM;
    VirtualMemoryTable[Index].Length       = IMX6_PHYSSIZE_BOOTROM;
    VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_UNCACHED_UNBUFFERED;
    ++Index;

    // IMX6 AIPS1, AIPS2 REGISTERS
    VirtualMemoryTable[Index].PhysicalBase = IMX6_PHYSADDR_AIPS1;
    VirtualMemoryTable[Index].VirtualBase  = IMX6_PHYSADDR_AIPS1;
    VirtualMemoryTable[Index].Length       = (IMX6_PHYSSIZE_AIPS1 + IMX6_PHYSSIZE_AIPS2);
    VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_DEVICE;
    ++Index;

    // IMX6 ARM peripherals and PL310 cache controller
    VirtualMemoryTable[Index].PhysicalBase = IMX6_PHYSADDR_ARM_PERIPH;
    VirtualMemoryTable[Index].VirtualBase  = IMX6_PHYSADDR_ARM_PERIPH;
    VirtualMemoryTable[Index].Length       = (IMX6_PHYSSIZE_ARM_PERIPH + IMX6_PHYSSIZE_PL310);
    VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_DEVICE;
    ++Index;

    // OCRAM
    VirtualMemoryTable[Index].PhysicalBase = IMX6_PHYSADDR_OCRAM;
    VirtualMemoryTable[Index].VirtualBase  = IMX6_PHYSADDR_OCRAM;
    VirtualMemoryTable[Index].Length       = IMX6_PHYSSIZE_OCRAM;
    VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_UNCACHED_UNBUFFERED;
    ++Index;

    // DRAM
    VirtualMemoryTable[Index].PhysicalBase = PcdGet64 (PcdSystemMemoryBase);
    VirtualMemoryTable[Index].VirtualBase  = PcdGet64 (PcdSystemMemoryBase);
    VirtualMemoryTable[Index].Length       = PcdGet64 (PcdSystemMemorySize);
    VirtualMemoryTable[Index].Attributes   = DDR_ATTRIBUTES_CACHED;
    ++Index;

    // End of Table
    VirtualMemoryTable[Index].PhysicalBase = 0;
    VirtualMemoryTable[Index].VirtualBase  = 0;
    VirtualMemoryTable[Index].Length       = 0;
    VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)0;
    ++Index;

    ASSERT(Index == MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);

    *VirtualMemoryMap = VirtualMemoryTable;
}

