/*   
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
*/
#ifndef __SPEC_PCDEF_INC
#define __SPEC_PCDEF_INC 

/*-----------------------------------------------------------------------------------------
  E820 Types
  -----------------------------------------------------------------------------------------*/

#define PC_E820_ADDRESSRANGE_MEMORY          1
#define PC_E820_ADDRESSRANGE_RESERVED        2
#define PC_E820_ADDRESSRANGE_ACPI            3
#define PC_E820_ADDRESSRANGE_NVS             4


/*-----------------------------------------------------------------------------------------
  Legacy IRQ assignment and IDT slots
  -----------------------------------------------------------------------------------------*/
#define PC_IRQ_PIT                           0
#define PC_IRQ_KEYBOARD                      1
#define PC_IRQ_PIC2                          2
#define PC_IRQ_SERIAL2                       3
#define PC_IRQ_SERIAL1                       4
#define PC_IRQ_PARALLEL2                     5
#define PC_IRQ_FLOPPY                        6
#define PC_IRQ_PARALLEL1                     7
#define PC_IRQ_RTC                           8
#define PC_IRQ_VGA_RETRACE                   9
#define PC_IRQ_MOUSE                         12
#define PC_IRQ_FPU                           13
#define PC_IRQ_HDD                           14
#define PC_IRQ_LAST                          15

#define PC_IDT_PIT                           8
#define PC_IDT_KEYBOARD                      9
#define PC_IDT_PIC2                          10
#define PC_IDT_SERIAL2                       11
#define PC_IDT_SERIAL1                       12
#define PC_IDT_PARALLEL2                     13
#define PC_IDT_FLOPPY                        14
#define PC_IDT_PARALLEL1                     15
#define PC_IDT_RTC                           112
#define PC_IDT_VGA_RETRACE                   113
#define PC_IDT_MOUSE                         116
#define PC_IDT_FPU                           117
#define PC_IDT_HDD                           118


/*-----------------------------------------------------------------------------------------
  PIC
  -----------------------------------------------------------------------------------------*/
#define PC_8259_PIC1_IOPORT                  0x20
#define PC_8259_PIC2_IOPORT                  0xA0

#define PC_PIC1_COMMAND                      (PC_8259_PIC1_IOPORT)
#define PC_PIC1_DATA                         ((PC_8259_PIC1_IOPORT)+1)

#define PC_PIC2_COMMAND                      (PC_8259_PIC2_IOPORT)
#define PC_PIC2_DATA                         ((PC_8259_PIC2_IOPORT)+1)

#define PC_8259_PIC_ICW1_WORD4NEEDED         0x01
#define PC_8259_PIC_ICW1_SINGLEMODE          0x02
#define PC_8259_PIC_ICW1_INTERVAL            0x04
#define PC_8259_PIC_ICW1_LEVELTRIGGER        0x08
#define PC_8259_PIC_ICW1_INIT                0x10

#define PC_8259_PIC_ICW4_8086MODE            0x01
#define PC_8259_PIC_ICW4_AUTOEOI             0x02
#define PC_8259_PIC_ICW4_BUF_SLAVE           0x08
#define PC_8259_PIC_ICW4_BUF_MASTER          0x0C
#define PC_8259_PIC_ICW4_SPECIAL             0x10

#define PC_8259_OCW2_ROTATE                  0x80
#define PC_8259_OCW2_SPECIFIC                0x40
#define PC_8259_OCW2_EOI                     0x20

#define PC_8259_OCW_NONSEPECIFC_EOI          PC_8259_OCW2_EOI
#define PC_8259_OCW_SPECIFIC_EOI(x)          (PC_8259_OCW2_EOI | PC_8259_OCW2_SPECIFIC | ((x)&0x7))
#define PC_8259_OCW_SETPRIORITY              0xC0

#define PC_NUM_IRQ                           16

#define PC_MAX_QUANTUM_DELTA_MS              5000


/*-----------------------------------------------------------------------------------------
  PIT
  -----------------------------------------------------------------------------------------*/

#define PC_PIT_FREQUENCY_HZ                  1193182  // div by 36157 = 33.0000
#define PC_PIT_CHANNEL0_IOPORT               0x0040
#define PC_PIT_CHANNEL1_IOPORT               0x0041
#define PC_PIT_CHANNEL2_IOPORT               0x0042
#define PC_PIT_CMD_IOPORT                    0x0043

#define PC_PIT_TIMER                         PC_PIT_CHANNEL0_IOPORT
#define PC_PIT_USELESS                       PC_PIT_CHANNEL1_IOPORT
#define PC_PIT_PCSPEAKER                     PC_PIT_CHANNEL2_IOPORT

#define PC_PIT_CMD_CHAN0                     0x00
#define PC_PIT_CMD_CHAN1                     0x40
#define PC_PIT_CMD_CHAN2                     0x80

