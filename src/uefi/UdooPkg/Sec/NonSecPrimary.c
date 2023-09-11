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

#include "Sec.h"

#include <Pi\PiFirmwareVolume.h>
#include <Pi\PiFirmwareFile.h>

RETURN_STATUS
EFIAPI
LzmaUefiDecompressGetInfo(
    IN  CONST VOID  *Source,
    IN  UINT32      SourceSize,
    OUT UINT32      *DestinationSize,
    OUT UINT32      *ScratchSize
);

RETURN_STATUS
EFIAPI
LzmaUefiDecompress(
    IN CONST VOID  *Source,
    IN UINTN       SourceSize,
    IN OUT VOID    *Destination,
    IN OUT VOID    *Scratch
);

#if WANDBOARD

static UINT32 const sgDramMuxInit[] =
{
    0x020e0798, 0x000c0000, //  GRP_DDR_TYPE
    0x020e0758, 0x00000000, //  GRP_DDRPKE
    0x020e0588, 0x00000028, //  DRAM_SDCLK0_P
    0x020e0594, 0x00000028, //  DRAM_SDCLK1_P
    0x020e056c, 0x00000028, //  DRAM_CAS_B
    0x020e0578, 0x00000028, //  DRAM_RAS_B
    0x020e074c, 0x00000028, //  GRP_ADDDS
    0x020e057c, 0x00000028, //  DRAM_RESET
    0x020e0590, 0x00000028, //  DRAM_SDCKE0
    0x020e0598, 0x00000028, //  DRAM_SDCKE1
    0x020e058c, 0x00000000, //  DRAM_SDBA2
    0x020e059c, 0x00000028, //  DRAM_ODT0
    0x020e05a0, 0x00000028, //  DRAM_ODT1
    0x020e078c, 0x00000028, //  GRP_CTLDS
    0x020e0750, 0x00020000, //  GRP_DDRMODE_CTL
    0x020e05a8, 0x00000028, //  DRAM_SDQS0_P
    0x020e05b0, 0x00000028, //  DRAM_SDQS1_P
    0x020e0524, 0x00000028, //  DRAM_SDQS2_P
    0x020e051c, 0x00000028, //  DRAM_SDQS3_P
    0x020e0518, 0x00000028, //  DRAM_SDQS4_P
    0x020e050c, 0x00000028, //  DRAM_SDQS5_P
    0x020e05b8, 0x00000028, //  DRAM_SDQS6_P
    0x020e05c0, 0x00000028, //  DRAM_SDQS7_P
    0x020e0774, 0x00020000, //  GRP_DDRMODE
    0x020e0784, 0x00000028, //  GRP_B0DS
    0x020e0788, 0x00000028, //  GRP_B1DS
    0x020e0794, 0x00000028, //  GRP_B2DS
    0x020e079c, 0x00000028, //  GRP_B3DS
    0x020e07a0, 0x00000028, //  GRP_B4DS
    0x020e07a4, 0x00000028, //  GRP_B5DS
    0x020e07a8, 0x00000028, //  GRP_B6DS
    0x020e0748, 0x00000028, //  GRP_B7DS
    0x020e05ac, 0x00000028, //  DRAM_DQM0
    0x020e05b4, 0x00000028, //  DRAM_DQM1
    0x020e0528, 0x00000028, //  DRAM_DQM2
    0x020e0520, 0x00000028, //  DRAM_DQM3
    0x020e0514, 0x00000028, //  DRAM_DQM4
    0x020e0510, 0x00000028, //  DRAM_DQM5
    0x020e05bc, 0x00000028, //  DRAM_DQM6
    0x020e05c4, 0x00000028  //  DRAM_DQM7
};

