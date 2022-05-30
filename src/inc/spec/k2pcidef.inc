/*   
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
*/

#ifndef __SPEC_PCIDEF_INC
#define __SPEC_PCIDEF_INC

#define PCI_MAX_WORD_COUNT          64
#define PCI_MAX_FUNCTION_COUNT      8
#define PCI_MAX_DEVICE_COUNT        32
#define PCI_MAX_BUS_COUNT           256

#define PCI_BAR_TYPE_IO_BIT         0x00000001
#define PCI_BAR_BASEMASK_MEMORY     0xFFFFFFF0
#define PCI_BAR_BASEMASK_IO         0xFFFFFFFC
#define PCI_BAR_MEMORY_TYPE_MASK    0x00000006
#define PCI_BAR_MEMORY_TYPE_SHIFT   1
#define PCI_BAR_MEMORY_PREF_BIT     0x00000008

#define PCI_CLASS_UKNOWN            0x00
#define PCI_CLASS_STORAGE           0x01
#define PCI_CLASS_NETWORK           0x02
#define PCI_CLASS_DISPLAY           0x03
#define PCI_CLASS_MULTIMEDIA        0x04
#define PCI_CLASS_MEMORY            0x05
#define PCI_CLASS_BRIDGE            0x06
#define PCI_CLASS_SIMPLEPORT        0x07
#define PCI_CLASS_SERIALBUS         0x0C

#define PCI_DISPLAY_SUBCLASS_VGA    0x00

#define PCI_STORAGE_SUBCLASS_SCSI   0x00
#define PCI_STORAGE_SUBCLASS_IDE    0x01
#define PCI_STORAGE_SUBCLASS_FLOPPY 0x02

#define PCI_NETWORK_SUBCLASS_ETHER  0x00

#define PCI_BRIDGE_SUBCLASS_HOST    0x00
#define PCI_BRIDGE_SUBCLASS_ISA     0x01
#define PCI_BRIDGE_SUBCLASS_EISA    0x02
#define PCI_BRIDGE_SUBCLASS_PCI     0x04
#define PCI_BRIDGE_SUBCLASS_PCMCIA  0x05
#define PCI_BRIDGE_SUBCLASS_CARDBUS 0x07

#endif // __SPEC_PCIDEF_INC

