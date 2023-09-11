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

void PrimaryCoreInNonSecure(void);

#if SLOW_SECONDARY_CORE_STARTUP
#define SECONDARY_START_US_DELAY    1000000 // 1 second
#else
#define SECONDARY_START_US_DELAY    1000     // 1000 us (1ms)
#endif

extern void _ModuleEntryPoint(void);

extern SPIN_LOCK gTzSpinLock;

static void sSecInitArch(void)
{
    UINT32 regVal;

    //
    // AIPS permissions
    //
    MmioWrite32(IMX6_PHYSADDR_AIPS1_MPR,    IMX6_AIPS_MPR_ALL_TRUST_UNBUFFERED);
    MmioWrite32(IMX6_PHYSADDR_AIPS1_OPACR0, IMX6_AIPS_OPACR_OPAC_TRUST_UNBUFFERED);
    MmioWrite32(IMX6_PHYSADDR_AIPS1_OPACR1, IMX6_AIPS_OPACR_OPAC_TRUST_UNBUFFERED);
    MmioWrite32(IMX6_PHYSADDR_AIPS1_OPACR2, IMX6_AIPS_OPACR_OPAC_TRUST_UNBUFFERED);
    MmioWrite32(IMX6_PHYSADDR_AIPS1_OPACR3, IMX6_AIPS_OPACR_OPAC_TRUST_UNBUFFERED);
    MmioWrite32(IMX6_PHYSADDR_AIPS1_OPACR4, IMX6_AIPS_OPACR_OPAC_TRUST_UNBUFFERED);

    MmioWrite32(IMX6_PHYSADDR_AIPS2_MPR,    IMX6_AIPS_MPR_ALL_TRUST_UNBUFFERED);
    MmioWrite32(IMX6_PHYSADDR_AIPS2_OPACR0, IMX6_AIPS_OPACR_OPAC_TRUST_UNBUFFERED);
    MmioWrite32(IMX6_PHYSADDR_AIPS2_OPACR1, IMX6_AIPS_OPACR_OPAC_TRUST_UNBUFFERED);
    MmioWrite32(IMX6_PHYSADDR_AIPS2_OPACR2, IMX6_AIPS_OPACR_OPAC_TRUST_UNBUFFERED);
    MmioWrite32(IMX6_PHYSADDR_AIPS2_OPACR3, IMX6_AIPS_OPACR_OPAC_TRUST_UNBUFFERED);
    MmioWrite32(IMX6_PHYSADDR_AIPS2_OPACR4, IMX6_AIPS_OPACR_OPAC_TRUST_UNBUFFERED);

    //
    // Clear MMDC channel mask
    //
    MmioWrite32(IMX6_PHYSADDR_CCM_CCDR, 0);

    //
    // Ensure bandgap has stabilized and is configured for best noise performance
    //
    do
    {
        regVal = MmioRead32(IMX6_PHYSADDR_ANALOG_PMU_MISC0);
    } while ((regVal & IMX6_ANALOG_MISC0_REFTOP_VBGUP) == 0);
    if (0 == (regVal & IMX6_ANALOG_MISC0_REFTOP_SELFBIASOFF))
    {
        regVal |= IMX6_ANALOG_MISC0_REFTOP_SELFBIASOFF;
        MmioWrite32(IMX6_PHYSADDR_ANALOG_PMU_MISC0, regVal);
    }

    //
    // disable watchdogs auto-power-down.  these are 16-bit registers
    //
    MmioWrite16(IMX6_PHYSADDR_WDOG1_WMCR, 0);
    MmioWrite16(IMX6_PHYSADDR_WDOG2_WMCR, 0);

    //
    // set vppdu power down and disable it
    //
    regVal = MmioRead32(IMX6_PHYSADDR_PGC_PU_GPU_CTRL);
    MmioWrite32(IMX6_PHYSADDR_PGC_PU_GPU_CTRL, regVal | IMX6_PGC_PU_GPU_CTRL_PCR);

    regVal = MmioRead32(IMX6_PHYSADDR_GPC_CNTR);
    MmioWrite32(IMX6_PHYSADDR_GPC_CNTR, regVal | IMX6_GPC_CNTR_GPU_VPU_PDN_REQ);
    while (MmioRead32(IMX6_PHYSADDR_GPC_CNTR) & IMX6_GPC_CNTR_GPU_VPU_PDN_REQ);

    //
    // reset APBH_DMA
    //
    // First make sure the clock to the module is on
    regVal = MmioRead32(IMX6_PHYSADDR_CCM_CCGR0);
    regVal |= (IMX6_RUN_AND_WAIT << IMX6_SHL_CCM_CCGR0_APBHDMA_HCLK);
    MmioWrite32(IMX6_PHYSADDR_CCM_CCGR0, regVal);
    // Clear SFTRST and wait for it to be clear
    MmioWrite32(IMX6_PHYSADDR_APBH_CTRL0_CLR, IMX6_APBH_DMA_CTRL0_SFTRST);
    while (MmioRead32(IMX6_PHYSADDR_APBH_CTRL0) & IMX6_APBH_DMA_CTRL0_SFTRST);
    // Clear CLKGATE and then set SFTRST
    MmioWrite32(IMX6_PHYSADDR_APBH_CTRL0_CLR, IMX6_APBH_DMA_CTRL0_CLKGATE);
    MmioWrite32(IMX6_PHYSADDR_APBH_CTRL0_SET, IMX6_APBH_DMA_CTRL0_SFTRST);
    // wait for CLKGATE to be set
    while (!(MmioRead32(IMX6_PHYSADDR_APBH_CTRL0) & IMX6_APBH_DMA_CTRL0_CLKGATE));
    // Clear SFTRST and wait for it to be clear
    MmioWrite32(IMX6_PHYSADDR_APBH_CTRL0_CLR, IMX6_APBH_DMA_CTRL0_SFTRST);
    while (MmioRead32(IMX6_PHYSADDR_APBH_CTRL0) & IMX6_APBH_DMA_CTRL0_SFTRST);
    // Clear CLKGATE and wait for it to be clear
    MmioWrite32(IMX6_PHYSADDR_APBH_CTRL0_CLR, IMX6_APBH_DMA_CTRL0_CLKGATE);
    while (MmioRead32(IMX6_PHYSADDR_APBH_CTRL0) & IMX6_APBH_DMA_CTRL0_CLKGATE);

    //
    // configure system reset controller to disable warm reset
    //
    regVal = MmioRead32(IMX6_PHYSADDR_SRC_SCR);
    regVal &= ~IMX6_SRC_SCR_WARM_RESET_ENABLE;
    MmioWrite32(IMX6_PHYSADDR_SRC_SCR, regVal);

    //
    // set VDOA, VPU, IPU cacheable
    //
    MmioWrite32(IMX6_PHYSADDR_IOMUXC_GPR4, 0xF00000CF);

    //
    // IPU0 and IPU1 QoS
    //
    MmioWrite32(IMX6_PHYSADDR_IOMUXC_GPR6, 0x007F007F);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC_GPR7, 0x007F007F);

    //
    // allow anybody to access the SNVS clock
    //
    regVal = MmioRead32(IMX6_PHYSADDR_SNVS_HPCOMR);
    regVal |= IMX6_SNVS_HPCOMR_NPSWA_EN;
    MmioWrite32(IMX6_PHYSADDR_SNVS_HPCOMR, regVal);

    // turn off the clock
    regVal = MmioRead32(IMX6_PHYSADDR_SNVS_HPCR);
    regVal &= ~IMX6_SNVS_HPCR_RTC_EN;
    MmioWrite32(IMX6_PHYSADDR_SNVS_HPCR, regVal);

    // reset it to zero
    // normally if we actually had a battery backed
    // clock we would get the value from it here
    // but we don't have one so we have no idea
    // what time it is.
    MmioWrite32(IMX6_PHYSADDR_SNVS_HPRTCLR, 0);
    MmioWrite32(IMX6_PHYSADDR_SNVS_HPRTCMR, 0);

    // turn the clock back on
    regVal = MmioRead32(IMX6_PHYSADDR_SNVS_HPCR);
    regVal |= IMX6_SNVS_HPCR_RTC_EN;
    MmioWrite32(IMX6_PHYSADDR_SNVS_HPCR, regVal);
}

