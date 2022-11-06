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

#ifndef __IMX6DEF_INC
#define __IMX6DEF_INC

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- *
                        BLOCK AND REGISTER BANK ADDRESSES
 * --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */

#define IMX6_HIGH_OSC_CLOCK_FREQ              24000000
#define IMX6_LOW_OSC_CLOCK_FREQ               32768

//
// Table 2-1 pg 213 in TRM Rev 3, 7/2015
//
#define IMX6_PHYSADDR_SDRAM_BASE              0x10000000
#define IMX6_PHYSSIZE_SDRAM_AREA              0xF0000000

#define IMX6_PHYSADDR_EIM_BASE                0x08000000
#define IMX6_PHYSSIZE_EIM_AREA                0x08000000

#define IMX6_PHYSADDR_IPU2                    0x02A00000
#define IMX6_PHYSSIZE_IPU2                    0x00400000

#define IMX6_PHYSADDR_IPU1                    0x02600000
#define IMX6_PHYSSIZE_IPU1                    0x00400000

#define IMX6_PHYSADDR_MIPI_HSI                0x02208000
#define IMX6_PHYSSIZE_MIPI_HSI                0x00004000

#define IMX6_PHYSADDR_OPENVG                  0x02204000
#define IMX6_PHYSSIZE_OPENVG                  0x00004000

#define IMX6_PHYSADDR_SATA                    0x02200000
#define IMX6_PHYSSIZE_SATA                    0x00004000

#define IMX6_PHYSADDR_AIPS2                   0x02100000
#define IMX6_PHYSSIZE_AIPS2                   0x00100000

#define IMX6_PHYSADDR_AIPS1                   0x02000000
#define IMX6_PHYSSIZE_AIPS1                   0x00100000

#define IMX6_PHYSADDR_PCIE_REGS               0x01FFC000
#define IMX6_PHYSSIZE_PCIE_REGS               0x00004000

#define IMX6_PHYSADDR_PCIE_SPACE              0x01000000
#define IMX6_PHYSSIZE_PCIE_SPACE              0x00FFC000

#define IMX6_PHYSADDR_GPV1                    0x00C00000
#define IMX6_PHYSSIZE_GPV1                    0x00100000

#define IMX6_PHYSADDR_GPV0                    0x00B00000
#define IMX6_PHYSSIZE_GPV0                    0x00100000

#define IMX6_PHYSADDR_PL310                   0x00A02000
#define IMX6_PHYSSIZE_PL310                   0x00001000

#define IMX6_PHYSADDR_ARM_PERIPH              0x00A00000
#define IMX6_PHYSSIZE_ARM_PERIPH              0x00002000

#define IMX6_PHYSADDR_OCRAM                   0x00900000
#define IMX6_PHYSSIZE_OCRAM                   0x00040000

#define IMX6_PHYSADDR_GPV4                    0x00800000
#define IMX6_PHYSSIZE_GPV4                    0x00100000

#define IMX6_PHYSADDR_GPV3                    0x00300000
#define IMX6_PHYSSIZE_GPV3                    0x00100000

#define IMX6_PHYSADDR_GPV2                    0x00200000
#define IMX6_PHYSSIZE_GPV2                    0x00100000

#define IMX6_PHYSADDR_DTCP                    0x00138000
#define IMX6_PHYSSIZE_DTCP                    0x00004000

#define IMX6_PHYSADDR_GPU2D                   0x00134000
#define IMX6_PHYSSIZE_GPU2D                   0x00004000

#define IMX6_PHYSADDR_GPU3D                   0x00130000
#define IMX6_PHYSSIZE_GPU3D                   0x00004000

#define IMX6_PHYSADDR_HDMI                    0x00120000
#define IMX6_PHYSSIZE_HDMI                    0x00009000

#define IMX6_PHYSADDR_BCH                     0x00114000
#define IMX6_PHYSSIZE_BCH                     0x00002000

#define IMX6_PHYSADDR_GPMI                    0x00112000
#define IMX6_PHYSSIZE_GPMI                    0x00002000

#define IMX6_PHYSADDR_APBH_DMA                0x00110000
#define IMX6_PHYSSIZE_APBH_DMA                0x00002000

#define IMX6_PHYSADDR_CAAM_AREA               0x00100000
#define IMX6_PHYSSIZE_CAAM_AREA               0x00008000

#define IMX6_PHYSADDR_BOOTROM                 0x00000000
#define IMX6_PHYSSIZE_BOOTROM                 0x00018000

/* --------------------------------------------------------------------------------- */

#define IMX6_PHYSADDR_OCRAM_FREE              0x00907000
#define IMX6_PHYSSIZE_OCRAM_FREE              0x00031000
#define IMX6_PHYSADDR_OCRAM_FREE_END          (IMX6_PHYSADDR_OCRAM_FREE + IMX6_PHYSSIZE_OCRAM_FREE)

#define IMX6_PHYSADDR_OCRAM_EXPAGE            0x0093F000
#define IMX6_PHYSADDR_OCRAM_EXVECTORS         0x0093FFB8

/* --------------------------------------------------------------------------------- */

#define IMX6_ARM_PERIPH_OFFSET_SCU            0x0000
#define IMX6_ARM_PERIPH_OFFSET_GIC_INTC       0x0100
#define IMX6_ARM_PERIPH_OFFSET_GLOBAL_TIMER   0x0200
#define IMX6_ARM_PERIPH_OFFSET_PRIVATE_TIMERS 0x0600
#define IMX6_ARM_PERIPH_OFFSET_GIC_DIST       0x1000

#define IMX6_PHYSADDR_ARM_SCU                 (IMX6_PHYSADDR_ARM_PERIPH + IMX6_ARM_PERIPH_OFFSET_SCU           )
#define IMX6_PHYSADDR_ARM_GIC_INTC            (IMX6_PHYSADDR_ARM_PERIPH + IMX6_ARM_PERIPH_OFFSET_GIC_INTC      )
#define IMX6_PHYSADDR_ARM_GLOBAL_TIMER        (IMX6_PHYSADDR_ARM_PERIPH + IMX6_ARM_PERIPH_OFFSET_GLOBAL_TIMER  )
#define IMX6_PHYSADDR_ARM_PRIVATE_TIMERS      (IMX6_PHYSADDR_ARM_PERIPH + IMX6_ARM_PERIPH_OFFSET_PRIVATE_TIMERS)
#define IMX6_PHYSADDR_ARM_GIC_DIST            (IMX6_PHYSADDR_ARM_PERIPH + IMX6_ARM_PERIPH_OFFSET_GIC_DIST      )

/* --------------------------------------------------------------------------------- */

#define IMX6_AIPS1_OFFSET_SDMA                0x000EC000 
#define IMX6_AIPS1_OFFSET_DCIC2               0x000E8000 
#define IMX6_AIPS1_OFFSET_DCIC1               0x000E4000 
#define IMX6_AIPS1_OFFSET_IOMUXC              0x000E0000 
#define IMX6_AIPS1_OFFSET_PGC_ARM             0x000DC2C0 
#define IMX6_AIPS1_OFFSET_PGC_PU              0x000DC260 
#define IMX6_AIPS1_OFFSET_GPC                 0x000DC000 
#define IMX6_AIPS1_OFFSET_SRC                 0x000D8000 
#define IMX6_AIPS1_OFFSET_EPIT2               0x000D4000 
#define IMX6_AIPS1_OFFSET_EPIT1               0x000D0000 
#define IMX6_AIPS1_OFFSET_SNVS                0x000CC000 
#define IMX6_AIPS1_OFFSET_USBPHY2             0x000CA000 
#define IMX6_AIPS1_OFFSET_USBPHY1             0x000C9000 
#define IMX6_AIPS1_OFFSET_ANALOG              0x000C8000 
#define IMX6_AIPS1_OFFSET_CCM                 0x000C4000 
#define IMX6_AIPS1_OFFSET_WDOG2               0x000C0000 
#define IMX6_AIPS1_OFFSET_WDOG1               0x000BC000 
#define IMX6_AIPS1_OFFSET_KPP                 0x000B8000 
#define IMX6_AIPS1_OFFSET_GPIO7               0x000B4000 
#define IMX6_AIPS1_OFFSET_GPIO6               0x000B0000 
#define IMX6_AIPS1_OFFSET_GPIO5               0x000AC000 
#define IMX6_AIPS1_OFFSET_GPIO4               0x000A8000 
#define IMX6_AIPS1_OFFSET_GPIO3               0x000A4000 
#define IMX6_AIPS1_OFFSET_GPIO2               0x000A0000 
#define IMX6_AIPS1_OFFSET_GPIO1               0x0009C000 
#define IMX6_AIPS1_OFFSET_GPT                 0x00098000 
#define IMX6_AIPS1_OFFSET_CAN2                0x00094000 
#define IMX6_AIPS1_OFFSET_CAN1                0x00090000 
#define IMX6_AIPS1_OFFSET_PWM4                0x0008C000 
#define IMX6_AIPS1_OFFSET_PWM3                0x00088000 
#define IMX6_AIPS1_OFFSET_PWM2                0x00084000 
#define IMX6_AIPS1_OFFSET_PWM1                0x00080000 
#define IMX6_AIPS1_OFFSET_CONFIG              0x0007C000 
#define IMX6_AIPS1_OFFSET_SPBA                0x0003C000 
#define IMX6_AIPS1_OFFSET_ASRC                0x00034000 
#define IMX6_AIPS1_OFFSET_SSI3                0x00030000 
#define IMX6_AIPS1_OFFSET_SSI2                0x0002C000 
#define IMX6_AIPS1_OFFSET_SSI1                0x00028000 
#define IMX6_AIPS1_OFFSET_ESAI                0x00024000 
#define IMX6_AIPS1_OFFSET_UART1               0x00020000 
#define IMX6_AIPS1_OFFSET_ECSPI4              0x00014000 
#define IMX6_AIPS1_OFFSET_ECSPI3              0x00010000 
#define IMX6_AIPS1_OFFSET_ECSPI2              0x0000C000 
#define IMX6_AIPS1_OFFSET_ECSPI1              0x00008000 
#define IMX6_AIPS1_OFFSET_SPDIF               0x00004000 

#define IMX6_PHYSADDR_SDMA                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_SDMA          )
#define IMX6_PHYSADDR_DCIC2                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_DCIC2         )
#define IMX6_PHYSADDR_DCIC1                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_DCIC1         )
#define IMX6_PHYSADDR_IOMUXC                  (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_IOMUXC        )
#define IMX6_PHYSADDR_PGC_ARM                 (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_PGC_ARM       )
#define IMX6_PHYSADDR_PGC_PU                  (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_PGC_PU        )
#define IMX6_PHYSADDR_GPC                     (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_GPC           )
#define IMX6_PHYSADDR_SRC                     (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_SRC           )
#define IMX6_PHYSADDR_EPIT2                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_EPIT2         )
#define IMX6_PHYSADDR_EPIT1                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_EPIT1         )
#define IMX6_PHYSADDR_SNVS                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_SNVS          )
#define IMX6_PHYSADDR_USBPHY2                 (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_USBPHY2       )
#define IMX6_PHYSADDR_USBPHY1                 (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_USBPHY1       )
#define IMX6_PHYSADDR_ANALOG                  (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_ANALOG        )
#define IMX6_PHYSADDR_CCM                     (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_CCM           )
#define IMX6_PHYSADDR_WDOG2                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_WDOG2         )
#define IMX6_PHYSADDR_WDOG1                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_WDOG1         )
#define IMX6_PHYSADDR_KPP                     (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_KPP           )
#define IMX6_PHYSADDR_GPIO7                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_GPIO7         )
#define IMX6_PHYSADDR_GPIO6                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_GPIO6         )
#define IMX6_PHYSADDR_GPIO5                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_GPIO5         )
#define IMX6_PHYSADDR_GPIO4                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_GPIO4         )
#define IMX6_PHYSADDR_GPIO3                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_GPIO3         )
#define IMX6_PHYSADDR_GPIO2                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_GPIO2         )
#define IMX6_PHYSADDR_GPIO1                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_GPIO1         )
#define IMX6_PHYSADDR_GPT                     (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_GPT           )
#define IMX6_PHYSADDR_CAN2                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_CAN2          )
#define IMX6_PHYSADDR_CAN1                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_CAN1          )
#define IMX6_PHYSADDR_PWM4                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_PWM4          )
#define IMX6_PHYSADDR_PWM3                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_PWM3          )
#define IMX6_PHYSADDR_PWM2                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_PWM2          )
#define IMX6_PHYSADDR_PWM1                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_PWM1          )
#define IMX6_PHYSADDR_AIPS1_CONFIG            (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_CONFIG        )
#define IMX6_PHYSADDR_AIPS1_SPBA              (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_SPBA          )
#define IMX6_PHYSADDR_ASRC                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_ASRC          )
#define IMX6_PHYSADDR_SSI3                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_SSI3          )
#define IMX6_PHYSADDR_SSI2                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_SSI2          )
#define IMX6_PHYSADDR_SSI1                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_SSI1          )
#define IMX6_PHYSADDR_ESAI                    (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_ESAI          )
#define IMX6_PHYSADDR_UART1                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_UART1         )
#define IMX6_PHYSADDR_ECSPI4                  (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_ECSPI4        )
#define IMX6_PHYSADDR_ECSPI3                  (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_ECSPI3        )
#define IMX6_PHYSADDR_ECSPI2                  (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_ECSPI2        )
#define IMX6_PHYSADDR_ECSPI1                  (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_ECSPI1        )
#define IMX6_PHYSADDR_SPDIF                   (IMX6_PHYSADDR_AIPS1 + IMX6_AIPS1_OFFSET_SPDIF         )

/* --------------------------------------------------------------------------------- */

#define IMX6_AIPS2_OFFSET_UART5               0x000F4000
#define IMX6_AIPS2_OFFSET_UART4               0x000F0000
#define IMX6_AIPS2_OFFSET_UART3               0x000EC000
#define IMX6_AIPS2_OFFSET_UART2               0x000E8000
#define IMX6_AIPS2_OFFSET_VDOA                0x000E4000
#define IMX6_AIPS2_OFFSET_MIPI_DSI            0x000E0000
#define IMX6_AIPS2_OFFSET_MIPI_CSI            0x000DC000
#define IMX6_AIPS2_OFFSET_AUDMUX              0x000D8000
#define IMX6_AIPS2_OFFSET_TZASC2              0x000D4000
#define IMX6_AIPS2_OFFSET_TZASC1              0x000D0000
#define IMX6_AIPS2_OFFSET_CSU                 0x000C0000
#define IMX6_AIPS2_OFFSET_OCOTP               0x000BC000
#define IMX6_AIPS2_OFFSET_EIM                 0x000B8000
#define IMX6_AIPS2_OFFSET_MMDC1               0x000B4000
#define IMX6_AIPS2_OFFSET_MMDC0               0x000B0000
#define IMX6_AIPS2_OFFSET_ROMCP               0x000AC000
#define IMX6_AIPS2_OFFSET_I2C3                0x000A8000
#define IMX6_AIPS2_OFFSET_I2C2                0x000A4000
#define IMX6_AIPS2_OFFSET_I2C1                0x000A0000
#define IMX6_AIPS2_OFFSET_USDHC4              0x0009C000
#define IMX6_AIPS2_OFFSET_USDHC3              0x00098000
#define IMX6_AIPS2_OFFSET_USDHC2              0x00094000
#define IMX6_AIPS2_OFFSET_USDHC1              0x00090000
#define IMX6_AIPS2_OFFSET_MLB150              0x0008C000
#define IMX6_AIPS2_OFFSET_ENET1               0x00088000
#define IMX6_AIPS2_OFFSET_USBOH3              0x00084000
#define IMX6_AIPS2_OFFSET_CONFIG              0x0007C000
#define IMX6_AIPS2_OFFSET_A9MPCORE            0x00040000
#define IMX6_AIPS2_OFFSET_CAAM                0x00000000

#define IMX6_PHYSADDR_UART5                   (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_UART5     )
#define IMX6_PHYSADDR_UART4                   (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_UART4     )
#define IMX6_PHYSADDR_UART3                   (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_UART3     )
#define IMX6_PHYSADDR_UART2                   (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_UART2     )
#define IMX6_PHYSADDR_VDOA                    (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_VDOA      )
#define IMX6_PHYSADDR_MIPI_DSI                (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_MIPI_DSI  )
#define IMX6_PHYSADDR_MIPI_CSI                (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_MIPI_CSI  )
#define IMX6_PHYSADDR_AUDMUX                  (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_AUDMUX    )
#define IMX6_PHYSADDR_TZASC1                  (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_TZASC1    )
#define IMX6_PHYSADDR_TZASC0                  (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_TZASC0    )
#define IMX6_PHYSADDR_CSU                     (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_CSU       )
#define IMX6_PHYSADDR_OCOTP                   (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_OCOTP     )
#define IMX6_PHYSADDR_EIM                     (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_EIM       )
#define IMX6_PHYSADDR_MMDC1                   (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_MMDC1     )
#define IMX6_PHYSADDR_MMDC0                   (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_MMDC0     )
#define IMX6_PHYSADDR_ROMCP                   (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_ROMCP     )
#define IMX6_PHYSADDR_I2C3                    (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_I2C3      )
#define IMX6_PHYSADDR_I2C2                    (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_I2C2      )
#define IMX6_PHYSADDR_I2C1                    (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_I2C1      )
#define IMX6_PHYSADDR_USDHC4                  (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_USDHC4    )
#define IMX6_PHYSADDR_USDHC3                  (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_USDHC3    )
#define IMX6_PHYSADDR_USDHC2                  (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_USDHC2    )
#define IMX6_PHYSADDR_USDHC1                  (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_USDHC1    )
#define IMX6_PHYSADDR_MLB150                  (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_MLB150    )
#define IMX6_PHYSADDR_ENET1                   (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_ENET1     )
#define IMX6_PHYSADDR_USBOH3                  (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_USBOH3    )
#define IMX6_PHYSADDR_AIPS2_CONFIG            (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_CONFIG    )
#define IMX6_PHYSADDR_A9MPCORE                (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_A9MPCORE  )
#define IMX6_PHYSADDR_CAAM                    (IMX6_PHYSADDR_AIPS2 + IMX6_AIPS2_OFFSET_CAAM      )

/* --------------------------------------------------------------------------------- */

#define IMX6_A9MPCORE_OFFSET_PLAT_CTRL       0x20000
#define IMX6_A9MPCORE_OFFSET_PTM3            0x1F000
#define IMX6_A9MPCORE_OFFSET_PTM2            0x1E000
#define IMX6_A9MPCORE_OFFSET_PTM1            0x1D000
#define IMX6_A9MPCORE_OFFSET_PTM0            0x1C000
#define IMX6_A9MPCORE_OFFSET_CTI3            0x1B000
#define IMX6_A9MPCORE_OFFSET_CTI2            0x1A000
#define IMX6_A9MPCORE_OFFSET_CTI1            0x19000
#define IMX6_A9MPCORE_OFFSET_CTI0            0x18000
#define IMX6_A9MPCORE_OFFSET_CPU3_PMU        0x17000
#define IMX6_A9MPCORE_OFFSET_CPU3_DEBUG      0x16000
#define IMX6_A9MPCORE_OFFSET_CPU2_PMU        0x15000
#define IMX6_A9MPCORE_OFFSET_CPU2_DEBUG      0x14000
#define IMX6_A9MPCORE_OFFSET_CPU1_PMU        0x13000
#define IMX6_A9MPCORE_OFFSET_CPU1_DEBUG      0x12000
#define IMX6_A9MPCORE_OFFSET_CPU0_PMU        0x11000
#define IMX6_A9MPCORE_OFFSET_CPU0_DEBUG      0x10000
#define IMX6_A9MPCORE_OFFSET_CA9_INTEG       0x0F000
#define IMX6_A9MPCORE_OFFSET_RESERVED        0x05000
#define IMX6_A9MPCORE_OFFSET_FUNNEL          0x04000
#define IMX6_A9MPCORE_OFFSET_TPIU            0x03000
#define IMX6_A9MPCORE_OFFSET_EXT_CTI         0x02000
#define IMX6_A9MPCORE_OFFSET_ETB             0x01000
#define IMX6_A9MPCORE_OFFSET_DAPROM_TABLE    0x00000

#define IMX6_PHYSADDR_A9MP_PLAT_CTRL       (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_PLAT_CTRL      )
#define IMX6_PHYSADDR_A9MP_PTM3            (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_PTM3           )
#define IMX6_PHYSADDR_A9MP_PTM2            (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_PTM2           )
#define IMX6_PHYSADDR_A9MP_PTM1            (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_PTM1           )
#define IMX6_PHYSADDR_A9MP_PTM0            (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_PTM0           )
#define IMX6_PHYSADDR_A9MP_CTI3            (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CTI3           )
#define IMX6_PHYSADDR_A9MP_CTI2            (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CTI2           )
#define IMX6_PHYSADDR_A9MP_CTI1            (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CTI1           )
#define IMX6_PHYSADDR_A9MP_CTI0            (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CTI0           )
#define IMX6_PHYSADDR_A9MP_CPU3_PMU        (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CPU3_PMU       )
#define IMX6_PHYSADDR_A9MP_CPU3_DEBUG      (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CPU3_DEBUG     )
#define IMX6_PHYSADDR_A9MP_CPU2_PMU        (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CPU2_PMU       )
#define IMX6_PHYSADDR_A9MP_CPU2_DEBUG      (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CPU2_DEBUG     )
#define IMX6_PHYSADDR_A9MP_CPU1_PMU        (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CPU1_PMU       )
#define IMX6_PHYSADDR_A9MP_CPU1_DEBUG      (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CPU1_DEBUG     )
#define IMX6_PHYSADDR_A9MP_CPU0_PMU        (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CPU0_PMU       )
#define IMX6_PHYSADDR_A9MP_CPU0_DEBUG      (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CPU0_DEBUG     )
#define IMX6_PHYSADDR_A9MP_CA9_INTEG       (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_CA9_INTEG      )
#define IMX6_PHYSADDR_A9MP_RESERVED        (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_RESERVED       )
#define IMX6_PHYSADDR_A9MP_FUNNEL          (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_FUNNEL         )
#define IMX6_PHYSADDR_A9MP_TPIU            (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_TPIU           )
#define IMX6_PHYSADDR_A9MP_EXT_CTI         (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_EXT_CTI        )
#define IMX6_PHYSADDR_A9MP_ETB             (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_ETB            )
#define IMX6_PHYSADDR_A9MP_DAPROM_TABLE    (IMX6_PHYSADDR_A9MPCORE + IMX6_A9MPCORE_OFFSET_DAPROM_TABLE   )

/* --------------------------------------------------------------------------------- */

#define IMX6_AIPS_OFFSET_MPR                  0x0000
#define IMX6_AIPS_OFFSET_OPACR0               0x0040
#define IMX6_AIPS_OFFSET_OPACR1               0x0044
#define IMX6_AIPS_OFFSET_OPACR2               0x0048
#define IMX6_AIPS_OFFSET_OPACR3               0x004C
#define IMX6_AIPS_OFFSET_OPACR4               0x0050

#define IMX6_PHYSADDR_AIPS1_MPR               (IMX6_PHYSADDR_AIPS1_CONFIG + IMX6_AIPS_OFFSET_MPR   )
#define IMX6_PHYSADDR_AIPS1_OPACR0            (IMX6_PHYSADDR_AIPS1_CONFIG + IMX6_AIPS_OFFSET_OPACR0)
#define IMX6_PHYSADDR_AIPS1_OPACR1            (IMX6_PHYSADDR_AIPS1_CONFIG + IMX6_AIPS_OFFSET_OPACR1)
#define IMX6_PHYSADDR_AIPS1_OPACR2            (IMX6_PHYSADDR_AIPS1_CONFIG + IMX6_AIPS_OFFSET_OPACR2)
#define IMX6_PHYSADDR_AIPS1_OPACR3            (IMX6_PHYSADDR_AIPS1_CONFIG + IMX6_AIPS_OFFSET_OPACR3)
#define IMX6_PHYSADDR_AIPS1_OPACR4            (IMX6_PHYSADDR_AIPS1_CONFIG + IMX6_AIPS_OFFSET_OPACR4)

#define IMX6_PHYSADDR_AIPS2_MPR               (IMX6_PHYSADDR_AIPS2_CONFIG + IMX6_AIPS_OFFSET_MPR   )
#define IMX6_PHYSADDR_AIPS2_OPACR0            (IMX6_PHYSADDR_AIPS2_CONFIG + IMX6_AIPS_OFFSET_OPACR0)
#define IMX6_PHYSADDR_AIPS2_OPACR1            (IMX6_PHYSADDR_AIPS2_CONFIG + IMX6_AIPS_OFFSET_OPACR1)
#define IMX6_PHYSADDR_AIPS2_OPACR2            (IMX6_PHYSADDR_AIPS2_CONFIG + IMX6_AIPS_OFFSET_OPACR2)
#define IMX6_PHYSADDR_AIPS2_OPACR3            (IMX6_PHYSADDR_AIPS2_CONFIG + IMX6_AIPS_OFFSET_OPACR3)
#define IMX6_PHYSADDR_AIPS2_OPACR4            (IMX6_PHYSADDR_AIPS2_CONFIG + IMX6_AIPS_OFFSET_OPACR4)

#define IMX6_PHYSADDR_AIPS3_MPR               (IMX6_PHYSADDR_AIPS3_CONFIG + IMX6_AIPS_OFFSET_MPR   )
#define IMX6_PHYSADDR_AIPS3_OPACR0            (IMX6_PHYSADDR_AIPS3_CONFIG + IMX6_AIPS_OFFSET_OPACR0)
#define IMX6_PHYSADDR_AIPS3_OPACR1            (IMX6_PHYSADDR_AIPS3_CONFIG + IMX6_AIPS_OFFSET_OPACR1)
#define IMX6_PHYSADDR_AIPS3_OPACR2            (IMX6_PHYSADDR_AIPS3_CONFIG + IMX6_AIPS_OFFSET_OPACR2)
#define IMX6_PHYSADDR_AIPS3_OPACR3            (IMX6_PHYSADDR_AIPS3_CONFIG + IMX6_AIPS_OFFSET_OPACR3)
#define IMX6_PHYSADDR_AIPS3_OPACR4            (IMX6_PHYSADDR_AIPS3_CONFIG + IMX6_AIPS_OFFSET_OPACR4)

#define IMX6_AIPS_MPR_MPROT0_SHL              28  // All masters excluding ARM core, SDMA, and CAAM
#define IMX6_AIPS_MPR_MPROT1_SHL              24  // ARM A9
#define IMX6_AIPS_MPR_MPROT2_SHL              20  // CAAM
#define IMX6_AIPS_MPR_MPROT3_SHL              16  // SDMA

#define IMX6_AIPS_MPR_MPROT_BIT_MPL           0x1   // Accesses NOT forced to user mode
#define IMX6_AIPS_MPR_MPROT_BIT_MTW           0x2   // Trust Write Accesses
#define IMX6_AIPS_MPR_MPROT_BIT_MTR           0x4   // Trust Read Accesses
#define IMX6_AIPS_MPR_MPROT_BIT_MBW           0x8   // Allowed to buffer writes
#define IMX6_AIPS_MPR_MPROT_BIT_MASK          0xF

#define IMX6_AIPS_MPR_MPROT_TRUST_UNBUFFERED  (IMX6_AIPS_MPR_MPROT_BIT_MTR | IMX6_AIPS_MPR_MPROT_BIT_MTW | IMX6_AIPS_MPR_MPROT_BIT_MPL)

#define IMX6_AIPS_MPR_MPROT0(x)               (((x) & IMX6_AIPS_MPR_MPROT_BIT_MASK) << IMX6_AIPS_MPR_MPROT0_SHL)
#define IMX6_AIPS_MPR_MPROT1(x)               (((x) & IMX6_AIPS_MPR_MPROT_BIT_MASK) << IMX6_AIPS_MPR_MPROT1_SHL)
#define IMX6_AIPS_MPR_MPROT2(x)               (((x) & IMX6_AIPS_MPR_MPROT_BIT_MASK) << IMX6_AIPS_MPR_MPROT2_SHL)
#define IMX6_AIPS_MPR_MPROT3(x)               (((x) & IMX6_AIPS_MPR_MPROT_BIT_MASK) << IMX6_AIPS_MPR_MPROT3_SHL)

#define IMX6_AIPS_MPR_ALL_TRUST_UNBUFFERED    (IMX6_AIPS_MPR_MPROT0(IMX6_AIPS_MPR_MPROT_TRUST_UNBUFFERED) | \
                                                 IMX6_AIPS_MPR_MPROT1(IMX6_AIPS_MPR_MPROT_TRUST_UNBUFFERED) | \
                                                 IMX6_AIPS_MPR_MPROT2(IMX6_AIPS_MPR_MPROT_TRUST_UNBUFFERED) | \
                                                 IMX6_AIPS_MPR_MPROT3(IMX6_AIPS_MPR_MPROT_TRUST_UNBUFFERED))

// OPACR07    GPIO1        
// OPACR06    GPT          
// OPACR05    CAN2         
// OPACR04    CAN1         
// OPACR03    PWM4         
// OPACR02    PWM3         
// OPACR01    PWM2         
// OPACR00    PWM1         
#define IMX6_AIPS_OPACR0_OPAC00_SHL             28  
#define IMX6_AIPS_OPACR0_OPAC01_SHL             24  
#define IMX6_AIPS_OPACR0_OPAC02_SHL             20  
#define IMX6_AIPS_OPACR0_OPAC03_SHL             16  
#define IMX6_AIPS_OPACR0_OPAC04_SHL             12  
#define IMX6_AIPS_OPACR0_OPAC05_SHL             8   
#define IMX6_AIPS_OPACR0_OPAC06_SHL             4   
#define IMX6_AIPS_OPACR0_OPAC07_SHL             0   

// OPACR15    WDOG1        
// OPACR14    KPP          
// OPACR13    GPIO7        
// OPACR12    GPIO6        
// OPACR11    GPIO5        
// OPACR10    GPIO4        
// OPACR09    GPIO3        
// OPACR08    GPIO2        
#define IMX6_AIPS_OPACR1_OPAC08_SHL             28  
#define IMX6_AIPS_OPACR1_OPAC09_SHL             24  
#define IMX6_AIPS_OPACR1_OPAC10_SHL             20  
#define IMX6_AIPS_OPACR1_OPAC11_SHL             16  
#define IMX6_AIPS_OPACR1_OPAC12_SHL             12  
#define IMX6_AIPS_OPACR1_OPAC13_SHL             8   
#define IMX6_AIPS_OPACR1_OPAC14_SHL             4   
#define IMX6_AIPS_OPACR1_OPAC15_SHL             0   

// OPACR23    Reserved     
// OPACR22    SRC          
// OPACR21    EPIT2        
// OPACR20    EPIT1        
// OPACR19    SNVS_HP      
// OPACR18    Reserved     
// OPACR17    CCM_DIGITAL  
// OPACR16    WDOG2        
#define IMX6_AIPS_OPACR2_OPAC16_SHL             28  
#define IMX6_AIPS_OPACR2_OPAC17_SHL             24  
#define IMX6_AIPS_OPACR2_OPAC18_SHL             20  
#define IMX6_AIPS_OPACR2_OPAC19_SHL             16  
#define IMX6_AIPS_OPACR2_OPAC20_SHL             12  
#define IMX6_AIPS_OPACR2_OPAC21_SHL             8   
#define IMX6_AIPS_OPACR2_OPAC22_SHL             4   
#define IMX6_AIPS_OPACR2_OPAC23_SHL             0   

// OPACR31    Reserved     
// OPACR30    Reserved     
// OPACR29    Reserved     
// OPACR28    Reserved     
// OPACR27    SDMA         
// OPACR26    DCIC2        
// OPACR25    DCIC1        
// OPACR24    IOMUXC       
#define IMX6_AIPS_OPACR3_OPAC24_SHL             28  
#define IMX6_AIPS_OPACR3_OPAC25_SHL             24  
#define IMX6_AIPS_OPACR3_OPAC26_SHL             20  
#define IMX6_AIPS_OPACR3_OPAC27_SHL             16  
#define IMX6_AIPS_OPACR3_OPAC28_SHL             12  
#define IMX6_AIPS_OPACR3_OPAC29_SHL             8   
#define IMX6_AIPS_OPACR3_OPAC30_SHL             4   
#define IMX6_AIPS_OPACR3_OPAC31_SHL             0   

// OPACR33    VPU          
// OPACR32    SPBA         
#define IMX6_AIPS_OPACR4_OPAC32_SHL             28  
#define IMX6_AIPS_OPACR4_OPAC33_SHL             24  

#define IMX6_AIPS_OPACR_OPAC_BIT_TP           0x1     // Trusted Protect (1 = accesses not allowed)
#define IMX6_AIPS_OPACR_OPAC_BIT_WP           0x2     // Write Protect (1 = write protected)
#define IMX6_AIPS_OPACR_OPAC_BIT_SP           0x4     // Supervisor Protect (1 = supervisor priviledge required)
#define IMX6_AIPS_OPACR_OPAC_BIT_BW           0x8     // Buffer Writes (1 = writes are allowed to be buffered)
#define IMX6_AIPS_OPACR_OPAC_BIT_MASK         0xF

#define IMX6_AIPS_OPACR_OPAC_TRUST_UNBUFFERED 0

/* --------------------------------------------------------------------------------- */

#define IMX6_CCM_OFFSET_CCR                   0x0000 
#define IMX6_CCM_OFFSET_CCDR                  0x0004 
#define IMX6_CCM_OFFSET_CSR                   0x0008 
#define IMX6_CCM_OFFSET_CCSR                  0x000C 
#define IMX6_CCM_OFFSET_CACRR                 0x0010 
#define IMX6_CCM_OFFSET_CBCDR                 0x0014 
#define IMX6_CCM_OFFSET_CBCMR                 0x0018 
#define IMX6_CCM_OFFSET_CSCMR1                0x001C 
#define IMX6_CCM_OFFSET_CSCMR2                0x0020 
#define IMX6_CCM_OFFSET_CSCDR1                0x0024 
#define IMX6_CCM_OFFSET_CS1CDR                0x0028 
#define IMX6_CCM_OFFSET_CS2CDR                0x002C 
#define IMX6_CCM_OFFSET_CDCDR                 0x0030 
#define IMX6_CCM_OFFSET_CHSCCDR               0x0034 
#define IMX6_CCM_OFFSET_CSCDR2                0x0038 
#define IMX6_CCM_OFFSET_CSCDR3                0x003C 
#define IMX6_CCM_OFFSET_CDHIPR                0x0048 
#define IMX6_CCM_OFFSET_CLPCR                 0x0054 
#define IMX6_CCM_OFFSET_CISR                  0x0058 
#define IMX6_CCM_OFFSET_CIMR                  0x005C 
#define IMX6_CCM_OFFSET_CCOSR                 0x0060 
#define IMX6_CCM_OFFSET_CGPR                  0x0064 
#define IMX6_CCM_OFFSET_CCGR0                 0x0068 
#define IMX6_CCM_OFFSET_CCGR1                 0x006C 
#define IMX6_CCM_OFFSET_CCGR2                 0x0070 
#define IMX6_CCM_OFFSET_CCGR3                 0x0074 
#define IMX6_CCM_OFFSET_CCGR4                 0x0078 
#define IMX6_CCM_OFFSET_CCGR5                 0x007C 
#define IMX6_CCM_OFFSET_CCGR6                 0x0080 
#define IMX6_CCM_OFFSET_CMEOR                 0x0088 