static UINT32 const sgDramCfgInit[] =
{
    0x021b081c, 0x33333333, //  MMDC1_MPRDDQBY0DL
    0x021b0820, 0x33333333, //  MMDC1_MPRDDQBY1DL
    0x021b0824, 0x33333333, //  MMDC1_MPRDDQBY2DL
    0x021b0828, 0x33333333, //  MMDC1_MPRDDQBY3DL
    0x021b481c, 0x33333333, //  MMDC2_MPRDDQBY0DL
    0x021b4820, 0x33333333, //  MMDC2_MPRDDQBY1DL
    0x021b4824, 0x33333333, //  MMDC2_MPRDDQBY2DL
    0x021b4828, 0x33333333, //  MMDC2_MPRDDQBY3DL
    0x021b0004, 0x00020036, //  MMDC1_MDPDC
    0x021b0008, 0x09444040, //  MMDC1_MDOTC
    0x021b000c, 0x54597955, //  MMDC1_MDCFG0
    0x021b0010, 0xff328f64, //  MMDC1_MDCFG1
    0x021b0014, 0x01ff00db, //  MMDC1_MDCFG2
    0x021b0018, 0x00001740, //  MMDC1_MDMISC
    0x021b001c, 0x00008000, //  MMDC1_MDSCR
    0x021b002c, 0x000026d2, //  MMDC1_MDRWD
    0x021b0030, 0x00591023, //  MMDC1_MDOR
    0x021b0040, 0x00000027, //  MMDC1_MDASP
    0x021b0000, 0x831a0000, //  MMDC1_MDCTL
    0x021b001c, 0x04088032, //  MMDC1_MDSCR
    0x021b001c, 0x00008033, //  MMDC1_MDSCR
    0x021b001c, 0x00048031, //  MMDC1_MDSCR
    0x021b001c, 0x09408030, //  MMDC1_MDSCR
    0x021b001c, 0x04008040, //  MMDC1_MDSCR
    0x021b0800, 0xa1380003, //  MMDC1_MPZQHWCTRL
    0x021b4800, 0xa1380003, //  MMDC2_MPZQHWCTRL
    0x021b0020, 0x00005800, //  MMDC1_MDREF
    0x021b0818, 0x00011117, //  MMDC1_MPODTCTRL
    0x021b4818, 0x00011117, //  MMDC2_MPODTCTRL
    0x021b083c, 0x4301030d, //  MMDC1_MPDGCTRL0
    0x021b0840, 0x03020277, //  MMDC1_MPDGCTRL1
    0x021b483c, 0x4300030a, //  MMDC2_MPDGCTRL0
    0x021b4840, 0x02780248, //  MMDC2_MPDGCTRL1
    0x021b0848, 0x4536393b, //  MMDC1_MPRDDLCTL
    0x021b4848, 0x36353441, //  MMDC2_MPRDDLCTL
    0x021b0850, 0x41414743, //  MMDC1_MPWRDLCTL
    0x021b4850, 0x462f453f, //  MMDC2_MPWRDLCTL
    0x021b080c, 0x001f001f, //  MMDC1_MPWLDECTRL0
    0x021b0810, 0x001f001f, //  MMDC1_MPWLDECTRL1
    0x021b480c, 0x001f001f, //  MMDC2_MPWLDECTRL0
    0x021b4810, 0x001f001f, //  MMDC2_MPWLDECTRL1
    0x021b08b8, 0x00000800, //  MMDC1_MPMUR0
    0x021b48b8, 0x00000800, //  MMDC2_MPMUR0
    0x021b0004, 0x00025576, //  MMDC1_MDPDC
    0x021b0404, 0x00011006, //  MMDC1_MAPSR
    0x021b001c, 0x00000000  //  MMDC1_MDSCR
};

static UINT32 const sgClockCfg[] =
{
    0x020c4068, 0x00c03f3f, //  CCM_CCGR0
    0x020c406c, 0x0030fc03, //  CCM_CCGR1
    0x020c4070, 0x0fffc000, //  CCM_CCGR2
    0x020c4074, 0x3ff03000, //  CCM_CCGR3   +6 (LDB IPU)
    0x020c4078, 0x00fff300, //  CCM_CCGR4
    0x020c407c, 0x0f0000c3, //  CCM_CCGR5
    0x020c4080, 0x000003ff  //  CCM_CCGR6
};

static UINT32 const sgModeCfg[] =
{
    0x020e0010, 0xf00000ff, //  GPR4
    0x020e0018, 0x007f007f, //  GPR6
    0x020e001c, 0x007f007f  //  GPR7
};

#else