static void sInitGPT(void)
{
    //
    // Initialize GPT to run at 1Mhz, so each tick is 1 microsecond
    //
    MmioWrite32(IMX6_PHYSADDR_GPT_CR, 0);

    MmioWrite32(IMX6_PHYSADDR_GPT_IR, 0);
    MmioWrite32(IMX6_PHYSADDR_GPT_SR, 0);

    MmioWrite32(IMX6_PHYSADDR_GPT_OCR1, 0);
    MmioWrite32(IMX6_PHYSADDR_GPT_OCR2, 0);
    MmioWrite32(IMX6_PHYSADDR_GPT_OCR3, 0);

    MmioWrite32(IMX6_PHYSADDR_GPT_CR,
        IMX6_GPT_CR_CLKSRC_OSC |
        IMX6_GPT_CR_SWR
        );
    while (0 != (MmioRead32(IMX6_PHYSADDR_GPT_CR) & IMX6_GPT_CR_SWR));
    
    MmioWrite32(IMX6_PHYSADDR_GPT_SR, 0x3F);

    MmioWrite32(IMX6_PHYSADDR_GPT_PR, 23);

    MmioWrite32(IMX6_PHYSADDR_GPT_CR,
        IMX6_GPT_CR_CLKSRC_OSC |
        IMX6_GPT_CR_ENMOD |
        IMX6_GPT_CR_EN
        );
}