#define IMX6_PHYSADDR_CCM_CCR                 (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CCR    )
#define IMX6_PHYSADDR_CCM_CCDR                (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CCDR   )
#define IMX6_PHYSADDR_CCM_CSR                 (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CSR    )
#define IMX6_PHYSADDR_CCM_CCSR                (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CCSR   )
#define IMX6_PHYSADDR_CCM_CACRR               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CACRR  )
#define IMX6_PHYSADDR_CCM_CBCDR               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CBCDR  )
#define IMX6_PHYSADDR_CCM_CBCMR               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CBCMR  )
#define IMX6_PHYSADDR_CCM_CSCMR1              (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CSCMR1 )
#define IMX6_PHYSADDR_CCM_CSCMR2              (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CSCMR2 )
#define IMX6_PHYSADDR_CCM_CSCDR1              (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CSCDR1 )
#define IMX6_PHYSADDR_CCM_CS1CDR              (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CS1CDR )
#define IMX6_PHYSADDR_CCM_CS2CDR              (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CS2CDR )
#define IMX6_PHYSADDR_CCM_CDCDR               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CDCDR  )
#define IMX6_PHYSADDR_CCM_CHSCCDR             (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CHSCCDR)
#define IMX6_PHYSADDR_CCM_CSCDR2              (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CSCDR2 )
#define IMX6_PHYSADDR_CCM_CSCDR3              (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CSCDR3 )
#define IMX6_PHYSADDR_CCM_CDHIPR              (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CDHIPR )
#define IMX6_PHYSADDR_CCM_CLPCR               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CLPCR  )
#define IMX6_PHYSADDR_CCM_CISR                (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CISR   )
#define IMX6_PHYSADDR_CCM_CIMR                (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CIMR   )
#define IMX6_PHYSADDR_CCM_CCOSR               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CCOSR  )
#define IMX6_PHYSADDR_CCM_CGPR                (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CGPR   )
#define IMX6_PHYSADDR_CCM_CCGR0               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CCGR0  )
#define IMX6_PHYSADDR_CCM_CCGR1               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CCGR1  )
#define IMX6_PHYSADDR_CCM_CCGR2               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CCGR2  )
#define IMX6_PHYSADDR_CCM_CCGR3               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CCGR3  )
#define IMX6_PHYSADDR_CCM_CCGR4               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CCGR4  )
#define IMX6_PHYSADDR_CCM_CCGR5               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CCGR5  )
#define IMX6_PHYSADDR_CCM_CCGR6               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CCGR6  )
#define IMX6_PHYSADDR_CCM_CMEOR               (IMX6_PHYSADDR_CCM + IMX6_CCM_OFFSET_CMEOR  )

#define IMX6_CCM_CACRR_ARM_PODF_MASK          0x00000007

#define IMX6_CCM_CSCDR1_UART_CLK_PODF_MASK    0x0000003F

#define IMX6_SHL_CCM_CCGR0_DTCP                              28
#define IMX6_SHL_CCM_CCGR0_DCIC2                             26
#define IMX6_SHL_CCM_CCGR0_DCIC1                             24
#define IMX6_SHL_CCM_CCGR0_CPU_DEBUG                         22
#define IMX6_SHL_CCM_CCGR0_CAN2_SERIAL                       20
#define IMX6_SHL_CCM_CCGR0_CAN2                              18
#define IMX6_SHL_CCM_CCGR0_CAN1_SERIAL                       16
#define IMX6_SHL_CCM_CCGR0_CAN1                              14
#define IMX6_SHL_CCM_CCGR0_CAAM_WRAPPER_IPG                  12
#define IMX6_SHL_CCM_CCGR0_CAAM_WRAPPER_ACLK                 10
#define IMX6_SHL_CCM_CCGR0_CAAM_SECURE_MEM                    8
#define IMX6_SHL_CCM_CCGR0_ASRC_CLOCK                         6
#define IMX6_SHL_CCM_CCGR0_APBHDMA_HCLK                       4
#define IMX6_SHL_CCM_CCGR0_AIPS_TZ2                           2
#define IMX6_SHL_CCM_CCGR0_AIPS_TZ1                           0
                                                               
#define IMX6_SHL_CCM_CCGR1_GPU3D                             26
#define IMX6_SHL_CCM_CCGR1_GPU2D                             24
#define IMX6_SHL_CCM_CCGR1_GPT_SERIAL                        22
#define IMX6_SHL_CCM_CCGR1_GPT_BUS                           20
#define IMX6_SHL_CCM_CCGR1_ESAI                              16
#define IMX6_SHL_CCM_CCGR1_EPIT2                             14
#define IMX6_SHL_CCM_CCGR1_EPIT1                             12
#define IMX6_SHL_CCM_CCGR1_ECSPI5                             8
#define IMX6_SHL_CCM_CCGR1_ECSPI4                             6
#define IMX6_SHL_CCM_CCGR1_ECSPI3                             4
#define IMX6_SHL_CCM_CCGR1_ECSPI2                             2
#define IMX6_SHL_CCM_CCGR1_ECSPI1                             0
                                                               
#define IMX6_SHL_CCM_CCGR2_IPSYNC_VDOA_IPG                   26
#define IMX6_SHL_CCM_CCGR2_IPSYNC_IP2APB_TZASC2_IPG          24
#define IMX6_SHL_CCM_CCGR2_IPSYNC_IP2APB_TZASC1_IPG          22
#define IMX6_SHL_CCM_CCGR2_IPMUX3                            20
#define IMX6_SHL_CCM_CCGR2_IPMUX2                            18
#define IMX6_SHL_CCM_CCGR2_IPMUX1                            16
#define IMX6_SHL_CCM_CCGR2_IOMUX_IPT_CLK_IO                  14
#define IMX6_SHL_CCM_CCGR2_OCOTP                             12
#define IMX6_SHL_CCM_CCGR2_I2C3                              10
#define IMX6_SHL_CCM_CCGR2_I2C2                               8
#define IMX6_SHL_CCM_CCGR2_I2C1                               6
#define IMX6_SHL_CCM_CCGR2_HDMI_TX_ISFRCLK                    4
#define IMX6_SHL_CCM_CCGR2_HDMI_TX_ENABLE                     0

#define IMX6_SHL_CCM_CCGR3_OPENGVAXICLK                      30                                                               
#define IMX6_SHL_CCM_CCGR3_OCRAM                             28
#define IMX6_SHL_CCM_CCGR3_MMDC_CORE_IPG_CLK_P0              24
#define IMX6_SHL_CCM_CCGR3_MMDC_CORE_ACLK_FAST_CORE_P0       20
#define IMX6_SHL_CCM_CCGR3_MLB                               18
#define IMX6_SHL_CCM_CCGR3_MIPI_CORE_CFG                     16
#define IMX6_SHL_CCM_CCGR3_LDB_DI1                           14
#define IMX6_SHL_CCM_CCGR3_LDB_DI0                           12
#define IMX6_SHL_CCM_CCGR3_IPU2_DI1                          10
#define IMX6_SHL_CCM_CCGR3_IPU2_DI0                           8
#define IMX6_SHL_CCM_CCGR3_IPU2_IPU                           6
#define IMX6_SHL_CCM_CCGR3_IPU1_DI1                           4
#define IMX6_SHL_CCM_CCGR3_IPU1_DI0                           2
#define IMX6_SHL_CCM_CCGR3_IPU1_IPU                           0
                                                               
#define IMX6_SHL_CCM_CCGR4_RAWNAND_U_GPMI_INPUT_APB          30
#define IMX6_SHL_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO  28
#define IMX6_SHL_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_BCH      26
#define IMX6_SHL_CCM_CCGR4_RAWNAND_U_BCH_INPUT_APB           24
#define IMX6_SHL_CCM_CCGR4_PWM4                              22
#define IMX6_SHL_CCM_CCGR4_PWM3                              20
#define IMX6_SHL_CCM_CCGR4_PWM2                              18
#define IMX6_SHL_CCM_CCGR4_PWM1                              16
#define IMX6_SHL_CCM_CCGR4_PL301_MX6QPER2_MAINCLK            14
#define IMX6_SHL_CCM_CCGR4_PL301_MX6QPER1_BCH                12
#define IMX6_SHL_CCM_CCGR4_PL301_MX6QFAST1_S133              8
#define IMX6_SHL_CCM_CCGR4_PCIE                              0
                                                               
#define IMX6_SHL_CCM_CCGR5_UART_SERIAL                       26
#define IMX6_SHL_CCM_CCGR5_UART                              24
#define IMX6_SHL_CCM_CCGR5_SSI3                              22
#define IMX6_SHL_CCM_CCGR5_SSI2                              20
#define IMX6_SHL_CCM_CCGR5_SSI1                              18
#define IMX6_SHL_CCM_CCGR5_SPDIF                             14
#define IMX6_SHL_CCM_CCGR5_SPBA                              12
#define IMX6_SHL_CCM_CCGR5_SDMA                               6
#define IMX6_SHL_CCM_CCGR5_SATA                               4
#define IMX6_SHL_CCM_CCGR5_ROM                                0

#define IMX6_SHL_CCM_CCGR6_VPU                               14
#define IMX6_SHL_CCM_CCGR6_VDOAXICLK                         12                                                              
#define IMX6_SHL_CCM_CCGR6_EIM_SLOW                          10
#define IMX6_SHL_CCM_CCGR6_USDHC4                             8
#define IMX6_SHL_CCM_CCGR6_USDHC3                             6
#define IMX6_SHL_CCM_CCGR6_USDHC2                             4
#define IMX6_SHL_CCM_CCGR6_USDHC1                             2
#define IMX6_SHL_CCM_CCGR6_USBOH3                             0

#define IMX6_CCM_CLOCK_OFF    0
#define IMX6_RUN_ONLY         1
#define IMX6_RUN_AND_WAIT     3

#define IMX6_CCM_CCOSR_CLKO2_EN                           0x01000000
#define IMX6_CCM_CCOSR_CLKO2_DIV_SHL                      21
#define IMX6_CCM_CCOSR_CLKO2_DIV_MASK                     0x00E00000
#define IMX6_CCM_CCOSR_CLKO2_SEL_MASK                     0x001F0000
#define IMX6_CCM_CCOSR_CLKO2_SEL_MMDC_CH0_CLK_ROOT        0x00000000
#define IMX6_CCM_CCOSR_CLKO2_SEL_MMDC_CH1_CLK_ROOT        0x00010000
#define IMX6_CCM_CCOSR_CLKO2_SEL_USDHC4_CLK_ROOT          0x00020000
#define IMX6_CCM_CCOSR_CLKO2_SEL_USDHC1_CLK_ROOT          0x00030000
#define IMX6_CCM_CCOSR_CLKO2_SEL_GPU2D_AXI_CLK_ROOT       0x00040000
#define IMX6_CCM_CCOSR_CLKO2_SEL_WRCK_CLK_ROOT            0x00050000
#define IMX6_CCM_CCOSR_CLKO2_SEL_ECSPI_CLK_ROOT           0x00060000
#define IMX6_CCM_CCOSR_CLKO2_SEL_GPU3D_AXI_CLK_ROOT       0x00070000
#define IMX6_CCM_CCOSR_CLKO2_SEL_USDHC3_CLK_ROOT          0x00080000
#define IMX6_CCM_CCOSR_CLKO2_SEL_125M_CLK_ROOT            0x00090000
#define IMX6_CCM_CCOSR_CLKO2_SEL_ARM_CLK_ROOT             0x000A0000
#define IMX6_CCM_CCOSR_CLKO2_SEL_IPU1_HSP_CLK_ROOT        0x000B0000
#define IMX6_CCM_CCOSR_CLKO2_SEL_IPU2_HSP_CLK_ROOT        0x000C0000
#define IMX6_CCM_CCOSR_CLKO2_SEL_VDO_AXI_CLK_ROOT         0x000D0000
#define IMX6_CCM_CCOSR_CLKO2_SEL_OSC_CLK                  0x000E0000
#define IMX6_CCM_CCOSR_CLKO2_SEL_GPU2D_CORE_CLK_ROOT      0x000F0000
#define IMX6_CCM_CCOSR_CLKO2_SEL_GPU3D_CORE_CLK_ROOT      0x00100000
#define IMX6_CCM_CCOSR_CLKO2_SEL_USDHC2_CLK_ROOT          0x00110000
#define IMX6_CCM_CCOSR_CLKO2_SEL_SSI1_CLK_ROOT            0x00120000
#define IMX6_CCM_CCOSR_CLKO2_SEL_SSI2_CLK_ROOT            0x00130000
#define IMX6_CCM_CCOSR_CLKO2_SEL_SSI3_CLK_ROOT            0x00140000
#define IMX6_CCM_CCOSR_CLKO2_SEL_GPU3D_SHADER_CLK_ROOT    0x00150000
#define IMX6_CCM_CCOSR_CLKO2_SEL_VPU_AXI_CLK_ROOT         0x00160000
#define IMX6_CCM_CCOSR_CLKO2_SEL_CAN_CLK_ROOT             0x00170000
#define IMX6_CCM_CCOSR_CLKO2_SEL_LDB_DI0_SERIAL_CLK_ROOT  0x00180000
#define IMX6_CCM_CCOSR_CLKO2_SEL_LDB_DI1_SERIAL_CLK_ROOT  0x00190000
#define IMX6_CCM_CCOSR_CLKO2_SEL_ESAI_CLK_ROOT            0x001A0000
#define IMX6_CCM_CCOSR_CLKO2_SEL_ACLK_EIM_SLOW_CLK_ROOT   0x001B0000
#define IMX6_CCM_CCOSR_CLKO2_SEL_UART_CLK_ROOT            0x001C0000
#define IMX6_CCM_CCOSR_CLKO2_SEL_SPDIF0_CLK_ROOT          0x001D0000
#define IMX6_CCM_CCOSR_CLKO2_SEL_SPDIF1_CLK_ROOT          0x001E0000
#define IMX6_CCM_CCOSR_CLKO2_SEL_HSI_TX_CLK_ROOT          0x001F0000

#define IMX6_CCM_CCOSR_CLK_OUT_SEL_CLKO1                  0x00000000
#define IMX6_CCM_CCOSR_CLK_OUT_SEL_CLKO2                  0x00000100
#define IMX6_CCM_CCOSR_CLKO1_EN                           0x00000080
#define IMX6_CCM_CCOSR_CLKO1_DIV_SHL                      4
#define IMX6_CCM_CCOSR_CLKO1_DIV_MASK                     0x00000070
#define IMX6_CCM_CCOSR_CLKO_SEL_MASK                      0x0000000F
#define IMX6_CCM_CCOSR_CLKO_SEL_PLL3_SW_CLK               0x00000000
#define IMX6_CCM_CCOSR_CLKO_SEL_PLL2_MAIN_CLK             0x00000001
#define IMX6_CCM_CCOSR_CLKO_SEL_PLL1_MAIN_CLK             0x00000002
#define IMX6_CCM_CCOSR_CLKO_SEL_PLL5_MAIN_CLK             0x00000003
#define IMX6_CCM_CCOSR_CLKO_SEL_VIDEO_27M_CLK_ROOT        0x00000004
#define IMX6_CCM_CCOSR_CLKO_SEL_AXI_CLK_ROOT              0x00000005
#define IMX6_CCM_CCOSR_CLKO_SEL_ENFC_CLK_ROOT             0x00000006
#define IMX6_CCM_CCOSR_CLKO_SEL_IPU1_DI0_CLK_ROOT         0x00000007
#define IMX6_CCM_CCOSR_CLKO_SEL_IPU1_DI1_CLK_ROOT         0x00000008
#define IMX6_CCM_CCOSR_CLKO_SEL_IPU2_DI0_CLK_ROOT         0x00000009
#define IMX6_CCM_CCOSR_CLKO_SEL_IPU2_DI1_CLK_ROOT         0x0000000A
#define IMX6_CCM_CCOSR_CLKO_SEL_AHB_CLK_ROOT              0x0000000B
#define IMX6_CCM_CCOSR_CLKO_SEL_IPG_CLK_ROOT              0x0000000C
#define IMX6_CCM_CCOSR_CLKO_SEL_PERCLK_ROOT               0x0000000D
#define IMX6_CCM_CCOSR_CLKO_SEL_CKIL_SYNC_CLK_ROOT        0x0000000E
#define IMX6_CCM_CCOSR_CLKO_SEL_PLL4_MAIN_CLK             0x0000000F

/* --------------------------------------------------------------------------------- */

#define IMX6_ANALOG_OFFSET_PMU_REG_1P1                  0x0110
#define IMX6_ANALOG_OFFSET_PMU_REG_3P0                  0x0120
#define IMX6_ANALOG_OFFSET_PMU_REG_2P5                  0x0130
#define IMX6_ANALOG_OFFSET_PMU_REG_CORE                 0x0140
#define IMX6_ANALOG_OFFSET_PMU_MISC0                    0x0150
#define IMX6_ANALOG_OFFSET_PMU_MISC1                    0x0160
#define IMX6_ANALOG_OFFSET_PMU_MISC1_SET                0x0164
#define IMX6_ANALOG_OFFSET_PMU_MISC1_CLR                0x0168
#define IMX6_ANALOG_OFFSET_PMU_MISC1_TOG                0x016C
#define IMX6_ANALOG_OFFSET_PMU_MISC2                    0x0170
#define IMX6_ANALOG_OFFSET_PMU_MISC2_SET                0x0174
#define IMX6_ANALOG_OFFSET_PMU_MISC2_CLR                0x0178
#define IMX6_ANALOG_OFFSET_PMU_MISC2_TOG                0x017C

#define IMX6_PHYSADDR_ANALOG_PMU_REG_1P1                (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_REG_1P1   )
#define IMX6_PHYSADDR_ANALOG_PMU_REG_3P0                (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_REG_3P0   )
#define IMX6_PHYSADDR_ANALOG_PMU_REG_2P5                (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_REG_2P5   )
#define IMX6_PHYSADDR_ANALOG_PMU_REG_CORE               (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_REG_CORE  )
#define IMX6_PHYSADDR_ANALOG_PMU_MISC0                  (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_MISC0     )
#define IMX6_PHYSADDR_ANALOG_PMU_MISC1                  (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_MISC1     )
#define IMX6_PHYSADDR_ANALOG_PMU_MISC1_SET              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_MISC1_SET )
#define IMX6_PHYSADDR_ANALOG_PMU_MISC1_CLR              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_MISC1_CLR )
#define IMX6_PHYSADDR_ANALOG_PMU_MISC1_TOG              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_MISC1_TOG )
#define IMX6_PHYSADDR_ANALOG_PMU_MISC2                  (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_MISC2     )
#define IMX6_PHYSADDR_ANALOG_PMU_MISC2_SET              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_MISC2_SET )
#define IMX6_PHYSADDR_ANALOG_PMU_MISC2_CLR              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_MISC2_CLR )
#define IMX6_PHYSADDR_ANALOG_PMU_MISC2_TOG              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PMU_MISC2_TOG )

#define IMX6_ANALOG_PMU_REG_1P1_OK_VDD1P1               0x00020000
#define IMX6_ANALOG_PMU_REG_1P1_OUTPUT_TRG_MASK         0x00001F00
#define IMX6_ANALOG_PMU_REG_1P1_OUTPUT_TRG_1P1          0x00001000
#define IMX6_ANALOG_PMU_REG_1P1_ENABLE_LINREG           0x00000001

#define IMX6_ANALOG_PMU_REG_3P0_OK_VDD3P0               0x00020000
#define IMX6_ANALOG_PMU_REG_3P0_OUTPUT_TRG_MASK         0x00001F00
#define IMX6_ANALOG_PMU_REG_3P0_OUTPUT_TRG_3P0          0x00000F00
#define IMX6_ANALOG_PMU_REG_3P0_VBUS_SEL                0x00000080
#define IMX6_ANALOG_PMU_REG_3P0_ENABLE_LINREG           0x00000001

#define IMX6_ANALOG_PMU_REG_2P5_OK_VDD2P5               0x00020000
#define IMX6_ANALOG_PMU_REG_2P5_OUTPUT_TRG_MASK         0x00001F00
#define IMX6_ANALOG_PMU_REG_2P5_OUTPUT_TRG_2P5          0x00001000
#define IMX6_ANALOG_PMU_REG_2P5_ENABLE_LINREG           0x00000001

#define IMX6_ANALOG_MISC0_REFTOP_VBGUP                  0x00000080
#define IMX6_ANALOG_MISC0_REFTOP_SELFBIASOFF            0x00000008

#define IMX6_ANALOG_OFFSET_PLL_ARM                    0x0000      
#define IMX6_ANALOG_OFFSET_PLL_ARM_SET                0x0004      
#define IMX6_ANALOG_OFFSET_PLL_ARM_CLR                0x0008      
#define IMX6_ANALOG_OFFSET_PLL_ARM_TOG                0x000C      
#define IMX6_ANALOG_OFFSET_PLL_USB1                   0x0010      
#define IMX6_ANALOG_OFFSET_PLL_USB1_SET               0x0014      
#define IMX6_ANALOG_OFFSET_PLL_USB1_CLR               0x0018      
#define IMX6_ANALOG_OFFSET_PLL_USB1_TOG               0x001C      
#define IMX6_ANALOG_OFFSET_PLL_USB2                   0x0020      
#define IMX6_ANALOG_OFFSET_PLL_USB2_SET               0x0024      
#define IMX6_ANALOG_OFFSET_PLL_USB2_CLR               0x0028      
#define IMX6_ANALOG_OFFSET_PLL_USB2_TOG               0x002C      
#define IMX6_ANALOG_OFFSET_PLL_SYS                    0x0030      
#define IMX6_ANALOG_OFFSET_PLL_SYS_SET                0x0034      
#define IMX6_ANALOG_OFFSET_PLL_SYS_CLR                0x0038      
#define IMX6_ANALOG_OFFSET_PLL_SYS_TOG                0x003C      
#define IMX6_ANALOG_OFFSET_PLL_SYS_SS                 0x0040      
#define IMX6_ANALOG_OFFSET_PLL_AUDIO                  0x0070      
#define IMX6_ANALOG_OFFSET_PLL_AUDIO_SET              0x0074      
#define IMX6_ANALOG_OFFSET_PLL_AUDIO_CLR              0x0078      
#define IMX6_ANALOG_OFFSET_PLL_AUDIO_TOG              0x007C      
#define IMX6_ANALOG_OFFSET_PLL_AUDIO_NUM              0x0080      
#define IMX6_ANALOG_OFFSET_PLL_AUDIO_DENOM            0x0090      
#define IMX6_ANALOG_OFFSET_PLL_VIDEO                  0x00A0      
#define IMX6_ANALOG_OFFSET_PLL_VIDEO_SET              0x00A4      
#define IMX6_ANALOG_OFFSET_PLL_VIDEO_CLR              0x00A8      
#define IMX6_ANALOG_OFFSET_PLL_VIDEO_TOG              0x00AC      
#define IMX6_ANALOG_OFFSET_PLL_VIDEO_NUM              0x00B0      
#define IMX6_ANALOG_OFFSET_PLL_VIDEO_DENOM            0x00C0      
#define IMX6_ANALOG_OFFSET_PLL_MLB                    0x00D0 
#define IMX6_ANALOG_OFFSET_PLL_MLB_SET                0x00D4 
#define IMX6_ANALOG_OFFSET_PLL_MLB_CLR                0x00D8 
#define IMX6_ANALOG_OFFSET_PLL_MLB_TOG                0x00DC 
#define IMX6_ANALOG_OFFSET_PLL_ENET                   0x00E0      
#define IMX6_ANALOG_OFFSET_PLL_ENET_SET               0x00E4      
#define IMX6_ANALOG_OFFSET_PLL_ENET_CLR               0x00E8      
#define IMX6_ANALOG_OFFSET_PLL_ENET_TOG               0x00EC      
#define IMX6_ANALOG_OFFSET_PFD_480                    0x00F0      
#define IMX6_ANALOG_OFFSET_PFD_480_SET                0x00F4      
#define IMX6_ANALOG_OFFSET_PFD_480_CLR                0x00F8      
#define IMX6_ANALOG_OFFSET_PFD_480_TOG                0x00FC      
#define IMX6_ANALOG_OFFSET_PFD_528                    0x0100      
#define IMX6_ANALOG_OFFSET_PFD_528_SET                0x0104      
#define IMX6_ANALOG_OFFSET_PFD_528_CLR                0x0108      
#define IMX6_ANALOG_OFFSET_PFD_528_TOG                0x010C      

#define IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE0         0x0180  
#define IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE0_SET     0x0184 
#define IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE0_CLR     0x0188 
#define IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE0_TOG     0x018C 
#define IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE1         0x0190 
#define IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE1_SET     0x0194 
#define IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE1_CLR     0x0198 
#define IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE1_TOG     0x019C 
#define IMX6_ANALOG_OFFSET_USB1_VBUS_DETECT           0x01A0 
#define IMX6_ANALOG_OFFSET_USB1_VBUS_DETECT_SET       0x01A4 
#define IMX6_ANALOG_OFFSET_USB1_VBUS_DETECT_CLR       0x01A8 
#define IMX6_ANALOG_OFFSET_USB1_VBUS_DETECT_TOG       0x01AC 
#define IMX6_ANALOG_OFFSET_USB1_CHRG_DETECT           0x01B0 
#define IMX6_ANALOG_OFFSET_USB1_CHRG_DETECT_SET       0x01B4 
#define IMX6_ANALOG_OFFSET_USB1_CHRG_DETECT_CLR       0x01B8 
#define IMX6_ANALOG_OFFSET_USB1_CHRG_DETECT_TOG       0x01BC 
#define IMX6_ANALOG_OFFSET_USB1_VBUS_DETECT_STAT      0x01C0 
#define IMX6_ANALOG_OFFSET_USB1_CHRG_DETECT_STAT      0x01D0 
#define IMX6_ANALOG_OFFSET_USB1_MISC                  0x01F0 
#define IMX6_ANALOG_OFFSET_USB1_MISC_SET              0x01F4 
#define IMX6_ANALOG_OFFSET_USB1_MISC_CLR              0x01F8 
#define IMX6_ANALOG_OFFSET_USB1_MISC_TOG              0x01FC 
#define IMX6_ANALOG_OFFSET_USB2_VBUS_DETECT           0x0200 
#define IMX6_ANALOG_OFFSET_USB2_VBUS_DETECT_SET       0x0204 
#define IMX6_ANALOG_OFFSET_USB2_VBUS_DETECT_CLR       0x0208 
#define IMX6_ANALOG_OFFSET_USB2_VBUS_DETECT_TOG       0x020C 
#define IMX6_ANALOG_OFFSET_USB2_CHRG_DETECT           0x0210 
#define IMX6_ANALOG_OFFSET_USB2_CHRG_DETECT_SET       0x0214 
#define IMX6_ANALOG_OFFSET_USB2_CHRG_DETECT_CLR       0x0218 
#define IMX6_ANALOG_OFFSET_USB2_CHRG_DETECT_TOG       0x021C 
#define IMX6_ANALOG_OFFSET_USB2_VBUS_DETECT_STAT      0x0220 
#define IMX6_ANALOG_OFFSET_USB2_CHRG_DETECT_STAT      0x0230 
#define IMX6_ANALOG_OFFSET_USB2_MISC                  0x0250 
#define IMX6_ANALOG_OFFSET_USB2_MISC_SET              0x0254 
#define IMX6_ANALOG_OFFSET_USB2_MISC_CLR              0x0258 
#define IMX6_ANALOG_OFFSET_USB2_MISC_TOG              0x025C 
#define IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE2         0x0290 
#define IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE2_SET     0x0294 
#define IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE2_CLR     0x0298 
#define IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE2_TOG     0x029C 

#define IMX6_PHYSADDR_ANALOG_PLL_ARM                  (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_ARM        )
#define IMX6_PHYSADDR_ANALOG_PLL_ARM_SET              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_ARM_SET    )
#define IMX6_PHYSADDR_ANALOG_PLL_ARM_CLR              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_ARM_CLR    )
#define IMX6_PHYSADDR_ANALOG_PLL_ARM_TOG              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_ARM_TOG    )
#define IMX6_PHYSADDR_ANALOG_PLL_USB1                 (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_USB1       )
#define IMX6_PHYSADDR_ANALOG_PLL_USB1_SET             (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_USB1_SET   )
#define IMX6_PHYSADDR_ANALOG_PLL_USB1_CLR             (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_USB1_CLR   )
#define IMX6_PHYSADDR_ANALOG_PLL_USB1_TOG             (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_USB1_TOG   )
#define IMX6_PHYSADDR_ANALOG_PLL_USB2                 (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_USB2       )
#define IMX6_PHYSADDR_ANALOG_PLL_USB2_SET             (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_USB2_SET   )
#define IMX6_PHYSADDR_ANALOG_PLL_USB2_CLR             (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_USB2_CLR   )
#define IMX6_PHYSADDR_ANALOG_PLL_USB2_TOG             (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_USB2_TOG   )
#define IMX6_PHYSADDR_ANALOG_PLL_SYS                  (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_SYS        )
#define IMX6_PHYSADDR_ANALOG_PLL_SYS_SET              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_SYS_SET    )
#define IMX6_PHYSADDR_ANALOG_PLL_SYS_CLR              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_SYS_CLR    )
#define IMX6_PHYSADDR_ANALOG_PLL_SYS_TOG              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_SYS_TOG    )
#define IMX6_PHYSADDR_ANALOG_PLL_SYS_SS               (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_SYS_SS     )
#define IMX6_PHYSADDR_ANALOG_PLL_AUDIO                (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_AUDIO      )
#define IMX6_PHYSADDR_ANALOG_PLL_AUDIO_SET            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_AUDIO_SET  )
#define IMX6_PHYSADDR_ANALOG_PLL_AUDIO_CLR            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_AUDIO_CLR  )
#define IMX6_PHYSADDR_ANALOG_PLL_AUDIO_TOG            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_AUDIO_TOG  )
#define IMX6_PHYSADDR_ANALOG_PLL_AUDIO_NUM            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_AUDIO_NUM  )
#define IMX6_PHYSADDR_ANALOG_PLL_AUDIO_DENOM          (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_AUDIO_DENOM)
#define IMX6_PHYSADDR_ANALOG_PLL_VIDEO                (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_VIDEO      )
#define IMX6_PHYSADDR_ANALOG_PLL_VIDEO_SET            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_VIDEO_SET  )
#define IMX6_PHYSADDR_ANALOG_PLL_VIDEO_CLR            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_VIDEO_CLR  )
#define IMX6_PHYSADDR_ANALOG_PLL_VIDEO_TOG            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_VIDEO_TOG  )
#define IMX6_PHYSADDR_ANALOG_PLL_VIDEO_NUM            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_VIDEO_NUM  )
#define IMX6_PHYSADDR_ANALOG_PLL_VIDEO_DENOM          (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_VIDEO_DENOM)
#define IMX6_PHYSADDR_ANALOG_PLL_MLB                  (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_MLB    )
#define IMX6_PHYSADDR_ANALOG_PLL_MLB_SET              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_MLB_SET)
#define IMX6_PHYSADDR_ANALOG_PLL_MLB_CLR              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_MLB_CLR)
#define IMX6_PHYSADDR_ANALOG_PLL_MLB_TOG              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_MLB_TOG)
#define IMX6_PHYSADDR_ANALOG_PLL_ENET                 (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_ENET       )
#define IMX6_PHYSADDR_ANALOG_PLL_ENET_SET             (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_ENET_SET   )
#define IMX6_PHYSADDR_ANALOG_PLL_ENET_CLR             (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_ENET_CLR   )
#define IMX6_PHYSADDR_ANALOG_PLL_ENET_TOG             (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PLL_ENET_TOG   )
#define IMX6_PHYSADDR_ANALOG_PFD_480                  (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PFD_480        )
#define IMX6_PHYSADDR_ANALOG_PFD_480_SET              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PFD_480_SET    )
#define IMX6_PHYSADDR_ANALOG_PFD_480_CLR              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PFD_480_CLR    )
#define IMX6_PHYSADDR_ANALOG_PFD_480_TOG              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PFD_480_TOG    )
#define IMX6_PHYSADDR_ANALOG_PFD_528                  (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PFD_528        )
#define IMX6_PHYSADDR_ANALOG_PFD_528_SET              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PFD_528_SET    )
#define IMX6_PHYSADDR_ANALOG_PFD_528_CLR              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PFD_528_CLR    )
#define IMX6_PHYSADDR_ANALOG_PFD_528_TOG              (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_PFD_528_TOG    )

#define IMX6_PHYSADDR_ANALOG_TEMPMON_TEMPSENSE0       (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE0     )
#define IMX6_PHYSADDR_ANALOG_TEMPMON_TEMPSENSE0_SET   (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE0_SET )
#define IMX6_PHYSADDR_ANALOG_TEMPMON_TEMPSENSE0_CLR   (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE0_CLR )
#define IMX6_PHYSADDR_ANALOG_TEMPMON_TEMPSENSE0_TOG   (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE0_TOG )
#define IMX6_PHYSADDR_ANALOG_TEMPMON_TEMPSENSE1       (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE1     )
#define IMX6_PHYSADDR_ANALOG_TEMPMON_TEMPSENSE1_SET   (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE1_SET )
#define IMX6_PHYSADDR_ANALOG_TEMPMON_TEMPSENSE1_CLR   (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE1_CLR )
#define IMX6_PHYSADDR_ANALOG_TEMPMON_TEMPSENSE1_TOG   (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE1_TOG )
#define IMX6_PHYSADDR_ANALOG_USB1_VBUS_DETECT         (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_VBUS_DETECT      )
#define IMX6_PHYSADDR_ANALOG_USB1_VBUS_DETECT_SET     (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_VBUS_DETECT_SET  )
#define IMX6_PHYSADDR_ANALOG_USB1_VBUS_DETECT_CLR     (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_VBUS_DETECT_CLR  )
#define IMX6_PHYSADDR_ANALOG_USB1_VBUS_DETECT_TOG     (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_VBUS_DETECT_TOG  )
#define IMX6_PHYSADDR_ANALOG_USB1_CHRG_DETECT         (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_CHRG_DETECT      )
#define IMX6_PHYSADDR_ANALOG_USB1_CHRG_DETECT_SET     (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_CHRG_DETECT_SET  )
#define IMX6_PHYSADDR_ANALOG_USB1_CHRG_DETECT_CLR     (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_CHRG_DETECT_CLR  )
#define IMX6_PHYSADDR_ANALOG_USB1_CHRG_DETECT_TOG     (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_CHRG_DETECT_TOG  )
#define IMX6_PHYSADDR_ANALOG_USB1_VBUS_DETECT_STAT    (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_VBUS_DETECT_STAT )
#define IMX6_PHYSADDR_ANALOG_USB1_CHRG_DETECT_STAT    (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_CHRG_DETECT_STAT )
#define IMX6_PHYSADDR_ANALOG_USB1_MISC                (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_MISC             )
#define IMX6_PHYSADDR_ANALOG_USB1_MISC_SET            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_MISC_SET         )
#define IMX6_PHYSADDR_ANALOG_USB1_MISC_CLR            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_MISC_CLR         )
#define IMX6_PHYSADDR_ANALOG_USB1_MISC_TOG            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB1_MISC_TOG         )
#define IMX6_PHYSADDR_ANALOG_USB2_VBUS_DETECT         (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_VBUS_DETECT      )
#define IMX6_PHYSADDR_ANALOG_USB2_VBUS_DETECT_SET     (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_VBUS_DETECT_SET  )
#define IMX6_PHYSADDR_ANALOG_USB2_VBUS_DETECT_CLR     (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_VBUS_DETECT_CLR  )
#define IMX6_PHYSADDR_ANALOG_USB2_VBUS_DETECT_TOG     (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_VBUS_DETECT_TOG  )
#define IMX6_PHYSADDR_ANALOG_USB2_CHRG_DETECT         (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_CHRG_DETECT      )
#define IMX6_PHYSADDR_ANALOG_USB2_CHRG_DETECT_SET     (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_CHRG_DETECT_SET  )
#define IMX6_PHYSADDR_ANALOG_USB2_CHRG_DETECT_CLR     (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_CHRG_DETECT_CLR  )
#define IMX6_PHYSADDR_ANALOG_USB2_CHRG_DETECT_TOG     (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_CHRG_DETECT_TOG  )
#define IMX6_PHYSADDR_ANALOG_USB2_VBUS_DETECT_STAT    (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_VBUS_DETECT_STAT )
#define IMX6_PHYSADDR_ANALOG_USB2_CHRG_DETECT_STAT    (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_CHRG_DETECT_STAT )
#define IMX6_PHYSADDR_ANALOG_USB2_MISC                (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_MISC             )
#define IMX6_PHYSADDR_ANALOG_USB2_MISC_SET            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_MISC_SET         )
#define IMX6_PHYSADDR_ANALOG_USB2_MISC_CLR            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_MISC_CLR         )
#define IMX6_PHYSADDR_ANALOG_USB2_MISC_TOG            (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_USB2_MISC_TOG         )
#define IMX6_PHYSADDR_ANALOG_TEMPMON_TEMPSENSE2       (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE2     )
#define IMX6_PHYSADDR_ANALOG_TEMPMON_TEMPSENSE2_SET   (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE2_SET )
#define IMX6_PHYSADDR_ANALOG_TEMPMON_TEMPSENSE2_CLR   (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE2_CLR )
#define IMX6_PHYSADDR_ANALOG_TEMPMON_TEMPSENSE2_TOG   (IMX6_PHYSADDR_ANALOG + IMX6_ANALOG_OFFSET_TEMPMON_TEMPSENSE2_TOG )

#define IMX6_ANALOG_USB_CHRG_DETECT_EN_B              0x00100000
#define IMX6_ANALOG_USB_CHK_CHRG_B                    0x00080000
#define IMX6_ANALOG_USB_CHK_CONTACT                   0x00040000

