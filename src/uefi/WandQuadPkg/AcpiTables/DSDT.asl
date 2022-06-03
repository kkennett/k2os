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
DefinitionBlock ("DSDT.aml", "DSDT", 5, "K2____", "WANDQUAD", 1)
{ 
    Scope (\_SB_)
    {
        //
        // Description: Processor#0
        //

        Device (CPU0)
        {
            Name (_HID, "ACPI0007")
            Name (_UID, 0x0)
            Method (_STA)
            {
                Return(0xf)
            }
        }

        //
        // Description: Processor#1
        //

        Device (CPU1)
        {
            Name (_HID, "ACPI0007")
            Name (_UID, 0x1)
            Method (_STA)
            {
                Return(0xf)
            }
        }

        //
        // Description: Processor#2
        //

        Device (CPU2)
        {
            Name (_HID, "ACPI0007")
            Name (_UID, 0x2)
            Method (_STA)
            {
                Return(0xf)
            }
        }

        //
        // Description: Processor#3
        //

        Device (CPU3)
        {
            Name (_HID, "ACPI0007")
            Name (_UID, 0x3)
            Method (_STA)
            {
                Return(0xf)
            }
        }

        //
        // Description: Timers HAL extension
        //

        Device (EPIT)
        {
            Name (_HID, "FSCL0001")
            Name (_UID, 0x0)
            Method (_STA)
            {
                Return(0xf)
            }
        }

        //
        // Description: EHCI USB Controller
        //

        Device (USB2)
        {
            Name (_CID, "PNP0D20")
            Name (_UID, 0x0)
            Name (_HID, "FSCL0010")
            Name (_S0W, 0x3)
            Method (_STA)
            {
                Return(0xf)
            }
            Method (_CRS, 0x0, NotSerialized) {
                Name (RBUF, ResourceTemplate () {
                    MEMORY32FIXED(ReadWrite, 0x02184300, 0x3D00)
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 75 }
                })
                Return(RBUF)
            }

            OperationRegion (OTGM, SystemMemory, 0x02184300, 0x3D00 )
            Field (OTGM, WordAcc, NoLock, Preserve)
            {
                Offset(0x84),   // skip to register 84h
                PTSC, 32,       // port status control
                Offset(0xA8),   // skip to register A8h
                DSBM, 32,       // UOG_USBMOD
            }

            Method (_UBF, 0x0, NotSerialized) 
            {
                Name(REG, Zero)
                Store(0x03, DSBM);      // set host mode & little endian
                Store(PTSC, REG);       // read PORTSC status
                Store(OR(REG,0x2),PTSC);// clear current PORTSC status
            }
        }
    }
} 