#if !WANDBOARD
static void sDisableWatchdogs(void)
{
    UINT32 regVal;
    UINT32 padConfig;

    padConfig =
        IMX6_MUX_PADCTL_PUS_100KOHM_PU |
        IMX6_MUX_PADCTL_PUE |
        IMX6_MUX_PADCTL_PKE |
        IMX6_MUX_PADCTL_SPEED_100MHZ |
        IMX6_MUX_PADCTL_DSE_40_OHM |
        IMX6_MUX_PADCTL_SRE_FAST;

    // WDT_EN is GPIO5-4 on EIM_A24
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_ADDR24, 0x05);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_ADDR24, padConfig);

    // WDT_TRG is GPIO3-19 on EIM_D19
    padConfig = 
        IMX6_MUX_PADCTL_SPEED_50MHZ |
        IMX6_MUX_PADCTL_DSE_HIZ |
        IMX6_MUX_PADCTL_SRE_SLOW;
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA19, 0x05);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA19, padConfig);

    // output 0 to WDT_TRG
    // output 1 to WDT_EN  (low enable, so set high)
    // set WDT_TRG as an input
    regVal = MmioRead32(IMX6_PHYSADDR_GPIO3_DR);
    regVal &= ~(1 << 19);
    MmioWrite32(IMX6_PHYSADDR_GPIO3_DR, regVal);
    regVal = MmioRead32(IMX6_PHYSADDR_GPIO3_GDIR);
    regVal |= (1 << 19);
    MmioWrite32(IMX6_PHYSADDR_GPIO3_GDIR, regVal);
    regVal = MmioRead32(IMX6_PHYSADDR_GPIO3_DR);
    regVal &= ~(1 << 19);
    MmioWrite32(IMX6_PHYSADDR_GPIO3_DR, regVal);

    regVal = MmioRead32(IMX6_PHYSADDR_GPIO5_DR);
    regVal |= (1 << 4);
    MmioWrite32(IMX6_PHYSADDR_GPIO5_DR, regVal);
    regVal = MmioRead32(IMX6_PHYSADDR_GPIO5_GDIR);
    regVal |= (1 << 4);
    MmioWrite32(IMX6_PHYSADDR_GPIO5_GDIR, regVal);
    regVal = MmioRead32(IMX6_PHYSADDR_GPIO5_DR);
    regVal |= (1 << 4);
    MmioWrite32(IMX6_PHYSADDR_GPIO5_DR, regVal);

    regVal = MmioRead32(IMX6_PHYSADDR_GPIO3_GDIR);
    regVal &= ~(1 << 19);
    MmioWrite32(IMX6_PHYSADDR_GPIO3_GDIR, regVal);
}
#endif