#define IMX6_ANALOG_PLL_USB_LOCK                      0x80000000
#define IMX6_ANALOG_PLL_USB_BYPASS                    0x00010000
#define IMX6_ANALOG_PLL_USB_BYPASS_CLK_SRC_MASK       0x0000C000
#define IMX6_ANALOG_PLL_USB_BYPASS_CLK_SRC_REF_24MHZ  0x00000000
#define IMX6_ANALOG_PLL_USB_BYPASS_CLK_SRC_CLK1       0x00004000
#define IMX6_ANALOG_PLL_USB_BYPASS_CLK_SRC_CLK2       0x00008000
#define IMX6_ANALOG_PLL_USB_BYPASS_CLK_SRC_XOR        0x0000C000
#define IMX6_ANALOG_PLL_USB_ENABLE                    0x00002000
#define IMX6_ANALOG_PLL_USB_POWER                     0x00001000
#define IMX6_ANALOG_PLL_USB_EN_USB_CLKS               0x00000040
#define IMX6_ANALOG_PLL_USB_DIV_SELECT                0x00000001

#define IMX6_ANALOG_MISC0_REFTOP_VBGUP                0x00000080
#define IMX6_ANALOG_MISC0_REFTOP_SELFBIASOFF          0x00000008

#define IMX6_ANALOG_PLL_ARM_DIV_SELECT_MASK           0x0000007F

#define IMX6_ANALOG_TEMPMON_TEMPSENSE0_ALARM_VALUE_MASK       0xFFF00000
#define IMX6_ANALOG_TEMPMON_TEMPSENSE0_ALARM_VALUE_SHL        20
#define IMX6_ANALOG_TEMPMON_TEMPSENSE0_TEMP_CNT_MASK          0x000FFF00
#define IMX6_ANALOG_TEMPMON_TEMPSENSE0_TEMP_CNT_SHL           8
#define IMX6_ANALOG_TEMPMON_TEMPSENSE0_FINISHED               0x00000004
#define IMX6_ANALOG_TEMPMON_TEMPSENSE0_MEASURE_TEMP           0x00000002
#define IMX6_ANALOG_TEMPMON_TEMPSENSE0_POWER_DOWN             0x00000001

#define IMX6_ANALOG_TEMPMON_TEMPSENSE1_MEASURE_FREQ_MASK      0x0000FFFF

#define IMX6_ANALOG_TEMPMON_TEMPSENSE2_PANIC_ALARM_VALUE_MASK 0x0FFF0000
#define IMX6_ANALOG_TEMPMON_TEMPSENSE2_PANIC_ALARM_VALUE_SHL  16
#define IMX6_ANALOG_TEMPMON_TEMPSENSE2_LOW_ALARM_VALUE_MASK   0x00000FFF

#define IMX6_ANALOG_PLL_VIDEO_LOCK                    0x80000000
#define IMX6_ANALOG_PLL_VIDEO_POST_DIV_SELECT_MASK    0x00180000
#define IMX6_ANALOG_PLL_VIDEO_POST_DIV_SELECT_SHL     19
#define IMX6_ANALOG_PLL_VIDEO_PFD_OFFSET_EN           0x00040000
#define IMX6_ANALOG_PLL_VIDEO_BYPASS                  0x00010000
#define IMX6_ANALOG_PLL_VIDEO_BYPASS_CLK_SRC_MASK     0x0000C000
#define IMX6_ANALOG_PLL_VIDEO_BYPASS_CLK_24M          0x00000000
#define IMX6_ANALOG_PLL_VIDEO_BYPASS_CLK_CLK1         0x00004000
#define IMX6_ANALOG_PLL_VIDEO_ENABLE                  0x00002000
#define IMX6_ANALOG_PLL_VIDEO_POWERDOWN               0x00001000
#define IMX6_ANALOG_PLL_VIDEO_DIV_SELECT_MASK         0x0000007F

#define IMX6_ANALOG_PLL_ENET_LOCK                     0x80000000
#define IMX6_ANALOG_PLL_ENET_ENET_25M_REF_EN          0x00200000
#define IMX6_ANALOG_PLL_ENET_ENET2_125M_REF_EN        0x00100000
#define IMX6_ANALOG_PLL_ENET_ENABLE_125M              0x00080000
#define IMX6_ANALOG_PLL_ENET_PFD_OFFSET_EN            0x00040000
#define IMX6_ANALOG_PLL_ENET_BYPASS                   0x00010000
#define IMX6_ANALOG_PLL_ENET_BYPASS_CLK_SRC_MASK      0x0000C000
#define IMX6_ANALOG_PLL_ENET_BYPASS_CLK_SRC_24M       0x00000000
#define IMX6_ANALOG_PLL_ENET_BYPASS_CLK_SRC_CLK1      0x00004000
#define IMX6_ANALOG_PLL_ENET_ENET1_125M_REF_EN        0x00002000
#define IMX6_ANALOG_PLL_ENET_POWERDOWN                0x00001000
#define IMX6_ANALOG_PLL_ENET_PLL1_DIV_SELECT_MASK     0x0000000C
#define IMX6_ANALOG_PLL_ENET_PLL1_DIV_SELECT_25M      0x00000000
#define IMX6_ANALOG_PLL_ENET_PLL1_DIV_SELECT_50M      0x00000004
#define IMX6_ANALOG_PLL_ENET_PLL1_DIV_SELECT_100M     0x00000008
#define IMX6_ANALOG_PLL_ENET_PLL1_DIV_SELECT_125M     0x0000000C 
#define IMX6_ANALOG_PLL_ENET_PLL0_DIV_SELECT_MASK     0x00000003
#define IMX6_ANALOG_PLL_ENET_PLL0_DIV_SELECT_25M      0x00000000
#define IMX6_ANALOG_PLL_ENET_PLL0_DIV_SELECT_50M      0x00000001
#define IMX6_ANALOG_PLL_ENET_PLL0_DIV_SELECT_100M     0x00000002
#define IMX6_ANALOG_PLL_ENET_PLL0_DIV_SELECT_125M     0x00000003 

/* --------------------------------------------------------------------------------- */

#define IMX6_WDOG_OFFSET_WCR      0x0000
#define IMX6_WDOG_OFFSET_WSR      0x0002
#define IMX6_WDOG_OFFSET_WRSR     0x0004
#define IMX6_WDOG_OFFSET_WICR     0x0006
#define IMX6_WDOG_OFFSET_WMCR     0x0008

#define IMX6_PHYSADDR_WDOG1_WCR   (IMX6_PHYSADDR_WDOG1 + IMX6_WDOG_OFFSET_WCR  )
#define IMX6_PHYSADDR_WDOG1_WSR   (IMX6_PHYSADDR_WDOG1 + IMX6_WDOG_OFFSET_WSR  )
#define IMX6_PHYSADDR_WDOG1_WRSR  (IMX6_PHYSADDR_WDOG1 + IMX6_WDOG_OFFSET_WRSR )
#define IMX6_PHYSADDR_WDOG1_WICR  (IMX6_PHYSADDR_WDOG1 + IMX6_WDOG_OFFSET_WICR )
#define IMX6_PHYSADDR_WDOG1_WMCR  (IMX6_PHYSADDR_WDOG1 + IMX6_WDOG_OFFSET_WMCR )

#define IMX6_PHYSADDR_WDOG2_WCR   (IMX6_PHYSADDR_WDOG2 + IMX6_WDOG_OFFSET_WCR  )
#define IMX6_PHYSADDR_WDOG2_WSR   (IMX6_PHYSADDR_WDOG2 + IMX6_WDOG_OFFSET_WSR  )
#define IMX6_PHYSADDR_WDOG2_WRSR  (IMX6_PHYSADDR_WDOG2 + IMX6_WDOG_OFFSET_WRSR )
#define IMX6_PHYSADDR_WDOG2_WICR  (IMX6_PHYSADDR_WDOG2 + IMX6_WDOG_OFFSET_WICR )
#define IMX6_PHYSADDR_WDOG2_WMCR  (IMX6_PHYSADDR_WDOG2 + IMX6_WDOG_OFFSET_WMCR )

// 16 bit registers
#define IMX6_WDOG_WMCR_PDE        0x0001

/* --------------------------------------------------------------------------------- */

#define IMX6_APBH_OFFSET_CTRL0                0x0000      
#define IMX6_APBH_OFFSET_CTRL0_SET            0x0004      
#define IMX6_APBH_OFFSET_CTRL0_CLR            0x0008      
#define IMX6_APBH_OFFSET_CTRL0_TOG            0x000C      
#define IMX6_APBH_OFFSET_CTRL1                0x0010      
#define IMX6_APBH_OFFSET_CTRL1_SET            0x0014      
#define IMX6_APBH_OFFSET_CTRL1_CLR            0x0018      
#define IMX6_APBH_OFFSET_CTRL1_TOG            0x001C      
#define IMX6_APBH_OFFSET_CTRL2                0x0020      
#define IMX6_APBH_OFFSET_CTRL2_SET            0x0024      
#define IMX6_APBH_OFFSET_CTRL2_CLR            0x0028      
#define IMX6_APBH_OFFSET_CTRL2_TOG            0x002C      
#define IMX6_APBH_OFFSET_CHANNEL_CTRL         0x0030      
#define IMX6_APBH_OFFSET_CHANNEL_CTRL_SET     0x0034      
#define IMX6_APBH_OFFSET_CHANNEL_CTRL_CLR     0x0038      
#define IMX6_APBH_OFFSET_CHANNEL_CTRL_TOG     0x003C      
#define IMX6_APBH_OFFSET_DEVSEL               0x0040      
#define IMX6_APBH_OFFSET_DMA_BURST_SIZE       0x0050      
#define IMX6_APBH_OFFSET_DEBUG                0x0060      
#define IMX6_APBH_OFFSET_CH0                  0x0100      
#define IMX6_APBH_OFFSET_VERSION              0x0800

#define IMX6_PHYSADDR_APBH_CTRL0              (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CTRL0           )
#define IMX6_PHYSADDR_APBH_CTRL0_SET          (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CTRL0_SET       )
#define IMX6_PHYSADDR_APBH_CTRL0_CLR          (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CTRL0_CLR       )
#define IMX6_PHYSADDR_APBH_CTRL0_TOG          (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CTRL0_TOG       )
#define IMX6_PHYSADDR_APBH_CTRL1              (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CTRL1           )
#define IMX6_PHYSADDR_APBH_CTRL1_SET          (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CTRL1_SET       )
#define IMX6_PHYSADDR_APBH_CTRL1_CLR          (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CTRL1_CLR       )
#define IMX6_PHYSADDR_APBH_CTRL1_TOG          (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CTRL1_TOG       )
#define IMX6_PHYSADDR_APBH_CTRL2              (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CTRL2           )
#define IMX6_PHYSADDR_APBH_CTRL2_SET          (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CTRL2_SET       )
#define IMX6_PHYSADDR_APBH_CTRL2_CLR          (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CTRL2_CLR       )
#define IMX6_PHYSADDR_APBH_CTRL2_TOG          (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CTRL2_TOG       )
#define IMX6_PHYSADDR_APBH_CHANNEL_CTRL       (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CHANNEL_CTRL    )
#define IMX6_PHYSADDR_APBH_CHANNEL_CTRL_SET   (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CHANNEL_CTRL_SET)
#define IMX6_PHYSADDR_APBH_CHANNEL_CTRL_CLR   (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CHANNEL_CTRL_CLR)
#define IMX6_PHYSADDR_APBH_CHANNEL_CTRL_TOG   (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CHANNEL_CTRL_TOG)
#define IMX6_PHYSADDR_APBH_DEVSEL             (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_DEVSEL          )
#define IMX6_PHYSADDR_APBH_DMA_BURST_SIZE     (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_DMA_BURST_SIZE  )
#define IMX6_PHYSADDR_APBH_DEBUG              (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_DEBUG           )
#define IMX6_PHYSADDR_APBH_CH0                (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_CH0             )
#define IMX6_PHYSADDR_APBH_VERSION            (IMX6_PHYSADDR_APBH_DMA + IMX6_APBH_OFFSET_VERSION         )

#define IMX6_APBH_CH_OFFSET_CURCMDAR          0x0000
#define IMX6_APBH_CH_OFFSET_NXTCMDAR          0x0010
#define IMX6_APBH_CH_OFFSET_CMD               0x0020
#define IMX6_APBH_CH_OFFSET_BAR               0x0030
#define IMX6_APBH_CH_OFFSET_SEMA              0x0040
#define IMX6_APBH_CH_OFFSET_DEBUG1            0x0050
#define IMX6_APBH_CH_OFFSET_DEBUG2            0x0060
#define IMX6_APBH_CH_REGS_BYTES               0x0070

#define IMX6_APBH_OFFSET_CH(x)                (IMX6_APBH_OFFSET_CH0 + (IMX6_APBH_CH_REGS_BYTES * (x)))

#define IMX6_PHYSADDR_APBH_CH_CURCMDAR(x)     (IMX6_APBH_OFFSET_CH(x) + IMX6_APBH_CH_OFFSET_CURCMDAR)
#define IMX6_PHYSADDR_APBH_CH_NXTCMDAR(x)     (IMX6_APBH_OFFSET_CH(x) + IMX6_APBH_CH_OFFSET_NXTCMDAR)
#define IMX6_PHYSADDR_APBH_CH_CMD(x)          (IMX6_APBH_OFFSET_CH(x) + IMX6_APBH_CH_OFFSET_CMD     )
#define IMX6_PHYSADDR_APBH_CH_BAR(x)          (IMX6_APBH_OFFSET_CH(x) + IMX6_APBH_CH_OFFSET_BAR     )
#define IMX6_PHYSADDR_APBH_CH_SEMA(x)         (IMX6_APBH_OFFSET_CH(x) + IMX6_APBH_CH_OFFSET_SEMA    )
#define IMX6_PHYSADDR_APBH_CH_DEBUG1(x)       (IMX6_APBH_OFFSET_CH(x) + IMX6_APBH_CH_OFFSET_DEBUG1  )
#define IMX6_PHYSADDR_APBH_CH_DEBUG2(x)       (IMX6_APBH_OFFSET_CH(x) + IMX6_APBH_CH_OFFSET_DEBUG2  )

#define IMX6_APBH_DMA_CTRL0_SFTRST            0x80000000
#define IMX6_APBH_DMA_CTRL0_CLKGATE           0x40000000
#define IMX6_APBH_DMA_CTRL0_BURST8_EN         0x20000000
#define IMX6_APBH_DMA_CTRL0_BURST_EN          0x10000000
#define IMX6_APBH_DMA_CTRL0_CLKGATE_CHMASK    0x0000FFFF

/* --------------------------------------------------------------------------------- */

#define IMX6_UART_OFFSET_URXD                 0x0000 
#define IMX6_UART_OFFSET_UTXD                 0x0040 
#define IMX6_UART_OFFSET_UCR1                 0x0080 
#define IMX6_UART_OFFSET_UCR2                 0x0084 
#define IMX6_UART_OFFSET_UCR3                 0x0088 
#define IMX6_UART_OFFSET_UCR4                 0x008C 
#define IMX6_UART_OFFSET_UFCR                 0x0090 
#define IMX6_UART_OFFSET_USR1                 0x0094 
#define IMX6_UART_OFFSET_USR2                 0x0098 
#define IMX6_UART_OFFSET_UESC                 0x009C 
#define IMX6_UART_OFFSET_UTIM                 0x00A0 
#define IMX6_UART_OFFSET_UBIR                 0x00A4 
#define IMX6_UART_OFFSET_UBMR                 0x00A8 
#define IMX6_UART_OFFSET_UBRC                 0x00AC 
#define IMX6_UART_OFFSET_ONEMS                0x00B0 
#define IMX6_UART_OFFSET_UTS                  0x00B4 
#define IMX6_UART_OFFSET_UMCR                 0x00B8 

#define IMX6_PHYSADDR_UART1_URXD              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_URXD )
#define IMX6_PHYSADDR_UART1_UTXD              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UTXD )
#define IMX6_PHYSADDR_UART1_UCR1              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UCR1 )
#define IMX6_PHYSADDR_UART1_UCR2              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UCR2 )
#define IMX6_PHYSADDR_UART1_UCR3              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UCR3 )
#define IMX6_PHYSADDR_UART1_UCR4              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UCR4 )
#define IMX6_PHYSADDR_UART1_UFCR              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UFCR )
#define IMX6_PHYSADDR_UART1_USR1              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_USR1 )
#define IMX6_PHYSADDR_UART1_USR2              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_USR2 )
#define IMX6_PHYSADDR_UART1_UESC              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UESC )
#define IMX6_PHYSADDR_UART1_UTIM              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UTIM )
#define IMX6_PHYSADDR_UART1_UBIR              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UBIR )
#define IMX6_PHYSADDR_UART1_UBMR              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UBMR )
#define IMX6_PHYSADDR_UART1_UBRC              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UBRC )
#define IMX6_PHYSADDR_UART1_ONEMS             (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_ONEMS)
#define IMX6_PHYSADDR_UART1_UTS               (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UTS  )
#define IMX6_PHYSADDR_UART1_UMCR              (IMX6_PHYSADDR_UART1 + IMX6_UART_OFFSET_UMCR )

#define IMX6_PHYSADDR_UART2_URXD              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_URXD )
#define IMX6_PHYSADDR_UART2_UTXD              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UTXD )
#define IMX6_PHYSADDR_UART2_UCR1              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UCR1 )
#define IMX6_PHYSADDR_UART2_UCR2              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UCR2 )
#define IMX6_PHYSADDR_UART2_UCR3              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UCR3 )
#define IMX6_PHYSADDR_UART2_UCR4              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UCR4 )
#define IMX6_PHYSADDR_UART2_UFCR              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UFCR )
#define IMX6_PHYSADDR_UART2_USR1              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_USR1 )
#define IMX6_PHYSADDR_UART2_USR2              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_USR2 )
#define IMX6_PHYSADDR_UART2_UESC              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UESC )
#define IMX6_PHYSADDR_UART2_UTIM              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UTIM )
#define IMX6_PHYSADDR_UART2_UBIR              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UBIR )
#define IMX6_PHYSADDR_UART2_UBMR              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UBMR )
#define IMX6_PHYSADDR_UART2_UBRC              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UBRC )
#define IMX6_PHYSADDR_UART2_ONEMS             (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_ONEMS)
#define IMX6_PHYSADDR_UART2_UTS               (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UTS  )
#define IMX6_PHYSADDR_UART2_UMCR              (IMX6_PHYSADDR_UART2 + IMX6_UART_OFFSET_UMCR )

#define IMX6_PHYSADDR_UART3_URXD              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_URXD )
#define IMX6_PHYSADDR_UART3_UTXD              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UTXD )
#define IMX6_PHYSADDR_UART3_UCR1              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UCR1 )
#define IMX6_PHYSADDR_UART3_UCR2              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UCR2 )
#define IMX6_PHYSADDR_UART3_UCR3              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UCR3 )
#define IMX6_PHYSADDR_UART3_UCR4              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UCR4 )
#define IMX6_PHYSADDR_UART3_UFCR              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UFCR )
#define IMX6_PHYSADDR_UART3_USR1              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_USR1 )
#define IMX6_PHYSADDR_UART3_USR2              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_USR2 )
#define IMX6_PHYSADDR_UART3_UESC              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UESC )
#define IMX6_PHYSADDR_UART3_UTIM              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UTIM )
#define IMX6_PHYSADDR_UART3_UBIR              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UBIR )
#define IMX6_PHYSADDR_UART3_UBMR              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UBMR )
#define IMX6_PHYSADDR_UART3_UBRC              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UBRC )
#define IMX6_PHYSADDR_UART3_ONEMS             (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_ONEMS)
#define IMX6_PHYSADDR_UART3_UTS               (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UTS  )
#define IMX6_PHYSADDR_UART3_UMCR              (IMX6_PHYSADDR_UART3 + IMX6_UART_OFFSET_UMCR )

#define IMX6_PHYSADDR_UART4_URXD              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_URXD )
#define IMX6_PHYSADDR_UART4_UTXD              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UTXD )
#define IMX6_PHYSADDR_UART4_UCR1              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UCR1 )
#define IMX6_PHYSADDR_UART4_UCR2              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UCR2 )
#define IMX6_PHYSADDR_UART4_UCR3              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UCR3 )
#define IMX6_PHYSADDR_UART4_UCR4              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UCR4 )
#define IMX6_PHYSADDR_UART4_UFCR              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UFCR )
#define IMX6_PHYSADDR_UART4_USR1              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_USR1 )
#define IMX6_PHYSADDR_UART4_USR2              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_USR2 )
#define IMX6_PHYSADDR_UART4_UESC              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UESC )
#define IMX6_PHYSADDR_UART4_UTIM              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UTIM )
#define IMX6_PHYSADDR_UART4_UBIR              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UBIR )
#define IMX6_PHYSADDR_UART4_UBMR              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UBMR )
#define IMX6_PHYSADDR_UART4_UBRC              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UBRC )
#define IMX6_PHYSADDR_UART4_ONEMS             (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_ONEMS)
#define IMX6_PHYSADDR_UART4_UTS               (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UTS  )
#define IMX6_PHYSADDR_UART4_UMCR              (IMX6_PHYSADDR_UART4 + IMX6_UART_OFFSET_UMCR )

#define IMX6_PHYSADDR_UART5_URXD              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_URXD )
#define IMX6_PHYSADDR_UART5_UTXD              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UTXD )
#define IMX6_PHYSADDR_UART5_UCR1              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UCR1 )
#define IMX6_PHYSADDR_UART5_UCR2              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UCR2 )
#define IMX6_PHYSADDR_UART5_UCR3              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UCR3 )
#define IMX6_PHYSADDR_UART5_UCR4              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UCR4 )
#define IMX6_PHYSADDR_UART5_UFCR              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UFCR )
#define IMX6_PHYSADDR_UART5_USR1              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_USR1 )
#define IMX6_PHYSADDR_UART5_USR2              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_USR2 )
#define IMX6_PHYSADDR_UART5_UESC              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UESC )
#define IMX6_PHYSADDR_UART5_UTIM              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UTIM )
#define IMX6_PHYSADDR_UART5_UBIR              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UBIR )
#define IMX6_PHYSADDR_UART5_UBMR              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UBMR )
#define IMX6_PHYSADDR_UART5_UBRC              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UBRC )
#define IMX6_PHYSADDR_UART5_ONEMS             (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_ONEMS)
#define IMX6_PHYSADDR_UART5_UTS               (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UTS  )
#define IMX6_PHYSADDR_UART5_UMCR              (IMX6_PHYSADDR_UART5 + IMX6_UART_OFFSET_UMCR )

#define IMX6_PHYSADDR_UART6_URXD              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_URXD )
#define IMX6_PHYSADDR_UART6_UTXD              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UTXD )
#define IMX6_PHYSADDR_UART6_UCR1              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UCR1 )
#define IMX6_PHYSADDR_UART6_UCR2              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UCR2 )
#define IMX6_PHYSADDR_UART6_UCR3              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UCR3 )
#define IMX6_PHYSADDR_UART6_UCR4              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UCR4 )
#define IMX6_PHYSADDR_UART6_UFCR              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UFCR )
#define IMX6_PHYSADDR_UART6_USR1              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_USR1 )
#define IMX6_PHYSADDR_UART6_USR2              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_USR2 )
#define IMX6_PHYSADDR_UART6_UESC              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UESC )
#define IMX6_PHYSADDR_UART6_UTIM              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UTIM )
#define IMX6_PHYSADDR_UART6_UBIR              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UBIR )
#define IMX6_PHYSADDR_UART6_UBMR              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UBMR )
#define IMX6_PHYSADDR_UART6_UBRC              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UBRC )
#define IMX6_PHYSADDR_UART6_ONEMS             (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_ONEMS)
#define IMX6_PHYSADDR_UART6_UTS               (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UTS  )
#define IMX6_PHYSADDR_UART6_UMCR              (IMX6_PHYSADDR_UART6 + IMX6_UART_OFFSET_UMCR )

#define IMX6_UART_URXD_CHARRDY        0x00008000
#define IMX6_UART_URXD_ERR            0x00004000
#define IMX6_UART_URXD_OVRRUN         0x00002000
#define IMX6_UART_URXD_FRMERR         0x00001000
#define IMX6_UART_URXD_BRK            0x00000800
#define IMX6_UART_URXD_PRERR          0x00000400
#define IMX6_UART_URXD_RX_DATA_MASK   0x000000FF

#define IMX6_UART_UTXD_TX_DATA_MASK   0x000000FF

#define IMX6_UART_UCR1_ADEN           0x00008000
#define IMX6_UART_UCR1_ADBR           0x00004000
#define IMX6_UART_UCR1_TRDYEN         0x00002000
#define IMX6_UART_UCR1_ICD            0x00001000
#define IMX6_UART_UCR1_RRDYEN         0x00000200
#define IMX6_UART_UCR1_RXDMAEN        0x00000100
#define IMX6_UART_UCR1_IREN           0x00000080
#define IMX6_UART_UCR1_TXMPTYEN       0x00000040
#define IMX6_UART_UCR1_RTSDEN         0x00000020
#define IMX6_UART_UCR1_SNDBRK         0x00000010
#define IMX6_UART_UCR1_TDMAEN         0x00000008
#define IMX6_UART_UCR1_ATDMAEN        0x00000004
#define IMX6_UART_UCR1_DOZE           0x00000002
#define IMX6_UART_UCR1_UARTEN         0x00000001

#define IMX6_UART_UCR2_ESCI           0x00008000
#define IMX6_UART_UCR2_IRTS           0x00004000
#define IMX6_UART_UCR2_CTSC           0x00002000
#define IMX6_UART_UCR2_CTS            0x00001000
#define IMX6_UART_UCR2_ESCEN          0x00000800
#define IMX6_UART_UCR2_RTEC_MASK      0x00000600
#define IMX6_UART_UCR2_PREN           0x00000100
#define IMX6_UART_UCR2_PROE           0x00000080
#define IMX6_UART_UCR2_STPB           0x00000040
#define IMX6_UART_UCR2_WS             0x00000020
#define IMX6_UART_UCR2_RTSEN          0x00000010
#define IMX6_UART_UCR2_ATEN           0x00000008
#define IMX6_UART_UCR2_TXEN           0x00000004
#define IMX6_UART_UCR2_RXEN           0x00000002
#define IMX6_UART_UCR2_SRST           0x00000001

#define IMX6_UART_UCR3_DPEC_MASK      0x0000C000
#define IMX6_UART_UCR3_DTREN          0x00002000
#define IMX6_UART_UCR3_PARERREN       0x00001000
#define IMX6_UART_UCR3_FRAERREN       0x00000800
#define IMX6_UART_UCR3_DSR            0x00000400
#define IMX6_UART_UCR3_DCD            0x00000200
#define IMX6_UART_UCR3_RI             0x00000100
#define IMX6_UART_UCR3_ADNIMP         0x00000080
#define IMX6_UART_UCR3_RXDSEN         0x00000040
#define IMX6_UART_UCR3_AIRINTEN       0x00000020
#define IMX6_UART_UCR3_AWAKEN         0x00000010
#define IMX6_UART_UCR3_DTRDEN         0x00000008
#define IMX6_UART_UCR3_RXDMUXSEL      0x00000004
#define IMX6_UART_UCR3_INVT           0x00000002
#define IMX6_UART_UCR3_ACIEN          0x00000001

#define IMX6_UART_UCR4_CTSTL_MASK     0x0000FC00
#define IMX6_UART_UCR4_INVR           0x00000200
#define IMX6_UART_UCR4_ENIRI          0x00000100
#define IMX6_UART_UCR4_WKEN           0x00000080
#define IMX6_UART_UCR4_IDDMAEN        0x00000040
#define IMX6_UART_UCR4_IRSC           0x00000020
#define IMX6_UART_UCR4_LPBYP          0x00000010
#define IMX6_UART_UCR4_TCEN           0x00000008
#define IMX6_UART_UCR4_BKEN           0x00000004
#define IMX6_UART_UCR4_OREN           0x00000002
#define IMX6_UART_UCR4_DREN           0x00000001

#define IMX6_UART_UFCR_TXTL_MASK      0x0000FC00
#define IMX6_UART_UFCR_RFDIV_MASK     0x00000380
#define IMX6_UART_UFCR_RFDIV_BY6      0x00000000
#define IMX6_UART_UFCR_RFDIV_BY5      0x00000080
#define IMX6_UART_UFCR_RFDIV_BY4      0x00000100
#define IMX6_UART_UFCR_RFDIV_BY3      0x00000180
#define IMX6_UART_UFCR_RFDIV_BY2      0x00000200
#define IMX6_UART_UFCR_RFDIV_BY1      0x00000280
#define IMX6_UART_UFCR_RFDIV_BY7      0x00000300

#define IMX6_UART_UFCR_DCEDTE         0x00000040
#define IMX6_UART_UFCR_RXTL_MASK      0x0000003F

#define IMX6_UART_USR1_PARITYERR      0x00008000
#define IMX6_UART_USR1_RTSS           0x00004000
#define IMX6_UART_USR1_TRDY           0x00002000
#define IMX6_UART_USR1_RTSD           0x00001000
#define IMX6_UART_USR1_ESCF           0x00000800
#define IMX6_UART_USR1_FRAMERR        0x00000400
#define IMX6_UART_USR1_RRDY           0x00000200
#define IMX6_UART_USR1_AGTIM          0x00000100
#define IMX6_UART_USR1_DTRD           0x00000080
#define IMX6_UART_USR1_RXDS           0x00000040
#define IMX6_UART_USR1_AIRINT         0x00000020
#define IMX6_UART_USR1_AWAKE          0x00000010
#define IMX6_UART_USR1_SAD            0x00000008

#define IMX6_UART_USR2_ADET           0x00008000
#define IMX6_UART_USR2_TXFE           0x00004000
#define IMX6_UART_USR2_DTRF           0x00002000
#define IMX6_UART_USR2_IDLE           0x00001000
#define IMX6_UART_USR2_ACST           0x00000800
#define IMX6_UART_USR2_RIDELT         0x00000400
#define IMX6_UART_USR2_RIIN           0x00000200
#define IMX6_UART_USR2_IRINT          0x00000100
#define IMX6_UART_USR2_WAKE           0x00000080
#define IMX6_UART_USR2_DCDDELT        0x00000040
#define IMX6_UART_USR2_DCDIN          0x00000020
#define IMX6_UART_USR2_RTSF           0x00000010
#define IMX6_UART_USR2_TXDC           0x00000008
#define IMX6_UART_USR2_BRCD           0x00000004
#define IMX6_UART_USR2_ORE            0x00000002
#define IMX6_UART_USR2_RDR            0x00000001

#define IMX6_UART_UTS_FRCPERR         0x00002000
#define IMX6_UART_UTS_LOOP            0x00001000
#define IMX6_UART_UTS_DBGEN           0x00000800
#define IMX6_UART_UTS_LOOPIR          0x00000400
#define IMX6_UART_UTS_RXDBG           0x00000200
#define IMX6_UART_UTS_TXEMPTY         0x00000040
#define IMX6_UART_UTS_RXEMPTY         0x00000020
#define IMX6_UART_UTS_TXFULL          0x00000010
#define IMX6_UART_UTS_RXFULL          0x00000008
#define IMX6_UART_UTS_SOFTRST         0x00000001

/* --------------------------------------------------------------------------------- */

#define IMX6_GPIO_OFFSET_DR                   0x0000
#define IMX6_GPIO_OFFSET_GDIR                 0x0004
#define IMX6_GPIO_OFFSET_PSR                  0x0008
#define IMX6_GPIO_OFFSET_ICR1                 0x000C
#define IMX6_GPIO_OFFSET_ICR2                 0x0010
#define IMX6_GPIO_OFFSET_IMR                  0x0014
#define IMX6_GPIO_OFFSET_ISR                  0x0018
#define IMX6_GPIO_OFFSET_EDGE_SEL             0x001C

#define IMX6_PHYSADDR_GPIO1_DR                (IMX6_PHYSADDR_GPIO1 + IMX6_GPIO_OFFSET_DR      )
#define IMX6_PHYSADDR_GPIO1_GDIR              (IMX6_PHYSADDR_GPIO1 + IMX6_GPIO_OFFSET_GDIR    )
#define IMX6_PHYSADDR_GPIO1_PSR               (IMX6_PHYSADDR_GPIO1 + IMX6_GPIO_OFFSET_PSR     )
#define IMX6_PHYSADDR_GPIO1_ICR1              (IMX6_PHYSADDR_GPIO1 + IMX6_GPIO_OFFSET_ICR1    )
#define IMX6_PHYSADDR_GPIO1_ICR2              (IMX6_PHYSADDR_GPIO1 + IMX6_GPIO_OFFSET_ICR2    )
#define IMX6_PHYSADDR_GPIO1_IMR               (IMX6_PHYSADDR_GPIO1 + IMX6_GPIO_OFFSET_IMR     )
#define IMX6_PHYSADDR_GPIO1_ISR               (IMX6_PHYSADDR_GPIO1 + IMX6_GPIO_OFFSET_ISR     )
#define IMX6_PHYSADDR_GPIO1_EDGE_SEL          (IMX6_PHYSADDR_GPIO1 + IMX6_GPIO_OFFSET_EDGE_SEL)

#define IMX6_PHYSADDR_GPIO2_DR                (IMX6_PHYSADDR_GPIO2 + IMX6_GPIO_OFFSET_DR      )
#define IMX6_PHYSADDR_GPIO2_GDIR              (IMX6_PHYSADDR_GPIO2 + IMX6_GPIO_OFFSET_GDIR    )
#define IMX6_PHYSADDR_GPIO2_PSR               (IMX6_PHYSADDR_GPIO2 + IMX6_GPIO_OFFSET_PSR     )
#define IMX6_PHYSADDR_GPIO2_ICR1              (IMX6_PHYSADDR_GPIO2 + IMX6_GPIO_OFFSET_ICR1    )
#define IMX6_PHYSADDR_GPIO2_ICR2              (IMX6_PHYSADDR_GPIO2 + IMX6_GPIO_OFFSET_ICR2    )
#define IMX6_PHYSADDR_GPIO2_IMR               (IMX6_PHYSADDR_GPIO2 + IMX6_GPIO_OFFSET_IMR     )
#define IMX6_PHYSADDR_GPIO2_ISR               (IMX6_PHYSADDR_GPIO2 + IMX6_GPIO_OFFSET_ISR     )
#define IMX6_PHYSADDR_GPIO2_EDGE_SEL          (IMX6_PHYSADDR_GPIO2 + IMX6_GPIO_OFFSET_EDGE_SEL)

#define IMX6_PHYSADDR_GPIO3_DR                (IMX6_PHYSADDR_GPIO3 + IMX6_GPIO_OFFSET_DR      )
#define IMX6_PHYSADDR_GPIO3_GDIR              (IMX6_PHYSADDR_GPIO3 + IMX6_GPIO_OFFSET_GDIR    )
#define IMX6_PHYSADDR_GPIO3_PSR               (IMX6_PHYSADDR_GPIO3 + IMX6_GPIO_OFFSET_PSR     )
#define IMX6_PHYSADDR_GPIO3_ICR1              (IMX6_PHYSADDR_GPIO3 + IMX6_GPIO_OFFSET_ICR1    )
#define IMX6_PHYSADDR_GPIO3_ICR2              (IMX6_PHYSADDR_GPIO3 + IMX6_GPIO_OFFSET_ICR2    )
#define IMX6_PHYSADDR_GPIO3_IMR               (IMX6_PHYSADDR_GPIO3 + IMX6_GPIO_OFFSET_IMR     )
#define IMX6_PHYSADDR_GPIO3_ISR               (IMX6_PHYSADDR_GPIO3 + IMX6_GPIO_OFFSET_ISR     )
#define IMX6_PHYSADDR_GPIO3_EDGE_SEL          (IMX6_PHYSADDR_GPIO3 + IMX6_GPIO_OFFSET_EDGE_SEL)

#define IMX6_PHYSADDR_GPIO4_DR                (IMX6_PHYSADDR_GPIO4 + IMX6_GPIO_OFFSET_DR      )
#define IMX6_PHYSADDR_GPIO4_GDIR              (IMX6_PHYSADDR_GPIO4 + IMX6_GPIO_OFFSET_GDIR    )
#define IMX6_PHYSADDR_GPIO4_PSR               (IMX6_PHYSADDR_GPIO4 + IMX6_GPIO_OFFSET_PSR     )
#define IMX6_PHYSADDR_GPIO4_ICR1              (IMX6_PHYSADDR_GPIO4 + IMX6_GPIO_OFFSET_ICR1    )
#define IMX6_PHYSADDR_GPIO4_ICR2              (IMX6_PHYSADDR_GPIO4 + IMX6_GPIO_OFFSET_ICR2    )
#define IMX6_PHYSADDR_GPIO4_IMR               (IMX6_PHYSADDR_GPIO4 + IMX6_GPIO_OFFSET_IMR     )
#define IMX6_PHYSADDR_GPIO4_ISR               (IMX6_PHYSADDR_GPIO4 + IMX6_GPIO_OFFSET_ISR     )
#define IMX6_PHYSADDR_GPIO4_EDGE_SEL          (IMX6_PHYSADDR_GPIO4 + IMX6_GPIO_OFFSET_EDGE_SEL)

