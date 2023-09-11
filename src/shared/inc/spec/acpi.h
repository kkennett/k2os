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
#ifndef __SPEC_ACPI_H
#define __SPEC_ACPI_H

//
// --------------------------------------------------------------------------------- 
//

#include <k2basetype.h>

#ifdef __cplusplus
extern "C" {
#endif

K2_PACKED_PUSH
struct _ACPI_COMMON_HEADER 
{
    UINT32  Signature;
    UINT32  Length;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_COMMON_HEADER ACPI_COMMON_HEADER;

K2_PACKED_PUSH
struct _ACPI_DESC_HEADER 
{
    UINT32  Signature;
    UINT32  Length;
    UINT8   Revision;
    UINT8   Checksum;
    UINT8   OemId[6];
    UINT64  OemTableId;
    UINT32  OemRevision;
    UINT32  CreatorId;
    UINT32  CreatorRevision;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_DESC_HEADER ACPI_DESC_HEADER;

K2_PACKED_PUSH
struct _ACPI_GAS
{
    UINT8   AddressSpaceId;
    UINT8   RegisterBitWidth;
    UINT8   RegisterBitOffset;
    UINT8   AccessSize;
    UINT64  Address;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_GAS ACPI_GAS;

#define ACPI_ASID_SYSTEM_MEMORY                     0
#define ACPI_ASID_SYSTEM_IO                         1
#define ACPI_ASID_PCI_CONFIGURATION_SPACE           2
#define ACPI_ASID_EMBEDDED_CONTROLLER               3
#define ACPI_ASID_SMBUS                             4
#define ACPI_ASID_PLATFORM_COMMUNICATION_CHANNEL    0x0A
#define ACPI_ASID_FUNCTIONAL_FIXED_HARDWARE         0x7F

#define ACPI_ASIZE_UNDEFINED  0
#define ACPI_ASIZE_BYTE       1
#define ACPI_ASIZE_WORD       2
#define ACPI_ASIZE_DWORD      3
#define ACPI_ASIZE_QWORD      4

K2_PACKED_PUSH
struct _ACPI_RSDP 
{
    UINT64  Signature;
    UINT8   Checksum;
    UINT8   OemId[6];
    UINT8   Revision;
    UINT32  RsdtAddress;
    UINT32  Length;
    UINT64  XsdtAddress;
    UINT8   ExtendedChecksum;
    UINT8   Reserved[3];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_RSDP ACPI_RSDP;

#define ACPI_FADT_SIGNATURE  0x50434146  // FACP

//
// ACPI PMTimer runs at 3579545 Hz
//
// this is 0.00000027936511484001458285899464876123 s per tick
// which is 0.27936511484001458285899464876123 microseconds per tick
// which is 279365115 femptoseconds per tick
// which is 0x10A6C5FB femptoseconds per tick
//
#define PMTMR_FEMPTO_PER_TICK       0x10A6C5FB
#define PMTMR_FEMPTO_1G_SCALED      0x11E11E38  // stall ticks to wait is ((((UINT64)microseconds) << 30) / scaled)

K2_PACKED_PUSH
struct _ACPI_FADT 
{
    ACPI_DESC_HEADER    Header;
    UINT32              FirmwareCtrl;
    UINT32              Dsdt;
    UINT8               Reserved0;
    UINT8               PreferredPmProfile;
    UINT16              SciInt;
    UINT32              SmiCmd;
    UINT8               AcpiEnable;
    UINT8               AcpiDisable;
    UINT8               S4BiosReq;
    UINT8               PstateCnt;
    UINT32              Pm1aEvtBlk;
    UINT32              Pm1bEvtBlk;
    UINT32              Pm1aCntBlk;
    UINT32              Pm1bCntBlk;
    UINT32              Pm2CntBlk;
    UINT32              PmTmrBlk;
    UINT32              Gpe0Blk;
    UINT32              Gpe1Blk;
    UINT8               Pm1EvtLen;
    UINT8               Pm1CntLen;
    UINT8               Pm2CntLen;
    UINT8               PmTmrLen;
    UINT8               Gpe0BlkLen;
    UINT8               Gpe1BlkLen;
    UINT8               Gpe1Base;
    UINT8               CstCnt;
    UINT16              PLvl2Lat;
    UINT16              PLvl3Lat;
    UINT16              FlushSize;
    UINT16              FlushStride;
    UINT8               DutyOffset;
    UINT8               DutyWidth;
    UINT8               DayAlrm;
    UINT8               MonAlrm;
    UINT8               Century;
    UINT16              IaPcBootArch;
    UINT8               Reserved1;
    UINT32              Flags;
    ACPI_GAS            ResetReg;
    UINT8               ResetValue;
    UINT8               Reserved2[3];
    UINT64              XFirmwareCtrl;
    UINT64              XDsdt;
    ACPI_GAS            XPm1aEvtBlk;
    ACPI_GAS            XPm1bEvtBlk;
    ACPI_GAS            XPm1aCntBlk;
    ACPI_GAS            XPm1bCntBlk;
    ACPI_GAS            XPm2CntBlk;
    ACPI_GAS            XPmTmrBlk;
    ACPI_GAS            XGpe0Blk;
    ACPI_GAS            XGpe1Blk;
    ACPI_GAS            SleepControlReg;
    ACPI_GAS            SleepStatusReg;
}  K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_FADT ACPI_FADT;

#define ACPI_FACS_SIGNATURE  0x53434146  // FACS
K2_PACKED_PUSH
struct _ACPI_FACS
{
    UINT32  Signature;
    UINT32  Length;
    UINT32  HardwareSignature;
    UINT32  FirmwareWakingVector;
    UINT32  GlobalLock;
    UINT32  Flags;
    UINT64  XFirmwareWakingVector;
    UINT8   Version;
    UINT8   Reserved0[3];
    UINT32  OspmFlags;
    UINT8   Reserved1[24];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_FACS ACPI_FACS;

#define ACPI_MADT_SIGNATURE 0x43495041  // APIC
K2_PACKED_PUSH
struct _ACPI_MADT
{
    ACPI_DESC_HEADER    Header;
    UINT32              LocalApicAddress;
    UINT32              Flags;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT ACPI_MADT;

#define ACPI_MADT_FLAGS_X32_DUAL_8259_PRESENT               1

#define ACPI_MADT_SUB_TYPE_PROCESSOR_LOCAL_APIC             0x00
#define ACPI_MADT_SUB_TYPE_IO_APIC                          0x01
#define ACPI_MADT_SUB_TYPE_INTERRUPT_SOURCE_OVERRIDE        0x02
#define ACPI_MADT_SUB_TYPE_NON_MASKABLE_INTERRUPT_SOURCE    0x03
#define ACPI_MADT_SUB_TYPE_LOCAL_APIC_NMI                   0x04
#define ACPI_MADT_SUB_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE      0x05
#define ACPI_MADT_SUB_TYPE_IO_SAPIC                         0x06
#define ACPI_MADT_SUB_TYPE_LOCAL_SAPIC                      0x07
#define ACPI_MADT_SUB_TYPE_PLATFORM_INTERRUPT_SOURCES       0x08
#define ACPI_MADT_SUB_TYPE_PROCESSOR_LOCAL_X2APIC           0x09
#define ACPI_MADT_SUB_TYPE_LOCAL_X2APIC_NMI                 0x0A
#define ACPI_MADT_SUB_TYPE_GIC                              0x0B
#define ACPI_MADT_SUB_TYPE_GICD                             0x0C

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_PROCESSOR_LOCAL_APIC 
{
    UINT8   Type;
    UINT8   Length;
    UINT8   AcpiProcessorId;
    UINT8   ApicId;
    UINT32  Flags;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_PROCESSOR_LOCAL_APIC ACPI_MADT_SUB_PROCESSOR_LOCAL_APIC;

#define ACPI_MADT_SUB_LOCAL_APIC_ENABLED        0x00000001

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_IO_APIC
{
    UINT8   Type;
    UINT8   Length;
    UINT8   IoApicId;
    UINT8   Reserved;
    UINT32  IoApicAddress;
    UINT32  GlobalSystemIrqLineBase;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_IO_APIC ACPI_MADT_SUB_IO_APIC;

#define ACPI_APIC_INTR_FLAG_POLARITY_MASK           0x0003
#define ACPI_APIC_INTR_FLAG_POLARITY_BUS            0x0000
#define ACPI_APIC_INTR_FLAG_POLARITY_ACTIVE_HIGH    0x0001
#define ACPI_APIC_INTR_FLAG_POLARITY_ACTIVE_LOW     0x0003

#define ACPI_APIC_INTR_FLAG_TRIGGER_MASK            0x000C
#define ACPI_APIC_INTR_FLAG_TRIGGER_BUS             0x0000
#define ACPI_APIC_INTR_FLAG_TRIGGER_EDGE            0x0004
#define ACPI_APIC_INTR_FLAG_TRIGGER_LEVEL           0x000C

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_INTERRUPT_SOURCE_OVERRIDE
{
    UINT8   Type;
    UINT8   Length;
    UINT8   Bus;
    UINT8   Source;
    UINT32  GlobalSystemInterrupt;
    UINT16  Flags;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_INTERRUPT_SOURCE_OVERRIDE ACPI_MADT_SUB_INTERRUPT_SOURCE_OVERRIDE;

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_PLATFORM_INTERRUPT_APIC
{
    UINT8   Type;
    UINT8   Length;
    UINT16  Flags;
    UINT8   InterruptType;
    UINT8   ProcessorId;
    UINT8   ProcessorEid;
    UINT8   IoSapicVector;
    UINT32  GlobalSystemInterrupt;
    UINT32  PlatformInterruptSourceFlags;
    UINT8   CpeiProcessorOverride;
    UINT8   Reserved[31];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_PLATFORM_INTERRUPT_APIC ACPI_MADT_SUB_PLATFORM_INTERRUPT_APIC;

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_NON_MASKABLE_INTERRUPT_SOURCE
{
    UINT8   Type;
    UINT8   Length;
    UINT16  Flags;
    UINT32  GlobalSystemInterrupt;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_NON_MASKABLE_INTERRUPT_SOURCE ACPI_MADT_SUB_NON_MASKABLE_INTERRUPT_SOURCE;

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_LOCAL_APIC_NMI
{
    UINT8   Type;
    UINT8   Length;
    UINT8   AcpiProcessorId;
    UINT16  Flags;
    UINT8   LocalApicLint;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_LOCAL_APIC_NMI ACPI_MADT_SUB_LOCAL_APIC_NMI;

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_LOCAL_APIC_ADDRESS_OVERRIDE
{
    UINT8   Type;
    UINT8   Length;
    UINT16  Reserved;
    UINT64  LocalApicAddress;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_LOCAL_APIC_ADDRESS_OVERRIDE ACPI_MADT_SUB_LOCAL_APIC_ADDRESS_OVERRIDE;

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_IO_SAPIC
{
    UINT8   Type;
    UINT8   Length;
    UINT8   IoApicId;
    UINT8   Reserved;
    UINT32  GlobalSystemIrqLineBase;
    UINT64  IoSapicAddress;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_IO_SAPIC ACPI_MADT_SUB_IO_SAPIC;

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_PROCESSOR_LOCAL_SAPIC
{
    UINT8   Type;
    UINT8   Length;
    UINT8   AcpiProcessorId;
    UINT8   LocalSapicId;
    UINT8   LocalSapicEid;
    UINT8   Reserved[3];
    UINT32  Flags;
    UINT32  ACPIProcessorUIDValue;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_PROCESSOR_LOCAL_SAPIC ACPI_MADT_SUB_PROCESSOR_LOCAL_SAPIC;

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_PLATFORM_INTERRUPT_SOURCES
{
    UINT8   Type;
    UINT8   Length;
    UINT16  Flags;
    UINT8   InterruptType;
    UINT8   ProcessorId;
    UINT8   ProcessorEid;
    UINT8   IoSapicVector;
    UINT32  GlobalSystemInterrupt;
    UINT32  PlatformInterruptSourceFlags;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_PLATFORM_INTERRUPT_SOURCES ACPI_MADT_SUB_PLATFORM_INTERRUPT_SOURCES;

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_PROCESSOR_LOCAL_X2APIC
{
    UINT8   Type;
    UINT8   Length;
    UINT8   Reserved[2];
    UINT32  X2ApicId;
    UINT32  Flags;
    UINT32  AcpiProcessorUid;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_PROCESSOR_LOCAL_X2APIC ACPI_MADT_SUB_PROCESSOR_LOCAL_X2APIC;

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_LOCAL_X2APIC_NMI
{
    UINT8   Type;
    UINT8   Length;
    UINT16  Flags;
    UINT32  AcpiProcessorUid;
    UINT8   LocalX2ApicLint;
    UINT8   Reserved[3];
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_LOCAL_X2APIC_NMI ACPI_MADT_SUB_LOCAL_X2APIC_NMI;

#define ACPI_DSDT_SIGNATURE  0x54445344  // DSDT

//
// HPET register definitions
//
#define HPET_OFFSET_GCID                0x0000
#define HPET_OFFSET_GCID_PERIOD32       0x0004
#define HPET_OFFSET_CONFIG              0x0010
#define HPET_OFFSET_GISR                0x0020
#define HPET_OFFSET_COUNT_LO32          0x00F0
#define HPET_OFFSET_COUNT_HI32          0x00F4
#define HPET_OFFSET_TIMER0              0x0100
#define HPET_TIMER_SPACE                0x0020

#define HPET_TIMER_OFFSET_CONFIG        0
#define HPET_TIMER_OFFSET_ROUTE_CAP     4
#define HPET_TIMER_OFFSET_COMPARE       8
#define HPET_TIMER_OFFSET_ROUTE         16

#define HPET_OFFSET_TIMER0_CONFIG       (HPET_OFFSET_TIMER0 + HPET_TIMER_OFFSET_CONFIG)
#define HPET_OFFSET_TIMER0_ROUTE_CAP    (HPET_OFFSET_TIMER0 + HPET_TIMER_OFFSET_ROUTE_CAP)
#define HPET_OFFSET_TIMER0_COMPARE      (HPET_OFFSET_TIMER0 + HPET_TIMER_OFFSET_COMPARE)
#define HPET_OFFSET_TIMER0_ROUTE        (HPET_OFFSET_TIMER0 + HPET_TIMER_OFFSET_ROUTE)

#define HPET_GCID_LO_REV_ID             0x000000FF
#define HPET_GCID_LO_NUM_TIM_CAP        0x00001F00
#define HPET_GCID_LO_NUM_TIM_CAP_SHL    8
#define HPET_GCID_LO_COUNT_SIZE_CAP     0x00002000
#define HPET_GCID_LO_RESERVED           0x00004000
#define HPET_GCID_LO_LEG_ROUTE_CAP      0x00008000
#define HPET_GCID_LO_VENDOR_ID          0xFFFF0000
#define HPET_GCID_LO_VENDOR_ID_SHL      16

#define HPET_CONFIG_ENABLE_CNF          0x00000001
#define HPET_CONFIG_LEG_RT_CNF          0x00000002
#define HPET_CONFIG_RESERVED            0xFFFFFFFC

#define HPET_GISR_T0_INT_STS            0x00000001
#define HPET_GISR_T1_INT_STS            0x00000002
#define HPET_GISR_T2_INT_STS            0x00000004
#define HPET_GISR_RESERVED              0xFFFFFFF8

#define HPET_TIMER_CONFIG_LO_RESERVED0  0x00000001
#define HPET_TIMER_CONFIG_LO_INT_TYPE   0x00000002
#define HPET_TIMER_CONFIG_LO_INT_ENB    0x00000004
#define HPET_TIMER_CONFIG_LO_TYPE       0x00000008
#define HPET_TIMER_CONFIG_LO_PER_INT    0x00000010
#define HPET_TIMER_CONFIG_LO_SIZE       0x00000020
#define HPET_TIMER_CONFIG_LO_VAL_SET    0x00000040
#define HPET_TIMER_CONFIG_LO_RESERVED7  0x00000080
#define HPET_TIMER_CONFIG_LO_32MODE     0x00000100
#define HPET_TIMER_CONFIG_LO_INT_ROUTE  0x00003E00
#define HPET_TIMER_CONFIG_LO_INT_ROUTE_SHL       9
#define HPET_TIMER_CONFIG_LO_FSB_EN     0x00004000
#define HPET_TIMER_CONFIG_LO_FSB_INT    0x00008000
#define HPET_TIMER_CONFIG_LO_RESERVED16 0xFFFF0000

K2_PACKED_PUSH
struct _ACPI_HPET
{
    ACPI_DESC_HEADER    Header;
    UINT32              Id;
    ACPI_GAS            Address;
    UINT8               Sequence;
    UINT16              MinimumTick;
    UINT8               Flags;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_HPET ACPI_HPET;

#define ACPI_HPET_PAGE_PROTECT_MASK (3)

#define ACPI_HPET_FLAGS_PAGE_PROTECT_MASK       0x00000003
#define ACPI_HPET_FLAGS_PAGE_PROTECT_NONE       0x00000000
#define ACPI_HPET_FLAGS_PAGE_PROTECT_4K         0x00000001
#define ACPI_HPET_FLAGS_PAGE_PROTECT_64K        0x00000002

#define ACPI_HPET_SIGNATURE  0x54455048  // HPET

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_GIC
{
    UINT8   Type;
    UINT8   Length;
    UINT16  Reserved;
    UINT32  GicId;
    UINT32  AcpiProcessorUid;
    UINT32  Flags;
    UINT32  ParkingProtocolVersion;
    UINT32  PerformanceInterruptGsiv;
    UINT64  ParkedAddress;
    UINT64  PhysicalBaseAddress;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_GIC ACPI_MADT_SUB_GIC;

#define ACPI_MADT_SUB_GIC_FLAGS_ENABLED                         0x00000001
#define ACPI_MADT_SUB_GIC_FLAGS_PERFORMANCE_INTERRUPT_MODEL     0x00000002

K2_PACKED_PUSH
struct _ACPI_MADT_SUB_GIC_DISTRIBUTOR
{
    UINT8   Type;
    UINT8   Length;
    UINT16  Reserved1;
    UINT32  GicId;
    UINT64  PhysicalBaseAddress;
    UINT32  SystemVectorBase;
    UINT32  Reserved2;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_MADT_SUB_GIC_DISTRIBUTOR ACPI_MADT_SUB_GIC_DISTRIBUTOR;

#define ACPI_CSRT_SIGNATURE  0x54525343  // CSRT

//
// CSRT Resource Descriptor Types
//
#define ACPI_CSRT_RD_TYPE_INTERRUPT 1
#define ACPI_CSRT_RD_TYPE_TIMER 2
#define ACPI_CSRT_RD_TYPE_DMA 3
#define ACPI_CSRT_RD_TYPE_CACHE 4

//
// CSRT Resource Descriptor Subtypes
//
#define ACPI_CSRT_RD_SUBTYPE_INTERRUPT_LINES 0
#define ACPI_CSRT_RD_SUBTYPE_INTERRUPT_CONTROLLER 1
#define ACPI_CSRT_RD_SUBTYPE_TIMER 0
#define ACPI_CSRT_RD_SUBTYPE_DMACHANNEL 0
#define ACPI_CSRT_RD_SUBTYPE_DMACONTROLLER 1
#define ACPI_CSRT_RD_SUBTYPE_CACHE 0

K2_PACKED_PUSH
struct _ACPI_CSRT_RESOURCE_GROUP_HEADER
{
    UINT32 Length;               // Length
    UINT32 VendorID;             // 4 bytes
    UINT32 SubVendorId;          // 4 bytes
    UINT16 DeviceId;             // 2 bytes
    UINT16 SubdeviceId;          // 2 bytes
    UINT16 Revision;             // 2 bytes
    UINT16 Reserved;             // 2 bytes
    UINT32 SharedInfoLength;     // 4 bytes
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_CSRT_RESOURCE_GROUP_HEADER ACPI_CSRT_RESOURCE_GROUP_HEADER;

K2_PACKED_PUSH
struct _ACPI_CSRT_RESOURCE_DESCRIPTOR_HEADER
{
    UINT32 Length;              // 4 bytes
    UINT16 ResourceType;        // 2 bytes
    UINT16 ResourceSubType;     // 2 bytes
    UINT32 UID;                 // 4 bytes    
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_CSRT_RESOURCE_DESCRIPTOR_HEADER ACPI_CSRT_RESOURCE_DESCRIPTOR_HEADER;

#define ACPI_CSRT_TIMER_CAP_ALWAYS_ON     0x00000001 // Timer is always ON.
#define ACPI_CSRT_TIMER_CAP_UP_COUNTER    0x00000002 // Up counter vs. down counter (0)
#define ACPI_CSRT_TIMER_CAP_READABLE      0x00000004 // Counter has a software interface.
#define ACPI_CSRT_TIMER_CAP_PERIODIC      0x00000008 // Can generate periodic interrupts.
#define ACPI_CSRT_TIMER_CAP_DRIVES_IRQ    0x00000010 // Timer interrupt drives a physical IRQ.
#define ACPI_CSRT_TIMER_CAP_ONE_SHOT      0x00000020 // Counter is capable pf generating one-shot interrupts

K2_PACKED_PUSH
struct _ACPI_CSRT_RD_TIMER
{
    ACPI_CSRT_RESOURCE_DESCRIPTOR_HEADER Header;
    UINT32 Capabilities;
    UINT32 Width;
    UINT32 Source;
    UINT32 Frequency;
    UINT32 FrequencyScale;
    UINT32 BaseAddress;
    UINT32 Size;
    UINT32 Interrupt;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_CSRT_RD_TIMER ACPI_CSRT_RD_TIMER;

// BGRT Boot Graphics Resource Table

#define ACPI_BGRT_SIGNATURE  0x54524742  // BGRT

K2_PACKED_PUSH
struct _ACPI_BGRT {
    ACPI_DESC_HEADER    Header;
    UINT16              Version;
    UINT8               Status;
    UINT8               ImageType;
    UINT64              ImageAddress;
    UINT32              ImageOffsetX;
    UINT32              ImageOffsetY;
} K2_PACKED_ATTRIB;
K2_PACKED_POP
typedef struct _ACPI_BGRT ACPI_BGRT;

#define ACPI_BGRT_TABLE_REVISION        1

#define ACPI_BGRT_VERSION               0x01

#define ACPI_BGRT_STATUS_NOT_DISPLAYED  0x00
#define ACPI_BGRT_STATUS_DISPLAYED      0x01

#define ACPI_BGRT_IMAGE_TYPE_BMP        0x00


#ifdef __cplusplus
};  // extern "C"
#endif

/* ------------------------------------------------------------------------- */

#endif  // __SPEC_ACPI_H

