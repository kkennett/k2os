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

#ifndef _UDOOQUAD_ACPIDEFS_H
#define _UDOOQUAD_ACPIDEFS_H

#include <IndustryStandard/Acpi50.h>

#define EFI_ACPI_OEM_ID                         {'K','2','_','_','_','_'}   // OEMID 6 bytes long
#define EFI_ACPI_OEM_TABLE_ID                   SIGNATURE_64('U','D','O','O','Q','U','A','D') // OEM table id 8 bytes long
#define EFI_ACPI_OEM_REVISION                   0x00000001
#define EFI_ACPI_CREATOR_ID                     SIGNATURE_32('K','2','_','_')
#define EFI_ACPI_CREATOR_REVISION               0x00000001

#define EFI_ACPI_VENDOR_ID                      SIGNATURE_32('K','2','_','_')
#define EFI_ACPI_CSRT_REVISION	                0x00000005

#define EFI_ACPI_5_0_CSRT_REVISION              0x00000000

//
// Resource Descriptor Types
//

#define EFI_ACPI_CSRT_RD_TYPE_INTERRUPT 1
#define EFI_ACPI_CSRT_RD_TYPE_TIMER 2
#define EFI_ACPI_CSRT_RD_TYPE_DMA 3
#define EFI_ACPI_CSRT_RD_TYPE_CACHE 4

//
// Resource Descriptor Subtypes
//

#define EFI_ACPI_CSRT_RD_SUBTYPE_INTERRUPT_LINES 0
#define EFI_ACPI_CSRT_RD_SUBTYPE_INTERRUPT_CONTROLLER 1
#define EFI_ACPI_CSRT_RD_SUBTYPE_TIMER 0
#define EFI_ACPI_CSRT_RD_SUBTYPE_DMACHANNEL 0
#define EFI_ACPI_CSRT_RD_SUBTYPE_DMACONTROLLER 1
#define EFI_ACPI_CSRT_RD_SUBTYPE_CACHE 0



//------------------------------------------------------------------------
// CSRT Resource Group header 24 bytes long
//------------------------------------------------------------------------
typedef struct
{
	UINT32 Length;			    // Length
	UINT32 VendorID;		    // 4 bytes
	UINT32 SubVendorId;		    // 4 bytes
	UINT16 DeviceId;		    // 2 bytes
	UINT16 SubdeviceId;		    // 2 bytes
	UINT16 Revision;		    // 2 bytes
	UINT16 Reserved;		    // 2 bytes
	UINT32 SharedInfoLength;	// 4 bytes
} __attribute__((packed)) EFI_ACPI_5_0_CSRT_RESOURCE_GROUP_HEADER;

//------------------------------------------------------------------------
// CSRT Resource Descriptor 12 bytes total
//------------------------------------------------------------------------
typedef struct
{
	UINT32 Length;			// 4 bytes
	UINT16 ResourceType;	// 2 bytes
	UINT16 ResourceSubType;	// 2 bytes
	UINT32 UID;				// 4 bytes    
} __attribute__((packed)) EFI_ACPI_5_0_CSRT_RESOURCE_DESCRIPTOR_HEADER;


#endif // _UDOOQUAD_ACPIDEFS_H