#define IMX6_PHYSADDR_GPIO5_DR                (IMX6_PHYSADDR_GPIO5 + IMX6_GPIO_OFFSET_DR      )
#define IMX6_PHYSADDR_GPIO5_GDIR              (IMX6_PHYSADDR_GPIO5 + IMX6_GPIO_OFFSET_GDIR    )
#define IMX6_PHYSADDR_GPIO5_PSR               (IMX6_PHYSADDR_GPIO5 + IMX6_GPIO_OFFSET_PSR     )
#define IMX6_PHYSADDR_GPIO5_ICR1              (IMX6_PHYSADDR_GPIO5 + IMX6_GPIO_OFFSET_ICR1    )
#define IMX6_PHYSADDR_GPIO5_ICR2              (IMX6_PHYSADDR_GPIO5 + IMX6_GPIO_OFFSET_ICR2    )
#define IMX6_PHYSADDR_GPIO5_IMR               (IMX6_PHYSADDR_GPIO5 + IMX6_GPIO_OFFSET_IMR     )
#define IMX6_PHYSADDR_GPIO5_ISR               (IMX6_PHYSADDR_GPIO5 + IMX6_GPIO_OFFSET_ISR     )
#define IMX6_PHYSADDR_GPIO5_EDGE_SEL          (IMX6_PHYSADDR_GPIO5 + IMX6_GPIO_OFFSET_EDGE_SEL)

#define IMX6_PHYSADDR_GPIO6_DR                (IMX6_PHYSADDR_GPIO6 + IMX6_GPIO_OFFSET_DR      )
#define IMX6_PHYSADDR_GPIO6_GDIR              (IMX6_PHYSADDR_GPIO6 + IMX6_GPIO_OFFSET_GDIR    )
#define IMX6_PHYSADDR_GPIO6_PSR               (IMX6_PHYSADDR_GPIO6 + IMX6_GPIO_OFFSET_PSR     )
#define IMX6_PHYSADDR_GPIO6_ICR1              (IMX6_PHYSADDR_GPIO6 + IMX6_GPIO_OFFSET_ICR1    )
#define IMX6_PHYSADDR_GPIO6_ICR2              (IMX6_PHYSADDR_GPIO6 + IMX6_GPIO_OFFSET_ICR2    )
#define IMX6_PHYSADDR_GPIO6_IMR               (IMX6_PHYSADDR_GPIO6 + IMX6_GPIO_OFFSET_IMR     )
#define IMX6_PHYSADDR_GPIO6_ISR               (IMX6_PHYSADDR_GPIO6 + IMX6_GPIO_OFFSET_ISR     )
#define IMX6_PHYSADDR_GPIO6_EDGE_SEL          (IMX6_PHYSADDR_GPIO6 + IMX6_GPIO_OFFSET_EDGE_SEL)

#define IMX6_PHYSADDR_GPIO7_DR                (IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_DR      )
#define IMX6_PHYSADDR_GPIO7_GDIR              (IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_GDIR    )
#define IMX6_PHYSADDR_GPIO7_PSR               (IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_PSR     )
#define IMX6_PHYSADDR_GPIO7_ICR1              (IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_ICR1    )
#define IMX6_PHYSADDR_GPIO7_ICR2              (IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_ICR2    )
#define IMX6_PHYSADDR_GPIO7_IMR               (IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_IMR     )
#define IMX6_PHYSADDR_GPIO7_ISR               (IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_ISR     )
#define IMX6_PHYSADDR_GPIO7_EDGE_SEL          (IMX6_PHYSADDR_GPIO7 + IMX6_GPIO_OFFSET_EDGE_SEL)

// GDIR: 1 is OUTPUT, 0 is INPUT

#define IMX6_GPIO_ICR_LOW_LEVEL       0
#define IMX6_GPIO_ICR_HIGH_LEVEL      1
#define IMX6_GPIO_ICR_RIDING_EDGE     2
#define IMX6_GPIO_ICR_FALLING_EDGE    3

/* --------------------------------------------------------------------------------- */

#define IMX6_IOMUXC_OFFSET_GPR0           0x0000 
#define IMX6_IOMUXC_OFFSET_GPR1           0x0004 
#define IMX6_IOMUXC_OFFSET_GPR2           0x0008 
#define IMX6_IOMUXC_OFFSET_GPR3           0x000C 
#define IMX6_IOMUXC_OFFSET_GPR4           0x0010 
#define IMX6_IOMUXC_OFFSET_GPR5           0x0014 
#define IMX6_IOMUXC_OFFSET_GPR6           0x0018 
#define IMX6_IOMUXC_OFFSET_GPR7           0x001C 
#define IMX6_IOMUXC_OFFSET_GPR8           0x0020 
#define IMX6_IOMUXC_OFFSET_GPR9           0x0024 
#define IMX6_IOMUXC_OFFSET_GPR10          0x0028 
#define IMX6_IOMUXC_OFFSET_GPR11          0x002C 
#define IMX6_IOMUXC_OFFSET_GPR12          0x0030 
#define IMX6_IOMUXC_OFFSET_GPR13          0x0034 

#define IMX6_PHYSADDR_IOMUXC_GPR0         (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR0 )
#define IMX6_PHYSADDR_IOMUXC_GPR1         (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR1 )
#define IMX6_PHYSADDR_IOMUXC_GPR2         (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR2 )
#define IMX6_PHYSADDR_IOMUXC_GPR3         (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR3 )
#define IMX6_PHYSADDR_IOMUXC_GPR4         (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR4 )
#define IMX6_PHYSADDR_IOMUXC_GPR5         (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR5 )
#define IMX6_PHYSADDR_IOMUXC_GPR6         (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR6 )
#define IMX6_PHYSADDR_IOMUXC_GPR7         (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR7 )
#define IMX6_PHYSADDR_IOMUXC_GPR8         (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR8 )
#define IMX6_PHYSADDR_IOMUXC_GPR9         (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR9 )
#define IMX6_PHYSADDR_IOMUXC_GPR10        (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR10)
#define IMX6_PHYSADDR_IOMUXC_GPR11        (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR11)
#define IMX6_PHYSADDR_IOMUXC_GPR12        (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR12)
#define IMX6_PHYSADDR_IOMUXC_GPR13        (IMX6_PHYSADDR_IOMUXC + IMX6_IOMUXC_OFFSET_GPR13)

#define IMX6_IOMUXC_GPR_GPR1_ENET2_TX_CLK_DIR     0x00040000
#define IMX6_IOMUXC_GPR_GPR1_ENET1_TX_CLK_DIR     0x00020000
#define IMX6_IOMUXC_GPR_GPR1_ENET2_CLK_SEL        0x00004000
#define IMX6_IOMUXC_GPR_GPR1_ENET1_CLK_SEL        0x00002000

#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD2_DATA1                   0x004C  
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD2_DATA2                   0x0050
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD2_DATA0                   0x0054
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_RGMII_TXC                   0x0058
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_RGMII_TD0                   0x005C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_RGMII_TD1                   0x0060
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_RGMII_TD2                   0x0064
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_RGMII_TD3                   0x0068
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_RGMII_RX_CTL                0x006C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_RGMII_RD0                   0x0070
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_RGMII_TX_CTL                0x0074
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_RGMII_RD1                   0x0078
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_RGMII_RD2                   0x007C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_RGMII_RD3                   0x0080
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_RGMII_RXC                   0x0084
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_ADDR25                  0x0088
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_EB2_B                   0x008C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA16                  0x0090
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA17                  0x0094
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA18                  0x0098
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA19                  0x009C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA20                  0x00A0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA21                  0x00A4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA22                  0x00A8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA23                  0x00AC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_EB3_B                   0x00B0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA24                  0x00B4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA25                  0x00B8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA26                  0x00BC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA27                  0x00C0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA28                  0x00C4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA29                  0x00C8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA30                  0x00CC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_DATA31                  0x00D0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_ADDR24                  0x00D4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_ADDR23                  0x00D8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_ADDR22                  0x00DC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_ADDR21                  0x00E0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_ADDR20                  0x00E4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_ADDR19                  0x00E8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_ADDR18                  0x00EC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_ADDR17                  0x00F0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_ADDR16                  0x00F4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_CS0_B                   0x00F8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_CS1_B                   0x00FC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_OE_B                    0x0100
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_RW                      0x0104
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_LBA_B                   0x0108
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_EB0_B                   0x010C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_EB1_B                   0x0110
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD00                    0x0114
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD01                    0x0118
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD02                    0x011C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD03                    0x0120
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD04                    0x0124
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD05                    0x0128
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD06                    0x012C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD07                    0x0130
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD08                    0x0134
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD09                    0x0138
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD10                    0x013C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD11                    0x0140
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD12                    0x0144
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD13                    0x0148
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD14                    0x014C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_AD15                    0x0150
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_WAIT_B                  0x0154
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_EIM_BCLK                    0x0158
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DI0_DISP_CLK                0x015C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DI0_PIN15                   0x0160
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DI0_PIN02                   0x0164
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DI0_PIN03                   0x0168
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DI0_PIN04                   0x016C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA00                0x0170
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA01                0x0174
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA02                0x0178
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA03                0x017C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA04                0x0180
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA05                0x0184
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA06                0x0188
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA07                0x018C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA08                0x0190
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA09                0x0194
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA10                0x0198
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA11                0x019C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA12                0x01A0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA13                0x01A4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA14                0x01A8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA15                0x01AC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA16                0x01B0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA17                0x01B4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA18                0x01B8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA19                0x01BC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA20                0x01C0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA21                0x01C4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA22                0x01C8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_DISP0_DATA23                0x01CC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_ENET_MDIO                   0x01D0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_ENET_REF_CLK                0x01D4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_ENET_RX_ER                  0x01D8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_ENET_CRS_DV                 0x01DC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_ENET_RX_DATA1               0x01E0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_ENET_RX_DATA0               0x01E4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_ENET_TX_EN                  0x01E8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_ENET_TX_DATA1               0x01EC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_ENET_TX_DATA0               0x01F0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_ENET_MDC                    0x01F4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_KEY_COL0                    0x01F8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_KEY_ROW0                    0x01FC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_KEY_COL1                    0x0200
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_KEY_ROW1                    0x0204
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_KEY_COL2                    0x0208
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_KEY_ROW2                    0x020C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_KEY_COL3                    0x0210
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_KEY_ROW3                    0x0214
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_KEY_COL4                    0x0218
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_KEY_ROW4                    0x021C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO00                      0x0220
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO01                      0x0224
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO09                      0x0228
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO03                      0x022C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO06                      0x0230
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO02                      0x0234
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO04                      0x0238
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO05                      0x023C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO07                      0x0240
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO08                      0x0244
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO16                      0x0248
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO17                      0x024C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO18                      0x0250
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_GPIO19                      0x0254
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_PIXCLK                 0x0258
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_HSYNC                  0x025C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA_EN                0x0260
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_VSYNC                  0x0264
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA04                 0x0268
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA05                 0x026C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA06                 0x0270
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA07                 0x0274
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA08                 0x0278
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA09                 0x027C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA10                 0x0280
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA11                 0x0284
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA12                 0x0288
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA13                 0x028C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA14                 0x0290
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA15                 0x0294
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA16                 0x0298
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA17                 0x029C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA18                 0x02A0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_CSI0_DATA19                 0x02A4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA7                   0x02A8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA6                   0x02AC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA5                   0x02B0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA4                   0x02B4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_CMD                     0x02B8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_CLK                     0x02BC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA0                   0x02C0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA1                   0x02C4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA2                   0x02C8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_DATA3                   0x02CC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD3_RESET                   0x02D0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_CLE                    0x02D4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_ALE                    0x02D8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_WP_B                   0x02DC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_READY_B                0x02E0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_CS0_B                  0x02E4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_CS1_B                  0x02E8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_CS2_B                  0x02EC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_CS3_B                  0x02F0
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD4_CMD                     0x02F4
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD4_CLK                     0x02F8
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_DATA00                 0x02FC
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_DATA01                 0x0300
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_DATA02                 0x0304
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_DATA03                 0x0308
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_DATA04                 0x030C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_DATA05                 0x0310
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_DATA06                 0x0314
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_NAND_DATA07                 0x0318
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD4_DATA0                   0x031C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD4_DATA1                   0x0320
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD4_DATA2                   0x0324
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD4_DATA3                   0x0328
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD4_DATA4                   0x032C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD4_DATA5                   0x0330
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD4_DATA6                   0x0334
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD4_DATA7                   0x0338
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD1_DATA1                   0x033C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD1_DATA0                   0x0340
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD1_DATA3                   0x0344
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD1_CMD                     0x0348
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD1_DATA2                   0x034C
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD1_CLK                     0x0350
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD2_CLK                     0x0354
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD2_CMD                     0x0358
#define IMX6_IOMUXC_OFFSET_SW_MUX_CTL_PAD_SD2_DATA3                   0x035C

#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD2_DATA1                   0x0360 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD2_DATA2                   0x0364 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD2_DATA0                   0x0368 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_RGMII_TXC                   0x036C 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_RGMII_TD0                   0x0370 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_RGMII_TD1                   0x0374 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_RGMII_TD2                   0x0378 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_RGMII_TD3                   0x037C 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_RGMII_RX_CTL                0x0380    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_RGMII_RD0                   0x0384 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_RGMII_TX_CTL                0x0388    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_RGMII_RD1                   0x038C 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_RGMII_RD2                   0x0390 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_RGMII_RD3                   0x0394 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_RGMII_RXC                   0x0398 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_ADDR25                  0x039C  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_EB2_B                   0x03A0 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA16                  0x03A4  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA17                  0x03A8  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA18                  0x03AC  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA19                  0x03B0  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA20                  0x03B4  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA21                  0x03B8  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA22                  0x03BC  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA23                  0x03C0  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_EB3_B                   0x03C4 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA24                  0x03C8  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA25                  0x03CC  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA26                  0x03D0  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA27                  0x03D4  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA28                  0x03D8  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA29                  0x03DC  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA30                  0x03E0  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_DATA31                  0x03E4  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_ADDR24                  0x03E8  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_ADDR23                  0x03EC  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_ADDR22                  0x03F0  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_ADDR21                  0x03F4  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_ADDR20                  0x03F8  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_ADDR19                  0x03FC  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_ADDR18                  0x0400  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_ADDR17                  0x0404  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_ADDR16                  0x0408  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_CS0_B                   0x040C 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_CS1_B                   0x0410 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_OE_B                    0x0414
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_RW                      0x0418
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_LBA_B                   0x041C 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_EB0_B                   0x0420 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_EB1_B                   0x0424 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD00                    0x0428
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD01                    0x042C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD02                    0x0430
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD03                    0x0434
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD04                    0x0438
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD05                    0x043C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD06                    0x0440
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD07                    0x0444
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD08                    0x0448
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD09                    0x044C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD10                    0x0450
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD11                    0x0454
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD12                    0x0458
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD13                    0x045C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD14                    0x0460
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_AD15                    0x0464
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_WAIT_B                  0x0468  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_EIM_BCLK                    0x046C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DI0_DISP_CLK                0x0470    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DI0_PIN15                   0x0474 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DI0_PIN02                   0x0478 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DI0_PIN03                   0x047C 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DI0_PIN04                   0x0480 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA00                0x0484    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA01                0x0488    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA02                0x048C    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA03                0x0490    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA04                0x0494    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA05                0x0498    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA06                0x049C    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA07                0x04A0    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA08                0x04A4    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA09                0x04A8    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA10                0x04AC    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA11                0x04B0    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA12                0x04B4    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA13                0x04B8    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA14                0x04BC    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA15                0x04C0    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA16                0x04C4    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA17                0x04C8    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA18                0x04CC    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA19                0x04D0    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA20                0x04D4    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA21                0x04D8    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA22                0x04DC    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DISP0_DATA23                0x04E0    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_ENET_MDIO                   0x04E4 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_ENET_REF_CLK                0x04E8    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_ENET_RX_ER                  0x04EC  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_ENET_CRS_DV                 0x04F0   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_ENET_RX_DATA1               0x04F4     
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_ENET_RX_DATA0               0x04F8     
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_ENET_TX_EN                  0x04FC  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_ENET_TX_DATA1               0x0500     
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_ENET_TX_DATA0               0x0504     
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_ENET_MDC                    0x0508
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDQS5_P                0x050C    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_DQM5                   0x0510 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_DQM4                   0x0514 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDQS4_P                0x0518    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDQS3_P                0x051C    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_DQM3                   0x0520 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDQS2_P                0x0524    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_DQM2                   0x0528 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR00                 0x052C   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR01                 0x0530   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR02                 0x0534   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR03                 0x0538   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR04                 0x053C   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR05                 0x0540   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR06                 0x0544   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR07                 0x0548   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR08                 0x054C   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR09                 0x0550   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR10                 0x0554   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR11                 0x0558   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR12                 0x055C   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR13                 0x0560   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR14                 0x0564   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ADDR15                 0x0568   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_CAS_B                  0x056C  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_CS0_B                  0x0570  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_CS1_B                  0x0574  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_RAS_B                  0x0578  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_RESET                  0x057C  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDBA0                  0x0580  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDBA1                  0x0584  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDCLK0_P               0x0588     
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDBA2                  0x058C  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDCKE0                 0x0590   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDCLK1_P               0x0594     
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDCKE1                 0x0598   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ODT0                   0x059C 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_ODT1                   0x05A0 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDWE_B                 0x05A4   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDQS0_P                0x05A8    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_DQM0                   0x05AC 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDQS1_P                0x05B0    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_DQM1                   0x05B4 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDQS6_P                0x05B8    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_DQM6                   0x05BC 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_SDQS7_P                0x05C0    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_DRAM_DQM7                   0x05C4 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_KEY_COL0                    0x05C8
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_KEY_ROW0                    0x05CC
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_KEY_COL1                    0x05D0
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_KEY_ROW1                    0x05D4
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_KEY_COL2                    0x05D8
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_KEY_ROW2                    0x05DC
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_KEY_COL3                    0x05E0
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_KEY_ROW3                    0x05E4
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_KEY_COL4                    0x05E8
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_KEY_ROW4                    0x05EC
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO00                      0x05F0
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO01                      0x05F4
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO09                      0x05F8
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO03                      0x05FC
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO06                      0x0600
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO02                      0x0604
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO04                      0x0608
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO05                      0x060C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO07                      0x0610
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO08                      0x0614
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO16                      0x0618
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO17                      0x061C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO18                      0x0620
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_GPIO19                      0x0624
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_PIXCLK                 0x0628   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_HSYNC                  0x062C  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA_EN                0x0630    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_VSYNC                  0x0634  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA04                 0x0638   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA05                 0x063C   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA06                 0x0640   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA07                 0x0644   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA08                 0x0648   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA09                 0x064C   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA10                 0x0650   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA11                 0x0654   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA12                 0x0658   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA13                 0x065C   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA14                 0x0660   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA15                 0x0664   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA16                 0x0668   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA17                 0x066C   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA18                 0x0670   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_CSI0_DATA19                 0x0674   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_JTAG_TMS                    0x0678
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_JTAG_MOD                    0x067C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_JTAG_TRSTB                  0x0680  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_JTAG_TDI                    0x0684
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_JTAG_TCK                    0x0688
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_JTAG_TDO                    0x068C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA7                   0x0690 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA6                   0x0694 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA5                   0x0698 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA4                   0x069C 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_CMD                     0x06A0
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_CLK                     0x06A4
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA0                   0x06A8 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA1                   0x06AC 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA2                   0x06B0 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_DATA3                   0x06B4 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD3_RESET                   0x06B8 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_CLE                    0x06BC
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_ALE                    0x06C0
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_WP_B                   0x06C4 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_READY_B                0x06C8    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_CS0_B                  0x06CC  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_CS1_B                  0x06D0  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_CS2_B                  0x06D4  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_CS3_B                  0x06D8  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD4_CMD                     0x06DC
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD4_CLK                     0x06E0
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_DATA00                 0x06E4   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_DATA01                 0x06E8   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_DATA02                 0x06EC   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_DATA03                 0x06F0   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_DATA04                 0x06F4   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_DATA05                 0x06F8   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_DATA06                 0x06FC   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_NAND_DATA07                 0x0700   
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD4_DATA0                   0x0704 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD4_DATA1                   0x0708 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD4_DATA2                   0x070C 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD4_DATA3                   0x0710 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD4_DATA4                   0x0714 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD4_DATA5                   0x0718 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD4_DATA6                   0x071C 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD4_DATA7                   0x0720 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD1_DATA1                   0x0724 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD1_DATA0                   0x0728 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD1_DATA3                   0x072C 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD1_CMD                     0x0730
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD1_DATA2                   0x0734 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD1_CLK                     0x0738
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD2_CLK                     0x073C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD2_CMD                     0x0740
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_PAD_SD2_DATA3                   0x0744 

#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_B7DS                        0x0748
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_ADDDS                       0x074C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_DDRMODE_CTL                 0x0750    
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_TERM_CTL0                   0x0754  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_DDRPKE                      0x0758
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_TERM_CTL1                   0x075C  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_TERM_CTL2                   0x0760  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_TERM_CTL3                   0x0764  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_DDRPK                       0x0768
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_TERM_CTL4                   0x076C  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_DDRHYS                      0x0770
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_DDRMODE                     0x0774
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_TERM_CTL5                   0x0778  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_TERM_CTL6                   0x077C  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_TERM_CTL7                   0x0780  
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_B0DS                        0x0784
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_B1DS                        0x0788
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_CTLDS                       0x078C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_DDR_TYPE_RGMII              0x0790       
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_B2DS                        0x0794
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_DDR_TYPE                    0x0798 
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_B3DS                        0x079C
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_B4DS                        0x07A0
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_B5DS                        0x07A4
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_B6DS                        0x07A8
#define IMX6_IOMUXC_OFFSET_SW_PAD_CTL_GRP_RGMII_TERM                  0x07AC   

#define IMX6_IOMUXC_OFFSET_ASRC_ASRCK_CLOCK_6_SELECT_INPUT            0x07B0       
#define IMX6_IOMUXC_OFFSET_AUD4_INPUT_DA_AMX_SELECT_INPUT             0x07B4      
#define IMX6_IOMUXC_OFFSET_AUD4_INPUT_DB_AMX_SELECT_INPUT             0x07B8      
#define IMX6_IOMUXC_OFFSET_AUD4_INPUT_RXCLK_AMX_SELECT_INPUT          0x07BC         
#define IMX6_IOMUXC_OFFSET_AUD4_INPUT_RXFS_AMX_SELECT_INPUT           0x07C0        
#define IMX6_IOMUXC_OFFSET_AUD4_INPUT_TXCLK_AMX_SELECT_INPUT          0x07C4         
#define IMX6_IOMUXC_OFFSET_AUD4_INPUT_TXFS_AMX_SELECT_INPUT           0x07C8        
#define IMX6_IOMUXC_OFFSET_AUD5_INPUT_DA_AMX_SELECT_INPUT             0x07CC      
#define IMX6_IOMUXC_OFFSET_AUD5_INPUT_DB_AMX_SELECT_INPUT             0x07D0      
#define IMX6_IOMUXC_OFFSET_AUD5_INPUT_RXCLK_AMX_SELECT_INPUT          0x07D4         
#define IMX6_IOMUXC_OFFSET_AUD5_INPUT_RXFS_AMX_SELECT_INPUT           0x07D8        
#define IMX6_IOMUXC_OFFSET_AUD5_INPUT_TXCLK_AMX_SELECT_INPUT          0x07DC         
#define IMX6_IOMUXC_OFFSET_AUD5_INPUT_TXFS_AMX_SELECT_INPUT           0x07E0        
#define IMX6_IOMUXC_OFFSET_FLEXCAN1_RX_SELECT_INPUT                   0x07E4
#define IMX6_IOMUXC_OFFSET_FLEXCAN2_RX_SELECT_INPUT                   0x07E8
#define IMX6_IOMUXC_OFFSET_CCM_PMIC_READY_SELECT_INPUT                0x07F0   
#define IMX6_IOMUXC_OFFSET_ECSPI1_CSPI_CLK_IN_SELECT_INPUT            0x07F4       
#define IMX6_IOMUXC_OFFSET_ECSPI1_MISO_SELECT_INPUT                   0x07F8
#define IMX6_IOMUXC_OFFSET_ECSPI1_MOSI_SELECT_INPUT                   0x07FC
#define IMX6_IOMUXC_OFFSET_ECSPI1_SS0_SELECT_INPUT                    0x0800
#define IMX6_IOMUXC_OFFSET_ECSPI1_SS1_SELECT_INPUT                    0x0804
#define IMX6_IOMUXC_OFFSET_ECSPI1_SS2_SELECT_INPUT                    0x0808
#define IMX6_IOMUXC_OFFSET_ECSPI1_SS3_SELECT_INPUT                    0x080C
#define IMX6_IOMUXC_OFFSET_ECSPI2_CSPI_CLK_IN_SELECT_INPUT            0x0810       
#define IMX6_IOMUXC_OFFSET_ECSPI2_MISO_SELECT_INPUT                   0x0814
#define IMX6_IOMUXC_OFFSET_ECSPI2_MOSI_SELECT_INPUT                   0x0818
#define IMX6_IOMUXC_OFFSET_ECSPI2_SS0_SELECT_INPUT                    0x081C
#define IMX6_IOMUXC_OFFSET_ECSPI2_SS1_SELECT_INPUT                    0x0820
#define IMX6_IOMUXC_OFFSET_ECSPI4_SS0_SELECT_INPUT                    0x0824
#define IMX6_IOMUXC_OFFSET_ECSPI5_CSPI_CLK_IN_SELECT_INPUT            0x0828       
#define IMX6_IOMUXC_OFFSET_ECSPI5_MISO_SELECT_INPUT                   0x082C
#define IMX6_IOMUXC_OFFSET_ECSPI5_MOSI_SELECT_INPUT                   0x0830
#define IMX6_IOMUXC_OFFSET_ECSPI5_SS0_SELECT_INPUT                    0x0834
#define IMX6_IOMUXC_OFFSET_ECSPI5_SS1_SELECT_INPUT                    0x0838
#define IMX6_IOMUXC_OFFSET_ENET_REF_CLK_SELECT_INPUT                  0x083C 
#define IMX6_IOMUXC_OFFSET_ENET_MAC0_MDIO_SELECT_INPUT                0x0840   
#define IMX6_IOMUXC_OFFSET_ENET_MAC0_RX_CLK_SELECT_INPUT              0x0844     
#define IMX6_IOMUXC_OFFSET_ENET_MAC0_RX_DATA0_SELECT_INPUT            0x0848       
#define IMX6_IOMUXC_OFFSET_ENET_MAC0_RX_DATA1_SELECT_INPUT            0x084C       
#define IMX6_IOMUXC_OFFSET_ENET_MAC0_RX_DATA2_SELECT_INPUT            0x0850       
#define IMX6_IOMUXC_OFFSET_ENET_MAC0_RX_DATA3_SELECT_INPUT            0x0854       
#define IMX6_IOMUXC_OFFSET_ENET_MAC0_RX_EN_SELECT_INPUT               0x0858    
#define IMX6_IOMUXC_OFFSET_ESAI_RX_FS_SELECT_INPUT                    0x085C
#define IMX6_IOMUXC_OFFSET_ESAI_TX_FS_SELECT_INPUT                    0x0860
#define IMX6_IOMUXC_OFFSET_ESAI_RX_HF_CLK_SELECT_INPUT                0x0864   
#define IMX6_IOMUXC_OFFSET_ESAI_TX_HF_CLK_SELECT_INPUT                0x0868   
#define IMX6_IOMUXC_OFFSET_ESAI_RX_CLK_SELECT_INPUT                   0x086C
#define IMX6_IOMUXC_OFFSET_ESAI_TX_CLK_SELECT_INPUT                   0x0870
#define IMX6_IOMUXC_OFFSET_ESAI_SDO0_SELECT_INPUT                     0x0874
#define IMX6_IOMUXC_OFFSET_ESAI_SDO1_SELECT_INPUT                     0x0878
#define IMX6_IOMUXC_OFFSET_ESAI_SDO2_SDI3_SELECT_INPUT                0x087C   
#define IMX6_IOMUXC_OFFSET_ESAI_SDO3_SDI2_SELECT_INPUT                0x0880   
#define IMX6_IOMUXC_OFFSET_ESAI_SDO4_SDI1_SELECT_INPUT                0x0884   
#define IMX6_IOMUXC_OFFSET_ESAI_SDO5_SDI0_SELECT_INPUT                0x0888   
#define IMX6_IOMUXC_OFFSET_HDMI_ICECIN_SELECT_INPUT                   0x088C
#define IMX6_IOMUXC_OFFSET_HDMI_II2C_CLKIN_SELECT_INPUT               0x0890    
#define IMX6_IOMUXC_OFFSET_HDMI_II2C_DATAIN_SELECT_INPUT              0x0894     
#define IMX6_IOMUXC_OFFSET_I2C1_SCL_IN_SELECT_INPUT                   0x0898
#define IMX6_IOMUXC_OFFSET_I2C1_SDA_IN_SELECT_INPUT                   0x089C
#define IMX6_IOMUXC_OFFSET_I2C2_SCL_IN_SELECT_INPUT                   0x08A0
#define IMX6_IOMUXC_OFFSET_I2C2_SDA_IN_SELECT_INPUT                   0x08A4
#define IMX6_IOMUXC_OFFSET_I2C3_SCL_IN_SELECT_INPUT                   0x08A8
#define IMX6_IOMUXC_OFFSET_I2C3_SDA_IN_SELECT_INPUT                   0x08AC
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_DATA10_SELECT_INPUT             0x08B0      
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_DATA11_SELECT_INPUT             0x08B4      
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_DATA12_SELECT_INPUT             0x08B8      
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_DATA13_SELECT_INPUT             0x08BC      
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_DATA14_SELECT_INPUT             0x08C0      
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_DATA15_SELECT_INPUT             0x08C4      
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_DATA16_SELECT_INPUT             0x08C8      
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_DATA17_SELECT_INPUT             0x08CC      
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_DATA18_SELECT_INPUT             0x08D0      
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_DATA19_SELECT_INPUT             0x08D4      
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_DATA_EN_SELECT_INPUT            0x08D8       
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_HSYNC_SELECT_INPUT              0x08DC     
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_PIX_CLK_SELECT_INPUT            0x08E0       
#define IMX6_IOMUXC_OFFSET_IPU2_SENS1_VSYNC_SELECT_INPUT              0x08E4     
#define IMX6_IOMUXC_OFFSET_KEY_COL5_SELECT_INPUT                      0x08E8
#define IMX6_IOMUXC_OFFSET_KEY_COL6_SELECT_INPUT                      0x08EC
#define IMX6_IOMUXC_OFFSET_KEY_COL7_SELECT_INPUT                      0x08F0
#define IMX6_IOMUXC_OFFSET_KEY_ROW5_SELECT_INPUT                      0x08F4
#define IMX6_IOMUXC_OFFSET_KEY_ROW6_SELECT_INPUT                      0x08F8
#define IMX6_IOMUXC_OFFSET_KEY_ROW7_SELECT_INPUT                      0x08FC
#define IMX6_IOMUXC_OFFSET_MLB_MLB_CLK_IN_SELECT_INPUT                0x0900   
#define IMX6_IOMUXC_OFFSET_MLB_MLB_DATA_IN_SELECT_INPUT               0x0904    
#define IMX6_IOMUXC_OFFSET_MLB_MLB_SIG_IN_SELECT_INPUT                0x0908   
#define IMX6_IOMUXC_OFFSET_SDMA_EVENTS14_SELECT_INPUT                 0x090C  
#define IMX6_IOMUXC_OFFSET_SDMA_EVENTS47_SELECT_INPUT                 0x0910  
#define IMX6_IOMUXC_OFFSET_SPDIF_SPDIF_IN1_SELECT_INPUT               0x0914    
#define IMX6_IOMUXC_OFFSET_SPDIF_TX_CLK2_SELECT_INPUT                 0x0918  
#define IMX6_IOMUXC_OFFSET_UART1_UART_RTS_B_SELECT_INPUT              0x091C     
#define IMX6_IOMUXC_OFFSET_UART1_UART_RX_DATA_SELECT_INPUT            0x0920       
#define IMX6_IOMUXC_OFFSET_UART2_UART_RTS_B_SELECT_INPUT              0x0924     
#define IMX6_IOMUXC_OFFSET_UART2_UART_RX_DATA_SELECT_INPUT            0x0928       
#define IMX6_IOMUXC_OFFSET_UART3_UART_RTS_B_SELECT_INPUT              0x092C     
#define IMX6_IOMUXC_OFFSET_UART3_UART_RX_DATA_SELECT_INPUT            0x0930       
#define IMX6_IOMUXC_OFFSET_UART4_UART_RTS_B_SELECT_INPUT              0x0934     
#define IMX6_IOMUXC_OFFSET_UART4_UART_RX_DATA_SELECT_INPUT            0x0938       
#define IMX6_IOMUXC_OFFSET_UART5_UART_RTS_B_SELECT_INPUT              0x093C     
#define IMX6_IOMUXC_OFFSET_UART5_UART_RX_DATA_SELECT_INPUT            0x0940       
#define IMX6_IOMUXC_OFFSET_USB_OTG_OC_SELECT_INPUT                    0x0944
#define IMX6_IOMUXC_OFFSET_USB_H1_OC_SELECT_INPUT                     0x0948
#define IMX6_IOMUXC_OFFSET_USDHC1_WP_ON_SELECT_INPUT                  0x094C 

#define IMX6_MUX_MODE_SION               0x00000010

#define IMX6_MUX_PADCTL_HYS              0x00010000

#define IMX6_MUX_PADCTL_PUS_100KOHM_PD   0x00000000
#define IMX6_MUX_PADCTL_PUS_47KOHM_PU   (0x00004000 | IMX6_MUX_PADCTL_PUE)
#define IMX6_MUX_PADCTL_PUS_100KOHM_PU  (0x00008000 | IMX6_MUX_PADCTL_PUE)
#define IMX6_MUX_PADCTL_PUS_22KOHM_PU   (0x0000C000 | IMX6_MUX_PADCTL_PUE)

#define IMX6_MUX_PADCTL_PUE              0x00002000

#define IMX6_MUX_PADCTL_PKE              0x00001000

#define IMX6_MUX_PADCTL_ODE              0x00000800

#define IMX6_MUX_PADCTL_SPEED_50MHZ      0x00000000
#define IMX6_MUX_PADCTL_SPEED_100MHZ     0x00000080
#define IMX6_MUX_PADCTL_SPEED_200MHZ     0x000000C0

#define IMX6_MUX_PADCTL_DSE_HIZ          0x00000000
#define IMX6_MUX_PADCTL_DSE_260_OHM      0x00000008
#define IMX6_MUX_PADCTL_DSE_240_OHM      0x00000008
#define IMX6_MUX_PADCTL_DSE_130_OHM      0x00000010
#define IMX6_MUX_PADCTL_DSE_120_OHM      0x00000010
#define IMX6_MUX_PADCTL_DSE_90_OHM       0x00000018
#define IMX6_MUX_PADCTL_DSE_80_OHM       0x00000018
#define IMX6_MUX_PADCTL_DSE_60_OHM       0x00000020
#define IMX6_MUX_PADCTL_DSE_50_OHM       0x00000028
#define IMX6_MUX_PADCTL_DSE_48_OHM       0x00000028
#define IMX6_MUX_PADCTL_DSE_40_OHM       0x00000030
#define IMX6_MUX_PADCTL_DSE_34_OHM       0x00000038
#define IMX6_MUX_PADCTL_DSE_33_OHM       0x00000038

#define IMX6_MUX_PADCTL_SRE_SLOW         0x00000000
#define IMX6_MUX_PADCTL_SRE_FAST         0x00000001
                                                 
/* --------------------------------------------------------------------------------- */

#define IMX6_GPT_OFFSET_CR            0x0000
#define IMX6_GPT_OFFSET_PR            0x0004
#define IMX6_GPT_OFFSET_SR            0x0008
#define IMX6_GPT_OFFSET_IR            0x000C
#define IMX6_GPT_OFFSET_OCR1          0x0010
#define IMX6_GPT_OFFSET_OCR2          0x0014
#define IMX6_GPT_OFFSET_OCR3          0x0018
#define IMX6_GPT_OFFSET_ICR1          0x001C
#define IMX6_GPT_OFFSET_ICR2          0x0020
#define IMX6_GPT_OFFSET_CNT           0x0024

