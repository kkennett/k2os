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

#include <WandQuad.h>
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