static void sInitUart(void)
{
    UINT32 regVal;

    //
    // UART input is PLL3_SW_CLK divided by CSCDR1.UART_CLK_PODF
    //
    // CCGR5[CG13] is uart clock gate
    // default frequency is 80Mhz input
    //
    //  PLL3_SW_CLK (480Mhz) -> 
    //      CCGR5[CG13] gate     -> 
    //          static divide by 6     -> =="PLL3_80M" 
    //              CSCDR1.UART_CLK_PODF     -> == "UART_CLK_ROOT"

    //
    // gate the clock
    //
    regVal = MmioRead32(IMX6_PHYSADDR_CCM_CCGR5);
    regVal &= ~((IMX6_RUN_AND_WAIT << IMX6_SHL_CCM_CCGR5_UART) | (IMX6_RUN_AND_WAIT << IMX6_SHL_CCM_CCGR5_UART_SERIAL));
    MmioWrite32(IMX6_PHYSADDR_CCM_CCGR5, regVal);

    //
    // change the divider, clear uart_clk_podf to get div by 1 which is 80Mhz clock through to module
    //
    regVal = MmioRead32(IMX6_PHYSADDR_CCM_CSCDR1);
    regVal &= ~IMX6_CCM_CSCDR1_UART_CLK_PODF_MASK;
    MmioWrite32(IMX6_PHYSADDR_CCM_CSCDR1, regVal);

    // 
    // re-enable the clock
    //
    regVal = MmioRead32(IMX6_PHYSADDR_CCM_CCGR5);
    regVal &= ~((IMX6_RUN_AND_WAIT << IMX6_SHL_CCM_CCGR5_UART) | (IMX6_RUN_AND_WAIT << IMX6_SHL_CCM_CCGR5_UART_SERIAL));
    regVal |= ((IMX6_RUN_AND_WAIT << IMX6_SHL_CCM_CCGR5_UART) | (IMX6_RUN_AND_WAIT << IMX6_SHL_CCM_CCGR5_UART_SERIAL));
    MmioWrite32(IMX6_PHYSADDR_CCM_CCGR5, regVal);

#if WANDBOARD
    // CSI0_DAT10 - UART1 TX
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA10,
        IMX6_MUX_PADCTL_HYS |
        IMX6_MUX_PADCTL_PUS_100KOHM_PU |
        IMX6_MUX_PADCTL_PUE |
        IMX6_MUX_PADCTL_PKE |
        IMX6_MUX_PADCTL_SPEED_100MHZ |
        IMX6_MUX_PADCTL_DSE_40_OHM |
        IMX6_MUX_PADCTL_SRE_FAST);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA10, 3);

    // CSI0_DAT11 - UART2 RX
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA11,
        IMX6_MUX_PADCTL_HYS |
        IMX6_MUX_PADCTL_PUS_100KOHM_PU |
        IMX6_MUX_PADCTL_PUE |
        IMX6_MUX_PADCTL_PKE |
        IMX6_MUX_PADCTL_SPEED_100MHZ |
        IMX6_MUX_PADCTL_DSE_40_OHM |
        IMX6_MUX_PADCTL_SRE_FAST);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_UART1_UART_RX_DATA_SELECT_INPUT, 1);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA10, 3);
#else
    // EIM_D26 - UART2 TX
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA26,
        IMX6_MUX_PADCTL_HYS |
        IMX6_MUX_PADCTL_PUS_100KOHM_PU |
        IMX6_MUX_PADCTL_PUE |
        IMX6_MUX_PADCTL_PKE |
        IMX6_MUX_PADCTL_SPEED_100MHZ |
        IMX6_MUX_PADCTL_DSE_40_OHM |
        IMX6_MUX_PADCTL_SRE_FAST);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA26, 4);

    // EIM_D27 - UART2 RX
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA27,
        IMX6_MUX_PADCTL_HYS |
        IMX6_MUX_PADCTL_PUS_100KOHM_PU |
        IMX6_MUX_PADCTL_PUE |
        IMX6_MUX_PADCTL_PKE |
        IMX6_MUX_PADCTL_SPEED_100MHZ |
        IMX6_MUX_PADCTL_DSE_40_OHM |
        IMX6_MUX_PADCTL_SRE_FAST);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_UART2_UART_RX_DATA_SELECT_INPUT, 1);
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA27, 4);
#endif
    // Set UART2 to 115200 8N1
    // 
    // UART_CLOCK_ROOT -> RFDIV(UCFR 2:0) -> ref_clk -> / (16 * (BMR/BIR)) -> Baud Rate
    //
    // 80Mhz -> /2 -> 40Mhz -> / (16 * 3125 / 144) -> 115200
    //
#if WANDBOARD
    IMX6_UART_SyncInitForDebug(IMX6_PHYSADDR_UART1, IMX6_UART_UFCR_RFDIV_BY2, 0x0F, 0x15B);
#else
    IMX6_UART_SyncInitForDebug(IMX6_PHYSADDR_UART2, IMX6_UART_UFCR_RFDIV_BY2, 0x0F, 0x15B);
#endif

    //
    // Any other optional inits in serial port library 
    //
    SerialPortInitialize();
}