#define IMX6_PHYSADDR_GPT_CR          (IMX6_PHYSADDR_GPT + IMX6_GPT_OFFSET_CR  )
#define IMX6_PHYSADDR_GPT_PR          (IMX6_PHYSADDR_GPT + IMX6_GPT_OFFSET_PR  )
#define IMX6_PHYSADDR_GPT_SR          (IMX6_PHYSADDR_GPT + IMX6_GPT_OFFSET_SR  )
#define IMX6_PHYSADDR_GPT_IR          (IMX6_PHYSADDR_GPT + IMX6_GPT_OFFSET_IR  )
#define IMX6_PHYSADDR_GPT_OCR1        (IMX6_PHYSADDR_GPT + IMX6_GPT_OFFSET_OCR1)
#define IMX6_PHYSADDR_GPT_OCR2        (IMX6_PHYSADDR_GPT + IMX6_GPT_OFFSET_OCR2)
#define IMX6_PHYSADDR_GPT_OCR3        (IMX6_PHYSADDR_GPT + IMX6_GPT_OFFSET_OCR3)
#define IMX6_PHYSADDR_GPT_ICR1        (IMX6_PHYSADDR_GPT + IMX6_GPT_OFFSET_ICR1)
#define IMX6_PHYSADDR_GPT_ICR2        (IMX6_PHYSADDR_GPT + IMX6_GPT_OFFSET_ICR2)
#define IMX6_PHYSADDR_GPT_CNT         (IMX6_PHYSADDR_GPT + IMX6_GPT_OFFSET_CNT )

#define IMX6_GPT_CR_FO3               0x80000000
#define IMX6_GPT_CR_FO2               0x40000000
#define IMX6_GPT_CR_FO1               0x20000000
#define IMX6_GPT_CR_OM3_MASK          0x1C000000
#define IMX6_GPT_CR_OM3_NONE          0x00000000
#define IMX6_GPT_CR_OM3_TOGGLE        0x04000000
#define IMX6_GPT_CR_OM3_CLEAR         0x08000000
#define IMX6_GPT_CR_OM3_SET           0x0C000000
#define IMX6_GPT_CR_OM3_LOWPULSE      0x10000000
#define IMX6_GPT_CR_OM2_MASK          0x03800000
#define IMX6_GPT_CR_OM2_NONE          0x00000000
#define IMX6_GPT_CR_OM2_TOGGLE        0x00800000
#define IMX6_GPT_CR_OM2_CLEAR         0x01000000
#define IMX6_GPT_CR_OM2_SET           0x01800000
#define IMX6_GPT_CR_OM2_LOWPULSE      0x02000000
#define IMX6_GPT_CR_OM1_MASK          0x00700000
#define IMX6_GPT_CR_OM1_NONE          0x00000000
#define IMX6_GPT_CR_OM1_TOGGLE        0x00100000
#define IMX6_GPT_CR_OM1_CLEAR         0x00200000
#define IMX6_GPT_CR_OM1_SET           0x00300000
#define IMX6_GPT_CR_OM1_LOWPULSE      0x00400000
#define IMX6_GPT_CR_IM2_MASK          0x000c0000
#define IMX6_GPT_CR_IM2_DISABLED      0x00000000
#define IMX6_GPT_CR_IM2_RISINGEDGE    0x00040000
#define IMX6_GPT_CR_IM2_FALLINGEDGE   0x00080000
#define IMX6_GPT_CR_IM2_BOTHEDGES     0x000C0000
#define IMX6_GPT_CR_IM1_MASK          0x00030000
#define IMX6_GPT_CR_IM1_DISABLED      0x00000000
#define IMX6_GPT_CR_IM1_RISINGEDGE    0x00010000
#define IMX6_GPT_CR_IM1_FALLINGEDGE   0x00020000
#define IMX6_GPT_CR_IM1_BOTHEDGES     0x00030000
#define IMX6_GPT_CR_SWR               0x00008000
#define IMX6_GPT_CR_FRR               0x00000200
#define IMX6_GPT_CR_CLKSRC_MASK       0x000001C0
#define IMX6_GPT_CR_CLKSRC_NONE       0x00000000
#define IMX6_GPT_CR_CLKSRC_PERIF      0x00000040
#define IMX6_GPT_CR_CLKSRC_HIGHFREQ   0x00000080
#define IMX6_GPT_CR_CLKSRC_EXTERN     0x000000C0
#define IMX6_GPT_CR_CLKSRC_LOWFREQ    0x00000100
#define IMX6_GPT_CR_CLKSRC_OSC_DIV8   0x00000140
#define IMX6_GPT_CR_CLKSRC_OSC        0x000001C0
#define IMX6_GPT_CR_STOPEN            0x00000020
#define IMX6_GPT_CR_DOZEEN            0x00000010
#define IMX6_GPT_CR_WAITEN            0x00000008
#define IMX6_GPT_CR_DBGEN             0x00000004
#define IMX6_GPT_CR_ENMOD             0x00000002
#define IMX6_GPT_CR_EN                0x00000001

#define IMX6_GPT_PR_PRESCALER_MASK    0x00000FFF

#define IMX6_GPT_SR_ROV               0x00000020
#define IMX6_GPT_SR_IF2               0x00000010
#define IMX6_GPT_SR_IF1               0x00000008
#define IMX6_GPT_SR_OF3               0x00000004
#define IMX6_GPT_SR_OF2               0x00000002
#define IMX6_GPT_SR_OF1               0x00000001

#define IMX6_GPT_IR_ROVIE             0x00000020
#define IMX6_GPT_IR_IF2IE             0x00000010
#define IMX6_GPT_IR_IF1IE             0x00000008
#define IMX6_GPT_IR_OF3IE             0x00000004
#define IMX6_GPT_IR_OF2IE             0x00000002
#define IMX6_GPT_IR_OF1IE             0x00000001

/* --------------------------------------------------------------------------------- */

#define IMX6_EPIT_OFFSET_CR           0x0000
#define IMX6_EPIT_OFFSET_SR           0x0004
#define IMX6_EPIT_OFFSET_LR           0x0008
#define IMX6_EPIT_OFFSET_CMPR         0x000C
#define IMX6_EPIT_OFFSET_CNR          0x0010

#define IMX6_PHYSADDR_EPIT1_CR        (IMX6_PHYSADDR_EPIT1 + IMX6_EPIT_OFFSET_CR  )
#define IMX6_PHYSADDR_EPIT1_SR        (IMX6_PHYSADDR_EPIT1 + IMX6_EPIT_OFFSET_SR  )
#define IMX6_PHYSADDR_EPIT1_LR        (IMX6_PHYSADDR_EPIT1 + IMX6_EPIT_OFFSET_LR  )
#define IMX6_PHYSADDR_EPIT1_CMPR      (IMX6_PHYSADDR_EPIT1 + IMX6_EPIT_OFFSET_CMPR)
#define IMX6_PHYSADDR_EPIT1_CNR       (IMX6_PHYSADDR_EPIT1 + IMX6_EPIT_OFFSET_CNR )

#define IMX6_PHYSADDR_EPIT2_CR        (IMX6_PHYSADDR_EPIT2 + IMX6_EPIT_OFFSET_CR  )
#define IMX6_PHYSADDR_EPIT2_SR        (IMX6_PHYSADDR_EPIT2 + IMX6_EPIT_OFFSET_SR  )
#define IMX6_PHYSADDR_EPIT2_LR        (IMX6_PHYSADDR_EPIT2 + IMX6_EPIT_OFFSET_LR  )
#define IMX6_PHYSADDR_EPIT2_CMPR      (IMX6_PHYSADDR_EPIT2 + IMX6_EPIT_OFFSET_CMPR)
#define IMX6_PHYSADDR_EPIT2_CNR       (IMX6_PHYSADDR_EPIT2 + IMX6_EPIT_OFFSET_CNR )

#define IMX6_EPIT_CR_CLKSRC_MASK      0x03000000
#define IMX6_EPIT_CR_CLKSRC_OFF       0x00000000
#define IMX6_EPIT_CR_CLKSRC_PERIPH    0x01000000
#define IMX6_EPIT_CR_CLKSRC_HIGHFREQ  0x02000000
#define IMX6_EPIT_CR_CLKSRC_LOWFREQ   0x03000000
#define IMX6_EPIT_CR_OM_MASK          0x00C00000
#define IMX6_EPIT_CR_OM_DISCONNECT    0x00000000
#define IMX6_EPIT_CR_OM_TOGGLE        0x00400000
#define IMX6_EPIT_CR_OM_CLEAR         0x00800000
#define IMX6_EPIT_CR_OM_SET           0x00C00000
#define IMX6_EPIT_CR_STOPEN           0x00200000
#define IMX6_EPIT_CR_WAITEN           0x00080000
#define IMX6_EPIT_CR_DBGEN            0x00040000
#define IMX6_EPIT_CR_IOVW             0x00020000
#define IMX6_EPIT_CR_SWR              0x00010000
#define IMX6_EPIT_CR_PRESCALER_MASK   0x0000FFF0
#define IMX6_EPIT_CR_PRESCALER_SHL    4
#define IMX6_EPIT_CR_RLD              0x00000008
#define IMX6_EPIT_CR_OCIEN            0x00000004
#define IMX6_EPIT_CR_ENMOD            0x00000002
#define IMX6_EPIT_CR_EN               0x00000001

#define IMX6_EPIT_SR_OCIF             0x00000001

/* --------------------------------------------------------------------------------- */

#define IMX6_IRQ_IOMUXC               32
#define IMX6_IRQ_DAP                  33
#define IMX6_IRQ_SDMA                 34
#define IMX6_IRQ_VPU_JPEG             35
#define IMX6_IRQ_SNVS_POWEROFF        36
#define IMX6_IRQ_IPU1_ERROR           37
#define IMX6_IRQ_IPU1_SYNC            38
#define IMX6_IRQ_IPU2_ERROR           39
#define IMX6_IRQ_IPU2_SYNC            40
#define IMX6_IRQ_GPU3D                41
#define IMX6_IRQ_R2D_GPU2D            42
#define IMX6_IRQ_V2D_GPU2D            43
#define IMX6_IRQ_VPU                  44
#define IMX6_IRQ_APBH_DMA             45
#define IMX6_IRQ_EIM                  46
#define IMX6_IRQ_BCH                  47
#define IMX6_IRQ_GPMI                 48
#define IMX6_IRQ_DTCP                 49
#define IMX6_IRQ_VDOA                 50
#define IMX6_IRQ_SNVS_SRTC            51
#define IMX6_IRQ_SNVS_SRTC_SEC        52
#define IMX6_IRQ_CSU                  53
#define IMX6_IRQ_USDHC1               54
#define IMX6_IRQ_USDHC2               55
#define IMX6_IRQ_USDHC3               56
#define IMX6_IRQ_USDHC4               57
#define IMX6_IRQ_UART1                58
#define IMX6_IRQ_UART2                59
#define IMX6_IRQ_UART3                60
#define IMX6_IRQ_UART4                61
#define IMX6_IRQ_UART5                62
#define IMX6_IRQ_ECSPI1               63
#define IMX6_IRQ_ECSPI2               64
#define IMX6_IRQ_ECSPI3               65
#define IMX6_IRQ_ECSPI4               66
#define IMX6_IRQ_ECSPI5               67
#define IMX6_IRQ_I2C1                 68
#define IMX6_IRQ_I2C2                 69
#define IMX6_IRQ_I2C3                 70
#define IMX6_IRQ_SATA                 71
#define IMX6_IRQ_USB_HOST1            72
#define IMX6_IRQ_USB_HOST2            73
#define IMX6_IRQ_USB_HOST3            74
#define IMX6_IRQ_USB_OTG              75
#define IMX6_IRQ_USB_PHY0             76
#define IMX6_IRQ_USB_PHY1             77
#define IMX6_IRQ_SSI1                 78
#define IMX6_IRQ_SSI2                 79
#define IMX6_IRQ_SSI3                 80
#define IMX6_IRQ_TEMPMON              81
#define IMX6_IRQ_ASRC                 82
#define IMX6_IRQ_ESAI                 83
#define IMX6_IRQ_SPDIF                84
#define IMX6_IRQ_MLB150_ERROR         85
#define IMX6_IRQ_PMU                  86
#define IMX6_IRQ_GPT                  87
#define IMX6_IRQ_EPIT1                88
#define IMX6_IRQ_EPIT2                89
#define IMX6_IRQ_GPIO1_7              90
#define IMX6_IRQ_GPIO1_6              91
#define IMX6_IRQ_GPIO1_5              92
#define IMX6_IRQ_GPIO1_4              93
#define IMX6_IRQ_GPIO1_3              94
#define IMX6_IRQ_GPIO1_2              95
#define IMX6_IRQ_GPIO1_1              96
#define IMX6_IRQ_GPIO1_0              97
#define IMX6_IRQ_GPIO1_0_15           98
#define IMX6_IRQ_GPIO1_16_31          99
#define IMX6_IRQ_GPIO2_0_15           100
#define IMX6_IRQ_GPIO2_16_31          101
#define IMX6_IRQ_GPIO3_0_15           102
#define IMX6_IRQ_GPIO3_16_31          103
#define IMX6_IRQ_GPIO4_0_15           104
#define IMX6_IRQ_GPIO4_16_31          105
#define IMX6_IRQ_GPIO5_0_15           106
#define IMX6_IRQ_GPIO5_16_31          107
#define IMX6_IRQ_GPIO6_0_15           108
#define IMX6_IRQ_GPIO6_16_31          109
#define IMX6_IRQ_GPIO7_0_15           110
#define IMX6_IRQ_GPIO7_16_31          111
#define IMX6_IRQ_WDOG1                112
#define IMX6_IRQ_WDOG2                113
#define IMX6_IRQ_KPP                  114
#define IMX6_IRQ_PWM1                 115
#define IMX6_IRQ_PWM2                 116
#define IMX6_IRQ_PWM3                 117
#define IMX6_IRQ_PWM4                 118
#define IMX6_IRQ_CCM1                 119
#define IMX6_IRQ_CCM2                 120
#define IMX6_IRQ_GPC                  121
#define IMX6_IRQ_RESERVED_122         122
#define IMX6_IRQ_SRC                  123
#define IMX6_IRQ_CPU_L2               124
#define IMX6_IRQ_CPU_PARITY           125
#define IMX6_IRQ_CPU_PERF             126
#define IMX6_IRQ_CPU_CTI              127
#define IMX6_IRQ_SRC_WDOGS            128
#define IMX6_IRQ_RESERVED_129         129
#define IMX6_IRQ_RESERVED_130         130
#define IMX6_IRQ_RESERVED_131         131
#define IMX6_IRQ_MIPI_CSI1            132
#define IMX6_IRQ_MIPI_CSI2            133
#define IMX6_IRQ_MIPI_DSI             134
#define IMX6_IRQ_MIPI_HSI             135
#define IMX6_IRQ_SJC                  136
#define IMX6_IRQ_CAAM_RING0           137
#define IMX6_IRQ_CAAM_RING1           138
#define IMX6_IRQ_RESERVED_139         139
#define IMX6_IRQ_ASC1                 140
#define IMX6_IRQ_ASC2                 141
#define IMX6_IRQ_FLEXCAN1             142
#define IMX6_IRQ_FLEXCAN2             143
#define IMX6_IRQ_RESERVED_144         144
#define IMX6_IRQ_RESERVED_145         145
#define IMX6_IRQ_RESERVED_146         146
#define IMX6_IRQ_HDMI_MASTER          147
#define IMX6_IRQ_HDMI_CEC_WAKEUP      148
#define IMX6_IRQ_MLB150_CH0_31        149
#define IMX6_IRQ_ENET                 150
#define IMX6_IRQ_ENET_TIMER           151
#define IMX6_IRQ_PCIE1                152
#define IMX6_IRQ_PCIE2                153
#define IMX6_IRQ_PCIE3                154
#define IMX6_IRQ_PCIE4                155
#define IMX6_IRQ_DCIC1                156
#define IMX6_IRQ_DCIC2                157
#define IMX6_IRQ_MLB150_CH32_63       158
#define IMX6_IRQ_PMU2                 159

/* --------------------------------------------------------------------------------- */

#if 0 // XXXXXXXXXXXXXXXXXXXX

// 16 bit registers

#define IMX6_I2C_OFFSET_IADR          0x0000        
#define IMX6_I2C_OFFSET_IFDR          0x0004 
#define IMX6_I2C_OFFSET_I2CR          0x0008        
#define IMX6_I2C_OFFSET_I2SR          0x000C        
#define IMX6_I2C_OFFSET_I2DR          0x0010       

#define IMX6_PHYSADDR_I2C1_IADR       (IMX6_PHYSADDR_I2C1 + IMX6_I2C_OFFSET_IADR) 
#define IMX6_PHYSADDR_I2C1_IFDR       (IMX6_PHYSADDR_I2C1 + IMX6_I2C_OFFSET_IFDR)
#define IMX6_PHYSADDR_I2C1_I2CR       (IMX6_PHYSADDR_I2C1 + IMX6_I2C_OFFSET_I2CR) 
#define IMX6_PHYSADDR_I2C1_I2SR       (IMX6_PHYSADDR_I2C1 + IMX6_I2C_OFFSET_I2SR) 
#define IMX6_PHYSADDR_I2C1_I2DR       (IMX6_PHYSADDR_I2C1 + IMX6_I2C_OFFSET_I2DR)

#define IMX6_PHYSADDR_I2C2_IADR       (IMX6_PHYSADDR_I2C2 + IMX6_I2C_OFFSET_IADR) 
#define IMX6_PHYSADDR_I2C2_IFDR       (IMX6_PHYSADDR_I2C2 + IMX6_I2C_OFFSET_IFDR)
#define IMX6_PHYSADDR_I2C2_I2CR       (IMX6_PHYSADDR_I2C2 + IMX6_I2C_OFFSET_I2CR) 
#define IMX6_PHYSADDR_I2C2_I2SR       (IMX6_PHYSADDR_I2C2 + IMX6_I2C_OFFSET_I2SR) 
#define IMX6_PHYSADDR_I2C2_I2DR       (IMX6_PHYSADDR_I2C2 + IMX6_I2C_OFFSET_I2DR)

#define IMX6_PHYSADDR_I2C3_IADR       (IMX6_PHYSADDR_I2C3 + IMX6_I2C_OFFSET_IADR) 
#define IMX6_PHYSADDR_I2C3_IFDR       (IMX6_PHYSADDR_I2C3 + IMX6_I2C_OFFSET_IFDR)
#define IMX6_PHYSADDR_I2C3_I2CR       (IMX6_PHYSADDR_I2C3 + IMX6_I2C_OFFSET_I2CR) 
#define IMX6_PHYSADDR_I2C3_I2SR       (IMX6_PHYSADDR_I2C3 + IMX6_I2C_OFFSET_I2SR) 
#define IMX6_PHYSADDR_I2C3_I2DR       (IMX6_PHYSADDR_I2C3 + IMX6_I2C_OFFSET_I2DR)

#define IMX6_PHYSADDR_I2C4_IADR       (IMX6_PHYSADDR_I2C4 + IMX6_I2C_OFFSET_IADR) 
#define IMX6_PHYSADDR_I2C4_IFDR       (IMX6_PHYSADDR_I2C4 + IMX6_I2C_OFFSET_IFDR)
#define IMX6_PHYSADDR_I2C4_I2CR       (IMX6_PHYSADDR_I2C4 + IMX6_I2C_OFFSET_I2CR) 
#define IMX6_PHYSADDR_I2C4_I2SR       (IMX6_PHYSADDR_I2C4 + IMX6_I2C_OFFSET_I2SR) 
#define IMX6_PHYSADDR_I2C4_I2DR       (IMX6_PHYSADDR_I2C4 + IMX6_I2C_OFFSET_I2DR)

#define IMX6_I2C_IADR_ADR_MASK    0x007E

#define IMX6_I2C_IFDR_IC_MASK     0x003F

#define IMX6_I2C_I2CR_IEN         0x0080
#define IMX6_I2C_I2CR_IIEN        0x0040
#define IMX6_I2C_I2CR_MSTA        0x0020
#define IMX6_I2C_I2CR_MTX         0x0010
#define IMX6_I2C_I2CR_TXAK        0x0008
#define IMX6_I2C_I2CR_RSTA        0x0004

#define IMX6_I2C_I2SR_ICF         0x0080
#define IMX6_I2C_I2SR_IAAS        0x0040
#define IMX6_I2C_I2SR_IBB         0x0020
#define IMX6_I2C_I2SR_IAL         0x0010
#define IMX6_I2C_I2SR_SRW         0x0004
#define IMX6_I2C_I2SR_IIF         0x0002
#define IMX6_I2C_I2SR_RXAK        0x0001

#define IMX6_I2C_I2DR_DATA_MASK   0x00FF

#endif // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

/* --------------------------------------------------------------------------------- */

#define IMX6_USDHC_OFFSET_DS_ADDR                    0x00000
#define IMX6_USDHC_OFFSET_BLK_ATT                    0x00004
#define IMX6_USDHC_OFFSET_CMD_ARG                    0x00008
#define IMX6_USDHC_OFFSET_CMD_XFR_TYP                0x0000C
#define IMX6_USDHC_OFFSET_CMD_RSP0                   0x00010
#define IMX6_USDHC_OFFSET_CMD_RSP1                   0x00014
#define IMX6_USDHC_OFFSET_CMD_RSP2                   0x00018
#define IMX6_USDHC_OFFSET_CMD_RSP3                   0x0001C
#define IMX6_USDHC_OFFSET_DATA_BUFF_ACC_PORT         0x00020
#define IMX6_USDHC_OFFSET_PRES_STATE                 0x00024
#define IMX6_USDHC_OFFSET_PROT_CTRL                  0x00028
#define IMX6_USDHC_OFFSET_SYS_CTRL                   0x0002C
#define IMX6_USDHC_OFFSET_INT_STATUS                 0x00030
#define IMX6_USDHC_OFFSET_INT_STATUS_EN              0x00034
#define IMX6_USDHC_OFFSET_INT_SIGNAL_EN              0x00038
#define IMX6_USDHC_OFFSET_AUTOCMD12_ERR_STATUS       0x0003C
#define IMX6_USDHC_OFFSET_HOST_CTRL_CAP              0x00040
#define IMX6_USDHC_OFFSET_WTMK_LVL                   0x00044
#define IMX6_USDHC_OFFSET_MIX_CTRL                   0x00048
#define IMX6_USDHC_OFFSET_FORCE_EVENT                0x00050
#define IMX6_USDHC_OFFSET_ADMA_ERR_STATUS            0x00054
#define IMX6_USDHC_OFFSET_ADMA_SYS_ADDR              0x00058
#define IMX6_USDHC_OFFSET_DLL_CTRL                   0x00060
#define IMX6_USDHC_OFFSET_DLL_STATUS                 0x00064
#define IMX6_USDHC_OFFSET_CLK_TUNE_CTRL_STATUS       0x00068
#define IMX6_USDHC_OFFSET_VEND_SPEC                  0x000C0
#define IMX6_USDHC_OFFSET_MMC_BOOT                   0x000C4
#define IMX6_USDHC_OFFSET_VEND_SPEC2                 0x000C8
#define IMX6_USDHC_OFFSET_TUNING_CTRL                0x000CC

#define IMX6_PHYSADDR_USDHC1_DS_ADDR                 (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_DS_ADDR             )
#define IMX6_PHYSADDR_USDHC1_BLK_ATT                 (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_BLK_ATT             )
#define IMX6_PHYSADDR_USDHC1_CMD_ARG                 (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_CMD_ARG             )
#define IMX6_PHYSADDR_USDHC1_CMD_XFR_TYP             (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_CMD_XFR_TYP         )
#define IMX6_PHYSADDR_USDHC1_CMD_RSP0                (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_CMD_RSP0            )
#define IMX6_PHYSADDR_USDHC1_CMD_RSP1                (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_CMD_RSP1            )
#define IMX6_PHYSADDR_USDHC1_CMD_RSP2                (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_CMD_RSP2            )
#define IMX6_PHYSADDR_USDHC1_CMD_RSP3                (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_CMD_RSP3            )
#define IMX6_PHYSADDR_USDHC1_DATA_BUFF_ACC_PORT      (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_DATA_BUFF_ACC_PORT  )
#define IMX6_PHYSADDR_USDHC1_PRES_STATE              (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_PRES_STATE          )
#define IMX6_PHYSADDR_USDHC1_PROT_CTRL               (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_PROT_CTRL           )
#define IMX6_PHYSADDR_USDHC1_SYS_CTRL                (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_SYS_CTRL            )
#define IMX6_PHYSADDR_USDHC1_INT_STATUS              (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_INT_STATUS          )
#define IMX6_PHYSADDR_USDHC1_INT_STATUS_EN           (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_INT_STATUS_EN       )
#define IMX6_PHYSADDR_USDHC1_INT_SIGNAL_EN           (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_INT_SIGNAL_EN       )
#define IMX6_PHYSADDR_USDHC1_AUTOCMD12_ERR_STATUS    (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_AUTOCMD12_ERR_STATUS)
#define IMX6_PHYSADDR_USDHC1_HOST_CTRL_CAP           (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_HOST_CTRL_CAP       )
#define IMX6_PHYSADDR_USDHC1_WTMK_LVL                (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_WTMK_LVL            )
#define IMX6_PHYSADDR_USDHC1_MIX_CTRL                (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_MIX_CTRL            )
#define IMX6_PHYSADDR_USDHC1_FORCE_EVENT             (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_FORCE_EVENT         )
#define IMX6_PHYSADDR_USDHC1_ADMA_ERR_STATUS         (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_ADMA_ERR_STATUS     )
#define IMX6_PHYSADDR_USDHC1_ADMA_SYS_ADDR           (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_ADMA_SYS_ADDR       )
#define IMX6_PHYSADDR_USDHC1_DLL_CTRL                (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_DLL_CTRL            )
#define IMX6_PHYSADDR_USDHC1_DLL_STATUS              (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_DLL_STATUS          )
#define IMX6_PHYSADDR_USDHC1_CLK_TUNE_CTRL_STATUS    (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_CLK_TUNE_CTRL_STATUS)
#define IMX6_PHYSADDR_USDHC1_VEND_SPEC               (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_VEND_SPEC           )
#define IMX6_PHYSADDR_USDHC1_MMC_BOOT                (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_MMC_BOOT            )
#define IMX6_PHYSADDR_USDHC1_VEND_SPEC2              (IMX6_PHYSADDR_USDHC1 + IMX6_USDHC_OFFSET_VEND_SPEC2          )

#define IMX6_PHYSADDR_USDHC2_DS_ADDR                 (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_DS_ADDR             )
#define IMX6_PHYSADDR_USDHC2_BLK_ATT                 (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_BLK_ATT             )
#define IMX6_PHYSADDR_USDHC2_CMD_ARG                 (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_CMD_ARG             )
#define IMX6_PHYSADDR_USDHC2_CMD_XFR_TYP             (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_CMD_XFR_TYP         )
#define IMX6_PHYSADDR_USDHC2_CMD_RSP0                (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_CMD_RSP0            )
#define IMX6_PHYSADDR_USDHC2_CMD_RSP1                (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_CMD_RSP1            )
#define IMX6_PHYSADDR_USDHC2_CMD_RSP2                (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_CMD_RSP2            )
#define IMX6_PHYSADDR_USDHC2_CMD_RSP3                (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_CMD_RSP3            )
#define IMX6_PHYSADDR_USDHC2_DATA_BUFF_ACC_PORT      (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_DATA_BUFF_ACC_PORT  )
#define IMX6_PHYSADDR_USDHC2_PRES_STATE              (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_PRES_STATE          )
#define IMX6_PHYSADDR_USDHC2_PROT_CTRL               (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_PROT_CTRL           )
#define IMX6_PHYSADDR_USDHC2_SYS_CTRL                (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_SYS_CTRL            )
#define IMX6_PHYSADDR_USDHC2_INT_STATUS              (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_INT_STATUS          )
#define IMX6_PHYSADDR_USDHC2_INT_STATUS_EN           (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_INT_STATUS_EN       )
#define IMX6_PHYSADDR_USDHC2_INT_SIGNAL_EN           (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_INT_SIGNAL_EN       )
#define IMX6_PHYSADDR_USDHC2_AUTOCMD12_ERR_STATUS    (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_AUTOCMD12_ERR_STATUS)
#define IMX6_PHYSADDR_USDHC2_HOST_CTRL_CAP           (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_HOST_CTRL_CAP       )
#define IMX6_PHYSADDR_USDHC2_WTMK_LVL                (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_WTMK_LVL            )
#define IMX6_PHYSADDR_USDHC2_MIX_CTRL                (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_MIX_CTRL            )
#define IMX6_PHYSADDR_USDHC2_FORCE_EVENT             (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_FORCE_EVENT         )
#define IMX6_PHYSADDR_USDHC2_ADMA_ERR_STATUS         (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_ADMA_ERR_STATUS     )
#define IMX6_PHYSADDR_USDHC2_ADMA_SYS_ADDR           (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_ADMA_SYS_ADDR       )
#define IMX6_PHYSADDR_USDHC2_DLL_CTRL                (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_DLL_CTRL            )
#define IMX6_PHYSADDR_USDHC2_DLL_STATUS              (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_DLL_STATUS          )
#define IMX6_PHYSADDR_USDHC2_CLK_TUNE_CTRL_STATUS    (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_CLK_TUNE_CTRL_STATUS)
#define IMX6_PHYSADDR_USDHC2_VEND_SPEC               (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_VEND_SPEC           )
#define IMX6_PHYSADDR_USDHC2_MMC_BOOT                (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_MMC_BOOT            )
#define IMX6_PHYSADDR_USDHC2_VEND_SPEC2              (IMX6_PHYSADDR_USDHC2 + IMX6_USDHC_OFFSET_VEND_SPEC2          )

#define IMX6_PHYSADDR_USDHC3_DS_ADDR                 (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_DS_ADDR             )
#define IMX6_PHYSADDR_USDHC3_BLK_ATT                 (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_BLK_ATT             )
#define IMX6_PHYSADDR_USDHC3_CMD_ARG                 (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_CMD_ARG             )
#define IMX6_PHYSADDR_USDHC3_CMD_XFR_TYP             (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_CMD_XFR_TYP         )
#define IMX6_PHYSADDR_USDHC3_CMD_RSP0                (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_CMD_RSP0            )
#define IMX6_PHYSADDR_USDHC3_CMD_RSP1                (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_CMD_RSP1            )
#define IMX6_PHYSADDR_USDHC3_CMD_RSP2                (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_CMD_RSP2            )
#define IMX6_PHYSADDR_USDHC3_CMD_RSP3                (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_CMD_RSP3            )
#define IMX6_PHYSADDR_USDHC3_DATA_BUFF_ACC_PORT      (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_DATA_BUFF_ACC_PORT  )
#define IMX6_PHYSADDR_USDHC3_PRES_STATE              (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_PRES_STATE          )
#define IMX6_PHYSADDR_USDHC3_PROT_CTRL               (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_PROT_CTRL           )
#define IMX6_PHYSADDR_USDHC3_SYS_CTRL                (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_SYS_CTRL            )
#define IMX6_PHYSADDR_USDHC3_INT_STATUS              (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_INT_STATUS          )
#define IMX6_PHYSADDR_USDHC3_INT_STATUS_EN           (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_INT_STATUS_EN       )
#define IMX6_PHYSADDR_USDHC3_INT_SIGNAL_EN           (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_INT_SIGNAL_EN       )
#define IMX6_PHYSADDR_USDHC3_AUTOCMD12_ERR_STATUS    (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_AUTOCMD12_ERR_STATUS)
#define IMX6_PHYSADDR_USDHC3_HOST_CTRL_CAP           (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_HOST_CTRL_CAP       )
#define IMX6_PHYSADDR_USDHC3_WTMK_LVL                (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_WTMK_LVL            )
#define IMX6_PHYSADDR_USDHC3_MIX_CTRL                (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_MIX_CTRL            )
#define IMX6_PHYSADDR_USDHC3_FORCE_EVENT             (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_FORCE_EVENT         )
#define IMX6_PHYSADDR_USDHC3_ADMA_ERR_STATUS         (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_ADMA_ERR_STATUS     )
#define IMX6_PHYSADDR_USDHC3_ADMA_SYS_ADDR           (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_ADMA_SYS_ADDR       )
#define IMX6_PHYSADDR_USDHC3_DLL_CTRL                (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_DLL_CTRL            )
#define IMX6_PHYSADDR_USDHC3_DLL_STATUS              (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_DLL_STATUS          )
#define IMX6_PHYSADDR_USDHC3_CLK_TUNE_CTRL_STATUS    (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_CLK_TUNE_CTRL_STATUS)
#define IMX6_PHYSADDR_USDHC3_VEND_SPEC               (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_VEND_SPEC           )
#define IMX6_PHYSADDR_USDHC3_MMC_BOOT                (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_MMC_BOOT            )
#define IMX6_PHYSADDR_USDHC3_VEND_SPEC2              (IMX6_PHYSADDR_USDHC3 + IMX6_USDHC_OFFSET_VEND_SPEC2          )

#define IMX6_PHYSADDR_USDHC4_DS_ADDR                 (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_DS_ADDR             )
#define IMX6_PHYSADDR_USDHC4_BLK_ATT                 (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_BLK_ATT             )
#define IMX6_PHYSADDR_USDHC4_CMD_ARG                 (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_CMD_ARG             )
#define IMX6_PHYSADDR_USDHC4_CMD_XFR_TYP             (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_CMD_XFR_TYP         )
#define IMX6_PHYSADDR_USDHC4_CMD_RSP0                (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_CMD_RSP0            )
#define IMX6_PHYSADDR_USDHC4_CMD_RSP1                (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_CMD_RSP1            )
#define IMX6_PHYSADDR_USDHC4_CMD_RSP2                (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_CMD_RSP2            )
#define IMX6_PHYSADDR_USDHC4_CMD_RSP3                (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_CMD_RSP3            )
#define IMX6_PHYSADDR_USDHC4_DATA_BUFF_ACC_PORT      (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_DATA_BUFF_ACC_PORT  )
#define IMX6_PHYSADDR_USDHC4_PRES_STATE              (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_PRES_STATE          )
#define IMX6_PHYSADDR_USDHC4_PROT_CTRL               (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_PROT_CTRL           )
#define IMX6_PHYSADDR_USDHC4_SYS_CTRL                (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_SYS_CTRL            )
#define IMX6_PHYSADDR_USDHC4_INT_STATUS              (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_INT_STATUS          )
#define IMX6_PHYSADDR_USDHC4_INT_STATUS_EN           (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_INT_STATUS_EN       )
#define IMX6_PHYSADDR_USDHC4_INT_SIGNAL_EN           (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_INT_SIGNAL_EN       )
#define IMX6_PHYSADDR_USDHC4_AUTOCMD12_ERR_STATUS    (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_AUTOCMD12_ERR_STATUS)
#define IMX6_PHYSADDR_USDHC4_HOST_CTRL_CAP           (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_HOST_CTRL_CAP       )
#define IMX6_PHYSADDR_USDHC4_WTMK_LVL                (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_WTMK_LVL            )
#define IMX6_PHYSADDR_USDHC4_MIX_CTRL                (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_MIX_CTRL            )
#define IMX6_PHYSADDR_USDHC4_FORCE_EVENT             (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_FORCE_EVENT         )
#define IMX6_PHYSADDR_USDHC4_ADMA_ERR_STATUS         (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_ADMA_ERR_STATUS     )
#define IMX6_PHYSADDR_USDHC4_ADMA_SYS_ADDR           (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_ADMA_SYS_ADDR       )
#define IMX6_PHYSADDR_USDHC4_DLL_CTRL                (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_DLL_CTRL            )
#define IMX6_PHYSADDR_USDHC4_DLL_STATUS              (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_DLL_STATUS          )
#define IMX6_PHYSADDR_USDHC4_CLK_TUNE_CTRL_STATUS    (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_CLK_TUNE_CTRL_STATUS)
#define IMX6_PHYSADDR_USDHC4_VEND_SPEC               (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_VEND_SPEC           )
#define IMX6_PHYSADDR_USDHC4_MMC_BOOT                (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_MMC_BOOT            )
#define IMX6_PHYSADDR_USDHC4_VEND_SPEC2              (IMX6_PHYSADDR_USDHC4 + IMX6_USDHC_OFFSET_VEND_SPEC2          )

// mask of defined interrupts in INT_STATUS, INT_STATUS_EN, INT_SIGNAL_EN
#define IMX6_USDHC_INTREG_MASK    0x157F51FF