static UINT32 const sgDramMuxInit[] =
{
    0x020e05a8, 0x00000030, //  DRAM_SDQS0_P
    0x020e05b0, 0x00000030, //  DRAM_SDQS1_P
    0x020e0524, 0x00000030, //  DRAM_SDQS2_P
    0x020e051c, 0x00000030, //  DRAM_SDQS3_P
    0x020e0518, 0x00000030, //  DRAM_SDQS4_P
    0x020e050c, 0x00000030, //  DRAM_SDQS5_P
    0x020e05b8, 0x00000030, //  DRAM_SDQS6_P
    0x020e05c0, 0x00000030, //  DRAM_SDQS7_P
    0x020e0784, 0x00000030, //  GRP_B0DS
    0x020e0788, 0x00000030, //  GRP_B1DS
    0x020e0794, 0x00000030, //  GRP_B2DS
    0x020e079c, 0x00000030, //  GRP_B3DS
    0x020e07a0, 0x00000030, //  GRP_B4DS
    0x020e07a4, 0x00000030, //  GRP_B5DS
    0x020e07a8, 0x00000030, //  GRP_B6DS
    0x020e0748, 0x00000030, //  GRP_B7DS
    0x020e074c, 0x00000030, //  GRP_ADDDS
    0x020e078c, 0x00000030, //  GRP_CTLDS
    0x020e05ac, 0x00020030, //  DRAM_DQM0
    0x020e05b4, 0x00020030, //  DRAM_DQM1
    0x020e0528, 0x00020030, //  DRAM_DQM2
    0x020e0520, 0x00020030, //  DRAM_DQM3
    0x020e0514, 0x00020030, //  DRAM_DQM4
    0x020e0510, 0x00020030, //  DRAM_DQM5
    0x020e05bc, 0x00020030, //  DRAM_DQM6
    0x020e05c4, 0x00020030, //  DRAM_DQM7
    0x020e056c, 0x00020030, //  DRAM_CAS_B
    0x020e0578, 0x00020030, //  DRAM_RAS_B
    0x020e0588, 0x00020030, //  DRAM_SDCLK0_P
    0x020e0594, 0x00020030, //  DRAM_SDCLK1_P
    0x020e057c, 0x00020030, //  DRAM_RESET
    0x020e0590, 0x00003000, //  DRAM_SDCKE0
    0x020e0598, 0x00003000, //  DRAM_SDCKE1
    0x020e059c, 0x00003030, //  DRAM_ODT0
    0x020e05a0, 0x00003030, //  DRAM_ODT1
    0x020e0750, 0x00020000, //  GRP_DDRMODE_CTL
    0x020e0774, 0x00020000, //  GRP_DDRMODE
    0x020e0758, 0x00000000, //  GRP_DDRPKE
    0x020e058c, 0x00000000, //  DRAM_SDBA2
    0x020e0798, 0x000c0000  //  GRP_DDR_TYPE
};

