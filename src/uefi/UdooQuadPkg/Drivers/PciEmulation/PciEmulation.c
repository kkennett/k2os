/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2016, Linaro, Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/EmbeddedExternalDevice.h>

#include <UdooQuad.h>
#include <Library/IMX6/IMX6GptLib.h>

static UINT32 sgRateGPT;

static
void
sDelayUs(
    UINT32 aUs
)
{
    IMX6_GPT_DelayUs(IMX6_PHYSADDR_GPT, sgRateGPT, aUs);
}

static
void
MmioSetBits32(
    UINT32 Addr,
    UINT32 Bits
)
{
    MmioWrite32(Addr, MmioRead32(Addr) | Bits);
}

static
void
MmioClrBits32(
    UINT32 Addr,
    UINT32 Bits
)
{
    MmioWrite32(Addr, MmioRead32(Addr) & ~Bits);
}


STATIC
EFI_STATUS
ConfigureUSBHost (
  NON_DISCOVERABLE_DEVICE   *Device
  )
{
    UINT32 Reg32;

    //
    // configure external hub HUB_USB_RST# and make sure reset is active
    //
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO17,
        IMX6_MUX_PADCTL_SPEED_100MHZ | IMX6_MUX_PADCTL_DSE_40_OHM | IMX6_MUX_PADCTL_SRE_SLOW);

    Reg32 = MmioRead32(IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_DR);
    Reg32 &= ~(1 << 12);
    MmioWrite32(IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_DR, Reg32);

    Reg32 = MmioRead32(IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_GDIR);
    Reg32 |= (1 << 12);
    MmioWrite32(IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_GDIR, Reg32);

    Reg32 = MmioRead32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO17);
    Reg32 &= ~0x17;
    Reg32 |= 5;
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO17, Reg32);

    //
    // set external hub USB_CLK to 24Mhz
    //
    Reg32 = MmioRead32(IMX6_PHYSADDR_CCM_CCOSR);
    Reg32 &= ~IMX6_CCM_CCOSR_CLKO2_EN;
    MmioWrite32(IMX6_PHYSADDR_CCM_CCOSR, Reg32);
    Reg32 &= ~(IMX6_CCM_CCOSR_CLKO2_DIV_MASK | IMX6_CCM_CCOSR_CLKO2_SEL_MASK);
    Reg32 |= IMX6_CCM_CCOSR_CLKO2_SEL_OSC_CLK;
    MmioWrite32(IMX6_PHYSADDR_CCM_CCOSR, Reg32);

    Reg32 = MmioRead32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_CS2_B);
    Reg32 &= ~0x17;
    Reg32 |= 4;
    MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_CS2_B, Reg32);

    Reg32 = MmioRead32(IMX6_PHYSADDR_CCM_CCOSR);
    Reg32 |= IMX6_CCM_CCOSR_CLKO2_EN;
    MmioWrite32(IMX6_PHYSADDR_CCM_CCOSR, Reg32);

    // LDO 1P1 to 1.1V
    Reg32 = MmioRead32(IMX6_PHYSADDR_ANALOG_PMU_REG_1P1);
    if (0 == (Reg32 & IMX6_ANALOG_PMU_REG_1P1_ENABLE_LINREG))
    {
        Reg32 &= ~IMX6_ANALOG_PMU_REG_1P1_OUTPUT_TRG_MASK;
        Reg32 |= IMX6_ANALOG_PMU_REG_1P1_OUTPUT_TRG_1P1;
        Reg32 &= ~IMX6_ANALOG_PMU_REG_1P1_OK_VDD1P1;
        Reg32 |= IMX6_ANALOG_PMU_REG_1P1_ENABLE_LINREG;
        MmioWrite32(IMX6_PHYSADDR_ANALOG_PMU_REG_1P1, Reg32);
        do
        {
            Reg32 = MmioRead32(IMX6_PHYSADDR_ANALOG_PMU_REG_1P1);
        } while (0 == (Reg32 & IMX6_ANALOG_PMU_REG_1P1_OK_VDD1P1));
    }
    else
    {
        if ((Reg32 & IMX6_ANALOG_PMU_REG_1P1_OUTPUT_TRG_MASK) != IMX6_ANALOG_PMU_REG_1P1_OUTPUT_TRG_1P1)
        {
            DebugPrint(0xFFFFFFFF, "REG_1P1 is already enabled but at wrong voltage\n");
            return EFI_DEVICE_ERROR;
        }
    }

    // LDO 2P5 to 2.5V
    Reg32 = MmioRead32(IMX6_PHYSADDR_ANALOG_PMU_REG_2P5);
    if (0 == (Reg32 & IMX6_ANALOG_PMU_REG_2P5_ENABLE_LINREG))
    {
        Reg32 &= ~IMX6_ANALOG_PMU_REG_2P5_OUTPUT_TRG_MASK;
        Reg32 |= IMX6_ANALOG_PMU_REG_2P5_OUTPUT_TRG_2P5;
        Reg32 &= ~IMX6_ANALOG_PMU_REG_2P5_OK_VDD2P5;
        Reg32 |= IMX6_ANALOG_PMU_REG_2P5_ENABLE_LINREG;
        MmioWrite32(IMX6_PHYSADDR_ANALOG_PMU_REG_2P5, Reg32);
        do
        {
            Reg32 = MmioRead32(IMX6_PHYSADDR_ANALOG_PMU_REG_2P5);
        } while (0 == (Reg32 & IMX6_ANALOG_PMU_REG_2P5_OK_VDD2P5));
    }
    else
    {
        if ((Reg32 & IMX6_ANALOG_PMU_REG_2P5_OUTPUT_TRG_MASK) != IMX6_ANALOG_PMU_REG_2P5_OUTPUT_TRG_2P5)
        {
            DebugPrint(0xFFFFFFFF, "REG_2P5 is already enabled but at wrong voltage\n");
            return EFI_DEVICE_ERROR;
        }
    }

    // USB LDO to 3.0V
    Reg32 = MmioRead32(IMX6_PHYSADDR_ANALOG_PMU_REG_3P0);
    if (0 == (Reg32 & IMX6_ANALOG_PMU_REG_3P0_ENABLE_LINREG))
    {
        Reg32 &= ~IMX6_ANALOG_PMU_REG_3P0_OUTPUT_TRG_MASK;
        Reg32 |= IMX6_ANALOG_PMU_REG_3P0_OUTPUT_TRG_3P0;
        Reg32 &= ~IMX6_ANALOG_PMU_REG_3P0_OK_VDD3P0;
        Reg32 &= ~IMX6_ANALOG_PMU_REG_3P0_VBUS_SEL;
        Reg32 |= IMX6_ANALOG_PMU_REG_3P0_ENABLE_LINREG;
        MmioWrite32(IMX6_PHYSADDR_ANALOG_PMU_REG_3P0, Reg32);
    }
    else
    {
        if ((Reg32 & IMX6_ANALOG_PMU_REG_3P0_OUTPUT_TRG_MASK) != IMX6_ANALOG_PMU_REG_3P0_OUTPUT_TRG_3P0)
        {
            DebugPrint(0xFFFFFFFF, "REG_3P0 is already enabled but at wrong voltage\n");
            return EFI_DEVICE_ERROR;
        }
        if (0 != (Reg32 & IMX6_ANALOG_PMU_REG_3P0_VBUS_SEL))
        {
            Reg32 &= ~IMX6_ANALOG_PMU_REG_3P0_VBUS_SEL;
            MmioWrite32(IMX6_PHYSADDR_ANALOG_PMU_REG_3P0, Reg32);
        }
    }

    // make sure USB_H1_PWR not selected on possible outputs (EIM_D31 NOT ALT6, GPIO0 NOT ALT6)
    Reg32 = MmioRead32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA31);
    if ((Reg32 & 7) == 6)
    {
        Reg32 &= ~7;
        Reg32 |= 5; // select GPIO3_IO31;
        MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA31, Reg32);
        // set GPIO3_IO31 as an input
        Reg32 = MmioRead32(IMX6_PHYSADDR_GPIO3 + IMX6_GPIO_OFFSET_GDIR);
        if (Reg32 & (1 << 31))
        {
            Reg32 &= ~(1 << 31);
            MmioWrite32(IMX6_PHYSADDR_GPIO3 + IMX6_GPIO_OFFSET_GDIR, Reg32);
        }
    }
    Reg32 = MmioRead32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO00);
    if ((Reg32 & 7) == 6)
    {
        Reg32 &= ~7;
        Reg32 |= 5; // select GPIO1_IO00;
        MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO00, Reg32);
        // set GPIO1_IO00 as an input
        Reg32 = MmioRead32(IMX6_PHYSADDR_GPIO1 + IMX6_GPIO_OFFSET_GDIR);
        if (Reg32 & 1)
        {
            Reg32 &= ~1;
            MmioWrite32(IMX6_PHYSADDR_GPIO1 + IMX6_GPIO_OFFSET_GDIR, Reg32);
        }
    }

    // make sure USB_H1_PWR_CTL_WAKE not selected on possible outputs (KEY_COL2 NOT ALT6)
    Reg32 = MmioRead32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_KEY_COL2);
    if ((Reg32 & 7) == 6)
    {
        Reg32 &= ~7;
        Reg32 |= 5; // select GPIO4_IO10;
        MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_KEY_COL2, Reg32);
        // set GPIO4_IO10 as an input
        Reg32 = MmioRead32(IMX6_PHYSADDR_GPIO4 + IMX6_GPIO_OFFSET_GDIR);
        if (Reg32 & (1 << 10))
        {
            Reg32 &= ~(1 << 10);
            MmioWrite32(IMX6_PHYSADDR_GPIO4 + IMX6_GPIO_OFFSET_GDIR, Reg32);
        }
    }

    // make sure USB_H1_OC not selected  on possible inputs (EIM D30 NOT ALT6, GPIO3 NOT ALT6)
    Reg32 = MmioRead32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA30);
    if ((Reg32 & 7) == 6)
    {
        Reg32 &= ~7;
        Reg32 |= 5; // select GPIO3_IO30;
        MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA30, Reg32);
        // set GPIO3_IO30 as an input
        Reg32 = MmioRead32(IMX6_PHYSADDR_GPIO3 + IMX6_GPIO_OFFSET_GDIR);
        if (Reg32 & (1 << 30))
        {
            Reg32 &= ~(1 << 30);
            MmioWrite32(IMX6_PHYSADDR_GPIO3 + IMX6_GPIO_OFFSET_GDIR, Reg32);
        }
    }
    Reg32 = MmioRead32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO03);
    if ((Reg32 & 7) == 6)
    {
        Reg32 &= ~7;
        Reg32 |= 5; // select GPIO1_IO03;
        MmioWrite32(IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO03, Reg32);
        // set GPIO1_IO03 as an input
        Reg32 = MmioRead32(IMX6_PHYSADDR_GPIO1 + IMX6_GPIO_OFFSET_GDIR);
        if (Reg32 & (1 << 3))
        {
            Reg32 &= ~(1 << 3);
            MmioWrite32(IMX6_PHYSADDR_GPIO1 + IMX6_GPIO_OFFSET_GDIR, Reg32);
        }
    }

    // clear clock bypasses
    MmioWrite32(IMX6_PHYSADDR_ANALOG_PLL_USB1_CLR, IMX6_ANALOG_PLL_USB_BYPASS);
    MmioWrite32(IMX6_PHYSADDR_ANALOG_PLL_USB2_CLR, IMX6_ANALOG_PLL_USB_BYPASS);

    // ensure plls are powered up, enabled, and USB clocks are enabled
    MmioWrite32(IMX6_PHYSADDR_ANALOG_PLL_USB1_SET,
        IMX6_ANALOG_PLL_USB_ENABLE |
        IMX6_ANALOG_PLL_USB_POWER |
        IMX6_ANALOG_PLL_USB_EN_USB_CLKS);
    MmioWrite32(IMX6_PHYSADDR_ANALOG_PLL_USB2_SET,
        IMX6_ANALOG_PLL_USB_ENABLE |
        IMX6_ANALOG_PLL_USB_POWER |
        IMX6_ANALOG_PLL_USB_EN_USB_CLKS);

    // ensure PLLs are locked before we continue
    do
    {
        Reg32 = MmioRead32(IMX6_PHYSADDR_ANALOG_PLL_USB1);
    } while (!(Reg32 & IMX6_ANALOG_PLL_USB_LOCK));
    do
    {
        Reg32 = MmioRead32(IMX6_PHYSADDR_ANALOG_PLL_USB2);
    } while (!(Reg32 & IMX6_ANALOG_PLL_USB_LOCK));

    // disable overcurrent detection, low active
    MmioSetBits32(IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USBNC_USB_UH1_CTRL, (1 << 8) | (1 << 7));

    // turn on (ungate) internal phy clocks
    MmioWrite32(IMX6_PHYSADDR_USBPHY2_CTRL_CLR, 0x40000000);

    sDelayUs(100);

    // enable clocks to UTMI blocks
    MmioWrite32(IMX6_PHYSADDR_ANALOG_USB2_MISC_SET, 0x40000000);

    sDelayUs(10);

    // install defaults to registers
    MmioWrite32(IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USBNC_USB_UH1_CTRL, 0x00003000);
    MmioWrite32(IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USBNC_USB_UH1_PHY_CTRL_0, 0x00000098);

    // Stop the host
    MmioClrBits32(IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USB_UH1_USBCMD, 1);
    do
    {
        Reg32 = MmioRead32(IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USB_UH1_USBSTS);
    } while (!(Reg32 & 0x00001000));

    // reset the controller
    MmioSetBits32(IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USB_UH1_USBCMD, 2);

    // wait for controller to complete reset
    do
    {
        Reg32 = MmioRead32(IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USB_UH1_USBCMD);
    } while (Reg32 & 2);

    // set USB PHY reset
    MmioWrite32(IMX6_PHYSADDR_USBPHY2_CTRL_SET, IMX6_USBPHY_CTRL_SFTRST);

    sDelayUs(10);

    // release resets and ungate clocks
    MmioWrite32(IMX6_PHYSADDR_USBPHY2_CTRL_CLR, IMX6_USBPHY_CTRL_SFTRST | IMX6_USBPHY_CTRL_CLKGATE);

    sDelayUs(10);

    // power up PHY
    MmioWrite32(IMX6_PHYSADDR_USBPHY2_PWD, 0);

    sDelayUs(10);

    // enable Full and Low speed devices
    MmioWrite32(IMX6_PHYSADDR_USBPHY2_CTRL_SET, IMX6_USBPHY_CTRL_ENUTMILEVEL3 | IMX6_USBPHY_CTRL_ENUTMILEVEL2);

    sDelayUs(10);

    // release reset on external hub
    Reg32 = MmioRead32(IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_DR);
    Reg32 |= (1 << 12);
    MmioWrite32(IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_DR, Reg32);

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciEmulationEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  UINT8 CapabilityLength;
  UINT8 PhysicalPorts;
  UINTN MemorySize;

  sgRateGPT = IMX6_GPT_GetRate(IMX6_PHYSADDR_CCM, IMX6_PHYSADDR_GPT);

  CapabilityLength = MmioRead8 (IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USB_UH1_CAPLENGTH_8);

  PhysicalPorts    = MmioRead32 (IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USB_UH1_HCSPARAMS) & 0x0000000F;
  ASSERT(PhysicalPorts >= 1);

  MemorySize       = CapabilityLength + IMX6_USBCORE_OPREG_SIZEBYTES + (4 * PhysicalPorts);

  return RegisterNonDiscoverableMmioDevice (
           NonDiscoverableDeviceTypeEhci,
           NonDiscoverableDeviceDmaTypeNonCoherent,
           ConfigureUSBHost,
           NULL,
           1,
           IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USB_UH1_CAPLENGTH_8, MemorySize
           );
}