#define IMX6_USDHC_CMD_XFR_TYP_CMDINX_SHL 24
#define IMX6_USDHC_CMD_XFR_TYP_CMDTYP_SHL 22
#define IMX6_USDHC_CMD_XFR_TYP_DPSEL_SHL  21
#define IMX6_USDHC_CMD_XFR_TYP_CICEN_SHL  20
#define IMX6_USDHC_CMD_XFR_TYP_CCCEN_SHL  19
#define IMX6_USDHC_CMD_XFR_TYP_RSPTYP_SHL 16

#define IMX6_USDHC_CMD(inx,typ,dpsel,cicen,cccen,rsptyp,chkerr) \
   ((((inx)&0x3F)<<IMX6_USDHC_CMD_XFR_TYP_CMDINX_SHL) |\
    (((typ)&3)<<IMX6_USDHC_CMD_XFR_TYP_CMDTYP_SHL)  |\
    (((dpsel)&1)<<IMX6_USDHC_CMD_XFR_TYP_DPSEL_SHL) |\
    (((cicen)&1)<<IMX6_USDHC_CMD_XFR_TYP_CICEN_SHL) |\
    (((cccen)&1)<<IMX6_USDHC_CMD_XFR_TYP_CCCEN_SHL) |\
    (((rsptyp)&3)<<IMX6_USDHC_CMD_XFR_TYP_RSPTYP_SHL) |\
    ((chkerr)&1))

#define IMX6_USDHC_CMD0   IMX6_USDHC_CMD( 0,0,0, 0,0,0, 0)
#define IMX6_USDHC_CMD1   IMX6_USDHC_CMD( 1,0,0, 0,0,2, 1)
#define IMX6_USDHC_CMD2   IMX6_USDHC_CMD( 2,0,0, 0,1,1, 0)
#define IMX6_USDHC_CMD3   IMX6_USDHC_CMD( 3,0,0, 1,1,2, 1)
#define IMX6_USDHC_CMD7   IMX6_USDHC_CMD( 7,0,0, 1,1,3, 1)
#define IMX6_USDHC_CMD8   IMX6_USDHC_CMD( 8,0,0, 1,1,2, 1)
#define IMX6_USDHC_CMD9   IMX6_USDHC_CMD( 9,0,0, 0,1,1, 1)
#define IMX6_USDHC_CMD16  IMX6_USDHC_CMD(16,0,0, 1,1,2, 1)
#define IMX6_USDHC_CMD17  IMX6_USDHC_CMD(17,0,1, 1,1,2, 1)
#define IMX6_USDHC_CMD18  IMX6_USDHC_CMD(18,0,1, 1,1,2, 1)
#define IMX6_USDHC_CMD23  IMX6_USDHC_CMD(23,0,0, 1,1,2, 1)
#define IMX6_USDHC_CMD55  IMX6_USDHC_CMD(55,0,0, 1,1,2, 1)
#define IMX6_USDHC_CMD58  IMX6_USDHC_CMD(58,0,0, 1,1,2, 1)
#define IMX6_USDHC_ACMD6  IMX6_USDHC_CMD( 6,0,0, 1,1,2, 1)
#define IMX6_USDHC_ACMD41 IMX6_USDHC_CMD(41,0,0, 0,0,2, 1)

/* --------------------------------------------------------------------------------- */

#define IMX6_SNVS_OFFSET_HPLR             0x0000 
#define IMX6_SNVS_OFFSET_HPCOMR           0x0004 
#define IMX6_SNVS_OFFSET_HPCR             0x0008 
#define IMX6_SNVS_OFFSET_HPSR             0x0014 
#define IMX6_SNVS_OFFSET_HPRTCMR          0x0024  
#define IMX6_SNVS_OFFSET_HPRTCLR          0x0028  
#define IMX6_SNVS_OFFSET_HPTAMR           0x002C 
#define IMX6_SNVS_OFFSET_HPTALR           0x0030 
#define IMX6_SNVS_OFFSET_LPLR             0x0034 
#define IMX6_SNVS_OFFSET_LPCR             0x0038 
#define IMX6_SNVS_OFFSET_LPSR             0x004C 
#define IMX6_SNVS_OFFSET_LPSMCMR          0x005C  
#define IMX6_SNVS_OFFSET_LPSMCLR          0x0060  
#define IMX6_SNVS_OFFSET_LPGPR            0x0068 
#define IMX6_SNVS_OFFSET_VIDR1            0x0BF8  
#define IMX6_SNVS_OFFSET_VIDR2            0x0BFC  

#define IMX6_PHYSADDR_SNVS_HPLR           (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_HPLR   )
#define IMX6_PHYSADDR_SNVS_HPCOMR         (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_HPCOMR )
#define IMX6_PHYSADDR_SNVS_HPCR           (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_HPCR   )
#define IMX6_PHYSADDR_SNVS_HPSR           (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_HPSR   )
#define IMX6_PHYSADDR_SNVS_HPRTCMR        (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_HPRTCMR)
#define IMX6_PHYSADDR_SNVS_HPRTCLR        (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_HPRTCLR)
#define IMX6_PHYSADDR_SNVS_HPTAMR         (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_HPTAMR )
#define IMX6_PHYSADDR_SNVS_HPTALR         (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_HPTALR )
#define IMX6_PHYSADDR_SNVS_LPLR           (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_LPLR   )
#define IMX6_PHYSADDR_SNVS_LPCR           (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_LPCR   )
#define IMX6_PHYSADDR_SNVS_LPSR           (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_LPSR   )
#define IMX6_PHYSADDR_SNVS_LPSMCMR        (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_LPSMCMR)
#define IMX6_PHYSADDR_SNVS_LPSMCLR        (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_LPSMCLR)
#define IMX6_PHYSADDR_SNVS_LPGPR          (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_LPGPR  )
#define IMX6_PHYSADDR_SNVS_HPVIDR1        (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_VIDR1)
#define IMX6_PHYSADDR_SNVS_HPVIDR2        (IMX6_PHYSADDR_SNVS + IMX6_SNVS_OFFSET_VIDR2)

#define IMX6_SNVS_HPLR_GPR_SL                 0x00000020
#define IMX6_SNVS_HPLR_MC_SL                  0x00000010

#define IMX6_SNVS_HPCOMR_NPSWA_EN             0x80000000
#define IMX6_SNVS_HPCOMR_LP_SWR_DIS           0x00000020
#define IMX6_SNVS_HPCOMR_LP_SWR               0x00000010

#define IMX6_SNVS_HPCR_HPCALB_VAL_MASK        0x00007C00
#define IMX6_SNVS_HPCR_HPCALB_EN              0x00000100
#define IMX6_SNVS_HPCR_PI_FREQ                0x000000F0
#define IMX6_SNVS_HPCR_PI_EN                  0x00000008 
#define IMX6_SNVS_HPCR_HPTA_EN                0x00000002
#define IMX6_SNVS_HPCR_RTC_EN                 0x00000001

#define IMX6_SNVS_HPSR_BI                     0x00000080
#define IMX6_SNVS_HPSR_BTN                    0x00000040

#define IMX6_SNVS_LPLR_GPR_HL                 0x00000020
#define IMX6_SNVS_LPLR_MC_HL                  0x00000010

#define IMX6_SNVS_LPCR_PK_OVERRIDE            0x00800000
#define IMX6_SNVS_LPCR_PK_EN                  0x00400000
#define IMX6_SNVS_LPCR_ON_TIME_MASK           0x00300000
#define IMX6_SNVS_LPCR_DEBOUNCE_MASK          0x000C0000
#define IMX6_SNVS_LPCR_BTN_PRESS_TIME_MASK    0x00030000
#define IMX6_SNVS_LPCR_PWR_GLITCH_EN          0x00000080
#define IMX6_SNVS_LPCR_TOP                    0x00000040
#define IMX6_SNVS_LPCR_DP_EN                  0x00000020
#define IMX6_SNVS_LPCR_MC_ENV                 0x00000004

#define IMX6_SNVS_LPSR_SPO                    0x00040000
#define IMX6_SNVS_LPSR_EO                     0x00020000
#define IMX6_SNVS_LPSR_MCR                    0x00000004

/* --------------------------------------------------------------------------------- */

#define IMX6_SRC_OFFSET_SCR        0x0000 
#define IMX6_SRC_OFFSET_SBMR1      0x0004 
#define IMX6_SRC_OFFSET_SRSR       0x0008 
#define IMX6_SRC_OFFSET_SISR       0x0014 
#define IMX6_SRC_OFFSET_SIMR       0x0018 
#define IMX6_SRC_OFFSET_SBMR2      0x001C 
#define IMX6_SRC_OFFSET_GPR1       0x0020 
#define IMX6_SRC_OFFSET_GPR2       0x0024 
#define IMX6_SRC_OFFSET_GPR3       0x0028 
#define IMX6_SRC_OFFSET_GPR4       0x002C 
#define IMX6_SRC_OFFSET_GPR5       0x0030 
#define IMX6_SRC_OFFSET_GPR6       0x0034 
#define IMX6_SRC_OFFSET_GPR7       0x0038 
#define IMX6_SRC_OFFSET_GPR8       0x003C 
#define IMX6_SRC_OFFSET_GPR9       0x0040 
#define IMX6_SRC_OFFSET_GPR10      0x0044 

#define IMX6_PHYSADDR_SRC_SCR        (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_SCR  )
#define IMX6_PHYSADDR_SRC_SBMR1      (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_SBMR1)
#define IMX6_PHYSADDR_SRC_SRSR       (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_SRSR )
#define IMX6_PHYSADDR_SRC_SISR       (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_SISR )
#define IMX6_PHYSADDR_SRC_SIMR       (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_SIMR )
#define IMX6_PHYSADDR_SRC_SBMR2      (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_SBMR2)
#define IMX6_PHYSADDR_SRC_GPR1       (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_GPR1 )
#define IMX6_PHYSADDR_SRC_GPR2       (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_GPR2 )
#define IMX6_PHYSADDR_SRC_GPR3       (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_GPR3 )
#define IMX6_PHYSADDR_SRC_GPR4       (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_GPR4 )
#define IMX6_PHYSADDR_SRC_GPR5       (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_GPR5 )
#define IMX6_PHYSADDR_SRC_GPR6       (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_GPR6 )
#define IMX6_PHYSADDR_SRC_GPR7       (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_GPR7 )
#define IMX6_PHYSADDR_SRC_GPR8       (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_GPR8 )
#define IMX6_PHYSADDR_SRC_GPR9       (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_GPR9 )
#define IMX6_PHYSADDR_SRC_GPR10      (IMX6_PHYSADDR_SRC + IMX6_SRC_OFFSET_GPR10)

#define IMX6_SRC_SCR_DBG_RST_MSK_PG                 0x02000000
#define IMX6_SRC_SCR_CORE3_ENABLE                   0x01000000
#define IMX6_SRC_SCR_CORE2_ENABLE                   0x00800000
#define IMX6_SRC_SCR_CORE1_ENABLE                   0x00400000
#define IMX6_SRC_SCR_CORES_DBG_RST                  0x00200000
#define IMX6_SRC_SCR_CORE3_DBG_RST                  0x00100000
#define IMX6_SRC_SCR_CORE2_DBG_RST                  0x00080000
#define IMX6_SRC_SCR_CORE1_DBG_RST                  0x00040000
#define IMX6_SRC_SCR_CORE0_DBG_RST                  0x00020000
#define IMX6_SRC_SCR_CORE3_RST                      0x00010000
#define IMX6_SRC_SCR_CORE2_RST                      0x00008000
#define IMX6_SRC_SCR_CORE1_RST                      0x00004000
#define IMX6_SRC_SCR_CORE0_RST                      0x00002000
#define IMX6_SRC_SCR_SW_IPU2_RST                    0x00001000
#define IMX6_SRC_SCR_EIM_RST                        0x00000800
#define IMX6_SRC_SCR_MASK_WDOG_RST_MASK             0x00000780
#define IMX6_SRC_SCR_WARM_RST_BYPASS_COUNT_MASK     0x00000060
#define IMX6_SRC_SCR_SW_OPEN_VG_RST                 0x00000010
#define IMX6_SRC_SCR_SW_IPU1_RST                    0x00000008
#define IMX6_SRC_SCR_SW_VPU_RST                     0x00000004
#define IMX6_SRC_SCR_SW_GPU_RST                     0x00000002
#define IMX6_SRC_SCR_WARM_RESET_ENABLE              0x00000001

#define IMX6_SRC_SBMR1_BOOT_CFG1_MASK               0x000000FF
#define IMX6_SRC_SBMR1_BOOT_CFG2_MASK               0x0000FF00
#define IMX6_SRC_SBMR1_BOOT_CFG2_SHL                8
#define IMX6_SRC_SBMR1_BOOT_CFG3_MASK               0x00FF0000
#define IMX6_SRC_SBMR1_BOOT_CFG3_SHL                16
#define IMX6_SRC_SBMR1_BOOT_CFG4_MASK               0xFF000000
#define IMX6_SRC_SBMR1_BOOT_CFG4_SHL                24

#define IMX6_SRC_SBMR2_BMOD_MASK                    0x03000000
#define IMX6_SRC_SBMR2_BMOD_FROM_FUSES              0x00000000
#define IMX6_SRC_SBMR2_BMOD_FROM_SERIAL_DL          0x01000000
#define IMX6_SRC_SBMR2_BMOD_FROM_INTERNAL           0x02000000
#define IMX6_SRC_SBMR2_BMOD_BT_FUSE_SEL             0x00000010
#define IMX6_SRC_SBMR2_BMOD_DIR_BT_DIS              0x00000008
#define IMX6_SRC_SBMR2_BMOD_SEC_CONFIG_OPEN         0x00000001
#define IMX6_SRC_SBMR2_BMOD_SEC_CONFIG_CLOSED       0x00000002

#define IMX6_SRSR_WARM_BOOT                         0x00010000
#define IMX6_SRSR_JTAG_SW_RST                       0x00000040
#define IMX6_SRSR_JTAG_RST_B                        0x00000020
#define IMX6_SRSR_WDOG_RST_B                        0x00000010
#define IMX6_SRSR_IPP_USER_RESET_B                  0x00000008
#define IMX6_SRSR_CSU_RESET_B                       0x00000004
#define IMX6_SRSR_IPP_RESET_B                       0x00000001

/* --------------------------------------------------------------------------------- */

#define IMX6_PHYSSIZE_USBO3H_USB                          0x4000

#define IMX6_USBO3H_OFFSET_USB_UOG                        0x0000
#define IMX6_USBO3H_OFFSET_USB_UH1                        0x0200
#define IMX6_USBO3H_OFFSET_USB_UH2                        0x0400
#define IMX6_USBO3H_OFFSET_USB_UH3                        0x0600

#define IXM6_PHYSADDR_USB_UOG                             (IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USB_UOG)
#define IXM6_PHYSADDR_USB_UH1                             (IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USB_UH1)
#define IXM6_PHYSADDR_USB_UH2                             (IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USB_UH2)
#define IXM6_PHYSADDR_USB_UH3                             (IMX6_PHYSADDR_USBOH3 + IMX6_USBO3H_OFFSET_USB_UH3)

#define IMX6_USBCORE_OFFSET_ID                            0x0000 
#define IMX6_USBCORE_OFFSET_HWGENERAL                     0x0004 
#define IMX6_USBCORE_OFFSET_HWHOST                        0x0008 
#define IMX6_USBCORE_OFFSET_HWDEVICE                      0x000C 
#define IMX6_USBCORE_OFFSET_HWTXBUF                       0x0010 
#define IMX6_USBCORE_OFFSET_HWRXBUF                       0x0014 
#define IMX6_USBCORE_OFFSET_GPTIMER0LD                    0x0080 
#define IMX6_USBCORE_OFFSET_GPTIMER0CTRL                  0x0084 
#define IMX6_USBCORE_OFFSET_GPTIMER1LD                    0x0088 
#define IMX6_USBCORE_OFFSET_GPTIMER1CTRL                  0x008C 
#define IMX6_USBCORE_OFFSET_SBUSCFG                       0x0090 
#define IMX6_USBCORE_OFFSET_CAPLENGTH_8                   0x0100 
#define IMX6_USBCORE_OFFSET_HCIVERSION_16                 0x0102 
#define IMX6_USBCORE_OFFSET_HCSPARAMS                     0x0104 
#define IMX6_USBCORE_OFFSET_HCCPARAMS                     0x0108 
#define IMX6_USBCORE_OFFSET_DCIVERSION                    0x0120 
#define IMX6_USBCORE_OFFSET_DCCPARAMS                     0x0124 

#define IMX6_USBCORE_OFFSET_USBCMD                        0x0140 
#define IMX6_USBCORE_OFFSET_USBSTS                        0x0144 
#define IMX6_USBCORE_OFFSET_USBINTR                       0x0148 
#define IMX6_USBCORE_OFFSET_FRINDEX                       0x014C 
#define IMX6_USBCORE_OFFSET_PERIODICLISTBASE              0x0154 
#define IMX6_USBCORE_OFFSET_DEVICEADDR                    0x0154 
#define IMX6_USBCORE_OFFSET_ASYNCLISTADDR                 0x0158 
#define IMX6_USBCORE_OFFSET_ENDPTLISTADDR                 0x0158 
#define IMX6_USBCORE_OFFSET_BURSTSIZE                     0x0160 
#define IMX6_USBCORE_OFFSET_TXFILLTUNING                  0x0164 
#define IMX6_USBCORE_OFFSET_ENDPTNAK                      0x0178 
#define IMX6_USBCORE_OFFSET_ENDPTNAKEN                    0x017C 
#define IMX6_USBCORE_OFFSET_CONFIGFLAG                    0x0180 
#define IMX6_USBCORE_OPREG_SIZEBYTES                      0x0044    // USBCMD to beginning of PORTSC1
#define IMX6_USBCORE_OFFSET_PORTSC1                       0x0184 

#define IMX6_USBCORE_OFFSET_OTGSC                         0x01A4 
#define IMX6_USBCORE_OFFSET_USBMODE                       0x01A8 
#define IMX6_USBCORE_OFFSET_ENDPTSETUPSTAT                0x01AC 
#define IMX6_USBCORE_OFFSET_ENDPTPRIME                    0x01B0 
#define IMX6_USBCORE_OFFSET_ENDPTFLUSH                    0x01B4 
#define IMX6_USBCORE_OFFSET_ENDPTSTAT                     0x01B8 
#define IMX6_USBCORE_OFFSET_ENDPTCOMPLETE                 0x01BC 
#define IMX6_USBCORE_OFFSET_ENDPTCTRL0                    0x01C0 
#define IMX6_USBCORE_OFFSET_ENDPTCTRL1                    0x01C4 
#define IMX6_USBCORE_OFFSET_ENDPTCTRL2                    0x01C8 
#define IMX6_USBCORE_OFFSET_ENDPTCTRL3                    0x01CC 
#define IMX6_USBCORE_OFFSET_ENDPTCTRL4                    0x01D0 
#define IMX6_USBCORE_OFFSET_ENDPTCTRL5                    0x01D4 
#define IMX6_USBCORE_OFFSET_ENDPTCTRL6                    0x01D8 
#define IMX6_USBCORE_OFFSET_ENDPTCTRL7                    0x01DC 


#define IMX6_USBO3H_OFFSET_USB_UOG_ID                     (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ID              )
#define IMX6_USBO3H_OFFSET_USB_UOG_HWGENERAL              (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_HWGENERAL       )
#define IMX6_USBO3H_OFFSET_USB_UOG_HWHOST                 (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_HWHOST          )
#define IMX6_USBO3H_OFFSET_USB_UOG_HWDEVICE               (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_HWDEVICE        )
#define IMX6_USBO3H_OFFSET_USB_UOG_HWTXBUF                (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_HWTXBUF         )
#define IMX6_USBO3H_OFFSET_USB_UOG_HWRXBUF                (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_HWRXBUF         )
#define IMX6_USBO3H_OFFSET_USB_UOG_GPTIMER0LD             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_GPTIMER0LD      )
#define IMX6_USBO3H_OFFSET_USB_UOG_GPTIMER0CTRL           (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_GPTIMER0CTRL    )
#define IMX6_USBO3H_OFFSET_USB_UOG_GPTIMER1LD             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_GPTIMER1LD      )
#define IMX6_USBO3H_OFFSET_USB_UOG_GPTIMER1CTRL           (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_GPTIMER1CTRL    )
#define IMX6_USBO3H_OFFSET_USB_UOG_SBUSCFG                (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_SBUSCFG         )
#define IMX6_USBO3H_OFFSET_USB_UOG_CAPLENGTH_8            (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_CAPLENGTH_8     )
#define IMX6_USBO3H_OFFSET_USB_UOG_HCIVERSION_16          (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_HCIVERSION_16   )
#define IMX6_USBO3H_OFFSET_USB_UOG_HCSPARAMS              (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_HCSPARAMS       )
#define IMX6_USBO3H_OFFSET_USB_UOG_HCCPARAMS              (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_HCCPARAMS       )
#define IMX6_USBO3H_OFFSET_USB_UOG_DCIVERSION             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_DCIVERSION      )
#define IMX6_USBO3H_OFFSET_USB_UOG_DCCPARAMS              (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_DCCPARAMS       )
#define IMX6_USBO3H_OFFSET_USB_UOG_USBCMD                 (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_USBCMD          )
#define IMX6_USBO3H_OFFSET_USB_UOG_USBSTS                 (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_USBSTS          )
#define IMX6_USBO3H_OFFSET_USB_UOG_USBINTR                (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_USBINTR         )
#define IMX6_USBO3H_OFFSET_USB_UOG_FRINDEX                (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_FRINDEX         )
#define IMX6_USBO3H_OFFSET_USB_UOG_PERIODICLISTBASE       (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_PERIODICLISTBASE)
#define IMX6_USBO3H_OFFSET_USB_UOG_DEVICEADDR             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_DEVICEADDR      )
#define IMX6_USBO3H_OFFSET_USB_UOG_ASYNCLISTADDR          (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ASYNCLISTADDR   )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTLISTADDR          (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTLISTADDR   )
#define IMX6_USBO3H_OFFSET_USB_UOG_BURSTSIZE              (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_BURSTSIZE       )
#define IMX6_USBO3H_OFFSET_USB_UOG_TXFILLTUNING           (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_TXFILLTUNING    )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTNAK               (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTNAK        )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTNAKEN             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTNAKEN      )
#define IMX6_USBO3H_OFFSET_USB_UOG_CONFIGFLAG             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_CONFIGFLAG      )
#define IMX6_USBO3H_OFFSET_USB_UOG_PORTSC1                (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_PORTSC1         )
#define IMX6_USBO3H_OFFSET_USB_UOG_OTGSC                  (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_OTGSC           )
#define IMX6_USBO3H_OFFSET_USB_UOG_USBMODE                (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_USBMODE         )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTSETUPSTAT         (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTSETUPSTAT  )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTPRIME             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTPRIME      )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTFLUSH             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTFLUSH      )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTSTAT              (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTSTAT       )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTCOMPLETE          (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTCOMPLETE   )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTCTRL0             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTCTRL0      )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTCTRL1             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTCTRL1      )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTCTRL2             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTCTRL2      )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTCTRL3             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTCTRL3      )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTCTRL4             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTCTRL4      )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTCTRL5             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTCTRL5      )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTCTRL6             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTCTRL6      )
#define IMX6_USBO3H_OFFSET_USB_UOG_ENDPTCTRL7             (IMX6_USBO3H_OFFSET_USB_UOG + IMX6_USBCORE_OFFSET_ENDPTCTRL7      )

#define IMX6_USBO3H_OFFSET_USB_UH1_ID                     (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ID              )
#define IMX6_USBO3H_OFFSET_USB_UH1_HWGENERAL              (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_HWGENERAL       )
#define IMX6_USBO3H_OFFSET_USB_UH1_HWHOST                 (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_HWHOST          )
#define IMX6_USBO3H_OFFSET_USB_UH1_HWDEVICE               (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_HWDEVICE        )
#define IMX6_USBO3H_OFFSET_USB_UH1_HWTXBUF                (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_HWTXBUF         )
#define IMX6_USBO3H_OFFSET_USB_UH1_HWRXBUF                (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_HWRXBUF         )
#define IMX6_USBO3H_OFFSET_USB_UH1_GPTIMER0LD             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_GPTIMER0LD      )
#define IMX6_USBO3H_OFFSET_USB_UH1_GPTIMER0CTRL           (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_GPTIMER0CTRL    )
#define IMX6_USBO3H_OFFSET_USB_UH1_GPTIMER1LD             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_GPTIMER1LD      )
#define IMX6_USBO3H_OFFSET_USB_UH1_GPTIMER1CTRL           (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_GPTIMER1CTRL    )
#define IMX6_USBO3H_OFFSET_USB_UH1_SBUSCFG                (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_SBUSCFG         )
#define IMX6_USBO3H_OFFSET_USB_UH1_CAPLENGTH_8            (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_CAPLENGTH_8     )
#define IMX6_USBO3H_OFFSET_USB_UH1_HCIVERSION_16          (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_HCIVERSION_16   )
#define IMX6_USBO3H_OFFSET_USB_UH1_HCSPARAMS              (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_HCSPARAMS       )
#define IMX6_USBO3H_OFFSET_USB_UH1_HCCPARAMS              (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_HCCPARAMS       )
#define IMX6_USBO3H_OFFSET_USB_UH1_DCIVERSION             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_DCIVERSION      )
#define IMX6_USBO3H_OFFSET_USB_UH1_DCCPARAMS              (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_DCCPARAMS       )
#define IMX6_USBO3H_OFFSET_USB_UH1_USBCMD                 (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_USBCMD          )
#define IMX6_USBO3H_OFFSET_USB_UH1_USBSTS                 (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_USBSTS          )
#define IMX6_USBO3H_OFFSET_USB_UH1_USBINTR                (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_USBINTR         )
#define IMX6_USBO3H_OFFSET_USB_UH1_FRINDEX                (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_FRINDEX         )
#define IMX6_USBO3H_OFFSET_USB_UH1_PERIODICLISTBASE       (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_PERIODICLISTBASE)
#define IMX6_USBO3H_OFFSET_USB_UH1_DEVICEADDR             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_DEVICEADDR      )
#define IMX6_USBO3H_OFFSET_USB_UH1_ASYNCLISTADDR          (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ASYNCLISTADDR   )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTLISTADDR          (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTLISTADDR   )
#define IMX6_USBO3H_OFFSET_USB_UH1_BURSTSIZE              (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_BURSTSIZE       )
#define IMX6_USBO3H_OFFSET_USB_UH1_TXFILLTUNING           (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_TXFILLTUNING    )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTNAK               (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTNAK        )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTNAKEN             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTNAKEN      )
#define IMX6_USBO3H_OFFSET_USB_UH1_CONFIGFLAG             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_CONFIGFLAG      )
#define IMX6_USBO3H_OFFSET_USB_UH1_PORTSC1                (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_PORTSC1         )
#define IMX6_USBO3H_OFFSET_USB_UH1_OTGSC                  (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_OTGSC           )
#define IMX6_USBO3H_OFFSET_USB_UH1_USBMODE                (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_USBMODE         )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTSETUPSTAT         (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTSETUPSTAT  )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTPRIME             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTPRIME      )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTFLUSH             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTFLUSH      )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTSTAT              (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTSTAT       )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTCOMPLETE          (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTCOMPLETE   )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTCTRL0             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTCTRL0      )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTCTRL1             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTCTRL1      )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTCTRL2             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTCTRL2      )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTCTRL3             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTCTRL3      )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTCTRL4             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTCTRL4      )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTCTRL5             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTCTRL5      )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTCTRL6             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTCTRL6      )
#define IMX6_USBO3H_OFFSET_USB_UH1_ENDPTCTRL7             (IMX6_USBO3H_OFFSET_USB_UH1 + IMX6_USBCORE_OFFSET_ENDPTCTRL7      )

#define IMX6_USBO3H_OFFSET_USB_UH2_ID                     (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ID              )
#define IMX6_USBO3H_OFFSET_USB_UH2_HWGENERAL              (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_HWGENERAL       )
#define IMX6_USBO3H_OFFSET_USB_UH2_HWHOST                 (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_HWHOST          )
#define IMX6_USBO3H_OFFSET_USB_UH2_HWDEVICE               (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_HWDEVICE        )
#define IMX6_USBO3H_OFFSET_USB_UH2_HWTXBUF                (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_HWTXBUF         )
#define IMX6_USBO3H_OFFSET_USB_UH2_HWRXBUF                (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_HWRXBUF         )
#define IMX6_USBO3H_OFFSET_USB_UH2_GPTIMER0LD             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_GPTIMER0LD      )
#define IMX6_USBO3H_OFFSET_USB_UH2_GPTIMER0CTRL           (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_GPTIMER0CTRL    )
#define IMX6_USBO3H_OFFSET_USB_UH2_GPTIMER1LD             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_GPTIMER1LD      )
#define IMX6_USBO3H_OFFSET_USB_UH2_GPTIMER1CTRL           (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_GPTIMER1CTRL    )
#define IMX6_USBO3H_OFFSET_USB_UH2_SBUSCFG                (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_SBUSCFG         )
#define IMX6_USBO3H_OFFSET_USB_UH2_CAPLENGTH_8            (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_CAPLENGTH_8     )
#define IMX6_USBO3H_OFFSET_USB_UH2_HCIVERSION_16          (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_HCIVERSION_16   )
#define IMX6_USBO3H_OFFSET_USB_UH2_HCSPARAMS              (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_HCSPARAMS       )
#define IMX6_USBO3H_OFFSET_USB_UH2_HCCPARAMS              (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_HCCPARAMS       )
#define IMX6_USBO3H_OFFSET_USB_UH2_DCIVERSION             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_DCIVERSION      )
#define IMX6_USBO3H_OFFSET_USB_UH2_DCCPARAMS              (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_DCCPARAMS       )
#define IMX6_USBO3H_OFFSET_USB_UH2_USBCMD                 (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_USBCMD          )
#define IMX6_USBO3H_OFFSET_USB_UH2_USBSTS                 (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_USBSTS          )
#define IMX6_USBO3H_OFFSET_USB_UH2_USBINTR                (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_USBINTR         )
#define IMX6_USBO3H_OFFSET_USB_UH2_FRINDEX                (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_FRINDEX         )
#define IMX6_USBO3H_OFFSET_USB_UH2_PERIODICLISTBASE       (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_PERIODICLISTBASE)
#define IMX6_USBO3H_OFFSET_USB_UH2_DEVICEADDR             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_DEVICEADDR      )
#define IMX6_USBO3H_OFFSET_USB_UH2_ASYNCLISTADDR          (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ASYNCLISTADDR   )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTLISTADDR          (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTLISTADDR   )
#define IMX6_USBO3H_OFFSET_USB_UH2_BURSTSIZE              (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_BURSTSIZE       )
#define IMX6_USBO3H_OFFSET_USB_UH2_TXFILLTUNING           (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_TXFILLTUNING    )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTNAK               (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTNAK        )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTNAKEN             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTNAKEN      )
#define IMX6_USBO3H_OFFSET_USB_UH2_CONFIGFLAG             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_CONFIGFLAG      )
#define IMX6_USBO3H_OFFSET_USB_UH2_PORTSC1                (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_PORTSC1         )
#define IMX6_USBO3H_OFFSET_USB_UH2_OTGSC                  (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_OTGSC           )
#define IMX6_USBO3H_OFFSET_USB_UH2_USBMODE                (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_USBMODE         )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTSETUPSTAT         (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTSETUPSTAT  )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTPRIME             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTPRIME      )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTFLUSH             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTFLUSH      )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTSTAT              (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTSTAT       )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTCOMPLETE          (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTCOMPLETE   )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTCTRL0             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTCTRL0      )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTCTRL1             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTCTRL1      )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTCTRL2             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTCTRL2      )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTCTRL3             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTCTRL3      )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTCTRL4             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTCTRL4      )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTCTRL5             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTCTRL5      )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTCTRL6             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTCTRL6      )
#define IMX6_USBO3H_OFFSET_USB_UH2_ENDPTCTRL7             (IMX6_USBO3H_OFFSET_USB_UH2 + IMX6_USBCORE_OFFSET_ENDPTCTRL7      )

#define IMX6_USBO3H_OFFSET_USB_UH3_ID                     (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_ID              )
#define IMX6_USBO3H_OFFSET_USB_UH3_HWGENERAL              (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_HWGENERAL       )
#define IMX6_USBO3H_OFFSET_USB_UH3_HWHOST                 (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_HWHOST          )
#define IMX6_USBO3H_OFFSET_USB_UH3_HWDEVICE               (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_HWDEVICE        )
#define IMX6_USBO3H_OFFSET_USB_UH3_HWTXBUF                (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_HWTXBUF         )
#define IMX6_USBO3H_OFFSET_USB_UH3_HWRXBUF                (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_HWRXBUF         )
#define IMX6_USBO3H_OFFSET_USB_UH3_GPTIMER0LD             (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_GPTIMER0LD      )
#define IMX6_USBO3H_OFFSET_USB_UH3_GPTIMER0CTRL           (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_GPTIMER0CTRL    )
#define IMX6_USBO3H_OFFSET_USB_UH3_GPTIMER1LD             (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_GPTIMER1LD      )
#define IMX6_USBO3H_OFFSET_USB_UH3_GPTIMER1CTRL           (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_GPTIMER1CTRL    )
#define IMX6_USBO3H_OFFSET_USB_UH3_SBUSCFG                (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_SBUSCFG         )
#define IMX6_USBO3H_OFFSET_USB_UH3_CAPLENGTH_8            (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_CAPLENGTH_8     )
#define IMX6_USBO3H_OFFSET_USB_UH3_HCIVERSION_16          (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_HCIVERSION_16   )
#define IMX6_USBO3H_OFFSET_USB_UH3_HCSPARAMS              (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_HCSPARAMS       )
#define IMX6_USBO3H_OFFSET_USB_UH3_HCCPARAMS              (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_HCCPARAMS       )
#define IMX6_USBO3H_OFFSET_USB_UH3_DCIVERSION             (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_DCIVERSION      )
#define IMX6_USBO3H_OFFSET_USB_UH3_DCCPARAMS              (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_DCCPARAMS       )
#define IMX6_USBO3H_OFFSET_USB_UH3_USBCMD                 (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_USBCMD          )
#define IMX6_USBO3H_OFFSET_USB_UH3_USBSTS                 (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_USBSTS          )
#define IMX6_USBO3H_OFFSET_USB_UH3_USBINTR                (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_USBINTR         )
#define IMX6_USBO3H_OFFSET_USB_UH3_FRINDEX                (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_FRINDEX         )
#define IMX6_USBO3H_OFFSET_USB_UH3_PERIODICLISTBASE       (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_PERIODICLISTBASE)
#define IMX6_USBO3H_OFFSET_USB_UH3_DEVICEADDR             (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_DEVICEADDR      )
#define IMX6_USBO3H_OFFSET_USB_UH3_ASYNCLISTADDR          (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_ASYNCLISTADDR   )
#define IMX6_USBO3H_OFFSET_USB_UH3_ENDPTLISTADDR          (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_ENDPTLISTADDR   )
#define IMX6_USBO3H_OFFSET_USB_UH3_BURSTSIZE              (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_BURSTSIZE       )
#define IMX6_USBO3H_OFFSET_USB_UH3_TXFILLTUNING           (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_TXFILLTUNING    )
#define IMX6_USBO3H_OFFSET_USB_UH3_ENDPTNAK               (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_ENDPTNAK        )
#define IMX6_USBO3H_OFFSET_USB_UH3_ENDPTNAKEN             (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_ENDPTNAKEN      )
#define IMX6_USBO3H_OFFSET_USB_UH3_CONFIGFLAG             (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_CONFIGFLAG      )
#define IMX6_USBO3H_OFFSET_USB_UH3_PORTSC1                (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_PORTSC1         )
#define IMX6_USBO3H_OFFSET_USB_UH3_OTGSC                  (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_OTGSC           )
#define IMX6_USBO3H_OFFSET_USB_UH3_USBMODE                (IMX6_USBO3H_OFFSET_USB_UH3 + IMX6_USBCORE_OFFSET_USBMODE         )

#define IMX6_USBO3H_OFFSET_USBNC_USB_OTG_CTRL             0x0800 
#define IMX6_USBO3H_OFFSET_USBNC_USB_UH1_CTRL             0x0804 
#define IMX6_USBO3H_OFFSET_USBNC_USB_UH2_CTRL             0x0808 
#define IMX6_USBO3H_OFFSET_USBNC_USB_UH3_CTRL             0x080C 

#define IMX6_USBO3H_OFFSET_USBNC_USB_UH2_HSIC_CTRL        0x0810
#define IMX6_USBO3H_OFFSET_USBNC_USB_UH3_HSIC_CTRL        0x0814
#define IMX6_USBO3H_OFFSET_USBNC_USB_OTG_PHY_CTRL_0       0x0818 
#define IMX6_USBO3H_OFFSET_USBNC_USB_UH1_PHY_CTRL_0       0x081C 