static UINT32 const sgDramCfgInit[] = 
{
    0x021b081c, 0x33333333, //  MMDC1_MPRDDQBY0DL
    0x021b0820, 0x33333333, //  MMDC1_MPRDDQBY1DL
    0x021b0824, 0x33333333, //  MMDC1_MPRDDQBY2DL
    0x021b0828, 0x33333333, //  MMDC1_MPRDDQBY3DL
    0x021b481c, 0x33333333, //  MMDC2_MPRDDQBY0DL
    0x021b4820, 0x33333333, //  MMDC2_MPRDDQBY1DL
    0x021b4824, 0x33333333, //  MMDC2_MPRDDQBY2DL
    0x021b4828, 0x33333333, //  MMDC2_MPRDDQBY3DL
    0x021b0004, 0x00020036, //  MMDC1_MDPDC
    0x021b0008, 0x09444040, //  MMDC1_MDOTC
    0x021b000c, 0x54597955, //  MMDC1_MDCFG0
    0x021b0010, 0xff328f64, //  MMDC1_MDCFG1
    0x021b0014, 0x01ff00db, //  MMDC1_MDCFG2
    0x021b0018, 0x00001740, //  MMDC1_MDMISC
    0x021b001c, 0x00008000, //  MMDC1_MDSCR
    0x021b002c, 0x000026d2, //  MMDC1_MDRWD
    0x021b0030, 0x00591023, //  MMDC1_MDOR
    0x021b0040, 0x00000027, //  MMDC1_MDASP
    0x021b0000, 0x831a0000, //  MMDC1_MDCTL
    0x021b001c, 0x04088032, //  MMDC1_MDSCR
    0x021b001c, 0x00008033, //  MMDC1_MDSCR
    0x021b001c, 0x00048031, //  MMDC1_MDSCR
    0x021b001c, 0x09408030, //  MMDC1_MDSCR
    0x021b001c, 0x04008040, //  MMDC1_MDSCR
    0x021b0800, 0xa1380003, //  MMDC1_MPZQHWCTRL
    0x021b4800, 0xa1380003, //  MMDC2_MPZQHWCTRL
    0x021b0020, 0x00005800, //  MMDC1_MDREF
    0x021b0818, 0x00011117, //  MMDC1_MPODTCTRL
    0x021b4818, 0x00011117, //  MMDC2_MPODTCTRL
    0x021b083c, 0x43510360, //  MMDC1_MPDGCTRL0
    0x021b0840, 0x0342033f, //  MMDC1_MPDGCTRL1
    0x021b483c, 0x033f033f, //  MMDC2_MPDGCTRL0
    0x021b4840, 0x03290266, //  MMDC2_MPDGCTRL1
    0x021b0848, 0x4b3e4141, //  MMDC1_MPRDDLCTL
    0x021b4848, 0x47413b4a, //  MMDC2_MPRDDLCTL
    0x021b0850, 0x42404843, //  MMDC1_MPWRDLCTL
    0x021b4850, 0x4c3f4c45, //  MMDC2_MPWRDLCTL
    0x021b080c, 0x00350035, //  MMDC1_MPWLDECTRL0
    0x021b0810, 0x001f001f, //  MMDC1_MPWLDECTRL1
    0x021b480c, 0x00010001, //  MMDC2_MPWLDECTRL0
    0x021b4810, 0x00010001, //  MMDC2_MPWLDECTRL1
    0x021b08b8, 0x00000800, //  MMDC1_MPMUR0
    0x021b48b8, 0x00000800, //  MMDC2_MPMUR0
    0x021b0004, 0x00025576, //  MMDC1_MDPDC
    0x021b0404, 0x00011006, //  MMDC1_MAPSR
    0x021b001c, 0x00000000  //  MMDC1_MDSCR
};

static UINT32 const sgClockCfg[] =
{
    0x020c4068, 0x00c03f3f, //  CCM_CCGR0
    0x020c406c, 0x0030fc03, //  CCM_CCGR1
    0x020c4070, 0x0fffc000, //  CCM_CCGR2
    0x020c4074, 0x3ff00000, //  CCM_CCGR3
    0x020c4078, 0x00fff300, //  CCM_CCGR4
    0x020c407c, 0x0f0000c3, //  CCM_CCGR5
    0x020c4080, 0x000003ff  //  CCM_CCGR6
};

static UINT32 const sgModeCfg[] =
{
    0x020e0010, 0xf00000ff, //  GPR4
    0x020e0018, 0x007f007f, //  GPR6
    0x020e001c, 0x007f007f  //  GPR7
};

#endif