#define PC_PIT_CMD1_TIMER                    PC_PIT_CMD_CHAN0
#define PC_PIT_CMD1_USELESS                  PC_PIT_CMD_CHAN1
#define PC_PIT_CMD1_PCSPEAKER                PC_PIT_CMD_CHAN2

#define PC_PIT_CMD2_COUNTERLATCH             0x00
#define PC_PIT_CMD2_RW_LO8                   0x10
#define PC_PIT_CMD2_RW_HI8                   0x20
#define PC_PIT_CMD2_RW_LO8_HI8               0x30

#define PC_PIT_CMD3_MODE0                    0x00
#define PC_PIT_CMD3_MODE1                    0x02
#define PC_PIT_CMD3_MODE2                    0x04
#define PC_PIT_CMD3_MODE3                    0x06
#define PC_PIT_CMD3_MODE4                    0x08
#define PC_PIT_CMD3_MODE5                    0x0A

#define PC_PIT_CMD4_BINARY_COUNT             0x00
#define PC_PIT_CMD4_DECIMAL_COUNT            0x01


/*-----------------------------------------------------------------------------------------
  ATA
  -----------------------------------------------------------------------------------------*/

#define PC_ATA_PORT0_DATA                    0x1F0
#define PC_ATA_PORT0_FEATURE_ERROR           0x1F1
#define PC_ATA_PORT0_SECTOR_COUNT            0x1F2
#define PC_ATA_PORT0_SECTOR_NUMBER           0x1F3
#define PC_ATA_PORT0_CYLINDER_LOW            0x1F4
#define PC_ATA_PORT0_CYLINDER_HIGH           0x1F5
#define PC_ATA_PORT0_DRIVE_HEAD              0x1F6
#define PC_ATA_PORT0_COMMAND_STATUS          0x1F7
#define PC_ATA_PORT0_CONTROL_ALTSTATUS       0x3F6

#define PC_ATA_CONTROL_nINT_ENABLE           0x01
#define PC_ATA_CONTROL_SOFT_RESET            0x02
#define PC_ATA_CONTROL_READ_HOB              0x80


/*-----------------------------------------------------------------------------------------
  Serial Ports
  -----------------------------------------------------------------------------------------*/

#define PC_SERIAL1_IOPORT               0x3F8
#define PC_SERIAL2_IOPORT               0x2F8
#define PC_SERIAL3_IOPORT               0x3E8
#define PC_SERIAL4_IOPORT               0x2E8

#define PC_SERIAL_OFFSET_THR_RHR        0
#define PC_SERIAL_OFFSET_DLAB1_LOWDIV   0
#define PC_SERIAL_OFFSET_IER            1
#define PC_SERIAL_OFFSET_DLAB1_HIDIV    1
#define PC_SERIAL_OFFSET_IIR_FIFOCR     2
#define PC_SERIAL_OFFSET_LCR            3
#define PC_SERIAL_OFFSET_MCR            4
#define PC_SERIAL_OFFSET_LSR            5
#define PC_SERIAL_OFFSET_MSR            6
#define PC_SERIAL_OFFSET_SCRATCH        7

#define PC_SERIAL1_THR_RHR              (PC_SERIAL1_IOPORT + PC_SERIAL_OFFSET_THR_RHR     )
#define PC_SERIAL1_DLAB1_LOWDIV         (PC_SERIAL1_IOPORT + PC_SERIAL_OFFSET_DLAB1_LOWDIV)
#define PC_SERIAL1_IER                  (PC_SERIAL1_IOPORT + PC_SERIAL_OFFSET_IER         )
#define PC_SERIAL1_DLAB1_HIDIV          (PC_SERIAL1_IOPORT + PC_SERIAL_OFFSET_DLAB1_HIDIV )
#define PC_SERIAL1_IIR_FIFOCR           (PC_SERIAL1_IOPORT + PC_SERIAL_OFFSET_IIR_FIFOCR  )
#define PC_SERIAL1_LCR                  (PC_SERIAL1_IOPORT + PC_SERIAL_OFFSET_LCR         )
#define PC_SERIAL1_MCR                  (PC_SERIAL1_IOPORT + PC_SERIAL_OFFSET_MCR         )
#define PC_SERIAL1_LSR                  (PC_SERIAL1_IOPORT + PC_SERIAL_OFFSET_LSR         )
#define PC_SERIAL1_MSR                  (PC_SERIAL1_IOPORT + PC_SERIAL_OFFSET_MSR         )
#define PC_SERIAL1_SCRATCH              (PC_SERIAL1_IOPORT + PC_SERIAL_OFFSET_SCRATCH     )

#define PC_SERIAL_MAXRATE_BPS           115200
#define PC_SERIAL_DIV_BAUD115200        1
#define PC_SERIAL_DIV_BAUD57600         2
#define PC_SERIAL_DIV_BAUD38400         3
#define PC_SERIAL_DIV_BAUD28800         4
#define PC_SERIAL_DIV_BAUD19200         6
#define PC_SERIAL_DIV_BAUD14400         8
#define PC_SERIAL_DIV_BAUD9600          12