#define IMX6_USB_PORTSC_CSC           0x00000002  // port status change
#define IMX6_USB_PORTSC_PEC           0x00000008  // port enable change
#define IMX6_USB_PORTSC_OCC           0x00000020  // overcurrent change
#define IMX6_USB_PORTSC_CHANGE_BITS   (IMX6_USB_PORTSC_CSC | IMX6_USB_PORTSC_PEC | IMX6_USB_PORTSC_OCC)

#define IMX6_USB_USBSTS_TI1           0x02000000
#define IMX6_USB_USBSTS_TI0           0x01000000
#define IMX6_USB_USBSTS_NAKI          0x00010000
#define IMX6_USB_USBSTS_AS            0x00008000
#define IMX6_USB_USBSTS_PS            0x00004000
#define IMX6_USB_USBSTS_RCL           0x00002000
#define IMX6_USB_USBSTS_HCH           0x00001000
#define IMX6_USB_USBSTS_ULPII         0x00000400
#define IMX6_USB_USBSTS_SLI           0x00000100
#define IMX6_USB_USBSTS_SRI           0x00000080
#define IMX6_USB_USBSTS_URI           0x00000040
#define IMX6_USB_USBSTS_AAI           0x00000020
#define IMX6_USB_USBSTS_SEI           0x00000010
#define IMX6_USB_USBSTS_FRI           0x00000008
#define IMX6_USB_USBSTS_PCI           0x00000004
#define IMX6_USB_USBSTS_UEI           0x00000002
#define IMX6_USB_USBSTS_UI            0x00000001

#define IMX6_USB_USBSTS_ALLINTS       0x030005FF

/* --------------------------------------------------------------------------------- */


#define IMX6_USBPHY_OFFSET_PWD              0x0000
#define IMX6_USBPHY_OFFSET_PWD_SET          0x0004
#define IMX6_USBPHY_OFFSET_PWD_CLR          0x0008
#define IMX6_USBPHY_OFFSET_PWD_TOG          0x000C
#define IMX6_USBPHY_OFFSET_TX               0x0010
#define IMX6_USBPHY_OFFSET_TX_SET           0x0014
#define IMX6_USBPHY_OFFSET_TX_CLR           0x0018
#define IMX6_USBPHY_OFFSET_TX_TOG           0x001C
#define IMX6_USBPHY_OFFSET_RX               0x0020
#define IMX6_USBPHY_OFFSET_RX_SET           0x0024
#define IMX6_USBPHY_OFFSET_RX_CLR           0x0028
#define IMX6_USBPHY_OFFSET_RX_TOG           0x002C
#define IMX6_USBPHY_OFFSET_CTRL             0x0030
#define IMX6_USBPHY_OFFSET_CTRL_SET         0x0034
#define IMX6_USBPHY_OFFSET_CTRL_CLR         0x0038
#define IMX6_USBPHY_OFFSET_CTRL_TOG         0x003C
#define IMX6_USBPHY_OFFSET_STATUS           0x0040
#define IMX6_USBPHY_OFFSET_DEBUG            0x0050
#define IMX6_USBPHY_OFFSET_DEBUG_SET        0x0054
#define IMX6_USBPHY_OFFSET_DEBUG_CLR        0x0058
#define IMX6_USBPHY_OFFSET_DEBUG_TOG        0x005C
#define IMX6_USBPHY_OFFSET_DEBUG0_STATUS    0x0060
#define IMX6_USBPHY_OFFSET_DEBUG1           0x0070
#define IMX6_USBPHY_OFFSET_DEBUG1_SET       0x0074
#define IMX6_USBPHY_OFFSET_DEBUG1_CLR       0x0078
#define IMX6_USBPHY_OFFSET_DEBUG1_TOG       0x007C
#define IMX6_USBPHY_OFFSET_VERSION          0x0080

#define IMX6_PHYSADDR_USBPHY1_PWD              (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_PWD           )
#define IMX6_PHYSADDR_USBPHY1_PWD_SET          (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_PWD_SET       )
#define IMX6_PHYSADDR_USBPHY1_PWD_CLR          (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_PWD_CLR       )
#define IMX6_PHYSADDR_USBPHY1_PWD_TOG          (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_PWD_TOG       )
#define IMX6_PHYSADDR_USBPHY1_TX               (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_TX            )
#define IMX6_PHYSADDR_USBPHY1_TX_SET           (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_TX_SET        )
#define IMX6_PHYSADDR_USBPHY1_TX_CLR           (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_TX_CLR        )
#define IMX6_PHYSADDR_USBPHY1_TX_TOG           (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_TX_TOG        )
#define IMX6_PHYSADDR_USBPHY1_RX               (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_RX            )
#define IMX6_PHYSADDR_USBPHY1_RX_SET           (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_RX_SET        )
#define IMX6_PHYSADDR_USBPHY1_RX_CLR           (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_RX_CLR        )
#define IMX6_PHYSADDR_USBPHY1_RX_TOG           (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_RX_TOG        )
#define IMX6_PHYSADDR_USBPHY1_CTRL             (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_CTRL          )
#define IMX6_PHYSADDR_USBPHY1_CTRL_SET         (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_CTRL_SET      )
#define IMX6_PHYSADDR_USBPHY1_CTRL_CLR         (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_CTRL_CLR      )
#define IMX6_PHYSADDR_USBPHY1_CTRL_TOG         (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_CTRL_TOG      )
#define IMX6_PHYSADDR_USBPHY1_STATUS           (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_STATUS        )
#define IMX6_PHYSADDR_USBPHY1_DEBUG            (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_DEBUG         )
#define IMX6_PHYSADDR_USBPHY1_DEBUG_SET        (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_DEBUG_SET     )
#define IMX6_PHYSADDR_USBPHY1_DEBUG_CLR        (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_DEBUG_CLR     )
#define IMX6_PHYSADDR_USBPHY1_DEBUG_TOG        (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_DEBUG_TOG     )
#define IMX6_PHYSADDR_USBPHY1_DEBUG0_STATUS    (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_DEBUG0_STATUS )
#define IMX6_PHYSADDR_USBPHY1_DEBUG1           (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_DEBUG1        )
#define IMX6_PHYSADDR_USBPHY1_DEBUG1_SET       (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_DEBUG1_SET    )
#define IMX6_PHYSADDR_USBPHY1_DEBUG1_CLR       (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_DEBUG1_CLR    )
#define IMX6_PHYSADDR_USBPHY1_DEBUG1_TOG       (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_DEBUG1_TOG    )
#define IMX6_PHYSADDR_USBPHY1_VERSION          (IMX6_PHYSADDR_USBPHY1 + IMX6_USBPHY_OFFSET_VERSION       )

#define IMX6_PHYSADDR_USBPHY2_PWD              (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_PWD           )
#define IMX6_PHYSADDR_USBPHY2_PWD_SET          (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_PWD_SET       )
#define IMX6_PHYSADDR_USBPHY2_PWD_CLR          (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_PWD_CLR       )
#define IMX6_PHYSADDR_USBPHY2_PWD_TOG          (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_PWD_TOG       )
#define IMX6_PHYSADDR_USBPHY2_TX               (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_TX            )
#define IMX6_PHYSADDR_USBPHY2_TX_SET           (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_TX_SET        )
#define IMX6_PHYSADDR_USBPHY2_TX_CLR           (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_TX_CLR        )
#define IMX6_PHYSADDR_USBPHY2_TX_TOG           (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_TX_TOG        )
#define IMX6_PHYSADDR_USBPHY2_RX               (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_RX            )
#define IMX6_PHYSADDR_USBPHY2_RX_SET           (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_RX_SET        )
#define IMX6_PHYSADDR_USBPHY2_RX_CLR           (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_RX_CLR        )
#define IMX6_PHYSADDR_USBPHY2_RX_TOG           (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_RX_TOG        )
#define IMX6_PHYSADDR_USBPHY2_CTRL             (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_CTRL          )
#define IMX6_PHYSADDR_USBPHY2_CTRL_SET         (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_CTRL_SET      )
#define IMX6_PHYSADDR_USBPHY2_CTRL_CLR         (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_CTRL_CLR      )
#define IMX6_PHYSADDR_USBPHY2_CTRL_TOG         (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_CTRL_TOG      )
#define IMX6_PHYSADDR_USBPHY2_STATUS           (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_STATUS        )
#define IMX6_PHYSADDR_USBPHY2_DEBUG            (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_DEBUG         )
#define IMX6_PHYSADDR_USBPHY2_DEBUG_SET        (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_DEBUG_SET     )
#define IMX6_PHYSADDR_USBPHY2_DEBUG_CLR        (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_DEBUG_CLR     )
#define IMX6_PHYSADDR_USBPHY2_DEBUG_TOG        (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_DEBUG_TOG     )
#define IMX6_PHYSADDR_USBPHY2_DEBUG0_STATUS    (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_DEBUG0_STATUS )
#define IMX6_PHYSADDR_USBPHY2_DEBUG1           (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_DEBUG1        )
#define IMX6_PHYSADDR_USBPHY2_DEBUG1_SET       (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_DEBUG1_SET    )
#define IMX6_PHYSADDR_USBPHY2_DEBUG1_CLR       (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_DEBUG1_CLR    )
#define IMX6_PHYSADDR_USBPHY2_DEBUG1_TOG       (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_DEBUG1_TOG    )
#define IMX6_PHYSADDR_USBPHY2_VERSION          (IMX6_PHYSADDR_USBPHY2 + IMX6_USBPHY_OFFSET_VERSION       )

#define IMX6_USBPHY_PWD_TXPWDFS                       0x00000400
#define IMX6_USBPHY_PWD_TXPWDIBIAS                    0x00000800
#define IMX6_USBPHY_PWD_TXPWDV2I                      0x00001000
#define IMX6_USBPHY_PWD_RXPDWENV                      0x00020000
#define IMX6_USBPHY_PWD_RXPWD1PT1                     0x00040000
#define IMX6_USBPHY_PWD_RXPWDDIFF                     0x00080000
#define IMX6_USBPHY_PWD_RXPWDRX                       0x00100000

#define IMX6_USBPHY_CTRL_SFTRST                       0x80000000
#define IMX6_USBPHY_CTRL_CLKGATE                      0x40000000
#define IMX6_USBPHY_CTRL_UTMI_SUSPENDM                0x20000000
#define IMX6_USBPHY_CTRL_HOST_FORCE_LS_SE0            0x10000000
#define IMX6_USBPHY_CTRL_OTG_ID_VALUE                 0x08000000
#define IMX6_USBPHY_CTRL_FSDLL_RST_EN                 0x01000000
#define IMX6_USBPHY_CTRL_ENVBUSCHG_WKUP               0x00800000
#define IMX6_USBPHY_CTRL_ENIDCHG_WKUP                 0x00400000
#define IMX6_USBPHY_CTRL_ENDPDMCHG_WKUP               0x00200000
#define IMX6_USBPHY_CTRL_ENAUTOCLR_PHY_PWD            0x00100000
#define IMX6_USBPHY_CTRL_ENAUTOCLR_CLKGATE            0x00080000
#define IMX6_USBPHY_CTRL_WAKEUP_IRQ                   0x00020000
#define IMX6_USBPHY_CTRL_ENIRQWAKEUP                  0x00010000
#define IMX6_USBPHY_CTRL_ENUTMILEVEL3                 0x00008000
#define IMX6_USBPHY_CTRL_ENUTMILEVEL2                 0x00004000
#define IMX6_USBPHY_CTRL_DATA_ON_LRADC                0x00002000
#define IMX6_USBPHY_CTRL_DEVPLUGIN_IRQ                0x00001000
#define IMX6_USBPHY_CTRL_ENIRQDEVPLUGIN               0x00000800
#define IMX6_USBPHY_CTRL_RESUME_IRQ                   0x00000400
#define IMX6_USBPHY_CTRL_ENIRQRESUMEDETECT            0x00000200
#define IMX6_USBPHY_CTRL_RESUMEIRQSTICKY              0x00000100
#define IMX6_USBPHY_CTRL_ENOTGIDDETECT                0x00000080
#define IMX6_USBPHY_CTRL_OTG_ID_CHG_IRQ               0x00000040
#define IMX6_USBPHY_CTRL_DEVPLUGIN_POLARITY           0x00000020
#define IMX6_USBPHY_CTRL_ENDEVPLUGINDETECT            0x00000010
#define IMX6_USBPHY_CTRL_HOSTDISCONDETECT_IRQ         0x00000008
#define IMX6_USBPHY_CTRL_ENIRQHOSTDISCON              0x00000004
#define IMX6_USBPHY_CTRL_ENHOSTDISCONDETECT           0x00000002
#define IMX6_USBPHY_CTRL_ENOTG_ID_CHG_IRQ             0x00000001

#define IMX6_USBPHY_STATUS_HOSTDISCONDETECT_STATUS    0x00000008
#define IMX6_USBPHY_STATUS_DEVPLUGIN_STATUS           0x00000040
#define IMX6_USBPHY_STATUS_OTGID_STATUS               0x00000100
#define IMX6_USBPHY_STATUS_RESUME_STATUS              0x00000400

/* --------------------------------------------------------------------------------- */

#if 0 // XXXXXXXXXXXXXXXXXXXX

#define IMX6_OCOTP_OFFSET_CTRL                0x0000  
#define IMX6_OCOTP_OFFSET_CTRL_SET            0x0004  
#define IMX6_OCOTP_OFFSET_CTRL_CLR            0x0008  
#define IMX6_OCOTP_OFFSET_CTRL_TOG            0x000C  
#define IMX6_OCOTP_OFFSET_TIMING              0x0010  
#define IMX6_OCOTP_OFFSET_DATA                0x0020  
#define IMX6_OCOTP_OFFSET_READ_CTRL           0x0030  
#define IMX6_OCOTP_OFFSET_READ_FUSE_DATA      0x0040  
#define IMX6_OCOTP_OFFSET_SW_STICKY           0x0050  
#define IMX6_OCOTP_OFFSET_SCS                 0x0060  
#define IMX6_OCOTP_OFFSET_SCS_SET             0x0064  
#define IMX6_OCOTP_OFFSET_SCS_CLR             0x0068  
#define IMX6_OCOTP_OFFSET_SCS_TOG             0x006C  
#define IMX6_OCOTP_OFFSET_VERSION             0x0090  
#define IMX6_OCOTP_OFFSET_LOCK                0x0400  
#define IMX6_OCOTP_OFFSET_CFG0                0x0410  
#define IMX6_OCOTP_OFFSET_CFG1                0x0420  
#define IMX6_OCOTP_OFFSET_CFG2                0x0430  
#define IMX6_OCOTP_OFFSET_CFG3                0x0440  
#define IMX6_OCOTP_OFFSET_CFG4                0x0450  
#define IMX6_OCOTP_OFFSET_CFG5                0x0460  
#define IMX6_OCOTP_OFFSET_CFG6                0x0470  
#define IMX6_OCOTP_OFFSET_MEM0                0x0480  
#define IMX6_OCOTP_OFFSET_MEM1                0x0490  
#define IMX6_OCOTP_OFFSET_MEM2                0x04A0  
#define IMX6_OCOTP_OFFSET_MEM3                0x04B0  
#define IMX6_OCOTP_OFFSET_MEM4                0x04C0  
#define IMX6_OCOTP_OFFSET_ANA0                0x04D0  
#define IMX6_OCOTP_OFFSET_ANA1                0x04E0  
#define IMX6_OCOTP_OFFSET_ANA2                0x04F0  
#define IMX6_OCOTP_OFFSET_SRK0                0x0580  
#define IMX6_OCOTP_OFFSET_SRK1                0x0590  
#define IMX6_OCOTP_OFFSET_SRK2                0x05A0  
#define IMX6_OCOTP_OFFSET_SRK3                0x05B0  
#define IMX6_OCOTP_OFFSET_SRK4                0x05C0  
#define IMX6_OCOTP_OFFSET_SRK5                0x05D0  
#define IMX6_OCOTP_OFFSET_SRK6                0x05E0  
#define IMX6_OCOTP_OFFSET_SRK7                0x05F0  
#define IMX6_OCOTP_OFFSET_RESP0               0x0600  
#define IMX6_OCOTP_OFFSET_HSJC_RESP1          0x0610  
#define IMX6_OCOTP_OFFSET_MAC0                0x0620  
#define IMX6_OCOTP_OFFSET_MAC1                0x0630  
#define IMX6_OCOTP_OFFSET_MAC2                0x0640  
#define IMX6_OCOTP_OFFSET_GP1                 0x0660  
#define IMX6_OCOTP_OFFSET_GP2                 0x0670  
#define IMX6_OCOTP_OFFSET_MISC_CONF           0x06D0  
#define IMX6_OCOTP_OFFSET_FIELD_RETURN        0x06E0  
#define IMX6_OCOTP_OFFSET_SRK_REVOKE          0x06F0  
#define IMX6_OCOTP_OFFSET_GP30                0x0A10  
#define IMX6_OCOTP_OFFSET_GP31                0x0A20  
#define IMX6_OCOTP_OFFSET_GP32                0x0A30  
#define IMX6_OCOTP_OFFSET_GP33                0x0A40  
#define IMX6_OCOTP_OFFSET_GP34                0x0A50  
#define IMX6_OCOTP_OFFSET_GP35                0x0A60  
#define IMX6_OCOTP_OFFSET_GP36                0x0A70  

#define IMX6_PHYSADDR_OCOTP_CTRL                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_CTRL           )
#define IMX6_PHYSADDR_OCOTP_CTRL_SET            (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_CTRL_SET       )
#define IMX6_PHYSADDR_OCOTP_CTRL_CLR            (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_CTRL_CLR       )
#define IMX6_PHYSADDR_OCOTP_CTRL_TOG            (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_CTRL_TOG       )
#define IMX6_PHYSADDR_OCOTP_TIMING              (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_TIMING         )
#define IMX6_PHYSADDR_OCOTP_DATA                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_DATA           )
#define IMX6_PHYSADDR_OCOTP_READ_CTRL           (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_READ_CTRL      )
#define IMX6_PHYSADDR_OCOTP_READ_FUSE_DATA      (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_READ_FUSE_DATA )
#define IMX6_PHYSADDR_OCOTP_SW_STICKY           (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SW_STICKY      )
#define IMX6_PHYSADDR_OCOTP_SCS                 (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SCS            )
#define IMX6_PHYSADDR_OCOTP_SCS_SET             (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SCS_SET        )
#define IMX6_PHYSADDR_OCOTP_SCS_CLR             (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SCS_CLR        )
#define IMX6_PHYSADDR_OCOTP_SCS_TOG             (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SCS_TOG        )
#define IMX6_PHYSADDR_OCOTP_VERSION             (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_VERSION        )
#define IMX6_PHYSADDR_OCOTP_LOCK                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_LOCK           )
#define IMX6_PHYSADDR_OCOTP_CFG0                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_CFG0           )
#define IMX6_PHYSADDR_OCOTP_CFG1                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_CFG1           )
#define IMX6_PHYSADDR_OCOTP_CFG2                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_CFG2           )
#define IMX6_PHYSADDR_OCOTP_CFG3                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_CFG3           )
#define IMX6_PHYSADDR_OCOTP_CFG4                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_CFG4           )
#define IMX6_PHYSADDR_OCOTP_CFG5                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_CFG5           )
#define IMX6_PHYSADDR_OCOTP_CFG6                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_CFG6           )
#define IMX6_PHYSADDR_OCOTP_MEM0                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_MEM0           )
#define IMX6_PHYSADDR_OCOTP_MEM1                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_MEM1           )
#define IMX6_PHYSADDR_OCOTP_MEM2                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_MEM2           )
#define IMX6_PHYSADDR_OCOTP_MEM3                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_MEM3           )
#define IMX6_PHYSADDR_OCOTP_MEM4                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_MEM4           )
#define IMX6_PHYSADDR_OCOTP_ANA0                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_ANA0           )
#define IMX6_PHYSADDR_OCOTP_ANA1                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_ANA1           )
#define IMX6_PHYSADDR_OCOTP_ANA2                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_ANA2           )
#define IMX6_PHYSADDR_OCOTP_SRK0                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SRK0           )
#define IMX6_PHYSADDR_OCOTP_SRK1                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SRK1           )
#define IMX6_PHYSADDR_OCOTP_SRK2                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SRK2           )
#define IMX6_PHYSADDR_OCOTP_SRK3                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SRK3           )
#define IMX6_PHYSADDR_OCOTP_SRK4                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SRK4           )
#define IMX6_PHYSADDR_OCOTP_SRK5                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SRK5           )
#define IMX6_PHYSADDR_OCOTP_SRK6                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SRK6           )
#define IMX6_PHYSADDR_OCOTP_SRK7                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SRK7           )
#define IMX6_PHYSADDR_OCOTP_RESP0               (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_RESP0          )
#define IMX6_PHYSADDR_OCOTP_HSJC_RESP1          (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_HSJC_RESP1     )
#define IMX6_PHYSADDR_OCOTP_MAC0                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_MAC0           )
#define IMX6_PHYSADDR_OCOTP_MAC1                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_MAC1           )
#define IMX6_PHYSADDR_OCOTP_MAC2                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_MAC2           )
#define IMX6_PHYSADDR_OCOTP_GP1                 (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_GP1            )
#define IMX6_PHYSADDR_OCOTP_GP2                 (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_GP2            )
#define IMX6_PHYSADDR_OCOTP_MISC_CONF           (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_MISC_CONF      )
#define IMX6_PHYSADDR_OCOTP_FIELD_RETURN        (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_FIELD_RETURN   )
#define IMX6_PHYSADDR_OCOTP_SRK_REVOKE          (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_SRK_REVOKE     )
#define IMX6_PHYSADDR_OCOTP_GP30                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_GP30           )
#define IMX6_PHYSADDR_OCOTP_GP31                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_GP31           )
#define IMX6_PHYSADDR_OCOTP_GP32                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_GP32           )
#define IMX6_PHYSADDR_OCOTP_GP33                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_GP33           )
#define IMX6_PHYSADDR_OCOTP_GP34                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_GP34           )
#define IMX6_PHYSADDR_OCOTP_GP35                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_GP35           )
#define IMX6_PHYSADDR_OCOTP_GP36                (IMX6_PHYSADDR_OCOTP + IMX6_OCOTP_OFFSET_GP36           )

#define IMX6_OCOTP_CTRL_WR_UNLOCK_MASK        0xFFFF0000
#define IMX6_OCOTP_CTRL_WR_UNLOCK_KEY         0x3E770000  // magic
#define IMX6_OCOTP_CTRL_RELOAD_SHADOWS        0x00000400
#define IMX6_OCOTP_CTRL_ERROR                 0x00000200
#define IMX6_OCOTP_CTRL_BUSY                  0x00000100
#define IMX6_OCOTP_CTRL_ADDR_MASK             0x000000FF

// section 63.2, table 63-1
#define IMX6_PHYSADDR_OCOTP_TEMP_CALIB_REG    IMX6_PHYSADDR_OCOTP_ANA1
#define IMX6_TEMP_CALIB_ROOM_COUNT_MASK       0xFFF00000
#define IMX6_TEMP_CALIB_ROOM_COUNT_SHL        20
#define IMX6_TEMP_CALIB_HOT_COUNT_MASK        0x000FFF00
#define IMX6_TEMP_CALIB_HOT_COUNT_SHL         8
#define IMX6_TEMP_CALIB_HOT_TEMP_C_MASK       0x000000FF

#endif // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

/* --------------------------------------------------------------------------------- */

#if 0 // XXXXXXXXXXXXXXXXXXXX

#define IMX6_LCDIF_OFFSET_RL                 0x0000       
#define IMX6_LCDIF_OFFSET_RL_SET             0x0004           
#define IMX6_LCDIF_OFFSET_RL_CLR             0x0008           
#define IMX6_LCDIF_OFFSET_RL_TOG             0x000C           
#define IMX6_LCDIF_OFFSET_CTRL1              0x0010          
#define IMX6_LCDIF_OFFSET_CTRL1_SET          0x0014              
#define IMX6_LCDIF_OFFSET_CTRL1_CLR          0x0018              
#define IMX6_LCDIF_OFFSET_CTRL1_TOG          0x001C              
#define IMX6_LCDIF_OFFSET_CTRL2              0x0020          
#define IMX6_LCDIF_OFFSET_CTRL2_SET          0x0024              
#define IMX6_LCDIF_OFFSET_CTRL2_CLR          0x0028              
#define IMX6_LCDIF_OFFSET_CTRL2_TOG          0x002C              
#define IMX6_LCDIF_OFFSET_TRANSFER_COUNT     0x0030                   
#define IMX6_LCDIF_OFFSET_CUR_BUF            0x0040            
#define IMX6_LCDIF_OFFSET_NEXT_BUF           0x0050             
#define IMX6_LCDIF_OFFSET_TIMING             0x0060           
#define IMX6_LCDIF_OFFSET_VDCTRL0            0x0070            
#define IMX6_LCDIF_OFFSET_VDCTRL0_SET        0x0074                
#define IMX6_LCDIF_OFFSET_VDCTRL0_CLR        0x0078                
#define IMX6_LCDIF_OFFSET_VDCTRL0_TOG        0x007C                
#define IMX6_LCDIF_OFFSET_VDCTRL1            0x0080            
#define IMX6_LCDIF_OFFSET_VDCTRL2            0x0090            
#define IMX6_LCDIF_OFFSET_VDCTRL3            0x00A0            
#define IMX6_LCDIF_OFFSET_VDCTRL4            0x00B0            
#define IMX6_LCDIF_OFFSET_DVICTRL0           0x00C0             
#define IMX6_LCDIF_OFFSET_DVICTRL1           0x00D0             
#define IMX6_LCDIF_OFFSET_DVICTRL2           0x00E0             
#define IMX6_LCDIF_OFFSET_DVICTRL3           0x00F0             
#define IMX6_LCDIF_OFFSET_DVICTRL4           0x0100             
#define IMX6_LCDIF_OFFSET_CSC_COEFF0         0x0110               
#define IMX6_LCDIF_OFFSET_CSC_COEFF1         0x0120               
#define IMX6_LCDIF_OFFSET_CSC_COEFF2         0x0130               
#define IMX6_LCDIF_OFFSET_CSC_COEFF3         0x0140               
#define IMX6_LCDIF_OFFSET_CSC_COEFF4         0x0150               
#define IMX6_LCDIF_OFFSET_CSC_OFFSET         0x0160               
#define IMX6_LCDIF_OFFSET_CSC_LIMIT          0x0170              
#define IMX6_LCDIF_OFFSET_DATA               0x0180         
#define IMX6_LCDIF_OFFSET_BM_ERROR_STAT      0x0190                  
#define IMX6_LCDIF_OFFSET_CRC_STAT           0x01A0             
#define IMX6_LCDIF_OFFSET_STAT               0x01B0         
#define IMX6_LCDIF_OFFSET_VERSION            0x01C0            
#define IMX6_LCDIF_OFFSET_DEBUG0             0x01D0           
#define IMX6_LCDIF_OFFSET_DEBUG1             0x01E0           
#define IMX6_LCDIF_OFFSET_DEBUG2             0x01F0           
#define IMX6_LCDIF_OFFSET_THRES              0x0200          
#define IMX6_LCDIF_OFFSET_AS_CTRL            0x0210            
#define IMX6_LCDIF_OFFSET_AS_BUF             0x0220           
#define IMX6_LCDIF_OFFSET_AS_NEXT_BUF        0x0230                
#define IMX6_LCDIF_OFFSET_AS_CLRKEYLOW       0x0240                 
#define IMX6_LCDIF_OFFSET_AS_CLRKEYHIGH      0x0250                  
#define IMX6_LCDIF_OFFSET_SYNC_DELAY         0x0260               
#define IMX6_LCDIF_OFFSET_DEBUG3             0x0270           
#define IMX6_LCDIF_OFFSET_DEBUG4             0x0280           
#define IMX6_LCDIF_OFFSET_DEBUG5             0x0290           

#define IMX6_PHYSADDR_LCDIF1_RL                 (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_RL              )
#define IMX6_PHYSADDR_LCDIF1_RL_SET             (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_RL_SET          )
#define IMX6_PHYSADDR_LCDIF1_RL_CLR             (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_RL_CLR          )
#define IMX6_PHYSADDR_LCDIF1_RL_TOG             (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_RL_TOG          )
#define IMX6_PHYSADDR_LCDIF1_CTRL1              (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CTRL1           )
#define IMX6_PHYSADDR_LCDIF1_CTRL1_SET          (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CTRL1_SET       )  
#define IMX6_PHYSADDR_LCDIF1_CTRL1_CLR          (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CTRL1_CLR       )  
#define IMX6_PHYSADDR_LCDIF1_CTRL1_TOG          (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CTRL1_TOG       )  
#define IMX6_PHYSADDR_LCDIF1_CTRL2              (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CTRL2           )
#define IMX6_PHYSADDR_LCDIF1_CTRL2_SET          (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CTRL2_SET       )  
#define IMX6_PHYSADDR_LCDIF1_CTRL2_CLR          (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CTRL2_CLR       )  
#define IMX6_PHYSADDR_LCDIF1_CTRL2_TOG          (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CTRL2_TOG       )  
#define IMX6_PHYSADDR_LCDIF1_TRANSFER_COUNT     (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_TRANSFER_COUNT  )       
#define IMX6_PHYSADDR_LCDIF1_CUR_BUF            (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CUR_BUF         )
#define IMX6_PHYSADDR_LCDIF1_NEXT_BUF           (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_NEXT_BUF        ) 
#define IMX6_PHYSADDR_LCDIF1_TIMING             (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_TIMING          )
#define IMX6_PHYSADDR_LCDIF1_VDCTRL0            (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_VDCTRL0         )
#define IMX6_PHYSADDR_LCDIF1_VDCTRL0_SET        (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_VDCTRL0_SET     )    
#define IMX6_PHYSADDR_LCDIF1_VDCTRL0_CLR        (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_VDCTRL0_CLR     )    
#define IMX6_PHYSADDR_LCDIF1_VDCTRL0_TOG        (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_VDCTRL0_TOG     )    
#define IMX6_PHYSADDR_LCDIF1_VDCTRL1            (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_VDCTRL1         )
#define IMX6_PHYSADDR_LCDIF1_VDCTRL2            (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_VDCTRL2         )
#define IMX6_PHYSADDR_LCDIF1_VDCTRL3            (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_VDCTRL3         )
#define IMX6_PHYSADDR_LCDIF1_VDCTRL4            (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_VDCTRL4         )
#define IMX6_PHYSADDR_LCDIF1_DVICTRL0           (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_DVICTRL0        ) 
#define IMX6_PHYSADDR_LCDIF1_DVICTRL1           (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_DVICTRL1        ) 
#define IMX6_PHYSADDR_LCDIF1_DVICTRL2           (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_DVICTRL2        ) 
#define IMX6_PHYSADDR_LCDIF1_DVICTRL3           (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_DVICTRL3        ) 
#define IMX6_PHYSADDR_LCDIF1_DVICTRL4           (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_DVICTRL4        ) 
#define IMX6_PHYSADDR_LCDIF1_CSC_COEFF0         (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CSC_COEFF0      )   
#define IMX6_PHYSADDR_LCDIF1_CSC_COEFF1         (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CSC_COEFF1      )   
#define IMX6_PHYSADDR_LCDIF1_CSC_COEFF2         (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CSC_COEFF2      )   
#define IMX6_PHYSADDR_LCDIF1_CSC_COEFF3         (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CSC_COEFF3      )   
#define IMX6_PHYSADDR_LCDIF1_CSC_COEFF4         (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CSC_COEFF4      )   
#define IMX6_PHYSADDR_LCDIF1_CSC_OFFSET         (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CSC_OFFSET      )   
#define IMX6_PHYSADDR_LCDIF1_CSC_LIMIT          (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CSC_LIMIT       )  
#define IMX6_PHYSADDR_LCDIF1_DATA               (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_DATA            )
#define IMX6_PHYSADDR_LCDIF1_BM_ERROR_STAT      (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_BM_ERROR_STAT   )      
#define IMX6_PHYSADDR_LCDIF1_CRC_STAT           (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_CRC_STAT        ) 
#define IMX6_PHYSADDR_LCDIF1_STAT               (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_STAT            )
#define IMX6_PHYSADDR_LCDIF1_VERSION            (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_VERSION         )
#define IMX6_PHYSADDR_LCDIF1_DEBUG0             (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_DEBUG0          )
#define IMX6_PHYSADDR_LCDIF1_DEBUG1             (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_DEBUG1          )
#define IMX6_PHYSADDR_LCDIF1_DEBUG2             (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_DEBUG2          )
#define IMX6_PHYSADDR_LCDIF1_THRES              (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_THRES           )
#define IMX6_PHYSADDR_LCDIF1_AS_CTRL            (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_AS_CTRL         )
#define IMX6_PHYSADDR_LCDIF1_AS_BUF             (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_AS_BUF          )
#define IMX6_PHYSADDR_LCDIF1_AS_NEXT_BUF        (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_AS_NEXT_BUF     )    
#define IMX6_PHYSADDR_LCDIF1_AS_CLRKEYLOW       (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_AS_CLRKEYLOW    )     
#define IMX6_PHYSADDR_LCDIF1_AS_CLRKEYHIGH      (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_AS_CLRKEYHIGH   )      
#define IMX6_PHYSADDR_LCDIF1_SYNC_DELAY         (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_SYNC_DELAY      )   
#define IMX6_PHYSADDR_LCDIF1_DEBUG3             (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_DEBUG3          )
#define IMX6_PHYSADDR_LCDIF1_DEBUG4             (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_DEBUG4          )
#define IMX6_PHYSADDR_LCDIF1_DEBUG5             (IMX6_PHYSADDR_LCDIF1 + IMX6_LCDIF_OFFSET_DEBUG5          )