static ARM_CORE_INFO sgCoreInfoPpiData[] =
{
  {
    0x0, 0x0,
    (EFI_PHYSICAL_ADDRESS)UDOO_PHYSADDR_PERCPU_PAGES,
    (EFI_PHYSICAL_ADDRESS)UDOO_PHYSADDR_PERCPU_PAGES,
    (EFI_PHYSICAL_ADDRESS)UDOO_PHYSADDR_PERCPU_PAGES,
    (UINT64)0
  },
  {
    0x0, 0x1,
    (EFI_PHYSICAL_ADDRESS)(UDOO_PHYSADDR_PERCPU_PAGES + 0x1000),
    (EFI_PHYSICAL_ADDRESS)(UDOO_PHYSADDR_PERCPU_PAGES + 0x1000),
    (EFI_PHYSICAL_ADDRESS)(UDOO_PHYSADDR_PERCPU_PAGES + 0x1000),
    (UINT64)0
  },
  {
    0x0, 0x2,
    (EFI_PHYSICAL_ADDRESS)(UDOO_PHYSADDR_PERCPU_PAGES + 0x2000),
    (EFI_PHYSICAL_ADDRESS)(UDOO_PHYSADDR_PERCPU_PAGES + 0x2000),
    (EFI_PHYSICAL_ADDRESS)(UDOO_PHYSADDR_PERCPU_PAGES + 0x2000),
    (UINT64)0
  },
  {
    0x0, 0x3,
    (EFI_PHYSICAL_ADDRESS)(UDOO_PHYSADDR_PERCPU_PAGES + 0x3000),
    (EFI_PHYSICAL_ADDRESS)(UDOO_PHYSADDR_PERCPU_PAGES + 0x3000),
    (EFI_PHYSICAL_ADDRESS)(UDOO_PHYSADDR_PERCPU_PAGES + 0x3000),
    (UINT64)0
  }
};
static EFI_STATUS sGetCoreInfo (
  OUT UINTN *           CoreCount,
  OUT ARM_CORE_INFO **  ArmCoreTable
  )
{
    *CoreCount    = 4;
    *ArmCoreTable = sgCoreInfoPpiData;
    return EFI_SUCCESS;
}
static EFI_GUID             sgArmMpCoreInfoPpiGuid = ARM_MP_CORE_INFO_PPI_GUID;
static ARM_MP_CORE_INFO_PPI sgCoreInfoPpi = { sGetCoreInfo };

static EFI_SEC_PEI_HAND_OFF sgSecCoreData;

VOID
SecSwitchStack(
    INTN    StackDelta
);

EFI_STATUS
EFIAPI
TemporaryRamSupport(
    IN CONST EFI_PEI_SERVICES   **PeiServices,
    IN EFI_PHYSICAL_ADDRESS     TempMemoryBase,
    IN EFI_PHYSICAL_ADDRESS     NewStackAddr,
    IN UINTN                    TempMemorySize
)
{
    //
    // copy the heap, which is placed just after the new stack.  the top of the new stack
    // is right where the end of the copied temp stack is
    //
    CopyMem((VOID *)(UINTN)(NewStackAddr + sgSecCoreData.StackSize), (VOID *)(UINTN)TempMemoryBase, TempMemorySize);

    //
    // copy the stack
    //
    CopyMem((VOID *)(UINTN)NewStackAddr, (VOID *)sgSecCoreData.StackBase, sgSecCoreData.StackSize);

    //
    // switch to the new stack
    //
    SecSwitchStack((UINTN)NewStackAddr - (UINTN)sgSecCoreData.StackBase);

    return EFI_SUCCESS;
}
static EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI const sgTempRamSupportPpi = { TemporaryRamSupport };

static EFI_PEI_PPI_DESCRIPTOR const sgSecPpiList[] = 
{
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiTemporaryRamSupportPpiGuid,
    (VOID *) &sgTempRamSupportPpi
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &sgArmMpCoreInfoPpiGuid,
    &sgCoreInfoPpi
  }
};

#define REG_WRITE_DELAY_US  10