#define PC_SERIAL_IER_LOWPOWERMODE      0x20
#define PC_SERIAL_IER_SLEEPMODE         0x10
#define PC_SERIAL_IER_MODEM_STATUS      0x08
#define PC_SERIAL_IER_LINE_STATUS       0x04
#define PC_SERIAL_IER_THR_EMPTY         0x02
#define PC_SERIAL_IER_RHR_FULL          0x01

#define PC_SERIAL_IIR_FIFO_ENB_NONE     0x00
#define PC_SERIAL_IIR_FIFO_ENB_ON       0xC0
#define PC_SERIAL_IIR_FIFO_ENB_OFF      0x80
#define PC_SERIAL_IIR_FIFO_ENB_MASK     0xC0
#define PC_SERIAL_IIR_FIFO64_ENB        0x20
#define PC_SERIAL_IIR_TYPE_MASK         0x0E
#define PC_SERIAL_IIR_TYPE_MODEM_STATUS 0x00
#define PC_SERIAL_IIR_TYPE_THR_EMPTY    0x02
#define PC_SERIAL_IIR_TYPE_RHR_FULL     0x04
#define PC_SERIAL_IIR_TYPE_LINE_STATUS  0x06
#define PC_SERIAL_IIR_TYPE_RHR_TIMEOUT  0x0C
#define PC_SERIAL_IIR_PENDING           0x01

#define PC_SERIAL_FCR_LCR_MASK          0xC0
#define PC_SERIAL_FCR_LCR_BYTE1         0x00
#define PC_SERIAL_FCR_LCR_BYTE4_16      0x40
#define PC_SERIAL_FCR_LCR_BYTE8_32      0x80
#define PC_SERIAL_FCR_LCR_BYTE14_56     0xC0
#define PC_SERIAL_FCR_ENB_FIFO64        0x20
#define PC_SERIAL_FCR_ENB_DMA           0x08
#define PC_SERIAL_FCR_CLEAR_TX          0x04
#define PC_SERIAL_FCR_CLEAR_RX          0x02
#define PC_SERIAL_FCR_ENABLE            0x01

#define PC_SERIAL_LCR_DLAB              0x80
#define PC_SERIAL_LCR_SET_BREAK         0x40
#define PC_SERIAL_LCR_PARITY_MASK       0x38
#define PC_SERIAL_LCR_PARITY_NONE       0x00
#define PC_SERIAL_LCR_PARITY_ODD        0x08
#define PC_SERIAL_LCR_PARITY_EVEN       0x18
#define PC_SERIAL_LCR_PARITY_MARK       0x28
#define PC_SERIAL_LCR_PARITY_SPACE      0x38
#define PC_SERIAL_LCR_STOPBITS_MASK     0x04
#define PC_SERIAL_LCR_STOPBITS_1        0x00
#define PC_SERIAL_LCR_STOPBITS_X        0x04
#define PC_SERIAL_LCR_DATABITS_MASK     0x03
#define PC_SERIAL_LCR_DATABITS_5        0x00
#define PC_SERIAL_LCR_DATABITS_6        0x01
#define PC_SERIAL_LCR_DATABITS_7        0x02
#define PC_SERIAL_LCR_DATABITS_8        0x03

#define PC_SERIAL_MCR_AUTOFLOW          0x20
#define PC_SERIAL_MCR_LOOPBACK          0x10
#define PC_SERIAL_MCR_AUX2              0x08
#define PC_SERIAL_MCR_AUX1              0x04
#define PC_SERIAL_MCR_RTS               0x02
#define PC_SERIAL_MCR_DTR               0x01

#define PC_SERIAL_LSR_FIFO_ERROR        0x80
#define PC_SERIAL_LSR_ALLTX_EMPTY       0x40
#define PC_SERIAL_LSR_THR_EMPTY         0x20
#define PC_SERIAL_LSR_BREAK             0x10
#define PC_SERIAL_LSR_FRAMING_ERROR     0x08
#define PC_SERIAL_LSR_PARITY_ERROR      0x04
#define PC_SERIAL_LSR_OVERRUN_ERROR     0x02
#define PC_SERIAL_LSR_RHR_FULL          0x01

#define PC_SERIAL_MSR_CARRIER_DETECT    0x80
#define PC_SERIAL_MSR_RING              0x40
#define PC_SERIAL_MSR_DSR               0x20
#define PC_SERIAL_MSR_CTS               0x10
#define PC_SERIAL_MSR_DDCD              0x08
#define PC_SERIAL_MSR_TRAILING_RING     0x04
#define PC_SERIAL_MSR_DDSR              0x02
#define PC_SERIAL_MSR_DCTS              0x01

/*-----------------------------------------------------------------------------------------
  PCI Coniguration ports
  -----------------------------------------------------------------------------------------*/
#define PC_PCI_CONFIG_ADDR_IOPORT       0x0CF8
#define PC_PCI_CONFIG_DATA_IOPORT       0x0CFC


#endif // __SPEC_PCDEF_INC 