#define IMX6_PHYSADDR_LCDIF2_RL                 (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_RL              )
#define IMX6_PHYSADDR_LCDIF2_RL_SET             (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_RL_SET          )
#define IMX6_PHYSADDR_LCDIF2_RL_CLR             (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_RL_CLR          )
#define IMX6_PHYSADDR_LCDIF2_RL_TOG             (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_RL_TOG          )
#define IMX6_PHYSADDR_LCDIF2_CTRL1              (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CTRL1           )
#define IMX6_PHYSADDR_LCDIF2_CTRL1_SET          (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CTRL1_SET       )  
#define IMX6_PHYSADDR_LCDIF2_CTRL1_CLR          (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CTRL1_CLR       )  
#define IMX6_PHYSADDR_LCDIF2_CTRL1_TOG          (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CTRL1_TOG       )  
#define IMX6_PHYSADDR_LCDIF2_CTRL2              (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CTRL2           )
#define IMX6_PHYSADDR_LCDIF2_CTRL2_SET          (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CTRL2_SET       )  
#define IMX6_PHYSADDR_LCDIF2_CTRL2_CLR          (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CTRL2_CLR       )  
#define IMX6_PHYSADDR_LCDIF2_CTRL2_TOG          (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CTRL2_TOG       )  
#define IMX6_PHYSADDR_LCDIF2_TRANSFER_COUNT     (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_TRANSFER_COUNT  )       
#define IMX6_PHYSADDR_LCDIF2_CUR_BUF            (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CUR_BUF         )
#define IMX6_PHYSADDR_LCDIF2_NEXT_BUF           (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_NEXT_BUF        ) 
#define IMX6_PHYSADDR_LCDIF2_TIMING             (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_TIMING          )
#define IMX6_PHYSADDR_LCDIF2_VDCTRL0            (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_VDCTRL0         )
#define IMX6_PHYSADDR_LCDIF2_VDCTRL0_SET        (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_VDCTRL0_SET     )    
#define IMX6_PHYSADDR_LCDIF2_VDCTRL0_CLR        (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_VDCTRL0_CLR     )    
#define IMX6_PHYSADDR_LCDIF2_VDCTRL0_TOG        (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_VDCTRL0_TOG     )    
#define IMX6_PHYSADDR_LCDIF2_VDCTRL1            (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_VDCTRL1         )
#define IMX6_PHYSADDR_LCDIF2_VDCTRL2            (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_VDCTRL2         )
#define IMX6_PHYSADDR_LCDIF2_VDCTRL3            (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_VDCTRL3         )
#define IMX6_PHYSADDR_LCDIF2_VDCTRL4            (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_VDCTRL4         )
#define IMX6_PHYSADDR_LCDIF2_DVICTRL0           (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_DVICTRL0        ) 
#define IMX6_PHYSADDR_LCDIF2_DVICTRL1           (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_DVICTRL1        ) 
#define IMX6_PHYSADDR_LCDIF2_DVICTRL2           (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_DVICTRL2        ) 
#define IMX6_PHYSADDR_LCDIF2_DVICTRL3           (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_DVICTRL3        ) 
#define IMX6_PHYSADDR_LCDIF2_DVICTRL4           (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_DVICTRL4        ) 
#define IMX6_PHYSADDR_LCDIF2_CSC_COEFF0         (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CSC_COEFF0      )   
#define IMX6_PHYSADDR_LCDIF2_CSC_COEFF1         (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CSC_COEFF1      )   
#define IMX6_PHYSADDR_LCDIF2_CSC_COEFF2         (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CSC_COEFF2      )   
#define IMX6_PHYSADDR_LCDIF2_CSC_COEFF3         (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CSC_COEFF3      )   
#define IMX6_PHYSADDR_LCDIF2_CSC_COEFF4         (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CSC_COEFF4      )   
#define IMX6_PHYSADDR_LCDIF2_CSC_OFFSET         (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CSC_OFFSET      )   
#define IMX6_PHYSADDR_LCDIF2_CSC_LIMIT          (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CSC_LIMIT       )  
#define IMX6_PHYSADDR_LCDIF2_DATA               (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_DATA            )
#define IMX6_PHYSADDR_LCDIF2_BM_ERROR_STAT      (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_BM_ERROR_STAT   )      
#define IMX6_PHYSADDR_LCDIF2_CRC_STAT           (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_CRC_STAT        ) 
#define IMX6_PHYSADDR_LCDIF2_STAT               (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_STAT            )
#define IMX6_PHYSADDR_LCDIF2_VERSION            (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_VERSION         )
#define IMX6_PHYSADDR_LCDIF2_DEBUG0             (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_DEBUG0          )
#define IMX6_PHYSADDR_LCDIF2_DEBUG1             (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_DEBUG1          )
#define IMX6_PHYSADDR_LCDIF2_DEBUG2             (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_DEBUG2          )
#define IMX6_PHYSADDR_LCDIF2_THRES              (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_THRES           )
#define IMX6_PHYSADDR_LCDIF2_AS_CTRL            (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_AS_CTRL         )
#define IMX6_PHYSADDR_LCDIF2_AS_BUF             (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_AS_BUF          )
#define IMX6_PHYSADDR_LCDIF2_AS_NEXT_BUF        (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_AS_NEXT_BUF     )    
#define IMX6_PHYSADDR_LCDIF2_AS_CLRKEYLOW       (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_AS_CLRKEYLOW    )     
#define IMX6_PHYSADDR_LCDIF2_AS_CLRKEYHIGH      (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_AS_CLRKEYHIGH   )      
#define IMX6_PHYSADDR_LCDIF2_SYNC_DELAY         (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_SYNC_DELAY      )   
#define IMX6_PHYSADDR_LCDIF2_DEBUG3             (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_DEBUG3          )
#define IMX6_PHYSADDR_LCDIF2_DEBUG4             (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_DEBUG4          )
#define IMX6_PHYSADDR_LCDIF2_DEBUG5             (IMX6_PHYSADDR_LCDIF2 + IMX6_LCDIF_OFFSET_DEBUG5          )

#define IMX6_LCDIF_RL_SFTRST                      0x80000000
#define IMX6_LCDIF_RL_CLKGATE                     0x40000000
#define IMX6_LCDIF_RL_YCBCR422_INPUT              0x20000000
#define IMX6_LCDIF_RL_READ_WRITEB                 0x10000000
#define IMX6_LCDIF_RL_WAIT_FOR_VSYNC_EDGE         0x08000000
#define IMX6_LCDIF_RL_DATA_SHIFT_DIR_LEFT         0x00000000
#define IMX6_LCDIF_RL_DATA_SHIFT_DIR_RIGHT        0x04000000
#define IMX6_LCDIF_RL_SHIFT_NUM_BITS_MASK         0x03E00000
#define IMX6_LCDIF_RL_SHIFT_NUM_BITS_SHL          21
#define IMX6_LCDIF_RL_DVI_MODE                    0x00100000
#define IMX6_LCDIF_RL_BYPASS_COUNT                0x00080000
#define IMX6_LCDIF_RL_VSYNC_MODE                  0x00040000
#define IMX6_LCDIF_RL_DOTCLK_MODE                 0x00020000
#define IMX6_LCDIF_RL_DATA_SELECT_CMD_MODE        0x00000000
#define IMX6_LCDIF_RL_DATA_SELECT_DATA_MODE       0x00010000
#define IMX6_LCDIF_RL_INPUT_DATA_SWIZZLE_MASK     0x0000C000
#define IMX6_LCDIF_RL_INPUT_DATA_SWIZZLE_NONE     0x00000000
#define IMX6_LCDIF_RL_INPUT_DATA_SWIZZLE_LE       0x00000000
#define IMX6_LCDIF_RL_INPUT_DATA_SWIZZLE_BE       0x00004000
#define IMX6_LCDIF_RL_INPUT_DATA_SWIZZLE_SWAP     0x00004000
#define IMX6_LCDIF_RL_INPUT_DATA_SWIZZLE_HSWAP    0x00008000
#define IMX6_LCDIF_RL_INPUT_DATA_SWIZZLE_HBSWAP   0x0000C000
#define IMX6_LCDIF_RL_CSC_DATA_SWIZZLE_MASK       0x00003000
#define IMX6_LCDIF_RL_CSC_DATA_SWIZZLE_NONE       0x00000000
#define IMX6_LCDIF_RL_CSC_DATA_SWIZZLE_LE         0x00000000
#define IMX6_LCDIF_RL_CSC_DATA_SWIZZLE_BE         0x00001000
#define IMX6_LCDIF_RL_CSC_DATA_SWIZZLE_SWAP       0x00001000
#define IMX6_LCDIF_RL_CSC_DATA_SWIZZLE_HSWAP      0x00002000
#define IMX6_LCDIF_RL_CSC_DATA_SWIZZLE_HBSWAP     0x00003000
#define IMX6_LCDIF_RL_LCD_DATABUS_WIDTH_MASK      0x00000C00
#define IMX6_LCDIF_RL_LCD_DATABUS_WIDTH_16        0x00000000
#define IMX6_LCDIF_RL_LCD_DATABUS_WIDTH_8         0x00000400
#define IMX6_LCDIF_RL_LCD_DATABUS_WIDTH_18        0x00000800
#define IMX6_LCDIF_RL_LCD_DATABUS_WIDTH_24        0x00000C00
#define IMX6_LCDIF_RL_WORD_LENGTH_MASK            0x00000300
#define IMX6_LCDIF_RL_WORD_LENGTH_16              0x00000000
#define IMX6_LCDIF_RL_WORD_LENGTH_8               0x00000100
#define IMX6_LCDIF_RL_WORD_LENGTH_18              0x00000200
#define IMX6_LCDIF_RL_WORD_LENGTH_24              0x00000300
#define IMX6_LCDIF_RL_RGB_TO_YCBCR422_CSC         0x00000080
#define IMX6_LCDIF_RL_ENABLE_PXP_HANDSHAKE        0x00000040
#define IMX6_LCDIF_RL_MASTER                      0x00000020
#define IMX6_LCDIF_RL_DATA_FORMAT_16_BIT          0x00000008
#define IMX6_LCDIF_RL_DATA_FORMAT_18_BIT          0x00000004
#define IMX6_LCDIF_RL_DATA_FORMAT_24_BIT          0x00000002
#define IMX6_LCDIF_RL_RUN                         0x00000001

#define IMX6_LCDIF_CTRL1_COMBINE_MPU_WR_STRB      0x08000000
#define IMX6_LCDIF_CTRL1_BM_ERROR_IRQ_EN          0x04000000
#define IMX6_LCDIF_CTRL1_BM_ERROR_IRQ             0x02000000
#define IMX6_LCDIF_CTRL1_RECOVER_ON_UNDERFLOW     0x01000000
#define IMX6_LCDIF_CTRL1_INTERLACE_FIELDS         0x00800000
#define IMX6_LCDIF_CTRL1_START_SECOND_FIELD       0x00400000
#define IMX6_LCDIF_CTRL1_FIFO_CLEAR               0x00200000
#define IMX6_LCDIF_CTRL1_IRQ_ON_ALT_FIELDS        0x00100000
#define IMX6_LCDIF_CTRL1_BYTE_PACKING_FORMAT_MASK 0x000F0000
#define IMX6_LCDIF_CTRL1_BYTE_PACKING_FORMAT_SHL  16
#define IMX6_LCDIF_CTRL1_OVERFLOW_IRQ_EN          0x00008000
#define IMX6_LCDIF_CTRL1_UNDERFLOW_IRQ_EN         0x00004000
#define IMX6_LCDIF_CTRL1_CUR_FRAME_DONE_IRQ_EN    0x00002000
#define IMX6_LCDIF_CTRL1_VSYNC_EDGE_IRQ_EN        0x00001000
#define IMX6_LCDIF_CTRL1_OVERFLOW_IRQ             0x00000800
#define IMX6_LCDIF_CTRL1_UNDERFLOW_IRQ            0x00000400
#define IMX6_LCDIF_CTRL1_CUR_FRAME_DONE_IRQ       0x00000200
#define IMX6_LCDIF_CTRL1_VSYNC_EDGE_IRQ           0x00000100
#define IMX6_LCDIF_CTRL1_BUSY_ENABLE              0x00000004
#define IMX6_LCDIF_CTRL1_MODE86                   0x00000002
#define IMX6_LCDIF_CTRL1_RESET                    0x00000001

#define IMX6_LCDIF_TRANSFER_COUNT_H_MASK          0x0000FFFF
#define IMX6_LCDIF_TRANSFER_COUNT_V_MASK          0xFFFF0000
#define IMX6_LCDIF_TRANSFER_COUNT_V_SHL           16

#define IMX6_LCDIF_VDCTRL0_VSYNC_OEB              0x20000000
#define IMX6_LCDIF_VDCTRL0_ENABLE_PRESENT         0x10000000
#define IMX6_LCDIF_VDCTRL0_VSYNC_POL              0x08000000
#define IMX6_LCDIF_VDCTRL0_HSYNC_POL              0x04000000
#define IMX6_LCDIF_VDCTRL0_DOTCLK_POL             0x02000000
#define IMX6_LCDIF_VDCTRL0_ENABLE_POL             0x01000000
#define IMX6_LCDIF_VDCTRL0_VSYNC_PERIOD_UNIT      0x00200000
#define IMX6_LCDIF_VDCTRL0_VSYNC_PULSE_UNIT       0x00100000
#define IMX6_LCDIF_VDCTRL0_HALF_LINE              0x00080000
#define IMX6_LCDIF_VDCTRL0_HALF_LINE_MODE         0x00040000
#define IMX6_LCDIF_VDCTRL0_VSYNC_PULSE_WIDTH_MASK 0x0003FFFF

#define IMX6_LCDIF_VDCTRL2_HSYNC_PULSE_WIDTH_MASK 0xFFFF0000
#define IMX6_LCDIF_VDCTRL2_HSYNC_PULSE_WIDTH_SHL  16
#define IMX6_LCDIF_VDCTRL2_HSYNC_PERIOD_MASK      0x0000FFFF

#define IMX6_LCDIF_VDCTRL3_MUX_SYNC_SIGNALS       0x20000000
#define IMX6_LCDIF_VDCTRL3_VSYNC_ONLY             0x10000000
#define IMX6_LCDIF_VDCTRL3_HORZ_WAIT_CNT_MASK     0x0FFF0000
#define IMX6_LCDIF_VDCTRL3_HORZ_WAIT_CNT_SHL      16
#define IMX6_LCDIF_VDCTRL3_VERT_WAIT_CNT_MASK     0x0000FFFF

#define IMX6_LCDIF_VDCTRL4_DOTCLK_DELAY_SEL_MASK  0xE0000000
#define IMX6_LCDIF_VDCTRL4_DOTCLK_DELAY_SEL_SHL   29
#define IMX6_LCDIF_VDCTRL4_SYNC_SIGNALS_ON        0x00040000
#define IMX6_LCDIF_VDCTRL4_DOTCLK_HVALID_DATA_CNT_MASK 0x0003FFFF

#endif // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

/* --------------------------------------------------------------------------------- */

#if 0 // XXXXXXXXXXXXXXXXXXXX

#define ENET_PACKET_BYTES       1518
#define ENET_PACKET_MAX         1536
#define ENET_PACKET_ALIGN       ((ENET_PACKET_MAX+255ul)&~255ul)

#define IMX6_ENET_OFFSET_ENET1_EIR                0x0004
#define IMX6_ENET_OFFSET_ENET1_EIMR               0x0008
#define IMX6_ENET_OFFSET_ENET1_RDAR               0x0010
#define IMX6_ENET_OFFSET_ENET1_TDAR               0x0014
#define IMX6_ENET_OFFSET_ENET1_ECR                0x0024
#define IMX6_ENET_OFFSET_ENET1_MMFR               0x0040
#define IMX6_ENET_OFFSET_ENET1_MSCR               0x0044
#define IMX6_ENET_OFFSET_ENET1_MIBC               0x0064
#define IMX6_ENET_OFFSET_ENET1_RCR                0x0084
#define IMX6_ENET_OFFSET_ENET1_TCR                0x00C4
#define IMX6_ENET_OFFSET_ENET1_PALR               0x00E4
#define IMX6_ENET_OFFSET_ENET1_PAUR               0x00E8
#define IMX6_ENET_OFFSET_ENET1_OPD                0x00EC
#define IMX6_ENET_OFFSET_ENET1_TXIC0              0x00F0
#define IMX6_ENET_OFFSET_ENET1_TXIC1              0x00F4
#define IMX6_ENET_OFFSET_ENET1_TXIC2              0x00F8
#define IMX6_ENET_OFFSET_ENET1_RXIC0              0x0100
#define IMX6_ENET_OFFSET_ENET1_RXIC1              0x0104
#define IMX6_ENET_OFFSET_ENET1_RXIC2              0x0108
#define IMX6_ENET_OFFSET_ENET1_IAUR               0x0118
#define IMX6_ENET_OFFSET_ENET1_IALR               0x011C
#define IMX6_ENET_OFFSET_ENET1_GAUR               0x0120
#define IMX6_ENET_OFFSET_ENET1_GALR               0x0124
#define IMX6_ENET_OFFSET_ENET1_TFWR               0x0144
#define IMX6_ENET_OFFSET_ENET1_RDSR1              0x0160
#define IMX6_ENET_OFFSET_ENET1_TDSR1              0x0164
#define IMX6_ENET_OFFSET_ENET1_MRBR1              0x0168
#define IMX6_ENET_OFFSET_ENET1_RDSR2              0x016C
#define IMX6_ENET_OFFSET_ENET1_TDSR2              0x0170
#define IMX6_ENET_OFFSET_ENET1_MRBR2              0x0174
#define IMX6_ENET_OFFSET_ENET1_RDSR               0x0180
#define IMX6_ENET_OFFSET_ENET1_TDSR               0x0184
#define IMX6_ENET_OFFSET_ENET1_MRBR               0x0188
#define IMX6_ENET_OFFSET_ENET1_RSFL               0x0190
#define IMX6_ENET_OFFSET_ENET1_RSEM               0x0194
#define IMX6_ENET_OFFSET_ENET1_RAEM               0x0198
#define IMX6_ENET_OFFSET_ENET1_RAFL               0x019C
#define IMX6_ENET_OFFSET_ENET1_TSEM               0x01A0
#define IMX6_ENET_OFFSET_ENET1_TAEM               0x01A4
#define IMX6_ENET_OFFSET_ENET1_TAFL               0x01A8
#define IMX6_ENET_OFFSET_ENET1_TIPG               0x01AC
#define IMX6_ENET_OFFSET_ENET1_FTRL               0x01B0
#define IMX6_ENET_OFFSET_ENET1_TACC               0x01C0
#define IMX6_ENET_OFFSET_ENET1_RACC               0x01C4
#define IMX6_ENET_OFFSET_ENET1_RCMR1              0x01C8
#define IMX6_ENET_OFFSET_ENET1_RCMR2              0x01CC
#define IMX6_ENET_OFFSET_ENET1_DMA1CFG            0x01D8
#define IMX6_ENET_OFFSET_ENET1_DMA2CFG            0x01DC
#define IMX6_ENET_OFFSET_ENET1_RDAR1              0x01E0
#define IMX6_ENET_OFFSET_ENET1_TDAR1              0x01E4
#define IMX6_ENET_OFFSET_ENET1_RDAR2              0x01E8
#define IMX6_ENET_OFFSET_ENET1_TDAR2              0x01EC
#define IMX6_ENET_OFFSET_ENET1_QOS                0x01F0
#define IMX6_ENET_OFFSET_ENET1_RMON_T_DROP        0x0200
#define IMX6_ENET_OFFSET_ENET1_RMON_T_PACKETS     0x0204
#define IMX6_ENET_OFFSET_ENET1_RMON_T_BC_PKT      0x0208
#define IMX6_ENET_OFFSET_ENET1_RMON_T_MC_PKT      0x020C
#define IMX6_ENET_OFFSET_ENET1_RMON_T_CRC_ALIGN   0x0210
#define IMX6_ENET_OFFSET_ENET1_RMON_T_UNDERSIZE   0x0214
#define IMX6_ENET_OFFSET_ENET1_RMON_T_OVERSIZE    0x0218
#define IMX6_ENET_OFFSET_ENET1_RMON_T_FRAG        0x021C
#define IMX6_ENET_OFFSET_ENET1_RMON_T_JAB         0x0220
#define IMX6_ENET_OFFSET_ENET1_RMON_T_COL         0x0224
#define IMX6_ENET_OFFSET_ENET1_RMON_T_P64         0x0228
#define IMX6_ENET_OFFSET_ENET1_RMON_T_P65TO127    0x022C
#define IMX6_ENET_OFFSET_ENET1_RMON_T_P128TO255   0x0230
#define IMX6_ENET_OFFSET_ENET1_RMON_T_P256TO511   0x0234
#define IMX6_ENET_OFFSET_ENET1_RMON_T_P512TO1023  0x0238
#define IMX6_ENET_OFFSET_ENET1_RMON_T_P1024TO2047 0x023C
#define IMX6_ENET_OFFSET_ENET1_RMON_T_P_GTE2048   0x0240
#define IMX6_ENET_OFFSET_ENET1_RMON_T_OCTETS      0x0244
#define IMX6_ENET_OFFSET_ENET1_IEEE_T_DROP        0x0248
#define IMX6_ENET_OFFSET_ENET1_IEEE_T_FRAME_OK    0x024C
#define IMX6_ENET_OFFSET_ENET1_IEEE_T_1COL        0x0250
#define IMX6_ENET_OFFSET_ENET1_IEEE_T_MCOL        0x0254
#define IMX6_ENET_OFFSET_ENET1_IEEE_T_DEF         0x0258
#define IMX6_ENET_OFFSET_ENET1_IEEE_T_LCOL        0x025C
#define IMX6_ENET_OFFSET_ENET1_IEEE_T_EXCOL       0x0260
#define IMX6_ENET_OFFSET_ENET1_IEEE_T_MACERR      0x0264
#define IMX6_ENET_OFFSET_ENET1_IEEE_T_CSERR       0x0268
#define IMX6_ENET_OFFSET_ENET1_IEEE_T_SQE         0x026C
#define IMX6_ENET_OFFSET_ENET1_IEEE_T_FDXFC       0x0270
#define IMX6_ENET_OFFSET_ENET1_IEEE_T_OCTETS_OK   0x0274
#define IMX6_ENET_OFFSET_ENET1_RMON_R_PACKETS     0x0284
#define IMX6_ENET_OFFSET_ENET1_RMON_R_BC_PKT      0x0288
#define IMX6_ENET_OFFSET_ENET1_RMON_R_MC_PKT      0x028C
#define IMX6_ENET_OFFSET_ENET1_RMON_R_CRC_ALIGN   0x0290
#define IMX6_ENET_OFFSET_ENET1_RMON_R_UNDERSIZE   0x0294
#define IMX6_ENET_OFFSET_ENET1_RMON_R_OVERSIZE    0x0298
#define IMX6_ENET_OFFSET_ENET1_RMON_R_FRAG        0x029C
#define IMX6_ENET_OFFSET_ENET1_RMON_R_JAB         0x02A0
#define IMX6_ENET_OFFSET_ENET1_RMON_R_RESVD_0     0x02A4
#define IMX6_ENET_OFFSET_ENET1_RMON_R_P64         0x02A8
#define IMX6_ENET_OFFSET_ENET1_RMON_R_P65TO127    0x02AC
#define IMX6_ENET_OFFSET_ENET1_RMON_R_P128TO255   0x02B0
#define IMX6_ENET_OFFSET_ENET1_RMON_R_P256TO511   0x02B4
#define IMX6_ENET_OFFSET_ENET1_RMON_R_P512TO1023  0x02B8
#define IMX6_ENET_OFFSET_ENET1_RMON_R_P1024TO2047 0x02BC
#define IMX6_ENET_OFFSET_ENET1_RMON_R_P_GTE2048   0x02C0
#define IMX6_ENET_OFFSET_ENET1_RMON_R_OCTETS      0x02C4
#define IMX6_ENET_OFFSET_ENET1_IEEE_R_DROP        0x02C8
#define IMX6_ENET_OFFSET_ENET1_IEEE_R_FRAME_OK    0x02CC
#define IMX6_ENET_OFFSET_ENET1_IEEE_R_CRC         0x02D0
#define IMX6_ENET_OFFSET_ENET1_IEEE_R_ALIGN       0x02D4
#define IMX6_ENET_OFFSET_ENET1_IEEE_R_MACERR      0x02D8
#define IMX6_ENET_OFFSET_ENET1_IEEE_R_FDXFC       0x02DC
#define IMX6_ENET_OFFSET_ENET1_IEEE_R_OCTETS_OK   0x02E0
#define IMX6_ENET_OFFSET_ENET1_ATCR               0x0400
#define IMX6_ENET_OFFSET_ENET1_ATVR               0x0404
#define IMX6_ENET_OFFSET_ENET1_ATOFF              0x0408
#define IMX6_ENET_OFFSET_ENET1_ATPER              0x040C
#define IMX6_ENET_OFFSET_ENET1_ATCOR              0x0410
#define IMX6_ENET_OFFSET_ENET1_ATINC              0x0414
#define IMX6_ENET_OFFSET_ENET1_ATSTMP             0x0418
#define IMX6_ENET_OFFSET_ENET1_TGSR               0x0604
#define IMX6_ENET_OFFSET_ENET1_TCSR0              0x0608
#define IMX6_ENET_OFFSET_ENET1_TCCR0              0x060C
#define IMX6_ENET_OFFSET_ENET1_TCSR1              0x0610
#define IMX6_ENET_OFFSET_ENET1_TCCR1              0x0614
#define IMX6_ENET_OFFSET_ENET1_TCSR2              0x0618
#define IMX6_ENET_OFFSET_ENET1_TCCR2              0x061C
#define IMX6_ENET_OFFSET_ENET1_TCSR3              0x0620
#define IMX6_ENET_OFFSET_ENET1_TCCR3              0x0624

#define IMX6_ENET_INT_BABR        0x40000000
#define IMX6_ENET_INT_BABT        0x20000000
#define IMX6_ENET_INT_GRA         0x10000000
#define IMX6_ENET_INT_TXF         0x08000000
#define IMX6_ENET_INT_TXB         0x04000000
#define IMX6_ENET_INT_RXF         0x02000000
#define IMX6_ENET_INT_RXB         0x01000000
#define IMX6_ENET_INT_MII         0x00800000
#define IMX6_ENET_INT_EBERR       0x00400000
#define IMX6_ENET_INT_LC          0x00200000
#define IMX6_ENET_INT_RL          0x00100000
#define IMX6_ENET_INT_UN          0x00080000
#define IMX6_ENET_INT_PLR         0x00040000
#define IMX6_ENET_INT_WAKEUP      0x00020000
#define IMX6_ENET_INT_TS_AVAIL    0x00010000
#define IMX6_ENET_INT_TS_TIMER    0x00008000
#define IMX6_ENET_INT_RXFLUSH_2   0x00004000
#define IMX6_ENET_INT_RXFLUSH_1   0x00002000
#define IMX6_ENET_INT_RXFLUSH_0   0x00001000
#define IMX6_ENET_INT_TXF2        0x00000080
#define IMX6_ENET_INT_TXB2        0x00000040
#define IMX6_ENET_INT_RXF2        0x00000020
#define IMX6_ENET_INT_RXB2        0x00000010
#define IMX6_ENET_INT_TXF1        0x00000008
#define IMX6_ENET_INT_TXB1        0x00000004
#define IMX6_ENET_INT_RXF1        0x00000002
#define IMX6_ENET_INT_RXB1        0x00000001

#define IMX6_ENET_ECR_SVLANDBL    0x00000800
#define IMX6_ENET_ECR_VLANUSE2ND  0x00000400
#define IMX6_ENET_ECR_SVLANEN     0x00000200
#define IMX6_ENET_ECR_DBSWP       0x00000100
#define IMX6_ENET_ECR_DBGEN       0x00000040
#define IMX6_ENET_ECR_SPEED       0x00000020
#define IMX6_ENET_ECR_EN1588      0x00000010
#define IMX6_ENET_ECR_SLEEP       0x00000008
#define IMX6_ENET_ECR_MAGICEN     0x00000004
#define IMX6_ENET_ECR_ETHEREN     0x00000002
#define IMX6_ENET_ECR_RESET       0x00000001

#define IMX6_ENET_RDAR_RDAR       0x01000000

#define IMX6_ENET_TDAR_TDAR       0x01000000

#define IMX6_ENET_RCR_GRS         0x80000000
#define IMX6_ENET_RCR_NLC         0x40000000
#define IMX6_ENET_RCR_MAX_FL_MASK 0x3FFF0000
#define IMX6_ENET_RCR_MAX_FL_SHL  16
#define IMX6_ENET_RCR_CFEN        0x00008000
#define IMX6_ENET_RCR_CRCFWD      0x00004000
#define IMX6_ENET_RCR_PAUFWD      0x00002000
#define IMX6_ENET_RCR_PADEN       0x00001000
#define IMX6_ENET_RCR_RMII_10T    0x00000200
#define IMX6_ENET_RCR_RMII_MODE   0x00000100
#define IMX6_ENET_RCR_RGMII_EN    0x00000040
#define IMX6_ENET_RCR_FCE         0x00000020
#define IMX6_ENET_RCR_BC_REJ      0x00000010
#define IMX6_ENET_RCR_PROM        0x00000008
#define IMX6_ENET_RCR_MII_MODE    0x00000004
#define IMX6_ENET_RCR_DRT         0x00000002
#define IMX6_ENET_RCR_LOOP        0x00000001

#define IMX6_ENET_TCR_CRCFWD      0x00000200
#define IMX6_ENET_TCR_ADDINS      0x00000100
#define IMX6_ENET_TCR_ADDSEL_MASK 0x000000E0
#define IMX6_ENET_TCR_RFC_PAUSE   0x00000010
#define IMX6_ENET_TCR_TFC_PAUSE   0x00000008
#define IMX6_ENET_TCR_FDEN        0x00000004
#define IMX6_ENET_TCR_GTS         0x00000001 

#define IMX6_ENET_MMFR_ST_MASK        0xC0000000
#define IMX6_ENET_MMFR_ST             0x40000000
#define IMX6_ENET_MMFR_OP_MASK        0x30000000
#define IMX6_ENET_MMFR_OP_WRITE       0x10000000
#define IMX6_ENET_MMFR_OP_READ        0x20000000
#define IMX6_ENET_MMFR_PA_MASK        0x0F800000
#define IMX6_ENET_MMFR_PA_SHL         23
#define IMX6_ENET_MMFR_RA_MASK        0x007C0000
#define IMX6_ENET_MMFR_RA_SHL         18
#define IMX6_ENET_MMFR_TA_MASK        0x00030000
#define IMX6_ENET_MMFR_TA             0x00020000
#define IMX6_ENET_MMFR_DATA_MASK      0x0000FFFF

#define IMX6_ENET_MSCR_HOLDTIME_MASK  0x00000700
#define IMX6_ENET_MSCR_HOLDTIME_SHL   8
#define IMX6_ENET_MSCR_DIS_PRE        0x00000080
#define IMX6_ENET_MSCR_MII_SPEED_MASK 0x0000007E
#define IMX6_ENET_MSCR_MII_SPEED_SHL  1

#define IMX6_ENET_MIBC_DISABLE        0x80000000
#define IMX6_ENET_MIBC_IDLE           0x40000000
#define IMX6_ENET_MIBC_CLEAR          0x20000000

#endif // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

/* --------------------------------------------------------------------------------- */

#if 0 // XXXXXXXXXXXXXXXXXXXX

#define IMX6_MU_OFFSET_ATR0           0x0000
#define IMX6_MU_OFFSET_ATR1           0x0004
#define IMX6_MU_OFFSET_ATR2           0x0008
#define IMX6_MU_OFFSET_ATR3           0x000C
#define IMX6_MU_OFFSET_ARR0           0x0010
#define IMX6_MU_OFFSET_ARR1           0x0014
#define IMX6_MU_OFFSET_ARR2           0x0018
#define IMX6_MU_OFFSET_ARR3           0x001C
#define IMX6_MU_OFFSET_ASR            0x0020
#define IMX6_MU_OFFSET_ACR            0x0024

#define IMX6_MU_ASR_GIP3              0x80000000
#define IMX6_MU_ASR_GIP2              0x40000000
#define IMX6_MU_ASR_GIP1              0x20000000
#define IMX6_MU_ASR_GIP0              0x10000000
#define IMX6_MU_ASR_RF3               0x08000000
#define IMX6_MU_ASR_RF2               0x04000000
#define IMX6_MU_ASR_RF1               0x02000000
#define IMX6_MU_ASR_RF0               0x01000000
#define IMX6_MU_ASR_TE3               0x00800000
#define IMX6_MU_ASR_TE2               0x00400000
#define IMX6_MU_ASR_TE1               0x00200000
#define IMX6_MU_ASR_TE0               0x00100000
#define IMX6_MU_ASR_BRDIP             0x00000200
#define IMX6_MU_ASR_FUP               0x00000100
#define IMX6_MU_ASR_BRS               0x00000080
#define IMX6_MU_ASR_EP                0x00000010
#define IMX6_MU_ASR_F2                0x00000004
#define IMX6_MU_ASR_F1                0x00000002
#define IMX6_MU_ASR_F0                0x00000001

#define IMX6_MU_ACR_GIE3              0x80000000
#define IMX6_MU_ACR_GIE2              0x40000000
#define IMX6_MU_ACR_GIE1              0x20000000
#define IMX6_MU_ACR_GIE0              0x10000000
#define IMX6_MU_ACR_RIE3              0x08000000
#define IMX6_MU_ACR_RIE2              0x04000000
#define IMX6_MU_ACR_RIE1              0x02000000
#define IMX6_MU_ACR_RIE0              0x01000000
#define IMX6_MU_ACR_TIE3              0x00800000
#define IMX6_MU_ACR_TIE2              0x00400000
#define IMX6_MU_ACR_TIE1              0x00200000
#define IMX6_MU_ACR_TIE0              0x00100000
#define IMX6_MU_ACR_GIR3              0x00080000
#define IMX6_MU_ACR_GIR2              0x00040000
#define IMX6_MU_ACR_GIR1              0x00020000
#define IMX6_MU_ACR_GIR0              0x00010000
#define IMX6_MU_ACR_BRDIE             0x00000040
#define IMX6_MU_ACR_MUR               0x00000020
#define IMX6_MU_ACR_ABF2              0x00000004
#define IMX6_MU_ACR_ABF1              0x00000002
#define IMX6_MU_ACR_ABF0              0x00000001

#endif // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

/* --------------------------------------------------------------------------------- */

#define IMX6_GPC_OFFSET_CNTR                0x000 
#define IMX6_GPC_OFFSET_PGR                 0x004 
#define IMX6_GPC_OFFSET_IMR1                0x008 
#define IMX6_GPC_OFFSET_IMR2                0x00C 
#define IMX6_GPC_OFFSET_IMR3                0x010 
#define IMX6_GPC_OFFSET_IMR4                0x014 
#define IMX6_GPC_OFFSET_ISR1                0x018 
#define IMX6_GPC_OFFSET_ISR2                0x01C 
#define IMX6_GPC_OFFSET_ISR3                0x020 
#define IMX6_GPC_OFFSET_ISR4                0x024 

#define IMX6_PHYSADDR_GPC_CNTR              (IMX6_PHYSADDR_GPC + IMX6_GPC_OFFSET_CNTR )
#define IMX6_PHYSADDR_GPC_PGR               (IMX6_PHYSADDR_GPC + IMX6_GPC_OFFSET_PGR  )
#define IMX6_PHYSADDR_GPC_IMR1              (IMX6_PHYSADDR_GPC + IMX6_GPC_OFFSET_IMR1 )
#define IMX6_PHYSADDR_GPC_IMR2              (IMX6_PHYSADDR_GPC + IMX6_GPC_OFFSET_IMR2 )
#define IMX6_PHYSADDR_GPC_IMR3              (IMX6_PHYSADDR_GPC + IMX6_GPC_OFFSET_IMR3 )
#define IMX6_PHYSADDR_GPC_IMR4              (IMX6_PHYSADDR_GPC + IMX6_GPC_OFFSET_IMR4 )
#define IMX6_PHYSADDR_GPC_ISR1              (IMX6_PHYSADDR_GPC + IMX6_GPC_OFFSET_ISR1 )
#define IMX6_PHYSADDR_GPC_ISR2              (IMX6_PHYSADDR_GPC + IMX6_GPC_OFFSET_ISR2 )
#define IMX6_PHYSADDR_GPC_ISR3              (IMX6_PHYSADDR_GPC + IMX6_GPC_OFFSET_ISR3 )
#define IMX6_PHYSADDR_GPC_ISR4              (IMX6_PHYSADDR_GPC + IMX6_GPC_OFFSET_ISR4 )

#define IMX6_GPC_CNTR_PUP_GPCIRQM           0x00200000
#define IMX6_GPC_CNTR_GPU_VPU_PUP_REQ       0x00000002
#define IMX6_GPC_CNTR_GPU_VPU_PDN_REQ       0x00000001

#define IMX6_PGC_PU_OFFSET_GPU_CTRL         0x0000
#define IMX6_PGC_PU_OFFSET_GPU_PUPSCR       0x0004  
#define IMX6_PGC_PU_OFFSET_GPU_PDNSCR       0x0008  
#define IMX6_PGC_PU_OFFSET_GPU_SR           0x000C
#define IMX6_PGC_PU_OFFSET_CPU_CTRL         0x0040
#define IMX6_PGC_PU_OFFSET_CPU_PUPSCR       0x0044  
#define IMX6_PGC_PU_OFFSET_CPU_PDNSCR       0x0048  
#define IMX6_PGC_PU_OFFSET_CPU_SR           0x004C

#define IMX6_PHYSADDR_PGC_PU_GPU_CTRL       (IMX6_PHYSADDR_PGC_PU + IMX6_PGC_PU_OFFSET_GPU_CTRL   )
#define IMX6_PHYSADDR_PGC_PU_GPU_PUPSCR     (IMX6_PHYSADDR_PGC_PU + IMX6_PGC_PU_OFFSET_GPU_PUPSCR )
#define IMX6_PHYSADDR_PGC_PU_GPU_PDNSCR     (IMX6_PHYSADDR_PGC_PU + IMX6_PGC_PU_OFFSET_GPU_PDNSCR )
#define IMX6_PHYSADDR_PGC_PU_GPU_SR         (IMX6_PHYSADDR_PGC_PU + IMX6_PGC_PU_OFFSET_GPU_SR     )
#define IMX6_PHYSADDR_PGC_PU_CPU_CTRL       (IMX6_PHYSADDR_PGC_PU + IMX6_PGC_PU_OFFSET_CPU_CTRL   )
#define IMX6_PHYSADDR_PGC_PU_CPU_PUPSCR     (IMX6_PHYSADDR_PGC_PU + IMX6_PGC_PU_OFFSET_CPU_PUPSCR )
#define IMX6_PHYSADDR_PGC_PU_CPU_PDNSCR     (IMX6_PHYSADDR_PGC_PU + IMX6_PGC_PU_OFFSET_CPU_PDNSCR )
#define IMX6_PHYSADDR_PGC_PU_CPU_SR         (IMX6_PHYSADDR_PGC_PU + IMX6_PGC_PU_OFFSET_CPU_SR     )

#define IMX6_PGC_PU_GPU_CTRL_PCR            0x00000001


/* --------------------------------------------------------------------------------- */


#endif  // __IMX6DEF_INC