static
void
BoardInit(
    void
)
{
    UINT32 regLeft;
    UINT32 const *pReg;
    UINT32 gpt;

    // set default functional clocks
    regLeft = (sizeof(sgClockCfg) / sizeof(UINT32)) / 2;
    pReg = sgClockCfg;
    do {
        MmioWrite32(pReg[0], pReg[1]);
        pReg += 2;
        gpt = MmioRead32(IMX6_PHYSADDR_GPT_CNT) + REG_WRITE_DELAY_US;
        while (MmioRead32(IMX6_PHYSADDR_GPT_CNT) < gpt);
    } while (--regLeft);

    // set pad mux
    regLeft = (sizeof(sgDramMuxInit) / sizeof(UINT32)) / 2;
    pReg = sgDramMuxInit;
    do {
        MmioWrite32(pReg[0], pReg[1]);
        pReg += 2;
        gpt = MmioRead32(IMX6_PHYSADDR_GPT_CNT) + REG_WRITE_DELAY_US;
        while (MmioRead32(IMX6_PHYSADDR_GPT_CNT) < gpt);
    } while (--regLeft);

    // run config and init sequence for dram
    regLeft = (sizeof(sgDramCfgInit) / sizeof(UINT32)) / 2;
    pReg = sgDramCfgInit;
    do {
        MmioWrite32(pReg[0], pReg[1]);
        pReg += 2;
        gpt = MmioRead32(IMX6_PHYSADDR_GPT_CNT) + REG_WRITE_DELAY_US;
        while (MmioRead32(IMX6_PHYSADDR_GPT_CNT) < gpt);
    } while (--regLeft);

    // set misc operational modes
    regLeft = (sizeof(sgModeCfg) / sizeof(UINT32)) / 2;
    pReg = sgModeCfg;
    do {
        MmioWrite32(pReg[0], pReg[1]);
        pReg += 2;
        gpt = MmioRead32(IMX6_PHYSADDR_GPT_CNT) + REG_WRITE_DELAY_US;
        while (MmioRead32(IMX6_PHYSADDR_GPT_CNT) < gpt);
    } while (--regLeft);

    gpt = MmioRead32(IMX6_PHYSADDR_GPT_CNT) + 1000;
    while (MmioRead32(IMX6_PHYSADDR_GPT_CNT) < gpt);
}

static EFI_GUID sgCompressedPeiFileGuid = 
{ 0x9E21FD93, 0x9C72, 0x4C15, { 0x8C, 0x4B, 0xE7, 0x7F, 0x1D, 0xB2, 0xD7, 0x92 } };

STATIC
UINT8
CalculateHeaderChecksum(
    IN EFI_FFS_FILE_HEADER  *FileHeader
)
{
    UINT8   *Ptr;
    UINTN   Index;
    UINT8   Sum;

    Sum = 0;
    Ptr = (UINT8 *)FileHeader;

    for (Index = 0; Index < sizeof(EFI_FFS_FILE_HEADER) - 3; Index += 4) {
        Sum = (UINT8)(Sum + Ptr[Index]);
        Sum = (UINT8)(Sum + Ptr[Index + 1]);
        Sum = (UINT8)(Sum + Ptr[Index + 2]);
        Sum = (UINT8)(Sum + Ptr[Index + 3]);
    }

    for (; Index < sizeof(EFI_FFS_FILE_HEADER); Index++) {
        Sum = (UINT8)(Sum + Ptr[Index]);
    }

    //
    // State field (since this indicates the different state of file).
    //
    Sum = (UINT8)(Sum - FileHeader->State);
    //
    // Checksum field of the file is not part of the header checksum.
    //
    Sum = (UINT8)(Sum - FileHeader->IntegrityCheck.Checksum.File);

    return Sum;
}

#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  (ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1))

static
UINT8
GetFileState(
    IN BOOLEAN              ErasePolarity,
    IN EFI_FFS_FILE_HEADER  *FfsHeader
)
{
    UINT8 FileState;
    UINT8 HighestBit;

    FileState = FfsHeader->State;

    if (ErasePolarity) {
        FileState = (UINT8)~FileState;
    }

    HighestBit = 0x80;
    while (HighestBit != 0 && (HighestBit & FileState) == 0) {
        HighestBit >>= 1;
    }

    return HighestBit;
}