UINT32
PrimaryTrustedMonitorInit(
    UINT32 StackPointer,
    UINT32 SVC_SPSR,
    UINT32 MON_SPSR,
    UINT32 MON_CPSR
)
{
#if SLOW_SECONDARY_CORE_STARTUP
    UINT32 ScuAddr;

    //
    // slow down secondary core startup to see this without having
    // all the cores stomp on each other's use of UART while initializing.
    // we can't use exclusives because the caches are off on the 
    // secondary cores
    DebugPrint(0xFFFFFFFF, "\nPrimary Trusted Monitor Init:\n");
    DebugPrint(0xFFFFFFFF, "Primary SCTLR        0x%08X\n", ArmReadSctlr());
    DebugPrint(0xFFFFFFFF, "Primary StackPointer 0x%08X\n", StackPointer);
    DebugPrint(0xFFFFFFFF, "Primary SVC_SPSR     0x%08X\n", SVC_SPSR);
    DebugPrint(0xFFFFFFFF, "Primary MON_SPSR     0x%08X\n", MON_SPSR);
    DebugPrint(0xFFFFFFFF, "Primary MON_CPSR     0x%08X\n", MON_CPSR);
    DebugPrint(0xFFFFFFFF, "Primary ACTLR  %08X\n", ArmReadAuxCr());
    DebugPrint(0xFFFFFFFF, "Primary SCR    %08X\n", ArmReadScr());
    DebugPrint(0xFFFFFFFF, "Primary NSACR  %08X\n", ArmReadNsacr());

    ScuAddr = ArmGetScuBaseAddress();

    DebugPrint(0xFFFFFFFF, "SCU.CONTROL %08X\n", MmioRead32(ScuAddr + A9_SCU_CONTROL_OFFSET));
    DebugPrint(0xFFFFFFFF, "SCU.CONFIG  %08X\n", MmioRead32(ScuAddr + A9_SCU_CONFIG_OFFSET));
    DebugPrint(0xFFFFFFFF, "SCU.PWRSTAT %08X\n", MmioRead32(ScuAddr + A9_SCU_POWERSTAT_OFFSET));
    DebugPrint(0xFFFFFFFF, "SCU.FILT_S  %08X\n", MmioRead32(ScuAddr + A9_SCU_FILT_START_OFFSET));
    DebugPrint(0xFFFFFFFF, "SCU.FILT_E  %08X\n", MmioRead32(ScuAddr + A9_SCU_FILT_END_OFFSET));
    DebugPrint(0xFFFFFFFF, "SCU.SACR    %08X\n", MmioRead32(ScuAddr + A9_SCU_SACR_OFFSET));
    DebugPrint(0xFFFFFFFF, "SCU.SSACR   %08X\n", MmioRead32(ScuAddr + A9_SCU_SSACR_OFFSET));
#endif

    //
    // Transfer all interrupts to cpu 0 to the Non-secure World
    //
    ArmGicSetupNonSecure(0, (INTN)PcdGet64(PcdGicDistributorBase), (INTN)PcdGet64(PcdGicInterruptInterfaceBase));

    //
    // Return where to go when you exit secure mode
    //
    return (UINT32)PrimaryCoreInNonSecure;
}

UINT32
UdooPrimaryCoreSecStart(
    VOID
)
{
    UINT32 Reg32;
    UINT32 BaseAddress;

    //
    // we are in SVC mode in secure state
    //
    sSecInitArch();

    //
    // init required peripherals
    //
    sInitGPT();
    sInitUart();
#if !WANDBOARD
    sDisableWatchdogs();
#endif
    
    DebugPrint(0xFFFFFFFF, "\r\nK2 IMX6 Secure Init\r\n");
    Reg32 = MmioRead32(IMX6_PHYSADDR_ANALOG_DIGPROG);
    DebugPrint(0xFFFFFFFF, "SoC Revision %02X.%02X.%02X\n",
        (Reg32 & IMX6_ANALOG_DIGPROG_MAJOR_UPPER_MASK) >> IMX6_ANALOG_DIGPROG_MAJOR_UPPER_SHL,
        (Reg32 & IMX6_ANALOG_DIGPROG_MAJOR_LOWER_MASK) >> IMX6_ANALOG_DIGPROG_MAJOR_LOWER_SHL,
        (Reg32 & IMX6_ANALOG_DIGPROG_MINOR_MASK) >> IMX6_ANALOG_DIGPROG_MINOR_SHL);
    DebugPrint(0xFFFFFFFF, "Built " __DATE__ " " __TIME__ "\r\n");

    //
    // init trustzone spinlock
    //
    InitializeSpinLock(&gTzSpinLock);

    //
    // confirm we are in secure state (sanity check)
    //
    Reg32 = ArmReadScr();
    ASSERT((Reg32 & 1) == 0);

    //
    // Get Boot Mode register
    //
    Reg32 = MmioRead32(IMX6_PHYSADDR_SRC_SBMR2);

    //
    // assert we are an open device
    //
    ASSERT(0 == (Reg32 & IMX6_SRC_SBMR2_BMOD_SEC_CONFIG_CLOSED));

    //
    // assert we booted from internal
    //
    ASSERT((Reg32 & IMX6_SRC_SBMR2_BMOD_MASK) == IMX6_SRC_SBMR2_BMOD_FROM_INTERNAL);

#if 0
    //
    // print reset and boot information 
    //
    Reg32 = MmioRead32(IMX6_PHYSADDR_SRC_SBMR1);
    DebugPrint(0xFFFFFFFF, "  BOOT FROM ");
    switch ((Reg32 & IMX6_SRC_SBMR1_BOOT_CFG1_MASK) >> 4)
    {
    case 0:
        DebugPrint(0xFFFFFFFF, "OneNAND\n");
        break;
    case 2:
        DebugPrint(0xFFFFFFFF, "SATA\n");
        break;
    case 3:
        DebugPrint(0xFFFFFFFF, "SERIAL ROM\n");
        break;
    case 4:
        DebugPrint(0xFFFFFFFF, "SD/eSD/SDXC NORMAL\n");
        break;
    case 5:
        DebugPrint(0xFFFFFFFF, "SD/eSD/SDXC FAST\n");
        break;
    case 6:
        DebugPrint(0xFFFFFFFF, "MMC/eMMC NORMAL\n");
        break;
    case 7:
        DebugPrint(0xFFFFFFFF, "MMC/eMMC FAST\n");
        break;
    default:
        DebugPrint(0xFFFFFFFF, "RAW NAND\n");
        break;
    }
#endif

    //
    // ensure TZASC is bypassed
    //
    Reg32 = MmioRead32(IMX6_PHYSADDR_IOMUXC_GPR9);
    ASSERT((Reg32 & 0x3) == 0);

    //
    // set CSU to enable all access at all security levels for all peripherals
    // this is critical or all peripherals in NS mode will be nonfunctional
    //
    for (Reg32 = 0;Reg32 < 40;Reg32++)
        MmioWrite32(IMX6_PHYSADDR_CSU + (4 * Reg32), 0x00FF00FF);

#if 0
    //
    // print reset method
    //
    Reg32 = MmioRead32(IMX6_PHYSADDR_SRC_SRSR);
    if (Reg32 & IMX6_SRSR_WARM_BOOT)
    {
        DebugPrint(0xFFFFFFFF, "  WARM RESET\n");
        Reg32 &= ~IMX6_SRSR_WARM_BOOT;
        MmioWrite32(IMX6_PHYSADDR_SRC_SRSR, Reg32);
    }
    else
    {
        if (Reg32 & IMX6_SRSR_JTAG_SW_RST)
            DebugPrint(0xFFFFFFFF, "  JTAG SW RESET\n");
        if (Reg32 & IMX6_SRSR_JTAG_RST_B)
            DebugPrint(0xFFFFFFFF, "  JTAG HI-Z RESET\n");
        if (Reg32 & IMX6_SRSR_WDOG_RST_B)
            DebugPrint(0xFFFFFFFF, "  WATCHDOG RESET\n");
        if (Reg32 & IMX6_SRSR_IPP_USER_RESET_B)
            DebugPrint(0xFFFFFFFF, "  USER COLD RESET\n");
        if (Reg32 & IMX6_SRSR_IPP_RESET_B)
            DebugPrint(0xFFFFFFFF, "  POWER ON COLD RESET\n");
    }
#endif

    //
    // Enable SWP instructions in secure state
    //
    ArmEnableSWPInstruction();

    //
    // Get Snoop Control Unit base address
    // Allow NS access to SCU register
    // Allow NS access to Private Peripherals
    //
    BaseAddress = ArmGetScuBaseAddress();
    MmioOr32(BaseAddress + A9_SCU_SACR_OFFSET, 0xf);
    MmioOr32(BaseAddress + A9_SCU_SSACR_OFFSET, 0xfff);

    //
    // Conditionally Enable Floating Point Coprocessor (usually enabled)
    //
    if (FixedPcdGet32(PcdVFPEnabled))
        ArmEnableVFP();

    //
    // Enable interrupt distributor and this cpu's interface
    //
    ArmGicEnableDistributor((INTN)PcdGet64(PcdGicDistributorBase));
    ArmGicEnableInterruptInterface((INTN)PcdGet64(PcdGicInterruptInterfaceBase));

    //
    // Enable Full Access to CoProcessors
    //
    ArmWriteCpacr(CPACR_CP_FULL_ACCESS);

    // 
    // Now start other cores.
    // the secondary cores 
    //   DO NOT LEAVE PHYSICAL MODE and 
    //   DO NOT HAVE CACHES ENABLED and therefore
    //   CANNOT USE EXCLUSIVE LOAD/STORES
    //
    *((UINT32*)(UDOO_PHYSADDR_PERCPU_PAGES + 0x1000)) = 0xFFFFFFFF;
    *((UINT32*)(UDOO_PHYSADDR_PERCPU_PAGES + 0x2000)) = 0xFFFFFFFF;
    *((UINT32*)(UDOO_PHYSADDR_PERCPU_PAGES + 0x3000)) = 0xFFFFFFFF;
    BaseAddress = (UINT32)PcdGet64(PcdSecureFvBaseAddress);
    MmioWrite32(IMX6_PHYSADDR_SRC_GPR1, BaseAddress);
    MmioWrite32(IMX6_PHYSADDR_SRC_GPR3, BaseAddress);
    MmioWrite32(IMX6_PHYSADDR_SRC_GPR5, BaseAddress);
    MmioWrite32(IMX6_PHYSADDR_SRC_GPR7, BaseAddress);
    Reg32 = MmioRead32(IMX6_PHYSADDR_SRC_SCR) | IMX6_SRC_SCR_CORE1_RST | IMX6_SRC_SCR_CORE1_ENABLE;
    MmioWrite32(IMX6_PHYSADDR_SRC_SCR, Reg32);
    Reg32 = MmioRead32(IMX6_PHYSADDR_GPT_CNT) + SECONDARY_START_US_DELAY;
    while (MmioRead32(IMX6_PHYSADDR_GPT_CNT) < Reg32);
    Reg32 = MmioRead32(IMX6_PHYSADDR_SRC_SCR) | IMX6_SRC_SCR_CORE2_RST | IMX6_SRC_SCR_CORE2_ENABLE;
    MmioWrite32(IMX6_PHYSADDR_SRC_SCR, Reg32);
    Reg32 = MmioRead32(IMX6_PHYSADDR_GPT_CNT) + SECONDARY_START_US_DELAY;
    while (MmioRead32(IMX6_PHYSADDR_GPT_CNT) < Reg32);
    Reg32 = MmioRead32(IMX6_PHYSADDR_SRC_SCR) | IMX6_SRC_SCR_CORE3_RST | IMX6_SRC_SCR_CORE3_ENABLE;
    MmioWrite32(IMX6_PHYSADDR_SRC_SCR, Reg32);
    Reg32 = MmioRead32(IMX6_PHYSADDR_GPT_CNT) + SECONDARY_START_US_DELAY;
    while (MmioRead32(IMX6_PHYSADDR_GPT_CNT) < Reg32);

    //
    // wait for all cores to be powered on and participating in SMP
    //
    BaseAddress = ArmGetScuBaseAddress();
    if (0x000000F3 != (0x000000F3 & MmioRead32(BaseAddress + A9_SCU_CONFIG_OFFSET)))
    {
        while (0x000000F3 != (0x000000F3 & MmioRead32(BaseAddress + A9_SCU_CONFIG_OFFSET)));
    }

    //
    // wait for all cores to be in SecSecondary before WFI
    //
    while (0 != MmioRead32(UDOO_PHYSADDR_PERCPU_PAGES + 0x1000));
    while (0 != MmioRead32(UDOO_PHYSADDR_PERCPU_PAGES + 0x2000));
    while (0 != MmioRead32(UDOO_PHYSADDR_PERCPU_PAGES + 0x3000));

    // Enter Trusted Monitor for setup there
    return (UINT32)PrimaryTrustedMonitorInit;
}