static
EFI_FFS_FILE_HEADER *
FindCompressedPeiFv(
    void
)
{
    EFI_FIRMWARE_VOLUME_HEADER *        FwVolHeader;
    EFI_FFS_FILE_HEADER *               FfsFileHeader;
    EFI_FIRMWARE_VOLUME_EXT_HEADER *    FwVolExHeaderInfo;
    UINT32                              FileLength;
    UINT32                              FileOccupiedSize;
    UINT32                              FileOffset;
    UINT64                              FvLength;
    UINT8                               ErasePolarity;
    UINT8                               FileState;

    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)FixedPcdGet32(PcdSecureFvBaseAddress);
    FvLength = FwVolHeader->FvLength;
    if (FwVolHeader->Attributes & EFI_FVB2_ERASE_POLARITY) {
        ErasePolarity = 1;
    }
    else {
        ErasePolarity = 0;
    }
    FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FwVolHeader + FwVolHeader->HeaderLength);
    if (FwVolHeader->ExtHeaderOffset != 0) {
        FwVolExHeaderInfo = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)(((UINT8 *)FwVolHeader) + FwVolHeader->ExtHeaderOffset);
        FfsFileHeader = (EFI_FFS_FILE_HEADER *)(((UINT8 *)FwVolExHeaderInfo) + FwVolExHeaderInfo->ExtHeaderSize);
    }
    FfsFileHeader = ALIGN_POINTER(FfsFileHeader, 8);

    FileOffset = (UINT32)((UINT8 *)FfsFileHeader - (UINT8 *)FwVolHeader);

    while (FileOffset < (FvLength - sizeof(EFI_FFS_FILE_HEADER))) {
        //
        // Get FileState which is the highest bit of the State
        //
        FileState = GetFileState(ErasePolarity, FfsFileHeader);

        switch (FileState) {

        case EFI_FILE_HEADER_INVALID:
            FileOffset += sizeof(EFI_FFS_FILE_HEADER);
            FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + sizeof(EFI_FFS_FILE_HEADER));
            break;

        case EFI_FILE_DATA_VALID:
        case EFI_FILE_MARKED_FOR_UPDATE:
            if (CalculateHeaderChecksum(FfsFileHeader) != 0) {
                return NULL;
            }

            FileLength = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
            FileOccupiedSize = GET_OCCUPIED_SIZE(FileLength, 8);
            
            if (FfsFileHeader->Type == EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE)
            {
                if (CompareGuid(&FfsFileHeader->Name, &sgCompressedPeiFileGuid)) {
                    return FfsFileHeader;
                }
            }

            FileOffset += FileOccupiedSize;
            FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
            break;

        case EFI_FILE_DELETED:
            FileLength = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
            FileOccupiedSize = GET_OCCUPIED_SIZE(FileLength, 8);
            FileOffset += FileOccupiedSize;
            FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
            break;

        default:
            return NULL;
        }
    }

    return NULL;
}

void
PrimaryCoreInNonSecure(
    void
)
{
    //
    // this happens last after all secondary cores are ready
    // 
    EFI_FFS_FILE_HEADER *           pCompressedPeiFv;
    UINT32                          FileSize;
    EFI_COMMON_SECTION_HEADER *     Section;
    UINTN                           SectionLength;
    EFI_GUID *                      SectionDefinitionGuid;
    UINT8 *                         pCompressedData;
    UINTN                           compressedDataBytes;
    UINT32                          destSize;
    UINT32                          scratchSize;
    EFI_STATUS                      efiStatus;
    EFI_FIRMWARE_VOLUME_HEADER *    pVolHeader;
    EFI_PEI_CORE_ENTRY_POINT        PeiCoreEntryPoint;

    // Data Cache should be off but we set it disable and invalidate it anyway
    ArmDisableDataCache();
    ArmInvalidateDataCache();

    // Instruction cache should be off but we invalidate it anyway then enable it
    ArmInvalidateInstructionCache();
    ArmEnableInstructionCache();
    // Enable program flow prediction, if supported.
    ArmEnableBranchPrediction();

    // Enable SWP instructions in NonSecure state
    ArmEnableSWPInstruction();

    // Enable Full Access to CoProcessors
    ArmWriteCpacr(CPACR_CP_FULL_ACCESS);

    // Enable the GIC Distributor
    ArmGicEnableDistributor(PcdGet64(PcdGicDistributorBase));

    BoardInit();

    //
    // find the compressed PEI FV
    //
    pCompressedPeiFv = FindCompressedPeiFv();
    while (NULL == pCompressedPeiFv);

    Section = (EFI_COMMON_SECTION_HEADER *)(pCompressedPeiFv + 1);
    FileSize = *(UINT32 *)(pCompressedPeiFv->Size) & 0x00FFFFFF;
    FileSize -= sizeof(EFI_FFS_FILE_HEADER);

    pCompressedData = NULL;
    compressedDataBytes = 0;
    efiStatus = EFI_NOT_FOUND;
    do {
        if (IS_SECTION2(Section)) {
            SectionLength = SECTION2_SIZE(Section);
        }
        else {
            SectionLength = SECTION_SIZE(Section);
        }
        if (EFI_SECTION_GUID_DEFINED == Section->Type)
        {
            if (IS_SECTION2(Section)) {
                SectionDefinitionGuid = &(((EFI_GUID_DEFINED_SECTION2 *)Section)->SectionDefinitionGuid);
            }
            else {
                SectionDefinitionGuid = &(((EFI_GUID_DEFINED_SECTION *)Section)->SectionDefinitionGuid);
            }
            if (CompareGuid(&gLzmaCustomDecompressGuid, SectionDefinitionGuid))
            {
                // this is the compressed firmware volume
                if (IS_SECTION2(Section)) 
                {
                    pCompressedData = ((UINT8 *)Section) + ((EFI_GUID_DEFINED_SECTION2 *)Section)->DataOffset;
                    compressedDataBytes = SectionLength - ((EFI_GUID_DEFINED_SECTION2 *)Section)->DataOffset;
                }
                else
                {
                    pCompressedData = ((UINT8 *)Section) + ((EFI_GUID_DEFINED_SECTION *)Section)->DataOffset;
                    compressedDataBytes = SectionLength - ((EFI_GUID_DEFINED_SECTION *)Section)->DataOffset;
                }
                destSize = 0;
                scratchSize = 0;
                efiStatus = LzmaUefiDecompressGetInfo(pCompressedData, compressedDataBytes, &destSize, &scratchSize);
//                ASSERT_EFI_ERROR(efiStatus);
                DebugPrint(0xFFFFFFFF, "Decompressing PEI...\n");
                while (EFI_ERROR(efiStatus));
                efiStatus = LzmaUefiDecompress(
                    pCompressedData,
                    compressedDataBytes,
                    (VOID *)UDOO_PHYSADDR_PEIFV_FILE_BASE,
                    (VOID *)((UDOO_PHYSADDR_PEIFV_FILE_BASE + destSize + 0xFFF) & ~0xFFF)
                );

                break;
            }
        }
        //
        // SectionLength is adjusted it is 4 byte aligned.
        // Go to the next section
        //
        SectionLength = GET_OCCUPIED_SIZE(SectionLength, 4);
//        ASSERT(SectionLength != 0);
        while (0 == SectionLength);
        if (SectionLength >= FileSize)
            break;
        FileSize -= SectionLength;
        Section = (EFI_COMMON_SECTION_HEADER *)((UINT8 *)Section + SectionLength);
    } while (0 < FileSize);

    if (EFI_ERROR(efiStatus))
    {
        DEBUG((EFI_D_ERROR, "*** PEI decompression error %r\n", efiStatus));
        while (1);
    }

    pVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UDOO_PHYSADDR_PEIFV_FILE_BASE + 0x1000);

    //
    // Enter PEI.  Entrypoint is second dword at pei volume address
    //
    PeiCoreEntryPoint = (EFI_PEI_CORE_ENTRY_POINT)*(((UINT32 *)pVolHeader) + 1);

    sgSecCoreData.DataSize = sizeof(EFI_SEC_PEI_HAND_OFF);

    sgSecCoreData.BootFirmwareVolumeBase = (VOID *)pVolHeader;
    sgSecCoreData.BootFirmwareVolumeSize = (UINTN)pVolHeader->FvLength;
    sgSecCoreData.TemporaryRamBase = (VOID *)UDOO_PHYSADDR_TEMP_RAM_AREA;
    sgSecCoreData.TemporaryRamSize = UDOO_PHYSSIZE_TEMP_RAM_AREA;
    sgSecCoreData.PeiTemporaryRamBase = sgSecCoreData.TemporaryRamBase;
    sgSecCoreData.PeiTemporaryRamSize = sgSecCoreData.TemporaryRamSize;
    sgSecCoreData.StackBase = (VOID *)UDOO_PHYSADDR_PERCPU_PAGES;
    sgSecCoreData.StackSize = 0x1000 - UDOO_PHYSSIZE_PERCPU_SEC_STACK;

    // Jump to PEI core entry point
//    DebugPrint(0xFFFFFFFF, "Entering PEI @ 0x%08X...\n", PeiCoreEntryPoint);
    ArmInvalidateInstructionCache();
    PeiCoreEntryPoint(&sgSecCoreData, sgSecPpiList);

    while (1);
}

